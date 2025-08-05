/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-04 17:25:01
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-05 08:46:54
 * @FilePath: /threadx_learn/tools/ulog/ulog_port.c
 * @Description: 
 */
#include "ulog_port.h"
#include "bsp_uart.h"
#include "compoent_config.h"
#include "dwt.h"
#include "tx_api.h"
#include "tx_port.h"
#include "ulog.h"
#include <stdio.h>

// 声明UART设备指针
static UART_Device* log_uart = NULL;

#if LOG_ASYNC_ENABLE
// 异步日志相关定义
#define LOG_QUEUE_SIZE 16
static TX_QUEUE log_queue;
static TX_THREAD log_thread;
static char log_queue_buffer[LOG_QUEUE_SIZE * sizeof(char*)];
// 直接使用静态分配的缓冲区数组，而不是指针数组
static char log_buffer_pool[LOG_QUEUE_SIZE][LOG_BUFFER_SIZE];
static uint8_t buffer_in_use[LOG_QUEUE_SIZE] = {0}; // 标记缓冲区使用状态
// 日志线程函数声明
static void log_thread_entry(ULONG thread_input);
static char* get_free_buffer(void);
static void release_buffer(char* buffer);
#else
// 声明互斥锁，用于保护日志输出
static TX_MUTEX log_mutex;
#endif

// ANSI颜色代码定义
#define COLOR_RESET     "\033[0m"
#define COLOR_RED       "\033[31m"
#define COLOR_GREEN     "\033[32m"
#define COLOR_YELLOW    "\033[33m"
#define COLOR_BLUE      "\033[34m"
#define COLOR_MAGENTA   "\033[35m"
#define COLOR_CYAN      "\033[36m"
#define COLOR_WHITE     "\033[37m"

// 根据日志级别获取颜色
static const char* get_color_by_level(ulog_level_t level) {
    switch (level) {
        case ULOG_CRITICAL_LEVEL:
        case ULOG_ERROR_LEVEL:
            return COLOR_RED;
        case ULOG_WARNING_LEVEL:
            return COLOR_YELLOW;
        case ULOG_INFO_LEVEL:
            return COLOR_GREEN;
        case ULOG_DEBUG_LEVEL:
            return COLOR_BLUE;
        case ULOG_TRACE_LEVEL:
            return COLOR_CYAN;
        default:
            return COLOR_WHITE;
    }
}

// // 使用ThreadX API实现时间戳函数
// static char* get_threadx_timestamp(void) {
//     static char timestamp_str[32];
//     ULONG ticks = tx_time_get();
//     // 假设系统时钟节拍率为1000Hz，即1tick=1ms
//     // 可根据实际配置调整
//     snprintf(timestamp_str, sizeof(timestamp_str), "[%lu]", ticks);
//     return timestamp_str;
// }

// 使用DWT实现时间戳函数
static char* get_dwt_timestamp(void) {
    static char timestamp_str[32];
    uint64_t us = DWT_GetTimeline_us();
    // 格式化为 [秒.微秒] 格式，例如 [123.456789]
    snprintf(timestamp_str, sizeof(timestamp_str), "[%lu.%06lu]", 
             (unsigned long)(us / 1000000),
             (unsigned long)(us % 1000000));
    return timestamp_str;
}


#if LOG_ASYNC_ENABLE
// 获取一个空闲的缓冲区
static char* get_free_buffer(void) {
    for (int i = 0; i < LOG_QUEUE_SIZE; i++) {
        if (!buffer_in_use[i]) {
            buffer_in_use[i] = 1;
            return log_buffer_pool[i];
        }
    }
    return NULL; // 没有空闲缓冲区
}

// 释放缓冲区
static void release_buffer(char* buffer) {
    // 通过指针计算确定缓冲区索引
    if (buffer >= (char*)log_buffer_pool && 
        buffer < (char*)(log_buffer_pool + LOG_QUEUE_SIZE)) {
        int index = (buffer - (char*)log_buffer_pool) / LOG_BUFFER_SIZE;
        if (index >= 0 && index < LOG_QUEUE_SIZE) {
            buffer_in_use[index] = 0;
        }
    }
}

// 异步日志线程入口函数
static void log_thread_entry(ULONG thread_input) {
    char* log_msg;
    while (1) {
        // 从队列中获取日志消息
        if (tx_queue_receive(&log_queue, &log_msg, TX_WAIT_FOREVER) == TX_SUCCESS) {
            // 实际输出日志
            if (log_uart != NULL && log_msg != NULL) {
                UART_Send(log_uart, (uint8_t*)log_msg, (uint16_t)strlen(log_msg));
                // 输出完成后释放缓冲区
                release_buffer(log_msg);
            }
        }
    }
}

// 异步日志输出函数
static void async_console_logger(ulog_level_t level, char* msg) {
    char* log_buffer = get_free_buffer();
    
    if (log_buffer == NULL) {
        // 没有空闲缓冲区，直接返回，丢弃日志
        return;
    }

#ifdef LOG_COLOR_ENABLE
    // 带颜色的日志输出，添加额外的回车符号
    const char* color = get_color_by_level(level);
    snprintf(log_buffer, LOG_BUFFER_SIZE, "%s%s [%s]: %s%s\r\n", 
             color,
             get_dwt_timestamp(), 
             ulog_level_name(level), 
             msg,
             COLOR_RESET);
#else
    // 不带颜色的日志输出，添加额外的回车符号
    snprintf(log_buffer, LOG_BUFFER_SIZE, "%s [%s]: %s\r\n", 
             get_dwt_timestamp(), 
             ulog_level_name(level), 
             msg);
#endif

    // 将日志消息发送到队列
    if (tx_queue_send(&log_queue, &log_buffer, TX_NO_WAIT) != TX_SUCCESS) {
        // 队列满，释放缓冲区并返回
        release_buffer(log_buffer);
    }
}
#endif

void my_console_logger(ulog_level_t level,char *msg) {
#if LOG_ASYNC_ENABLE
    // 使用异步日志输出
    async_console_logger(level, msg);
    return;
#else
    static char log_buffer[512]; // 静态缓冲区
    int len;
    
    // 获取互斥锁
    tx_mutex_get(&log_mutex, TX_WAIT_FOREVER);
    
#ifdef LOG_COLOR_ENABLE
    // 带颜色的日志输出，添加额外的回车符号
    const char* color = get_color_by_level(level);
    len = snprintf(log_buffer, sizeof(log_buffer), "%s%s [%s]: %s%s\r\n", 
                  color,
                  get_dwt_timestamp(), 
                  ulog_level_name(level), 
                  msg,
                  COLOR_RESET);
#else
    // 不带颜色的日志输出，添加额外的回车符号
    len = snprintf(log_buffer, sizeof(log_buffer), "%s [%s]: %s\r\n", 
                  get_dwt_timestamp(), 
                  ulog_level_name(level), 
                  msg);
#endif

    if (len > 0) {
        // 使用UART发送函数输出日志
        if (log_uart != NULL) {
            BSP_UART_Send(log_uart, (uint8_t*)log_buffer, (uint16_t)len);
        } 
    }
    // 释放互斥锁
    tx_mutex_put(&log_mutex);
#endif
}



void ulog_port_init(void)
{
#if LOG_ASYNC_ENABLE
    // 初始化异步日志相关资源
    // 初始化缓冲区使用状态
    memset(buffer_in_use, 0, sizeof(buffer_in_use));
    // 创建队列
    tx_queue_create(&log_queue, "Log Queue", sizeof(char*)/sizeof(ULONG), 
                    log_queue_buffer, sizeof(log_queue_buffer));

    // 用内存池分配监控线程栈
    CHAR *log_thread_stack;
    log_thread_stack = threadx_malloc(1024);

    // 创建日志线程
    tx_thread_create(&log_thread, "Log Thread", log_thread_entry, 0,
                     log_thread_stack, 1024,
                     1, 1, TX_NO_TIME_SLICE, TX_AUTO_START);
#else
    // 初始化互斥锁
    tx_mutex_create(&log_mutex, "Log Mutex", TX_NO_INHERIT);
#endif

    // 初始化UART设备
    // UART配置
    UART_Device_init_config log_uart_cfg = {
        .huart = &LOG_SETTING_UART,  
        .expected_len = 0,
        .rx_buf = NULL,
        .rx_buf_size = 0,
        .rx_mode = UART_MODE_IT,
        #if LOG_ASYNC_ENABLE
        .tx_mode = UART_MODE_DMA,
        #else
        .tx_mode = UART_MODE_BLOCKING,
        #endif
        .timeout = 1000,
        .rx_complete_cb = NULL,
        .cb_type = UART_CALLBACK_EVENT,
        .event_flag = UART_RX_DONE_EVENT,
    };

    log_uart = BSP_UART_Init(&log_uart_cfg);
    ULOG_INIT();
    ULOG_SUBSCRIBE(my_console_logger, LOG_LEVEL_INFO);
}


