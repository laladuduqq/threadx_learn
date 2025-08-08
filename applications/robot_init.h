/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-06 08:57:55
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-08 15:02:19
 * @FilePath: /threadx_learn/applications/robot_init.h
 * @Description: 
 */
#ifndef _ROBOT_INIT_H_
#define _ROBOT_INIT_H_

#include "tx_api.h"

void bsp_init(void);
void modules_init(TX_BYTE_POOL *pool);
void applications_init(TX_BYTE_POOL *pool);

#endif // _ROBOT_INIT_H_