#include "can.h"
#include "compoent_config.h"
#include "tx_api.h"
#include "dji.h"
#include "imu.h"
#include "subpub.h"
#include "motor_def.h"
#include "offline.h"
#include "robotdef.h"
#include "systemwatch.h"
#include "user_lib.h"
#include "chassiscmd.h"


#define LOG_TAG              "chassis"
#define LOG_LVL              LOG_LVL_DBG
#include <ulog.h>

static TX_THREAD chassisTask_thread;

static Chassis_Ctrl_Cmd_s* chassis_cmd_recv;         // 底盘接收到的控制命令 
static DJIMotor_t *motor_lf, *motor_rf, *motor_lb, *motor_rb; // left right forward back
/* 私有函数计算的中介变量,设为静态避免参数传递的开销 */
static float chassis_vx, chassis_vy;     // 将云台系的速度投影到底盘
static PIDInstance chassis_follow_pid;
static pubsub_subscriber_t *chassis_cmd_sub;                  // cmd控制消息订阅者
static const IMU_DATA_T* imu;  

extern void ChassisCalculate(float chassis_vx, float chassis_vy, float chassis_wz,float *wheel_ops);
void ChassisInit(void)
{
    imu = INS_GetData();

    PID_Init_Config_s config = {.MaxOut = 0.5,
                                .IntegralLimit = 0.01,
                                .DeadBand = 1,
                                .Kp = 0.01,
                                .Ki = 0,
                                .Kd = 0.001,
                                .Improve = 0x01}; // enable integratiaon limit
    PIDInit(&chassis_follow_pid, &config);

    
    // 四个轮子的参数一样,改tx_id和反转标志位即可
    Motor_Init_Config_s chassis_motor_config = {
        .offline_device_motor ={
          .timeout_ms = 100,                              // 超时时间
          .level = OFFLINE_LEVEL_HIGH,                     // 离线等级
          .enable = OFFLINE_ENABLE,                       // 是否启用离线管理
        },
        .can_init_config = {
            .can_handle = &hcan1,
        },
        .controller_param_init_config = {
            .lqr_config ={
                .K ={0.0043f}, 
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
            .PowerControlState =PowerControlState_ON,
        },
        .Motor_init_Info ={
              .motor_type = M3508,
              .max_current = 20.0f,
              .gear_ratio = 19,
              .max_torque = 6.0f,
              .max_speed = 10000,
              .torque_constant = 0.0156f
        }
    };
    chassis_motor_config.can_init_config.tx_id = 1;
    chassis_motor_config.controller_setting_init_config.motor_reverse_flag = MOTOR_DIRECTION_NORMAL;
    chassis_motor_config.offline_device_motor.name = "m3508_1";
    chassis_motor_config.offline_device_motor.beep_times = 1;
    motor_lf = DJIMotorInit(&chassis_motor_config);

    chassis_motor_config.can_init_config.tx_id = 2;
    chassis_motor_config.controller_setting_init_config.motor_reverse_flag = MOTOR_DIRECTION_NORMAL;
    chassis_motor_config.offline_device_motor.name = "m3508_2";
    chassis_motor_config.offline_device_motor.beep_times = 2;
    motor_rf = DJIMotorInit(&chassis_motor_config);

    chassis_motor_config.can_init_config.tx_id = 3;
    chassis_motor_config.controller_setting_init_config.motor_reverse_flag = MOTOR_DIRECTION_NORMAL;
    chassis_motor_config.offline_device_motor.name = "m3508_3";
    chassis_motor_config.offline_device_motor.beep_times = 3;
    motor_rb = DJIMotorInit(&chassis_motor_config);

    chassis_motor_config.can_init_config.tx_id = 4;
    chassis_motor_config.controller_setting_init_config.motor_reverse_flag = MOTOR_DIRECTION_NORMAL;
    chassis_motor_config.offline_device_motor.name = "m3508_4";
    chassis_motor_config.offline_device_motor.beep_times = 4;
    motor_lb = DJIMotorInit(&chassis_motor_config);


    chassis_cmd_sub = pubsub_create_subscriber("chassis_cmd", sizeof(Chassis_Ctrl_Cmd_s));
}

void chassis_thread_entry(ULONG thread_input)
{
    (void)thread_input;
    
    SYSTEMWATCH_REGISTER_TASK(&chassisTask_thread, "chassisTask");
    for (;;) {
        SYSTEMWATCH_UPDATE_TASK(&chassisTask_thread);
        PUBSUB_RECEIVE_PTR(chassis_cmd_sub, &chassis_cmd_recv);

        if (!get_device_status(motor_lf->offline_index)
            && !get_device_status(motor_lb->offline_index)
            && !get_device_status(motor_rf->offline_index)
            && !get_device_status(motor_rb->offline_index))
        {
            if (chassis_cmd_recv->chassis_mode == CHASSIS_ZERO_FORCE)
            { 
                DJIMotorStop(motor_lf);
                DJIMotorStop(motor_rf);
                DJIMotorStop(motor_lb);
                DJIMotorStop(motor_rb);
            }
            else
            { 
                DJIMotorEnable(motor_lf);
                DJIMotorEnable(motor_rf);
                DJIMotorEnable(motor_lb);
                DJIMotorEnable(motor_rb);
            }

            // 根据控制模式设定旋转速度
            switch (chassis_cmd_recv->chassis_mode)
            {
            case CHASSIS_ROTATE_REVERSE: // 自旋反转,同时保持全向机动;当前wz维持定值,后续增加不规则的变速策略
                chassis_cmd_recv->wz =-2;
                break;
            case CHASSIS_FOLLOW_GIMBAL_YAW: // 跟随云台,不单独设置pid,以误差角度平方为速度输出
                PIDCalculate(&chassis_follow_pid,chassis_cmd_recv->offset_angle,0);
                chassis_cmd_recv->wz = chassis_follow_pid.Output;
                break;
            case CHASSIS_ROTATE: // 自旋,同时保持全向机动;当前wz维持定值,后续增加不规则的变速策略
                chassis_cmd_recv->wz = 0.45;
                break;
            case CHASSIS_AUTO_MODE:  
            break;
            
            default:
                break;
            }

            if (CHASSIS_TYPE == 2)
            {
                chassis_cmd_recv->vx = chassis_cmd_recv->vx * 784;
                chassis_cmd_recv->vy = chassis_cmd_recv->vy * 784;
            }

            // 根据云台和底盘的角度offset将控制量映射到底盘坐标系上
            // 底盘逆时针旋转为角度正方向;云台命令的方向以云台指向的方向为x,采用右手系(x指向正北时y在正东)
            static float sin_theta, cos_theta,wheel_ops[4];
            cos_theta = arm_cos_f32(chassis_cmd_recv->offset_angle * DEGREE_2_RAD);
            sin_theta = arm_sin_f32(chassis_cmd_recv->offset_angle * DEGREE_2_RAD);
            chassis_vx = chassis_cmd_recv->vx * cos_theta - chassis_cmd_recv->vy * sin_theta;
            chassis_vy = chassis_cmd_recv->vx * sin_theta + chassis_cmd_recv->vy * cos_theta;

            ChassisCalculate(chassis_vx, chassis_vy, chassis_cmd_recv->wz, wheel_ops);
            DJIMotorSetRef(motor_lf, wheel_ops[0] * 6.0f);
            DJIMotorSetRef(motor_rf, wheel_ops[1] * 6.0f);
            DJIMotorSetRef(motor_lb, wheel_ops[2] * 6.0f);
            DJIMotorSetRef(motor_rb, wheel_ops[3] * 6.0f);
        }
        else
        {
            DJIMotorStop(motor_lf);
            DJIMotorStop(motor_rf);
            DJIMotorStop(motor_lb);
            DJIMotorStop(motor_rb);
        }
        tx_thread_sleep(4);
    } 
}

void chassis_task_init(TX_BYTE_POOL *pool){
    
    CHAR *chassis_thread_stack;
    
    chassis_thread_stack = threadx_malloc(CHASSIS_THREAD_STACK_SIZE);

    UINT status = tx_thread_create(&chassisTask_thread, "chassisTask", chassis_thread_entry, 0,chassis_thread_stack, 
        CHASSIS_THREAD_STACK_SIZE, CHASSIS_THREAD_PRIORITY, CHASSIS_THREAD_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);

    if(status != TX_SUCCESS) {
        ULOG_TAG_ERROR("Failed to create chassis task!");
        return;
    }

    ChassisInit();

    ULOG_TAG_INFO("chassis init and task start successfully!");
}

