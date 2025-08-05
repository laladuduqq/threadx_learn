<!--
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-05 09:51:10
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-05 15:43:57
 * @FilePath: /threadx_learn/BSP/FLASH/flash.md
 * @Description: 
-->

# Flash 驱动文档

## 简介

BSP_FLASH 是一个基于 ThreadX RTOS 的 Flash 驱动模块，提供了对 STM32 Flash 外设的高级封装。该模块支持扇区擦除、数据读写等基本操作，并提供了线程安全的访问机制。同时，该模块还提供了一个简化的用户 Flash 接口，方便开发者快速使用预定义的 Flash 扇区进行数据存储。

## 特性

1. **线程安全**：使用 ThreadX 互斥锁保护 Flash 访问
2. **灵活的扇区操作**：支持任意扇区的擦除和读写
3. **简化用户接口**：提供预定义用户扇区的读写接口
4. **错误处理**：完善的错误码定义和错误处理机制
5. **日志支持**：集成 ulog 日志系统，方便调试和问题追踪

## 数据结构

### Flash_Status

Flash操作状态码枚举：

```c
typedef enum {
    FLASH_OK = 0,        // 操作成功
    FLASH_ERR_BUSY,      // Flash忙
    FLASH_ERR_PARAM,     // 参数错误
    FLASH_ERR_ERASE,     // 擦除错误
    FLASH_ERR_WRITE,     // 写入错误
    FLASH_ERR_ADDR       // 地址错误
} Flash_Status;
```

## 宏定义

### Flash扇区地址定义

```c
#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000)  /* 扇区0基地址，16 Kbytes */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000)  /* 扇区1基地址，16 Kbytes */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000)  /* 扇区2基地址，16 Kbytes */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000)  /* 扇区3基地址，16 Kbytes */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000)  /* 扇区4基地址，64 Kbytes */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000)  /* 扇区5基地址，128 Kbytes */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000)  /* 扇区6基地址，128 Kbytes */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000)  /* 扇区7基地址，128 Kbytes */
#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000)  /* 扇区8基地址，128 Kbytes */
#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000)  /* 扇区9基地址，128 Kbytes */
#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000)  /* 扇区10基地址，128 Kbytes */
#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000)  /* 扇区11基地址，128 Kbytes */
```

### 用户Flash扇区定义

```c
#define USER_FLASH_SECTOR_ADDR    ADDR_FLASH_SECTOR_11  // 用户Flash扇区地址
#define USER_FLASH_SECTOR_NUMBER  FLASH_SECTOR_11       // 用户Flash扇区编号
```

## 使用示例

### 基本Flash操作

```c
// 初始化Flash模块（系统启动时调用一次）
BSP_Flash_Init();

// 定义要写入的数据
uint8_t tmpdata[32] = "Hello Flash!";

// 擦除扇区11
Flash_Status status = BSP_Flash_Erase(FLASH_SECTOR_11, 1);
if (status != FLASH_OK) {
    printf("Flash擦除失败\n");
    return;
}

// 向扇区11写入数据
status = BSP_Flash_Write(ADDR_FLASH_SECTOR_11, tmpdata, sizeof(tmpdata));
if (status != FLASH_OK) {
    printf("Flash写入失败\n");
    return;
}

// 从扇区11读取数据
uint8_t read_buf[32] = {0};
status = BSP_Flash_Read(ADDR_FLASH_SECTOR_11, read_buf, sizeof(read_buf));
if (status != FLASH_OK) {
    printf("Flash读取失败\n");
    return;
}

printf("读取到的数据: %s\n", read_buf);
```

### 简化用户Flash操作

```c
// 初始化Flash模块（系统启动时调用一次）
BSP_Flash_Init();

// 定义要写入的数据
uint8_t userdata[32] = "User Data";

// 使用简化接口写入用户Flash扇区（自动擦除）
Flash_Status status = BSP_UserFlash_Write(userdata, sizeof(userdata));
if (status != FLASH_OK) {
    printf("用户Flash写入失败\n");
    return;
}

// 使用简化接口从用户Flash扇区读取数据
uint8_t user_read_buf[32] = {0};
status = BSP_UserFlash_Read(user_read_buf, sizeof(user_read_buf));
if (status != FLASH_OK) {
    printf("用户Flash读取失败\n");
    return;
}

printf("从用户Flash读取到的数据: %s\n", user_read_buf);
```

## 注意事项

1. **线程安全**：所有API都是线程安全的，可以在多个线程中同时调用
2. **数据对齐**：写入Flash的数据必须4字节对齐，数据大小必须是4的倍数
3. **扇区擦除**：写入数据前必须先擦除相应扇区
4. **初始化**：使用Flash功能前必须调用BSP_Flash_Init()进行初始化
5. **错误处理**：应始终检查函数返回值并进行适当处理
6. **用户扇区**：BSP_UserFlash_Write会自动擦除整个扇区，不适合频繁的小数据写入

## 常见问题

1. **FLASH_ERR_WRITE写入错误**：
   
   - 检查地址是否4字节对齐
   - 确认数据大小是否为4的倍数
   - 确认目标地址所在的扇区是否已擦除

2. **FLASH_ERR_ERASE擦除错误**：
   
   - 检查扇区编号是否正确
   - 确认Flash接口是否正常

3. **FLASH_ERR_PARAM参数错误**：
   
   - 检查传入的参数是否符合要求
   - 确认指针不为NULL，大小不为0

4. **FLASH_ERR_BUSY忙错误**：
   
   - 可能有其他线程正在访问Flash
   - 等待一段时间后重试
