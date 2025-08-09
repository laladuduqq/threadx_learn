/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-08 17:26:24
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-09 10:00:40
 * @FilePath: /threadx_learn/applications/gimbal/gimbalcmd.c
 * @Description: 
 */
#include "can.h"
#include "compensation.h"
#include "compoent_config.h"
#include "tx_api.h"
#include "damiao.h"
#include "dji.h"
#include "subpub.h"
#include "motor_def.h"
#include "offline.h"
#include "robotdef.h"
#include "imu.h"
#include "dm_imu.h"
#include "systemwatch.h"
#include <string.h>
#include "gimbalcmd.h"

#define LOG_TAG              "gimbal"
#include <ulog.h>

static TX_THREAD gimbalTask_thread;
    
static pubsub_topic_t *gimbal_feed_topic;                   // 云台应用消息发布者(云台反馈给cmd)
static pubsub_subscriber_t   *gimbal_cmd_sub;                  // cmd控制消息订阅者
static Gimbal_Upload_Data_s gimbal_feedback_data; // 回传给cmd的云台状态信息
static Gimbal_Ctrl_Cmd_s* gimbal_cmd_recv =NULL;         // 来自cmd的控制信息

static const IMU_DATA_T* imu;
static const DM_IMU_DATA_T* dm_imu_data;

#define SINE_FREQ_YAW 0.2f           // 正弦运动频率(Hz)
#define SINE_FREQ_Pitch 1.5f           // 正弦运动频率(Hz)
#define YAW_SINE_AMP 80.0f      // yaw轴正弦运动幅度(度) - 基于中间位置的偏移量
#define PITCH_SINE_AMP 10.0f     // pitch轴正弦运动幅度(度)
static float auto_angle_record=0.0f;
static float small_yaw_offset=0.0f;

DJIMotor_t *big_yaw = NULL;
DJIMotor_t *small_yaw = NULL; 
DMMOTOR_t *pitch_motor = NULL; 
void gimbal_init(void)
{
    imu = INS_GetData();
    dm_imu_data = DM_IMU_GetData();

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

    Motor_Init_Config_s small_yaw_config = {
            .offline_device_motor ={
              .name = "6020_small",                        // 设备名称
              .timeout_ms = 100,                              // 超时时间
              .level = OFFLINE_LEVEL_HIGH,                     // 离线等级
              .beep_times = 3,                                // 蜂鸣次数
              .enable = OFFLINE_ENABLE,                       // 是否启用离线管理
            },
            .can_init_config = {
                .can_handle = &hcan1,
                .tx_id = 1,
            },
            .controller_param_init_config = {
                .other_angle_feedback_ptr = imu->YawTotalAngle,
                .other_speed_feedback_ptr = &((*imu->gyro)[2]),
                .lqr_config ={
                    .K ={17.32f,1.0f},
                    .output_max = 2.223,
                    .output_min =-2.223,
                    .state_dim = 2,
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
    small_yaw =DJIMotorInit(&small_yaw_config);

        // PITCH
    Motor_Init_Config_s pitch_config = {
            .offline_device_motor ={
              .name = "dm4310",                        // 设备名称
              .timeout_ms = 100,                              // 超时时间
              .level = OFFLINE_LEVEL_HIGH,                     // 离线等级
              .beep_times = 4,                                // 蜂鸣次数
              .enable = OFFLINE_ENABLE,                       // 是否启用离线管理
            },
            .can_init_config = {
                .can_handle = &hcan1,
                .tx_id = 0X23,
                .rx_id = 0X206,
            },
            .controller_param_init_config = {
                .other_angle_feedback_ptr = imu->Pitch,
                .other_speed_feedback_ptr = &(((const float*)imu->gyro)[0]),
                .lqr_config ={
                    .K ={44.7214f,3.3411f}, //28.7312f,2.5974f
                    .output_max = 7,
                    .output_min = -7,
                    .state_dim = 2,
                    .feedforward_func = create_gravity_compensation_wrapper(16,0.09)
                }
            },
            .controller_setting_init_config = {
                .control_algorithm = CONTROL_LQR,
                .feedback_reverse_flag =FEEDBACK_DIRECTION_REVERSE,
                .angle_feedback_source =OTHER_FEED,
                .speed_feedback_source =OTHER_FEED,
                .outer_loop_type = ANGLE_LOOP,
                .close_loop_type = ANGLE_LOOP | SPEED_LOOP,
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

    gimbal_cmd_sub    = pubsub_create_subscriber("gimbal_cmd", sizeof(Gimbal_Ctrl_Cmd_s));
    gimbal_feed_topic = pubsub_create_topic("gimbal_feed", PUBSUB_MODE_POINTER,sizeof(Gimbal_Upload_Data_s));

}

void gimbal_thread_entry(ULONG thread_input)
{   
    (void)thread_input;
    SYSTEMWATCH_REGISTER_TASK(&gimbalTask_thread, "gimbalTask");
    for (;;)
    {
        SYSTEMWATCH_UPDATE_TASK(&gimbalTask_thread);

        PUBSUB_RECEIVE_PTR(gimbal_cmd_sub, &gimbal_cmd_recv);

        if (!get_device_status(big_yaw->offline_index) 
         && !get_device_status(small_yaw->offline_index) 
         && !get_device_status(pitch_motor->offline_index)
         && gimbal_cmd_recv != NULL) 
        {
            // 计算相对角度
            small_yaw_offset = *(imu->YawTotalAngle) - small_yaw->measure.total_angle; 
            if (gimbal_cmd_recv->gimbal_mode != GIMBAL_ZERO_FORCE)
            {
                //DJIMotorEnable(big_yaw);
                //DJIMotorEnable(small_yaw);
                DMMotorEnable(pitch_motor); 
            }
            else
            {
                DJIMotorStop(big_yaw);
                DJIMotorStop(small_yaw);
                DMMotorStop(pitch_motor); 
            }
            switch (gimbal_cmd_recv->gimbal_mode) 
            {
                case GIMBAL_KEEPING_SMALL_YAW:
                {
                    DJIMotorSetRef(big_yaw,auto_angle_record + gimbal_cmd_recv->yaw);
                    DMMotorSetRef(pitch_motor, gimbal_cmd_recv->pitch); 
                    DJIMotorSetRef(small_yaw, small_yaw_offset);
                    break;
                }
                case GIMBAL_KEEPING_BIG_YAW:
                {
                    DJIMotorSetRef(big_yaw,auto_angle_record + gimbal_cmd_recv->yaw);
                    DMMotorSetRef(pitch_motor, gimbal_cmd_recv->pitch);
                    DJIMotorSetRef(small_yaw, gimbal_cmd_recv->small_yaw + small_yaw_offset);
                    break;
                }
                case GIMBAL_AUTO_MODE:
                {
                    break;
                }
                default:
                    break;
            }
        }
        else
        {
            DJIMotorStop(big_yaw);
            DJIMotorStop(small_yaw);
            DMMotorStop(pitch_motor);
        }

        // 设置反馈数据,主要是imu和yaw的ecd
        if (get_device_status(big_yaw->offline_index)==STATE_ONLINE)
        {
            gimbal_feedback_data.yaw_motor_single_round_angle = big_yaw->measure.angle_single_round;
            // 推送消息
            pubsub_publish(gimbal_feed_topic, &gimbal_feedback_data, sizeof(Gimbal_Upload_Data_s));
        }else
        {
            gimbal_feedback_data.yaw_motor_single_round_angle = YAW_ALIGN_ANGLE * ECD_ANGLE_COEF_DJI;
            // 推送消息
            pubsub_publish(gimbal_feed_topic, &gimbal_feedback_data, sizeof(Gimbal_Upload_Data_s));
        }
        tx_thread_sleep(4);
    }
}

void gimbal_task_init(TX_BYTE_POOL *pool)
{
    // 用内存池分配监控线程栈
    CHAR *gimbal_thread_stack;

    gimbal_thread_stack = threadx_malloc(GIMBAL_THREAD_STACK_SIZE);
    if (gimbal_thread_stack == NULL)
    {
        ULOG_TAG_ERROR("gimbal_thread_stack malloc error");
    }

    UINT status = tx_thread_create(&gimbalTask_thread, "gimbalTask", gimbal_thread_entry, 0,gimbal_thread_stack, 
        GIMBAL_THREAD_STACK_SIZE, GIMBAL_THREAD_PRIORITY, GIMBAL_THREAD_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);

    if(status != TX_SUCCESS) {
        ULOG_TAG_ERROR("Failed to create gimbal task!");
        return;
    }

    gimbal_init();

    ULOG_TAG_INFO("gimbal task created");
}
