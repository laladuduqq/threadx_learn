#ifndef __GIMBALCMD_H__
#define __GIMBALCMD_H__

#include "tx_api.h"

#if defined(USE_COMPOENT_CONFIG_H) && USE_COMPOENT_CONFIG_H
#include "compoent_config.h"
#else
#define GIMBAL_THREAD_STACK_SIZE 1024 // 云台线程栈大小
#define GIMBAL_THREAD_PRIORITY 9      // 云台线程优先级
#endif // _COMPOENT_CONFIG_H_


void gimbal_task_init(TX_BYTE_POOL *pool);

#endif /* __GIMBALCMD_H__ */
