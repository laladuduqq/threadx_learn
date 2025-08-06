#include "dt7.h"
#include "bsp_uart.h"
#include "offline.h"
#include <stdlib.h>

#define LOG_TAG  "dt7"
#include "ulog.h"

static dt7_info_t dt7_info;
static uint8_t dt7_buf[2][REMOTE_UART_RX_BUF_SIZE];

void dt7_callback(uint8_t *buf, uint16_t len)
{
    //长度检查
    if (len!=18) {return;}
    //检查缓冲区指针
    if (buf == NULL) {return;}
    //这里完成的是数据的分离和拼接，减去1024是为了让数据的中间值变为0
    dt7_info.dt7_input.ch1 = (buf[0] | buf[1] << 8) & 0x07FF;
    dt7_info.dt7_input.ch1 -= 1024;
    dt7_info.dt7_input.ch2 = (buf[1] >> 3 | buf[2] << 5) & 0x07FF;
    dt7_info.dt7_input.ch2 -= 1024;
    dt7_info.dt7_input.ch3 = (buf[2] >> 6 | buf[3] << 2 | buf[4] << 10) & 0x07FF;
    dt7_info.dt7_input.ch3 -= 1024;
    dt7_info.dt7_input.ch4 = (buf[4] >> 1 | buf[5] << 7) & 0x07FF;
    dt7_info.dt7_input.ch4 -= 1024;
    
    //防止数据零漂，设置正负DT7_DEAD_ZONE的死区
    if(abs(dt7_info.dt7_input.ch1) <= DT7_DEAD_ZONE) dt7_info.dt7_input.ch1 = 0;
    if(abs(dt7_info.dt7_input.ch2) <= DT7_DEAD_ZONE) dt7_info.dt7_input.ch2 = 0;
    if(abs(dt7_info.dt7_input.ch3) <= DT7_DEAD_ZONE) dt7_info.dt7_input.ch3 = 0;
    if(abs(dt7_info.dt7_input.ch4) <= DT7_DEAD_ZONE) dt7_info.dt7_input.ch4 = 0;
    
    dt7_info.dt7_input.sw1 = ((buf[5] >> 4) & 0x000C) >> 2;
    dt7_info.dt7_input.sw2 = (buf[5] >> 4) & 0x0003;
    
    //防止数据溢出
    if ((abs(dt7_info.dt7_input.ch1) > (DT7_CH_VALUE_MAX - DT7_CH_VALUE_MIN)/2) || \
        (abs(dt7_info.dt7_input.ch2) > (DT7_CH_VALUE_MAX - DT7_CH_VALUE_MIN)/2) || \
        (abs(dt7_info.dt7_input.ch3) > (DT7_CH_VALUE_MAX - DT7_CH_VALUE_MIN)/2) || \
        (abs(dt7_info.dt7_input.ch4) > (DT7_CH_VALUE_MAX - DT7_CH_VALUE_MIN)/2))
    {
        memset(&dt7_info.dt7_input, 0, sizeof(dt7_input_t));
        return;
    }

    // 鼠标数据边界检查
    if (dt7_info.dt7_input.mouse.x < -32768 || dt7_info.dt7_input.mouse.x > 32767) {dt7_info.dt7_input.mouse.x = 0;}
    if (dt7_info.dt7_input.mouse.y < -32768 || dt7_info.dt7_input.mouse.y > 32767) {dt7_info.dt7_input.mouse.y = 0;}
    if (dt7_info.dt7_input.mouse.z < -32768 || dt7_info.dt7_input.mouse.z > 32767) {dt7_info.dt7_input.mouse.z = 0;}

    dt7_info.dt7_input.mouse.x = buf[6] | (buf[7] << 8); // x axis
    dt7_info.dt7_input.mouse.y = buf[8] | (buf[9] << 8);
    dt7_info.dt7_input.mouse.z = buf[10] | (buf[11] << 8);
    dt7_info.dt7_input.mouse.l = buf[12];
    dt7_info.dt7_input.mouse.r = buf[13];

    // 更新按键状态
    dt7_info.dt7_input.kb.last.key_code = dt7_info.dt7_input.kb.current.key_code;  // 保存上一次状态
    dt7_info.dt7_input.kb.current.key_code = buf[14] | buf[15] << 8;     // 更新当前状态

    // 按键翻转锁存
    uint16_t changed = dt7_info.dt7_input.kb.current.key_code ^ dt7_info.dt7_input.kb.last.key_code;
    uint16_t pressed = dt7_info.dt7_input.kb.current.key_code & changed;
    dt7_info.dt7_input.key_toggle ^= pressed;
    
    dt7_info.dt7_input.wheel = (buf[16] | buf[17] << 8) - DT7_CH_VALUE_OFFSET;
    if(abs(dt7_info.dt7_input.wheel) <= DT7_DEAD_ZONE*10) dt7_info.dt7_input.wheel = 0;

    offline_device_update(dt7_info.offline_index);
}

dt7_info_t* Get_DT7_Data(void)
{
    return &dt7_info;
}

void dt7_init(UART_HandleTypeDef *huart){
    // 参数检查
    if (huart == NULL) {
        ULOG_TAG_ERROR("Invalid UART handle");
        return;
    }
    memset(&dt7_info,0,sizeof(dt7_info_t));

    HAL_UART_Init(huart);  // 重新初始化 UART

    OfflineDeviceInit_t offline_init = {
        .name = "dt7",
        .timeout_ms = 100,
        .level = OFFLINE_LEVEL_HIGH,
        .beep_times = 0,
        .enable = OFFLINE_ENABLE,
    };
    dt7_info.offline_index = offline_device_register(&offline_init);

    UART_Device_init_config dt7_cfg = {
        .huart = huart,
        .expected_len = 18,      
        .rx_buf = (uint8_t (*)[2])dt7_buf,
        .rx_buf_size = REMOTE_UART_RX_BUF_SIZE,
        .rx_mode = UART_MODE_DMA,
        .tx_mode = UART_MODE_BLOCKING,
        .timeout = 1000,
        .rx_complete_cb = dt7_callback, 
        .cb_type = UART_CALLBACK_DIRECT,
        .event_flag = 0x01,
    };
    UART_Device *remote = BSP_UART_Device_Init(&dt7_cfg);
    if (remote != NULL)
    {
        dt7_info.uart_device = remote;
    }
    else
    {
        ULOG_TAG_ERROR("init uart failed");
    }
    
    ULOG_TAG_INFO("dt7 init success!");
}
