# Board Communication Module (board_com)

## 概述

Board Communication 模块是一个用于在多个电路板之间进行通信的模块。它基于 CAN 总线实现，支持在云台板和底盘板之间传输数据。该模块具有以下特性：

1. 支持 CAN 总线通信
2. 集成掉线检测机制
3. 根据不同板卡类型（云台/底盘）处理不同类型的数据
4. 支持单板模式（调试用）
5. 数据压缩和解压缩功能

## 宏定义

模块提供了宏定义以便在不同模式下使用：

### BOARD_COM_INIT

- 单板模式下返回 NULL
- 多板模式下调用 [board_com_init](file:///home/pan/code/threadx_learn/modules/board_com/board_com.h#L66-L66) 函数

### BOARD_COM_SEND

- 单板模式下为空操作
- 多板模式下调用 [board_com_send](file:///home/pan/code/threadx_learn/modules/board_com/board_com.h#L73-L73) 函数

### BOARD_COM_GET_DATA

- 单板模式下返回 NULL
- 多板模式下调用 [board_com_get_data](file:///home/pan/code/threadx_learn/modules/board_com/board_com.h#L78-L78) 函数

## 数据结构

### board_com_t

板间通信主结构体，包含：

- [candevice](file:///home/pan/code/threadx_learn/modules/board_com/board_com.h#L46-L46): CAN 设备指针
- [offlinemanage_index](file:///home/pan/code/threadx_learn/modules/board_com/board_com.h#L47-L47): 掉线管理索引
- 根据板卡类型包含不同数据结构：
  - 云台板: `Chassis_referee_Upload_Data_s` (裁判系统数据)
  - 底盘板: `Chassis_Ctrl_Cmd_s` (底盘控制命令)

### board_com_init_t

板间通信初始化配置结构体，包含：

- [Can_Device_Init_Config](file:///home/pan/code/threadx_learn/modules/board_com/board_com.h#L57-L57): CAN 设备初始化配置
- [offline_manage_init](file:///home/pan/code/threadx_learn/modules/board_com/board_com.h#L58-L58): 掉线管理初始化配置

## 数据压缩方案

### 底盘板发送数据格式 (Chassis_referee_Upload_Data_s)

发送到云台板的裁判系统数据被压缩为 8 字节：

- Robot_Color: 1 bit (0或1)
- power_management_shooter_output: 1 bit (0或1)
- projectile_allowance_17mm: 10 bits (0-1000)
- current_hp_percent: 9 bits (0-400)
- outpost_HP: 11 bits (0-1500)
- base_HP: 13 bits (0-5000)
- game_progess: 3 bits (0-5)

### 云台板发送数据格式 (Chassis_Ctrl_Cmd_s)

发送到底盘板的控制命令被压缩为 8 字节：

- vx: 8 bits (-10.0 到 10.0 m/s，精度 0.1)
- vy: 8 bits (-10.0 到 10.0 m/s，精度 0.1)
- wz: 8 bits (-10.0 到 10.0 rad/s，精度 0.1)
- offset_angle: 16 bits (-720.0 到 720.0 度，精度 0.1)
- chassis_mode: 4 bits (底盘模式)

## 使用示例

```c
// 初始化配置
board_com_init_t board_com_config = {
    .offline_manage_init = {
        .name = "board_com",
        .timeout_ms = 100,
        .level = OFFLINE_LEVEL_HIGH,
        .beep_times = 8,
        .enable = OFFLINE_ENABLE,
    },
    .Can_Device_Init_Config = {
        .can_handle = &hcan2,
        .tx_id = GIMBAL_ID,
        .rx_id = CHASSIS_ID,
    }
};

// 初始化板间通信
board_com_t *board_com = BOARD_COM_INIT(&board_com_config);

// 发送数据
Chassis_Ctrl_Cmd_s ctrl_cmd = {0};
ctrl_cmd.vx = 1.0f;
ctrl_cmd.vy = 0.0f;
ctrl_cmd.wz = 0.5f;
BOARD_COM_SEND((uint8_t*)&ctrl_cmd);

// 接收数据
Chassis_referee_Upload_Data_s *referee_data = 
    (Chassis_referee_Upload_Data_s*)BOARD_COM_GET_DATA();
```

## 编译选项

### ONE_BOARD

定义此宏时，模块进入单板模式，所有通信函数将变为空操作或返回 NULL，用于单板

### GIMBAL_BOARD / CHASSIS_BOARD

定义板卡类型以启用相应的数据处理逻辑。

## 错误处理

模块使用 ulog 日志系统记录错误信息：

- 空指针检查
- CAN 设备初始化失败
- CAN 设备为空时尝试发送消息

## 注意事项

1. 确保在使用前正确配置 CAN 外设
2. 根据实际板卡类型定义相应宏（GIMBAL_BOARD 或 CHASSIS_BOARD）
3. 
4. 发送的数据会被压缩，接收时会自动解压缩
