#include "bsp_gpio.h"
#include <string.h>

#define LOG_TAG "bsp_gpio"
#include "ulog.h"

/* GPIO中断回调设备管理数组 */
static GPIO_EXTI_Device gpio_exti_devices[GPIO_EXTI_DEVICE_NUM] = {0};
static uint8_t gpio_exti_initialized = 0;

void BSP_GPIO_EXTI_Module_Init(void) {
    if (gpio_exti_initialized) {
        return;
    }
    
    // 初始化所有GPIO中断回调设备为未使用状态
    memset(gpio_exti_devices, 0, sizeof(gpio_exti_devices));
    gpio_exti_initialized = 1;
    
    ULOG_TAG_INFO("GPIO EXTI callback module initialized");
}

int BSP_GPIO_EXTI_Register_Callback(uint16_t pin, gpio_exti_callback_t callback) {
    // 首次注册时初始化模块
    if (!gpio_exti_initialized) {
        BSP_GPIO_EXTI_Module_Init();
    }
    
    // 检查参数有效性
    if (!callback) {
        ULOG_TAG_ERROR("Invalid callback function");
        return -1;
    }
    
    // 确定引脚号(0-15)
    uint8_t pin_num = 0;
    uint8_t pin_found = 0;
    for (int i = 0; i < 16; i++) {
        if (pin & (1 << i)) {
            if (pin_found) {
                // 多个引脚被设置，无效
                ULOG_TAG_ERROR("Multiple pins specified in pin mask");
                return -1;
            }
            pin_num = i;
            pin_found = 1;
        }
    }
    
    if (!pin_found) {
        ULOG_TAG_ERROR("No valid pin specified");
        return -1;
    }
    
    // 检查是否已注册过该引脚的回调
    if (gpio_exti_devices[pin_num].is_used) {
        ULOG_TAG_WARNING("Callback already registered for pin %d, overwriting", pin_num);
    }
    
    // 注册回调函数
    gpio_exti_devices[pin_num].pin = pin;
    gpio_exti_devices[pin_num].callback = callback;
    gpio_exti_devices[pin_num].is_used = 1;
    
    ULOG_TAG_INFO("GPIO EXTI callback registered for pin %d", pin_num);
    return 0;
}

int BSP_GPIO_EXTI_Unregister_Callback(uint16_t pin) {
    // 确定引脚号(0-15)
    uint8_t pin_num = 0;
    uint8_t pin_found = 0;
    for (int i = 0; i < 16; i++) {
        if (pin & (1 << i)) {
            if (pin_found) {
                // 多个引脚被设置，无效
                ULOG_TAG_ERROR("Multiple pins specified in pin mask");
                return -1;
            }
            pin_num = i;
            pin_found = 1;
        }
    }
    
    if (!pin_found) {
        ULOG_TAG_ERROR("No valid pin specified");
        return -1;
    }
    
    // 检查该引脚是否已注册回调
    if (!gpio_exti_devices[pin_num].is_used) {
        ULOG_TAG_WARNING("No callback registered for pin %d", pin_num);
        return -1;
    }
    
    // 注销回调函数
    gpio_exti_devices[pin_num].pin = 0;
    gpio_exti_devices[pin_num].callback = NULL;
    gpio_exti_devices[pin_num].is_used = 0;
    
    ULOG_TAG_INFO("GPIO EXTI callback unregistered for pin %d", pin_num);
    return 0;
}

/**
 * @description: GPIO外部中断回调处理函数
 * @param {uint16_t} pin GPIO引脚号
 * @return {void}
 */
void BSP_GPIO_EXTI_Handle_Callback(uint16_t pin) {
    // 确定引脚号(0-15)
    uint8_t pin_num = 0;
    uint8_t pin_found = 0;
    for (int i = 0; i < 16; i++) {
        if (pin & (1 << i)) {
            if (pin_found) {
                // 多个引脚被设置，无效
                return;
            }
            pin_num = i;
            pin_found = 1;
        }
    }
    
    if (!pin_found) {
        return;
    }
    
    // 检查该引脚是否已注册回调
    if (gpio_exti_devices[pin_num].is_used && gpio_exti_devices[pin_num].callback) {
        // 执行回调函数
        gpio_exti_devices[pin_num].callback(GPIO_EXTI_EVENT_TRIGGERED);
    }
}


//中断处理函数
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
    // 调用外部中断回调处理函数
    BSP_GPIO_EXTI_Handle_Callback(GPIO_Pin);
}
