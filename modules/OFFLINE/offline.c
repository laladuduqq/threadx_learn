/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-06 10:51:09
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-06 16:20:18
 * @FilePath: /threadx_learn/modules/OFFLINE/offline.c
 * @Description: 
 */
#include "offline.h"
#include "RGB.h"
#include "dwt.h"
#include "systemwatch.h"
#include "beep.h"
#include <stdint.h>
#include <string.h>
#include "tx_api.h"


#define LOG_TAG  "offline"
#include "ulog.h"

#if OFFLINE_Enable
// 静态变量
static OfflineManager_t offline_manager;
static TX_THREAD offline_thread;
static uint8_t rgb_times = 0;
static void rgb_ctrl_times(void);
int32_t offline_set_rgb_times(uint8_t times);
// 声明任务函数
static void offline_task(ULONG thread_input);

void offline_init(TX_BYTE_POOL *pool)
{
        // 初始化管理器
        memset(&offline_manager, 0, sizeof(offline_manager)); 
        CHAR *offline_thread_stack;
        offline_thread_stack = threadx_malloc(OFFLINE_THREAD_STACK_SIZE);
        if (offline_thread_stack == NULL) {
            ULOG_TAG_ERROR("Failed to allocate stack for offlineTask!");
            return;
        }
        UINT status = tx_thread_create(&offline_thread, "offlineTask", offline_task, 0,offline_thread_stack, OFFLINE_THREAD_STACK_SIZE,
                                        OFFLINE_THREAD_PRIORITY, OFFLINE_THREAD_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);

        if(status != TX_SUCCESS) {
            ULOG_TAG_ERROR("Failed to create offline task!");
            return;
        }
        
        ULOG_TAG_INFO("Offline task initialized successfully.");
}

void offline_task(ULONG thread_input)
{    
    (void)(thread_input);
    SYSTEMWATCH_REGISTER_TASK(&offline_thread, "offline_task");
    for (;;)
    {
        SYSTEMWATCH_UPDATE_TASK(&offline_thread);

        static uint8_t beep_time = 0xFF;
        static uint8_t error_level = 0;
        static uint8_t display_index = OFFLINE_INVALID_INDEX;
        uint32_t current_time = tx_time_get();
        
        // 重置错误状态
        error_level = 0;
        display_index = OFFLINE_INVALID_INDEX;
        bool any_device_offline = false;
    
        // 检查所有设备状态
        for (uint8_t i = 0; i < offline_manager.device_count; i++) {
            OfflineDevice_t* device = &offline_manager.devices[i];
            
            if (!device->enable) {
                continue;
            }
    
            if (current_time - device->last_time > device->timeout_ms) {
                device->is_offline = STATE_OFFLINE;
                any_device_offline = true;
                
                // 更新最高优先级设备
                if (device->level > error_level) {
                    error_level = device->level;
                    display_index = i;
                    beep_time = device->beep_times;
                }
                // 相同优先级时的处理
                else if (device->level == error_level) {
                    // 如果当前设备不需要蜂鸣（beep_times=0），保持原来的设备
                    if (device->beep_times == 0) {
                        continue;
                    }
                    // 如果之前选中的设备不需要蜂鸣，或者当前设备蜂鸣次数更少
                    if (beep_time == 0 || 
                        (device->beep_times > 0 && device->beep_times < beep_time)) {
                        display_index = i;
                        beep_time = device->beep_times;
                    }
                }
            } else {
                device->is_offline = STATE_ONLINE;
            }
        }
    
        // 触发报警或清除报警
        if (display_index != OFFLINE_INVALID_INDEX && any_device_offline) {
            #if OFFLINE_Beep_Enable == 1
            beep_set_times(offline_manager.devices[display_index].beep_times);
            #endif
            offline_set_rgb_times(offline_manager.devices[display_index].beep_times);
        } else {
            // 所有设备都在线，清除报警
            #if OFFLINE_Beep_Enable == 1
            beep_set_times(0);
            #endif
            offline_set_rgb_times(0);
            RGB_show(LED_Green);              // 表示所有设备都在线
        }

        #if OFFLINE_Beep_Enable == 1
        beep_ctrl_times();
        #endif
        rgb_ctrl_times();
        
        tx_thread_sleep(10);
    }
}

uint8_t offline_device_register(const OfflineDeviceInit_t* init)
{
        if (init == NULL || offline_manager.device_count >= MAX_OFFLINE_DEVICES) {
            return OFFLINE_INVALID_INDEX;
        }
        
        uint8_t index = offline_manager.device_count;
        OfflineDevice_t* device = &offline_manager.devices[index];
        
        strncpy(device->name, init->name, sizeof(device->name) - 1);
        device->timeout_ms = init->timeout_ms;
        device->level = init->level;
        device->beep_times = init->beep_times;
        device->is_offline = STATE_OFFLINE;
        device->last_time = tx_time_get();
        device->index = index;
        device->enable = init->enable;
        
        offline_manager.device_count++;
        return index;
}

void offline_device_update(uint8_t device_index)
{
        if (device_index < offline_manager.device_count) {
            offline_manager.devices[device_index].last_time = tx_time_get();
            offline_manager.devices[device_index].dt = DWT_GetDeltaT(&offline_manager.devices[device_index].dt_cnt);
        }
}

void offline_device_enable(uint8_t device_index)
{
        if (device_index < offline_manager.device_count) {
            offline_manager.devices[device_index].enable = OFFLINE_ENABLE;
        }
}

void offline_device_disable(uint8_t device_index)
{
        if (device_index < offline_manager.device_count) {
            offline_manager.devices[device_index].enable = OFFLINE_DISABLE;
        }
}
uint8_t get_device_status(uint8_t device_index){
        if(device_index < offline_manager.device_count){
            return offline_manager.devices[device_index].is_offline;
        }
        else {return STATE_ONLINE;}
}

uint8_t get_system_status(void){
    uint8_t status = 0;
    for (uint8_t i = 0; i < offline_manager.device_count; i++) {
        if (offline_manager.devices[i].is_offline) {
            status |= (1 << i);
        }
    }
    return status;
}


/**
 * @brief 控制RGB灯闪烁次数，需周期性调用
 * @param NULL
 * @retval void
 */
static void rgb_ctrl_times(void)
{
    static uint32_t rgb_tick;
    static uint32_t times_tick;
    static uint8_t times;
    static bool led_on = false;  // 添加LED状态标志

    if (DWT_GetTimeline_ms() - rgb_tick > BEEP_PERIOD)
    {
        times = rgb_times;
        rgb_tick = DWT_GetTimeline_ms();
        times_tick = DWT_GetTimeline_ms();
    }
    else if (times != 0)
    {
        if (DWT_GetTimeline_ms() - times_tick < BEEP_ON_TIME)
        {
            RGB_show(LED_Red);  // 红灯表示设备离线
            led_on = true;
        }
        else if (DWT_GetTimeline_ms() - times_tick < BEEP_ON_TIME + BEEP_OFF_TIME)
        {
            RGB_show(LED_Black);  // 熄灭灯
            led_on = false;
        }
        else
        {
            times--;
            times_tick = DWT_GetTimeline_ms();
        }
    }
    // 当times为0但有设备离线时（beep_times=0的情况），LED常亮红色
    else if (times == 0 && get_system_status() != 0) 
    {
        RGB_show(LED_Red);
        led_on = true;
    }
    // 当没有设备离线时，恢复绿灯
    else if (!led_on) 
    {
        RGB_show(LED_Green);  // 表示所有设备都在线
    }
}

int32_t offline_set_rgb_times(uint8_t times)
{
    rgb_times = times;
    return 0;
}

#endif
