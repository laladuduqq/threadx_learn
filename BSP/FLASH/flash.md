# flash

```c
flash使用示例
BSP_Flash_Erase(FLASH_SECTOR_11, 1); //注意这里指向的都是一个地址，可以跳转看一下。
BSP_Flash_Write(ADDR_FLASH_SECTOR_11, tmpdata, sizeof(tmpdata));//要保证siezof(tmpdata）4字节对齐
BSP_Flash_Read(ADDR_FLASH_SECTOR_11, tmpdata, sizeof(tmpdata));
```
