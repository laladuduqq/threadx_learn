/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-01 18:17:58
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-01 18:34:32
 * @FilePath: /threadx_learn/applications/compoent_config.c
 * @Description: 
 */
#include "compoent_config.h"
#include "tx_api.h"
#include <stddef.h>


//内存分配
extern TX_BYTE_POOL tx_app_byte_pool;
// 内存分配函数
void* threadx_malloc(size_t size) {
    void *ptr = NULL;
    if (tx_byte_allocate(&tx_app_byte_pool, &ptr, size, TX_NO_WAIT) == TX_SUCCESS) {
        return ptr;
    } else {
        return NULL;
    }
}

// 内存释放函数
void threadx_free(void *ptr) {
    if (ptr != NULL) {
        tx_byte_release(ptr);
    }
}

