/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-08 14:47:47
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-08 14:59:08
 * @FilePath: /threadx_learn/applications/robot_control/robot_task.h
 * @Description: 
 */
#ifndef __ROBOT_TASK_H__
#define __ROBOT_TASK_H__

#include "tx_api.h"

#if defined(USE_COMPOENT_CONFIG_H) && USE_COMPOENT_CONFIG_H
#include "compoent_config.h"
#else
#endif // _COMPOENT_CONFIG_H_




void robot_control_task_init(TX_BYTE_POOL *pool);

#endif /* __ROBOT_TASK_H__ */
