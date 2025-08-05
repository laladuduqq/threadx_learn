<!--
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-05 16:00:23
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-05 16:00:45
 * @FilePath: /threadx_learn/BSP/PWM/pwm.md
 * @Description: 
-->

# PWM 驱动文档

## 简介

BSP_PWM 是一个基于 STM32 HAL 库的 PWM 驱动模块，提供了对 STM32 定时器 PWM 功能的高级封装。该模块支持多设备管理，可以方便地配置和控制 PWM 输出的频率和占空比。

## 特性

1. **多设备支持**：支持最多 MAX_PWM_DEVICES 个 PWM 设备
2. **频率控制**：可动态设置 PWM 输出频率
3. **占空比控制**：可动态设置 PWM 输出占空比(0-100%)
4. **模式选择**：支持普通和反转两种 PWM 模式
5. **动态配置**：支持运行时动态修改频率和占空比

## 数据结构

### PWM_Mode

PWM 模式枚举：

```c
typedef enum {
    PWM_MODE_NORMAL,     // 普通PWM模式
    PWM_MODE_INVERTED    // 反转PWM模式
} PWM_Mode;
```

### PWM_Channel

PWM 通道枚举：

```c
typedef enum {
    PWM_CHANNEL_1 = TIM_CHANNEL_1,
    PWM_CHANNEL_2 = TIM_CHANNEL_2,
    PWM_CHANNEL_3 = TIM_CHANNEL_3,
    PWM_CHANNEL_4 = TIM_CHANNEL_4
} PWM_Channel;
```

### PWM_Device

PWM 设备实例结构体：

```c
typedef struct {
    TIM_HandleTypeDef* htim;      // 定时器句柄
    PWM_Channel channel;          // PWM通道
    uint32_t period;             // 周期值
    uint32_t pulse;              // 脉冲宽度值
    uint32_t frequency;          // 频率(Hz)
    uint8_t duty_cycle;          // 占空比(0-100)
    PWM_Mode mode;               // PWM模式
} PWM_Device;
```

### PWM_Init_Config

PWM 初始化配置结构体：

```c
typedef struct {
    TIM_HandleTypeDef* htim;
    PWM_Channel channel;
    uint32_t frequency;          // 频率(Hz)
    uint8_t duty_cycle;          // 初始占空比(0-100)
    PWM_Mode mode;               // PWM模式
} PWM_Init_Config;
```

## 使用示例

### 基本PWM操作

```c
// 配置PWM设备
PWM_Init_Config pwm_config = {
    .htim = &htim1,              // 使用TIM1
    .channel = PWM_CHANNEL_1,    // 使用通道1
    .frequency = 1000,           // 频率1kHz
    .duty_cycle = 50,            // 占空比50%
    .mode = PWM_MODE_NORMAL      // 普通模式
};

// 初始化PWM设备
PWM_Device* pwm_dev = BSP_PWM_Init(&pwm_config);
if (pwm_dev == NULL) {
    printf("PWM设备初始化失败\n");
    return;
}

// 启动PWM输出
if (BSP_PWM_Start(pwm_dev) != HAL_OK) {
    printf("PWM启动失败\n");
    return;
}

// 延时一段时间
HAL_Delay(1000);

// 修改PWM频率
BSP_PWM_Set_Frequency(pwm_dev, 2000); // 修改为2kHz

// 修改PWM占空比
BSP_PWM_Set_DutyCycle(pwm_dev, 75); // 修改为75%占空比

// 同时修改频率和占空比
BSP_PWM_Set_Frequency_And_DutyCycle(pwm_dev, 500, 30); // 修改为500Hz，30%占空比

// 延时一段时间
HAL_Delay(1000);

// 停止PWM输出
BSP_PWM_Stop(pwm_dev);

// 销毁PWM设备
BSP_PWM_DeInit(pwm_dev);
```

## 注意事项

1. **定时器初始化**：确保在使用 BSP_PWM_Init 之前已经正确初始化了对应的定时器
2. **频率范围**：PWM 频率受定时器时钟频率限制，过高频率可能无法实现
3. **占空比范围**：占空比范围为 0-100，超出范围将导致函数返回错误
4. **设备数量**：系统最多支持 MAX_PWM_DEVICES 个 PWM 设备
5. **时钟配置**：函数使用默认的时钟配置，如果系统时钟配置不同，可能需要修改 calculate_timer_params 函数

## 常见问题

1. **PWM 无输出**：
   
   - 检查定时器是否已正确初始化
   - 确认 GPIO 引脚已配置为复用推挽输出模式
   - 检查是否调用了 BSP_PWM_Start 启动 PWM 输出

2. **频率不准确**：
   
   - 检查系统时钟配置是否与代码中的假设一致
   - 确认定时器时钟源配置正确
   - 频率过高可能导致精度下降

3. **占空比不正确**：
   
   - 确认占空比参数在 0-100 范围内
   - 检查是否在设置占空比后正确调用了相关函数

4. **设备初始化失败**：
   
   - 检查是否已达到最大设备数量限制
   - 确认传入的参数是否有效
   - 检查定时器初始化是否成功