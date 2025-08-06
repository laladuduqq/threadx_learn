/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-06 10:31:16
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-06 10:38:37
 * @FilePath: /threadx_learn/modules/BEEP/beep.h
 * @Description: 蜂鸣器模块头文件
 */
#ifndef _BEEP_H_
#define _BEEP_H_

#include "bsp_pwm.h"
#include "tim.h"
#include <stdint.h>

#if defined(USE_COMPOENT_CONFIG_H) && USE_COMPOENT_CONFIG_H
#include "compoent_config.h"
#else
// 蜂鸣器配置参数
#define BEEP_PERIOD   2000  // 注意这里的周期，由于在offline task(10ms)中,尽量保证整除
#define BEEP_ON_TIME  100   // BEEP_ON_TIME BEEP_OFF_TIME 共同影响在周期内的最大beep times
#define BEEP_OFF_TIME 100

#define BEEP_TUNE_VALUE 500  // 决定beep的音调
#define BEEP_CTRL_VALUE 100  // 决定beep的音量

#if !defined(OFFLINE_Beep_Enable)
#define OFFLINE_Beep_Enable 1
#endif
#endif // _COMPOENT_CONFIG_H_



/**
 * @description: 初始化蜂鸣器模块
 * @return {int8_t} 0-成功, -1-失败
 */
int8_t beep_init();

/**
 * @description: 设置蜂鸣器响声次数
 * @param {uint8_t} times 响声次数
 * @return {int32_t} 0-成功
 */
int32_t beep_set_times(uint8_t times);

/**
 * @description: 设置蜂鸣器音调和音量
 * @param {uint16_t} tune 自动重装载值，影响频率
 * @param {uint16_t} ctrl 比较值，影响占空比
 * @return {void}
 */
void beep_set_tune(uint16_t tune, uint16_t ctrl);

/**
 * @description: 控制蜂鸣器响声次数，需周期性调用(每个BEEP_PERIOD周期)
 * @param {void}
 * @return {void}
 */
void beep_ctrl_times(void);

/**
 * @description: 反初始化蜂鸣器模块
 * @param {void}
 * @return {void}
 */
void beep_deinit(void);

#endif // _BEEP_H_