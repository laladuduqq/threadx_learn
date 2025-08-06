/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-06 10:51:09
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-06 11:04:59
 * @FilePath: /threadx_learn/modules/OFFLINE/offline.h
 * @Description: 
 */

#ifndef OFFLINE_H
#define OFFLINE_H

#include "tx_api.h"
#include <stdint.h>
#include <stdbool.h>

#if defined(USE_COMPOENT_CONFIG_H) && USE_COMPOENT_CONFIG_H
#include "compoent_config.h"
#else
#define OFFLINE_Enable 1               // 开启离线检测功能
#if OFFLINE_Enable
    #define OFFLINE_THREAD_STACK_SIZE 1024     // 离线检测线程栈大小
    #define OFFLINE_THREAD_PRIORITY 3          // 离线检测线程优先级
    #define OFFLINE_Beep_Enable 1              // 开启离线蜂鸣器功能
    #define MAX_OFFLINE_DEVICES    12         // 最大离线设备数量，这里根据需要自己修改 
#endif
#endif // _COMPOENT_CONFIG_H_

#define OFFLINE_INVALID_INDEX  0xFF

// 状态定义
#define STATE_ONLINE           0
#define STATE_OFFLINE          1
#define OFFLINE_ENABLE         1
#define OFFLINE_DISABLE        0

// 离线等级定义
typedef enum {
    OFFLINE_LEVEL_LOW = 1,    // 低优先级
    OFFLINE_LEVEL_MEDIUM = 2, // 中优先级
    OFFLINE_LEVEL_HIGH = 3    // 高优先级
} OfflineLevel_e;

// 设备初始化配置结构体
typedef struct {
    const char* name;         // 设备名称
    uint32_t timeout_ms;      // 超时时间
    OfflineLevel_e level;     // 离线等级
    uint8_t beep_times;       // 蜂鸣次数
    uint8_t enable;           // 是否启用检测
} OfflineDeviceInit_t;

// 离线设备结构体
typedef struct {
    char name[32];          
    uint32_t timeout_ms;    
    OfflineLevel_e level;   
    uint8_t beep_times;     
    bool is_offline;        
    uint32_t last_time;
    uint8_t index;          // 添加索引字段，用于快速更新
    uint8_t enable;         // 是否启用检测

    float dt;
    uint32_t dt_cnt;
} OfflineDevice_t; 

// 离线管理器结构体
typedef struct {
    OfflineDevice_t devices[MAX_OFFLINE_DEVICES];
    uint8_t device_count;
} OfflineManager_t;

#if OFFLINE_Enable
  #define OFFLINE_INIT(pool) offline_init(pool)
  #define OFFLINE_DEVICE_REGISTER(init) offline_device_register(init)
  #define OFFLINE_DEVICE_UPDATE(device_index) offline_device_update(device_index)
  #define OFFLINE_DEVICE_ENABLE(device_index) offline_device_enable(device_index)
  #define OFFLINE_DEVICE_DISABLE(device_index) offline_device_disable(device_index)
  #define GET_DEVICE_STATUS(device_index) get_device_status(device_index)
  #define GET_SYSTEM_STATUS() get_system_status()
#else
  #define OFFLINE_INIT(pool) do {} while(0)
  #define OFFLINE_DEVICE_REGISTER(init) OFFLINE_INVALID_INDEX
  #define OFFLINE_DEVICE_UPDATE(device_index) do {} while(0)
  #define OFFLINE_DEVICE_ENABLE(device_index) do {} while(0)
  #define OFFLINE_DEVICE_DISABLE(device_index) do {} while(0)
  #define GET_DEVICE_STATUS(device_index) STATE_ONLINE
  #define GET_SYSTEM_STATUS() 0
#endif

// 函数声明
/**
 * @description: offline模块的初始化
 * @param {TX_BYTE_POOL} *pool
 * @return {*}
 */
void offline_init(TX_BYTE_POOL *pool);
/**
 * @description: 注册设备
 * @param {OfflineDeviceInit_t*} init
 * @return 成功返回对应的index，否则则会返回OFFLINE_INVALID_INDEX
 */
uint8_t offline_device_register(const OfflineDeviceInit_t* init);
/**
 * @description: 更新对应的设备离线状态
 * @param {uint8_t} device_index
 * @return {*}
 */
void offline_device_update(uint8_t device_index);
/**
 * @description: 开启对应设备离线检测
 * @param {uint8_t} device_index
 * @return {*}
 */
void offline_device_enable(uint8_t device_index);
/**
 * @description: 关闭对应设备的离线检测
 * @param {uint8_t} device_index
 * @return {*}
 */
void offline_device_disable(uint8_t device_index);
/**
 * @description: 获取对应设备的离线状态
 * @param {uint8_t} device_index
 * @return 设备在线则会返回STATE_ONLINE，否则返回STATE_OFFLINE
 */
uint8_t get_device_status(uint8_t device_index);
/**
 * @description: 获取所有注册设备的状态和
 * @return 所有设备在线返回STATE_ONLINE，否则返回STATE_OFFLINE
 */
uint8_t get_system_status(void);

#endif /* OFFLINE_H */