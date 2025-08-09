#include "damiao.h"
#include "arm_math.h"
#include "motor_def.h"
#include "offline.h"
#include "tx_api.h"
#include <stdint.h>
#include <string.h>

#if defined (USE_DAMIAO_MOTOR) 

static uint8_t idx;
static DMMOTOR_t dmm_motor_list[DM_MOTOR_CNT]= {0}; // 会在motor_task任务中遍历该数组进行pid计算

#define LOG_TAG              "dm"
#include <ulog.h>


#define LIMIT_MIN_MAX(x, min, max) (x) = (((x) <= (min)) ? (min) : (((x) >= (max)) ? (max) : (x)))


/* 两个用于将uint值和float值进行映射的函数,在设定发送值和解析反馈值时使用 */
static uint16_t float_to_uint(float x, float x_min, float x_max, uint8_t bits)
{
    float span = x_max - x_min;
    float offset = x_min;
    return (uint16_t)((x - offset) * ((float)((1 << bits) - 1)) / span);
}
static float uint_to_float(int x_int, float x_min, float x_max, int bits)
{
    float span = x_max - x_min;
    float offset = x_min;
    return ((float)x_int) * span / ((float)((1 << bits) - 1)) + offset;
}

/**
 * @brief 将弧度转换为角度
 * 
 * @param rad 弧度值
 * @return float 转换后的角度值
 */
float rad_to_deg(float rad)
{
    return rad * (180.0f / PI);
}

/**
 * @brief 将角度转换为弧度
 * 
 * @param deg 角度值
 * @return float 转换后的弧度值
 */
float deg_to_rad(float deg)
{
    return deg * (PI / 180.0f);
}

void DMMotorSetMode(DMMotor_Mode_e cmd, DMMOTOR_t *motor)
{

    motor->can_device->tx_buff[0]=0xff;
    motor->can_device->tx_buff[1]=0xff;
    motor->can_device->tx_buff[2]=0xff;
    motor->can_device->tx_buff[3]=0xff;
    motor->can_device->tx_buff[4]=0xff;
    motor->can_device->tx_buff[5]=0xff;
    motor->can_device->tx_buff[6]=0xff;
    motor->can_device->tx_buff[7] = (uint8_t)cmd; // 最后一位是命令id
    motor->can_device->txconf.StdId = motor->can_device->tx_id+motor->DMMotor_Mode_type;
    motor->can_device->txconf.DLC = 8;
    CAN_SendMessage(motor->can_device,motor->can_device->txconf.DLC);
}

void mit_ctrl(DMMOTOR_t *motor, float pos, float vel,float kp, float kd, float torq)
{
	uint16_t pos_tmp,vel_tmp,kp_tmp,kd_tmp,tor_tmp;
	
	pos_tmp = float_to_uint(pos,  DM_P_MIN,  DM_P_MAX,  16);
	vel_tmp = float_to_uint(vel,  DM_V_MIN,  DM_V_MAX,  12);
	kp_tmp  = float_to_uint(kp,   DM_KP_MIN, DM_KP_MAX, 12);
	kd_tmp  = float_to_uint(kd,   DM_KD_MIN, DM_KD_MAX, 12);
	tor_tmp = float_to_uint(torq, DM_T_MIN,  DM_T_MAX,  12);

    motor->can_device->txconf.StdId = motor->can_device->tx_id + MIT_MODE;
    motor->can_device->txconf.DLC = 8;
	motor->can_device->tx_buff[0] = (pos_tmp >> 8);
	motor->can_device->tx_buff[1] = pos_tmp;
	motor->can_device->tx_buff[2] = (vel_tmp >> 4);
	motor->can_device->tx_buff[3] = ((vel_tmp&0xF)<<4)|(kp_tmp>>8);
	motor->can_device->tx_buff[4] = kp_tmp;
	motor->can_device->tx_buff[5] = (kd_tmp >> 4);
	motor->can_device->tx_buff[6] = ((kd_tmp&0xF)<<4)|(tor_tmp>>8);
	motor->can_device->tx_buff[7] = tor_tmp;
	
	CAN_SendMessage(motor->can_device,motor->can_device->txconf.DLC);
}


void pos_speed_ctrl(DMMOTOR_t *motor, float pos_degree, float vel)
{
	uint8_t *pbuf, *vbuf;

    float pos_rad = deg_to_rad(pos_degree);
    LIMIT_MIN_MAX(pos_rad, DM_P_MIN, DM_P_MAX);
	
	motor->can_device->txconf.StdId = motor->can_device->tx_id + POS_MODE;
    motor->can_device->txconf.DLC = 8;

	pbuf=(uint8_t*)&pos_rad;
	vbuf=(uint8_t*)&vel;
	
	motor->can_device->tx_buff[0] = *pbuf;
	motor->can_device->tx_buff[1] = *(pbuf+1);
	motor->can_device->tx_buff[2] = *(pbuf+2);
	motor->can_device->tx_buff[3] = *(pbuf+3);
	motor->can_device->tx_buff[4] = *vbuf;
	motor->can_device->tx_buff[5] = *(vbuf+1);
	motor->can_device->tx_buff[6] = *(vbuf+2);
	motor->can_device->tx_buff[7] = *(vbuf+3);
	
	CAN_SendMessage(motor->can_device,motor->can_device->txconf.DLC);
}


void speed_ctrl(DMMOTOR_t *motor, float vel)
{
	
	uint8_t *vbuf;
	motor->can_device->txconf.StdId = motor->can_device->tx_id + SPEED_MODE;
    motor->can_device->txconf.DLC = 4;
	
	vbuf=(uint8_t*)&vel;
	
	motor->can_device->tx_buff[0] = *vbuf;
	motor->can_device->tx_buff[1] = *(vbuf+1);
	motor->can_device->tx_buff[2] = *(vbuf+2);
	motor->can_device->tx_buff[3] = *(vbuf+3);
	
	CAN_SendMessage(motor->can_device,motor->can_device->txconf.DLC);
}

void DMMotorDecode(const CAN_HandleTypeDef* hcan,const  uint32_t rx_id)
{
    for (uint8_t i = 0; i < idx; i++) {
        if (dmm_motor_list[i].can_device != NULL
            &&dmm_motor_list[i].can_device->can_handle == hcan 
            && dmm_motor_list[i].can_device->rx_id == rx_id)
        {
            uint8_t *rxbuff = dmm_motor_list[i].can_device->rx_buff;  
            offline_device_update(dmm_motor_list[i].offline_index);

            uint16_t tmp; // 用于暂存解析值,稍后转换成float数据,避免多次创建临时变量

            dmm_motor_list[i].measure.last_position = dmm_motor_list[i].measure.position;
            tmp = (uint16_t)((rxbuff[1] << 8) | rxbuff[2]);
            dmm_motor_list[i].measure.position = uint_to_float(tmp, DM_P_MIN, DM_P_MAX, 16);
            dmm_motor_list[i].measure.position = rad_to_deg(dmm_motor_list[i].measure.position);

            tmp = (uint16_t)((rxbuff[3] << 4) | rxbuff[4] >> 4);
            dmm_motor_list[i].measure.velocity = uint_to_float(tmp, DM_V_MIN, DM_V_MAX, 12);
            dmm_motor_list[i].measure.velocity = rad_to_deg(dmm_motor_list[i].measure.velocity);

            tmp = (uint16_t)(((rxbuff[4] & 0x0f) << 8) | rxbuff[5]);
            dmm_motor_list[i].measure.torque = uint_to_float(tmp, DM_T_MIN, DM_T_MAX, 12);

            dmm_motor_list[i].measure.T_Mos = (float)rxbuff[6];
            dmm_motor_list[i].measure.T_Rotor = (float)rxbuff[7];

            // 多圈角度计算
            int16_t delta_position = dmm_motor_list[i].measure.position - dmm_motor_list[i].measure.last_position;
             
            if (delta_position > 180) {dmm_motor_list[i].measure.total_round--;} 
            else if (delta_position < -180) {dmm_motor_list[i].measure.total_round++;}
             
            dmm_motor_list[i].measure.total_angle = dmm_motor_list[i].measure.total_round * 360.0f + dmm_motor_list[i].measure.position;

            // 解析错误码
            uint8_t error_code = (rxbuff[0] >> 4) & 0x0F;
            dmm_motor_list[i].measure.id = rxbuff[0] & 0x0F;
            dmm_motor_list[i].measure.Error_Code = (error_code >= 0x08 && error_code <= 0x0E) ? (DMMotorError_t)error_code : DM_NO_ERROR;
        }
    }
}

void DMMotorCaliEncoder(DMMOTOR_t *motor)
{
    DMMotorSetMode(DM_CMD_ZERO_POSITION, motor);
    tx_thread_sleep(10);
}
DMMOTOR_t *DMMotorInit(Motor_Init_Config_s *config,uint32_t DM_Mode_type){
    
    // 检查索引是否超出数组范围
    if (idx >= DM_MOTOR_CNT) {
        ULOG_TAG_ERROR("DM motor count exceeds maximum limit\n");
        return NULL;
    }

    // 直接在数组中初始化电机数据
    DMMOTOR_t *DMMotor = &dmm_motor_list[idx];
    memset(DMMotor, 0, sizeof(DMMOTOR_t));

    // motor basic setting 电机基本设置
    DMMotor->dm_motor_info = config->Motor_init_Info;                 //电机信息
    DMMotor->motor_settings = config->controller_setting_init_config; // 正反转,闭环类型等
    DMMotor->DMMotor_Mode_type =DM_Mode_type;

    // CAN 设备初始化配置
    Can_Device_Init_Config_s can_config = {
        .can_handle = config->can_init_config.can_handle,
        .tx_id = config->can_init_config.tx_id,
        .rx_id = config->can_init_config.rx_id,
        .tx_mode = CAN_MODE_BLOCKING,
        .rx_mode = CAN_MODE_IT,
        .can_callback = DMMotorDecode
    };
    // 注册 CAN 设备并获取引用
    Can_Device *can_dev = BSP_CAN_Device_Init(&can_config);
    if (can_dev == NULL) {
        ULOG_TAG_ERROR("Failed to initialize CAN device for DJI motor");
        return NULL;
    }
    // 保存设备指针
    DMMotor->can_device = can_dev;

    DMMotor->motor_settings.control_algorithm = config->controller_setting_init_config.control_algorithm;
    switch (config->controller_setting_init_config.control_algorithm) {
        case CONTROL_PID:
            // motor controller init 电机控制器初始化
            PIDInit(&DMMotor->motor_controller.speed_PID, &config->controller_param_init_config.speed_PID);
            PIDInit(&DMMotor->motor_controller.angle_PID, &config->controller_param_init_config.angle_PID);
            DMMotor->motor_controller.other_angle_feedback_ptr = config->controller_param_init_config.other_angle_feedback_ptr;
            DMMotor->motor_controller.other_speed_feedback_ptr = config->controller_param_init_config.other_speed_feedback_ptr;
            DMMotor->motor_controller.current_feedforward_ptr = config->controller_param_init_config.current_feedforward_ptr;
            DMMotor->motor_controller.speed_feedforward_ptr = config->controller_param_init_config.speed_feedforward_ptr;
            break;
        case CONTROL_LQR:
            LQRInit(&DMMotor->motor_controller.lqr, &config->controller_param_init_config.lqr_config);
            DMMotor->motor_controller.other_angle_feedback_ptr = config->controller_param_init_config.other_angle_feedback_ptr;
            DMMotor->motor_controller.other_speed_feedback_ptr = config->controller_param_init_config.other_speed_feedback_ptr;
            DMMotor->motor_controller.current_feedforward_ptr = config->controller_param_init_config.current_feedforward_ptr;
            DMMotor->motor_controller.speed_feedforward_ptr = config->controller_param_init_config.speed_feedforward_ptr;
            break;
        case CONTROL_OTHER:
            // 未来添加其他控制算法的初始化
            break;
    }
    
    //掉线检测
    DMMotor->offline_index =offline_device_register(&config->offline_device_motor);

    DMMotor->measure.Error_Code = DM_NO_ERROR;
    DMMotorSetMode(DM_CMD_CLEAR_ERROR,DMMotor);

    // 增加索引并返回对应元素的地址
    return &dmm_motor_list[idx++];
}

void DMMotorSetRef(DMMOTOR_t *motor, float ref)
{
    switch (motor->motor_settings.control_algorithm) 
    {
        case CONTROL_PID:
            motor->motor_controller.pid_ref = ref;
            break;
        case CONTROL_LQR:
            motor->motor_controller.lqr_ref = ref;
            break;
        case CONTROL_OTHER:
            break;
    }
}

void DMMotorEnable(DMMOTOR_t *motor)
{
    motor->stop_flag = MOTOR_ENALBED;
    DMMotorSetMode(DM_CMD_MOTOR_MODE, motor);
}

void DMMotorStop(DMMOTOR_t *motor)
{
    motor->stop_flag = MOTOR_STOP;
}

void DMMotorOuterLoop(DMMOTOR_t *motor, Closeloop_Type_e type,LQR_Init_Config_s *lqr_config)
{
    motor->motor_settings.outer_loop_type = type;

    // 如果是LQR控制且提供了配置参数，则重新初始化，其他算法传递NULL即可
    if (motor->motor_settings.control_algorithm == CONTROL_LQR && lqr_config != NULL) {
        LQRInit(&motor->motor_controller.lqr, lqr_config);
    }
}

void DMMotorChangeFeed(DMMOTOR_t *motor, Closeloop_Type_e loop, Feedback_Source_e type)
{
    if (loop == ANGLE_LOOP)
        motor->motor_settings.angle_feedback_source = type;
    else if (loop == SPEED_LOOP)
        motor->motor_settings.speed_feedback_source = type;
    else
        ULOG_TAG_ERROR("loop type error, check and func param\n");
}

static float CalculatePIDOutput(DMMOTOR_t *motor)
{
    float pid_measure, pid_ref;
    
    pid_ref = motor->motor_controller.pid_ref;
    if (motor->motor_settings.motor_reverse_flag == MOTOR_DIRECTION_REVERSE) {pid_ref *= -1;}

    // pid_ref会顺次通过被启用的闭环充当数据的载体
    // 计算位置环,只有启用位置环且外层闭环为位置时会计算速度环输出
    if ((motor->motor_settings.close_loop_type & ANGLE_LOOP) && motor->motor_settings.outer_loop_type == ANGLE_LOOP)
    {
        if (motor->motor_settings.angle_feedback_source == OTHER_FEED)
            pid_measure = *motor->motor_controller.other_angle_feedback_ptr;
        else
            pid_measure = motor->measure.total_angle; 

        if (motor->motor_settings.feedback_reverse_flag == FEEDBACK_DIRECTION_REVERSE) 
           pid_measure *= -1;
        // 更新pid_ref进入下一个环
        pid_ref = PIDCalculate(&motor->motor_controller.angle_PID, pid_measure, pid_ref);
    }
    // 计算速度环,(外层闭环为速度或位置)且(启用速度环)时会计算速度环
    if ((motor->motor_settings.close_loop_type & SPEED_LOOP) && (motor->motor_settings.outer_loop_type & (ANGLE_LOOP | SPEED_LOOP)))
    {
        if (motor->motor_settings.feedforward_flag & SPEED_FEEDFORWARD)
            pid_ref += *motor->motor_controller.speed_feedforward_ptr;

        if (motor->motor_settings.speed_feedback_source == OTHER_FEED)
            pid_measure = *motor->motor_controller.other_speed_feedback_ptr;
        else // MOTOR_FEED
            pid_measure = motor->measure.velocity;

        if (motor->motor_settings.feedback_reverse_flag == FEEDBACK_DIRECTION_REVERSE) 
           pid_measure *= -1;
        // 更新pid_ref进入下一个环
        pid_ref = PIDCalculate(&motor->motor_controller.speed_PID, pid_measure, pid_ref);
    }

    return pid_ref;
}

static float CalculateLQROutput(DMMOTOR_t *motor)
{
    float degree = 0, angular_velocity = 0;
    
    if(motor->motor_settings.motor_reverse_flag == MOTOR_DIRECTION_REVERSE)
    {
        motor->motor_controller.lqr_ref *= -1;
    }

    // 位置状态计算
    if ((motor->motor_settings.close_loop_type & ANGLE_LOOP) && 
        motor->motor_settings.outer_loop_type == ANGLE_LOOP)
    {
        degree = (motor->motor_settings.angle_feedback_source == OTHER_FEED) ?
                 *motor->motor_controller.other_angle_feedback_ptr :
                 motor->measure.total_angle;

        if (motor->motor_settings.feedback_reverse_flag == FEEDBACK_DIRECTION_REVERSE) degree *= -1;
    }

    // 速度状态计算
    if ((motor->motor_settings.close_loop_type & SPEED_LOOP) && 
        (motor->motor_settings.outer_loop_type & (ANGLE_LOOP | SPEED_LOOP)))
    {
        angular_velocity = (motor->motor_settings.speed_feedback_source == OTHER_FEED) ?
                 *motor->motor_controller.other_speed_feedback_ptr :
                 motor->measure.velocity;
        if (motor->motor_settings.feedback_reverse_flag == FEEDBACK_DIRECTION_REVERSE) angular_velocity *= -1;
    }

    float torque = LQRCalculate(&motor->motor_controller.lqr, 
                               degree, 
                               angular_velocity, 
                               motor->motor_controller.lqr_ref);


    return torque;
}

void DMMotorcontrol(void){
    DMMOTOR_t *motor =NULL;
    float control_output;

    for (size_t i = 0; i < idx; ++i){
        if (i >= DJI_MOTOR_CNT || dmm_motor_list[i].can_device == NULL){continue;}      
        motor = &dmm_motor_list[i];
        if (motor == NULL) {
            ULOG_TAG_ERROR("Motor %d is NULL, skipping", i);
            continue;
        }
        if (get_device_status(motor->offline_index)==1 || motor->stop_flag == MOTOR_STOP) // 如果电机处于离线状态,发送0 若该电机处于停止状态,直接将buff置零
        {
            DMMotorSetMode(DM_CMD_RESET_MODE, motor);
            if (motor->motor_settings.control_algorithm == CONTROL_PID)
            {
                // 当电机停止或离线时，将PID控制器输出设为0
                motor->motor_controller.speed_PID.Output = 0;
                motor->motor_controller.angle_PID.Output = 0;
            }
        }
        else 
        {
            switch (motor->DMMotor_Mode_type)
            {
                case MIT_MODE:
                {
                    // 根据控制算法计算输出
                    switch (motor->motor_settings.control_algorithm) 
                    {
                        case CONTROL_PID:
                            control_output = CalculatePIDOutput(motor);
                            break;
                        case CONTROL_LQR:
                            control_output = CalculateLQROutput(motor);
                            break;
                        default:
                            control_output = 0;
                            break;
                    }
                    mit_ctrl(motor, 0, 0, 0, 0, control_output);
                    break;
                }
                case POS_MODE:
                {               
                    // 根据控制算法计算输出
                    switch (motor->motor_settings.control_algorithm) 
                    {
                        case CONTROL_PID:
                            control_output = motor->motor_controller.pid_ref;
                            break;
                        case CONTROL_LQR:
                            control_output = motor->motor_controller.lqr_ref;
                            break;
                        default:
                            control_output = 0;
                            break;
                    }
                    LIMIT_MIN_MAX(control_output, DM_P_MIN, DM_P_MAX);
                    pos_speed_ctrl(motor, control_output, PI);
                    break;
                }
                case SPEED_MODE:
                {
                    switch (motor->motor_settings.control_algorithm) 
                    {
                        case CONTROL_PID:
                            control_output = motor->motor_controller.pid_ref;
                            break;
                        case CONTROL_LQR:
                            control_output = motor->motor_controller.lqr_ref;
                            break;
                        default:
                            control_output = 0;
                            break;
                    }
                    LIMIT_MIN_MAX(control_output, DM_V_MIN, DM_V_MAX);
                    speed_ctrl(motor, control_output);
                    break;
                }
            }
        }
    }
}

#endif
