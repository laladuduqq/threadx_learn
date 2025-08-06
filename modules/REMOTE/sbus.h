/*
 * @Author: laladuduqq 17503181697@163.com
 * @Date: 2025-06-06 21:25:00
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-06 16:06:59
 * @FilePath: /threadx_learn/modules/REMOTE/sbus.h
 * @Description: 
 * 
 */
#ifndef __SBUS_H
#define __SBUS_H
#include "bsp_uart.h"
#include <stdint.h>

#if defined(USE_COMPOENT_CONFIG_H) && USE_COMPOENT_CONFIG_H
#include "compoent_config.h"
#else
#define REMOTE_UART_RX_BUF_SIZE 30 // 定义遥控器接收缓冲区大小
#define SBUS_DEAD_ZONE 5 // 定义遥控器死区范围
#endif // _COMPOENT_CONFIG_H_


#define SBUS_CHX_BIAS ((uint16_t)1024)
#define SBUS_CHX_UP   ((uint16_t)240)
#define SBUS_CHX_DOWN ((uint16_t)1807)
// 三个判断开关状态的宏
#define sbus_switch_is_down(s) (s == SBUS_CHX_DOWN)
#define sbus_switch_is_mid(s) (s == SBUS_CHX_BIAS)
#define sbus_switch_is_up(s) (s == SBUS_CHX_UP)

typedef struct
{
    int16_t CH1;//1通道
    int16_t CH2;//2通道
    int16_t CH3;//3通道
    int16_t CH4;//4通道
    uint16_t CH5;//5通道
    uint16_t CH6;//6通道
    uint16_t CH7;//7通道
    uint16_t CH8;//8通道
    uint16_t CH9;//9通道
    uint16_t CH10;//10通道
    uint16_t CH11;//11通道
    uint16_t CH12;//12通道
    uint16_t CH13;//13通道
    uint16_t CH14;//14通道
    uint16_t CH15;//15通道
    uint16_t CH16;//16通道
    uint8_t ConnectState;   //连接的标志
}SBUS_CH_Struct;

typedef struct{
    SBUS_CH_Struct SBUS_CH;
    uint8_t offline_index; // 离线索引
    UART_Device *uart_device; // UART实例
}sbus_info_t;

/**
 * @description: 获取SBUS 数据结构体指针
 * @return {sbus_info_t*} 指向SBUS 数据的指针
 */
sbus_info_t* Get_SBUS_Data(void);
/**
 * @description: 初始化 SBUS 遥控器接收，配置 UART 并启动接收
 * @param {UART_HandleTypeDef*} huart UART句柄指针
 * @return {void}
 */
void sbus_init(UART_HandleTypeDef *huart);

#endif // SBUS_H
