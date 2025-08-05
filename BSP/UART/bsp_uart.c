#include "bsp_uart.h"
#include "stm32f4xx_hal_uart.h"
#include "tx_api.h"
#include <stdint.h>
#include <string.h>


#define LOG_TAG "BSP_UART"
#include "ulog.h"
#include "ulog_port.h"


static UART_Device registered_uart[UART_MAX_INSTANCE_NUM] = {0}; // 结构体数组
static bool uart_used[UART_MAX_INSTANCE_NUM] = {false};         // 添加使用状态标记
static uint8_t default_buf[UART_MAX_INSTANCE_NUM][2][UART_DEFAULT_BUF_SIZE];

// 私有函数声明
static void Start_Rx(UART_Device *inst);
static void Process_Rx_Complete(UART_Device *inst, uint16_t Size);

UART_Device* BSP_UART_Init(UART_Device_init_config *config) {
    if (!config || !config->huart) {
        ULOG_TAG_ERROR(LOG_TAG, "UART_Init: Invalid configuration");
        return NULL;
    }
    // 检查实例是否已存在
    for(int i=0; i<UART_MAX_INSTANCE_NUM; i++){
        if(uart_used[i] && registered_uart[i].huart == config->huart)
        {   
            ULOG_TAG_ERROR(LOG_TAG, "UART_Init: Instance already exists");
            return NULL;
        }
    }
    // 查找空闲槽位
    int free_index = -1;
    for(int i=0; i<UART_MAX_INSTANCE_NUM; i++){
        if(!uart_used[i]){
            free_index = i;
            break;
        }
    }
    if(free_index == -1) {
        ULOG_TAG_ERROR(LOG_TAG, "UART_Init: No more instances available");
        return NULL;
    }
    // 初始化参数
    UART_Device *inst = &registered_uart[free_index];
    memset(inst, 0, sizeof(UART_Device));
    inst->huart = config->huart;
    inst->huart = config->huart;
    // 缓冲区配置部分
    if (config->rx_buf != NULL) {
        inst->rx_buf = (uint8_t (*)[2])config->rx_buf;
        inst->rx_buf_size = config->rx_buf_size;
    } else {
        inst->rx_buf = (uint8_t (*)[2])default_buf[free_index];  // 类型转换
        inst->rx_buf_size = UART_DEFAULT_BUF_SIZE;
    }
    // 配置接收长度
    inst->expected_len = config->expected_len;
    if (inst->expected_len > inst->rx_buf_size) 
    {
        inst->expected_len = inst->rx_buf_size; // 确保不超过缓冲区大小
    }
    // 配置模式和回调
    inst->rx_mode = config->rx_mode;
    inst->tx_mode = config->tx_mode;
    inst->rx_complete_cb = config->rx_complete_cb;
    inst->cb_type = config->cb_type;
    inst->event_flag = config->event_flag;
    //初始化事件标志组
    if (tx_event_flags_create(&inst->rx_event, "uart_event") != TX_SUCCESS) {
        ULOG_TAG_ERROR(LOG_TAG, "UART_Init: Failed to create event flags");
        return NULL;
    }
    // 初始化信号量
    if (tx_semaphore_create(&inst->tx_sem, "uart_sem", 1) != TX_SUCCESS) {
        ULOG_TAG_ERROR(LOG_TAG, "UART_Init: Failed to create semaphore");    
        tx_event_flags_delete(&inst->rx_event);
        return NULL;
    }
    // 增加使用计数
    uart_used[free_index] = true;
    // 启动接收
    Start_Rx(inst);
    ULOG_TAG_INFO("%s device , UART config - RX mode: %d, TX mode: %d, buffer size: %d", 
                    inst->huart->Instance == USART1 ? "USART1" :
                   inst->huart->Instance == USART3 ? "USART3" :
                   inst->huart->Instance == USART6 ? "USART6" : "Unknown",
                   inst->rx_mode, inst->tx_mode, inst->rx_buf_size);
    // 返回实例指针
    return inst;
}

HAL_StatusTypeDef BSP_UART_Send(UART_Device *inst, uint8_t *data, uint16_t len) {
    tx_semaphore_get(&inst->tx_sem, TX_WAIT_FOREVER);

    switch(inst->tx_mode){
        case UART_MODE_BLOCKING:
            HAL_UART_Transmit(inst->huart, data, len, 100);
            tx_semaphore_put(&inst->tx_sem);
            break;
            
        case UART_MODE_IT:
            HAL_UART_Transmit_IT(inst->huart, data, len);
            break;
            
        case UART_MODE_DMA:
            HAL_UART_Transmit_DMA(inst->huart, data, len);
            break;
    }
    return HAL_OK;
}



int BSP_UART_Read(UART_Device *inst, uint8_t **data) {
    if (!inst || !data) return 0;
    *data = inst->rx_buf[!inst->rx_active_buf];
    return inst->rx_len;
}

void BSP_UART_Deinit(UART_Device *inst) {
    for(int i=0; i<UART_MAX_INSTANCE_NUM; i++){
        if(&registered_uart[i] == inst){
            HAL_UART_Abort(inst->huart);
            tx_event_flags_delete(&inst->rx_event);
            tx_semaphore_delete(&inst->tx_sem);
            memset(inst, 0, sizeof(UART_Device));
            uart_used[i] = false;
            break;
        }
    }
}

/*--------------------------中断函数处理------------------------------------*/

// 接收完成回调（包括空闲中断）
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    for(int i=0; i<UART_MAX_INSTANCE_NUM; i++){
        if(uart_used[i] && registered_uart[i].huart == huart){
            Process_Rx_Complete(&registered_uart[i], Size);
            Start_Rx(&registered_uart[i]); // 重启接收
            break;
        }
    }
}

//发送完成回调
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    for(int i=0; i<UART_MAX_INSTANCE_NUM; i++){
        if(uart_used[i] && registered_uart[i].huart == huart){
            tx_semaphore_put(&registered_uart[i].tx_sem);
            break;
        }
    }
}


// 错误处理回调
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    for(int i=0; i<UART_MAX_INSTANCE_NUM; i++){
        if(uart_used[i] && registered_uart[i].huart == huart){        
            HAL_UART_AbortReceive(huart);
            Start_Rx(&registered_uart[i]);
            break;
        }
    }
}



/*----------------------------------私有函数实现----------------------------*/
static void Start_Rx(UART_Device *inst) {
    uint8_t next_buf = !inst->rx_active_buf;
    uint16_t max_len = inst->rx_buf_size;

    switch(inst->rx_mode){
        case UART_MODE_BLOCKING:
            HAL_UART_Receive(inst->huart, (uint8_t*)&inst->rx_buf[next_buf], 
                inst->expected_len ? inst->expected_len : max_len, 100);
            break;
            
        case UART_MODE_IT:
            HAL_UARTEx_ReceiveToIdle_IT(inst->huart, (uint8_t*)&inst->rx_buf[next_buf], 
                inst->expected_len ? inst->expected_len : max_len);
            break;
            
        case UART_MODE_DMA:
            HAL_UARTEx_ReceiveToIdle_DMA(inst->huart, (uint8_t*)&inst->rx_buf[next_buf], 
                inst->expected_len ? inst->expected_len : max_len);
            break;
    }
    
    inst->rx_active_buf = next_buf;
}

static void Process_Rx_Complete(UART_Device *inst, uint16_t Size) {
    // 计算实际接收长度
    if(inst->expected_len == 0){
        if(inst->rx_mode == UART_MODE_DMA){
            // 获取单缓冲区大小
            Size = inst->rx_buf_size - __HAL_DMA_GET_COUNTER(inst->huart->hdmarx);
            // 长度校验
            if(Size > inst->rx_buf_size){
                Size = inst->rx_buf_size;
            }
        }
    }
    inst->rx_len = Size;

    // 触发回调
    uint8_t *data = inst->rx_buf[!inst->rx_active_buf]; // 使用非活动缓冲区
    
    if(inst->cb_type == UART_CALLBACK_DIRECT){
        if(inst->rx_complete_cb)
            inst->rx_complete_cb(data, Size);
    }
    else{
        if(inst->cb_type == UART_CALLBACK_EVENT){
            if(inst->rx_event.tx_event_flags_group_id != TX_CLEAR_ID) {
                tx_event_flags_set(&inst->rx_event, inst->event_flag, TX_OR);
            }
        }
    }
}
