/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-08 17:41:07
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-08 17:41:16
 * @FilePath: /threadx_learn/applications/shoot/shootcmd.h
 * @Description: 
 */
#ifndef __SHOOTCMD_H__
#define __SHOOTCMD_H__

#include "tx_api.h"


#if defined(USE_COMPOENT_CONFIG_H) && USE_COMPOENT_CONFIG_H
#include "compoent_config.h"
#else
#define SHOOT_THREAD_STACK_SIZE 1024 // 发射线程栈大小
#define SHOOT_THREAD_PRIORITY 10      // 发射线程优先级
#endif // _COMPOENT_CONFIG_H_


void shoot_task_init(TX_BYTE_POOL *pool);

#endif /* __SHOOTCMD_H__ */
