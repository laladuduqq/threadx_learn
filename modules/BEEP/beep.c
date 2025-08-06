/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-06 10:31:16
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-06 10:38:30
 * @FilePath: /threadx_learn/modules/BEEP/beep.c
 * @Description: 蜂鸣器模块，基于BSP_PWM组件实现
 */
#include "beep.h"
#include "bsp_pwm.h"
#include "dwt.h"
#include "tim.h"
#include <stdint.h>

// 蜂鸣器设备实例
static PWM_Device* beep_pwm_dev = NULL;
// 蜂鸣器响声次数
static uint8_t beep_times = 0;

int8_t beep_init()
{
    
    // 配置PWM设备
    PWM_Init_Config pwm_config = {
        .htim = &htim4,
        .channel = PWM_CHANNEL_3,
        .frequency = 1000000 / BEEP_TUNE_VALUE, // 根据自动重装载值计算频率
        .duty_cycle = (BEEP_CTRL_VALUE * 100) / BEEP_TUNE_VALUE, // 计算占空比
        .mode = PWM_MODE_NORMAL
    };
    
    // 初始化PWM设备
    beep_pwm_dev = BSP_PWM_Device_Init(&pwm_config);
    if (beep_pwm_dev == NULL) {
        return -1;
    }
    
    // 启动PWM输出
    if (BSP_PWM_Start(beep_pwm_dev) != HAL_OK) {
        BSP_PWM_DeInit(beep_pwm_dev);
        beep_pwm_dev = NULL;
        return -1;
    }
    
    // 默认关闭蜂鸣器
    beep_set_tune(0, 0);
    
    return 0;
}

int32_t beep_set_times(uint8_t times)
{
    beep_times = times;
    return 0;
}

void beep_set_tune(uint16_t tune, uint16_t ctrl)
{
    if (beep_pwm_dev == NULL) {
        return;
    }
    
    if (tune == 0) {
        // 关闭蜂鸣器
        BSP_PWM_Set_DutyCycle(beep_pwm_dev, 0);
    } else {
        // 设置频率和占空比
        uint32_t frequency = 1000000 / tune;
        uint8_t duty_cycle = (ctrl * 100) / tune;
        
        BSP_PWM_Set_Frequency_And_DutyCycle(beep_pwm_dev, frequency, duty_cycle);
    }
}

void beep_ctrl_times(void)
{
    static uint32_t beep_tick;
    static uint32_t times_tick;
    static uint8_t times;

    if (DWT_GetTimeline_ms() - beep_tick > BEEP_PERIOD)
    {
        times = beep_times;
        beep_tick = DWT_GetTimeline_ms();
        times_tick = DWT_GetTimeline_ms();
    }
    else if (times != 0)
    {
        if (DWT_GetTimeline_ms() - times_tick < BEEP_ON_TIME)
        {
            #if OFFLINE_Beep_Enable == 1
            beep_set_tune(BEEP_TUNE_VALUE, BEEP_CTRL_VALUE);
            #else
            beep_set_tune(0, 0);
            #endif
        }
        else if (DWT_GetTimeline_ms() - times_tick < BEEP_ON_TIME + BEEP_OFF_TIME)
        {
            beep_set_tune(0, 0);
        }
        else
        {
            times--;
            times_tick = DWT_GetTimeline_ms();
        }
    }
}

void beep_deinit(void)
{
    if (beep_pwm_dev != NULL) {
        BSP_PWM_Stop(beep_pwm_dev);
        BSP_PWM_DeInit(beep_pwm_dev);
        beep_pwm_dev = NULL;
    }
}