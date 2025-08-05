#include "bsp_iic.h"
#include <string.h>
#include "tx_api.h"

#define LOG_TAG "bsp_iic"
#include "ulog.h"

/* I2C总线硬件配置数组 */
static const struct {
    I2C_HandleTypeDef *hi2c;
    const char *name;
} i2c_configs[I2C_BUS_NUM] = {
    {&hi2c2, "I2C2"},
    {&hi2c3, "I2C3"}
};

/* 总线管理器实例 */
static I2C_Bus_Manager i2c_bus[I2C_BUS_NUM] = {
    {
        .hi2c = NULL,
        .devices = {{0}},
        .i2c_mutex = {0},
        .device_count = 0,
        .active_dev = NULL
    },
    {
        .hi2c = NULL,
        .devices = {{0}},
        .i2c_mutex = {0},
        .device_count = 0,
        .active_dev = NULL
    }
};

/* 私有函数声明 */
static void I2C_BusInit(void);
static void complete_i2c_transfer(I2C_Bus_Manager *bus, I2C_Event_Type event);
static I2C_Bus_Manager* find_bus_by_handle(I2C_HandleTypeDef *hi2c);

I2C_Device* BSP_I2C_Device_Init(I2C_Device_Init_Config* config) {
    /* 首次注册时初始化总线 */
    if (i2c_bus[0].hi2c == NULL) {
        ULOG_TAG_INFO("First time initializing I2C bus");
        I2C_BusInit();
    }

    /* 查找对应总线 */
    I2C_Bus_Manager *bus = NULL;
    for(uint8_t i = 0; i < I2C_BUS_NUM; i++) {
        if(i2c_bus[i].hi2c == config->hi2c) {
            bus = &i2c_bus[i];
            break;
        }
    }

    if(!bus) {
        ULOG_TAG_ERROR("I2C bus not found for handle: 0x%08X", (unsigned int)config->hi2c);
        return NULL;
    }

    /* 检查设备数量 */
    if(bus->device_count >= MAX_DEVICES_PER_I2C_BUS) {
        ULOG_TAG_ERROR("Maximum devices reached on bus");
        return NULL;
    }

    /* 查找空闲设备槽位 */
    I2C_Device *dev = NULL;
    for(uint8_t i = 0; i < MAX_DEVICES_PER_I2C_BUS; i++) {
        if(bus->devices[i].hi2c == NULL) {
            dev = &bus->devices[i];
            break;
        }
    }

    if(!dev) {
        ULOG_TAG_ERROR("No free device slot available on bus");
        return NULL;
    }

    /* 初始化设备 */
    dev->hi2c = config->hi2c;
    dev->dev_address = config->dev_address;
    dev->tx_mode = config->tx_mode;
    dev->rx_mode = config->rx_mode;
    dev->timeout = config->timeout;
    dev->i2c_callback = config->i2c_callback;

    bus->device_count++;
    ULOG_TAG_INFO("I2C device registered on bus %s with address 0x%02X", 
                  i2c_configs[config->hi2c == &hi2c2 ? 0 : 1].name, 
                  config->dev_address);

    return dev;
}

void BSP_I2C_Device_DeInit(I2C_Device* dev) {
    if (!dev) return;
    
    for(uint8_t i = 0; i < I2C_BUS_NUM; i++) {
        for(uint8_t j = 0; j < MAX_DEVICES_PER_I2C_BUS; j++) {
            if(&i2c_bus[i].devices[j] == dev) {
                // 在销毁设备前确保没有正在进行的传输
                if (i2c_bus[i].active_dev == dev) {
                    i2c_bus[i].active_dev = NULL;
                }
                memset(dev, 0, sizeof(I2C_Device));
                i2c_bus[i].device_count--;
                return;
            }
        }
    }
}

HAL_StatusTypeDef BSP_I2C_TransReceive(I2C_Device* dev, uint8_t* tx_data, 
                                      uint8_t* rx_data, uint16_t size)
{
    if (!dev || !tx_data || !rx_data || size == 0) {
        return HAL_ERROR;
    }

    I2C_Bus_Manager *bus = find_bus_by_handle(dev->hi2c);
    if(!bus) return HAL_ERROR;

    // 获取互斥锁
    if (tx_mutex_get(&bus->i2c_mutex, TX_WAIT_FOREVER) != TX_SUCCESS) {
        return HAL_ERROR;
    }

    HAL_StatusTypeDef status;
    bus->active_dev = dev;

    if (dev->tx_mode == I2C_MODE_DMA && dev->rx_mode == I2C_MODE_DMA) {
        status = HAL_I2C_Master_Transmit_DMA(dev->hi2c, dev->dev_address, tx_data, size);
        if (status == HAL_OK) {
            // 等待发送完成
            while (HAL_I2C_GetState(dev->hi2c) != HAL_I2C_STATE_READY) {
                // 等待发送完成
            }
            // 接收数据
            status = HAL_I2C_Master_Receive_DMA(dev->hi2c, dev->dev_address, rx_data, size);
        }
        // DMA模式下不立即释放锁，将在回调中释放
        if (status != HAL_OK) {
            complete_i2c_transfer(bus, I2C_EVENT_ERROR);
        }
    }
    else if (dev->tx_mode == I2C_MODE_IT && dev->rx_mode == I2C_MODE_IT) {
        status = HAL_I2C_Master_Transmit_IT(dev->hi2c, dev->dev_address, tx_data, size);
        if (status == HAL_OK) {
            // 等待发送完成
            while (HAL_I2C_GetState(dev->hi2c) != HAL_I2C_STATE_READY) {
                // 等待发送完成
            }
            // 接收数据
            status = HAL_I2C_Master_Receive_IT(dev->hi2c, dev->dev_address, rx_data, size);
        }
        // 中断模式下不立即释放锁，将在回调中释放
        if (status != HAL_OK) {
            complete_i2c_transfer(bus, I2C_EVENT_ERROR);
        }
    }
    else {
        status = HAL_I2C_Master_Transmit(dev->hi2c, dev->dev_address, tx_data, size, dev->timeout);
        if (status == HAL_OK) {
            status = HAL_I2C_Master_Receive(dev->hi2c, dev->dev_address, rx_data, size, dev->timeout);
        }
        // 阻塞模式下立即完成传输
        if (status == HAL_OK) {
            complete_i2c_transfer(bus, I2C_EVENT_TX_RX_COMPLETE);
        } else {
            complete_i2c_transfer(bus, I2C_EVENT_ERROR);
        }
    }

    return status;
}

HAL_StatusTypeDef BSP_I2C_Transmit(I2C_Device* dev, uint8_t* tx_data, uint16_t size)
{
    if (!dev || !tx_data || size == 0) {
        return HAL_ERROR;
    }

    I2C_Bus_Manager *bus = find_bus_by_handle(dev->hi2c);
    if(!bus) return HAL_ERROR;

    // 获取互斥锁
    if (tx_mutex_get(&bus->i2c_mutex, TX_WAIT_FOREVER) != TX_SUCCESS) {
        return HAL_ERROR;
    }

    HAL_StatusTypeDef status;
    bus->active_dev = dev;

    if (dev->tx_mode == I2C_MODE_DMA) {
        status = HAL_I2C_Master_Transmit_DMA(dev->hi2c, dev->dev_address, tx_data, size);
        // DMA模式下不立即释放锁，将在回调中释放
        if (status != HAL_OK) {
            complete_i2c_transfer(bus, I2C_EVENT_ERROR);
        }
    }
    else if (dev->tx_mode == I2C_MODE_IT) {
        status = HAL_I2C_Master_Transmit_IT(dev->hi2c, dev->dev_address, tx_data, size);
        // 中断模式下不立即释放锁，将在回调中释放
        if (status != HAL_OK) {
            complete_i2c_transfer(bus, I2C_EVENT_ERROR);
        }
    }
    else {
        status = HAL_I2C_Master_Transmit(dev->hi2c, dev->dev_address, tx_data, size, dev->timeout);
        // 阻塞模式下立即完成传输
        if (status == HAL_OK) {
            complete_i2c_transfer(bus, I2C_EVENT_TX_COMPLETE);
        } else {
            complete_i2c_transfer(bus, I2C_EVENT_ERROR);
        }
    }

    return status;
}

HAL_StatusTypeDef BSP_I2C_Receive(I2C_Device* dev, uint8_t* rx_data, uint16_t size)
{
    if (!dev || !rx_data || size == 0) {
        return HAL_ERROR;
    }

    I2C_Bus_Manager *bus = find_bus_by_handle(dev->hi2c);
    if(!bus) return HAL_ERROR;

    // 获取互斥锁
    if (tx_mutex_get(&bus->i2c_mutex, TX_WAIT_FOREVER) != TX_SUCCESS) {
        return HAL_ERROR;
    }

    HAL_StatusTypeDef status;
    bus->active_dev = dev;

    if (dev->rx_mode == I2C_MODE_DMA) {
        status = HAL_I2C_Master_Receive_DMA(dev->hi2c, dev->dev_address, rx_data, size);
        // DMA模式下不立即释放锁，将在回调中释放
        if (status != HAL_OK) {
            complete_i2c_transfer(bus, I2C_EVENT_ERROR);
        }
    }
    else if (dev->rx_mode == I2C_MODE_IT) {
        status = HAL_I2C_Master_Receive_IT(dev->hi2c, dev->dev_address, rx_data, size);
        // 中断模式下不立即释放锁，将在回调中释放
        if (status != HAL_OK) {
            complete_i2c_transfer(bus, I2C_EVENT_ERROR);
        }
    }
    else {
        status = HAL_I2C_Master_Receive(dev->hi2c, dev->dev_address, rx_data, size, dev->timeout);
        // 阻塞模式下立即完成传输
        if (status == HAL_OK) {
            complete_i2c_transfer(bus, I2C_EVENT_RX_COMPLETE);
        } else {
            complete_i2c_transfer(bus, I2C_EVENT_ERROR);
        }
    }

    return status;
}

HAL_StatusTypeDef BSP_I2C_Mem_Write_Read(I2C_Device* dev, uint16_t mem_address, 
                                        uint16_t mem_add_size, uint8_t* data, 
                                        uint16_t size, uint8_t is_write)
{
    if (!dev || !data || size == 0) {
        return HAL_ERROR;
    }

    I2C_Bus_Manager *bus = find_bus_by_handle(dev->hi2c);
    if(!bus) return HAL_ERROR;

    // 获取互斥锁
    if (tx_mutex_get(&bus->i2c_mutex, TX_WAIT_FOREVER) != TX_SUCCESS) {
        return HAL_ERROR;
    }

    HAL_StatusTypeDef status;
    bus->active_dev = dev;

    if (is_write) {
        if (dev->tx_mode == I2C_MODE_DMA) {
            status = HAL_I2C_Mem_Write_DMA(dev->hi2c, dev->dev_address, mem_address, mem_add_size, data, size);
            // DMA模式下不立即释放锁，将在回调中释放
            if (status != HAL_OK) {
                complete_i2c_transfer(bus, I2C_EVENT_ERROR);
            }
        }
        else if (dev->tx_mode == I2C_MODE_IT) {
            status = HAL_I2C_Mem_Write_IT(dev->hi2c, dev->dev_address, mem_address, mem_add_size, data, size);
            // 中断模式下不立即释放锁，将在回调中释放
            if (status != HAL_OK) {
                complete_i2c_transfer(bus, I2C_EVENT_ERROR);
            }
        }
        else {
            status = HAL_I2C_Mem_Write(dev->hi2c, dev->dev_address, mem_address, mem_add_size, data, size, dev->timeout);
            // 阻塞模式下立即完成传输
            if (status == HAL_OK) {
                complete_i2c_transfer(bus, I2C_EVENT_TX_COMPLETE);
            } else {
                complete_i2c_transfer(bus, I2C_EVENT_ERROR);
            }
        }
    } else {
        if (dev->rx_mode == I2C_MODE_DMA) {
            status = HAL_I2C_Mem_Read_DMA(dev->hi2c, dev->dev_address, mem_address, mem_add_size, data, size);
            // DMA模式下不立即释放锁，将在回调中释放
            if (status != HAL_OK) {
                complete_i2c_transfer(bus, I2C_EVENT_ERROR);
            }
        }
        else if (dev->rx_mode == I2C_MODE_IT) {
            status = HAL_I2C_Mem_Read_IT(dev->hi2c, dev->dev_address, mem_address, mem_add_size, data, size);
            // 中断模式下不立即释放锁，将在回调中释放
            if (status != HAL_OK) {
                complete_i2c_transfer(bus, I2C_EVENT_ERROR);
            }
        }
        else {
            status = HAL_I2C_Mem_Read(dev->hi2c, dev->dev_address, mem_address, mem_add_size, data, size, dev->timeout);
            // 阻塞模式下立即完成传输
            if (status == HAL_OK) {
                complete_i2c_transfer(bus, I2C_EVENT_RX_COMPLETE);
            } else {
                complete_i2c_transfer(bus, I2C_EVENT_ERROR);
            }
        }
    }

    return status;
}

HAL_StatusTypeDef BSP_I2C_IsDeviceReady(I2C_Device* dev)
{
    if (!dev) {
        return HAL_ERROR;
    }

    I2C_Bus_Manager *bus = find_bus_by_handle(dev->hi2c);
    if(!bus) return HAL_ERROR;

    // 获取互斥锁
    if (tx_mutex_get(&bus->i2c_mutex, TX_WAIT_FOREVER) != TX_SUCCESS) {
        return HAL_ERROR;
    }

    HAL_StatusTypeDef status;
    bus->active_dev = dev;

    status = HAL_I2C_IsDeviceReady(dev->hi2c, dev->dev_address, 3, dev->timeout);
    
    // 完成立即释放
    complete_i2c_transfer(bus, I2C_EVENT_TX_COMPLETE); // 使用发送完成事件表示设备检测完成
    
    return status;
}

/// I2C 接收完成回调
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    I2C_Bus_Manager *bus = find_bus_by_handle(hi2c);
    if (bus && bus->active_dev != NULL) {
        TX_INTERRUPT_SAVE_AREA
        TX_DISABLE
        
        complete_i2c_transfer(bus, I2C_EVENT_RX_COMPLETE);
        
        TX_RESTORE
    }
}

// I2C 发送完成回调
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    I2C_Bus_Manager *bus = find_bus_by_handle(hi2c);
    if (bus && bus->active_dev != NULL) {
        TX_INTERRUPT_SAVE_AREA
        TX_DISABLE
        
        complete_i2c_transfer(bus, I2C_EVENT_TX_COMPLETE);
        
        TX_RESTORE
    }
}

// 添加错误回调处理
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    I2C_Bus_Manager *bus = find_bus_by_handle(hi2c);
    if (bus && bus->active_dev != NULL) {
        TX_INTERRUPT_SAVE_AREA
        TX_DISABLE
        
        // 在错误情况下也完成传输过程
        complete_i2c_transfer(bus, I2C_EVENT_ERROR);
        
        TX_RESTORE
    }
}

// 内存读写完成回调
void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    I2C_Bus_Manager *bus = find_bus_by_handle(hi2c);
    if (bus && bus->active_dev != NULL) {
        TX_INTERRUPT_SAVE_AREA
        TX_DISABLE
        
        complete_i2c_transfer(bus, I2C_EVENT_TX_COMPLETE);
        
        TX_RESTORE
    }
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    I2C_Bus_Manager *bus = find_bus_by_handle(hi2c);
    if (bus && bus->active_dev != NULL) {
        TX_INTERRUPT_SAVE_AREA
        TX_DISABLE
        
        complete_i2c_transfer(bus, I2C_EVENT_RX_COMPLETE);
        
        TX_RESTORE
    }
}

/** 内部私有函数 */

//查找总线
static I2C_Bus_Manager* find_bus_by_handle(I2C_HandleTypeDef *hi2c) {
    for(uint8_t i = 0; i < I2C_BUS_NUM; i++) {
        if(i2c_bus[i].hi2c == hi2c) {
            return &i2c_bus[i];
        }
    }
    return NULL;
}

// 完成传输操作
static void complete_i2c_transfer(I2C_Bus_Manager *bus, I2C_Event_Type event) {
    if (bus && bus->active_dev) {
        // 执行回调函数（如果存在）
        if(bus->active_dev->i2c_callback) {
            bus->active_dev->i2c_callback(event);
        }
        
        // 清除活动设备标记
        bus->active_dev = NULL;
        
        // 释放互斥锁
        tx_mutex_put(&bus->i2c_mutex);
    }
}

static void I2C_BusInit(void) {
    for(uint8_t i = 0; i < I2C_BUS_NUM; i++) {
        i2c_bus[i].hi2c = i2c_configs[i].hi2c;
        
        //创建互斥锁
        if(tx_mutex_create(&i2c_bus[i].i2c_mutex,
                           (CHAR *)i2c_configs[i].name,
                           TX_NO_INHERIT) != TX_SUCCESS) {
            ULOG_TAG_ERROR("Failed to create %s mutex", i2c_configs[i].name);
            continue;
        }
        
        ULOG_TAG_INFO("%s bus initialized", i2c_configs[i].name);
    }
}