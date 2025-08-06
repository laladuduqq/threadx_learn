# IMU

## 简介

IMU（Inertial Measurement Unit，惯性测量单元）模块是机器人系统中的核心姿态解算模块。该模块基于BMI088传感器数据，通过扩展卡尔曼滤波（EKF）算法实现高精度的姿态解算，包括Roll、Pitch、Yaw角度以及四元数等信息。

该模块集成了以下主要功能：

- 基于BMI088传感器的数据采集
- 扩展卡尔曼滤波姿态解算
- 坐标系转换（机体坐标系与地球坐标系）
- 运动加速度计算
- IMU安装误差修正

## 算法介绍

[# RoboMaster机器人姿态解算方案开源](https://zhuanlan.zhihu.com/p/540676773)

## 参考代码

- https://github.com/WangHongxi2001/RoboMaster-C-Board-INS-Example
- [modules/imu/ins_task.c · HNUYueLuRM/basic_framework - Gitee.com](https://gitee.com/hnuyuelurm/basic_framework/blob/master/modules/imu/ins_task.c)

## 模块初始化

IMU模块会在系统初始化时自动创建任务线程，无需手动初始化。任务线程以1kHz频率运行姿态解算算法。

## 数据获取

通过[INS_GetData()](file:///home/pan/code/threadx_learn/modules/IMU/imu.h#L80-L80)函数获取IMU数据指针，该函数返回一个[IMU_DATA_T](file:///home/pan/code/threadx_learn/modules/IMU/imu.h#L57-L63)结构体的常量指针，包含以下数据：

| 数据成员          | 类型                 | 描述           |
| ------------- | ------------------ | ------------ |
| Yaw           | const float*       | 偏航角（绕Z轴旋转）   |
| Pitch         | const float*       | 俯仰角（绕Y轴旋转）   |
| Roll          | const float*       | 横滚角（绕X轴旋转）   |
| YawTotalAngle | const float*       | 累计偏航角（无角度限制） |
| gyro          | const float (*)[3] | 陀螺仪数据（角速度）   |

## 使用示例

### 基本使用方法

```c
const IMU_DATA_T* imu = INS_GetData();
float yaw = *(imu->Yaw);           // 使用解引用获取值
float pitch = *(imu->Pitch);
float roll = *(imu->Roll);
float yawTotal = *(imu->YawTotalAngle);
const float gyro = *imu->gyro[2];  // 获取陀螺仪数据指针
```

### 指针操作

```c
const IMU_DATA_T* imu = INS_GetData();

struct {
    const float *other_angle_feedback_ptr;  
    const float *other_speed_feedback_ptr; 
} control = {
    .other_angle_feedback_ptr = imu->YawTotalAngle,
    .other_speed_feedback_ptr = &((*imu->gyro)[2]),
};
```

### 在控制算法中的应用

```c
// 获取IMU数据
const IMU_DATA_T* imu = INS_GetData();

// 使用姿态角进行控制
float current_yaw = *(imu->Yaw);
float current_pitch = *(imu->Pitch);
float current_roll = *(imu->Roll);

// 使用陀螺仪数据进行角速度控制
float gyro_x = (*imu->gyro)[0];
float gyro_y = (*imu->gyro)[1];
float gyro_z = (*imu->gyro)[2];

// 使用累计角度进行绝对角度控制
float total_yaw = *(imu->YawTotalAngle);
```

## 注意事项

1. 所有IMU数据都需要通过解引用获取实际值，如`*(imu->Yaw)`。
2. IMU模块在系统启动时会自动进行初始化和校准
3. 陀螺仪数据gyro是一个指向包含3个元素的数组的指针，访问时需要使用`(*imu->gyro)[index]`语法。
