/*
 * @Author: laladuduqq 17503181697@163.com
 * @Date: 2025-06-06 18:47:55
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-06 09:43:16
 * @FilePath: /threadx_learn/modules/systemwatch/systemwatch.h
 * @Description: 
 * 
 */
#ifndef __SYSTEMWATCH_H__
#define __SYSTEMWATCH_H__

#include <stdint.h>
#include "tx_api.h"


#if defined(USE_COMPOENT_CONFIG_H) && USE_COMPOENT_CONFIG_H
#include "compoent_config.h"
#else
#define MAX_MONITORED_TASKS 10     // 最大监控任务数
#define TASK_BLOCK_TIMEOUT 1       // 任务阻塞判定时间 (s)
#define SystemWatch_Enable 1       // 开启系统监控功能  //注意下述两个宏定义只有在开启系统监控功能时才会生效
#define SystemWatch_Reset_Enable 0 // 开启系统监控重置功能
#define SystemWatch_Iwdg_Enable 1  // 开启看门狗功能
#endif // _COMPOENT_CONFIG_H_




typedef struct
{
    uint8_t code;    /* 状态码 */
    char *name; /* 名称 */
} tx_thread_status_t;

typedef struct {
    TX_THREAD* handle;        //任务句柄
    const char* name;        //任务名称
    uint8_t isActive;       //是否在监控
    float dt;              // 更新间隔
    uint32_t dt_cnt;
    float last_report_time; // 上次上报时间，单位秒
} TaskMonitor_t;


#if SystemWatch_Enable
  #define SYSTEMWATCH_INIT(pool) systemwatch_init(pool)
  #define SYSTEMWATCH_REGISTER_TASK(taskHandle, taskName) systemwatch_register_task(taskHandle, taskName)
  #define SYSTEMWATCH_UPDATE_TASK(taskHandle) systemwatch_update_task(taskHandle)
#else
  #define SYSTEMWATCH_INIT(pool) do {} while(0)
  #define SYSTEMWATCH_REGISTER_TASK(taskHandle, taskName) do {} while(0)
  #define SYSTEMWATCH_UPDATE_TASK(taskHandle) do {} while(0)
#endif


/**
 * @description: systemwatch 初始化
 * @param {TX_BYTE_POOL} *pool
 * @return {*}
 */
void systemwatch_init(TX_BYTE_POOL *pool);
/**
 * @description: 注册需要监控的任务
 * @param {TX_THREAD} *taskHandle
 * @param {char*} taskName
 * @return {int8_t} 返回值: 0-成功, -1-失败
 */
int8_t systemwatch_register_task(TX_THREAD *taskHandle, const char* taskName);
/**
 * @description: 在被监控的任务中调用此函数更新计数器
 * @param {TX_THREAD} *taskHandle
 * @return {*}
 */
void systemwatch_update_task(TX_THREAD *taskHandle);

#endif /* __SYSTEMWATCH_H__ */
