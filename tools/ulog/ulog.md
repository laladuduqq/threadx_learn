# uLog 日志系统使用文档

## 简介

uLog 是一个轻量级的日志系统，专为嵌入式系统设计。它具有以下特点：

- 轻量级，占用资源少
- 支持多种日志级别
- 支持同步和异步日志输出
- 支持彩色日志输出
- 支持带标签(Tag)的日志输出
- 基于ThreadX RTOS实现线程安全
- 可配置性强

## 日志级别

uLog 支持以下日志级别，按严重程度递增排列：

1. `ULOG_TRACE_LEVEL` - 跟踪信息，最详细的日志
2. `ULOG_DEBUG_LEVEL` - 调试信息，用于调试程序
3. `ULOG_INFO_LEVEL` - 普通信息，程序运行的一般信息
4. `ULOG_WARNING_LEVEL` - 警告信息，程序可以继续运行但需要注意
5. `ULOG_ERROR_LEVEL` - 错误信息，发生了错误但程序可能还能继续
6. `ULOG_CRITICAL_LEVEL` - 严重错误，程序可能无法继续运行
7. `ULOG_ALWAYS_LEVEL` - 总是输出，不受日志级别限制

## 快速开始

### 1. 启用日志系统

在使用日志系统前，需要先初始化：

```c
#include "ulog_port.h"
#include "ulog.h"

int main(void) {
    // 初始化日志系统
    ulog_port_init();
    
    // 在你的应用程序中使用日志
    ULOG_INFO("系统启动完成");    
}
```

### 2.使用不同级别的日志

```c
// 跟踪日志
ULOG_TRACE("这是跟踪信息: value = %d", some_value);

// 调试日志
ULOG_DEBUG("调试信息: pointer = 0x%X", some_pointer);

// 普通信息日志
ULOG_INFO("系统状态: temperature = %d°C", temperature);

// 警告日志
ULOG_WARNING("警告: battery level low (%d%%)", battery_level);

// 错误日志
ULOG_ERROR("错误: failed to read sensor, error code = %d", error_code);

// 严重错误日志
ULOG_CRITICAL("严重错误: system overheating, temperature = %d°C", temperature);

// 总是输出的日志
ULOG_ALWAYS("重要信息: system shutdown");
```

### 3.带标签的日志

```c
#define LOG_TAG "WiFi" //在顶部定义tag
#include "ulog_port.h"
#include "ulog.h"

void wifi_init(void) {
    // 这些日志会自动带有 [WiFi] 标签
    ULOG_TAG_INFO("开始初始化WiFi模块");
    ULOG_TAG_DEBUG("WiFi芯片ID: 0x%X", chip_id);
    ULOG_TAG_ERROR("WiFi初始化失败");
}
```

## 配置选项

### 基本配置

```c
// 启用日志系统 (1=启用, 0=禁用)
#define LOG_ENABLE 1

// 设置日志输出级别，低于此级别的日志不会输出
#define LOG_LEVEL_INFO ULOG_DEBUG_LEVEL

// 设置使用的UART设备
#define LOG_SETTING_UART huart6

// 启用彩色日志输出 (1=启用, 0=禁用)
#define LOG_COLOR_ENABLE 1

// 启用异步日志输出 (1=启用, 0=禁用)
#define LOG_ASYNC_ENABLE 0

// 日志缓冲区大小
#define LOG_BUFFER_SIZE 512
```

> ### 配置说明
> 
> 1. **LOG_ENABLE**: 控制是否启用日志系统
>    
>    - 设置为1时启用日志
>    - 设置为0时完全禁用日志，不产生任何代码
> 
> 2. **LOG_LEVEL_INFO**: 设置日志输出级别
>    
>    - 只有等于或高于此级别的日志才会被输出
>    - 可选值: `ULOG_TRACE_LEVEL`, `ULOG_DEBUG_LEVEL`, `ULOG_INFO_LEVEL`, `ULOG_WARNING_LEVEL`, `ULOG_ERROR_LEVEL`, `ULOG_CRITICAL_LEVEL`
> 
> 3. **LOG_SETTING_UART**: 设置日志输出的UART设备
>    
>    - 默认使用huart6，可根据硬件配置修改
> 
> 4. **LOG_COLOR_ENABLE**: 控制是否启用彩色日志
>    
>    - 设置为1时启用ANSI颜色代码
>    - 设置为0时输出无颜色日志
> 
> 5. **LOG_ASYNC_ENABLE**: 控制日志输出模式
>    
>    - 设置为1时启用异步日志（使用队列和线程）
>    - 设置为0时使用同步日志（直接输出）
> 
> 6. **LOG_BUFFER_SIZE**: 设置日志缓冲区大小
>    
>    - 异步模式下每个日志消息的最大长度
>    - 同步模式下此设置不影响静态缓冲区大小

### 使用模式

1. 同步模式（LOG_ASYNC_ENABKE = 0）

在同步模式下，日志函数会阻塞直到日志完全输出。这种模式简单可靠，但可能影响实时性。

特点：

- 实现简单
- 日志按顺序输出
- 可能阻塞调用线程
- 适用于对实时性要求不高的场景
2. 异步模式（LOG_ASYNC_ENABKE = 1）

在异步模式下，日志函数将日志消息放入队列后立即返回，由专门的日志线程负责输出。

特点：

- 不阻塞调用线程
- 高效的并发处理
- 需要额外的内存和线程资源
- 适用于对实时性要求较高的场景

## 注意事项

## 注意事项

1. **浮点数输出**: 默认情况下，嵌入式系统可能不支持浮点数格式化。如需输出浮点数，请确保编译器配置支持浮点格式化。

2. **缓冲区大小**: 确保 LOG_BUFFER_SIZE 足够大以容纳最长的日志消息，避免截断。

3. **异步模式资源**: 异步模式需要额外的内存和线程资源，请根据系统资源合理选择。

4. **线程安全**: 日志系统已实现线程安全，可在多线程环境中安全使用。

5. **性能影响**: 频繁的日志输出可能影响系统性能，建议在生产环境中适当降低日志级别。

## 故障排除

### 无法输出日志

1. 检查 LOG_ENABLE 是否设置为1
2. 检查日志级别设置是否正确
3. 确认UART设备初始化正常
4. 检查硬件连接是否正确

### 日志输出混乱

1. 同步模式下确保使用阻塞传输
2. 异步模式下检查队列和缓冲区配置
3. 确认没有缓冲区溢出