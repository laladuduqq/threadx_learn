/*
 * @Author: laladuduqq 17503181697@163.com
 * @Date: 2025-06-18 09:10:28
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-06 16:08:06
 * @FilePath: /threadx_learn/modules/REMOTE/vt03.h
 * @Description: 
 * 
 */
#ifndef VT03_H
#define VT03_H

#include "bsp_uart.h"
#include <stdint.h>
#include "common_remote.h"


#if defined(USE_COMPOENT_CONFIG_H) && USE_COMPOENT_CONFIG_H
#include "compoent_config.h"
#else
#define REMOTE_UART_RX_BUF_SIZE 30 // 定义遥控器接收缓冲区大小
#define VT03_DEAD_ZONE 5 // 定义遥控器死区范围
#endif // _COMPOENT_CONFIG_H_

#define VT03_CH_VALUE_MIN ((uint16_t)364)
#define VT03_CH_VALUE_OFFSET ((uint16_t)1024)
#define VT03_CH_VALUE_MAX ((uint16_t)1684)


#define BUTTON_TOGGLE_CUSTOM_L   0
#define BUTTON_TOGGLE_CUSTOM_R   1
#define BUTTON_TOGGLE_TRIGGER    2
#define BUTTON_TOGGLE_PAUSE      3

/* 按键位定义 */
typedef union {
    uint8_t value;
    struct {
        uint8_t switch_pos : 2;    // 挡位切换开关 [0-C, 1-N, 2-S]
        uint8_t pause : 1;         // 暂停按键
        uint8_t custom_left : 1;   // 自定义按键(左)
        uint8_t custom_right : 1;  // 自定义按键(右)
        uint8_t trigger : 1;       // 扳机键
        uint8_t reserved : 2;      // 保留位，确保字节对齐
    } bit;
}button_state_t;

// 遥控器通信协议结构体
typedef struct {
    /* 帧头 */
    uint8_t frame_header1;     // 固定值 0xA9
    uint8_t frame_header2;     // 固定值 0x53

    /* 通道数据 */
    struct {
        int16_t ch0;     // 右摇杆水平方向 [364, 1024, 1684]
        int16_t ch1;     // 右摇杆竖直方向 [364, 1024, 1684]
        int16_t ch2;     // 左摇杆竖直方向 [364, 1024, 1684]
        int16_t ch3;     // 左摇杆水平方向 [364, 1024, 1684]
    } channels;

    /* 拨轮 */
    int16_t wheel;          // 拨轮位置 [364, 1024, 1684]

    /* 开关键 */
    button_state_t button_last;
    button_state_t button_current;
    uint8_t button_toggle[4];

    /* 鼠标数据 */
    struct {
        int16_t x;                // 鼠标X轴速度 [-32768, 0, 32767]
        int16_t y;                // 鼠标Y轴速度 [-32768, 0, 32767]
        int16_t z;                // 鼠标Z轴速度 [-32768, 0, 32767]
        uint8_t left : 2;         // 鼠标左键
        uint8_t right : 2;        // 鼠标右键
        uint8_t middle : 2;       // 鼠标中键
    } mouse;

    /* 键盘按键状态 */
    keyboard_state_t key_last;
    keyboard_state_t key_current;
    uint16_t key_toggle;

    uint16_t crc16;              // CRC-16 校验值
}vt03_remote_data_t;


typedef struct{
    vt03_remote_data_t vt03_remote_data;
    uint8_t offline_index; // 离线索引
    UART_Device *uart_device; // UART实例
}vt03_info_t;

/**
 * @description: 获取vt03 数据结构体指针
 * @return {vt03_info_t*} 指向vt03 数据的指针
 */
vt03_info_t* Get_VT03_Data(void);
/**
 * @description: 初始化 vt03 图传接收，配置 UART 并启动接收
 * @param {UART_HandleTypeDef*} huart UART句柄指针
 * @return {void}
 */
void VT03_init(UART_HandleTypeDef *huart);

#endif // VT03_H
