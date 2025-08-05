/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-05 18:00:00
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-05 18:00:00
 * @FilePath: /threadx_learn/BSP/GPIO/bsp_gpio.h
 * @Description: GPIO中断回调管理
 */
#ifndef _BSP_GPIO_H_
#define _BSP_GPIO_H_

#include "gpio.h"
#include "tx_api.h"

#if defined(USE_COMPOENT_CONFIG_H) && USE_COMPOENT_CONFIG_H
#include "compoent_config.h"
#else
#define GPIO_EXTI_DEVICE_NUM 16         // 最大GPIO中断回调设备数
#endif

/* GPIO中断事件类型 */
typedef enum {
    GPIO_EXTI_EVENT_TRIGGERED,          // 中断触发事件
} GPIO_EXTI_Event_Type;

/* GPIO中断回调函数类型 */
typedef void (*gpio_exti_callback_t)(GPIO_EXTI_Event_Type event);

/* GPIO中断设备实例结构体 */
typedef struct {
    uint16_t pin;                       // GPIO引脚号 (GPIO_PIN_0 - GPIO_PIN_15)
    gpio_exti_callback_t callback;      // 回调函数
    uint8_t is_used;                    // 设备是否已使用
} GPIO_EXTI_Device;

/**
 * @description: 注册GPIO中断回调函数
 * @param {uint16_t} pin GPIO引脚号 (GPIO_PIN_0 - GPIO_PIN_15)
 * @param {gpio_exti_callback_t} callback 回调函数
 * @return {int} 成功返回0，失败返回-1
 */
int BSP_GPIO_EXTI_Register_Callback(uint16_t pin, gpio_exti_callback_t callback);

/**
 * @description: 注销GPIO中断回调函数
 * @param {uint16_t} pin GPIO引脚号 (GPIO_PIN_0 - GPIO_PIN_15)
 * @return {int} 成功返回0，失败返回-1
 */
int BSP_GPIO_EXTI_Unregister_Callback(uint16_t pin);

/**
 * @description: GPIO中断回调模块初始化（系统初始化时调用一次）
 * @return {void}
 */
void BSP_GPIO_EXTI_Module_Init(void);

#endif // _BSP_GPIO_H_