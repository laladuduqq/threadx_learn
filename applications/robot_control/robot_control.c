#include "board_com.h"
#include "can.h"
#include "dt7.h"
#include "subpub.h"
#include "offline.h"
#include "remote.h"
#include "robotdef.h"
#include <string.h>
#include "imu.h"
#include "sbus.h"
#include "user_lib.h"
#include "vt03.h"
#include <stdint.h>

#define LOG_TAG              "robotcontrol"
#include <ulog.h>


#ifdef ONE_BOARD

#endif

#ifndef ONE_BOARD
    #if defined (GIMBAL_BOARD)
            //部分仅限内部使用函数声明
            static float CalcOffsetAngle(float getyawangle);
            static void RemoteControlSet(Chassis_Ctrl_Cmd_s *Chassis_Ctrl,Shoot_Ctrl_Cmd_s *Shoot_Ctrl,Gimbal_Ctrl_Cmd_s *Gimbal_Ctrl);
            
            static pubsub_topic_t *gimbal_cmd_topic;            // 云台控制消息发布者
            static Gimbal_Ctrl_Cmd_s gimbal_cmd_send={0};      // 传递给云台的控制信息

            static pubsub_subscriber_t*  gimbal_feed_sub;          // 云台反馈信息订阅者
            static Gimbal_Upload_Data_s* gimbal_fetch_data; // 从云台获取的反馈信息

            static pubsub_topic_t *shoot_cmd_topic;           // 发射控制消息发布者
            static Shoot_Ctrl_Cmd_s shoot_cmd_send={0};      // 传递给发射的控制信息

            static Chassis_Ctrl_Cmd_s chassis_cmd_send = {0};      // 发送给底盘应用的信息,包括控制信息和UI绘制相关

            static board_com_t *board_com = NULL; // 板间通讯实例
            static const IMU_DATA_T* imu;
            void robot_control_init(void)
            {
                // 获取imu数据
                imu = INS_GetData();

                //订阅 发布注册
                gimbal_cmd_topic = pubsub_create_topic("gimbal_cmd", PUBSUB_MODE_POINTER,sizeof(Gimbal_Ctrl_Cmd_s));
                gimbal_feed_sub  = pubsub_create_subscriber("gimbal_feed",sizeof(Gimbal_Upload_Data_s));
                shoot_cmd_topic    = pubsub_create_topic("shoot_cmd", PUBSUB_MODE_POINTER,sizeof(Shoot_Ctrl_Cmd_s));
           
                //获取板间通讯结构体
                board_com = BOARD_COM_GET();
            }

            void robot_control(void)
            {
                PUBSUB_RECEIVE_PTR(gimbal_feed_sub,&gimbal_fetch_data);
                chassis_cmd_send.offset_angle = CalcOffsetAngle(gimbal_fetch_data->yaw_motor_single_round_angle);
                RemoteControlSet(&chassis_cmd_send,&shoot_cmd_send,&gimbal_cmd_send);
                pubsub_publish(gimbal_cmd_topic, &gimbal_cmd_send, sizeof(Gimbal_Ctrl_Cmd_s));
                pubsub_publish(shoot_cmd_topic, &shoot_cmd_send, sizeof(Shoot_Ctrl_Cmd_s));

               if (GET_DEVICE_STATUS(board_com->offlinemanage_index)== STATE_OFFLINE)
               {
                    memset(&chassis_cmd_send,0,sizeof(Chassis_Ctrl_Cmd_s));
               }
               BOARD_COM_SEND((uint8_t *)&chassis_cmd_send);
            } 
    #else
            static Chassis_referee_Upload_Data_s Chassis_referee_Upload_Data;
            static board_com_t *board_com = NULL; // 板间通讯实例
            static pubsub_topic_t *chassis_cmd_pub;                  // 底盘控制消息发布者
            static Chassis_Ctrl_Cmd_s chassis_cmd_send={0};      // 传递给底盘的控制信息
            void robot_control_init(void)
            {
                //订阅注册
                chassis_cmd_pub = pubsub_create_topic("chassis_cmd", PUBSUB_MODE_POINTER,sizeof(Chassis_Ctrl_Cmd_s));
                //获取板间通讯结构体
                board_com = BOARD_COM_GET();
            }

            void robot_control(void)
            {
                if (GET_DEVICE_STATUS(board_com->offlinemanage_index)==STATE_OFFLINE)
                {
                    chassis_cmd_send.vx =0;
                    chassis_cmd_send.vy =0;
                    chassis_cmd_send.wz =0;
                    chassis_cmd_send.offset_angle = 0;
                    chassis_cmd_send.chassis_mode =CHASSIS_ZERO_FORCE;
                }
                else
                {
                    chassis_cmd_send = *(Chassis_Ctrl_Cmd_s *)board_com_get_data();
                }
                
                pubsub_publish(chassis_cmd_pub, &chassis_cmd_send,sizeof(Chassis_Ctrl_Cmd_s));

                BOARD_COM_SEND((uint8_t *)&Chassis_referee_Upload_Data);     
            } 
    #endif 
#endif 


/**
 * @brief 根据gimbal app传回的当前电机角度计算和零位的误差
 *        单圈绝对角度的范围是0~360,说明文档中有图示
 *
 */
#if defined (GIMBAL_BOARD) || defined (ONE_BOARD) 
static float CalcOffsetAngle(float getyawangle)
{
    static float offsetangle;
    // 从云台获取的当前yaw电机单圈角度
#if YAW_ECD_GREATER_THAN_4096                               // 如果大于180度
    if (getyawangle > YAW_ALIGN_ANGLE && getyawangle <= 180.0f + YAW_ALIGN_ANGLE)
    {    
        offsetangle = getyawangle - YAW_ALIGN_ANGLE;
        return offsetangle;
    }
    else if (getyawangle > 180.0f + YAW_ALIGN_ANGLE)
    {    
        offsetangle = getyawangle - YAW_ALIGN_ANGLE - 360.0f;
        return offsetangle;
    }
    else
    {
        offsetangle = getyawangle - YAW_ALIGN_ANGLE;
        return offsetangle;
    }
#else // 小于180度
    if (getyawangle > YAW_ALIGN_ANGLE)
    {    
        offsetangle = getyawangle - YAW_ALIGN_ANGLE;
        return offsetangle;
    }
    else if (getyawangle <= YAW_ALIGN_ANGLE && getyawangle >= YAW_ALIGN_ANGLE - 180.0f)
    {
        offsetangle = getyawangle - YAW_ALIGN_ANGLE;
        return offsetangle;
    }
    else
    {
        offsetangle = getyawangle - YAW_ALIGN_ANGLE + 360.0f;
        return offsetangle;
    }
#endif
}


static void RemoteControlSet(Chassis_Ctrl_Cmd_s *Chassis_Ctrl,Shoot_Ctrl_Cmd_s *Shoot_Ctrl,Gimbal_Ctrl_Cmd_s *Gimbal_Ctrl)
{
    remote_info_t *remote_info = get_remote_info();

    switch (remote_info->remote_type) 
    {
        case REMOTE_TYPE_SBUS:
        {
            if (get_device_status(remote_info->remote.sbus->offline_index)==STATE_ONLINE) 
            {
                Chassis_Ctrl->vx = -1.0f * remote_info->remote.sbus->SBUS_CH.CH2/(SBUS_CHX_DOWN-SBUS_CHX_BIAS);
                Chassis_Ctrl->vy =  1.0f * remote_info->remote.sbus->SBUS_CH.CH1/(SBUS_CHX_DOWN-SBUS_CHX_BIAS); 
                //云台控制部分
                if (sbus_switch_is_up(remote_info->remote.sbus->SBUS_CH.CH6)) 
                {
                    Gimbal_Ctrl->gimbal_mode = GIMBAL_KEEPING_SMALL_YAW;
                    
                    Gimbal_Ctrl->yaw -= 0.001f * (float)(remote_info->remote.sbus->SBUS_CH.CH4);
                    Gimbal_Ctrl->small_yaw = SMALL_YAW_ALIGN_ANGLE;
                    Gimbal_Ctrl->pitch = SMALL_YAW_PITCH_HORIZON_ANGLE;
                }
                else if (sbus_switch_is_mid(remote_info->remote.sbus->SBUS_CH.CH6)) 
                {
                    Gimbal_Ctrl->gimbal_mode = GIMBAL_KEEPING_BIG_YAW;
                            
                    Gimbal_Ctrl->small_yaw -= 0.001f * (float)(remote_info->remote.sbus->SBUS_CH.CH4);
                    VAL_LIMIT(Gimbal_Ctrl->small_yaw, SMALL_YAW_MIN_ANGLE, SMALL_YAW_MAX_ANGLE);
                    Gimbal_Ctrl->pitch += 0.001f * (float)(remote_info->remote.sbus->SBUS_CH.CH3);
                    VAL_LIMIT(Gimbal_Ctrl->pitch, SMALL_YAW_PITCH_MIN_ANGLE, SMALL_YAW_PITCH_MAX_ANGLE);
                }
                else if (sbus_switch_is_down(remote_info->remote.sbus->SBUS_CH.CH6)) 
                {
                    Gimbal_Ctrl->gimbal_mode = GIMBAL_AUTO_MODE;
                    Chassis_Ctrl->chassis_mode = CHASSIS_AUTO_MODE;
                }
                //发射机构控制部分
                if (sbus_switch_is_up(remote_info->remote.sbus->SBUS_CH.CH5)) 
                {
                    Shoot_Ctrl->shoot_mode = SHOOT_OFF;
                    Shoot_Ctrl->friction_mode = FRICTION_OFF;
                    Shoot_Ctrl->load_mode = LOAD_STOP;
                }
                else if (sbus_switch_is_mid(remote_info->remote.sbus->SBUS_CH.CH5)) 
                {
                    Shoot_Ctrl->shoot_mode =SHOOT_ON;
                    Shoot_Ctrl->friction_mode = FRICTION_OFF;
                    Shoot_Ctrl->load_mode = LOAD_STOP;
                }
                else if (sbus_switch_is_down(remote_info->remote.sbus->SBUS_CH.CH5)) 
                {
                    Shoot_Ctrl->shoot_mode =SHOOT_ON;
                    Shoot_Ctrl->friction_mode = FRICTION_ON;
                    if (sbus_switch_is_mid(remote_info->remote.sbus->SBUS_CH.CH7)) {Shoot_Ctrl->load_mode = LOAD_STOP;}
                    else if (sbus_switch_is_up(remote_info->remote.sbus->SBUS_CH.CH7)) {Shoot_Ctrl->load_mode = LOAD_REVERSE;}
                    else if (sbus_switch_is_down(remote_info->remote.sbus->SBUS_CH.CH7)) {Shoot_Ctrl->load_mode = LOAD_BURSTFIRE;}
                }
                //地盘控制部分
                if (sbus_switch_is_up(remote_info->remote.sbus->SBUS_CH.CH8)) 
                {
                    if(Chassis_Ctrl->chassis_mode != CHASSIS_AUTO_MODE){Chassis_Ctrl->chassis_mode = CHASSIS_FOLLOW_GIMBAL_YAW;}
                }
                else if (sbus_switch_is_mid(remote_info->remote.sbus->SBUS_CH.CH8)) {Chassis_Ctrl->chassis_mode = CHASSIS_ROTATE;}
                else if (sbus_switch_is_down(remote_info->remote.sbus->SBUS_CH.CH8)) {Chassis_Ctrl->chassis_mode = CHASSIS_ROTATE_REVERSE;}
            }
            else 
            {
                Gimbal_Ctrl->gimbal_mode = GIMBAL_ZERO_FORCE;
                Chassis_Ctrl->chassis_mode = CHASSIS_ZERO_FORCE;
                Shoot_Ctrl->shoot_mode = SHOOT_OFF;
                Shoot_Ctrl->friction_mode = FRICTION_OFF;
                Shoot_Ctrl->load_mode = LOAD_STOP;
                memset(Chassis_Ctrl, 0, sizeof(Chassis_Ctrl_Cmd_s));
            }
            
            break;
        }
        case REMOTE_TYPE_DT7:
        {
            if (get_device_status(remote_info->remote.dt7->offline_index)==STATE_ONLINE) 
            {
                Chassis_Ctrl->vx = -1.0f * remote_info->remote.dt7->dt7_input.ch2/(DT7_CH_VALUE_MAX-DT7_CH_VALUE_OFFSET);
                Chassis_Ctrl->vy =  1.0f * remote_info->remote.dt7->dt7_input.ch1/(DT7_CH_VALUE_MAX-DT7_CH_VALUE_OFFSET); 
                //云台控制部分
                if (dt7_switch_is_mid(remote_info->remote.dt7->dt7_input.sw1)) 
                {
                    Gimbal_Ctrl->gimbal_mode = GIMBAL_KEEPING_SMALL_YAW;
                    
                    Gimbal_Ctrl->yaw -= 0.001f * (float)(remote_info->remote.dt7->dt7_input.ch3);
                    Gimbal_Ctrl->small_yaw = SMALL_YAW_ALIGN_ANGLE;
                    Gimbal_Ctrl->pitch += 0.001f * (float)(remote_info->remote.dt7->dt7_input.ch4);
                    VAL_LIMIT(Gimbal_Ctrl->pitch, SMALL_YAW_PITCH_MIN_ANGLE, SMALL_YAW_PITCH_MAX_ANGLE);
                }
                else if (dt7_switch_is_down(remote_info->remote.dt7->dt7_input.sw1)) 
                {
                    Gimbal_Ctrl->gimbal_mode = GIMBAL_AUTO_MODE;
                    Chassis_Ctrl->chassis_mode = CHASSIS_AUTO_MODE;
                }
                //发射机构控制部分
                if (dt7_switch_is_mid(remote_info->remote.dt7->dt7_input.sw1)) 
                {
                    Shoot_Ctrl->shoot_mode = SHOOT_ON;
                    Shoot_Ctrl->friction_mode = FRICTION_OFF;
                    Shoot_Ctrl->load_mode = LOAD_STOP;
                }
                else if (dt7_switch_is_up(remote_info->remote.dt7->dt7_input.sw1)) 
                {
                    Shoot_Ctrl->shoot_mode =SHOOT_ON;
                    Shoot_Ctrl->friction_mode = FRICTION_ON;
                    if (remote_info->remote.dt7->dt7_input.wheel==0) {Shoot_Ctrl->load_mode = LOAD_STOP;}
                    else if (remote_info->remote.dt7->dt7_input.wheel>0) {Shoot_Ctrl->load_mode = LOAD_REVERSE;}
                    else if (remote_info->remote.dt7->dt7_input.wheel<0) {Shoot_Ctrl->load_mode = LOAD_BURSTFIRE;}
                }
                //地盘控制部分
                if (dt7_switch_is_mid(remote_info->remote.dt7->dt7_input.sw2)) 
                {
                    if(Chassis_Ctrl->chassis_mode != CHASSIS_AUTO_MODE){Chassis_Ctrl->chassis_mode = CHASSIS_FOLLOW_GIMBAL_YAW;}
                }
                else if (dt7_switch_is_up(remote_info->remote.dt7->dt7_input.sw2)) {Chassis_Ctrl->chassis_mode = CHASSIS_ROTATE;}
                else if (dt7_switch_is_down(remote_info->remote.dt7->dt7_input.sw2)) {Chassis_Ctrl->chassis_mode = CHASSIS_ROTATE_REVERSE;}
            }
            else 
            {
                Gimbal_Ctrl->gimbal_mode = GIMBAL_ZERO_FORCE;
                Chassis_Ctrl->chassis_mode = CHASSIS_ZERO_FORCE;
                Shoot_Ctrl->shoot_mode = SHOOT_OFF;
                Shoot_Ctrl->friction_mode = FRICTION_OFF;
                Shoot_Ctrl->load_mode = LOAD_STOP;
                memset(Chassis_Ctrl, 0, sizeof(Chassis_Ctrl_Cmd_s));
            }
            
            break;
        }
        case REMOTE_TYPE_NONE:
        {
            if (remote_info->vt_type == VT_TYPE_VT03) 
            {
                if (get_device_status(remote_info->vt.vt03->offline_index)==STATE_ONLINE) 
                {
                    Chassis_Ctrl->vx = -1.0f * remote_info->vt.vt03->vt03_remote_data.channels.ch1/(VT03_CH_VALUE_MAX-VT03_CH_VALUE_OFFSET);
                    Chassis_Ctrl->vy =  1.0f * remote_info->vt.vt03->vt03_remote_data.channels.ch0/(VT03_CH_VALUE_MAX-VT03_CH_VALUE_OFFSET); 
                    //云台控制部分
                    if (remote_info->vt.vt03->vt03_remote_data.button_current.bit.switch_pos==0) 
                    {
                        Gimbal_Ctrl->gimbal_mode = GIMBAL_KEEPING_SMALL_YAW;
                        
                        Gimbal_Ctrl->yaw -= 0.001f * (float)(remote_info->vt.vt03->vt03_remote_data.channels.ch3);
                        Gimbal_Ctrl->small_yaw = SMALL_YAW_ALIGN_ANGLE;
                        Gimbal_Ctrl->pitch = SMALL_YAW_PITCH_HORIZON_ANGLE;
                    }
                    else if (remote_info->vt.vt03->vt03_remote_data.button_current.bit.switch_pos==1) 
                    {
                        Gimbal_Ctrl->gimbal_mode = GIMBAL_KEEPING_BIG_YAW;
                                
                        Gimbal_Ctrl->small_yaw -= 0.001f * (float)(remote_info->vt.vt03->vt03_remote_data.channels.ch3);
                        VAL_LIMIT(Gimbal_Ctrl->small_yaw, SMALL_YAW_MIN_ANGLE, SMALL_YAW_MAX_ANGLE);
                        Gimbal_Ctrl->pitch     += 0.001f * (float)(remote_info->vt.vt03->vt03_remote_data.channels.ch2);
                        VAL_LIMIT(Gimbal_Ctrl->pitch, SMALL_YAW_PITCH_MIN_ANGLE, SMALL_YAW_PITCH_MAX_ANGLE);
                    }
                    else if (remote_info->vt.vt03->vt03_remote_data.button_current.bit.switch_pos==2) 
                    {
                        Gimbal_Ctrl->gimbal_mode = GIMBAL_AUTO_MODE;
                        Chassis_Ctrl->chassis_mode = CHASSIS_AUTO_MODE;
                    }
                    //发射机构控制部分
                    if (remote_info->vt.vt03->vt03_remote_data.button_toggle[BUTTON_TOGGLE_TRIGGER]==0) 
                    {
                        Shoot_Ctrl->shoot_mode = SHOOT_ON;
                        Shoot_Ctrl->friction_mode = FRICTION_OFF;
                        Shoot_Ctrl->load_mode = LOAD_STOP;
                    }
                    else
                    {
                        Shoot_Ctrl->shoot_mode =SHOOT_ON;
                        Shoot_Ctrl->friction_mode = FRICTION_ON;
                        if (remote_info->vt.vt03->vt03_remote_data.wheel==0) {Shoot_Ctrl->load_mode = LOAD_STOP;}
                        else if (remote_info->vt.vt03->vt03_remote_data.wheel>0) {Shoot_Ctrl->load_mode = LOAD_REVERSE;}
                        else if (remote_info->vt.vt03->vt03_remote_data.wheel<0) {Shoot_Ctrl->load_mode = LOAD_BURSTFIRE;}
                    }

                    //地盘控制部分
                    if (remote_info->vt.vt03->vt03_remote_data.button_toggle[BUTTON_TOGGLE_CUSTOM_R]==0) 
                    {
                        if(remote_info->vt.vt03->vt03_remote_data.button_current.bit.switch_pos!=2){Chassis_Ctrl->chassis_mode = CHASSIS_FOLLOW_GIMBAL_YAW;}
                    }
                    else 
                    {
                        if (remote_info->vt.vt03->vt03_remote_data.button_toggle[BUTTON_TOGGLE_CUSTOM_L]==0)
                        {
                            Chassis_Ctrl->chassis_mode = CHASSIS_ROTATE;
                        }
                        else {
                            Chassis_Ctrl->chassis_mode = CHASSIS_ROTATE_REVERSE;
                        }
                    }
                }else 
                {
                    Gimbal_Ctrl->gimbal_mode = GIMBAL_ZERO_FORCE;
                    Chassis_Ctrl->chassis_mode = CHASSIS_ZERO_FORCE;
                    Shoot_Ctrl->shoot_mode = SHOOT_OFF;
                    Shoot_Ctrl->friction_mode = FRICTION_OFF;
                    Shoot_Ctrl->load_mode = LOAD_STOP;
                    memset(Chassis_Ctrl, 0, sizeof(Chassis_Ctrl_Cmd_s));
                }
            }
            else 
            {
                Gimbal_Ctrl->gimbal_mode = GIMBAL_ZERO_FORCE;
                Chassis_Ctrl->chassis_mode = CHASSIS_ZERO_FORCE;
                Shoot_Ctrl->shoot_mode = SHOOT_OFF;
                Shoot_Ctrl->friction_mode = FRICTION_OFF;
                Shoot_Ctrl->load_mode = LOAD_STOP;
                memset(Chassis_Ctrl, 0, sizeof(Chassis_Ctrl_Cmd_s));
            }
            break;
        }
        default:
        {
            Gimbal_Ctrl->gimbal_mode = GIMBAL_ZERO_FORCE;
            Chassis_Ctrl->chassis_mode = CHASSIS_ZERO_FORCE;
            Shoot_Ctrl->shoot_mode = SHOOT_OFF;
            Shoot_Ctrl->friction_mode = FRICTION_OFF;
            Shoot_Ctrl->load_mode = LOAD_STOP;
            memset(Chassis_Ctrl, 0, sizeof(Chassis_Ctrl_Cmd_s));
        }
    }

}
#endif
