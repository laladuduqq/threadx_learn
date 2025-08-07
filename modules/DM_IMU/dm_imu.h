/*
 * @Author: laladuduqq 17503181697@163.com
 * @Date: 2025-06-06 22:18:03
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-07 10:31:25
 * @FilePath: /threadx_learn/modules/DM_IMU/dm_imu.h
 * @Description: 
 * 
 */
#ifndef __DM_IMU_H
#define __DM_IMU_H

#include "bsp_can.h"

#if defined(USE_COMPOENT_CONFIG_H) && USE_COMPOENT_CONFIG_H
#include "compoent_config.h"
#else
#define DM_IMU_ENABLE 1
#if DM_IMU_ENABLE
#define DM_IMU_RX_ID 0x11                      // 达妙IMU的接收
#define DM_IMU_TX_ID 0x01                      // 达妙IMU的发送
#define DM_IMU_CAN_BUS hcan2                    // 达妙IMU使用的CAN总线
#define DM_IMU_OFFLINE_BEEP_TIMES 1             // 达妙IMU离线检测蜂鸣器响次
#define DM_IMU_OFFLINE_TIMEOUT_MS 100          // 达妙IMU离线检测超时时间（毫秒）
#define DM_IMU_THREAD_STACK_SIZE 1024           // 达妙IMU线程栈大小
#define DM_IMU_THREAD_PRIORITY 7                // 达妙IMU线程优先级
#endif
#endif // _COMPOENT_CONFIG_H_

#if DM_IMU_ENABLE
#define DM_IMU_INIT(pool) DM_IMU_Init(pool)
#define DM_IMU_GET_DATA() DM_IMU_GetData()
#else
#define DM_IMU_INIT(pool) do {} while(0)
#define DM_IMU_GET_DATA() NULL
#endif


#define ACCEL_CAN_MAX (58.8f)
#define ACCEL_CAN_MIN	(-58.8f)
#define GYRO_CAN_MAX	(34.88f)
#define GYRO_CAN_MIN	(-34.88f)
#define PITCH_CAN_MAX	(90.0f)
#define PITCH_CAN_MIN	(-90.0f)
#define ROLL_CAN_MAX	(180.0f)
#define ROLL_CAN_MIN	(-180.0f)
#define YAW_CAN_MAX		(180.0f)
#define YAW_CAN_MIN 	(-180.0f)
#define TEMP_MIN			(0.0f)
#define TEMP_MAX			(60.0f)
#define Quaternion_MIN	(-1.0f)
#define Quaternion_MAX	(1.0f)

#define DM_RID_ACCEL 1
#define DM_RID_GYRO  2
#define DM_RID_EULER 3
#define DM_RID_Quaternion 4

typedef struct
{
	float pitch;
	float roll;
	float yaw;

	float gyro[3];
	float accel[3];
	
	float q[4];

	float cur_temp;
    float yawlast;
    float yaw_round;
    float YawTotalAngle;
	Can_Device *can_device;
	uint8_t offline_index;
}dm_imu_t;

typedef struct {
    const float *Yaw;
    const float *Pitch; 
    const float *Roll;
    const float *YawTotalAngle;
    const float (*gyro)[3];    // 指向float[3]数组的指针
} DM_IMU_DATA_T;

/**
 * @description: 达妙imu数据请求函数
 * @param {uint16_t} can_id
 * @param {uint8_t} reg
 * @return {*}
 */
void DM_IMU_RequestData(uint16_t can_id,uint8_t reg);
/**
 * @description: 达妙imu初始化函数
 * @param {TX_BYTE_POOL} *pool
 * @return {*}
 */
void DM_IMU_Init(TX_BYTE_POOL *pool);
/**
 * @description: 获取达妙imu数据
 * @return {DM_IMU_DATA_T*} 指向DM_IMU_DATA_T数据的指针
 */
const DM_IMU_DATA_T* DM_IMU_GetData(void);


#endif
