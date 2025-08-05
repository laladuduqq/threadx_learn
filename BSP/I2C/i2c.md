<!--
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-05 15:35:12
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-05 15:37:59
 * @FilePath: /threadx_learn/BSP/I2C/i2c.md
 * @Description: 
-->

# I2C 驱动文档

## 简介

BSP_I2C 是一个基于 ThreadX RTOS 的 I2C 驱动模块，提供了对 STM32 I2C 外设的高级封装。该模块支持多设备管理、线程安全访问以及阻塞、中断和 DMA 三种传输模式，适用于各种 I2C 设备的通信需求。

## 特性

1. **多设备支持**：每条 I2C 总线支持最多 MAX_DEVICES_PER_I2C_BUS 个设备
2. **线程安全**：使用 ThreadX 互斥锁保护总线访问
3. **多种传输模式**：
   - 阻塞模式（Blocking）
   - 中断模式（Interrupt）
   - DMA 模式（Direct Memory Access）
4. **事件回调机制**：支持传输完成和错误事件回调
5. **设备管理**：支持多个 I2C 设备的统一管理

## 数据结构

### I2C_Mode

传输模式枚举：

```c
typedef enum {
    I2C_MODE_BLOCKING,  // 阻塞模式
    I2C_MODE_IT,        // 中断模式
    I2C_MODE_DMA        // DMA模式
} I2C_Mode;
```

### I2C_Event_Type

I2C事件类型枚举：

```c
typedef enum {
    I2C_EVENT_TX_COMPLETE,      // 发送完成
    I2C_EVENT_RX_COMPLETE,      // 接收完成
    I2C_EVENT_TX_RX_COMPLETE,   // 发送接收完成
    I2C_EVENT_ERROR             // 错误事件
} I2C_Event_Type;
```

### I2C_Device

I2C设备实例结构体：

```c
typedef struct {
    I2C_HandleTypeDef* hi2c;         // I2C句柄
    uint16_t dev_address;            // 设备地址
    I2C_Mode tx_mode;               // 发送模式
    I2C_Mode rx_mode;               // 接收模式
    uint32_t timeout;               // 超时时间
    void (*i2c_callback)(I2C_Event_Type event);    // 回调函数
} I2C_Device;
```

### I2C_Device_Init_Config

I2C设备初始化配置结构体：

```c
typedef struct {
    I2C_HandleTypeDef* hi2c;       // I2C句柄
    uint16_t dev_address;          // 设备地址
    I2C_Mode tx_mode;              // 发送模式
    I2C_Mode rx_mode;              // 接收模式
    uint32_t timeout;              // 超时时间
    void (*i2c_callback)(I2C_Event_Type event);    // 回调函数
} I2C_Device_Init_Config;
```

## 使用示例

```c
// 定义回调函数
void i2c_callback_handler(I2C_Event_Type event) {
    switch(event) {
        case I2C_EVENT_TX_COMPLETE:
            // 处理发送完成事件
            printf("I2C发送完成\n");
            break;
        case I2C_EVENT_RX_COMPLETE:
            // 处理接收完成事件
            printf("I2C接收完成\n");
            break;
        case I2C_EVENT_TX_RX_COMPLETE:
            // 处理发送接收完成事件
            printf("I2C发送接收完成\n");
            break;
        case I2C_EVENT_ERROR:
            // 处理错误事件
            printf("I2C错误发生\n");
            break;
    }
}

// 配置I2C设备
I2C_Device_Init_Config i2c_config = {
    .hi2c = &hi2c3,                // 使用I2C3
    .dev_address = 0x48 << 1,      // 设备地址(7位地址左移1位)
    .tx_mode = I2C_MODE_DMA,       // 发送使用DMA模式
    .rx_mode = I2C_MODE_DMA,       // 接收使用DMA模式
    .timeout = 1000,               // 超时时间1000ms
    .i2c_callback = i2c_callback_handler  // 回调函数
};

// 初始化设备
I2C_Device* my_i2c_device = BSP_I2C_Device_Init(&i2c_config);
if (my_i2c_device == NULL) {
    printf("I2C设备初始化失败\n");
}

// 检查设备是否在线
if (BSP_I2C_IsDeviceReady(my_i2c_device) == HAL_OK) {
    printf("I2C设备在线\n");
} else {
    printf("I2C设备不在线\n");
}

// 发送数据
uint8_t tx_data[5] = {0x01, 0x02, 0x03, 0x04, 0x05};
HAL_StatusTypeDef status = BSP_I2C_Transmit(my_i2c_device, tx_data, 5);
if (status == HAL_OK) {
    printf("数据发送成功\n");
}

// 接收数据
uint8_t rx_data[8];
status = BSP_I2C_Receive(my_i2c_device, rx_data, 8);
if (status == HAL_OK) {
    // 处理接收到的数据
    for (int i = 0; i < 8; i++) {
        printf("rx_data[%d] = %d\n", i, rx_data[i]);
    }
}

// 发送并接收数据
uint8_t tx_buffer[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
uint8_t rx_buffer[10];
status = BSP_I2C_TransReceive(my_i2c_device, tx_buffer, rx_buffer, 10);
if (status == HAL_OK) {
    // 处理接收到的数据
    for (int i = 0; i < 10; i++) {
        printf("rx_buffer[%d] = %d\n", i, rx_buffer[i]);
    }
}

// 内存读写操作
uint8_t mem_data[4] = {0x11, 0x22, 0x33, 0x44};
// 写入内存
status = BSP_I2C_Mem_Write_Read(my_i2c_device, 0x10, I2C_MEMADD_SIZE_8BIT, mem_data, 4, 1);
if (status == HAL_OK) {
    printf("内存写入成功\n");
}

// 读取内存
memset(mem_data, 0, sizeof(mem_data));
status = BSP_I2C_Mem_Write_Read(my_i2c_device, 0x10, I2C_MEMADD_SIZE_8BIT, mem_data, 4, 0);
if (status == HAL_OK) {
    // 处理读取到的数据
    for (int i = 0; i < 4; i++) {
        printf("mem_data[%d] = 0x%02X\n", i, mem_data[i]);
    }
}
```

## 注意事项

1. **线程安全**：所有API都是线程安全的，可以在多个线程中同时使用不同的I2C设备
2. **回调函数**：回调函数在中断上下文中执行，应避免在回调函数中进行耗时操作
3. **DMA模式**：使用DMA模式时，数据缓冲区应确保在传输完成前不被修改或释放
4. **超时设置**：阻塞模式下，超时时间应根据实际应用需求合理设置
5. **设备地址**：I2C设备地址需要左移1位，以匹配STM32 HAL库的要求
6. **错误处理**：应及时处理I2C错误事件，避免影响系统稳定性

## 常见问题

1. **I2C设备初始化失败**：
   
   - 检查I2C总线是否已正确配置
   - 确认设备数量未超过每条总线的最大设备数
   - 检查设备地址是否正确

2. **传输失败**：
   
   - 检查I2C时钟配置是否正确
   - 确认上拉电阻是否连接正确
   - 检查电源和地线连接

3. **回调函数未执行**：
   
   - 确认回调函数指针是否正确设置
   - 检查NVIC中断配置是否正确
   - 确认HAL库回调函数是否被正确调用