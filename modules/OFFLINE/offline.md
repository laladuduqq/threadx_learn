<!--
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-06 10:51:09
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-06 11:24:33
 * @FilePath: /threadx_learn/modules/OFFLINE/offline.md
 * @Description: 
-->

# OFFLINE 离线检测模块

## 简介

OFFLINE 离线检测模块是一个基于 ThreadX RTOS 的设备离线检测系统，用于监控各种设备的在线状态。当设备超过设定时间未更新状态时，模块会将其标记为离线，并通过蜂鸣器和RGB灯提供视觉和听觉报警。

## 功能特性

1. **多设备支持**：支持最多 MAX_OFFLINE_DEVICES 个设备的离线检测
2. **优先级管理**：支持低、中、高三种优先级设置
3. **报警机制**：结合蜂鸣器和RGB灯提供报警提示
4. **灵活配置**：每个设备可独立配置超时时间、报警次数等参数
5. **状态查询**：支持查询单个设备或系统整体状态

## 配置选项

在 [offline.h](file:///home/pan/code/threadx_learn/modules/OFFLINE/offline.h) 中可以通过以下宏定义进行配置：

- [OFFLINE_Enable](file:///home/pan/code/threadx_learn/modules/OFFLINE/offline.h#L18-L18)：开启离线检测功能，默认开启
- [OFFLINE_THREAD_STACK_SIZE](file:///home/pan/code/threadx_learn/modules/OFFLINE/offline.h#L20-L20)：离线检测线程栈大小，默认1024字节
- [OFFLINE_THREAD_PRIORITY](file:///home/pan/code/threadx_learn/modules/OFFLINE/offline.h#L21-L21)：离线检测线程优先级，默认3
- [OFFLINE_Beep_Enable](file:///home/pan/code/threadx_learn/modules/OFFLINE/offline.h#L22-L22)：开启离线蜂鸣器功能，默认开启
- [MAX_OFFLINE_DEVICES](file:///home/pan/code/threadx_learn/modules/OFFLINE/offline.h#L23-L23)：最大离线设备数量，默认12个

## 数据结构

### OfflineLevel_e

设备离线优先级枚举：

```c
typedef enum {
    OFFLINE_LEVEL_LOW = 1,    // 低优先级
    OFFLINE_LEVEL_MEDIUM = 2, // 中优先级
    OFFLINE_LEVEL_HIGH = 3    // 高优先级
} OfflineLevel_e;
```

### OfflineDeviceInit_t

设备初始化配置结构体：

```c
typedef struct {
    const char* name;         // 设备名称
    uint32_t timeout_ms;      // 超时时间(毫秒)
    OfflineLevel_e level;     // 离线等级
    uint8_t beep_times;       // 蜂鸣次数
    uint8_t enable;           // 是否启用检测(1-启用, 0-禁用)
} OfflineDeviceInit_t;
```

## 使用示例

```c
// 初始化离线检测模块
OFFLINE_INIT(&tx_app_byte_pool);

// 注册设备
static uint8_t device_index;
OfflineDeviceInit_t device_init = {
    .name = "Sensor1",
    .timeout_ms = 1000,              // 1秒超时
    .level = OFFLINE_LEVEL_HIGH,     // 高优先级
    .beep_times = 3,                 // 报警3次
    .enable = OFFLINE_ENABLE         // 启用检测
};
device_index = OFFLINE_DEVICE_REGISTER(&device_init);

// 在设备通信正常时定期更新设备状态
OFFLINE_DEVICE_UPDATE(device_index);

// 查询设备状态
if (GET_DEVICE_STATUS(device_index) == STATE_OFFLINE) {
    // 处理设备离线情况
}

// 查询系统整体状态
uint8_t system_status = GET_SYSTEM_STATUS();
if (system_status != 0) {
    // 有设备离线
}
```

## 工作原理

1. **设备注册**：通过OFFLINE_DEVICE_REGISTER注册需要监控的设备
2. **状态更新**：设备正常工作时，定期调用OFFLINE_DEVICE_UPDATE更新时间戳
3. **状态检测**：离线检测任务周期性检查各设备时间戳，超时未更新则标记为离线
4. **报警处理**：根据设备优先级和配置触发蜂鸣器和RGB灯报警
5. **状态查询**：通过API接口查询设备或系统状态

## 注意事项

1. **定期更新**：必须定期调用OFFLINE_DEVICE_UPDATE以保持设备在线状态
2. **优先级机制**：高优先级设备离线时会优先报警
3. **报警同步**：蜂鸣器和RGB灯同步报警，提供双重提示
4. **资源管理**：合理设置MAX_OFFLINE_DEVICES以控制内存使用
