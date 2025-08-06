/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-04 17:25:16
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-06 10:27:33
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


void ulog_port_init(void);


#endif // _ULOG_PORT_H_