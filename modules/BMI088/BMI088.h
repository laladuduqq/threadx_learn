/*
 * @Author: laladuduqq 17503181697@163.com
 * @Date: 2025-06-06 18:24:48
 * @LastEditors: laladuduqq 17503181697@163.com
 * @LastEditTime: 2025-06-13 23:10:47
 * @FilePath: \rm_threadx\modules\BMI088\BMI088.h
 * @Description: 
 * 
 */
#ifndef __BMI088_H
#define __BMI088_H

#include <stdint.h>
#include "controller.h"

#define GxOFFSET -0.00601393497f
#define GyOFFSET -0.00196841615f
#define GzOFFSET 0.00114696583f
#define gNORM 9.67463112f

/* BMI088数据*/
typedef struct
{
    float gyro[3];     // 陀螺仪数据,xyz
    float acc[3];      // 加速度计数据,xyz
    float temperature; // 温度
    float TempWhenCali; //标定时温度
    uint32_t BMI088_ERORR_CODE;
    PIDInstance imu_temp_pid;
    // 标定数据
    float AccelScale;
    float GyroOffset[3];
    float gNorm;          // 重力加速度模长,从标定获取
} BMI088_Data_t;

typedef struct
{
    const float (*gyro)[3];    // 陀螺仪数据,xyz
    const float (*acc)[3];     // 加速度计数据,xyz
} BMI088_GET_Data_t;

/**
 * @description: bmi088温度控制函数
 * @param {float} set_tmp
 * @return {*}
 */
void bmi088_temp_ctrl(void);
/**
 * @description: bmi088初始化函数
 * @return {*}
 */
void BMI088_init(void);
/**
 * @description: 获取bmi088数据
 * @return BMI088_GET_Data_t结构体
 */
BMI088_GET_Data_t BMI088_GET_DATA(void);

#endif
