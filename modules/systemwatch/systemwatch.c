#include "systemwatch.h"
#include "dwt.h"
#include "iwdg.h"
#include "stm32f4xx_hal_iwdg.h"
#include "tx_api.h"
#include "tx_port.h"
#include <stdint.h>
#include <string.h>

#define LOG_TAG  "systemwatch"
#include "ulog.h"

#if SystemWatch_Enable

// 静态变量
static TaskMonitor_t taskList[MAX_MONITORED_TASKS];
static uint8_t taskCount = 0;
static TX_THREAD watchTaskHandle;
static tx_thread_status_t tx_thread_status[] =
    {
        {TX_READY, "READY"},
        {TX_COMPLETED, "COMPLETED"},
        {TX_TERMINATED, "TERMINATED"},
        {TX_SUSPENDED, "SUSPENDED"},
        {TX_SLEEP, "SLEEP"},
        {TX_QUEUE_SUSP, "QUEUE_SUSP"},
        {TX_SEMAPHORE_SUSP, "SEMAPHORE_SUSP"},
        {TX_EVENT_FLAG, "EVENT_FLAG"},
        {TX_BLOCK_MEMORY, "BLOCK_MEMORY"},
        {TX_BYTE_MEMORY, "BYTE_MEMORY"},
        {TX_IO_DRIVER, "IO_DRIVER"},
        {TX_FILE, "FILE"},
        {TX_TCP_IP, "TCP_IP"},
        {TX_MUTEX_SUSP, "MUTEX_SUSP"},
        {TX_PRIORITY_CHANGE, "PRIORITY_CHANGE"},
};
static void PrintTaskInfo(TaskMonitor_t *pxTaskMonitor);
static void PrintSystemStackInfo(void);
static void PrintMemoryPoolInfo(TX_BYTE_POOL *pool);

static void SystemWatch_Task(ULONG thread_input)
{
    (void)thread_input;

    #if SystemWatch_Iwdg_Enable
        __HAL_DBGMCU_FREEZE_IWDG();
        MX_IWDG_Init();
    #endif
    
    while(1) {
        HAL_IWDG_Refresh(&hiwdg);
        taskList[0].dt = DWT_GetDeltaT(&taskList[0].dt_cnt); //自身更新
        taskList[0].last_report_time = DWT_GetTimeline_s();
        float now = DWT_GetTimeline_s();
        for(uint8_t i = 0; i < taskCount; i++) {
            if(taskList[i].isActive) {
                float dt_since_last_report = now - taskList[i].last_report_time;
                // 检查任务执行间隔是否过长
                if(dt_since_last_report > TASK_BLOCK_TIMEOUT) {
                    // ThreadX临界区
                    UINT old_posture = tx_interrupt_control(TX_INT_DISABLE);
                    
                    ULOG_TAG_ERROR("**** Task Blocked Detected! System State Dump ****");
                    ULOG_TAG_ERROR("Time: %.3f s", DWT_GetTimeline_s());
                    ULOG_TAG_ERROR("----------------------------------------");
                    ULOG_TAG_ERROR("Blocked Task Information:");
                    // 打印被阻塞任务信息
                    PrintTaskInfo(&taskList[i]);
                    // 打印系统堆栈信息
                    PrintSystemStackInfo();
                    // 打印内存池信息
                    extern TX_BYTE_POOL tx_app_byte_pool;
                    PrintMemoryPoolInfo(&tx_app_byte_pool);

                    #if SystemWatch_Reset_Enable
                        HAL_NVIC_SystemReset(); 
                    #else
                        tx_thread_terminate(taskList[i].handle);
                        tx_thread_delete(taskList[i].handle);
                    #endif

                    // 从监控列表中移除任务
                    // 将后续任务前移
                    for (uint8_t j = i; j < taskCount - 1; j++) {
                        taskList[j] = taskList[j + 1];
                    }
                    taskCount--;
                    i--; // 调整索引，因为后面的元素已经前移

                    tx_interrupt_control(old_posture);
                }
            }
        }
        tx_thread_sleep(10);
    }
}



void systemwatch_init(TX_BYTE_POOL *pool)
{
    // 初始化任务列表
    memset(taskList, 0, sizeof(taskList));
    taskCount = 0;
    // 用内存池分配线程栈
    CHAR *watch_thread_stack;
    watch_thread_stack = threadx_malloc(SYSTEMWATCH_THREAD_STACK_SIZE);
    if (watch_thread_stack == NULL) {
        ULOG_TAG_ERROR("Failed to allocate memory for watch thread stack.");
        return;
    }
    UINT status=tx_thread_create(&watchTaskHandle, "WatchTask", SystemWatch_Task, 0,
                                    watch_thread_stack, SYSTEMWATCH_THREAD_STACK_SIZE,
                                    SYSTEMWATCH_THREAD_PRIORITY, SYSTEMWATCH_THREAD_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);

    if (status != TX_SUCCESS) {
        ULOG_TAG_ERROR("Failed to create watch task, status: %u", status);
        threadx_free(watch_thread_stack);
        return;
    }
    systemwatch_register_task(&watchTaskHandle, "WatchTask");
    ULOG_TAG_INFO("SystemWatch initialized, watch task created.");
}

static void PrintTaskInfo(TaskMonitor_t *pxTaskMonitor)
{
    ULOG_TAG_ERROR("Name: %s", pxTaskMonitor->name);
    ULOG_TAG_ERROR("status: %s",tx_thread_status[pxTaskMonitor->handle->tx_thread_state].name);
    ULOG_TAG_ERROR("Handle: 0x%p", (void*)pxTaskMonitor->handle);
    ULOG_TAG_ERROR("Last dt: %.3f ms", pxTaskMonitor->dt * 1000.0f);

    TX_THREAD *thread = pxTaskMonitor->handle;
    if (thread) {
        CHAR *stack_start = (CHAR *)thread->tx_thread_stack_start;
        ULONG stack_size = thread->tx_thread_stack_size;
        CHAR *stack_highest = (CHAR *)thread->tx_thread_stack_highest_ptr;

        // 注意：Cortex-M 架构栈向下增长
        ULONG stack_used = (ULONG)(stack_start + stack_size - stack_highest);
        ULONG stack_free = stack_size - stack_used;

        ULOG_TAG_ERROR("Stack start: 0x%p", stack_start);
        ULOG_TAG_ERROR("Stack size: %lu bytes", stack_size);
        ULOG_TAG_ERROR("Stack used: %lu bytes", stack_used);
        ULOG_TAG_ERROR("Stack free: %lu bytes", stack_free);
    } else {
        ULOG_TAG_ERROR("Thread pointer is NULL!");
    }
}

static void PrintSystemStackInfo(void)
{
    // 打印系统堆栈使用情况
    extern uint8_t _estack; /* 系统堆栈顶部地址 (由链接器脚本定义) */
    extern uint32_t _Min_Stack_Size; /* 最小堆栈大小 (由链接器脚本定义) */
    
    // 获取当前堆栈指针
    uint32_t current_stack_ptr;
    __asm__ volatile("MRS %0, MSP" : "=r"(current_stack_ptr));
    
    // 计算堆栈使用情况
    uint32_t stack_top = (uint32_t)&_estack;
    uint32_t stack_bottom = stack_top - (uint32_t)&_Min_Stack_Size;
    uint32_t stack_size = (uint32_t)&_Min_Stack_Size;
    uint32_t stack_used = stack_top - current_stack_ptr;
    uint32_t stack_free = current_stack_ptr - stack_bottom;
    
    ULOG_TAG_ERROR("----------------------------------------");
    ULOG_TAG_ERROR("System Stack Information:");
    ULOG_TAG_ERROR("Stack top (estack): 0x%08lX", stack_top);
    ULOG_TAG_ERROR("Stack bottom: 0x%08lX", stack_bottom);
    ULOG_TAG_ERROR("Current SP: 0x%08lX", current_stack_ptr);
    ULOG_TAG_ERROR("Stack size: %lu bytes", stack_size);
    ULOG_TAG_ERROR("Stack used: %lu bytes", stack_used);
    ULOG_TAG_ERROR("Stack free: %lu bytes", stack_free);
}

// 新增函数：打印内存池使用情况
static void PrintMemoryPoolInfo(TX_BYTE_POOL *pool)
{
    if (pool == TX_NULL) {
        ULOG_TAG_ERROR("----------------------------------------");
        ULOG_TAG_ERROR("Memory Pool Information: Pool pointer is NULL");
        return;
    }
    
    // 禁用中断以安全地访问内存池信息
    TX_INTERRUPT_SAVE_AREA
    TX_DISABLE
    
    // 获取内存池信息
    CHAR *pool_start = (CHAR *)pool->tx_byte_pool_start;
    ULONG pool_size = pool->tx_byte_pool_size;
    
    // 计算已使用的内存
    ULONG available_bytes = pool->tx_byte_pool_available;
    ULONG used_bytes = pool_size - available_bytes;
    
    // 计算内存碎片信息
    ULONG fragments = pool->tx_byte_pool_fragments;
    
    TX_RESTORE
    
    ULOG_TAG_ERROR("----------------------------------------");
    ULOG_TAG_ERROR("Memory Pool Information:");
    ULOG_TAG_ERROR("Pool start: 0x%p", pool_start);
    ULOG_TAG_ERROR("Pool size: %lu bytes", pool_size);
    ULOG_TAG_ERROR("Used bytes: %lu bytes", used_bytes);
    ULOG_TAG_ERROR("Available bytes: %lu bytes", available_bytes);
    ULOG_TAG_ERROR("Fragment count: %lu", fragments);
    ULOG_TAG_ERROR("Usage percentage: %.2f%%", (float)used_bytes * 100.0f / (float)pool_size);
}


int8_t systemwatch_register_task(TX_THREAD *taskHandle, const char* taskName)
{
    if (taskHandle == NULL || taskName == NULL) { return -1;}
    
    
    UINT old_posture = tx_interrupt_control(TX_INT_DISABLE);
    // 检查任务是否已注册
    for(uint8_t i = 0; i < taskCount; i++) {
        if(taskList[i].handle == taskHandle) {
            tx_interrupt_control(old_posture);
            return -1;
        }
    }
    // 检查是否超过最大监控任务数
    if(taskCount >= MAX_MONITORED_TASKS) {
        tx_interrupt_control(old_posture);
        return -1;
    }
    // 添加任务
    TaskMonitor_t* newTask = &taskList[taskCount];
    newTask->handle = taskHandle;
    newTask->name = taskName;
    newTask->dt = DWT_GetDeltaT(&taskList[taskCount].dt_cnt);
    newTask->isActive = 1;
    newTask->last_report_time = DWT_GetTimeline_s();

    // 增加任务计数
    taskCount++;

    tx_interrupt_control(old_posture);

    return 0;
}

void systemwatch_update_task(TX_THREAD *taskHandle)
{

    for(uint8_t i = 0; i < taskCount; i++) {
        if(taskList[i].handle == taskHandle) {
            taskList[i].last_report_time = DWT_GetTimeline_s();
            taskList[i].dt = DWT_GetDeltaT(&taskList[i].dt_cnt);
            HAL_IWDG_Refresh(&hiwdg);
            break;
        }
    }

}

#endif
