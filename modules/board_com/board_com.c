#include "board_com.h"
#include "compoent_config.h"
#include "offline.h"
#include "robotdef.h"
#include "stm32f4xx_hal_def.h"
#include <string.h>


#define LOG_TAG              "board_com"
#include "ulog.h"

#ifndef ONE_BOARD

static board_com_t board_com;
static void board_recv(const CAN_HandleTypeDef* hcan, const uint32_t rx_id);

void board_com_init()
{
    static board_com_init_t board_com_config = {
        .offline_manage_init = {
            .name = "board_com",
            .timeout_ms = BOARD_COM_OFFLINE_MS,
            .level = OFFLINE_LEVEL_HIGH,
            .beep_times = BOARD_COM_OFFLINE_BEEP_TIMES,
            .enable = OFFLINE_ENABLE,
        },
        .Can_Device_Init_Config = {
            .can_handle = &BOARD_COM_CAN_BUS,
            #if defined (GIMBAL_BOARD)
            .tx_id = GIMBAL_ID,
            .rx_id = CHASSIS_ID,
            #elif defined (CHASSIS_BOARD)
            .tx_id = CHASSIS_ID,
            .rx_id = GIMBAL_ID,
            #endif
            .tx_mode = CAN_MODE_BLOCKING,
            .rx_mode = CAN_MODE_IT,
            .can_callback = board_recv
        }
    };
    
    // 初始化板间通讯的掉线检测
    board_com.offlinemanage_index = offline_device_register(&board_com_config.offline_manage_init);

    // CAN 设备初始化配置
    Can_Device *can_dev = BSP_CAN_Device_Init(&board_com_config.Can_Device_Init_Config);
    if (can_dev == NULL) {
        ULOG_TAG_ERROR("Failed to initialize CAN device for board_com");
    }
    board_com.candevice = can_dev;
}

board_com_t *get_board_com(){
    return &board_com;
}

void board_com_send(uint8_t *data)
{
    if (data == NULL) {
        ULOG_TAG_ERROR("board_com_send: NULL data pointer");
        return;
    }
    
    #if defined (CHASSIS_BOARD)
        Chassis_referee_Upload_Data_s *cmd =(Chassis_referee_Upload_Data_s *)data;
        // 编码裁判系统数据到8字节
        // Robot_Color: 0或1 (1位)
        // power_management_shooter_output: 0或1 (1位)
        // projectile_allowance_17mm: 0-1000 (10位)
        // current_hp_percent: 0-400 (9位)
        // outpost_HP: 0-1500 (11位)
        // base_HP: 0-5000 (13位)
        // game_progess: 0-5 (3位)

        // 第1字节
        board_com.candevice->tx_buff[0] = ((cmd->Robot_Color & 0x01) << 7) | 
                    ((cmd->power_management_shooter_output & 0x01) << 6) |
                    ((cmd->projectile_allowance_17mm >> 4) & 0x3F);
        // 第2字节
        board_com.candevice->tx_buff[1] = ((cmd->projectile_allowance_17mm & 0x0F) << 4) | 
                    ((cmd->current_hp_percent >> 5) & 0x0F);
        // 第3字节
        board_com.candevice->tx_buff[2] = ((cmd->current_hp_percent & 0x1F) << 3) | 
                    ((cmd->outpost_HP >> 8) & 0x07);
        // 第4字节
        board_com.candevice->tx_buff[3] = cmd->outpost_HP & 0xFF;
        // 第5字节
        board_com.candevice->tx_buff[4] = (cmd->base_HP >> 5) & 0xFF;
        // 第6字节
        board_com.candevice->tx_buff[5] = ((cmd->base_HP & 0x1F) << 3) | 
                    ((cmd->game_progess & 0x07) << 0);
        // 第7、8字节保留
        board_com.candevice->tx_buff[6] = 0;
        board_com.candevice->tx_buff[7] = 0;

    #elif defined (GIMBAL_BOARD)
        Chassis_Ctrl_Cmd_s *cmd =(Chassis_Ctrl_Cmd_s *)data;
        // 将float类型的vx, vy, wz, offset_angle压缩
        // 速度范围为-10.0 到 10.0 m/s，角度范围为-720 到 720度
        // 使用int8_t类型存储vx,vy,wz，int16_t存储offset_angle，chassis_mode使用4位
        int8_t vx_int = (int8_t)(cmd->vx * 10);         // 精确到0.1 m/s (-10.0到10.0)
        int8_t vy_int = (int8_t)(cmd->vy * 10);         // 精确到0.1 m/s (-10.0到10.0)
        int8_t wz_int = (int8_t)(cmd->wz * 10);         // 精确到0.1 rad/s (-10.0到10.0)
        int16_t angle_int = (int16_t)(cmd->offset_angle * 10); // 精确到0.1度 (-720.0到720.0)
        
        // 将数据打包到8字节中 (总共64位)
        board_com.candevice->tx_buff[0] = vx_int;                               // vx 8位
        board_com.candevice->tx_buff[1] = vy_int;                               // vy 8位
        board_com.candevice->tx_buff[2] = wz_int;                               // wz 8位
        board_com.candevice->tx_buff[3] = (angle_int >> 8) & 0xFF;              // offset_angle高8位
        board_com.candevice->tx_buff[4] = angle_int & 0xFF;                     // offset_angle低8位
        board_com.candevice->tx_buff[5] = (cmd->chassis_mode & 0x0F) << 4;      // chassis_mode 4位 (高4位)
        board_com.candevice->tx_buff[6] = 0;                                    // 保留
        board_com.candevice->tx_buff[7] = 0;                                    // 保留
    #endif

    
    // 发送前检查CAN设备状态
    if (board_com.candevice != NULL) {
        CAN_SendMessage(board_com.candevice, 8);
    } else {
        ULOG_TAG_ERROR("CAN device is NULL, cannot send message");
        return;
    }
}

static void board_recv(const CAN_HandleTypeDef* hcan, const uint32_t rx_id)
{
    UNUSED(hcan);
    UNUSED(rx_id);

    // 检查CAN设备是否有效
    if (board_com.candevice == NULL) {
        ULOG_TAG_ERROR("CAN device is NULL");
        return;
    }

    #if defined (CHASSIS_BOARD)
    if (rx_id == GIMBAL_ID) {
        // 从8字节中解包数据
        int8_t vx_int = (int8_t)board_com.candevice->rx_buff[0];
        int8_t vy_int = (int8_t)board_com.candevice->rx_buff[1];
        int8_t wz_int = (int8_t)board_com.candevice->rx_buff[2];
        int16_t angle_int = (int16_t)((board_com.candevice->rx_buff[3] << 8) | board_com.candevice->rx_buff[4]);
        // 将整数还原为float类型
        board_com.chassis_ctrl_cmd.vx = vx_int / 10.0f;             // 精确到0.1 m/s
        board_com.chassis_ctrl_cmd.vy = vy_int / 10.0f;             // 精确到0.1 m/s
        board_com.chassis_ctrl_cmd.wz = wz_int / 10.0f;             // 精确到0.1 rad/s
        board_com.chassis_ctrl_cmd.offset_angle = angle_int / 10.0f; // 精确到0.1度
        // 提取底盘模式
        board_com.chassis_ctrl_cmd.chassis_mode = (chassis_mode_e)((board_com.candevice->rx_buff[5] >> 4) & 0x0F);
    }
    #elif defined (GIMBAL_BOARD)
    if (rx_id == CHASSIS_ID) {
        // 从8字节中解码裁判系统数据
        // Robot_Color: 0或1 (1位)
        board_com.Chassis_referee_Upload_Data.Robot_Color = (board_com.candevice->rx_buff[0] >> 7) & 0x01;
        // power_management_shooter_output: 0或1 (1位)
        board_com.Chassis_referee_Upload_Data.power_management_shooter_output = (board_com.candevice->rx_buff[0] >> 6) & 0x01;
        // projectile_allowance_17mm: 0-1000 (10位)
        board_com.Chassis_referee_Upload_Data.projectile_allowance_17mm = ((uint16_t)(board_com.candevice->rx_buff[0] & 0x3F) << 4) | 
                                        ((uint16_t)(board_com.candevice->rx_buff[1] >> 4) & 0x0F);
        // current_hp_percent: 0-400 (9位)
        board_com.Chassis_referee_Upload_Data.current_hp_percent = ((uint16_t)(board_com.candevice->rx_buff[1] & 0x0F) << 5) | 
                                ((uint16_t)(board_com.candevice->rx_buff[2] >> 3) & 0x1F);
        // outpost_HP: 0-1500 (11位)
        board_com.Chassis_referee_Upload_Data.outpost_HP = ((uint16_t)(board_com.candevice->rx_buff[2] & 0x07) << 8) | 
                        (uint16_t)board_com.candevice->rx_buff[3];
        // base_HP: 0-5000 (13位)
        board_com.Chassis_referee_Upload_Data.base_HP = ((uint16_t)(board_com.candevice->rx_buff[4] & 0xFF) << 5) | 
                        ((uint16_t)(board_com.candevice->rx_buff[5] >> 3) & 0x1F);
        // game_progess: 0-5 (3位)
        board_com.Chassis_referee_Upload_Data.game_progess = board_com.candevice->rx_buff[5] & 0x07;
    }
    #endif    
    
    // 更新掉线检测
    offline_device_update(board_com.offlinemanage_index);
}

void *board_com_get_data(void)
{
    #if defined (CHASSIS_BOARD)
        return &board_com.chassis_ctrl_cmd;
    #elif defined (GIMBAL_BOARD)
        return &board_com.Chassis_referee_Upload_Data;
    #else
        return NULL;
    #endif
}

#endif