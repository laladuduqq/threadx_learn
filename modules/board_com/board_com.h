/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-06 21:35:41
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-06 23:47:06
 * @FilePath: /threadx_learn/modules/board_com/board_com.h
 * @Description: 
 */
/*
 * @Author: laladuduqq 17503181697@163.com
 * @Date: 2025-06-06 23:35:26
 * @LastEditors: laladuduqq 17503181697@163.com
 * @LastEditTime: 2025-06-15 14:07:59
 * @FilePath: \rm_threadx\modules\board_com\board_com.h
 * @Description: 
 * 
 */
#ifndef __BOARD_COM_H
#define __BOARD_COM_H

#include "bsp_can.h"
#include "offline.h"
#include <stdint.h>
#include "robotdef.h"

#if defined(USE_COMPOENT_CONFIG_H) && USE_COMPOENT_CONFIG_H
#include "compoent_config.h"
#else
#define GIMBAL_ID 0X310
#define CHASSIS_ID 0X311
#endif // _COMPOENT_CONFIG_H_

#ifndef ONE_BOARD
#define BOARD_COM_INIT(config) board_com_init(config)
#define BOARD_COM_SEND(data) board_com_send(data);
#define BOARD_COM_GET_DATA() board_com_get_data()
#else
#define BOARD_COM_INIT(config) NULL
#define BOARD_COM_SEND(data) do{} while(0);
#define BOARD_COM_GET_DATA() NULL
#endif



// 板间通信结构体
typedef struct {
    Can_Device *candevice;
    uint8_t offlinemanage_index;
    #if defined(GIMBAL_BOARD)
    Chassis_referee_Upload_Data_s Chassis_referee_Upload_Data;
    #elif defined (CHASSIS_BOARD)
    Chassis_Ctrl_Cmd_s chassis_ctrl_cmd;
    #endif
} board_com_t;

typedef struct
{
    Can_Device_Init_Config_s Can_Device_Init_Config;
    OfflineDeviceInit_t offline_manage_init;
} board_com_init_t;

/**
 * @description: 板件通讯初始化
 * @param {board_com_init_t*} board_com_init
 * @return *board_com_t
 */
board_com_t *board_com_init(board_com_init_t* board_com_init);
/**
 * @description: 板件通讯发送函数
 * @param {uint8_t} *data
 * @param {uint8_t} size
 * @return {*}
 */
void board_com_send(uint8_t *data);
/**
 * @description: 获取板件通讯数据，注意返回的是数组指针
 * @return {*}
 */
void *board_com_get_data(void);

#endif // __BOARD_COM_H