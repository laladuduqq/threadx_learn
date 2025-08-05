<!--
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-05 21:34:24
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-05 21:34:31
 * @FilePath: /threadx_learn/BSP/GPIO/gpio.md
 * @Description: 
-->

# GPIO 中断回调管理文档

## 简介

BSP_GPIO 是一个简洁的 GPIO 中断回调管理模块，专门用于管理已在 CubeMX 中配置好的 GPIO 外部中断回调函数。该模块提供了一种简单的方式来注册和注销 GPIO 中断回调函数，无需关心底层的 GPIO 和 NVIC 配置。

## 特性

1. **简洁设计**：仅关注回调函数管理，不涉及底层GPIO和NVIC配置
2. **多设备支持**：支持最多16个GPIO引脚的中断回调管理
3. **线程安全**：模块内部状态管理安全
4. **易于使用**：提供简单的注册和注销接口
5. **日志支持**：集成ulog日志系统，便于调试

## 数据结构

### GPIO_EXTI_Event_Type

GPIO 中断事件类型枚举：

```c
typedef enum {
    GPIO_EXTI_EVENT_TRIGGERED,          // 中断触发事件
} GPIO_EXTI_Event_Type;
```

### gpio_exti_callback_t

GPIO 中断回调函数类型：

```c
typedef void (*gpio_exti_callback_t)(GPIO_EXTI_Event_Type event);
```

## 使用示例

### 基本使用

```c
// 定义中断回调函数
void my_gpio_callback(GPIO_EXTI_Event_Type event) {
    switch(event) {
        case GPIO_EXTI_EVENT_TRIGGERED:
            printf("GPIO中断触发\n");
            // 在这里添加中断处理逻辑
            break;
    }
}

// 系统初始化时调用一次
BSP_GPIO_EXTI_Module_Init();

// 注册GPIO中断回调函数（假设使用PA0引脚）
if (BSP_GPIO_EXTI_Register_Callback(GPIO_PIN_0, my_gpio_callback) == 0) {
    printf("GPIO中断回调注册成功\n");
} else {
    printf("GPIO中断回调注册失败\n");
}

// 在HAL_GPIO_EXTI_Callback中调用处理函数
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    BSP_GPIO_EXTI_Handle_Callback(GPIO_Pin);
}

// 如果需要注销回调函数
if (BSP_GPIO_EXTI_Unregister_Callback(GPIO_PIN_0) == 0) {
    printf("GPIO中断回调注销成功\n");
} else {
    printf("GPIO中断回调注销失败\n");
}
```

### 多个GPIO中断回调

```c
// 定义多个回调函数
void button_callback(GPIO_EXTI_Event_Type event) {
    printf("按键中断触发\n");
}

void sensor_callback(GPIO_EXTI_Event_Type event) {
    printf("传感器中断触发\n");
}

void encoder_callback(GPIO_EXTI_Event_Type event) {
    printf("编码器中断触发\n");
}

// 系统初始化
BSP_GPIO_EXTI_Module_Init();

// 注册多个GPIO中断回调
BSP_GPIO_EXTI_Register_Callback(GPIO_PIN_0, button_callback);   // PA0按键
BSP_GPIO_EXTI_Register_Callback(GPIO_PIN_1, sensor_callback);   // PA1传感器
BSP_GPIO_EXTI_Register_Callback(GPIO_PIN_2, encoder_callback);  // PA2编码器

// 在HAL回调中统一处理
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    BSP_GPIO_EXTI_Handle_Callback(GPIO_Pin);
}
```

### 条件注册回调

```c
// 根据不同条件注册不同的回调函数
void setup_gpio_callbacks(int mode) {
    BSP_GPIO_EXTI_Module_Init();
    
    if (mode == 1) {
        // 模式1：只注册按键回调
        BSP_GPIO_EXTI_Register_Callback(GPIO_PIN_0, button_callback);
    } else if (mode == 2) {
        // 模式2：注册所有回调
        BSP_GPIO_EXTI_Register_Callback(GPIO_PIN_0, button_callback);
        BSP_GPIO_EXTI_Register_Callback(GPIO_PIN_1, sensor_callback);
        BSP_GPIO_EXTI_Register_Callback(GPIO_PIN_2, encoder_callback);
    }
    
    // 注册错误处理回调
    BSP_GPIO_EXTI_Register_Callback(GPIO_PIN_3, error_callback);
}
```

## 注意事项

1. **CubeMX配置**：GPIO和中断必须在CubeMX中正确配置，该模块仅管理回调函数

2. **回调函数**：回调函数在中断上下文中执行，应避免在回调函数中进行耗时操作

3. **引脚限制**：每个引脚号(0-15)只能注册一个回调函数

4. **HAL回调集成**：需要在`HAL_GPIO_EXTI_Callback`中调用`BSP_GPIO_EXTI_Handle_Callback`

5. **初始化顺序**：需要先调用BSP_GPIO_EXTI_Module_Init初始化模块，再注册回调函数

## 常见问题

1. **回调函数未执行**：
   
   - 确认是否在`HAL_GPIO_EXTI_Callback`中调用了`BSP_GPIO_EXTI_Handle_Callback`
   - 检查回调函数是否正确注册
   - 确认CubeMX中GPIO和中断配置是否正确

2. **注册失败**：
   
   - 检查回调函数指针是否为NULL
   - 确认引脚参数是否正确(只能指定一个引脚)
   - 检查是否已达到最大设备数限制

3. **重复注册**：
   
   - 重复注册同一引脚会覆盖之前的回调函数，并输出警告日志

4. **CubeMX配置问题**：
   
   - 确保GPIO引脚配置为外部中断模式
   - 确认NVIC中断已使能
   - 检查触发模式(上升沿、下降沿或双边沿)是否正确配置

## 与HAL库的集成

在使用该模块时，需要在项目中的stm32f4xx_it.c文件中添加以下代码：

```c
#include "bsp_gpio.h"

// 在HAL库的GPIO中断回调函数中调用我们的处理函数
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    BSP_GPIO_EXTI_Handle_Callback(GPIO_Pin);
}
```

这样就可以将HAL库的中断回调与我们的回调管理模块集成起来。
