/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-05 16:04:23
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-05 16:07:13
 * @FilePath: /threadx_learn/BSP/CAN/bsp_can.h
 * @Description: 
 */
#ifndef BSP_CAN_H
#define BSP_CAN_H

#include "can.h"
#include "tx_api.h"
#include <stdint.h>


#if defined(USE_COMPOENT_CONFIG_H) && USE_COMPOENT_CONFIG_H
#include "compoent_config.h"
#else
#define CAN_BUS_NUM 2            // 总线数量
#define MAX_DEVICES_PER_BUS  8  // 每总线最大设备数
#define CAN_SEND_RETRY_CNT  3  // 重试次数
#define CAN_SEND_TIMEOUT_US 100 
#endif // _COMPOENT_CONFIG_H_


/* 接收模式枚举 */
typedef enum {
    CAN_MODE_BLOCKING,
    CAN_MODE_IT
} CAN_Mode;

/* CAN设备实例结构体 */
typedef struct
{
    CAN_HandleTypeDef *can_handle;  // CAN句柄
    CAN_TxHeaderTypeDef txconf;     // 发送配置
    uint32_t tx_id;                 // 发送ID
    uint32_t tx_mailbox;            // 发送邮箱号
    uint8_t tx_buff[8];             // 发送缓冲区

    uint32_t rx_id;                 // 接收ID
    uint8_t rx_buff[8];             // 接收缓冲区
    uint8_t rx_len;                 // 接收长度

    CAN_Mode tx_mode;
    CAN_Mode rx_mode;

    void (*can_callback)(const CAN_HandleTypeDef* hcan, const uint32_t rx_id); // 接收回调
} Can_Device;

/* 初始化配置结构体 */
typedef struct
{
    CAN_HandleTypeDef *can_handle;
    uint32_t tx_id;
    uint32_t rx_id;
    CAN_Mode tx_mode;
    CAN_Mode rx_mode;
    void (*can_callback)(const CAN_HandleTypeDef* hcan, const uint32_t rx_id);
} Can_Device_Init_Config_s;

/* CAN总线管理结构 */
typedef struct {
    CAN_HandleTypeDef *hcan;
    Can_Device devices[MAX_DEVICES_PER_BUS];
    TX_MUTEX tx_mutex;
    uint8_t device_count;
} CANBusManager;

typedef struct
{
    CAN_HandleTypeDef *can_handle;
    CAN_TxHeaderTypeDef txconf;     // 发送配置
    uint32_t tx_mailbox;            // 发送邮箱号
    uint8_t tx_buff[8];             // 发送缓冲区
} CanMessage_t;

/* 公有函数声明 */
Can_Device* BSP_CAN_Device_Init(Can_Device_Init_Config_s *config);
uint8_t CAN_SendMessage(Can_Device *device, uint8_t len);
uint8_t CAN_SendMessage_hcan(CAN_HandleTypeDef *hcan, CAN_TxHeaderTypeDef *pHeader,
                                const uint8_t aData[], uint32_t *pTxMailbox,uint8_t len);

#endif // BSP_CAN_H
