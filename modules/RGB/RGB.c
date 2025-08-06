/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-05 22:16:27
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-06 09:06:16
 * @FilePath: /threadx_learn/modules/RGB/RGB.c
 * @Description: 
 */
#include "RGB.h"
#include "bsp_pwm.h"

// RGB PWM设备指针
static PWM_Device* rgb_red_pwm = NULL;
static PWM_Device* rgb_green_pwm = NULL;
static PWM_Device* rgb_blue_pwm = NULL;

void RGB_init(void)
{
    // 初始化红色PWM通道 (TIM5 CH3)
    PWM_Init_Config red_config = {
        .htim = &htim5,
        .channel = PWM_CHANNEL_3,
        .frequency = 1000,  // 1kHz
        .duty_cycle = 0,    // 初始占空比0%
        .mode = PWM_MODE_NORMAL
    };
    
    // 初始化绿色PWM通道 (TIM5 CH2)
    PWM_Init_Config green_config = {
        .htim = &htim5,
        .channel = PWM_CHANNEL_2,
        .frequency = 1000,  // 1kHz
        .duty_cycle = 0,    // 初始占空比0%
        .mode = PWM_MODE_NORMAL
    };
    
    // 初始化蓝色PWM通道 (TIM5 CH1)
    PWM_Init_Config blue_config = {
        .htim = &htim5,
        .channel = PWM_CHANNEL_1,
        .frequency = 1000,  // 1kHz
        .duty_cycle = 0,    // 初始占空比0%
        .mode = PWM_MODE_NORMAL
    };
    
    // 初始化PWM设备
    rgb_red_pwm   = BSP_PWM_Device_Init(&red_config);
    rgb_green_pwm = BSP_PWM_Device_Init(&green_config);
    rgb_blue_pwm  = BSP_PWM_Device_Init(&blue_config);
    
    // 启动PWM输出
    if (rgb_red_pwm) {
        BSP_PWM_Start(rgb_red_pwm);
    }
    
    if (rgb_green_pwm) {
        BSP_PWM_Start(rgb_green_pwm);
    }
    
    if (rgb_blue_pwm) {
        BSP_PWM_Start(rgb_blue_pwm);
    }
}

void RGB_show(uint32_t aRGB)
{
    if (!rgb_red_pwm || !rgb_green_pwm || !rgb_blue_pwm) {
        return;
    }
    
    static uint8_t alpha;
    static uint8_t red, green, blue;
    
    // 提取ARGB值
    alpha = (aRGB & 0xFF000000) >> 24;
    red = (aRGB & 0x00FF0000) >> 16;
    green = (aRGB & 0x0000FF00) >> 8;
    blue = (aRGB & 0x000000FF) >> 0;
    
    // 应用alpha通道（如果需要）
    if (alpha != 0xFF) {
        red = (red * alpha) / 255;
        green = (green * alpha) / 255;
        blue = (blue * alpha) / 255;
    }
    
    // 将0-255的值转换为0-100的占空比
    uint8_t red_duty = (red * 100) / 255;
    uint8_t green_duty = (green * 100) / 255;
    uint8_t blue_duty = (blue * 100) / 255;
    
    // 设置PWM占空比
    BSP_PWM_Set_DutyCycle(rgb_red_pwm, red_duty);
    BSP_PWM_Set_DutyCycle(rgb_green_pwm, green_duty);
    BSP_PWM_Set_DutyCycle(rgb_blue_pwm, blue_duty);
}