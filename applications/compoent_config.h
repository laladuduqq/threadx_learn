/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-01 17:51:42
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-06 10:36:35
 * @FilePath: /threadx_learn/applications/compoent_config.h
 * @Description: 
 */
#ifndef _COMPOENT_CONFIG_H_
#define _COMPOENT_CONFIG_H_

#include <stddef.h>

//tools配置宏定义
//ulog配置宏定义
#define LOG_ENABLE 1                            //启用日志
#define LOG_LEVEL_INFO ULOG_INFO_LEVEL          //日志等级
#define LOG_SETTING_UART huart6                 //使用的串口
#define LOG_COLOR_ENABLE 1                      //启用颜色
#define LOG_ASYNC_ENABLE 0                      //启用异步日志
#if LOG_ASYNC_ENABLE //注意，下面宏定义只有在启用异步日志时才有效
    #define LOG_BUFFER_SIZE 512                 //日志缓冲区大小
    #define LOG_THREAD_STACK_SIZE 1024          //日志线程栈大小
    #define LOG_THREAD_PRIORITY 1               //日志线程优先级    
    #define LOG_ASYNC_BUF_SIZE 512              //异步日志缓冲区大小
#endif


//bsp组件配置宏定义
//uart配置宏定义
#define UART_MAX_INSTANCE_NUM 3        //可用串口数量
#define UART_RX_DONE_EVENT (0x01 << 0) //接收完成事件
#define UART_DEFAULT_BUF_SIZE 32  // 添加默认最小缓冲区大小定义
//spi配置宏定义
#define SPI_BUS_NUM 2                  // 总线数量
#define MAX_DEVICES_PER_BUS 4         // 每条总线最大设备数
//flash配置宏定义
#define USER_FLASH_SECTOR_ADDR    ADDR_FLASH_SECTOR_11
#define USER_FLASH_SECTOR_NUMBER  FLASH_SECTOR_11
//iic配置宏定义
#define I2C_BUS_NUM 2                  // 总线数量
#define MAX_DEVICES_PER_I2C_BUS 4      // 每条总线最大设备数
//pwm配置宏定义
#define MAX_PWM_DEVICES 10             // 最大PWM设备数量
//can配置宏定义
#define CAN_BUS_NUM 2               // 总线数量
#define MAX_CAN_DEVICES_PER_BUS  8  // 每总线最大设备数
#define CAN_SEND_RETRY_CNT  3       // 重试次数
#define CAN_SEND_TIMEOUT_US 100     // 发送超时时间，单位微秒
//adc配置宏定义
#define ADC_DEVICE_NUM 4         // 最大ADC设备数
//gpio外部中断配置宏定义
#define GPIO_EXTI_DEVICE_NUM 16     // 最大GPIO中断回调设备数


//组件配置宏定义
//systemwatch配置宏定义
#define SystemWatch_Enable 1       // 开启系统监控功能,注意下述宏定义只有在开启系统监控功能时才会生效
#if SystemWatch_Enable
    #define MAX_MONITORED_TASKS 10              // 最大监控任务数
    #define TASK_BLOCK_TIMEOUT 1                // 任务阻塞判定时间 (s)
    #define SystemWatch_Reset_Enable 0          // 检测到线程阻塞，是否系统重置开关，0-->删除对应的阻塞线程，1-->重置
    #define SystemWatch_Iwdg_Enable 1           // 开启看门狗功能
    #define SYSTEMWATCH_THREAD_STACK_SIZE 1024  //线程栈大小
    #define SYSTEMWATCH_THREAD_PRIORITY 2       //线程优先级 
#endif  
//beep配置宏定义
// 蜂鸣器配置参数
#define BEEP_PERIOD   2000  // 注意这里的周期，由于在offline task(10ms)中,尽量保证整除
#define BEEP_ON_TIME  100   // BEEP_ON_TIME BEEP_OFF_TIME 共同影响在周期内的最大beep times
#define BEEP_OFF_TIME 100

#define BEEP_TUNE_VALUE 500  // 决定beep的音调
#define BEEP_CTRL_VALUE 100  // 决定beep的音量

#if !defined(OFFLINE_Beep_Enable)
#define OFFLINE_Beep_Enable 1
#endif









//rtos内存分配函数
void* threadx_malloc(size_t size);
void threadx_free(void *ptr);


#endif // _COMPOENT_CONFIG_H_