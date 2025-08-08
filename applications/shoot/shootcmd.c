#include "can.h"
#include "compoent_config.h"
#include "tx_api.h"
#include "dji.h"
#include "subpub.h"
#include "motor_def.h"
#include "offline.h"
#include "robotdef.h"
#include "systemwatch.h"
#include "user_lib.h"
#include <string.h>
#include "shootcmd.h"


static TX_THREAD shootTask_thread;

DJIMotor_t *friction_l = NULL; 
DJIMotor_t *friction_r = NULL; 
DJIMotor_t *loader     = NULL; // 拨盘电机
static Shoot_Ctrl_Cmd_s*       shoot_cmd_recv;                   // 来自cmd的发射控制信息
static pubsub_subscriber_t*    shoot_cmd_sub;

#define LOG_TAG "shoot"
#include <ulog.h>
void ShootInit(void)
{
    Motor_Init_Config_s friction_config = {
        .offline_device_motor ={
          .name = "3508_1",                        // 设备名称
          .timeout_ms = 100,                              // 超时时间
          .level = OFFLINE_LEVEL_HIGH,                     // 离线等级
          .beep_times = 5,                                // 蜂鸣次数
          .enable = OFFLINE_ENABLE,                       // 是否启用离线管理
        },
        .can_init_config = {
            .can_handle = &hcan1,
        },
        .controller_param_init_config = {
            .lqr_config ={
                .K ={0.07011f}, //0.0317
                .output_max = 6.0f,
                .output_min =-6.0f,
                .state_dim = 1,
            }
        },
        .controller_setting_init_config = {
            .angle_feedback_source = MOTOR_FEED,
            .speed_feedback_source = MOTOR_FEED,
            .outer_loop_type    = SPEED_LOOP,
            .close_loop_type    = SPEED_LOOP,
            .feedback_reverse_flag =FEEDBACK_DIRECTION_NORMAL,
            .control_algorithm =CONTROL_LQR,
        },
        .Motor_init_Info ={
              .motor_type = M3508,
              .max_current = 10.0f,
              .gear_ratio = 19,
              .max_torque = 6.0f,
              .max_speed = 10000,
              .torque_constant = 0.0156f
        }
    };
    // 左摩擦轮
    friction_config.can_init_config.tx_id = 1,
    friction_l                            = DJIMotorInit(&friction_config);

    friction_config.can_init_config.tx_id                             = 2; // 右摩擦轮,改txid和方向就行
    friction_config.offline_device_motor.name = "3508_2";
    friction_config.offline_device_motor.beep_times = 6;
    friction_r                                                        = DJIMotorInit(&friction_config);

    // 拨盘电机
    Motor_Init_Config_s loader_config = {
        .offline_device_motor ={
          .name = "m2006",                        // 设备名称
          .timeout_ms = 100,                              // 超时时间
          .level = OFFLINE_LEVEL_HIGH,                     // 离线等级
          .beep_times = 7,                                // 蜂鸣次数
          .enable = OFFLINE_ENABLE,                       // 是否启用离线管理
        },
        .can_init_config = {
            .can_handle = &hcan1,
            .tx_id = 3,
        },
        .controller_param_init_config = {
            .lqr_config ={
                .K ={0.0034f}, //0.0317
                .output_max = 1.8f,
                .output_min =-1.8f,
                .state_dim = 1,
            }
        },
        .controller_setting_init_config = {
            .angle_feedback_source = MOTOR_FEED,
            .speed_feedback_source = MOTOR_FEED,
            .outer_loop_type       = SPEED_LOOP,
            .close_loop_type       = SPEED_LOOP,
            .feedback_reverse_flag = FEEDBACK_DIRECTION_NORMAL,
            .control_algorithm     = CONTROL_LQR,
        },
        .Motor_init_Info ={
              .motor_type = M2006,
              .max_current = 10.0f,
              .gear_ratio = 36,
              .max_torque = 1.8f,
              .max_speed = 10000,
              .torque_constant = 0.005f
        }
    };
    loader = DJIMotorInit(&loader_config);
    shoot_cmd_sub = pubsub_create_subscriber("shoot_cmd", sizeof(Shoot_Ctrl_Cmd_s));
}

/* 机器人发射机构控制核心任务 */
void shootask(ULONG thread_input)
{
    (void)thread_input;
    SYSTEMWATCH_REGISTER_TASK(&shootTask_thread, "shootTask");
    while (1)
    {
        SYSTEMWATCH_UPDATE_TASK(&shootTask_thread);
        // 从cmd获取控制数据
        if (   !get_device_status(friction_l->offline_index)
            && !get_device_status(friction_r->offline_index)
            && !get_device_status(loader->offline_index)) 
        {
            
            PUBSUB_RECEIVE_PTR(shoot_cmd_sub, &shoot_cmd_recv);
            if (shoot_cmd_recv->shoot_mode ==SHOOT_ON)
            {
                DJIMotorEnable(friction_l);
                DJIMotorEnable(friction_r);
                DJIMotorEnable(loader);
                //确定是否开启摩擦轮,后续可能修改为键鼠模式下始终开启摩擦轮(上场时建议一直开启)
                if (shoot_cmd_recv->friction_mode == FRICTION_ON)
                {
                    // 根据收到的弹速设置设定摩擦轮电机参考值,需实测后填入
                    DJIMotorSetRef(friction_l, 6200*RPM_2_ANGLE_PER_SEC);
                    DJIMotorSetRef(friction_r, -6200*RPM_2_ANGLE_PER_SEC);
                    switch (shoot_cmd_recv->load_mode)
                    {
                        // 停止拨盘
                        case LOAD_STOP:     
                            DJIMotorSetRef(loader, 0);      
                            break;
                        // 单发模式,根据鼠标按下的时间,触发一次之后需要进入不响应输入的状态(否则按下的时间内可能多次进入,导致多次发射)
                        case  LOAD_1_BULLET :
                            DJIMotorSetRef(loader, -2000 * 6.0f);
                            break;
                        case LOAD_BURSTFIRE:
                            DJIMotorSetRef(loader, -6000 * 6.0f);
                            break;
                        case LOAD_3_BULLET:
                        default:
                            break;
                    }
                }
                else // 关闭摩擦轮
                {
                    DJIMotorSetRef(friction_l, 0);
                    DJIMotorSetRef(friction_r, 0);
                    DJIMotorSetRef(loader, 0);
                }              
            }
            else 
            {
                DJIMotorStop(friction_l);
                DJIMotorStop(friction_r);
                DJIMotorStop(loader);
            }
        }
        else 
        {
            DJIMotorStop(friction_l);
            DJIMotorStop(friction_r);
            DJIMotorStop(loader);
        }
        tx_thread_sleep(4);
    }
}
void shoot_task_init(TX_BYTE_POOL *pool){

    CHAR *shoot_thread_stack;
    shoot_thread_stack = threadx_malloc(SHOOT_THREAD_STACK_SIZE);

    UINT status = tx_thread_create(&shootTask_thread, "shootTask", shootask, 0,shoot_thread_stack, 
        SHOOT_THREAD_STACK_SIZE, SHOOT_THREAD_PRIORITY, SHOOT_THREAD_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);

    if(status != TX_SUCCESS) {
        ULOG_TAG_ERROR("Failed to create shoot task!");
        return;
    }

    ShootInit();

    ULOG_TAG_INFO("shoot init and task finished");
}
