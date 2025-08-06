/*
 * @Author: laladuduqq 17503181697@163.com
 * @Date: 2025-06-18 10:32:47
 * @LastEditors: laladuduqq 17503181697@163.com
 * @LastEditTime: 2025-06-27 09:00:37
 * @FilePath: \rm_threadx\modules\REMOTE\vt02.h
 * @Description: 
 * 
 */
#ifndef VT02_H
#define VT02_H

#include "bsp_uart.h"
#include "common_remote.h"
#include "referee_protocol.h"
#include <stdint.h>

typedef struct{
    int16_t mouse_x;
    int16_t mouse_y;
    int16_t mouse_z;
    int8_t  left_button_down;
    int8_t  right_button_down;
    keyboard_state_t key_current;
    uint16_t reserved;
}vt02_remote_data_t;

typedef struct{
    xFrameHeader FrameHeader; // 接收到的帧头信息
	uint16_t CmdID;
    vt02_remote_data_t vt02_remote_data;
    keyboard_state_t key_last;
    uint16_t key_toggle;
    uint8_t offline_index; // 离线索引
    UART_Device *uart_device; // UART实例
}vt02_info_t;


/**
 * @description: 获取vt02 数据结构体指针
 * @return {vt02_info_t*} 指向vt02 数据的指针
 */
vt02_info_t* Get_VT02_Data(void);
/**
 * @description: 初始化 vt02 图传接收，配置 UART 并启动接收
 * @param {UART_HandleTypeDef*} huart UART句柄指针
 * @return {void}
 */
void vt02_init(UART_HandleTypeDef *huart);

#endif // VT02_H
