#include "vt02.h"
#include "bsp_uart.h"
#include "crc_rm.h"
#include "offline.h"
#include <stdint.h>
#include <string.h>


#define LOG_TAG  "vt02"
#include "ulog.h"

static vt02_info_t vt02_info;
static uint8_t vt02_buf[2][REMOTE_UART_RX_BUF_SIZE];

void vt02_callback(uint8_t *buff,uint16_t len)
{
    (void)len;

    uint16_t judge_length; // 统计一帧数据长度
    if (buff == NULL)	   // 空数据包，则不作任何处理
        return;
 
    // 写入帧头数据(5-byte),用于判断是否开始存储裁判数据
    memcpy(&vt02_info.FrameHeader, buff, LEN_HEADER);
 
    // 判断帧头数据(0)是否为0xA5
    if (buff[SOF] == REFEREE_SOF)
    {
        // 帧头CRC8校验
        if (Verify_CRC8_Check_Sum(buff, LEN_HEADER) == RM_TRUE)
        {
            // 统计一帧数据长度(byte),用于CR16校验
            judge_length = buff[DATA_LENGTH] + LEN_HEADER + LEN_CMDID + LEN_TAIL;
            // 长度检查，防止缓冲区溢出
            if (judge_length > REMOTE_UART_RX_BUF_SIZE) {return;}
            // 帧尾CRC16校验
            if (Verify_CRC16_Check_Sum(buff, judge_length) == RM_TRUE)
            {
                // 2个8位拼成16位int
                vt02_info.CmdID = (buff[6] << 8 | buff[5]);
                // 解析数据命令码,将数据拷贝到相应结构体中(注意拷贝数据的长度)
                // 第8个字节开始才是数据 data=7
                if (vt02_info.CmdID == 0x0304) 
                {
                    vt02_info.key_last = vt02_info.vt02_remote_data.key_current;
                    memcpy(&vt02_info.vt02_remote_data, (buff + DATA_Offset), sizeof(vt02_remote_data_t));

                    // 按键翻转锁存
                    uint16_t changed = vt02_info.vt02_remote_data.key_current.key_code ^ vt02_info.key_last.key_code;
                    uint16_t pressed = vt02_info.vt02_remote_data.key_current.key_code & changed;
                    vt02_info.key_toggle ^= pressed;
                    
                    offline_device_update(vt02_info.offline_index);
                }
            }
        }
        // 首地址加帧长度,指向CRC16下一字节,用来判断是否为0xA5,从而判断一个数据包是否有多帧数据
        // 指针边界检查
        uint8_t* next_frame_ptr = buff + sizeof(xFrameHeader) + LEN_CMDID + vt02_info.FrameHeader.DataLength + LEN_TAIL;
        if ((next_frame_ptr >= buff) && 
            (next_frame_ptr < (buff + REMOTE_UART_RX_BUF_SIZE)) &&
            (*next_frame_ptr == 0xA5))
        { // 如果一个数据包出现了多帧数据,则再次调用解析函数,直到所有数据包解析完毕
            vt02_callback(next_frame_ptr, 0);
        }
    }
}

vt02_info_t* Get_VT02_Data(void)
{
    return &vt02_info;
}

void vt02_init(UART_HandleTypeDef *huart){
    // 参数检查
    if (huart == NULL) {
        ULOG_TAG_ERROR("Invalid UART handle");
        return;
    }

    memset(&vt02_info,0,sizeof(vt02_info_t));
    
    HAL_UART_Init(huart);  // 重新初始化 UART

    OfflineDeviceInit_t offline_init = {
        .name = "vt02",
        .timeout_ms = 100,
        .level = OFFLINE_LEVEL_HIGH,
        .beep_times = 0,
        .enable = OFFLINE_ENABLE,
    };
    vt02_info.offline_index = offline_device_register(&offline_init);

    UART_Device_init_config vt02_cfg = {
        .huart = huart,
        .expected_len = 18,      
        .rx_buf = (uint8_t (*)[2])vt02_buf,
        .rx_buf_size = REMOTE_UART_RX_BUF_SIZE,
        .rx_mode = UART_MODE_DMA,
        .tx_mode = UART_MODE_BLOCKING,
        .timeout = 1000,
        .rx_complete_cb = vt02_callback, 
        .cb_type = UART_CALLBACK_DIRECT,
        .event_flag = 0x01,
    };
    UART_Device *remote = BSP_UART_Device_Init(&vt02_cfg);
    if (remote != NULL)
    {
        vt02_info.uart_device = remote;
    }
    else
    {
        ULOG_TAG_ERROR("init uart failed");
        return;
    }
    
    ULOG_TAG_INFO("vt02 init success!");
}
