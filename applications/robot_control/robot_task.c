/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-08 14:47:47
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-08 15:00:27
 * @FilePath: /threadx_learn/applications/robot_control/robot_task.c
 * @Description: 
 */
#include "compoent_config.h"
#include "tx_api.h"
#include "robot_control.h"
#include "systemwatch.h"
#include "robot_task.h"

#define LOG_TAG              "robottask"
#include <ulog.h>

static TX_THREAD robotTask_thread;

void robotcmdtask(ULONG thread_input)
{
    (void)thread_input; 
    SYSTEMWATCH_REGISTER_TASK(&robotTask_thread, "robotTask");
    for (;;)
    {
        SYSTEMWATCH_UPDATE_TASK(&robotTask_thread);
        robot_control();  
        tx_thread_sleep(4);
    }
}

void robot_control_task_init(TX_BYTE_POOL *pool){
    CHAR *robot_thread_stack;
    robot_thread_stack = threadx_malloc(ROBOT_CONTROL_THREAD_STACK_SIZE);

    UINT status = tx_thread_create(&robotTask_thread, "robotTask", robotcmdtask, 0,robot_thread_stack, 
        ROBOT_CONTROL_THREAD_STACK_SIZE, ROBOT_CONTROL_THREAD_PRIORITY, ROBOT_CONTROL_THREAD_PRIORITY, 
        TX_NO_TIME_SLICE, TX_AUTO_START);

    if(status != TX_SUCCESS) {
        ULOG_TAG_ERROR("Failed to create robot task!");
        return;
    }

    robot_control_init();

    ULOG_TAG_INFO("robot_control task created and init finish");
}

