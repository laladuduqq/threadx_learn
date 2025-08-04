/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-01 17:51:42
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-04 19:47:44
 * @FilePath: /threadx_learn/applications/compoent_config.h
 * @Description: 
 */
#ifndef _COMPOENT_CONFIG_H_
#define _COMPOENT_CONFIG_H_

#include <stddef.h>

//bsp组件配置宏定义

//uart配置宏定义
#define UART_MAX_INSTANCE_NUM 3        //可用串口数量
#define UART_RX_DONE_EVENT (0x01 << 0) //接收完成事件
#define UART_DEFAULT_BUF_SIZE 32  // 添加默认最小缓冲区大小定义


//组件配置宏定义

//ulog配置宏定义
#define LOG_ENABLE 1                            //启用日志
#define LOG_LEVEL_INFO ULOG_DEBUG_LEVEL          //日志等级
#define LOG_SETTING_UART huart6                 //使用的串口
#define LOG_COLOR_ENABLE 1                      //启用颜色
#define LOG_ASYNC_ENABLE 0                      //启用异步日志
#define LOG_BUFFER_SIZE 512                     //日志缓冲区大小







//rtos内存分配函数
void* threadx_malloc(size_t size);
void threadx_free(void *ptr);


#endif // _COMPOENT_CONFIG_H_