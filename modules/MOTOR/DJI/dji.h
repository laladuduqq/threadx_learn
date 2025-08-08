/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-07 16:57:18
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-08 13:50:40
 * @FilePath: /threadx_learn/modules/MOTOR/DJI/dji.h
 * @Description: 
 */
#ifndef __DJI_H
#define __DJI_H

#include "bsp_can.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C"{
#endif

#include "motor_def.h"

#if defined(USE_COMPOENT_CONFIG_H) && USE_COMPOENT_CONFIG_H
#include "compoent_config.h"
#else
#define USE_DJI_MOTOR           // 启用大疆电机
#if defined (USE_DJI_MOTOR)
    #define DJI_MOTOR_CNT 8              //DJI电机数量
    /* 滤波系数设置为1的时候即关闭滤波 */
    #define SPEED_SMOOTH_COEF 0.9f      // 最好大于0.85
    #define CURRENT_SMOOTH_COEF 0.9f     // 必须大于0.9
    #define ECD_ANGLE_COEF_DJI 0.043945f // (360/8192),将编码器值转化为角度制
#endif
#endif // _COMPOENT_CONFIG_H_

#if defined (USE_DJI_MOTOR)
#define DJI_MOTOR_INIT(init_config) DJIMotorInit(init_config)
#define DJI_MOTOR_SET_REF(MOTOR,REF) DJIMotorSetRef(MOTOR,REF)
#define DJI_MOTOR_CHANGE_FEED(MOTOR,LOOP,TYPE) DJIMotorChangeFeed(MOTOR,LOOP,TYPE)
#define DJI_MOTOR_CONTROL() DJIMotorControl()
#define DJI_MOTOR_STOP(MOTOR) DJIMotorStop(MOTOR)
#define DJI_MOTOR_ENABLE(MOTOR) DJIMotorEnable(MOTOR)
#define DJI_MOTOR_OUTER_LOOP(MOTOR,OUTLOOP,LQR_CONFIG) DJIMotorOuterLoop(MOTOR,OUTLOOP,LQR_CONFIG)
#else
#define DJI_MOTOR_INIT(init_config) NULL
#define DJI_MOTOR_SET_REF(MOTOR,REF) do{} while(0);
#define DJI_MOTOR_CHANGE_FEED(MOTOR,LOOP,TYPE) do{} while(0);
#define DJI_MOTOR_CONTROL() do{} while(0);
#define DJI_MOTOR_STOP(MOTOR) do{} while(0);
#define DJI_MOTOR_ENABLE(MOTOR) do{} while(0);
#define DJI_MOTOR_OUTER_LOOP(MOTOR,OUTLOOP,LQR_CONFIG) do{} while(0);
#endif



/* DJI电机CAN反馈信息*/
typedef struct
{
    uint16_t last_ecd;        // 上一次读取的编码器值
    uint16_t ecd;             // 0-8191,刻度总共有8192格
    float angle_single_round; // 单圈角度
    float speed_rpm;          // 转速
    float speed_aps;          // 角速度,单位为:度/秒
    int16_t real_current;     // 实际电流
    uint8_t temperature;      // 温度 Celsius

    float total_angle;   // 总角度,注意方向
    int32_t total_round; // 总圈数,注意方向
} DJI_Motor_Measure_s;

/**
 * @brief DJI intelligent motor typedef
 *
 */
typedef struct
{
    DJI_Motor_Measure_s measure;            // 电机测量值
    Motor_Control_Setting_s motor_settings; // 电机设置
    Motor_Controller_s motor_controller;    // 电机控制器
    // 分组发送设置
    uint8_t sender_group;   // 分组发送组号
    uint8_t message_num;    // 分组发送消息数量
    Motor_Info_s DJI_Motor_Info;    //电机信息
    Motor_Working_Type_e stop_flag; // 启停标志
    uint8_t offline_index;  // 离线检测索引
    Can_Device *can_device; // CAN设备
} DJIMotor_t;

/**
 * @description: DJI电机初始化
 * @param {Motor_Init_Config_s} *config
 * @return {DJIMotor_t} *motor,返回电机指针
 */
DJIMotor_t *DJIMotorInit(Motor_Init_Config_s *config);
 /**
  * @description: DJI电机设置参考值
  * @param {DJIMotor_t} *motor
  * @return {*}
  */
void DJIMotorSetRef(DJIMotor_t *motor, float ref);
/**
 * @description: DJI电机修改对应闭环的反馈数据源
 * @param {DJIMotor_t} *motor，电机指针
 * @param {Closeloop_Type_e} loop，对应的闭环类型
 * @param {Feedback_Source_e} type，修改的反馈数据源
 * @return {*}
 */
void DJIMotorChangeFeed(DJIMotor_t *motor, Closeloop_Type_e loop, Feedback_Source_e type);
/**
 * @description: DJI电机控制
 * @param {*}
 * @return {*}
 */
void DJIMotorControl(void);
/**
 * @description: DJI电机停止
 * @param {DJIMotor_t} *motor，电机指针
 * @return {*}
 */
void DJIMotorStop(DJIMotor_t *motor);
/**
 * @description: DJI电机使能
 * @param {DJIMotor_t} *motor，电机指针
 * @return {*}
 */
void DJIMotorEnable(DJIMotor_t *motor);
/**
 * @description: DJI电机外环修改
 * @param {DJIMotor_t} *motor，电机指针
 * @param {Closeloop_Type_e} outer_loop，外环类型
 * @param {LQR_Init_Config_s} *lqr_config，LQR参数,如果不是lqr算法直接传入NULL即可
 * @return {*}
 */
void DJIMotorOuterLoop(DJIMotor_t *motor, Closeloop_Type_e outer_loop, LQR_Init_Config_s *lqr_config);




#ifdef __cplusplus
}
#endif

#endif // DJI_H
