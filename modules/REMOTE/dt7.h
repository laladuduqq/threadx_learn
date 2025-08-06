/*
 * @Author: laladuduqq 17503181697@163.com
 * @Date: 2025-06-16 11:31:06
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-06 15:56:31
 * @FilePath: /threadx_learn/modules/REMOTE/dt7.h
 * @Description: 
 * 
 */
#ifndef DT7_H
#define DT7_H

#include "bsp_uart.h"
#include <stdint.h>
#include "common_remote.h"

#if defined(USE_COMPOENT_CONFIG_H) && USE_COMPOENT_CONFIG_H
#include "compoent_config.h"
#else
#define REMOTE_UART_RX_BUF_SIZE 30 // 定义遥控器接收缓冲区大小
#define DT7_DEAD_ZONE 5 // 定义遥控器死区范围
#endif // _COMPOENT_CONFIG_H_




#define DT7_CH_VALUE_MIN ((uint16_t)364)
#define DT7_CH_VALUE_OFFSET ((uint16_t)1024)
#define DT7_CH_VALUE_MAX ((uint16_t)1684)

#define DT7_SW_UP ((uint16_t)1)   // 开关向上时的值
#define DT7_SW_MID ((uint16_t)3)  // 开关中间时的值
#define DT7_SW_DOWN ((uint16_t)2) // 开关向下时的值
// 三个判断开关状态的宏
#define dt7_switch_is_down(s) (s == DT7_SW_DOWN)
#define dt7_switch_is_mid(s) (s == DT7_SW_MID)
#define dt7_switch_is_up(s) (s == DT7_SW_UP)



typedef struct {
    int16_t ch1;
    int16_t ch2;
    int16_t ch3;
    int16_t ch4;
    uint8_t sw1;
    uint8_t sw2;
    struct {
        int16_t x;
        int16_t y;
        int16_t z;
        uint8_t l;
        uint8_t r;
    } mouse;
    struct {
        keyboard_state_t last;
        keyboard_state_t current;
    } kb;
    uint16_t key_toggle;
    int16_t wheel;
} dt7_input_t;

typedef struct
{
  dt7_input_t dt7_input; 
  uint8_t offline_index; // 离线索引
  UART_Device *uart_device; // UART实例
}dt7_info_t;


/**
 * @description: 初始化 dt7 遥控器接收，配置 UART 并启动接收
 * @param {UART_HandleTypeDef*} huart UART句柄指针
 * @return {void}
 */
void dt7_init(UART_HandleTypeDef *huart);
/**
 * @description: dt7获取数据
 * @return {dt7_info_t*} 指向dt7数据的指针
 */
dt7_info_t* Get_DT7_Data(void);
#endif // DT7_H
