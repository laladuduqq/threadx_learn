/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-06 15:38:10
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-06 16:01:49
 * @FilePath: /threadx_learn/modules/REMOTE/sbus.c
 * @Description: 
 */
#include "sbus.h"
#include "bsp_uart.h"
#include "offline.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG  "sbus"
#include "ulog.h"

static sbus_info_t sbus_info;
static uint8_t sbus_buf[2][REMOTE_UART_RX_BUF_SIZE];

void sbus_callback(uint8_t *buf, uint16_t len)
{ 
    // 数据验证
    if(buf == NULL) {return;}
    // 长度验证
    if (len != 25) {return;}
    if(buf[0] == 0x0F && buf[24] == 0x00)
    {
        //1.获取原始数据 前四个通道减去SBUS_CHX_BIAS是为了让数据的中间值变为0
        sbus_info.SBUS_CH.CH1      = ((int16_t)buf[1] >> 0 | ((int16_t)buf[2] << 8)) & 0x07FF;
        sbus_info.SBUS_CH.CH1      -=SBUS_CHX_BIAS;
        sbus_info.SBUS_CH.CH2      = ((int16_t)buf[2] >> 3 | ((int16_t)buf[3] << 5)) & 0x07FF;
        sbus_info.SBUS_CH.CH2      -=SBUS_CHX_BIAS;
        sbus_info.SBUS_CH.CH3      = ((int16_t)buf[3] >> 6 | ((int16_t)buf[4] << 2) | (int16_t)buf[5] << 10) & 0x07FF;
        sbus_info.SBUS_CH.CH3      -=SBUS_CHX_BIAS;
        sbus_info.SBUS_CH.CH4      = ((int16_t)buf[5] >> 1 | ((int16_t)buf[6] << 7)) & 0x07FF;
        sbus_info.SBUS_CH.CH4      -=SBUS_CHX_BIAS;

        //防止数据零漂，设置正负SBUS_DEAD_ZONE的死区
        if(abs(sbus_info.SBUS_CH.CH1) <= SBUS_DEAD_ZONE) sbus_info.SBUS_CH.CH1 = 0;
        if(abs(sbus_info.SBUS_CH.CH2) <= SBUS_DEAD_ZONE) sbus_info.SBUS_CH.CH2 = 0;
        if(abs(sbus_info.SBUS_CH.CH3) <= SBUS_DEAD_ZONE) sbus_info.SBUS_CH.CH3 = 0;
        if(abs(sbus_info.SBUS_CH.CH4) <= SBUS_DEAD_ZONE) sbus_info.SBUS_CH.CH4 = 0;

        //通道数据边界检查
        if(sbus_info.SBUS_CH.CH1 < -1024 || sbus_info.SBUS_CH.CH1 > 1024) sbus_info.SBUS_CH.CH1 = 0;
        if(sbus_info.SBUS_CH.CH2 < -1024 || sbus_info.SBUS_CH.CH2 > 1024) sbus_info.SBUS_CH.CH2 = 0;
        if(sbus_info.SBUS_CH.CH3 < -1024 || sbus_info.SBUS_CH.CH3 > 1024) sbus_info.SBUS_CH.CH3 = 0;
        if(sbus_info.SBUS_CH.CH4 < -1024 || sbus_info.SBUS_CH.CH4 > 1024) sbus_info.SBUS_CH.CH4 = 0;


        sbus_info.SBUS_CH.CH5      = ((int16_t)buf[6] >> 4 | ((int16_t)buf[7] << 4)) & 0x07FF;
        sbus_info.SBUS_CH.CH6      = ((int16_t)buf[7] >> 7 | ((int16_t)buf[8] << 1) | (int16_t)buf[9] << 9) & 0x07FF;
        sbus_info.SBUS_CH.CH7      = ((int16_t)buf[9] >> 2 | ((int16_t)buf[10] << 6)) & 0x07FF;
        sbus_info.SBUS_CH.CH8      = ((int16_t)buf[10] >> 5 | ((int16_t)buf[11] << 3)) & 0x07FF;
        sbus_info.SBUS_CH.CH9      = ((int16_t)buf[12] << 0 | ((int16_t)buf[13] << 8)) & 0x07FF;
        sbus_info.SBUS_CH.CH10     = ((int16_t)buf[13] >> 3 | ((int16_t)buf[14] << 5)) & 0x07FF;
        sbus_info.SBUS_CH.CH11     = ((int16_t)buf[14] >> 6 | ((int16_t)buf[15] << 2) | (int16_t)buf[16] << 10) & 0x07FF;
        sbus_info.SBUS_CH.CH12     = ((int16_t)buf[16] >> 1 | ((int16_t)buf[17] << 7)) & 0x07FF;
        sbus_info.SBUS_CH.CH13     = ((int16_t)buf[17] >> 4 | ((int16_t)buf[18] << 4)) & 0x07FF;
        sbus_info.SBUS_CH.CH14     = ((int16_t)buf[18] >> 7 | ((int16_t)buf[19] << 1) | (int16_t)buf[20] << 9) & 0x07FF;
        sbus_info.SBUS_CH.CH15     = ((int16_t)buf[20] >> 2 | ((int16_t)buf[21] << 6)) & 0x07FF;
        sbus_info.SBUS_CH.CH16     = ((int16_t)buf[21] >> 5 | ((int16_t)buf[22] << 3)) & 0x07FF;
        sbus_info.SBUS_CH.ConnectState = buf[23];

        if (sbus_info.SBUS_CH.ConnectState == 0x00)
        {
            offline_device_update(sbus_info.offline_index);
        }
    }
}

sbus_info_t* Get_SBUS_Data(void)
{
    return &sbus_info;
}

void sbus_init(UART_HandleTypeDef *huart){
    // 参数检查
    if (huart == NULL) {
        ULOG_TAG_ERROR("Invalid UART handle");
        return;
    }
    memset(&sbus_info,0,sizeof(sbus_info_t));

    HAL_UART_Init(huart);  // 重新初始化 UART

    OfflineDeviceInit_t offline_init = {
        .name = "sbus",
        .timeout_ms = 100,
        .level = OFFLINE_LEVEL_HIGH,
        .beep_times = 0,
        .enable = OFFLINE_ENABLE,
    };
    sbus_info.offline_index = offline_device_register(&offline_init);

    UART_Device_init_config sbus_cfg = {
        .huart = huart,
        .expected_len = 25,      
        .rx_buf = (uint8_t (*)[2])sbus_buf,
        .rx_buf_size = REMOTE_UART_RX_BUF_SIZE,
        .rx_mode = UART_MODE_DMA,
        .tx_mode = UART_MODE_BLOCKING,
        .timeout = 1000,
        .rx_complete_cb = sbus_callback, 
        .cb_type = UART_CALLBACK_DIRECT,
        .event_flag = 0x01,
    };
    UART_Device *remote = BSP_UART_Device_Init(&sbus_cfg);
    if (remote != NULL)
    {
        sbus_info.uart_device = remote;
    }
    else
    {
        ULOG_TAG_ERROR("init uart failed");
    }
    
    ULOG_TAG_INFO("sbus init success!");
}
