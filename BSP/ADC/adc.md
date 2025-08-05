<!--
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-05 17:55:18
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-05 18:07:32
 * @FilePath: /threadx_learn/BSP/ADC/adc.md
 * @Description: 
-->

# ADC 驱动文档

## 简介

BSP_ADC 是一个基于 ThreadX RTOS 的 ADC 驱动模块，提供了对 STM32 ADC 外设的高级封装。该模块支持多设备管理、线程安全访问以及阻塞、中断和 DMA 三种转换模式，适用于各种 ADC 采集需求。

## 特性

1. **多设备支持**：支持最多 ADC_DEVICE_NUM 个 ADC 设备
2. **线程安全**：使用 ThreadX 互斥锁保护 ADC 访问
3. **多种转换模式**：
   - 阻塞模式（Blocking）
   - 中断模式（Interrupt）
   - DMA 模式（Direct Memory Access）
4. **事件回调机制**：支持转换完成和错误事件回调
5. **灵活配置**：支持多种 ADC 通道和采样时间配置
6. **系统参数检测**：支持温度传感器和电池电压检测
7. **自动校准**：在初始化时自动完成参考电压校准

## 数据结构

### ADC_Mode

ADC 工作模式枚举：

```c
typedef enum {
    ADC_MODE_BLOCKING,           // 阻塞模式
    ADC_MODE_IT,                 // 中断模式
    ADC_MODE_DMA                 // DMA模式
} ADC_Mode;
```

### ADC_Event_Type

ADC 事件类型枚举：

```c
typedef enum {
    ADC_EVENT_CONVERSION_COMPLETE,  // 转换完成
    ADC_EVENT_ERROR                 // 错误事件
} ADC_Event_Type;
```

### ADC_Device

ADC 设备实例结构体：

```c
typedef struct {
    ADC_HandleTypeDef* hadc;            // ADC句柄
    ADC_ChannelConfTypeDef channel_config; // 通道配置
    ADC_Mode mode;                     // 工作模式
    uint32_t timeout;                  // 超时时间（仅阻塞模式使用）
    void (*adc_callback)(ADC_Event_Type event); // 回调函数
    uint32_t value;                    // 转换结果
    TX_MUTEX adc_mutex;                // 互斥锁保护ADC访问
} ADC_Device;
```

### ADC_Device_Init_Config

ADC 初始化配置结构体：

```c
typedef struct {
    ADC_HandleTypeDef* hadc;
    ADC_ChannelConfTypeDef channel_config;
    ADC_Mode mode;
    uint32_t timeout;
    void (*adc_callback)(ADC_Event_Type event);
} ADC_Device_Init_Config;
```

## 使用示例

### 基本ADC操作

```c
// ADC 设备初始化配置
ADC_Device_Init_Config adc_config = {
    .hadc = &hadc1,
    .channel_config = {
        .Channel = ADC_CHANNEL_0,
        .Rank = 1,
        .SamplingTime = ADC_SAMPLETIME_3CYCLES,
        .Offset = 0
    },
    .mode = ADC_MODE_BLOCKING,
    .timeout = 100,
    .adc_callback = NULL  // 阻塞模式下不需要回调
};

// 初始化ADC模块（系统启动时调用一次，包含自动校准）
BSP_ADC_Init();

// 初始化ADC设备
ADC_Device* adc_dev = BSP_ADC_Device_Init(&adc_config);
if (adc_dev == NULL) {
    printf("ADC设备初始化失败\n");
    return;
}

// 启动ADC转换
if (BSP_ADC_Start(adc_dev) != HAL_OK) {
    printf("ADC转换启动失败\n");
    return;
}

// 获取转换结果
uint32_t adc_value;
if (BSP_ADC_Get_Value(adc_dev, &adc_value) == HAL_OK) {
    printf("ADC转换结果: %lu\n", adc_value);
}

// 销毁ADC设备
BSP_ADC_Device_DeInit(adc_dev);
```

### 中断模式ADC操作

```c
// 定义回调函数
void adc_callback_handler(ADC_Event_Type event) {
    switch(event) {
        case ADC_EVENT_CONVERSION_COMPLETE:
            // 处理转换完成事件
            printf("ADC转换完成\n");
            break;
        case ADC_EVENT_ERROR:
            // 处理错误事件
            printf("ADC错误发生\n");
            break;
    }
}

// ADC 设备初始化配置（中断模式）
ADC_Device_Init_Config adc_config_it = {
    .hadc = &hadc1,
    .channel_config = {
        .Channel = ADC_CHANNEL_1,
        .Rank = 1,
        .SamplingTime = ADC_SAMPLETIME_3CYCLES,
        .Offset = 0
    },
    .mode = ADC_MODE_IT,
    .timeout = 0,  // 中断模式下不需要超时
    .adc_callback = adc_callback_handler
};

// 初始化ADC设备
ADC_Device* adc_dev_it = BSP_ADC_Device_Init(&adc_config_it);
if (adc_dev_it == NULL) {
    printf("ADC设备初始化失败\n");
    return;
}

// 启动ADC转换（中断模式）
if (BSP_ADC_Start(adc_dev_it) != HAL_OK) {
    printf("ADC转换启动失败\n");
    return;
}

// 在回调函数中处理转换结果
// ...

// 停止ADC转换
BSP_ADC_Stop(adc_dev_it);

// 销毁ADC设备
BSP_ADC_Device_DeInit(adc_dev_it);
```

### DMA模式ADC操作

```c
// ADC 设备初始化配置（DMA模式）
ADC_Device_Init_Config adc_config_dma = {
    .hadc = &hadc1,
    .channel_config = {
        .Channel = ADC_CHANNEL_2,
        .Rank = 1,
        .SamplingTime = ADC_SAMPLETIME_3CYCLES,
        .Offset = 0
    },
    .mode = ADC_MODE_DMA,
    .timeout = 0,  // DMA模式下不需要超时
    .adc_callback = adc_callback_handler  // 可选的回调函数
};

// 初始化ADC设备
ADC_Device* adc_dev_dma = BSP_ADC_Device_Init(&adc_config_dma);
if (adc_dev_dma == NULL) {
    printf("ADC设备初始化失败\n");
    return;
}

// 启动ADC转换（DMA模式）
if (BSP_ADC_Start(adc_dev_dma) != HAL_OK) {
    printf("ADC转换启动失败\n");
    return;
}

// 获取转换结果（DMA会自动更新结果）
uint32_t dma_adc_value;
if (BSP_ADC_Get_Value(adc_dev_dma, &dma_adc_value) == HAL_OK) {
    printf("DMA ADC转换结果: %lu\n", dma_adc_value);
}

// 停止ADC转换
BSP_ADC_Stop(adc_dev_dma);

// 销毁ADC设备
BSP_ADC_Device_DeInit(adc_dev_dma);
```

### 系统参数检测

```c
// 系统启动时初始化ADC模块（包含自动校准）
BSP_ADC_Init();

// 获取芯片温度
fp32 temperature = BSP_ADC_Get_Temperature();
printf("芯片温度: %.2f°C\n", (double)temperature);

// 获取电池电压
fp32 battery_voltage = BSP_ADC_Get_Battery_Voltage();
printf("电池电压: %.2fV\n", (double)battery_voltage);
```

## 注意事项

1. **线程安全**：所有API都是线程安全的，可以在多个线程中同时调用
2. **回调函数**：回调函数在中断上下文中执行，应避免在回调函数中进行耗时操作
3. **DMA模式**：使用DMA模式时，转换结果会自动更新到设备结构体中
4. **初始化顺序**：需要先调用 BSP_ADC_Init 初始化模块，再初始化具体设备
5. **自动校准**：参考电压校准在 BSP_ADC_Init 中自动完成，无需手动调用
6. **温度传感器**：使用温度传感器前需要确保ADC1已正确配置并启用
7. **错误处理**：应及时处理ADC错误事件，避免影响系统稳定性

## 常见问题

1. **ADC设备初始化失败**：
   
   - 检查ADC句柄是否正确配置
   - 确认设备数量未超过最大设备数限制
   - 检查ADC通道配置是否正确

2. **转换失败**：
   
   - 检查ADC时钟配置是否正确
   - 确认ADC引脚配置是否正确
   - 检查电源和地线连接

3. **回调函数未执行**：
   
   - 确认回调函数指针是否正确设置
   - 检查NVIC中断配置是否正确
   - 确认HAL库回调函数是否被正确调用

4. **转换结果为0或异常值**：
   
   - 检查ADC输入信号是否正确
   - 确认参考电压是否稳定
   - 检查采样时间设置是否合适

5. 
