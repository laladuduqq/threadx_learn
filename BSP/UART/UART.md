<!--
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-01 17:55:10
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-04 10:52:39
 * @FilePath: /threadx_learn/BSP/UART/UART.md
 * @Description: 
-->


# uart

## 原理与整体流程简介

本驱动实现了多实例、双缓冲、支持阻塞/中断/ DMA 三种收发模式的 UART 通用驱动，适配 RTOS 线程环境，兼容回调和事件通知两种数据处理方式。

### 设计思想

- **多实例管理**：通过结构体数组统一管理多个 UART 设备，每个实例独立。
- **双缓冲机制**：每个 UART 实例拥有两个缓冲区，接收时交替切换，避免数据覆盖。
- **多种收发模式**：支持阻塞（Blocking）、中断（IT）、DMA 三种方式，灵活适配不同场景。
- **回调与事件通知**：接收完成后可选择直接回调（适合实时性高的场合）或事件通知（适合线程异步处理）。
- **RTOS 友好**：利用信号量和事件标志组实现线程安全的收发和通知。

### 典型数据流转流程

1. **初始化**：
    - 用户配置 `UART_Device_init_config`，调用 `UART_Init` 注册 UART 设备。
    - 分配缓冲区、初始化信号量和事件标志组，启动首次接收。
2. **接收数据**：
    - 硬件接收完成后进入 HAL 回调（如 `HAL_UARTEx_RxEventCallback`）。
    - 驱动切换缓冲区，保存接收长度。
    - 根据配置，触发回调函数或设置事件标志。
    - 自动重启下一次接收。
3. **数据处理**：
    - 直接回调：用户在回调函数内处理数据（注意不能阻塞）。
    - 事件通知：线程等待事件标志，收到后用 `UART_Read` 读取数据。
4. **发送数据**：
    - 用户调用 `UART_Send`，根据配置选择阻塞/IT/DMA 方式。
    - IT/DMA 发送完成后自动释放信号量，允许下次发送。
5. **反初始化**：
    - 调用 `UART_Deinit` 释放资源。
---

> 如何新添加串口实例
>
> 修改 `#define UART_MAX_INSTANCE_NUM 3` 即可，支持的最大实例数由此宏定义决定。

---

## 1. 结构体与配置说明

### UART_Device_init_config 结构体

```c
typedef struct {
    UART_HandleTypeDef *huart;           // 硬件句柄
    uint8_t (*rx_buf)[2];                // 外部缓冲区指针（可选）
    uint16_t rx_buf_size;                // 缓冲区大小
    uint16_t expected_len;               // 期望接收长度（0为不定长）
    UART_Mode rx_mode;                   // 接收模式（BLOCKING/IT/DMA）
    UART_Mode tx_mode;                   // 发送模式
    uint32_t timeout;                    // 超时时间（部分模式可用）
    void (*rx_complete_cb)(uint8_t*, uint16_t); // 接收完成回调
    UART_CallbackType cb_type;           // 回调类型（直接/事件）
    uint32_t event_flag;                 // 事件标志位
} UART_Device_init_config;
```

### UART_Device 结构体

```c
typedef struct {
    UART_HandleTypeDef *huart;
    uint8_t (*rx_buf)[2];                // 双缓冲区指针
    uint16_t rx_buf_size;                // 缓冲区大小
    volatile uint8_t rx_active_buf;      // 当前活动缓冲区
    uint16_t rx_len;                     // 实际接收长度
    uint16_t expected_len;               // 期望长度
    TX_SEMAPHORE tx_sem;                 // 发送信号量
    void (*rx_complete_cb)(uint8_t*, uint16_t); // 回调
    TX_EVENT_FLAGS_GROUP rx_event;       // 事件标志组
    ULONG event_flag;                    // 事件标志
    UART_CallbackType cb_type;           // 回调类型
    UART_Mode rx_mode;                   // 接收模式
    UART_Mode tx_mode;                   // 发送模式
} UART_Device;
```

---

## 2. 典型用法

### 直接回调函数版本

适用于实时性要求高、数据包长度固定的场景。

```c
static uint8_t sbus_buf[2][30];
UART_Device_init_config uart3_cfg = {
    .huart = &huart3,
    .expected_len = 25,  
    .rx_buf = (uint8_t (*)[2])sbus_buf,
    .rx_buf_size = 30,
    .rx_mode = UART_MODE_DMA,
    .tx_mode = UART_MODE_BLOCKING,
    .rx_complete_cb = uart3_rx_callback, // 回调函数，自己声明，严禁延时
    .cb_type = UART_CALLBACK_DIRECT,
    .event_flag = 0x01,
};
UART_Device *remote = UART_Init(&uart3_cfg); // 注意缓冲区要自己定义，设备会优先调用外部接收缓冲区

void uart3_rx_callback(uint8_t *buf, uint16_t len) {
    // 处理接收到的数据
    // buf 是指向非活动缓冲区的指针，len 是实际接收长度
    // 注意：此处严禁使用延时或阻塞操作
}
```

### 事件通知线程处理版本

适用于数据长度不定、需要线程异步处理的场景。

```c
static uint8_t referee_buf[2][1024];
UART_Device_init_config uart6_cfg = {
    .huart = &huart6,
    .expected_len = 0,       // 不定长
    .rx_buf_size = 1024,
    .rx_buf = (uint8_t (*)[2])referee_buf,
    .rx_mode = UART_MODE_DMA,
    .tx_mode = UART_MODE_DMA,
    .timeout = 1000,
    .rx_complete_cb = NULL,
    .cb_type = UART_CALLBACK_EVENT, // 事件通知
    .event_flag = UART_RX_DONE_EVENT
};
UART_Device* uart6 = UART_Init(&uart6_cfg);

//在线程中等待事件
ULONG actual_flags = 0;
// 等待事件标志，自动清除
UINT status = tx_event_flags_get(
    &referee_info.uart_device->rx_event, // 事件组
    UART_RX_DONE_EVENT,
    TX_OR_CLEAR,      // 自动清除
    &actual_flags,
    TX_WAIT_FOREVER   // 无限等待
);
if ((status == TX_SUCCESS) && (actual_flags & UART_RX_DONE_EVENT)) {
    uint8_t *data = NULL;
    int len = UART_Read(referee_info.uart_device, &data);
    if (len > 0) {
        // 直接处理 data 指向的数据，长度为 len
    }
}

```

---

## 3. API 说明


### 初始化

```c
UART_Device* UART_Init(UART_Device_init_config *config);
```
> 初始化 UART 设备，配置缓冲区、回调、事件等。返回设备指针，失败返回 NULL。

**使用示例：**

```c
static uint8_t my_buf[2][64];
UART_Device_init_config my_uart_cfg = {
    .huart = &huart1,
    .expected_len = 32,
    .rx_buf = (uint8_t (*)[2])my_buf,
    .rx_buf_size = 64,
    .rx_mode = UART_MODE_DMA,
    .tx_mode = UART_MODE_BLOCKING,
    .rx_complete_cb = my_rx_callback,
    .cb_type = UART_CALLBACK_DIRECT,
    .event_flag = 0x01,
};
UART_Device *my_uart = UART_Init(&my_uart_cfg);
if (my_uart == NULL) {
    // 初始化失败，处理错误
}
```


### 发送数据

```c
HAL_StatusTypeDef UART_Send(UART_Device *inst, uint8_t *data, uint16_t len);
```
> 支持阻塞、IT、DMA 三种发送模式。阻塞模式下会等待发送完成，IT/DMA 模式下发送完成后自动释放信号量。

**使用示例：**

```c
uint8_t tx_data[] = "hello uart";
if (UART_Send(my_uart, tx_data, sizeof(tx_data)) == HAL_OK) {
    // 发送成功
} else {
    // 发送失败或忙，需重试
}
```

### 读取数据（零拷贝）

```c
int UART_Read(UART_Device *inst, uint8_t **data);
```
> 获取最近一次接收完成的数据指针和长度，避免 memcpy，效率更高。

**使用示例：**

```c
uint8_t *data = NULL;
int len = UART_Read(my_uart, &data);
if (len > 0) {
    // data 指向非活动缓冲区，长度为 len
    // 直接处理 data 指向的数据即可
}
```


### 反初始化

```c
void UART_Deinit(UART_Device *inst);
```
> 释放资源，删除事件标志组和信号量。

**使用示例：**

```c
UART_Deinit(my_uart);
my_uart = NULL;
```

---

## 4. 注意事项与扩展说明

- 支持双缓冲机制，避免数据覆盖。
- 支持外部自定义缓冲区，也可使用默认缓冲区（不传 rx_buf 时自动分配）。
- 事件通知适合线程环境，直接回调适合中断快速处理。
- 不同串口实例独立管理，互不影响。
- 若 expected_len=0 且为 DMA 模式，接收长度由 DMA 剩余计数自动计算。
- 发送为 IT/DMA 模式时，需等待信号量释放后方可再次发送。
- 回调函数内严禁阻塞或延时。

---

如需扩展串口数量，修改 `UART_MAX_INSTANCE_NUM` 并确保缓冲区和硬件资源充足。
