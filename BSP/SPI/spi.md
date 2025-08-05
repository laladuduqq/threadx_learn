<!--
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-05 08:57:51
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-05 09:34:59
 * @FilePath: /threadx_learn/BSP/SPI/spi.md
 * @Description: 
-->

<!--
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-05 08:57:51
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-05 09:32:11
 * @FilePath: /threadx_learn/BSP/SPI/spi.md
 * @Description: 
-->

# SPI 驱动文档

## 简介

BSP_SPI 是一个基于 ThreadX RTOS 的 SPI 驱动模块，提供了对 STM32 SPI 外设的高级封装。该模块支持多设备管理、线程安全访问以及阻塞、中断和 DMA 三种传输模式，适用于各种 SPI 设备的通信需求。

## 特性

1. **多设备支持**：每条 SPI 总线支持最多 MAX_DEVICES_PER_BUS 个设备
2. **线程安全**：使用 ThreadX 互斥锁保护总线访问
3. **多种传输模式**：
   - 阻塞模式（Blocking）
   - 中断模式（Interrupt）
   - DMA 模式（Direct Memory Access）
4. **事件回调机制**：支持传输完成和错误事件回调
5. **自动片选控制**：自动控制设备的 CS 引脚

## 数据结构

### SPI_Mode

传输模式枚举：

```c
typedef enum {
    SPI_MODE_BLOCKING,  // 阻塞模式
    SPI_MODE_IT,        // 中断模式
    SPI_MODE_DMA        // DMA模式
} SPI_Mode;
```

### SPI_Event_Type

SPI事件类型枚举：

```c
typedef enum {
    SPI_EVENT_TX_COMPLETE,      // 发送完成
    SPI_EVENT_RX_COMPLETE,      // 接收完成
    SPI_EVENT_TX_RX_COMPLETE,   // 发送接收完成
    SPI_EVENT_ERROR             // 错误事件
} SPI_Event_Type;
```

### SPI_Device

SPI设备实例结构体：

```c
typedef struct {
    SPI_HandleTypeDef* hspi;         // SPI句柄
    GPIO_TypeDef* cs_port;          // 片选端口
    uint16_t cs_pin;                // 片选引脚
    SPI_Mode tx_mode;              // 发送模式
    SPI_Mode rx_mode;              // 接收模式
    uint32_t timeout;              // 超时时间
    void (*spi_callback)(SPI_Event_Type event);    // 回调函数
} SPI_Device;
```

### SPI_Device_Init_Config

SPI设备初始化配置结构体：

```c
typedef struct {
    SPI_HandleTypeDef* hspi;       // SPI句柄
    GPIO_TypeDef* cs_port;         // 片选端口
    uint16_t cs_pin;               // 片选引脚
    SPI_Mode tx_mode;              // 发送模式
    SPI_Mode rx_mode;              // 接收模式
    uint32_t timeout;              // 超时时间
    void (*spi_callback)(SPI_Event_Type event);    // 回调函数
} SPI_Device_Init_Config;
```

## 使用示例

```c
// 定义回调函数
void spi_callback_handler(SPI_Event_Type event) {
    switch(event) {
        case SPI_EVENT_TX_COMPLETE:
            // 处理发送完成事件
            printf("SPI发送完成\n");
            break;
        case SPI_EVENT_RX_COMPLETE:
            // 处理接收完成事件
            printf("SPI接收完成\n");
            break;
        case SPI_EVENT_TX_RX_COMPLETE:
            // 处理发送接收完成事件
            printf("SPI发送接收完成\n");
            break;
        case SPI_EVENT_ERROR:
            // 处理错误事件
            printf("SPI错误发生\n");
            break;
    }
}

// 配置SPI设备
SPI_Device_Init_Config spi_config = {
    .hspi = &hspi1,                // 使用SPI1
    .cs_port = GPIOB,              // CS引脚端口
    .cs_pin = GPIO_PIN_0,          // CS引脚号
    .tx_mode = SPI_MODE_DMA,       // 发送使用DMA模式
    .rx_mode = SPI_MODE_DMA,       // 接收使用DMA模式
    .timeout = 1000,               // 超时时间1000ms
    .spi_callback = spi_callback_handler  // 回调函数
};

// 初始化设备
SPI_Device* my_spi_device = BSP_SPI_Device_Init(&spi_config);
if (my_spi_device == NULL) {
    printf("SPI设备初始化失败\n");
}



//trans and receive
uint8_t tx_buffer[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
uint8_t rx_buffer[10];

// 发送并接收数据
HAL_StatusTypeDef status = BSP_SPI_TransReceive(my_spi_device, tx_buffer, rx_buffer, 10);
if (status == HAL_OK) {
    // 处理接收到的数据
    for (int i = 0; i < 10; i++) {
        printf("rx_buffer[%d] = %d\n", i, rx_buffer[i]);
    }
}

//单向发送
uint8_t tx_data[5] = {0x01, 0x02, 0x03, 0x04, 0x05};

// 发送数据
HAL_StatusTypeDef status = BSP_SPI_Transmit(my_spi_device, tx_data, 5);
if (status == HAL_OK) {
    printf("数据发送成功\n");
}

//单向接收
uint8_t rx_data[8];

// 接收数据
HAL_StatusTypeDef status = BSP_SPI_Receive(my_spi_device, rx_data, 8);
if (status == HAL_OK) {
    // 处理接收到的数据
    for (int i = 0; i < 8; i++) {
        printf("rx_data[%d] = %d\n", i, rx_data[i]);
    }
}


//连续发送
uint8_t command[2] = {0x80, 0x01};  // 读取命令
uint8_t data[4] = {0x00, 0x00, 0x00, 0x00};  // 用于接收数据的缓冲区

// 先发送命令，再发送读取数据的时钟脉冲
HAL_StatusTypeDef status = BSP_SPI_TransAndTrans(my_spi_device, command, 2, data, 4);
if (status == HAL_OK) {
    printf("连续发送完成\n");
}
```

## 注意事项

1. **线程安全**：所有API都是线程安全的，可以在多个线程中同时使用不同的SPI设备
2. **回调函数**：回调函数在中断上下文中执行，应避免在回调函数中进行耗时操作
3. **DMA模式**：使用DMA模式时，数据缓冲区应确保在传输完成前不被修改或释放
4. **超时设置**：阻塞模式下，超时时间应根据实际应用需求合理设置
5. **错误处理**：应及时处理SPI错误事件，避免影响系统稳定性

## 常见问题

1. **SPI设备初始化失败**：
   
   - 检查SPI总线是否已正确配置
   - 确认设备数量未超过每条总线的最大设备数
   - 检查GPIO配置是否正确

2. **传输失败**：
   
   - 检查SPI时钟配置是否正确
   - 确认CS引脚连接是否正确
   - 检查电源和地线连接

3. **回调函数未执行**：
   
   - 确认回调函数指针是否正确设置
   - 检查NVIC中断配置是否正确
   - 确认HAL库回调函数是否被正确调用
