/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-08 17:54:25
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-08 17:54:57
 * @FilePath: /threadx_learn/applications/chassis/chassiscmd.h
 * @Description: 
 */
#ifndef __CHASSISCMD_H__
#define __CHASSISCMD_H__

#include "tx_api.h"

#if defined(USE_COMPOENT_CONFIG_H) && USE_COMPOENT_CONFIG_H
#include "compoent_config.h"
#else
#define CHASSIS_THREAD_STACK_SIZE 1024 // 底盘线程栈大小
#define CHASSIS_THREAD_PRIORITY 9      // 底盘线程优先级
#endif // _COMPOENT_CONFIG_H_


void chassis_task_init(TX_BYTE_POOL *pool);


#endif /* __CHASSISCMD_H__ */
