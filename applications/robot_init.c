/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-06 08:57:47
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-07 09:51:16
 * @FilePath: /threadx_learn/applications/robot_init.c
 * @Description: 
 */
#include "robot_init.h"
#include "BMI088.h"
#include "RGB.h"
#include "beep.h"
#include "bsp_adc.h"
#include "bsp_flash.h"
#include "bsp_gpio.h"
#include "dwt.h"
#include "imu.h"
#include "offline.h"
#include "referee.h"
#include "remote.h"
#include "subpub.h"
#include "systemwatch.h"
#include "ulog_port.h"

void bsp_init(void){
    DWT_Init(168);
    ulog_port_init();  // 初始化日志系统
    BSP_GPIO_EXTI_Module_Init();
    BSP_Flash_Init();
    BSP_ADC_Init();
}

void modules_init(TX_BYTE_POOL *pool){
    RGB_init();  // 初始化RGB灯
    beep_init();
    BMI088_init();  // 初始化BMI088传感器
    SYSTEMWATCH_INIT(pool);  // 初始化系统监控
    OFFLINE_INIT(pool);  // 初始化离线检测模块
    INS_TASK_init(pool);  // 初始化imu
    REFEREE_INIT(pool);  // 初始化裁判系统
    remote_init();  // 初始化遥控器
    pubsub_init();  // 初始化发布订阅系统
}
