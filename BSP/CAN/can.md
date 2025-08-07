<!--
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-05 16:04:23
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-07 15:01:52
 * @FilePath: /threadx_learn/BSP/CAN/can.md
 * @Description: 
-->

# can

## bsp_can介绍

整体通过总线管理，设备注册的方式来实现不同can设备的管理和调度。

### 性能特点

1. **线程安全**：使用TX_MUTEX互斥锁保护发送操作，确保多线程环境下的安全访问
2. **灵活配置**：支持阻塞和中断两种接收模式

### can_bus介绍

这是对应的canbusmanager，实现对每一路can总线的管理

```c
/* CAN总线管理结构 */
typedef struct {
    CAN_HandleTypeDef *hcan;
    Can_Device devices[MAX_DEVICES_PER_BUS];
    TX_MUTEX tx_mutex;
    uint8_t device_count;
} CANBusManager;
```

```c
#define CAN_BUS_NUM 2                   // 总线数量
#define MAX_CAN_DEVICES_PER_BUS  8      // 每总线最大设备数
#define CAN_SEND_RETRY_CNT  3           // 发送重试次数
#define CAN_SEND_TIMEOUT_US 100         // 发送超时时间(微秒)
```

上述宏定义用来确定canbusmanager的总线个数，以及每个总线上可挂载的最大设备数，按照需要修改。

如果需要添加新的总线（以三路can为例）： 首先宏定义修改为3,其次

```c
/* CAN总线硬件配置数组 */
static const struct {
    CAN_HandleTypeDef *hcan;
    const char *name;
} can_configs[CAN_BUS_NUM] = {
    {&hcan1, "CAN1"},
    {&hcan2, "CAN2"},
    {&hcan3, "CAN3"} //这里添加新的can对应的句柄和名字
};

/* 总线管理器实例 */
static CANBusManager can_bus[CAN_BUS_NUM] = {
    {.hcan = NULL,.devices = {{0}},.tx_mutex = {0},.device_count = 0},
    {.hcan = NULL,.devices = {{0}},.tx_mutex = {0},.device_count = 0},
    {.hcan = NULL,.devices = {{0}},.tx_mutex = {0},.device_count = 0} //这里copy一个新的即可
};
```

### can设备

```c
/* CAN设备实例结构体 */
typedef struct
{
    CAN_HandleTypeDef *can_handle;  // CAN句柄
    CAN_TxHeaderTypeDef txconf;     // 发送配置
    uint32_t tx_id;                 // 发送ID
    uint32_t tx_mailbox;            // 发送邮箱号
    uint8_t tx_buff[8];             // 发送缓冲区

    uint32_t rx_id;                 // 接收ID
    uint8_t rx_buff[8];             // 接收缓冲区
    uint8_t rx_len;                 // 接收长度

    CAN_Mode tx_mode;
    CAN_Mode rx_mode;

    void (*can_callback)(const CAN_HandleTypeDef* hcan, const uint32_t rx_id); // 接收回调
} Can_Device;
```

初始化can设备的结构体

```c
/* 初始化配置结构体 */
typedef struct
{
    CAN_HandleTypeDef *can_handle;
    uint32_t tx_id;
    uint32_t rx_id;
    CAN_Mode tx_mode;
    CAN_Mode rx_mode;
    void (*can_callback)(const CAN_HandleTypeDef* hcan, const uint32_t rx_id);
} Can_Device_Init_Config_s;
```

### 使用示例

```c
// CAN 设备初始化配置
Can_Device_Init_Config_s can_config = {
        .can_handle = &hcan2,
        .tx_id = 0x01,
        .rx_id = 0x11,
        .tx_mode = CAN_MODE_BLOCKING,
        .rx_mode = CAN_MODE_IT,
        .can_callback = user_can_callback // 用户编写的回调函数，严禁有任何延时处理
};

// 注册 CAN 设备并获取引用
Can_Device *can_dev = BSP_CAN_Device_Init(&can_config);
if (can_dev == NULL) {
    // 处理初始化失败
    return;
}

// 准备发送数据
uint8_t send_data[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
memcpy(can_dev->tx_buff, send_data, sizeof(send_data));

// 发送数据
if (CAN_SendMessage(can_dev, sizeof(send_data)) != HAL_OK) {
    // 处理发送失败
}

// 在回调函数中处理接收到的数据
void user_can_callback(const CAN_HandleTypeDef* hcan, const uint32_t rx_id) {
    // 注意：此函数在中断上下文中执行，严禁有任何延时操作
    // 可以在这里处理接收到的数据
    // 例如设置标志位、发送事件等
}
```

### 注意事项

1. **回调函数**：回调函数在中断上下文中执行，严禁有任何延时处理（如HAL_Delay、tx_thread_sleep等）
2. **线程安全**：发送函数具有互斥锁保护，可以在多线程环境中安全使用
3. **ID冲突检测**：系统会自动检测接收ID冲突，避免设备间干扰
4. **数据长度**：CAN数据长度限制为1-8字节
5. **设备数量**：每条CAN总线最多支持MAX_CAN_DEVICES_PER_BUS个设备
6. **FIFO分配**：根据发送ID的奇偶性自动分配到不同的接收FIFO
