#ifndef __MOTOR_DEF_H
#define __MOTOR_DEF_H

#ifdef __cplusplus
extern "C"{
#endif

#include "controller.h"
#include "LQR.h"
#include "bsp_can.h"
#include "offline.h"

/**
 * @brief 闭环类型,如果需要多个闭环,则使用或运算
 *        例如需要速度环和电流环: CURRENT_LOOP|SPEED_LOOP
 */
typedef enum
{
    OPEN_LOOP = 0x0, // 0b0000
    CURRENT_LOOP = 0x1, // 0b0001 -> 1<<0
    SPEED_LOOP = 0x2, // 0b0010 -> 1<<1
    ANGLE_LOOP = 0x4, // 0b0100 -> 1<<2

    // 组合值保持原有逻辑
    SPEED_AND_CURRENT_LOOP = 0x3, // 0b0011
    ANGLE_AND_SPEED_LOOP = 0x6, // 0b0110
    ALL_THREE_LOOP = 0x7, // 0b0111
} Closeloop_Type_e;

/* 反馈来源设定,若设为OTHER_FEED则需要指定数据来源指针,详见Motor_Controller_s*/
typedef enum
{
    MOTOR_FEED = 0,
    OTHER_FEED,
} Feedback_Source_e;

/* 电机正反转标志 */
typedef enum
{
    MOTOR_DIRECTION_NORMAL = 0,
    MOTOR_DIRECTION_REVERSE = 1
} Motor_Reverse_Flag_e;

/* 反馈量正反标志 */
typedef enum
{
    FEEDBACK_DIRECTION_NORMAL = 0,
    FEEDBACK_DIRECTION_REVERSE = 1
} Feedback_Reverse_Flag_e;
/* 电机工作状态 */
typedef enum
{
    MOTOR_STOP = 0,
    MOTOR_ENALBED = 1,
} Motor_Working_Type_e;

/* 控制算法类型 */
typedef enum
{
    CONTROL_PID = 0,
    CONTROL_LQR,
    CONTROL_OTHER // 保留用于未来添加其他控制算法
} Control_Algorithm_Type_e;

/* 电机功率控制开关*/
typedef enum
{
    PowerControlState_OFF=0,
    PowerControlState_ON,
} PowerControlState_e;

/* 电机控制设置,包括闭环类型,反转标志和反馈来源 */
typedef struct
{
    Closeloop_Type_e outer_loop_type;              // 最外层的闭环,未设置时默认为最高级的闭环
    Closeloop_Type_e close_loop_type;              // 使用几个闭环(串级)
    Motor_Reverse_Flag_e motor_reverse_flag;       // 是否反转
    Feedback_Reverse_Flag_e feedback_reverse_flag; // 反馈是否反向
    Feedback_Source_e angle_feedback_source;       // 角度反馈类型
    Feedback_Source_e speed_feedback_source;       // 速度反馈类型
    Control_Algorithm_Type_e control_algorithm;    // 控制算法类型
    PowerControlState_e PowerControlState;         //功率开关

} Motor_Control_Setting_s;

/* 电机控制器,包括其他来源的反馈数据指针,3环控制器和电机的参考输入*/
typedef struct
{
    const float *other_angle_feedback_ptr; // 其他反馈来源的反馈数据指针
    const float *other_speed_feedback_ptr;

    PIDInstance speed_PID;
    PIDInstance angle_PID;
    LQRInstance lqr; // LQR控制器实例

    float ref; // 参考输入
} Motor_Controller_s;

/* 电机类型枚举 */
typedef enum
{
    MOTOR_TYPE_NONE = 0,
    GM6020_CURRENT,
    GM6020_VOLTAGE,
    M3508,
    M2006,
    DM4310,
    DM6220,
} Motor_Type_e;

/* 电机基本信息结构体，包含电机类型、减速比和扭矩常数等 */
typedef struct
{
    Motor_Type_e motor_type;        // 电机类型
    float gear_ratio;               // 减速比
    float torque_constant;          // 减速前扭矩常数 (Nm/A)
    float max_current;              // 最大电流 (A)
    float max_torque;               // 最大扭矩，减速前 (Nm)
    float max_speed;                // 最大转速，减速前 (rpm)
} Motor_Info_s;


/**
 * @brief 电机控制器初始化结构体,包括三环PID的配置以及两个反馈数据来源指针
 *        如果不需要某个控制环,可以不设置对应的pid config
 *        需要其他数据来源进行反馈闭环,不仅要设置这里的指针还需要在Motor_Control_Setting_s启用其他数据来源标志
 */
typedef struct
{
    const float *other_angle_feedback_ptr; // 角度反馈数据指针,注意电机使用total_angle
    const float *other_speed_feedback_ptr; // 速度反馈数据指针,单位为angle per sec

    PID_Init_Config_s speed_PID;
    PID_Init_Config_s angle_PID;

    LQR_Init_Config_s lqr_config; // LQR控制器初始化配置
} Motor_Controller_Init_s;

/* 用于初始化CAN电机的结构体,各类电机通用 */
typedef struct
{
    Motor_Controller_Init_s controller_param_init_config;
    Motor_Control_Setting_s controller_setting_init_config;
    Motor_Info_s Motor_init_Info;
    Can_Device_Init_Config_s can_init_config;
    OfflineDeviceInit_t offline_device_motor;
} Motor_Init_Config_s;

#ifdef __cplusplus
}
#endif

#endif // MOTOR_DEF_H
