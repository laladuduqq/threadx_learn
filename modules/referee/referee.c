#include "referee.h"
#include "bsp_uart.h"
#include "compoent_config.h"
#include "crc_rm.h"
#include "offline.h"
#include "referee_protocol.h"
#include "systemwatch.h"
#include "tx_port.h"
#include "usart.h"
#include <stdint.h>
#include <string.h>
#include "tx_api.h"
 
#define LOG_TAG "referee"
#include "ulog.h"

#if REFEREE_ENABLE

static referee_info_t referee_info;			  // 裁判系统数据
uint8_t UI_Seq;
static TX_THREAD refereeTask_recv_thread;
static TX_THREAD refereeTask_send_thread;
static uint8_t referee_buf[2][REFEREE_UART_RX_BUF_SIZE];

void RefereeTask(ULONG thread_input);
void referee_send_task(ULONG thread_input);
void JudgeReadData(uint8_t *buff);
void DeterminRobotID(void);
void RefereeInit(TX_BYTE_POOL *pool)
{   
    OfflineDeviceInit_t offline_init = {
        .name = "referee",
        .timeout_ms = 500,
        .level = OFFLINE_LEVEL_HIGH,
        .beep_times = 6,
        .enable = OFFLINE_ENABLE
    };
    
    referee_info.offline_index = offline_device_register(&offline_init);

    UART_Device_init_config uart6_cfg = {
        .huart = &REFEREE_UART,
        .expected_len = 0,       // 不定长
        .rx_buf_size = REFEREE_UART_RX_BUF_SIZE,
        .rx_buf = (uint8_t (*)[2])referee_buf,
        .rx_mode = UART_MODE_DMA,
        .tx_mode = UART_MODE_DMA,
        .timeout = 1000,
        .rx_complete_cb = NULL,
        .cb_type = UART_CALLBACK_EVENT,
        .event_flag = UART_RX_DONE_EVENT
    };
    UART_Device* uart6 = BSP_UART_Device_Init(&uart6_cfg);
    referee_info.uart_device = uart6;

    // 用内存池分配监控线程栈
    CHAR *referee_recv_thread_stack;
    CHAR *referee_send_thread_stack;

    referee_send_thread_stack = threadx_malloc(REFEREE_SEND_THREAD_STACK_SIZE);
    referee_recv_thread_stack = threadx_malloc(REFEREE_RECV_THREAD_STACK_SIZE);
    if (referee_send_thread_stack == NULL || referee_recv_thread_stack == NULL) {
        ULOG_TAG_ERROR("Failed to allocate memory for referee task stacks!");
        return;
    }

    UINT status = tx_thread_create(&refereeTask_recv_thread, "refereeTask", RefereeTask, 0, 
                                   referee_recv_thread_stack, REFEREE_RECV_THREAD_STACK_SIZE, 
                                   REFEREE_RECV_THREAD_PRIORITY, REFEREE_RECV_THREAD_PRIORITY, 
                                   TX_NO_TIME_SLICE, TX_AUTO_START);
    status = tx_thread_create(&refereeTask_send_thread, "refereesendTask", referee_send_task, 0, 
                              referee_send_thread_stack, REFEREE_SEND_THREAD_STACK_SIZE, 
                              REFEREE_SEND_THREAD_PRIORITY, REFEREE_SEND_THREAD_PRIORITY, 
                              TX_NO_TIME_SLICE, TX_AUTO_START);

    if(status != TX_SUCCESS) {
        ULOG_TAG_ERROR("Failed to create referee task!");
        return;
    }
}

void RefereeTask(ULONG thread_input)
{ 
        (void)thread_input;
        while (1) {
            ULONG actual_flags = 0;
            // 等待事件标志，自动清除
            UINT status = tx_event_flags_get(
                &referee_info.uart_device->rx_event, // 事件组
                UART_RX_DONE_EVENT,
                TX_OR_CLEAR,      // 自动清除
                &actual_flags,
                TX_WAIT_FOREVER   // 无限等待
            );
            if ((status == TX_SUCCESS) && (actual_flags & UART_RX_DONE_EVENT)) {
                JudgeReadData(*referee_info.uart_device->rx_buf);
            }
        }
}

void referee_send_task(ULONG thread_input){
    (void)thread_input;
    SYSTEMWATCH_REGISTER_TASK(&refereeTask_send_thread, "referee_send_task");
    while (1) 
    {
        SYSTEMWATCH_UPDATE_TASK(&refereeTask_send_thread);
        // ui绘制和机器人车间通信
        tx_thread_sleep(100); 
    }
}

 /**
  * @brief  读取裁判数据,中断中读取保证速度
  * @param  buff: 读取到的裁判系统原始数据
  * @retval 是否对正误判断做处理
  * @attention  在此判断帧头和CRC校验,无误再写入数据，不重复判断帧头
  */
void JudgeReadData(uint8_t *buff)
{
    uint16_t judge_length; // 统计一帧数据长度
    if (buff == NULL)	   // 空数据包，则不作任何处理
        return;
 
    // 写入帧头数据(5-byte),用于判断是否开始存储裁判数据
    memcpy(&referee_info.FrameHeader, buff, LEN_HEADER);
 
    // 判断帧头数据(0)是否为0xA5
    if (buff[SOF] == REFEREE_SOF)
    {
        // 帧头CRC8校验
         if (Verify_CRC8_Check_Sum(buff, LEN_HEADER) == RM_TRUE)
        {
            offline_device_update(referee_info.offline_index);
             // 统计一帧数据长度(byte),用于CR16校验
             judge_length = buff[DATA_LENGTH] + LEN_HEADER + LEN_CMDID + LEN_TAIL;
             // 帧尾CRC16校验
             if (Verify_CRC16_Check_Sum(buff, judge_length) == RM_TRUE)
             {
                 // 2个8位拼成16位int
                referee_info.CmdID = (buff[6] << 8 | buff[5]);
                 // 解析数据命令码,将数据拷贝到相应结构体中(注意拷贝数据的长度)
                 // 第8个字节开始才是数据 data=7
                switch (referee_info.CmdID)
                {
                 case ID_game_state: // 0x0001
                    memcpy(&referee_info.GameState, (buff + DATA_Offset), LEN_game_state);
                    break;
                 case ID_game_result: // 0x0002
                    memcpy(&referee_info.GameResult, (buff + DATA_Offset), LEN_game_result);
                    break;
                 case ID_game_robot_survivors: // 0x0003
                    memcpy(&referee_info.GameRobotHP, (buff + DATA_Offset), LEN_game_robot_HP);
                    break;
                 case ID_event_data: // 0x0101
                    memcpy(&referee_info.EventData, (buff + DATA_Offset), LEN_event_data);
                    break;
                 case ID_supply_projectile_action: // 0x0102
                    memcpy(&referee_info.SupplyProjectileAction, (buff + DATA_Offset), LEN_supply_projectile_action);
                    break;
                 case ID_game_robot_state: // 0x0201
                    memcpy(&referee_info.GameRobotState, (buff + DATA_Offset), LEN_game_robot_state);
                    if (referee_info.init_flag == 0)
                    {
                        DeterminRobotID();
                        referee_info.init_flag=1;
                    }
                    break;
                 case ID_power_heat_data: // 0x0202
                    memcpy(&referee_info.PowerHeatData, (buff + DATA_Offset), LEN_power_heat_data);
                    break;
                 case ID_game_robot_pos: // 0x0203
                    memcpy(&referee_info.GameRobotPos, (buff + DATA_Offset), LEN_game_robot_pos);
                    break;
                 case ID_buff_musk: // 0x0204
                    memcpy(&referee_info.BuffMusk, (buff + DATA_Offset), LEN_buff_musk);
                    break;
                 case ID_aerial_robot_energy: // 0x0205
                    memcpy(&referee_info.AerialRobotEnergy, (buff + DATA_Offset), LEN_aerial_robot_energy);
                    break;
                 case ID_robot_hurt: // 0x0206
                    memcpy(&referee_info.RobotHurt, (buff + DATA_Offset), LEN_robot_hurt);
                    break;
                 case ID_shoot_data: // 0x0207
                    memcpy(&referee_info.ShootData, (buff + DATA_Offset), LEN_shoot_data);
                    break;
                 case ID_shoot_remaining:
                    memcpy(&referee_info.ext_shoot_remaing, (buff + DATA_Offset), LEN_shoot_remaing);
                    break;
                 case ID_rfid_status:
                    memcpy(&referee_info.rfid_status, (buff + DATA_Offset), LEN_rfid_status);
                    break;
                 case ID_ground_robot_position :
                    memcpy(&referee_info.ground_robot_position, (buff + DATA_Offset), LEN_ground_robot_position);
                    break;
                 case ID_sentry_info:
                    memcpy(&referee_info.sentry_info, (buff + DATA_Offset), LEN_sentry_info);
                    break;
                 default:
                    break;
                }
            }
        }
        // 首地址加帧长度,指向CRC16下一字节,用来判断是否为0xA5,从而判断一个数据包是否有多帧数据
        // 检查指针是否越界
        uint8_t* next_frame_ptr = buff + sizeof(xFrameHeader) + LEN_CMDID + referee_info.FrameHeader.DataLength + LEN_TAIL;
        // 确保下一个帧头指针在缓冲区内
        if ((next_frame_ptr >= buff) && 
            (next_frame_ptr < (buff + REFEREE_UART_RX_BUF_SIZE)) && (*next_frame_ptr == 0xA5))
        { 
            // 如果一个数据包出现了多帧数据,则再次调用解析函数,直到所有数据包解析完毕
            JudgeReadData(next_frame_ptr);
        }
    }
}

void RefereeSend(uint8_t *send, uint16_t tx_len){
    if (referee_info.uart_device != NULL) {
        BSP_UART_Send(referee_info.uart_device, send, tx_len);
    }
}
 
void DeterminRobotID(void)
{
    // id小于7是红色,大于7是蓝色,0为红色，1为蓝色   #define Robot_Red 0    #define Robot_Blue 1
    referee_info.referee_id.Robot_Color = referee_info.GameRobotState.robot_id > 7 ? Robot_Blue : Robot_Red;
    referee_info.referee_id.Robot_ID = referee_info.GameRobotState.robot_id;
    referee_info.referee_id.Cilent_ID = 0x0100 + referee_info.referee_id.Robot_ID; // 计算客户端ID
    referee_info.referee_id.Receiver_Robot_ID = 0;
}

const void* GetRefereeDataByCmd(CmdID_e cmd_id)
{
    switch(cmd_id)
    {
        case ID_game_state:
            return &referee_info.GameState;

        case ID_game_result:
            return &referee_info.GameResult;

        case ID_game_robot_survivors:
            return &referee_info.GameRobotHP;

        case ID_event_data:
            return &referee_info.EventData;

        case ID_supply_projectile_action:
            return &referee_info.SupplyProjectileAction;

        case ID_game_robot_state:
            return &referee_info.GameRobotState;

        case ID_power_heat_data:
            return &referee_info.PowerHeatData;

        case ID_game_robot_pos:
            return &referee_info.GameRobotPos;

        case ID_buff_musk:
            return &referee_info.BuffMusk;

        case ID_aerial_robot_energy:
            return &referee_info.AerialRobotEnergy;

        case ID_robot_hurt:
            return &referee_info.RobotHurt;

        case ID_shoot_data:
            return &referee_info.ShootData;

        case ID_shoot_remaining:
            return &referee_info.ext_shoot_remaing;

        case ID_rfid_status:
            return &referee_info.rfid_status;

        case ID_ground_robot_position:
            return &referee_info.ground_robot_position;

        case ID_sentry_info:
            return &referee_info.sentry_info;

        default:
            return NULL;
    }
}

#endif
