/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-05 14:57:02
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-05 14:57:13
 * @FilePath: /threadx_learn/BSP/iic/bsp_iic.h
 * @Description: 
 */
#ifndef _BSP_IIC_H_
#define _BSP_IIC_H_

#include "i2c.h"
#include "tx_api.h"

#if defined(USE_COMPOENT_CONFIG_H) && USE_COMPOENT_CONFIG_H
#include "compoent_config.h"
#else
#define I2C_BUS_NUM 2                  // 总线数量
#define MAX_DEVICES_PER_I2C_BUS 4      // 每条总线最大设备数
#endif // _COMPOENT_CONFIG_H_

/* 传输模式枚举 */
typedef enum {
    I2C_MODE_BLOCKING,
    I2C_MODE_IT,
    I2C_MODE_DMA
} I2C_Mode;

/* I2C事件类型 */
typedef enum {
    I2C_EVENT_TX_COMPLETE,      // 发送完成
    I2C_EVENT_RX_COMPLETE,      // 接收完成
    I2C_EVENT_TX_RX_COMPLETE,   // 发送接收完成
    I2C_EVENT_ERROR             // 错误事件
} I2C_Event_Type;

/* I2C设备实例结构体 */
typedef struct {
    I2C_HandleTypeDef* hi2c;         // I2C句柄
    uint16_t dev_address;           // 设备地址
    I2C_Mode tx_mode;               // 发送模式
    I2C_Mode rx_mode;               // 接收模式
    uint32_t timeout;               // 超时时间
    void (*i2c_callback)(I2C_Event_Type event);    // 回调函数
} I2C_Device;

/* 初始化配置结构体 */
typedef struct {
    I2C_HandleTypeDef* hi2c;
    uint16_t dev_address;
    I2C_Mode tx_mode;
    I2C_Mode rx_mode;
    uint32_t timeout;
    void (*i2c_callback)(I2C_Event_Type event);    // 回调函数
} I2C_Device_Init_Config;

/* I2C总线管理结构 */
typedef struct {
    I2C_HandleTypeDef* hi2c;
    I2C_Device devices[MAX_DEVICES_PER_I2C_BUS];
    TX_MUTEX i2c_mutex;             // 互斥锁保护总线访问
    uint8_t device_count;           // 当前设备数量
    I2C_Device* active_dev;         // 当前活动设备
} I2C_Bus_Manager;

/**
 * @description:  初始化I2C设备
 * @param {I2C_Device_Init_Config*} config，初始化配置，见I2C_Device_Init_Config结构体
 * @return {I2C_Device*} 成功返回设备实例指针，失败返回NULL
 */
I2C_Device* BSP_I2C_Device_Init(I2C_Device_Init_Config* config);

/**
 * @description:  销毁I2C设备实例
 * @param {I2C_Device*} dev，要销毁的设备实例
 * @return {*}
 */
void BSP_I2C_Device_DeInit(I2C_Device* dev);

/**
 * @description: I2C发送和接受数据
 * @details      发送数据并接收响应
 * @param {I2C_Device*} dev，要操作的I2C设备实例
 * @param {uint8_t*} tx_data，发送数据指针
 * @param {uint8_t*} rx_data，接收数据指针
 * @param {uint16_t} size，数据大小
 * @return {HAL_StatusTypeDef} 返回操作状态
 */
HAL_StatusTypeDef BSP_I2C_TransReceive(I2C_Device* dev, uint8_t* tx_data, uint8_t* rx_data, uint16_t size);

/**
 * @description: I2C发送数据
 * @details      适用于单向发送数据
 * @param {I2C_Device*} dev，要操作的I2C设备实例
 * @param {uint8_t*} tx_data，发送数据指针
 * @param {uint16_t} size，数据大小
 * @return {HAL_StatusTypeDef} 返回操作状态
 */
HAL_StatusTypeDef BSP_I2C_Transmit(I2C_Device* dev, uint8_t* tx_data, uint16_t size);

/**
 * @description: I2C接收数据
 * @details      适用于单向接收数据
 * @param {I2C_Device*} dev，要操作的I2C设备实例
 * @param {uint8_t*} rx_data，接收数据指针
 * @param {uint16_t} size，数据大小
 * @return {HAL_StatusTypeDef} 返回操作状态
 */
HAL_StatusTypeDef BSP_I2C_Receive(I2C_Device* dev, uint8_t* rx_data, uint16_t size);

/**
 * @description: I2C内存读写操作
 * @details      适用于需要指定内存地址的读写操作
 * @param {I2C_Device*} dev，要操作的I2C设备实例
 * @param {uint16_t} mem_address，内存地址
 * @param {uint16_t} mem_add_size，内存地址大小
 * @param {uint8_t*} data，数据指针
 * @param {uint16_t} size，数据大小
 * @param {uint8_t} is_write，1表示写操作，0表示读操作
 * @return {HAL_StatusTypeDef} 返回操作状态
 */
HAL_StatusTypeDef BSP_I2C_Mem_Write_Read(I2C_Device* dev, uint16_t mem_address, 
                                        uint16_t mem_add_size, uint8_t* data, 
                                        uint16_t size, uint8_t is_write);

/**
 * @description: 检查I2C设备是否在线
 * @param {I2C_Device*} dev，要检查的I2C设备实例
 * @return {HAL_StatusTypeDef} 返回操作状态
 */
HAL_StatusTypeDef BSP_I2C_IsDeviceReady(I2C_Device* dev);

#endif // _BSP_IIC_H_