/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-05 09:51:10
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-05 14:33:50
 * @FilePath: /threadx_learn/BSP/FLASH/bsp_flash.h
 * @Description: 
 */
#ifndef BSP_FLASH_H
#define BSP_FLASH_H

#include <stdint.h>

/* Base address of the Flash sectors */
#define ADDR_FLASH_SECTOR_0 ((uint32_t)0x08000000)  /* Base address of Sector 0, 16 Kbytes   */
#define ADDR_FLASH_SECTOR_1 ((uint32_t)0x08004000)  /* Base address of Sector 1, 16 Kbytes   */
#define ADDR_FLASH_SECTOR_2 ((uint32_t)0x08008000)  /* Base address of Sector 2, 16 Kbytes   */
#define ADDR_FLASH_SECTOR_3 ((uint32_t)0x0800C000)  /* Base address of Sector 3, 16 Kbytes   */
#define ADDR_FLASH_SECTOR_4 ((uint32_t)0x08010000)  /* Base address of Sector 4, 64 Kbytes   */
#define ADDR_FLASH_SECTOR_5 ((uint32_t)0x08020000)  /* Base address of Sector 5, 128 Kbytes  */
#define ADDR_FLASH_SECTOR_6 ((uint32_t)0x08040000)  /* Base address of Sector 6, 128 Kbytes  */
#define ADDR_FLASH_SECTOR_7 ((uint32_t)0x08060000)  /* Base address of Sector 7, 128 Kbytes  */
#define ADDR_FLASH_SECTOR_8 ((uint32_t)0x08080000)  /* Base address of Sector 8, 128 Kbytes  */
#define ADDR_FLASH_SECTOR_9 ((uint32_t)0x080A0000)  /* Base address of Sector 9, 128 Kbytes  */
#define ADDR_FLASH_SECTOR_10 ((uint32_t)0x080C0000) /* Base address of Sector 10, 128 Kbytes */
#define ADDR_FLASH_SECTOR_11 ((uint32_t)0x080E0000) /* Base address of Sector 11, 128 Kbytes */

#define FLASH_END_ADDR ((uint32_t)0x08100000)       /* Base address of Sector 23, 128 Kbytes */


#if defined(USE_COMPOENT_CONFIG_H) && USE_COMPOENT_CONFIG_H
#include "compoent_config.h"
#else
// 定义用户使用的Flash扇区
#define USER_FLASH_SECTOR_ADDR    ADDR_FLASH_SECTOR_11
#define USER_FLASH_SECTOR_NUMBER  FLASH_SECTOR_11
#endif // _COMPOENT_CONFIG_H_


// 错误码定义
typedef enum {
    FLASH_OK = 0,
    FLASH_ERR_BUSY,
    FLASH_ERR_PARAM,
    FLASH_ERR_ERASE,
    FLASH_ERR_WRITE,
    FLASH_ERR_ADDR
} Flash_Status;

/**
 * @description: Flash 擦除函数
 * @param {uint32_t} sector，扇区对应的基地址宏定义，如 ADDR_FLASH_SECTOR_11
 * @param {uint32_t} pages，要擦除的扇区数量
 * @return {Flash_Status} 状态码，0表示成功，非0表示失败
 */
Flash_Status BSP_Flash_Erase(uint32_t sector, uint32_t pages);
/**
 * @description: Flash 写入函数
 * @param {uint32_t} addr，写入的起始地址，必须是4字节对齐
 * @param {uint8_t*} data，指向要写入的数据缓冲区
 * @param {uint16_t} size，要写入的数据大小，必须是4字节的整数倍
 * @return {Flash_Status} 状态码，0表示成功，非0表示失败
 */
Flash_Status BSP_Flash_Write(uint32_t addr, uint8_t *data, uint16_t size);
/**
 * @description: Flash 读取函数
 * @param {uint32_t} addr，读取的起始地址
 * @param {uint8_t*} buf，指向存放读取数据的缓冲区
 * @param {uint16_t} size，要读取的数据大小
 * @return {Flash_Status} 状态码，0表示成功，非0表示失败
 */
Flash_Status BSP_Flash_Read(uint32_t addr, uint8_t *buf, uint16_t size);
/**
 * @description: 用户Flash写入函数，写入预定义的用户Flash扇区
 * @param {uint8_t*} data，指向要写入的数据缓冲区
 * @param {uint16_t} size，要写入的数据大小，必须是4字节的整数倍且不超过扇区大小
 * @return {Flash_Status} 状态码，0表示成功，非0表示失败
 */
Flash_Status BSP_UserFlash_Write(uint8_t *data, uint16_t size);
/**
 * @description: 用户Flash读取函数，读取预定义的用户Flash扇区
 * @param {uint8_t*} buf，指向存放读取数据的缓冲区
 * @param {uint16_t} size，要读取的数据大小，不超过扇区大小
 * @return {Flash_Status} 状态码，0表示成功，非0表示失败
 */
Flash_Status BSP_UserFlash_Read(uint8_t *buf, uint16_t size);
/**
 * @description: Flash初始化函数
 * @return {void}
 */
void BSP_Flash_Init(void);

#endif //BSP_FLASH_H