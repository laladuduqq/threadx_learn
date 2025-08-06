/*
 * @Author: laladuduqq 17503181697@163.com
 * @Date: 2025-06-18 11:10:50
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-06 15:59:43
 * @FilePath: /threadx_learn/modules/REMOTE/remote.h
 * @Description: 
 * 
 */
#ifndef REMOTE_H
#define REMOTE_H

#include "sbus.h"
#include "dt7.h"
#include "vt03.h"
#include "vt02.h"

#if defined(USE_COMPOENT_CONFIG_H) && USE_COMPOENT_CONFIG_H
#include "compoent_config.h"
#else
//在这里决定控制来源 注意键鼠控制部分 图传优先遥控器 uart部分c板只有 huart1/huart3/huart6
//遥控器选择 sbus 0 dt7 1 none 2
#define REMOTE_SOURCE    0
#define REMOTE_UART      huart3
//图传选择 vt02 0 vt03 1  
#define REMOTE_VT_SOURCE 1
#define REMOTE_VT_UART   huart6
#endif // _COMPOENT_CONFIG_H_


/* 统一的按键类型定义 */
typedef enum {
    KEY_W = 0,
    KEY_S,
    KEY_A,
    KEY_D,
    KEY_SHIFT,
    KEY_CTRL,
    KEY_Q,
    KEY_E,
    KEY_R,
    KEY_F,
    KEY_G,
    KEY_Z,
    KEY_X,
    KEY_C,
    KEY_V,
    KEY_B,
    KEY_COUNT  // 用于计数
} remote_key_e;

typedef enum {
    INPUT_FROM_DT7,    // 来自 DT7 遥控器
    INPUT_FROM_VT03,   // 来自 VT03 图传
    INPUT_FROM_VT02    // 来自 VT02 图传
} input_source_e;

// 遥控器类型枚举
typedef enum {
    REMOTE_TYPE_NONE,
    REMOTE_TYPE_SBUS,
    REMOTE_TYPE_DT7
} remote_type_e;

// 图传类型枚举
typedef enum {
    VT_TYPE_NONE,
    VT_TYPE_VT02,
    VT_TYPE_VT03
} vt_type_e;

// 遥控器状态结构体
typedef struct {
    union {
        sbus_info_t *sbus;
        dt7_info_t *dt7;
    } remote;
    
    union {
        vt02_info_t *vt02;
        vt03_info_t *vt03;
    } vt;

    remote_type_e remote_type;
    vt_type_e vt_type;
} remote_info_t;

/**
 * @description: 初始化 remote 遥控器
 * @return {void}
 */
void remote_init(void);

/**
 * @description: 所有遥控器数据数据
 * @return {remote_info_t*} 指向remote数据的指针
 */
remote_info_t *get_remote_info(void);
/**
 * @description: 获取对应的按键状态
 * @param {int} i 从remote_key_e中选择
 * @param {uint8_t} input_source 从input_source_e中选择
 * @return {对应按键状态，如果无则返回0}
 */
uint8_t remote_get_key_state(int i,uint8_t input_source);

#endif // REMOTE_H
