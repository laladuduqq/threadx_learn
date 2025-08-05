#include "bsp_flash.h"
#include "string.h"
#include "main.h"
#include "tx_api.h"


#define LOG_TAG "bsp_flash"
#include "ulog.h"

// 定义Flash操作互斥锁
static TX_MUTEX flash_mutex;

Flash_Status BSP_Flash_Erase(uint32_t sector, uint32_t pages) { //第二个参数 pages 传递要擦除的扇区数量，而不是字节数
    FLASH_EraseInitTypeDef erase;
    uint32_t sector_error;
    Flash_Status status = FLASH_OK;

    // 获取互斥锁
    if (tx_mutex_get(&flash_mutex, TX_WAIT_FOREVER) != TX_SUCCESS) {
        ULOG_TAG_ERROR("Failed to get flash mutex");
        return FLASH_ERR_BUSY;
    }

    ULOG_TAG_INFO("Start erasing flash sector %lu, pages: %lu", sector, pages);
    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.Sector = sector;
    erase.NbSectors = pages;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    HAL_FLASH_Unlock();

    if (HAL_FLASHEx_Erase(&erase, &sector_error) != HAL_OK) {
        ULOG_TAG_ERROR("Flash erase failed at sector: %lu, error sector: %lu", sector, sector_error);
        HAL_FLASH_Lock();
        status = FLASH_ERR_ERASE;
    }

    HAL_FLASH_Lock();
    
    // 释放互斥锁
    tx_mutex_put(&flash_mutex);
    
    if (status == FLASH_OK) {
        ULOG_TAG_INFO("Flash erase completed successfully");
    }
    return status;
}

Flash_Status BSP_Flash_Write(uint32_t addr, uint8_t *data, uint16_t size) {
    Flash_Status status = FLASH_OK;
    
    if (size % 4 != 0 || addr % 4 != 0){
        ULOG_TAG_ERROR("Invalid parameters: addr=0x%08lX, size=%u (both must be 4-byte aligned)", addr, size);
        return FLASH_ERR_PARAM;
    }
    
    // 获取互斥锁
    if (tx_mutex_get(&flash_mutex, TX_WAIT_FOREVER) != TX_SUCCESS) {
        ULOG_TAG_ERROR("Failed to get flash mutex");
        return FLASH_ERR_BUSY;
    }
    
    ULOG_TAG_INFO("Start writing %u bytes to flash address 0x%08lX", size, addr);
    HAL_FLASH_Unlock();

    for (uint16_t i = 0; i < size; i += 4) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,addr + i,*(uint32_t*)(data + i)) != HAL_OK) {
            ULOG_TAG_ERROR("Flash write failed at address 0x%08lX", addr + i);
            HAL_FLASH_Lock();
            status = FLASH_ERR_WRITE;
            break;
        }
    }
    HAL_FLASH_Lock();
    
    // 释放互斥锁
    tx_mutex_put(&flash_mutex);
    
    if (status == FLASH_OK) {
        ULOG_TAG_INFO("Flash write completed successfully");
    }
    return status;
}

Flash_Status BSP_Flash_Read(uint32_t addr, uint8_t *buf, uint16_t size) {
    if (!IS_FLASH_ADDRESS(addr) || !buf || size == 0){
        ULOG_TAG_ERROR("Invalid parameters for flash read: addr=0x%08lX, buf=%p, size=%u", addr, buf, size); 
        return FLASH_ERR_PARAM;
    }
    
    // 获取互斥锁
    if (tx_mutex_get(&flash_mutex, TX_WAIT_FOREVER) != TX_SUCCESS) {
        ULOG_TAG_ERROR("Failed to get flash mutex");
        return FLASH_ERR_BUSY;
    }
    
    ULOG_TAG_INFO("Reading %u bytes from flash address 0x%08lX", size, addr);
    memcpy(buf, (void*)addr, size);
    
    // 释放互斥锁
    tx_mutex_put(&flash_mutex);
    
    return FLASH_OK;
}

Flash_Status BSP_UserFlash_Write(uint8_t *data, uint16_t size) {
    Flash_Status status;
    
    if (!data || size == 0) {
        ULOG_TAG_ERROR("Invalid parameters for user flash write: data=%p, size=%u", data, size);
        return FLASH_ERR_PARAM;
    }
    
    if (size % 4 != 0) {
        ULOG_TAG_ERROR("Data size must be 4-byte aligned: size=%u", size);
        return FLASH_ERR_PARAM;
    }
    
    // 获取互斥锁
    if (tx_mutex_get(&flash_mutex, TX_WAIT_FOREVER) != TX_SUCCESS) {
        ULOG_TAG_ERROR("Failed to get flash mutex");
        return FLASH_ERR_BUSY;
    }
    
    // 擦除扇区
    status = BSP_Flash_Erase(USER_FLASH_SECTOR_NUMBER, 1);
    if (status != FLASH_OK) {
        ULOG_TAG_ERROR("Failed to erase user flash sector");
        tx_mutex_put(&flash_mutex);
        return status;
    }
    
    // 写入数据
    status = BSP_Flash_Write(USER_FLASH_SECTOR_ADDR, data, size);
    if (status != FLASH_OK) {
        ULOG_TAG_ERROR("Failed to write data to user flash");
        tx_mutex_put(&flash_mutex);
        return status;
    }
    
    // 释放互斥锁
    tx_mutex_put(&flash_mutex);
    
    ULOG_TAG_INFO("User flash write completed successfully");
    return FLASH_OK;
}

Flash_Status BSP_UserFlash_Read(uint8_t *buf, uint16_t size) {
    Flash_Status status;
    
    if (!buf || size == 0) {
        ULOG_TAG_ERROR("Invalid parameters for user flash read: buf=%p, size=%u", buf, size);
        return FLASH_ERR_PARAM;
    }
    
    // 获取互斥锁
    if (tx_mutex_get(&flash_mutex, TX_WAIT_FOREVER) != TX_SUCCESS) {
        ULOG_TAG_ERROR("Failed to get flash mutex");
        return FLASH_ERR_BUSY;
    }
    
    status = BSP_Flash_Read(USER_FLASH_SECTOR_ADDR, buf, size);
    
    // 释放互斥锁
    tx_mutex_put(&flash_mutex);
    
    return status;
}

// 初始化Flash模块，创建互斥锁
void BSP_Flash_Init(void) {
    // 创建互斥锁
    tx_mutex_create(&flash_mutex, "Flash Mutex", TX_NO_INHERIT);
}