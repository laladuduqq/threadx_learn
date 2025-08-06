/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-05 15:47:06
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-06 08:59:52
 * @FilePath: /threadx_learn/BSP/PWM/bsp_pwm.h
 * @Description: 
 */
#ifndef _BSP_PWM_H_
#define _BSP_PWM_H_

#include "tim.h"
#include "tx_api.h"

#if defined(USE_COMPOENT_CONFIG_H) && USE_COMPOENT_CONFIG_H
#include "compoent_config.h"
#else
#define MAX_PWM_DEVICES 10
#endif

/* PWM模式枚举 */
typedef enum {
    PWM_MODE_NORMAL,     // 普通PWM模式
    PWM_MODE_INVERTED    // 反转PWM模式
} PWM_Mode;

/* PWM通道枚举 */
typedef enum {
    PWM_CHANNEL_1 = TIM_CHANNEL_1,
    PWM_CHANNEL_2 = TIM_CHANNEL_2,
    PWM_CHANNEL_3 = TIM_CHANNEL_3,
    PWM_CHANNEL_4 = TIM_CHANNEL_4
} PWM_Channel;

/* PWM设备实例结构体 */
typedef struct {
    TIM_HandleTypeDef* htim;      // 定时器句柄
    PWM_Channel channel;          // PWM通道
    uint32_t period;             // 周期值
    uint32_t pulse;              // 脉冲宽度值
    uint32_t frequency;          // 频率(Hz)
    uint8_t duty_cycle;          // 占空比(0-100)
    PWM_Mode mode;               // PWM模式
} PWM_Device;

/* PWM初始化配置结构体 */
typedef struct {
    TIM_HandleTypeDef* htim;
    PWM_Channel channel;
    uint32_t frequency;          // 频率(Hz)
    uint8_t duty_cycle;          // 初始占空比(0-100)
    PWM_Mode mode;               // PWM模式
} PWM_Init_Config;

/**
 * @description: 初始化PWM设备
 * @param {PWM_Init_Config*} config PWM初始化配置
 * @return {PWM_Device*} 成功返回PWM设备实例指针，失败返回NULL
 */
PWM_Device* BSP_PWM_Device_Init(PWM_Init_Config* config);

/**
 * @description: 启动PWM输出
 * @param {PWM_Device*} pwm_dev PWM设备实例
 * @return {HAL_StatusTypeDef} HAL状态
 */
HAL_StatusTypeDef BSP_PWM_Start(PWM_Device* pwm_dev);

/**
 * @description: 停止PWM输出
 * @param {PWM_Device*} pwm_dev PWM设备实例
 * @return {HAL_StatusTypeDef} HAL状态
 */
HAL_StatusTypeDef BSP_PWM_Stop(PWM_Device* pwm_dev);

/**
 * @description: 设置PWM频率
 * @param {PWM_Device*} pwm_dev PWM设备实例
 * @param {uint32_t} frequency 目标频率(Hz)
 * @return {HAL_StatusTypeDef} HAL状态
 */
HAL_StatusTypeDef BSP_PWM_Set_Frequency(PWM_Device* pwm_dev, uint32_t frequency);

/**
 * @description: 设置PWM占空比
 * @param {PWM_Device*} pwm_dev PWM设备实例
 * @param {uint8_t} duty_cycle 目标占空比(0-100)
 * @return {HAL_StatusTypeDef} HAL状态
 */
HAL_StatusTypeDef BSP_PWM_Set_DutyCycle(PWM_Device* pwm_dev, uint8_t duty_cycle);

/**
 * @description: 同时设置PWM频率和占空比
 * @param {PWM_Device*} pwm_dev PWM设备实例
 * @param {uint32_t} frequency 目标频率(Hz)
 * @param {uint8_t} duty_cycle 目标占空比(0-100)
 * @return {HAL_StatusTypeDef} HAL状态
 */
HAL_StatusTypeDef BSP_PWM_Set_Frequency_And_DutyCycle(PWM_Device* pwm_dev, uint32_t frequency, uint8_t duty_cycle);

/**
 * @description: 获取PWM频率
 * @param {PWM_Device*} pwm_dev PWM设备实例
 * @return {uint32_t} 当前频率(Hz)
 */
uint32_t BSP_PWM_Get_Frequency(PWM_Device* pwm_dev);

/**
 * @description: 获取PWM占空比
 * @param {PWM_Device*} pwm_dev PWM设备实例
 * @return {uint8_t} 当前占空比(0-100)
 */
uint8_t BSP_PWM_Get_DutyCycle(PWM_Device* pwm_dev);

/**
 * @description: 销毁PWM设备实例
 * @param {PWM_Device*} pwm_dev PWM设备实例
 * @return {void}
 */
void BSP_PWM_DeInit(PWM_Device* pwm_dev);

#endif // _BSP_PWM_H_