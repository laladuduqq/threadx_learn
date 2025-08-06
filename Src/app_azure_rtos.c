
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
#include "board_com.h"
#include "robot_init.h"
#include "tx_api.h"
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
    tx_byte_allocate(&tx_app_byte_pool, (VOID **)&init_thread_stack, 1024, TX_NO_WAIT);
    tx_thread_create(&init_thread, "initTask", init_Task, 0,init_thread_stack, 1024, 0, 0, TX_NO_TIME_SLICE, TX_AUTO_START);
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
    // 初始化配置
    board_com_init_t board_com_config = {
        .offline_manage_init = {
                          .name = "board_com",
                          .timeout_ms = 100,
                          .level = OFFLINE_LEVEL_HIGH,
                          .beep_times = 8,
                          .enable = OFFLINE_ENABLE,
                        },
        .Can_Device_Init_Config = {
                            .can_handle = &hcan2,
                            .tx_id = GIMBAL_ID,
                            .rx_id = CHASSIS_ID,
                        }
    };
    // 初始化板间通讯
    board_com_t *board_com = BOARD_COM_INIT(&board_com_config);
    while (1)
    {
      
      tx_thread_sleep(10);
    }
}
/* USER CODE END  0 */
