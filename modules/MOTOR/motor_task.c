/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-07 16:57:18
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-08 14:00:43
 * @FilePath: /threadx_learn/modules/MOTOR/motor_task.c
 * @Description: 
 */
#include "motor_task.h"
#include "damiao.h"
#include "compoent_config.h"
#include "systemwatch.h"
#include "tx_api.h"
#include "dji.h"

#define LOG_TAG  "motortask"
#include "ulog.h"

static TX_THREAD motorTask_thread;

void motortask(ULONG thread_input)
{
    (void)thread_input; 
    SYSTEMWATCH_REGISTER_TASK(&motorTask_thread, "motorTask");
    for (;;)
    {
        DJI_MOTOR_CONTROL();
        DM_MOTOR_CONTROL();
        SYSTEMWATCH_UPDATE_TASK(&motorTask_thread);
        tx_thread_sleep(2);
    }
}

void motor_task_init(TX_BYTE_POOL *pool){
    CHAR *motor_thread_stack;
    motor_thread_stack = threadx_malloc(MOTOR_THREAD_STACK_SIZE);
    if (motor_thread_stack == NULL)
    {
        ULOG_TAG_ERROR("Failed to allocate stack for motorTask!");
        return;
    }
    UINT status = tx_thread_create(&motorTask_thread, "motorTask", motortask, 0,motor_thread_stack, 
                                    MOTOR_THREAD_STACK_SIZE, MOTOR_THREAD_PRIORITY, MOTOR_THREAD_PRIORITY, 
                                    TX_NO_TIME_SLICE, TX_AUTO_START);
    if(status != TX_SUCCESS) {
        ULOG_TAG_ERROR("Failed to create ins task!,status:%s",status);
        return;
    }
    ULOG_TAG_INFO("motorTask create success");
}
