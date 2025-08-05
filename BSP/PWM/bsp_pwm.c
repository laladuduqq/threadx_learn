#include "bsp_pwm.h"
#include "stm32f4xx_hal_tim.h"
#include <string.h>
#include <math.h>

#define LOG_TAG "bsp_pwm"
#include "ulog.h"

/* PWM设备管理数组 */
static PWM_Device pwm_devices[MAX_PWM_DEVICES] = {0};
static uint8_t pwm_device_count = 0;

/**
 * @description: 查找空闲的PWM设备槽位
 * @return {PWM_Device*} 空闲设备指针，无空闲则返回NULL
 */
static PWM_Device* find_free_pwm_device(void) {
    for (int i = 0; i < MAX_PWM_DEVICES; i++) {
        if (pwm_devices[i].htim == NULL) {
            return &pwm_devices[i];
        }
    }
    return NULL;
}

/**
 * @description: 计算定时器参数
 * @param {PWM_Device*} pwm_dev PWM设备实例
 * @return {void}
 */
static void calculate_timer_params(PWM_Device* pwm_dev) {
    // 获取定时器时钟频率
    uint32_t tim_clock = HAL_RCC_GetPCLK2Freq(); // 默认使用APB2时钟
    
    // 如果APB2预分频器不等于1，时钟频率需要乘以2
    if ((RCC->CFGR & RCC_CFGR_PPRE2) != 0) {
        tim_clock *= 2;
    }
    
    // 对于某些定时器可能使用APB1时钟
    if (pwm_dev->htim->Instance == TIM2 || pwm_dev->htim->Instance == TIM3 || 
        pwm_dev->htim->Instance == TIM4 || pwm_dev->htim->Instance == TIM5 ||
        pwm_dev->htim->Instance == TIM6 || pwm_dev->htim->Instance == TIM7 ||
        pwm_dev->htim->Instance == TIM12 || pwm_dev->htim->Instance == TIM13 || 
        pwm_dev->htim->Instance == TIM14) {
        tim_clock = HAL_RCC_GetPCLK1Freq();
        if ((RCC->CFGR & RCC_CFGR_PPRE1) != 0) {
            tim_clock *= 2;
        }
    }
    
    // 计算周期和脉冲宽度
    // Period = (tim_clock / (prescaler + 1)) / frequency - 1
    // 为了简化计算，我们使用预分频器为0的情况
    pwm_dev->period = tim_clock / pwm_dev->frequency - 1;
    
    // 确保周期不超过16位最大值
    if (pwm_dev->period > 0xFFFF) {
        // 需要使用预分频器
        uint32_t prescaler = (pwm_dev->period >> 16) + 1;
        pwm_dev->period = (tim_clock / prescaler) / pwm_dev->frequency - 1;
    } else {
        // 不需要预分频器
        pwm_dev->htim->Init.Prescaler = 0;
    }
    
    // 计算脉冲宽度
    pwm_dev->pulse = (pwm_dev->period + 1) * pwm_dev->duty_cycle / 100;
}

/**
 * @description: 配置定时器PWM通道
 * @param {PWM_Device*} pwm_dev PWM设备实例
 * @return {HAL_StatusTypeDef} HAL状态
 */
static HAL_StatusTypeDef configure_pwm_channel(PWM_Device* pwm_dev) {
    TIM_OC_InitTypeDef sConfigOC = {0};
    
    // 根据模式设置OC模式
    if (pwm_dev->mode == PWM_MODE_INVERTED) {
        sConfigOC.OCMode = TIM_OCMODE_PWM2;
    } else {
        sConfigOC.OCMode = TIM_OCMODE_PWM1;
    }
    
    sConfigOC.Pulse = pwm_dev->pulse;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    
    return HAL_TIM_PWM_ConfigChannel(pwm_dev->htim, &sConfigOC, pwm_dev->channel);
}

PWM_Device* BSP_PWM_Init(PWM_Init_Config* config) {
    if (!config || !config->htim || 
        config->duty_cycle > 100 || 
        config->frequency == 0) {
        ULOG_TAG_ERROR("Invalid PWM configuration parameters");
        return NULL;
    }
    
    // 查找空闲设备
    PWM_Device* pwm_dev = find_free_pwm_device();
    if (!pwm_dev) {
        ULOG_TAG_ERROR("No free PWM device slots available");
        return NULL;
    }
    
    // 初始化设备参数
    pwm_dev->htim = config->htim;
    pwm_dev->channel = config->channel;
    pwm_dev->frequency = config->frequency;
    pwm_dev->duty_cycle = config->duty_cycle;
    pwm_dev->mode = config->mode;
    
    // 计算定时器参数
    calculate_timer_params(pwm_dev);
    
    // 配置定时器基础参数
    pwm_dev->htim->Init.Prescaler = 0;
    pwm_dev->htim->Init.CounterMode = TIM_COUNTERMODE_UP;
    pwm_dev->htim->Init.Period = pwm_dev->period;
    pwm_dev->htim->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    pwm_dev->htim->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    
    // 初始化定时器
    if (HAL_TIM_PWM_Init(pwm_dev->htim) != HAL_OK) {
        ULOG_TAG_ERROR("Failed to initialize PWM timer");
        memset(pwm_dev, 0, sizeof(PWM_Device));
        return NULL;
    }
    
    // 配置PWM通道
    if (configure_pwm_channel(pwm_dev) != HAL_OK) {
        ULOG_TAG_ERROR("Failed to configure PWM channel");
        memset(pwm_dev, 0, sizeof(PWM_Device));
        return NULL;
    }
    
    pwm_device_count++;
    ULOG_TAG_INFO("PWM device initialized: TIM %p, Channel %d, Frequency %lu Hz, Duty Cycle %d%%", 
                  pwm_dev->htim, pwm_dev->channel, pwm_dev->frequency, pwm_dev->duty_cycle);
    
    return pwm_dev;
}

HAL_StatusTypeDef BSP_PWM_Start(PWM_Device* pwm_dev) {
    if (!pwm_dev || !pwm_dev->htim) {
        ULOG_TAG_ERROR("Invalid PWM device");
        return HAL_ERROR;
    }
    
    HAL_StatusTypeDef status = HAL_TIM_PWM_Start(pwm_dev->htim, pwm_dev->channel);
    if (status == HAL_OK) {
        ULOG_TAG_INFO("PWM started: TIM %p, Channel %d", pwm_dev->htim, pwm_dev->channel);
    } else {
        ULOG_TAG_ERROR("Failed to start PWM: TIM %p, Channel %d", pwm_dev->htim, pwm_dev->channel);
    }
    
    return status;
}

HAL_StatusTypeDef BSP_PWM_Stop(PWM_Device* pwm_dev) {
    if (!pwm_dev || !pwm_dev->htim) {
        ULOG_TAG_ERROR("Invalid PWM device");
        return HAL_ERROR;
    }
    
    HAL_StatusTypeDef status = HAL_TIM_PWM_Stop(pwm_dev->htim, pwm_dev->channel);
    if (status == HAL_OK) {
        ULOG_TAG_INFO("PWM stopped: TIM %p, Channel %d", pwm_dev->htim, pwm_dev->channel);
    } else {
        ULOG_TAG_ERROR("Failed to stop PWM: TIM %p, Channel %d", pwm_dev->htim, pwm_dev->channel);
    }
    
    return status;
}

HAL_StatusTypeDef BSP_PWM_Set_Frequency(PWM_Device* pwm_dev, uint32_t frequency) {
    if (!pwm_dev || !pwm_dev->htim || frequency == 0) {
        ULOG_TAG_ERROR("Invalid PWM device or frequency");
        return HAL_ERROR;
    }
    
    // 保存当前占空比
    uint8_t current_duty_cycle = pwm_dev->duty_cycle;
    
    // 更新频率
    pwm_dev->frequency = frequency;
    
    // 重新计算定时器参数
    calculate_timer_params(pwm_dev);
    
    // 更新定时器周期
    __HAL_TIM_SET_AUTORELOAD(pwm_dev->htim, pwm_dev->period);
    
    // 重新计算脉冲宽度以保持占空比
    pwm_dev->pulse = (pwm_dev->period + 1) * current_duty_cycle / 100;
    __HAL_TIM_SET_COMPARE(pwm_dev->htim, pwm_dev->channel, pwm_dev->pulse);
    
    ULOG_TAG_DEBUG("PWM frequency updated: %lu Hz, Duty Cycle: %d%%", frequency, current_duty_cycle);
    
    return HAL_OK;
}

HAL_StatusTypeDef BSP_PWM_Set_DutyCycle(PWM_Device* pwm_dev, uint8_t duty_cycle) {
    if (!pwm_dev || !pwm_dev->htim || duty_cycle > 100) {
        ULOG_TAG_ERROR("Invalid PWM device or duty cycle");
        return HAL_ERROR;
    }
    
    // 更新占空比
    pwm_dev->duty_cycle = duty_cycle;
    
    // 重新计算脉冲宽度
    pwm_dev->pulse = (pwm_dev->period + 1) * duty_cycle / 100;
    
    // 更新比较值
    __HAL_TIM_SET_COMPARE(pwm_dev->htim, pwm_dev->channel, pwm_dev->pulse);
    
    ULOG_TAG_DEBUG("PWM duty cycle updated: %d%%", duty_cycle);
    
    return HAL_OK;
}

HAL_StatusTypeDef BSP_PWM_Set_Frequency_And_DutyCycle(PWM_Device* pwm_dev, uint32_t frequency, uint8_t duty_cycle) {
    if (!pwm_dev || !pwm_dev->htim || duty_cycle > 100 || frequency == 0) {
        ULOG_TAG_ERROR("Invalid PWM device, frequency or duty cycle");
        return HAL_ERROR;
    }
    
    // 更新参数
    pwm_dev->frequency = frequency;
    pwm_dev->duty_cycle = duty_cycle;
    
    // 重新计算定时器参数
    calculate_timer_params(pwm_dev);
    
    // 更新定时器周期
    __HAL_TIM_SET_AUTORELOAD(pwm_dev->htim, pwm_dev->period);
    
    // 更新脉冲宽度
    pwm_dev->pulse = (pwm_dev->period + 1) * duty_cycle / 100;
    __HAL_TIM_SET_COMPARE(pwm_dev->htim, pwm_dev->channel, pwm_dev->pulse);
    
    ULOG_TAG_DEBUG("PWM frequency and duty cycle updated: %lu Hz, %d%%", frequency, duty_cycle);
    
    return HAL_OK;
}

uint32_t BSP_PWM_Get_Frequency(PWM_Device* pwm_dev) {
    if (!pwm_dev) {
        return 0;
    }
    return pwm_dev->frequency;
}

uint8_t BSP_PWM_Get_DutyCycle(PWM_Device* pwm_dev) {
    if (!pwm_dev) {
        return 0;
    }
    return pwm_dev->duty_cycle;
}

void BSP_PWM_DeInit(PWM_Device* pwm_dev) {
    if (!pwm_dev) {
        return;
    }
    
    // 停止PWM输出
    BSP_PWM_Stop(pwm_dev);
    
    // 清空设备信息
    memset(pwm_dev, 0, sizeof(PWM_Device));
    pwm_device_count--;
    
    ULOG_TAG_INFO("PWM device deinitialized");
}