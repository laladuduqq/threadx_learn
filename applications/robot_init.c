/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-06 08:57:47
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-06 09:02:50
 * @FilePath: /threadx_learn/applications/robot_init.c
 * @Description: 
 */
#include "robot_init.h"
#include "BMI088.h"
#include "RGB.h"
#include "bsp_adc.h"
#include "bsp_flash.h"
#include "bsp_gpio.h"
#include "dwt.h"
#include "ulog_port.h"

void bsp_init(void){
    DWT_Init(168);
    ulog_port_init();  // 初始化日志系统
    BSP_GPIO_EXTI_Module_Init();
    BSP_Flash_Init();
    BSP_ADC_Init();
}

void modules_init(void){
    RGB_init();  // 初始化RGB灯
    BMI088_init();  // 初始化BMI088传感器
}
