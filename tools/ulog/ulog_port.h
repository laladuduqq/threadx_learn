/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-04 17:25:16
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-05 09:43:25
 * @FilePath: /threadx_learn/tools/ulog/ulog_port.h
 * @Description: 
 */
#ifndef _ULOG_PORT_H_
#define _ULOG_PORT_H_



#if defined(USE_COMPOENT_CONFIG_H) && USE_COMPOENT_CONFIG_H
#include "compoent_config.h"
#else
#define LOG_ENABLE 0                           //启用日志
#define LOG_LEVEL_INFO ULOG_INFO_LEVEL          //日志等级
#define LOG_SETTING_UART huart6                 //使用的串口
#define LOG_COLOR_ENABLE 1                      //启用颜色
#define LOG_ASYNC_ENABLE 1                      //启用异步日志
#define LOG_BUFFER_SIZE 512                     //日志缓冲区大小
#define LOG_THREAD_STACK_SIZE 1024              //日志线程栈大小
#define LOG_THREAD_PRIORITY 1                   //日志线程优先级    
#endif // _COMPOENT_CONFIG_H_

// 默认TAG定义，如果用户没有定义则使用默认值
#ifndef LOG_TAG
#define LOG_TAG "Default"
#endif

// 带默认TAG的日志宏定义
#define ULOG_TAG_TRACE(fmt, ...)    ULOG_TRACE("[%s] " fmt, LOG_TAG, ##__VA_ARGS__)
#define ULOG_TAG_DEBUG(fmt, ...)    ULOG_DEBUG("[%s] " fmt, LOG_TAG, ##__VA_ARGS__)
#define ULOG_TAG_INFO(fmt, ...)     ULOG_INFO("[%s] " fmt, LOG_TAG, ##__VA_ARGS__)
#define ULOG_TAG_WARNING(fmt, ...)  ULOG_WARNING("[%s] " fmt, LOG_TAG, ##__VA_ARGS__)
#define ULOG_TAG_ERROR(fmt, ...)    ULOG_ERROR("[%s] " fmt, LOG_TAG, ##__VA_ARGS__)
#define ULOG_TAG_CRITICAL(fmt, ...) ULOG_CRITICAL("[%s] " fmt, LOG_TAG, ##__VA_ARGS__)


void ulog_port_init(void);


#endif // _ULOG_PORT_H_