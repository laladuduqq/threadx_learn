#include "bsp_spi.h"
#include <string.h>
#include "tx_api.h"


#define LOG_TAG "bsp_spi"
#include "ulog.h"

/* SPI总线硬件配置数组 */
static const struct {
    SPI_HandleTypeDef *hspi;
    const char *name;
} spi_configs[SPI_BUS_NUM] = {
    {&hspi1, "SPI1"},
    {&hspi2, "SPI2"}
};

/* 总线管理器实例 */
static SPI_Bus_Manager spi_bus[SPI_BUS_NUM] = {
    {
        .hspi = NULL,
        .devices = {{0}},
        .spi_mutex = {0},
        .device_count = 0,
        .active_dev = NULL
    },
    {
        .hspi = NULL,
        .devices = {{0}},
        .spi_mutex = {0},
        .device_count = 0,
        .active_dev = NULL
    }
};

/* 私有函数声明 */
static void SPI_BusInit(void);
static void complete_spi_transfer(SPI_Bus_Manager *bus, SPI_Event_Type event);
static SPI_Bus_Manager* find_bus_by_handle(SPI_HandleTypeDef *hspi);

SPI_Device* BSP_SPI_Device_Init(SPI_Device_Init_Config* config) {
    /* 首次注册时初始化总线 */
    if (spi_bus[0].hspi == NULL) {
        ULOG_TAG_INFO("First time initializing SPI bus");
        SPI_BusInit();
    }

    /* 查找对应总线 */
    SPI_Bus_Manager *bus = NULL;
    for(uint8_t i = 0; i < SPI_BUS_NUM; i++) {
        if(spi_bus[i].hspi == config->hspi) {
            bus = &spi_bus[i];
            break;
        }
    }

    if(!bus) {
        ULOG_TAG_ERROR("SPI bus not found for handle: 0x%08X", (unsigned int)config->hspi);
        return NULL;
    }

    /* 检查设备数量 */
    if(bus->device_count >= MAX_DEVICES_PER_BUS) {
        ULOG_TAG_ERROR("Maximum devices reached on bus");
        return NULL;
    }

    /* 查找空闲设备槽位 */
    SPI_Device *dev = NULL;
    for(uint8_t i = 0; i < MAX_DEVICES_PER_BUS; i++) {
        if(bus->devices[i].hspi == NULL) {
            dev = &bus->devices[i];
            break;
        }
    }

    if(!dev) {
        ULOG_TAG_ERROR("No free device slot available on bus");
        return NULL;
    }

    /* 初始化设备 */
    dev->hspi = config->hspi;
    dev->cs_port = config->cs_port;
    dev->cs_pin = config->cs_pin;
    dev->tx_mode = config->tx_mode;
    dev->rx_mode = config->rx_mode;
    dev->timeout = config->timeout;
    dev->spi_callback = config->spi_callback;

    /* 配置CS引脚 */
    GPIO_InitTypeDef GPIO_InitStruct = {
        .Pin = dev->cs_pin,
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_HIGH
    };
    HAL_GPIO_Init(dev->cs_port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);

    bus->device_count++;
    ULOG_TAG_INFO("SPI device registered on bus %s", 
          spi_configs[config->hspi == &hspi1 ? 0 : 1].name);

    return dev;
}

HAL_StatusTypeDef BSP_SPI_TransReceive(SPI_Device* dev, const uint8_t* tx_data, 
                                      uint8_t* rx_data, uint16_t size)
{
    if (!dev || !tx_data || !rx_data || size == 0) {
        return HAL_ERROR;
    }

    SPI_Bus_Manager *bus = find_bus_by_handle(dev->hspi);
    if(!bus) return HAL_ERROR;

    // 获取互斥锁
    if (tx_mutex_get(&bus->spi_mutex, TX_WAIT_FOREVER) != TX_SUCCESS) {
        return HAL_ERROR;
    }

    HAL_StatusTypeDef status;
    bus->active_dev = dev;
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);

    if (dev->tx_mode == SPI_MODE_DMA && dev->rx_mode == SPI_MODE_DMA) {
        status = HAL_SPI_TransmitReceive_DMA(dev->hspi, tx_data, rx_data, size);
        // DMA模式下不立即释放锁，将在回调中释放
        if (status != HAL_OK) {
            complete_spi_transfer(bus, SPI_EVENT_ERROR);
        }
    }
    else if (dev->tx_mode == SPI_MODE_IT && dev->rx_mode == SPI_MODE_IT) {
        status = HAL_SPI_TransmitReceive_IT(dev->hspi, tx_data, rx_data, size);
        // 中断模式下不立即释放锁，将在回调中释放
        if (status != HAL_OK) {
            complete_spi_transfer(bus, SPI_EVENT_ERROR);
        }
    }
    else {
        status = HAL_SPI_TransmitReceive(dev->hspi, tx_data, rx_data, size, dev->timeout);
        // 阻塞模式下立即完成传输
        if (status == HAL_OK) {
            complete_spi_transfer(bus, SPI_EVENT_TX_RX_COMPLETE);
        } else {
            complete_spi_transfer(bus, SPI_EVENT_ERROR);
        }
    }

    return status;
}


void BSP_SPI_Device_DeInit(SPI_Device* dev) {
    if (!dev) return;
    
    for(uint8_t i = 0; i < SPI_BUS_NUM; i++) {
        for(uint8_t j = 0; j < MAX_DEVICES_PER_BUS; j++) {
            if(&spi_bus[i].devices[j] == dev) {
                // 在销毁设备前确保没有正在进行的传输
                if (spi_bus[i].active_dev == dev) {
                    spi_bus[i].active_dev = NULL;
                }
                memset(dev, 0, sizeof(SPI_Device));
                spi_bus[i].device_count--;
                return;
            }
        }
    }
}

HAL_StatusTypeDef BSP_SPI_Transmit(SPI_Device* dev, const uint8_t* tx_data, uint16_t size)
{
    if (!dev || !tx_data || size == 0) {
        return HAL_ERROR;
    }

    SPI_Bus_Manager *bus = find_bus_by_handle(dev->hspi);
    if(!bus) return HAL_ERROR;

    // 获取互斥锁
    if (tx_mutex_get(&bus->spi_mutex, TX_WAIT_FOREVER) != TX_SUCCESS) {
        return HAL_ERROR;
    }

    HAL_StatusTypeDef status;
    bus->active_dev = dev;
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);

    if (dev->tx_mode == SPI_MODE_DMA) {
        status = HAL_SPI_Transmit_DMA(dev->hspi, tx_data, size);
        // DMA模式下不立即释放锁，将在回调中释放
        if (status != HAL_OK) {
            complete_spi_transfer(bus, SPI_EVENT_ERROR);
        }
    }
    else if (dev->tx_mode == SPI_MODE_IT) {
        status = HAL_SPI_Transmit_IT(dev->hspi, tx_data, size);
        // 中断模式下不立即释放锁，将在回调中释放
        if (status != HAL_OK) {
            complete_spi_transfer(bus, SPI_EVENT_ERROR);
        }
    }
    else {
        status = HAL_SPI_Transmit(dev->hspi, tx_data, size, dev->timeout);
        // 阻塞模式下立即完成传输
        if (status == HAL_OK) {
            complete_spi_transfer(bus, SPI_EVENT_TX_COMPLETE);
        } else {
            complete_spi_transfer(bus, SPI_EVENT_ERROR);
        }
    }

    return status;
}

HAL_StatusTypeDef BSP_SPI_Receive(SPI_Device* dev, uint8_t* rx_data, uint16_t size)
{
    if (!dev || !rx_data || size == 0) {
        return HAL_ERROR;
    }

    SPI_Bus_Manager *bus = find_bus_by_handle(dev->hspi);
    if(!bus) return HAL_ERROR;

    // 获取互斥锁
    if (tx_mutex_get(&bus->spi_mutex, TX_WAIT_FOREVER) != TX_SUCCESS) {
        return HAL_ERROR;
    }

    HAL_StatusTypeDef status;
    bus->active_dev = dev;
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);

    if (dev->rx_mode == SPI_MODE_DMA) {
        status = HAL_SPI_Receive_DMA(dev->hspi, rx_data, size);
        // DMA模式下不立即释放锁，将在回调中释放
        if (status != HAL_OK) {
            complete_spi_transfer(bus, SPI_EVENT_ERROR);
        }
    }
    else if (dev->rx_mode == SPI_MODE_IT) {
        status = HAL_SPI_Receive_IT(dev->hspi, rx_data, size);
        // 中断模式下不立即释放锁，将在回调中释放
        if (status != HAL_OK) {
            complete_spi_transfer(bus, SPI_EVENT_ERROR);
        }
    }
    else {
        status = HAL_SPI_Receive(dev->hspi, rx_data, size, dev->timeout);
        // 阻塞模式下立即完成传输
        if (status == HAL_OK) {
            complete_spi_transfer(bus, SPI_EVENT_RX_COMPLETE);
        } else {
            complete_spi_transfer(bus, SPI_EVENT_ERROR);
        }
    }

    return status;
}

HAL_StatusTypeDef BSP_SPI_TransAndTrans(SPI_Device* dev, const uint8_t* tx_data1, 
                                       uint16_t size1, const uint8_t* tx_data2, 
                                       uint16_t size2)
{
    if (!dev || !tx_data1 || !tx_data2 || size1 == 0 || size2 == 0) {
        return HAL_ERROR;
    }

    SPI_Bus_Manager *bus = find_bus_by_handle(dev->hspi);
    if(!bus) return HAL_ERROR;

    // 获取互斥锁
    if (tx_mutex_get(&bus->spi_mutex, TX_WAIT_FOREVER) != TX_SUCCESS) {
        return HAL_ERROR;
    }

    HAL_StatusTypeDef status = HAL_OK;
    bus->active_dev = dev;
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);
    
    // 阻塞模式连续传输
    do {
        // 第一次传输
        if((status = HAL_SPI_Transmit(dev->hspi, (uint8_t*)tx_data1, size1, dev->timeout)) != HAL_OK) break;
        // 第二次传输
        status = HAL_SPI_Transmit(dev->hspi, (uint8_t*)tx_data2, size2, dev->timeout);
    } while(0);

    // 无论成功与否都要恢复CS和释放锁
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);
    bus->active_dev = NULL;
    tx_mutex_put(&bus->spi_mutex);
    
    return status;
}

/// SPI 接收完成回调
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    SPI_Bus_Manager *bus = find_bus_by_handle(hspi);
    if (bus && bus->active_dev != NULL) {
        TX_INTERRUPT_SAVE_AREA
        TX_DISABLE
        
        complete_spi_transfer(bus, SPI_EVENT_RX_COMPLETE);
        
        TX_RESTORE
    }
}

// SPI 发送接收完成回调
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    SPI_Bus_Manager *bus = find_bus_by_handle(hspi);
    if (bus && bus->active_dev != NULL) {
        TX_INTERRUPT_SAVE_AREA
        TX_DISABLE
        
        complete_spi_transfer(bus, SPI_EVENT_TX_RX_COMPLETE);
        
        TX_RESTORE
    }
}

// SPI 发送完成回调
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    SPI_Bus_Manager *bus = find_bus_by_handle(hspi);
    if (bus && bus->active_dev != NULL) {
        TX_INTERRUPT_SAVE_AREA
        TX_DISABLE
        
        complete_spi_transfer(bus, SPI_EVENT_TX_COMPLETE);
        
        TX_RESTORE
    }
}

// 添加错误回调处理
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    SPI_Bus_Manager *bus = find_bus_by_handle(hspi);
    if (bus && bus->active_dev != NULL) {
        TX_INTERRUPT_SAVE_AREA
        TX_DISABLE
        
        // 在错误情况下也完成传输过程
        complete_spi_transfer(bus, SPI_EVENT_ERROR);
        
        TX_RESTORE
    }
}


/** 内部私有函数 */

//查找总线
static SPI_Bus_Manager* find_bus_by_handle(SPI_HandleTypeDef *hspi) {
    for(uint8_t i = 0; i < SPI_BUS_NUM; i++) {
        if(spi_bus[i].hspi == hspi) {
            return &spi_bus[i];
        }
    }
    return NULL;
}

// 完成传输操作
static void complete_spi_transfer(SPI_Bus_Manager *bus, SPI_Event_Type event) {
    if (bus && bus->active_dev) {
        // 恢复CS引脚
        HAL_GPIO_WritePin(bus->active_dev->cs_port, bus->active_dev->cs_pin, GPIO_PIN_SET);
        
        // 执行回调函数（如果存在）
        if(bus->active_dev->spi_callback) {
            bus->active_dev->spi_callback(event);
        }
        
        // 清除活动设备标记
        bus->active_dev = NULL;
        
        // 释放互斥锁
        tx_mutex_put(&bus->spi_mutex);
    }
}

static void SPI_BusInit(void) {
    for(uint8_t i = 0; i < SPI_BUS_NUM; i++) {
        spi_bus[i].hspi = spi_configs[i].hspi;
        
        //创建互斥锁
        if(tx_mutex_create(&spi_bus[i].spi_mutex,
                           (CHAR *)spi_configs[i].name,
                           TX_NO_INHERIT) != TX_SUCCESS) {
            ULOG_TAG_ERROR("Failed to create %s mutex", spi_configs[i].name);
            continue;
        }
        
        ULOG_TAG_INFO("%s bus initialized", spi_configs[i].name);
    }
}
