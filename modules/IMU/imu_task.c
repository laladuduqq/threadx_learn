#include "BMI088.h"
#include "imu.h"
#include "QuaternionEKF.h"
#include "dwt.h"
#include "systemwatch.h"
#include "tx_api.h"
#include "user_lib.h"
#include <string.h>


#define LOG_TAG  "ins"
#include "ulog.h"

static INS_t INS;
static IMU_DATA_T ins_data = {0};  
static IMU_Param_t IMU_Param;
static TX_THREAD INS_thread;
static BMI088_GET_Data_t BMI088_GET_Data;

const float xb[3] = {1, 0, 0};
const float yb[3] = {0, 1, 0};
const float zb[3] = {0, 0, 1};


#if defined(USE_COMPOENT_CONFIG_H) && USE_COMPOENT_CONFIG_H
#include "compoent_config.h"
#else
#define IMU_ENABLE 1
#if IMU_ENABLE
    #define IMU_THREAD_STACK_SIZE 1024     // imu线程栈大小
    #define IMU_THREAD_PRIORITY 4          // imu线程优先级
#endif
#endif // _COMPOENT_CONFIG_H_

// 使用加速度计的数据初始化Roll和Pitch,而Yaw置0,这样可以避免在初始时候的姿态估计误差
static void InitQuaternion(float *init_q4)
{
    float acc_init[3] = {0};
    float gravity_norm[3] = {0, 0, 1}; // 导航系重力加速度矢量,归一化后为(0,0,1)
    float axis_rot[3] = {0};           // 旋转轴
    // 读取100次加速度计数据,取平均值作为初始值
    for (uint8_t i = 0; i < 100; ++i)
    {
        BMI088_GET_Data = BMI088_GET_DATA();
        acc_init[0] += (*BMI088_GET_Data.acc)[0];
        acc_init[1] += (*BMI088_GET_Data.acc)[1];
        acc_init[2] += (*BMI088_GET_Data.acc)[2];
        DWT_Delay(0.001);
    }
    for (uint8_t i = 0; i < 3; ++i)
        acc_init[i] /= 100;
    Norm3d(acc_init);
    // 计算原始加速度矢量和导航系重力加速度矢量的夹角
    float angle = acosf(Dot3d(acc_init, gravity_norm));
    Cross3d(acc_init, gravity_norm, axis_rot);
    Norm3d(axis_rot);
    init_q4[0] = cosf(angle / 2.0f);
    for (uint8_t i = 0; i < 2; ++i)
        init_q4[i + 1] = axis_rot[i] * sinf(angle / 2.0f); // 轴角公式,第三轴为0(没有z轴分量)
}

void INS_Init(void)
{
    if (!INS.init) INS.init = 1;

    IMU_Param.scale[0] = 1;
    IMU_Param.scale[1] = 1;
    IMU_Param.scale[2] = 1;
    IMU_Param.Yaw = 0;
    IMU_Param.Pitch = 0;
    IMU_Param.Roll = 0;
    IMU_Param.flag = 1;

    float init_quaternion[4] = {0};
    InitQuaternion(init_quaternion);
    IMU_QuaternionEKF_Init(init_quaternion, 10, 0.001, 1000000, 1, 0);

    // noise of accel is relatively big and of high freq,thus lpf is used
    INS.AccelLPF = 0.0085f;
}

void INSTask(ULONG thread_input)
{
    (void)(thread_input);
    const float gravity[3] = {0, 0, 9.81f};
    SYSTEMWATCH_REGISTER_TASK(&INS_thread, "INS Task");
    for (;;) {
        SYSTEMWATCH_UPDATE_TASK(&INS_thread);
        INS.dt = DWT_GetDeltaT(&INS.dwt_cnt);
        // ins update
        BMI088_GET_Data = BMI088_GET_DATA();
        INS.Accel[0] = (*BMI088_GET_Data.acc)[0];
        INS.Accel[1] = (*BMI088_GET_Data.acc)[1];
        INS.Accel[2] = (*BMI088_GET_Data.acc)[2];
        INS.Gyro[0]  = (*BMI088_GET_Data.gyro)[0];
        INS.Gyro[1]  = (*BMI088_GET_Data.gyro)[1];
        INS.Gyro[2]  = (*BMI088_GET_Data.gyro)[2];
    
        // demo function,用于修正安装误差,可以不管
        IMU_Param_Correction(&IMU_Param, INS.Gyro, INS.Accel);
    
        // 计算重力加速度矢量和b系的XY两轴的夹角,可用作功能扩展,本demo暂时没用
        // INS.atanxz = -atan2f(INS.Accel[X], INS.Accel[Z]) * 180 / PI;
        // INS.atanyz = atan2f(INS.Accel[Y], INS.Accel[Z]) * 180 / PI;
    
        // 核心函数,EKF更新四元数
        IMU_QuaternionEKF_Update(INS.Gyro[0], INS.Gyro[1], INS.Gyro[2], INS.Accel[0], INS.Accel[1], INS.Accel[2], INS.dt);
    
        memcpy(INS.q, QEKF_INS.q, sizeof(QEKF_INS.q));
    
        // 机体系基向量转换到导航坐标系，本例选取惯性系为导航系
        BodyFrameToEarthFrame(xb, INS.xn, INS.q);
        BodyFrameToEarthFrame(yb, INS.yn, INS.q);
        BodyFrameToEarthFrame(zb, INS.zn, INS.q);
    
        // 将重力从导航坐标系n转换到机体系b,随后根据加速度计数据计算运动加速度
        float gravity_b[3];
        EarthFrameToBodyFrame(gravity, gravity_b, INS.q);
        for (uint8_t i = 0; i < 3; ++i) // 同样过一个低通滤波
        {
            INS.MotionAccel_b[i] = (INS.Accel[i] - gravity_b[i]) * INS.dt / (INS.AccelLPF + INS.dt) + INS.MotionAccel_b[i] * INS.AccelLPF / (INS.AccelLPF + INS.dt);
        }
        BodyFrameToEarthFrame(INS.MotionAccel_b, INS.MotionAccel_n, INS.q); // 转换回导航系n
    
        INS.Yaw = QEKF_INS.Yaw;
        INS.Pitch = QEKF_INS.Pitch;
        INS.Roll = QEKF_INS.Roll;
        INS.YawTotalAngle = QEKF_INS.YawTotalAngle;
        INS.YawRoundCount = QEKF_INS.YawRoundCount;
    
        // temperature control
        bmi088_temp_ctrl();
        tx_thread_sleep(1);
    }    
}

void INS_TASK_init(TX_BYTE_POOL *pool)
{
    #if IMU_ENABLE
        // 在任务初始化时设置指针
        ins_data.Yaw = &(INS.Yaw);                
        ins_data.Pitch = &(INS.Pitch);            
        ins_data.Roll = &(INS.Roll);   
        ins_data.YawTotalAngle = &(INS.YawTotalAngle);  
        ins_data.gyro = (const float (*)[3])&(INS.Gyro); 

        INS_Init();

        // 用内存池分配监控线程栈
        CHAR *ins_thread_stack;
        ins_thread_stack = threadx_malloc(1024);
        if (ins_thread_stack == NULL) {
            ULOG_TAG_ERROR("Failed to allocate memory for ins thread stack!");
            return;
        }

        UINT status = tx_thread_create(&INS_thread, "insTask", INSTask, 0,ins_thread_stack, 
            IMU_THREAD_STACK_SIZE, IMU_THREAD_PRIORITY, IMU_THREAD_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);

        if(status != TX_SUCCESS) {
            ULOG_TAG_ERROR("Failed to create ins task!");
            return;
        }
        
        ULOG_TAG_INFO("INS init and task created successfully");
    #else
        (void)(pool);  // 如果IMU模块未启用，避免编译器警告
        // 如果IMU模块未启用，打印日志并跳过初始化
        ULOG_TAG_INFO("INS module is disabled, skipping initialization.");
    #endif
}

const IMU_DATA_T* INS_GetData(void)
{
    return &ins_data;
}
