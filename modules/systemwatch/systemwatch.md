<!--
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-06 09:14:08
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-06 10:23:01
 * @FilePath: /threadx_learn/modules/systemwatch/systemwatch.md
 * @Description: 
-->

# SystemWatch 系统监控模块

## 简介

SystemWatch 是一个基于 ThreadX RTOS 的系统监控模块，用于监控系统中各个任务的运行状态，检测任务是否阻塞，并集成看门狗功能以提高系统的可靠性。

## 功能特性

1. **任务运行状态监控**：实时监控注册任务的运行状态
2. **任务阻塞检测**：检测任务是否在指定时间内没有响应
3. **看门狗集成**：集成硬件看门狗，防止系统死机
4. **任务堆栈信息**：提供任务堆栈使用情况的详细信息
5. **系统状态报告**：在检测到异常时输出详细的系统状态信息

## 配置选项

在 [systemwatch.h](file:///home/pan/code/threadx_learn/modules/systemwatch/systemwatch.h) 中可以通过以下宏定义进行配置：

- [MAX_MONITORED_TASKS](file:///home/pan/code/threadx_learn/modules/systemwatch/systemwatch.h#L19-L20)：最大监控任务数，默认为10
- [TASK_BLOCK_TIMEOUT](file:///home/pan/code/threadx_learn/modules/systemwatch/systemwatch.h#L20-L21)：任务阻塞判定时间（秒），默认为1秒
- [SystemWatch_Enable](file:///home/pan/code/threadx_learn/modules/systemwatch/systemwatch.h#L21-L22)：开启系统监控功能，默认开启
- [SystemWatch_Reset_Enable](file:///home/pan/code/threadx_learn/modules/systemwatch/systemwatch.h#L22-L23)：开启系统监控重置功能，默认关闭
- [SystemWatch_Iwdg_Enable](file:///home/pan/code/threadx_learn/modules/systemwatch/systemwatch.h#L23-L24)：开启看门狗功能，默认开启

## 使用示例

```c
//首先初始化systemwatch
SYSTEMWATCH_INIT(&pool);
//注册任务监控
static TX_THREAD TaskHandle;
SYSTEMWATCH_REGISTER_TASK(&TaskHandle, "Task");

//在任务中定期更新监控信息
SYSTEMWATCH_UPDATE_TASK(&TaskHandle);

```
## 注意事项
1. **看门狗功能**：
    - 这里将main.c的看门狗初始化去除，在systemwatch模块中进行看门狗初始化


