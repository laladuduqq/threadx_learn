/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-05 17:30:00
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-05 17:30:00
 * @FilePath: /threadx_learn/BSP/ADC/bsp_adc.h
 * @Description: 
 */
#ifndef _BSP_ADC_H_
#define _BSP_ADC_H_

#include "adc.h"
#include "tx_api.h"

#if defined(USE_COMPOENT_CONFIG_H) && USE_COMPOENT_CONFIG_H
#include "compoent_config.h"
#else
#define ADC_DEVICE_NUM 4         // 最大ADC设备数
#endif

/* 浮点数类型定义 */
typedef float fp32;

/* ADC模式枚举 */
typedef enum {
    ADC_MODE_BLOCKING,           // 阻塞模式
    ADC_MODE_IT,                 // 中断模式
    ADC_MODE_DMA                 // DMA模式
} ADC_Mode;

/* ADC事件类型 */
typedef enum {
    ADC_EVENT_CONVERSION_COMPLETE,  // 转换完成
    ADC_EVENT_ERROR                 // 错误事件
} ADC_Event_Type;

/* ADC设备实例结构体 */
typedef struct {
    ADC_HandleTypeDef* hadc;            // ADC句柄
    ADC_ChannelConfTypeDef channel_config; // 通道配置
    ADC_Mode mode;                     // 工作模式
    uint32_t timeout;                  // 超时时间（仅阻塞模式使用）
    void (*adc_callback)(ADC_Event_Type event); // 回调函数
    uint32_t value;                    // 转换结果
    TX_MUTEX adc_mutex;                // 互斥锁保护ADC访问
} ADC_Device;

/* ADC初始化配置结构体 */
typedef struct {
    ADC_HandleTypeDef* hadc;
    ADC_ChannelConfTypeDef channel_config;
    ADC_Mode mode;
    uint32_t timeout;
    void (*adc_callback)(ADC_Event_Type event);
} ADC_Device_Init_Config;

/**
 * @description: 初始化ADC设备
 * @param {ADC_Device_Init_Config*} config ADC初始化配置
 * @return {ADC_Device*} 成功返回ADC设备实例指针，失败返回NULL
 */
ADC_Device* BSP_ADC_Device_Init(ADC_Device_Init_Config* config);

/**
 * @description: 销毁ADC设备实例
 * @param {ADC_Device*} dev ADC设备实例
 * @return {void}
 */
void BSP_ADC_Device_DeInit(ADC_Device* dev);

/**
 * @description: 启动ADC转换
 * @param {ADC_Device*} dev ADC设备实例
 * @return {HAL_StatusTypeDef} HAL状态
 */
HAL_StatusTypeDef BSP_ADC_Start(ADC_Device* dev);

/**
 * @description: 停止ADC转换
 * @param {ADC_Device*} dev ADC设备实例
 * @return {HAL_StatusTypeDef} HAL状态
 */
HAL_StatusTypeDef BSP_ADC_Stop(ADC_Device* dev);

/**
 * @description: 获取ADC转换值
 * @param {ADC_Device*} dev ADC设备实例
 * @param {uint32_t*} value 转换结果指针
 * @return {HAL_StatusTypeDef} HAL状态
 */
HAL_StatusTypeDef BSP_ADC_Get_Value(ADC_Device* dev, uint32_t* value);

/**
 * @description: ADC初始化（系统初始化时调用一次）
 * @return {void}
 */
void BSP_ADC_Init(void);

/**
 * @description: 获取温度值
 * @return {fp32} 温度值(摄氏度)
 */
fp32 BSP_ADC_Get_Temperature(void);

#endif // _BSP_ADC_H_