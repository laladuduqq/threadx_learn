/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-06 08:57:47
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-08 17:59:58
 * @FilePath: /threadx_learn/applications/robot_init.c
 * @Description: 
 */
#include "robot_init.h"
#include "BMI088.h"
#include "RGB.h"
#include "beep.h"
#include "board_com.h"
#include "bsp_adc.h"
#include "bsp_flash.h"
#include "bsp_gpio.h"
#include "chassiscmd.h"
#include "dm_imu.h"
#include "dwt.h"
#include "gimbalcmd.h"
#include "imu.h"
#include "motor_task.h"
#include "offline.h"
#include "referee.h"
#include "remote.h"
#include "robot_task.h"
#include "shootcmd.h"
#include "subpub.h"
#include "systemwatch.h"
#include "ulog_port.h"
#include "robotdef.h"
#include "compoent_config.h"

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
    #ifndef ONE_BOARD
        #if defined (CHASSIS_BOARD)
        REFEREE_INIT(pool);  // 初始化裁判系统
        #elif defined (GIMBAL_BOARD)
        remote_init();  // 初始化遥控器
        DM_IMU_INIT(pool);  // 初始化达妙IMU,如果需要用才初始化
        #endif
    #else  
        REFEREE_INIT(pool);  // 初始化裁判系统
        remote_init();  // 初始化遥控器
    #endif 
    pubsub_init();  // 初始化发布订阅系统
    board_com_init();   // 板间通讯初始化
    motor_task_init(pool);
}

void applications_init(TX_BYTE_POOL *pool){
    robot_control_task_init(pool);
    #ifndef ONE_BOARD
        #if defined (CHASSIS_BOARD)
        chassis_task_init(pool);
        #elif defined (GIMBAL_BOARD)
        gimbal_task_init(pool);
        shoot_task_init(pool);
        #endif
    #else  
        
    #endif 
}
