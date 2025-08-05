
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
#include "bsp_flash.h"
#include "bsp_iic.h"
#include "compoent_config.h"
#include "stm32f4xx_hal_gpio.h"
#include "tx_api.h"
#include "tx_port.h"
#include "ulog.h"
#include "ulog_port.h"
#include "dwt.h"
#include <stdint.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
static TX_THREAD test_thread;
void testTask(ULONG thread_input);
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
    DWT_Init(168);
    ulog_port_init();
    BSP_Flash_Init();
    static CHAR *test_thread_stack;
    test_thread_stack = threadx_malloc(1024);
    tx_thread_create(&test_thread, "testTask", testTask, 0,test_thread_stack, 1024, 5, 5, TX_NO_TIME_SLICE, TX_AUTO_START);
    ULOG_TAG_INFO("ThreadX initialized successfully");
    /* USER CODE END  App_ThreadX_Init_Success */

  }

}

/* USER CODE BEGIN  0 */
#define IST8310_WRITE_REG_NUM 4     // 方便阅读
#define IST8310_DATA_REG 0x03       // ist8310的数据寄存器
#define IST8310_WHO_AM_I 0x00       // ist8310 id 寄存器值
#define IST8310_WHO_AM_I_VALUE 0x10 // 用于检测是否连接成功,读取ist8310的whoami会返回该值

// -------------------初始化写入数组,只使用一次,详见datasheet-------------------------
// the first column:the registers of IST8310. 第一列:IST8310的寄存器
// the second column: the value to be writed to the registers.第二列:需要写入的寄存器值
// the third column: return error value.第三列:返回的错误码
static uint8_t ist8310_write_reg_data_error[IST8310_WRITE_REG_NUM][3] = {
    {0x0B, 0x08, 0x01},  // enalbe interrupt  and low pin polarity.开启中断，并且设置低电平
    {0x41, 0x09, 0x02},  // average 2 times.平均采样两次
    {0x42, 0xC0, 0x03},  // must be 0xC0. 必须是0xC0
    {0x0A, 0x0B, 0x04}}; // 200Hz output rate.200Hz输出频率
// 定义回调函数
void i2c_callback_handler(I2C_Event_Type event) {
    if (event == I2C_EVENT_TX_COMPLETE) {
        ULOG_TAG_INFO("I2C Transmission Complete");
    } else if (event == I2C_EVENT_RX_COMPLETE) {
        ULOG_TAG_INFO("I2C Reception Complete");
    } else if (event == I2C_EVENT_ERROR) {
        ULOG_TAG_ERROR("I2C Error Occurred");
    }
}
void testTask(ULONG thread_input)
{
  // 配置I2C设备
  I2C_Device_Init_Config i2c_config = {
      .hi2c = &hi2c3,                // 使用I2C1
      .dev_address = 0x0E << 1,      // 设备地址(7位地址左移1位)
      .tx_mode = I2C_MODE_BLOCKING,       
      .rx_mode = I2C_MODE_BLOCKING,      
      .timeout = 1000,               // 超时时间1000ms
      .i2c_callback = i2c_callback_handler  // 回调函数
  };

  // 初始化设备
  I2C_Device* my_i2c_device = BSP_I2C_Device_Init(&i2c_config);
    // 重置IST8310,需要HAL_Delay()等待传感器完成Reset
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_6, GPIO_PIN_RESET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_6, GPIO_PIN_SET);
    HAL_Delay(50);

    // 读取IST8310的ID,如果不是0x10(whoami macro的值),则返回错误
    uint8_t check_who_i_am = 0;          // 用于检测ist8310是否连接成功
    BSP_I2C_Mem_Write_Read(my_i2c_device, IST8310_WHO_AM_I, I2C_MEMADD_SIZE_8BIT, &check_who_i_am, 1, 0);
    if (check_who_i_am != IST8310_WHO_AM_I_VALUE){ 
        ULOG_TAG_ERROR("IST8310 not connected, check wiring");
    }

    // 进行初始化配置写入并检查是否写入成功,这里用循环把最上面初始化数组的东西都写进去
    for (uint8_t i = 0; i < IST8310_WRITE_REG_NUM; i++)
    { // 写入配置,写一句就读一下看看ist8310是否仍然在线
        BSP_I2C_Mem_Write_Read(my_i2c_device, ist8310_write_reg_data_error[i][0], I2C_MEMADD_SIZE_8BIT, &ist8310_write_reg_data_error[i][1], 1, 1);
        // 读取回写的值,如果不对则返回错误
        BSP_I2C_Mem_Write_Read(my_i2c_device, ist8310_write_reg_data_error[i][0], I2C_MEMADD_SIZE_8BIT, &check_who_i_am, 1, 0);
        if (check_who_i_am != ist8310_write_reg_data_error[i][1])
          ULOG_TAG_ERROR("[ist8310] init error, code %d", ist8310_write_reg_data_error[i][2]); // 掉线/写入失败/未知错误,会返回对应的错误码
    }

    while (1)
    {
      static uint8_t read[8]; 
      static float ist[3]; // 用于存储解码后的数据
      // 读取数据,读取6个字节,即3个轴的数据
      BSP_I2C_Mem_Write_Read(my_i2c_device, IST8310_DATA_REG, I2C_MEMADD_SIZE_8BIT, read, 6, 0);
      // 传输完成后会进入IST8310Decode函数进行数据解析
      int16_t temp[3];                                     // 用于存储解码后的数据

      memcpy(temp, read, 6 * sizeof(uint8_t)); // 不要强制转换,直接cpy
      for (uint8_t i = 0; i < 3; i++) {ist[i] = (float)temp[i] * 0.3f; }// 乘以灵敏度转换成uT(微特斯拉)
      tx_thread_sleep(100);
    }
}
/* USER CODE END  0 */
