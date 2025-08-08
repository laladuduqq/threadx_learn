<!--
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-08 09:56:42
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-08 10:00:13
 * @FilePath: /threadx_learn/modules/MOTOR/motor.md
 * @Description: 
-->

# 电机模块文档

## 概述

电机模块是机器人控制系统中的核心组件，负责控制各种类型的电机。该模块支持多种控制算法（如PID、LQR）和多种电机类型，具有良好的扩展性。

## 支持的电机类型

当前支持的电机类型包括：

- GM6020（电流控制模式）
- GM6020（电压控制模式）
- M3508
- M2006
- DM4310
- DM6220

## 核心概念

### 1. 控制回路类型

电机支持多种控制回路类型，可以通过位运算组合使用：

- OPEN_LOOP: 开环控制
- CURRENT_LOOP: 电流环控制
- SPEED_LOOP: 速度环控制
- ANGLE_LOOP: 角度环控制

示例：

```c
// 只使用速度环
motor_setting.close_loop_type = SPEED_LOOP;

// 使用速度环和电流环（串级控制）
motor_setting.close_loop_type = SPEED_LOOP | CURRENT_LOOP;
```

### 2. 控制算法

支持多种控制算法：

- CONTROL_PID: PID控制
- CONTROL_LQR: LQR控制
- CONTROL_OTHER: 其他控制算法（预留）

### 3. 反馈源

电机反馈可以来自：

- MOTOR_FEED: 电机自身编码器反馈
- OTHER_FEED: 外部反馈（如IMU等）

## 使用方法

### 1. 初始化电机（LQR算法）

```c
 Motor_Init_Config_s yaw_config = {
            .offline_device_motor ={
              .name = "6020_big",                        // 设备名称
              .timeout_ms = 100,                              // 超时时间
              .level = OFFLINE_LEVEL_HIGH,                     // 离线等级
              .beep_times = 2,                                // 蜂鸣次数
              .enable = OFFLINE_ENABLE,                       // 是否启用离线管理
            },
            .can_init_config = {
                .can_handle = &hcan2,
                .tx_id = 1,
            },
            .controller_param_init_config = {
                .other_angle_feedback_ptr = dm_imu_data->YawTotalAngle,
                .other_speed_feedback_ptr = &((*dm_imu_data->gyro)[2]),
                .lqr_config ={
                    .K ={22.36f,4.05f},
                    .output_max = 2.223,
                    .output_min =-2.223,
                    .state_dim = 2,
                    .compensation_type =COMPENSATION_NONE,
                }
            },
            .controller_setting_init_config = {
                .control_algorithm = CONTROL_LQR,
                .feedback_reverse_flag =FEEDBACK_DIRECTION_NORMAL,
                .angle_feedback_source =OTHER_FEED,
                .speed_feedback_source =OTHER_FEED,
                .outer_loop_type = ANGLE_LOOP,
                .close_loop_type = ANGLE_LOOP | SPEED_LOOP,
            },
            .Motor_init_Info ={
              .motor_type = GM6020_CURRENT,
              .max_current = 3.0f,
              .gear_ratio = 1,
              .max_torque = 2.223,
              .max_speed = 320,
              .torque_constant = 0.741f
            }

        };
        big_yaw = DJIMotorInit(&yaw_config);
```

### 2.初始化电机（pid算法）

```c
Motor_Init_Config_s pitch_config = {
        .offline_manage_motor ={
            .event = OFFLINE_GIMBAL_PITCH,
            .enable = OFFLINE_ENABLE,
            .online_state = STATE_OFFLINE,
            .error_level =OFFLINE_ERROR_LEVEL,
            .beep_times = 2,
            .last_time =rt_tick_get(),
            .offline_time =1000,
        },
        .can_init_config = {
            .can_handle = &can1_bus,
            .tx_id = 0X23,
            .rx_id = 0X206,
        },
        .controller_param_init_config = {
            .angle_PID = {
                .Kp = 0.01, // 8
                .Ki = 0,
                .Kd = 0,
                .DeadBand = 0.1,
                .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit,
                .IntegralLimit = 0,
                .MaxOut = 100,
            },
            .speed_PID = {
                .Kp = 0.3,  // 50
                .Ki = 0, // 200
                .Kd = 0,
                .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit,
                .IntegralLimit = 100,
                .MaxOut = 0.2,
            },
            .other_angle_feedback_ptr = &INS.Pitch,
            .other_speed_feedback_ptr = &INS.Gyro[0],
        },
        .controller_setting_init_config = {
            .angle_feedback_source = MOTOR_FEED,
            .speed_feedback_source = MOTOR_FEED,
            .outer_loop_type = ANGLE_LOOP,
            .close_loop_type = ANGLE_LOOP | SPEED_LOOP,
            .motor_reverse_flag = MOTOR_DIRECTION_NORMAL,
        },
        .Motor_init_Info ={
              .motor_type = DM4310,
              .max_current = 7.5f,
              .gear_ratio = 10,
              .max_torque = 7,
              .max_speed = 200,
              .torque_constant = 0.093f
            }

};
pitch_motor = DMMotorInit(&pitch_config,MIT_MODE);
```

### 3. 设置电机参考值

```c
// 设置目标速度，以速度环为例
DJI_MOTOR_SET_REF(motor, 100.0f); // 100 RPM
```

### 4. 控制电机启停

```c
// 启用电机
DJI_MOTOR_ENABLE(motor);

// 停止电机
DJI_MOTOR_STOP(motor);
```

### 5. 电机控制任务

```c
void motortask(ULONG thread_input)
{
    for (;;)
    {
        DJI_MOTOR_CONTROL();  // 执行电机控制
        DM_MOTOR_CONTROL();
        tx_thread_sleep(2);   // 控制周期
    }
}
```

## 添加新类型电机的方法

要添加新类型的电机，需要按照以下步骤进行：

### 1. 在枚举中添加新电机类型

在motor_def.h文件中的`Motor_Type_e`枚举中添加新的电机类型：

```c
typedef enum
{
    MOTOR_TYPE_NONE = 0,
    GM6020_CURRENT,
    GM6020_VOLTAGE,
    M3508,
    M2006,
    DM4310,        
    DM6220,        
    YOUR_NEW_MOTOR // 新增的电机类型
} Motor_Type_e;
```

### 2. 实现电机控制逻辑

为新电机创建一个新目录，例如`MOTOR/YOUR_MOTOR/`，并实现以下文件：

#### your_motor.h

```c
#ifndef __YOUR_MOTOR_H
#define __YOUR_MOTOR_H

#include "motor_def.h"

#if defined(USE_COMPOENT_CONFIG_H) && USE_COMPOENT_CONFIG_H
#include "compoent_config.h"
#else
#define USE_YOUR_MOTOR           
#if defined (USE_YOUR_MOTOR)
    #define YOUR_MOTOR_CNT 4  // 定义最大电机数量
#endif
#endif // _COMPOENT_CONFIG_H

// 定义宏以便统一调用
#if defined (USE_YOUR_MOTOR)
#define YOUR_MOTOR_INIT(init_config) YourMotorInit(init_config)
#define YOUR_MOTOR_SET_REF(MOTOR,REF) YourMotorSetRef(MOTOR,REF)
#define YOUR_MOTOR_CONTROL() YourMotorControl()
#define YOUR_MOTOR_STOP(MOTOR) YourMotorStop(MOTOR)
#define YOUR_MOTOR_ENABLE(MOTOR) YourMotorEnable(MOTOR)
#else
#define YOUR_MOTOR_INIT(init_config) NULL
#define YOUR_MOTOR_SET_REF(MOTOR,REF) do{} while(0);
#define YOUR_MOTOR_CONTROL() do{} while(0);
#define YOUR_MOTOR_STOP(MOTOR) do{} while(0);
#define YOUR_MOTOR_ENABLE(MOTOR) do{} while(0);
#endif_

// 定义新电机特定的结构体
typedef struct
{
    // 新电机特定的属性
} YourMotor_Measure_s;

typedef struct
{
    YourMotor_Measure_s measure;
    Motor_Control_Setting_s motor_settings;
    Motor_Controller_s motor_controller;
    // 其他字段
    Motor_Info_s motor_info;
    Motor_Working_Type_e stop_flag;
    uint8_t offline_index;
    Can_Device *can_device; 
    // 其他该电机独有字段
} YourMotor_t;

// 函数声明
YourMotor_t *YourMotorInit(Motor_Init_Config_s *config);
void YourMotorSetRef(YourMotor_t *motor, float ref);
void YourMotorControl(void);
void YourMotorStop(YourMotor_t *motor);
void YourMotorEnable(YourMotor_t *motor);

#endif
```

#### your_motor.c

```c
#include "your_motor.h"
#include "bsp_can.h"
#include "offline.h"
// 包含其他需要的头文件

// can中断回调函数
void DecodeYourMotor(const CAN_HandleTypeDef* hcan, const uint32_t rx_id)
{
    // 解析CAN反馈数据
    // 更新电机状态
}

// 实现初始化函数
YourMotor_t *YourMotorInit(Motor_Init_Config_s *config)
{
    // 实现初始化逻辑
    // 1. 创建电机实例
    // 2. 初始化CAN设备
    // 3. 初始化控制参数
    // 4. 注册离线检测
    // 5. 返回电机实例
}

// 实现设置参考值函数
void YourMotorSetRef(YourMotor_t *motor, float ref)
{
    // 设置电机参考值
}

// 实现控制函数
void YourMotorControl(void)
{
    // 实现控制算法
    // 首先进行电机离线检测，然后才是正常控制流程
    // 1. 读取反馈数据
    // 2. 执行控制算法
    // 3. 输出控制量
}

// 实现停止函数
void YourMotorStop(YourMotor_t *motor)
{
    motor->stop_flag = MOTOR_STOP;
}

// 实现启用函数
void YourMotorEnable(YourMotor_t *motor)
{
    motor->stop_flag = MOTOR_ENALBED;
}
```

### 4. 在配置文件中添加开关

在compoent_config.h中添加新电机的使能开关：

```c
#define USE_YOUR_MOTOR
#if defined (USE_YOUR_MOTOR)
    #define YOUR_MOTOR_CNT 4  // 定义最大电机数量
#endif
```

### 5. 更新电机任务

在motor_task.c中确保新电机的控制函数被周期性调用：

```c
void motortask(ULONG thread_input)
{
    for (;;)
    {
        DJI_MOTOR_CONTROL();     // 控制DJI电机
        YOUR_MOTOR_CONTROL();    // 控制新类型电机
        tx_thread_sleep(2);
    }
}
```

## 注意事项

1. 电机控制任务运行频率直接影响控制性能，通常设置为1-10ms周期。
2. 电机控制中使用离线检测机制，确保系统在电机异常时能及时响应。
3. 多个电机共享CAN总线时，需要注意CAN消息ID的分配，避免冲突。
