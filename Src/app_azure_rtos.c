
/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_azure_rtos.c
  * @author  MCD Application Team
  * @brief   azure_rtos application implementation file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/

#include "app_azure_rtos.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "damiao.h"
#include "dm_imu.h"
#include "imu.h"
#include "robot_init.h"
#include "tx_api.h"
#include "tx_port.h"
#include "ulog.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
static TX_THREAD init_thread;
void init_Task(ULONG thread_input);
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN TX_Pool_Buffer */
/* USER CODE END TX_Pool_Buffer */
static UCHAR tx_byte_pool_buffer[TX_APP_MEM_POOL_SIZE];
TX_BYTE_POOL tx_app_byte_pool;

/* USER CODE BEGIN PV */
DMMOTOR_t *pitch_motor = NULL; 
const static IMU_DATA_T* imu;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/**
  * @brief  Define the initial system.
  * @param  first_unused_memory : Pointer to the first unused memory
  * @retval None
  */
VOID tx_application_define(VOID *first_unused_memory)
{
  /* USER CODE BEGIN  tx_application_define */

  /* USER CODE END  tx_application_define */

  VOID *memory_ptr;

  if (tx_byte_pool_create(&tx_app_byte_pool, "Tx App memory pool", tx_byte_pool_buffer, TX_APP_MEM_POOL_SIZE) != TX_SUCCESS)
  {
    /* USER CODE BEGIN TX_Byte_Pool_Error */

    /* USER CODE END TX_Byte_Pool_Error */
  }
  else
  {
    /* USER CODE BEGIN TX_Byte_Pool_Success */

    /* USER CODE END TX_Byte_Pool_Success */

    memory_ptr = (VOID *)&tx_app_byte_pool;

    if (App_ThreadX_Init(memory_ptr) != TX_SUCCESS)
    {
      /* USER CODE BEGIN  App_ThreadX_Init_Error */

      /* USER CODE END  App_ThreadX_Init_Error */
    }

    /* USER CODE BEGIN  App_ThreadX_Init_Success */
    static CHAR *init_thread_stack;
    tx_byte_allocate(&tx_app_byte_pool, (VOID **)&init_thread_stack, 2048, TX_NO_WAIT);
    tx_thread_create(&init_thread, "initTask", init_Task, 0,init_thread_stack, 2048, 0, 0, TX_NO_TIME_SLICE, TX_AUTO_START);
    /* USER CODE END  App_ThreadX_Init_Success */

  }

}

/* USER CODE BEGIN  0 */
void init_Task(ULONG thread_input)
{
    // 保存当前中断状态并禁用中断
    UINT old_posture = tx_interrupt_control(TX_INT_DISABLE);
    bsp_init();
    modules_init(&tx_app_byte_pool);
    // 恢复中断状态
    tx_interrupt_control(old_posture);
    ULOG_TAG_INFO("robot init success");
    imu = INS_GetData();
    Motor_Init_Config_s pitch_config = {
        .offline_device_motor ={
              .name = "DM4310",                        // 设备名称
              .timeout_ms = 100,                              // 超时时间
              .level = OFFLINE_LEVEL_HIGH,                     // 离线等级
              .beep_times = 2,                                // 蜂鸣次数
              .enable = OFFLINE_ENABLE,                       // 是否启用离线管理
            },
        .can_init_config = {
                .can_handle = &hcan2,
                .tx_id = 0X23,
                .rx_id = 0X206,
            },
        .controller_param_init_config = {
            .angle_PID = {
                .Kp = 0.01, // 8
                .Ki = 0,
                .Kd = 0,
                .DeadBand = 0.1,
                .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit,
                .IntegralLimit = 0,
                .MaxOut = 100,
            },
            .speed_PID = {
                .Kp = 0.3,  // 50
                .Ki = 0, // 200
                .Kd = 0,
                .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit,
                .IntegralLimit = 100,
                .MaxOut = 0.2,
            },
                .other_angle_feedback_ptr = imu->Pitch,
                .other_speed_feedback_ptr = &((*imu->gyro)[0]),
        },
        .controller_setting_init_config = {
            .angle_feedback_source = MOTOR_FEED,
            .speed_feedback_source = MOTOR_FEED,
            .outer_loop_type = ANGLE_LOOP,
            .close_loop_type = ANGLE_LOOP | SPEED_LOOP,
            .motor_reverse_flag = MOTOR_DIRECTION_NORMAL,
        },
        .Motor_init_Info ={
              .motor_type = DM4310,
              .max_current = 7.5f,
              .gear_ratio = 10,
              .max_torque = 7,
              .max_speed = 200,
              .torque_constant = 0.093f
            }

    };
    pitch_motor = DM_MOTOR_INIT(&pitch_config,MIT_MODE);
    while (1)
    {
      
      tx_thread_sleep(10);
    }
}
/* USER CODE END  0 */
