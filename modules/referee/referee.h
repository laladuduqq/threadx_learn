/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-06 15:04:58
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-06 15:32:26
 * @FilePath: /threadx_learn/modules/referee/referee.h
 * @Description: 
 */
#ifndef __REFEREE_H
#define __REFEREE_H

#include "bsp_uart.h"
#include "referee_protocol.h"
#include <stdint.h>

#if defined(USE_COMPOENT_CONFIG_H) && USE_COMPOENT_CONFIG_H
#include "compoent_config.h"
#else
#define REFEREE_ENABLE 1               // 开启裁判系统功能
#if REFEREE_ENABLE
    #define REFEREE_RECV_THREAD_STACK_SIZE 1024    // 裁判系统接收线程栈大小
    #define REFEREE_RECV_THREAD_PRIORITY 7         // 裁判系统接收线程优先级
    #define REFEREE_SEND_THREAD_STACK_SIZE 1024    // 裁判系统发送线程栈大小
    #define REFEREE_SEND_THREAD_PRIORITY 12        // 裁判系统发送线程优先级
#endif
#endif // _COMPOENT_CONFIG_H_

#pragma pack(1)
typedef struct
{
	uint8_t Robot_Color;		// 机器人颜色
	uint16_t Robot_ID;			// 本机器人ID
	uint16_t Cilent_ID;			// 本机器人对应的客户端ID
	uint16_t Receiver_Robot_ID; // 机器人车间通信时接收者的ID，必须和本机器人同颜色
} referee_id_t;

// 此结构体包含裁判系统接收数据以及UI绘制与机器人车间通信的相关信息
typedef struct
{
	referee_id_t referee_id;

	xFrameHeader FrameHeader; // 接收到的帧头信息
	uint16_t CmdID;
	ext_game_state_t GameState;							   // 0x0001
	ext_game_result_t GameResult;						   // 0x0002
	ext_game_robot_HP_t GameRobotHP;					   // 0x0003
	ext_event_data_t EventData;							   // 0x0101
	ext_supply_projectile_action_t SupplyProjectileAction; // 0x0102
	ext_game_robot_state_t GameRobotState;				   // 0x0201
	ext_power_heat_data_t PowerHeatData;				   // 0x0202
	ext_game_robot_pos_t GameRobotPos;					   // 0x0203
	ext_buff_musk_t BuffMusk;							   // 0x0204
	aerial_robot_energy_t AerialRobotEnergy;			   // 0x0205
	ext_robot_hurt_t RobotHurt;							   // 0x0206
	ext_shoot_data_t ShootData;							   // 0x0207
	ext_shoot_remaing_t ext_shoot_remaing;				   // 0x0208
	rfid_status_t rfid_status;							   // 0x0209
	ground_robot_position_t ground_robot_position;         // 0x020b
	sentry_info_t sentry_info;                             // 0x020d

	// 自定义交互数据的接收
	Communicate_ReceiveData_t ReceiveData;

	uint8_t init_flag;
	uint8_t offline_index; // 离线检测索引

	UART_Device *uart_device; // UART实例
} referee_info_t;

#pragma pack()

extern uint8_t UI_Seq;

#if REFEREE_ENABLE
#define REFEREE_INIT(pool) RefereeInit(pool)
#define REFEREE_SEND(send, tx_len) RefereeSend(send, tx_len)
#define GET_REFEREE_DATA_BY_CMD(cmd_id) GetRefereeDataByCmd(cmd_id)
#else  
#define REFEREE_INIT(pool) do {} while(0)
#define REFEREE_SEND(send, tx_len) do {} while(0)
#define GET_REFEREE_DATA_BY_CMD(cmd_id) NULL
#endif


/**
 * @description: 初始化裁判系统
 * @param {TX_BYTE_POOL} *pool
 * @return {*}
 */
void RefereeInit(TX_BYTE_POOL *pool);
/**
 * @description: 发送裁判系统数据
 * @param {uint8_t*} send 发送的数据指针
 * @param {uint16_t} tx_len 发送数据长度
 * @return {*}
 */
void RefereeSend(uint8_t *send, uint16_t tx_len);
/**
 * @brief 根据命令码获取对应的裁判系统数据
 * @param cmd_id 命令码
 * @return const void* 返回对应数据的指针，如果命令码无效则返回NULL
 */
const void* GetRefereeDataByCmd(CmdID_e cmd_id);

#endif 
