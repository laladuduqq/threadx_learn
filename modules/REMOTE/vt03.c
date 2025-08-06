#include "vt03.h"
#include "bsp_uart.h"
#include "crc_rm.h"
#include "offline.h"
#include <stdint.h>


#define LOG_TAG  "VT03"
#include "ulog.h"

#define VT03_FRAME_LENGTH 21

static vt03_info_t vt03_info;
static uint8_t VT03_buf[2][REMOTE_UART_RX_BUF_SIZE];

void vt03_callback(uint8_t *buff, uint16_t len)
{
    //数据验证
    if (buff == NULL) {return;}
    if (buff[0] == 0XA9 && buff[1] == 0X53 && len == VT03_FRAME_LENGTH)
    {
        if (Verify_CRC16_Check_Sum(buff, VT03_FRAME_LENGTH) == RM_TRUE)
        {
            vt03_info.vt03_remote_data.frame_header1 = buff[0];
            vt03_info.vt03_remote_data.frame_header2 = buff[1];
            // 保存上一次的开关和按键状态
            vt03_info.vt03_remote_data.button_last = vt03_info.vt03_remote_data.button_current;
            vt03_info.vt03_remote_data.key_last = vt03_info.vt03_remote_data.key_current;

            // 更新通道数据
            vt03_info.vt03_remote_data.channels.ch0 = (buff[2] | (buff[3] << 8)) & 0x07FF;
            vt03_info.vt03_remote_data.channels.ch0 -= VT03_CH_VALUE_OFFSET;
            vt03_info.vt03_remote_data.channels.ch1 = ((buff[3] >> 3) | (buff[4] << 5)) & 0x07FF;
            vt03_info.vt03_remote_data.channels.ch1 -= VT03_CH_VALUE_OFFSET;
            vt03_info.vt03_remote_data.channels.ch2 = ((buff[4] >> 6) | (buff[5] << 2) | (buff[6] << 10)) & 0x07FF;
            vt03_info.vt03_remote_data.channels.ch2 -= VT03_CH_VALUE_OFFSET;
            vt03_info.vt03_remote_data.channels.ch3 = ((buff[6] >> 1) | (buff[7] << 7)) & 0x07FF;
            vt03_info.vt03_remote_data.channels.ch3 -= VT03_CH_VALUE_OFFSET;

            //防止数据零漂，设置正负VT03_DEAD_ZONE的死区
            if(abs(vt03_info.vt03_remote_data.channels.ch0) <= VT03_DEAD_ZONE) vt03_info.vt03_remote_data.channels.ch0 = 0;
            if(abs(vt03_info.vt03_remote_data.channels.ch1) <= VT03_DEAD_ZONE) vt03_info.vt03_remote_data.channels.ch1 = 0;
            if(abs(vt03_info.vt03_remote_data.channels.ch2) <= VT03_DEAD_ZONE) vt03_info.vt03_remote_data.channels.ch2 = 0;
            if(abs(vt03_info.vt03_remote_data.channels.ch3) <= VT03_DEAD_ZONE) vt03_info.vt03_remote_data.channels.ch3 = 0;

            //防止数据溢出
            if ((abs(vt03_info.vt03_remote_data.channels.ch0) > (VT03_CH_VALUE_MAX - VT03_CH_VALUE_MIN)/2) || \
                (abs(vt03_info.vt03_remote_data.channels.ch1) > (VT03_CH_VALUE_MAX - VT03_CH_VALUE_MIN)/2) || \
                (abs(vt03_info.vt03_remote_data.channels.ch2) > (VT03_CH_VALUE_MAX - VT03_CH_VALUE_MIN)/2) || \
                (abs(vt03_info.vt03_remote_data.channels.ch3) > (VT03_CH_VALUE_MAX - VT03_CH_VALUE_MIN)/2))
            {
                memset(&vt03_info.vt03_remote_data, 0, sizeof(vt03_remote_data_t));
                return;
            }

            // 更新拨轮数据
            vt03_info.vt03_remote_data.wheel = ((buff[8] >> 1) | (buff[9] << 7)) & 0x07FF;
            vt03_info.vt03_remote_data.wheel -= VT03_CH_VALUE_OFFSET;

            // 更新按键状态
            vt03_info.vt03_remote_data.button_current.bit.switch_pos = (buff[7] >> 4) & 0x03;
            vt03_info.vt03_remote_data.button_current.bit.pause = (buff[7] >> 6) & 0x01;
            vt03_info.vt03_remote_data.button_current.bit.custom_left = (buff[7] >> 7) & 0x01;
            vt03_info.vt03_remote_data.button_current.bit.custom_right = (buff[8] >> 0) & 0x01;
            vt03_info.vt03_remote_data.button_current.bit.trigger = (buff[9] >> 4) & 0x01;

            // 按键翻转锁存（custom_left, custom_right, trigger, pause）
            // custom_left
            if (vt03_info.vt03_remote_data.button_current.bit.custom_left && !vt03_info.vt03_remote_data.button_last.bit.custom_left) {
                vt03_info.vt03_remote_data.button_toggle[0] = !vt03_info.vt03_remote_data.button_toggle[0];
            }
            // custom_right
            if (vt03_info.vt03_remote_data.button_current.bit.custom_right && !vt03_info.vt03_remote_data.button_last.bit.custom_right) {
                vt03_info.vt03_remote_data.button_toggle[1] = !vt03_info.vt03_remote_data.button_toggle[1];
            }
            // trigger
            if (vt03_info.vt03_remote_data.button_current.bit.trigger && !vt03_info.vt03_remote_data.button_last.bit.trigger) {
                vt03_info.vt03_remote_data.button_toggle[2] = !vt03_info.vt03_remote_data.button_toggle[2];
            }
            // pause
            if (vt03_info.vt03_remote_data.button_current.bit.pause && !vt03_info.vt03_remote_data.button_last.bit.pause) {
                vt03_info.vt03_remote_data.button_toggle[3] = !vt03_info.vt03_remote_data.button_toggle[3];
            }

            // 更新鼠标数据
            vt03_info.vt03_remote_data.mouse.x = buff[10] | (buff[11] << 8);
            vt03_info.vt03_remote_data.mouse.y = buff[12] | (buff[13] << 8);
            vt03_info.vt03_remote_data.mouse.z = buff[14] | (buff[15] << 8);
            vt03_info.vt03_remote_data.mouse.left = buff[16] & 0x03;
            vt03_info.vt03_remote_data.mouse.right = (buff[16] >> 2) & 0x03;
            vt03_info.vt03_remote_data.mouse.middle = (buff[16] >> 4) & 0x03;

            // 鼠标数据边界检查
            if (vt03_info.vt03_remote_data.mouse.x < -32768 || vt03_info.vt03_remote_data.mouse.x > 32767) {
                vt03_info.vt03_remote_data.mouse.x = 0;
            }
            if (vt03_info.vt03_remote_data.mouse.y < -32768 || vt03_info.vt03_remote_data.mouse.y > 32767) {
                vt03_info.vt03_remote_data.mouse.y = 0;
            }
            if (vt03_info.vt03_remote_data.mouse.z < -32768 || vt03_info.vt03_remote_data.mouse.z > 32767) {
                vt03_info.vt03_remote_data.mouse.z = 0;
            }

            // 更新键盘数据
            vt03_info.vt03_remote_data.key_current.key_code = buff[17] | (buff[18] << 8);

            // 按键翻转锁存
            uint16_t changed = vt03_info.vt03_remote_data.key_current.key_code ^ vt03_info.vt03_remote_data.key_last.key_code;
            uint16_t pressed = vt03_info.vt03_remote_data.key_current.key_code & changed;
            vt03_info.vt03_remote_data.key_toggle ^= pressed;

            // 更新 CRC
            vt03_info.vt03_remote_data.crc16 = buff[19] | (buff[20] << 8);

            // 更新离线检测
            offline_device_update(vt03_info.offline_index);
        }
    }
}

vt03_info_t* Get_VT03_Data(void)
{
    return &vt03_info;
}

void VT03_init(UART_HandleTypeDef *huart){
    // 参数检查
    if (huart == NULL) {
        ULOG_TAG_ERROR("Invalid UART handle");
        return;
    }
    memset(&vt03_info,0,sizeof(vt03_info_t));
    
    HAL_UART_Init(huart);  // 重新初始化 UART

    OfflineDeviceInit_t offline_init = {
        .name = "VT03",
        .timeout_ms = 1000,
        .level = OFFLINE_LEVEL_HIGH,
        .beep_times = 0,
        .enable = OFFLINE_ENABLE,
    };
    vt03_info.offline_index = offline_device_register(&offline_init);

    UART_Device_init_config vt03_cfg = {
        .huart = huart,
        .expected_len = VT03_FRAME_LENGTH,      
        .rx_buf = (uint8_t (*)[2])VT03_buf,
        .rx_buf_size = REMOTE_UART_RX_BUF_SIZE,
        .rx_mode = UART_MODE_DMA,
        .tx_mode = UART_MODE_BLOCKING,
        .timeout = 1000,
        .rx_complete_cb = vt03_callback, 
        .cb_type = UART_CALLBACK_DIRECT,
        .event_flag = 0x01,
    };
    UART_Device *remote = BSP_UART_Device_Init(&vt03_cfg);
    if (remote != NULL)
    {
        vt03_info.uart_device = remote;
    }
    else
    {
        ULOG_TAG_ERROR("init uart failed");
        return;
    }
    
    ULOG_TAG_INFO("vt03 init success!");
}


