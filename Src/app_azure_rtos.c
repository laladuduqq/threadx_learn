
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
#include "bsp_gpio.h"
#include "dji.h"
#include "motor_def.h"
#include "robot_init.h"
#include "stm32f4xx_hal_gpio.h"
#include "tx_api.h"
#include "tx_port.h"
#include "ulog.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
static DJIMotor_t *motor_lf; // left right forward back
static CHAR key_flag =0;
void key_callback(GPIO_EXTI_Event_Type event);
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
    Motor_Init_Config_s chassis_motor_config = {
        .offline_device_motor ={
          .timeout_ms = 100,                              // 超时时间
          .level = OFFLINE_LEVEL_HIGH,                     // 离线等级
          .enable = OFFLINE_ENABLE,                       // 是否启用离线管理
        },
        .can_init_config = {
            .can_handle = &hcan1,
        },
        .controller_param_init_config = {
            .lqr_config ={
                .K ={0.05f}, 
                .output_max = 6.0f,
                .output_min =-6.0f,
                .state_dim = 1,
            }
        },
        .controller_setting_init_config = {
            .angle_feedback_source = MOTOR_FEED,
            .speed_feedback_source = MOTOR_FEED,
            .outer_loop_type    = SPEED_LOOP,
            .close_loop_type    = SPEED_LOOP,
            .feedback_reverse_flag =FEEDBACK_DIRECTION_NORMAL,
            .control_algorithm =CONTROL_LQR,
            .PowerControlState =PowerControlState_ON,
        },
        .Motor_init_Info ={
              .motor_type = M3508,
              .max_current = 10.0f,
              .gear_ratio = 19,
              .max_torque = 6.0f,
              .max_speed = 482,
              .torque_constant = 0.0156f
            }
    };
    chassis_motor_config.can_init_config.tx_id = 4;
    chassis_motor_config.controller_setting_init_config.motor_reverse_flag = MOTOR_DIRECTION_NORMAL;
    chassis_motor_config.offline_device_motor.name = "m3508_1";
    chassis_motor_config.offline_device_motor.beep_times = 1;
    motor_lf = DJIMotorInit(&chassis_motor_config);
    
    BSP_GPIO_EXTI_Register_Callback(GPIO_PIN_0,key_callback);
    while (1)
    {
      
      tx_thread_sleep(10);
    }
}

void key_callback(GPIO_EXTI_Event_Type event) {
    if (key_flag==0)
    {
      DJI_MOTOR_ENABLE(motor_lf);
      DJI_MOTOR_SET_REF(motor_lf, 1000 * 6.0f);
      key_flag=1;
    }
    else if (key_flag==1) {
      DJI_MOTOR_ENABLE(motor_lf);
      DJI_MOTOR_SET_REF(motor_lf, 0);
      key_flag=0;
    }
    else
    {
      DJI_MOTOR_STOP(motor_lf);
      DJI_MOTOR_SET_REF(motor_lf, 0);
      key_flag=0;
    }
}
/* USER CODE END  0 */
