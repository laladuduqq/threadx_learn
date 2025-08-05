#include "bsp_adc.h"
#include <stdio.h>
#include <string.h>

#define LOG_TAG "bsp_adc"
#include "ulog.h"

/* ADC设备管理数组 */
static ADC_Device adc_devices[ADC_DEVICE_NUM] = {0};
static uint8_t adc_device_count = 0;
static uint8_t adc_initialized = 0;

/* 电压参考校准值 */
volatile fp32 voltage_vrefint_proportion = 8.0586080586080586080586080586081e-4f;

/**
 * @description: 查找空闲的ADC设备槽位
 * @return {ADC_Device*} 空闲设备指针，无空闲则返回NULL
 */
static ADC_Device* find_free_adc_device(void) {
    for (int i = 0; i < ADC_DEVICE_NUM; i++) {
        if (adc_devices[i].hadc == NULL) {
            return &adc_devices[i];
        }
    }
    return NULL;
}

/**
 * @description: 完成ADC转换操作
 * @param {ADC_Device*} dev ADC设备实例
 * @param {ADC_Event_Type} event 事件类型
 * @return {void}
 */
static void complete_adc_conversion(ADC_Device* dev, ADC_Event_Type event) {
    if (dev && dev->adc_callback) {
        dev->adc_callback(event);
    }
    
    // 释放互斥锁
    tx_mutex_put(&dev->adc_mutex);
}

/**
 * @description: 获取指定通道的ADC值
 * @param {ADC_HandleTypeDef*} ADCx ADC句柄
 * @param {uint32_t} ch ADC通道
 * @return {uint16_t} ADC转换结果
 */
static uint16_t adcx_get_chx_value(ADC_HandleTypeDef *ADCx, uint32_t ch)
{
    static ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ch;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;

    if (HAL_ADC_ConfigChannel(ADCx, &sConfig) != HAL_OK)
    {
        ULOG_TAG_ERROR("Failed to configure ADC channel %lu", ch);
        return 0;
    }

    HAL_ADC_Start(ADCx);
    HAL_ADC_PollForConversion(ADCx, 10);
    return (uint16_t)HAL_ADC_GetValue(ADCx);
}

/**
 * @description: 初始化内部参考电压倒数比例因子
 * @return {void}
 */
static void adc_init_vrefint_reciprocal(void)
{
    uint8_t i = 0;
    uint32_t total_adc = 0;
    for(i = 0; i < 200; i++)
    {
        total_adc += adcx_get_chx_value(&hadc1, ADC_CHANNEL_VREFINT);
    }

    voltage_vrefint_proportion = 200 * 1.2f / total_adc;
    ULOG_TAG_INFO("Vrefint reciprocal initialized: %f", (double)voltage_vrefint_proportion);
}

void BSP_ADC_Init(void) {
    if (adc_initialized) {
        return;
    }
    
    // 初始化所有ADC设备句柄为NULL
    memset(adc_devices, 0, sizeof(adc_devices));
    adc_device_count = 0;
    adc_initialized = 1;
    
    // 初始化参考电压校准值
    adc_init_vrefint_reciprocal();
    
    ULOG_TAG_INFO("ADC module initialized");
}

ADC_Device* BSP_ADC_Device_Init(ADC_Device_Init_Config* config) {
    if (!config || !config->hadc) {
        ULOG_TAG_ERROR("Invalid ADC configuration parameters");
        return NULL;
    }
    
    // 首次注册时初始化模块
    if (!adc_initialized) {
        BSP_ADC_Init();
    }
    
    // 查找空闲设备
    ADC_Device* adc_dev = find_free_adc_device();
    if (!adc_dev) {
        ULOG_TAG_ERROR("No free ADC device slots available");
        return NULL;
    }
    
    // 初始化设备参数
    adc_dev->hadc = config->hadc;
    adc_dev->channel_config = config->channel_config;
    adc_dev->mode = config->mode;
    adc_dev->timeout = config->timeout;
    adc_dev->adc_callback = config->adc_callback;
    adc_dev->value = 0;
    
    // 创建互斥锁
    char mutex_name[16];
    snprintf(mutex_name, sizeof(mutex_name), "ADC_Mutex_%d", adc_device_count);
    if (tx_mutex_create(&adc_dev->adc_mutex, mutex_name, TX_NO_INHERIT) != TX_SUCCESS) {
        ULOG_TAG_ERROR("Failed to create ADC mutex");
        memset(adc_dev, 0, sizeof(ADC_Device));
        return NULL;
    }
    
    // 配置ADC通道
    if (HAL_ADC_ConfigChannel(adc_dev->hadc, &adc_dev->channel_config) != HAL_OK) {
        ULOG_TAG_ERROR("Failed to configure ADC channel");
        tx_mutex_delete(&adc_dev->adc_mutex);
        memset(adc_dev, 0, sizeof(ADC_Device));
        return NULL;
    }
    
    adc_device_count++;
    ULOG_TAG_INFO("ADC device initialized: ADC handle %p, Channel %lu", 
                  adc_dev->hadc, adc_dev->channel_config.Channel);
    
    return adc_dev;
}

void BSP_ADC_Device_DeInit(ADC_Device* dev) {
    if (!dev) {
        return;
    }
    
    // 停止ADC转换
    BSP_ADC_Stop(dev);
    
    // 删除互斥锁
    tx_mutex_delete(&dev->adc_mutex);
    
    // 清空设备信息
    memset(dev, 0, sizeof(ADC_Device));
    adc_device_count--;
    
    ULOG_TAG_INFO("ADC device deinitialized");
}

HAL_StatusTypeDef BSP_ADC_Start(ADC_Device* dev) {
    if (!dev || !dev->hadc) {
        ULOG_TAG_ERROR("Invalid ADC device");
        return HAL_ERROR;
    }
    
    // 获取互斥锁
    if (tx_mutex_get(&dev->adc_mutex, TX_WAIT_FOREVER) != TX_SUCCESS) {
        ULOG_TAG_ERROR("Failed to get ADC mutex");
        return HAL_ERROR;
    }
    
    HAL_StatusTypeDef status = HAL_OK;
    
    switch (dev->mode) {
        case ADC_MODE_BLOCKING:
            status = HAL_ADC_Start(dev->hadc);
            if (status == HAL_OK) {
                // 等待转换完成
                status = HAL_ADC_PollForConversion(dev->hadc, dev->timeout);
                if (status == HAL_OK) {
                    dev->value = HAL_ADC_GetValue(dev->hadc);
                }
            }
            complete_adc_conversion(dev, ADC_EVENT_CONVERSION_COMPLETE);
            break;
            
        case ADC_MODE_IT:
            status = HAL_ADC_Start_IT(dev->hadc);
            // 中断模式下不立即释放锁，将在回调中释放
            if (status != HAL_OK) {
                complete_adc_conversion(dev, ADC_EVENT_ERROR);
            }
            break;
            
        case ADC_MODE_DMA:
            status = HAL_ADC_Start_DMA(dev->hadc, &dev->value, 1);
            // DMA模式下不立即释放锁，将在回调中释放
            if (status != HAL_OK) {
                complete_adc_conversion(dev, ADC_EVENT_ERROR);
            }
            break;
            
        default:
            status = HAL_ERROR;
            complete_adc_conversion(dev, ADC_EVENT_ERROR);
            break;
    }
    
    return status;
}

HAL_StatusTypeDef BSP_ADC_Stop(ADC_Device* dev) {
    if (!dev || !dev->hadc) {
        ULOG_TAG_ERROR("Invalid ADC device");
        return HAL_ERROR;
    }
    
    HAL_StatusTypeDef status = HAL_OK;
    
    switch (dev->mode) {
        case ADC_MODE_BLOCKING:
            status = HAL_ADC_Stop(dev->hadc);
            break;
            
        case ADC_MODE_IT:
            status = HAL_ADC_Stop_IT(dev->hadc);
            break;
            
        case ADC_MODE_DMA:
            status = HAL_ADC_Stop_DMA(dev->hadc);
            break;
            
        default:
            status = HAL_ERROR;
            break;
    }
    
    if (status == HAL_OK) {
        ULOG_TAG_INFO("ADC stopped successfully");
    } else {
        ULOG_TAG_ERROR("Failed to stop ADC");
    }
    
    return status;
}

HAL_StatusTypeDef BSP_ADC_Get_Value(ADC_Device* dev, uint32_t* value) {
    if (!dev || !value) {
        ULOG_TAG_ERROR("Invalid parameters");
        return HAL_ERROR;
    }
    
    *value = dev->value;
    return HAL_OK;
}

/**
 * @description: 获取温度值
 * @return {fp32} 温度值(摄氏度)
 */
fp32 BSP_ADC_Get_Temperature(void)
{
    uint16_t adcx = 0;
    fp32 temperate;

    // 确保ADC已初始化
    if (!adc_initialized) {
        BSP_ADC_Init();
    }

    adcx = adcx_get_chx_value(&hadc1, ADC_CHANNEL_TEMPSENSOR);
    temperate = (fp32)adcx * voltage_vrefint_proportion;
    temperate = (temperate - 0.76f) * 400.0f + 25.0f;

    return temperate;
}



/**
 * @description: ADC转换完成回调函数
 * @param {ADC_HandleTypeDef*} hadc ADC句柄
 * @return {void}
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    // 查找对应的设备
    for (int i = 0; i < ADC_DEVICE_NUM; i++) {
        if (adc_devices[i].hadc == hadc) {
            ADC_Device* dev = &adc_devices[i];
            dev->value = HAL_ADC_GetValue(hadc);
            complete_adc_conversion(dev, ADC_EVENT_CONVERSION_COMPLETE);
            return;
        }
    }
}

/**
 * @description: ADC错误回调函数
 * @param {ADC_HandleTypeDef*} hadc ADC句柄
 * @return {void}
 */
void HAL_ADC_ErrorCallback(ADC_HandleTypeDef* hadc) {
    // 查找对应的设备
    for (int i = 0; i < ADC_DEVICE_NUM; i++) {
        if (adc_devices[i].hadc == hadc) {
            ADC_Device* dev = &adc_devices[i];
            complete_adc_conversion(dev, ADC_EVENT_ERROR);
            return;
        }
    }
}