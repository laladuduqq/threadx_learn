<!--
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-06 10:31:16
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-06 10:46:15
 * @FilePath: /threadx_learn/modules/BEEP/beep.md
 * @Description: 蜂鸣器模块文档
-->

# BEEP 蜂鸣器模块

## 简介

BEEP 蜂鸣器模块是一个基于 BSP_PWM 组件实现的蜂鸣器控制模块。该模块提供了对蜂鸣器的初始化、音调控制、响声次数控制等功能，可以方便地实现不同频率和响声次数的蜂鸣器提示音。

## 特性

1. **基于PWM控制**：使用 BSP_PWM 组件控制蜂鸣器，避免直接操作寄存器
2. **音调控制**：支持设置蜂鸣器的频率和占空比，控制音调和音量
3. **响声次数控制**：支持设置蜂鸣器响声次数，并自动控制响声间隔
4. **易于集成**：提供简单的API接口，易于集成到现有项目中

## 配置参数

```c
#define BEEP_PERIOD   2000  // 蜂鸣器控制周期，单位ms
#define BEEP_ON_TIME  100   // 单次蜂鸣持续时间，单位ms
#define BEEP_OFF_TIME 100   // 单次蜂鸣间隔时间，单位ms

#define BEEP_TUNE_VALUE 500  // 蜂鸣器音调值，影响频率
#define BEEP_CTRL_VALUE 100  // 蜂鸣器控制值，影响占空比(音量)
```

## 使用示例

```c
#include "beep.h"

int main(void)
{
    // 系统初始化
    SystemInit();
    
    // 初始化蜂鸣器
    if (beep_init() != 0) {
        // 初始化失败处理
        return -1;
    }
    
    // 设置蜂鸣器响声3次
    beep_set_times(3);
    
    while (1) {
        // 周期性调用控制函数
        beep_ctrl_times();
        
        // 其他任务处理
        HAL_Delay(10);
    }
}
```

## 工作原理

1. **初始化**：beep_init 函数会初始化 TIM4 的通道3为PWM输出模式
2. **响声控制**：通过 beep_set_times 设置响声次数，beep_ctrl_times 周期性检查并控制响声
3. **音调控制**：beep_set_tune 函数通过设置PWM的频率和占空比控制蜂鸣器音调和音量
4. **时序控制**：每次响声持续 BEEP_ON_TIME 时间，然后静音 BEEP_OFF_TIME 时间

## 注意事项

1. **定时调用**：beep_ctrl_times 需要周期性调用，建议每10ms调用一次
2. **参数配置**：BEEP_PERIOD、BEEP_ON_TIME、BEEP_OFF_TIME 需要合理配置，确保响声效果