/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-07-01 13:39:01
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-05 08:17:45
 * @FilePath: /threadx_learn/BSP/UART/bsp_uart.h
 * @Description: 
 */
#ifndef __BSP_UART_H__
#define __BSP_UART_H__


#include "tx_port.h"
#include "usart.h"
#include "tx_api.h"
#include <stdbool.h>

#if defined(USE_COMPOENT_CONFIG_H) && USE_COMPOENT_CONFIG_H
#include "compoent_config.h"
#else
#define UART_MAX_INSTANCE_NUM 3        //可用串口数量
#define UART_RX_DONE_EVENT (0x01 << 0) //接收完成事件
#define UART_DEFAULT_BUF_SIZE 32  // 添加默认最小缓冲区大小定义
#endif // _COMPOENT_CONFIG_H_

// 模式选择
typedef enum {
    UART_MODE_BLOCKING,
    UART_MODE_IT,
    UART_MODE_DMA
} UART_Mode;

// 回调触发方式
typedef enum {
    UART_CALLBACK_DIRECT,
    UART_CALLBACK_EVENT
} UART_CallbackType;

// UART设备结构体
typedef struct {
    UART_HandleTypeDef *huart;
    
    // 接收相关
    uint8_t (*rx_buf)[2];    // 指向外部定义的双缓冲区
    uint16_t rx_buf_size;    // 缓冲区大小
    volatile uint8_t rx_active_buf;  // 当前活动缓冲区
    uint16_t rx_len;         // 接收数据长度
    uint16_t expected_len;   // 预期长度（0为不定长）

    //发送相关
    TX_SEMAPHORE tx_sem;
    
    // 回调相关
    void (*rx_complete_cb)(uint8_t *data, uint16_t len);
    TX_EVENT_FLAGS_GROUP rx_event;
    ULONG event_flag;
    UART_CallbackType cb_type;
    
    // 配置参数
    UART_Mode rx_mode;
    UART_Mode tx_mode;
} UART_Device;

// 初始化配置结构体
typedef struct {
    UART_HandleTypeDef *huart;
    uint8_t (*rx_buf)[2];    // 外部缓冲区指针
    uint16_t rx_buf_size;    // 缓冲区大小
    uint16_t expected_len;
    UART_Mode rx_mode;
    UART_Mode tx_mode;
    uint32_t timeout;
    void (*rx_complete_cb)(uint8_t *data, uint16_t len);
    UART_CallbackType cb_type;
    uint32_t event_flag;
} UART_Device_init_config;

/**
 * @description: UART初始化函数
 * @details      初始化UART设备，配置缓冲区和回调函数
 * @param        config：UART_Device_init_config指针
 * @return       UART_Device*：指向初始化后的UART_Device结构体指针
 */
UART_Device* BSP_UART_Init(UART_Device_init_config *config);
/**
 * @description: UART发送函数
 * @details      发送数据到UART设备
 * @param        inst：UART_Device指针
 * @param        data：要发送的数据指针
 * @param        len：数据长度
 * @return       HAL_StatusTypeDef：HAL状态码
 */
HAL_StatusTypeDef BSP_UART_Send(UART_Device *inst, uint8_t *data, uint16_t len);
/**
 * @description: UART反初始化函数
 * @details      释放UART设备资源，删除事件标志组和信号量
 * @param        inst：UART_Device指针
 * @return {*}
 */
void BSP_UART_Deinit(UART_Device *inst);
/**
 * @brief  获取UART接收缓冲区的数据指针和长度
 * @param  inst: UART_Device指针
 * @param  data: 指向非活动缓冲区指针的指针，返回数据区地址
 * @retval 实际接收的字节数
 */
int BSP_UART_Read(UART_Device *inst, uint8_t **data);

#endif /* __BSP_UART_H__ */
