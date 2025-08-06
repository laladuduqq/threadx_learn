/*
 * @Author: laladuduqq 17503181697@163.com
 * @Date: 2025-06-18 11:10:43
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-06 15:51:18
 * @FilePath: /threadx_learn/modules/REMOTE/remote.c
 * @Description: 
 * 
 */
#include "remote.h"
#include <stdint.h>
#include <string.h>
#include "sbus.h"

#define LOG_TAG "remote"
#include "ulog.h"

static remote_info_t remote_info;

remote_info_t *get_remote_info(void)
{
    return &remote_info;
}

void remote_init(void)
{
    memset(&remote_info, 0, sizeof(remote_info_t));

    // 初始化遥控器
    #if defined(REMOTE_SOURCE) && (REMOTE_SOURCE == 0)
        sbus_init(&REMOTE_UART);  // SBUS初始化
        remote_info.remote.sbus = Get_SBUS_Data();
        remote_info.remote_type = REMOTE_TYPE_SBUS;
        ULOG_TAG_INFO("Using SBUS remote control");
    #elif defined(REMOTE_SOURCE) && (REMOTE_SOURCE == 1)
        dt7_init(&REMOTE_UART);     // DT7初始化
        remote_info.remote.dt7 = Get_DT7_Data();
        remote_info.remote_type = REMOTE_TYPE_DT7;
        ULOG_TAG_INFO("Using DT7 remote control");
    #else
        remote_info.remote_type = REMOTE_TYPE_NONE;
        ULOG_TAG_INFO("No traditional remote control");
    #endif

    // 初始化图传
    #if defined(REMOTE_VT_SOURCE) && (REMOTE_VT_SOURCE == 0)
        REMOTE_VT_UART.Init.BaudRate = 115200;
        vt02_init(&REMOTE_VT_UART);    // VT02初始化
        remote_info.vt.vt02 = Get_VT02_Data();
        remote_info.vt_type = VT_TYPE_VT02;
        ULOG_TAG_INFO("Using VT02 video transmission");
    #elif defined(REMOTE_VT_SOURCE) && (REMOTE_VT_SOURCE == 1)
        REMOTE_VT_UART.Init.BaudRate = 921600;
        VT03_init(&REMOTE_VT_UART);    // VT03初始化
        remote_info.vt.vt03 = Get_VT03_Data();
        remote_info.vt_type = VT_TYPE_VT03;
        ULOG_TAG_INFO("Using VT03 video transmission");
    #else
        remote_info.vt_type = VT_TYPE_NONE;
        ULOG_TAG_INFO("No video transmission");
    #endif
}


uint8_t remote_get_key_state(int i,uint8_t input_source)
{
    // 参数验证
    if (i < 0 || i >= KEY_COUNT) {
        return 0;
    }
    switch (input_source) 
    {
        case INPUT_FROM_DT7:
        {
            return (remote_info.remote.dt7->dt7_input.key_toggle >> i) & 0x01;
            break;
        }
        case INPUT_FROM_VT02:
        {
            return (remote_info.vt.vt02->key_toggle >> i) & 0x01;
            break;            
        }
        case INPUT_FROM_VT03:
        {
            return (remote_info.vt.vt03->vt03_remote_data.key_toggle >> i) & 0x01;
            break;
        }
    }
    return 0;
}

