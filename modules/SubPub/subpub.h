/*
 * @Author: laladuduqq 17503181697@163.com
 * @Date: 2025-06-28 11:56:06
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-07 09:30:18
 * @FilePath: /threadx_learn/modules/SubPub/subpub.h
 * @Description: 高性能无锁发布订阅消息系统 - 原子操作版本，针对1对1通信优化
 * 
 */
#ifndef SUBPUB_H
#define SUBPUB_H

#include <stdint.h>
#include <stddef.h>

// 配置参数

#if defined(USE_COMPOENT_CONFIG_H) && USE_COMPOENT_CONFIG_H
#include "compoent_config.h"
#else
#define PUBSUB_MAX_TOPICS 16                    // 最大话题数量
#define PUBSUB_MAX_SUBSCRIBERS_PER_TOPIC 1      // 每个话题最大订阅者数量（针对1对1优化）
#define PUBSUB_MAX_DATA_SIZE 64                 // 最大数据大小
#define PUBSUB_MAX_TOPIC_NAME_LEN 32            // 最大话题名称长度
#define PUBSUB_MAX_SUBSCRIBER_NAME_LEN 32       // 最大订阅者名称长度
#define PUBSUB_QUEUE_SIZE 4                     // 无锁队列大小（必须是2的幂）
#endif // _COMPOENT_CONFIG_H_

// 编译器优化提示
#if defined(__GNUC__) || defined(__clang__)
    #define PUBSUB_INLINE __attribute__((always_inline)) inline
    #define PUBSUB_HOT __attribute__((hot))
    #define PUBSUB_CONST __attribute__((const))
#else
    #define PUBSUB_INLINE inline
    #define PUBSUB_HOT
    #define PUBSUB_CONST
#endif


// 发布模式选择
#define PUBSUB_MODE_POINTER 0                   // 指针模式：传递数据指针
#define PUBSUB_MODE_COPY 1                      // 复制模式：复制数据内容

// 返回状态码
#define PUBSUB_SUCCESS 0                        // 成功
#define PUBSUB_ERROR_INVALID_PARAM -1           // 无效参数
#define PUBSUB_ERROR_NO_MEMORY -2               // 内存不足
#define PUBSUB_ERROR_TOPIC_EXISTS -3            // 话题已存在
#define PUBSUB_ERROR_TOPIC_NOT_FOUND -4         // 话题不存在
#define PUBSUB_ERROR_SUBSCRIBER_EXISTS -5       // 订阅者已存在
#define PUBSUB_ERROR_SUBSCRIBER_NOT_FOUND -6    // 订阅者不存在
#define PUBSUB_ERROR_QUEUE_FULL -7              // 队列已满
#define PUBSUB_ERROR_QUEUE_EMPTY -8             // 队列为空
#define PUBSUB_ERROR_MODE_MISMATCH -9           // 模式不匹配

typedef enum {
    PUBSUB_DEBUG_TOPIC_LIST,
    PUBSUB_DEBUG_SUBSCRIBER_LIST,
    PUBSUB_DEBUG_TOPIC_SUBSCRIBERS,
    PUBSUB_DEBUG_SYSTEM_STATUS
} pubsub_debug_type_t;

// 前向声明
typedef struct pubsub_topic pubsub_topic_t;
typedef struct pubsub_subscriber pubsub_subscriber_t;

// 无锁环形队列结构体
typedef struct {
    volatile uint32_t head;                     // 写入位置
    volatile uint32_t tail;                     // 读取位置
    union {
        uint8_t data[PUBSUB_QUEUE_SIZE][PUBSUB_MAX_DATA_SIZE]; // 复制模式数据缓冲区
        void *pointers[PUBSUB_QUEUE_SIZE];      // 指针模式指针缓冲区
    } storage;
    uint8_t data_len[PUBSUB_QUEUE_SIZE];        // 每条消息的长度
    uint8_t mode;                               // 队列模式（指针/复制）
} pubsub_lockfree_queue_t;

// 话题结构体
struct pubsub_topic {
    char name[PUBSUB_MAX_TOPIC_NAME_LEN];       // 话题名称
    volatile uint8_t is_active;                 // 是否激活
    volatile uint8_t subscriber_count;          // 订阅者数量
    uint8_t publish_mode;                       // 发布模式
    uint8_t publisher_data_len;                 // 发布者规定的数据长度
    pubsub_subscriber_t *subscribers[PUBSUB_MAX_SUBSCRIBERS_PER_TOPIC]; // 订阅者数组（针对1对1优化）
    pubsub_topic_t *next;                       // 下一个话题
    uint8_t reserved[3];                        // 填充到64字节边界（根据实际结构调整）
} __attribute__((aligned(8)));

// 订阅者结构体
struct pubsub_subscriber {
    char name[PUBSUB_MAX_SUBSCRIBER_NAME_LEN];  // 订阅者名称
    volatile uint8_t is_active;                 // 是否激活
    pubsub_lockfree_queue_t queue;              // 无锁消息队列
    pubsub_subscriber_t *next;                  // 下一个订阅者
    uint8_t expected_data_len;                  // 订阅者期望的数据长度
    
    // 话题缓存机制 - 支持延迟绑定
    char target_topic_name[PUBSUB_MAX_TOPIC_NAME_LEN]; // 目标话题名称
    pubsub_topic_t *cached_topic_ptr;           // 缓存的话题指针
    uint8_t topic_cache_valid;                  // 话题缓存是否有效
    
    uint8_t reserved[3];                        // 填充到64字节边界（根据实际结构调整）
} __attribute__((aligned(8)));

#define PUBSUB_RECEIVE_PTR(subscriber, data_ptr) \
    pubsub_receive_ptr(subscriber, (void**)(data_ptr))

#define PUBSUB_RECEIVE_COPY(subscriber, data_ptr) \
    pubsub_receive_copy(subscriber, (void*)(data_ptr))

/**
 * @description:  初始化发布订阅系统
 * @return {int} 返回状态码
 * @note: 该函数必须在使用其他API之前调用
 */
int pubsub_init(void);
/**
 * @description:  注销一个话题
 * @param {char*} topic_name 要注销的话题名称
 * @return {int} 0表示成功，-1表示失败
 */
int pubsub_deinit(void);
/**
 * @description: 创建一个新的话题
 * @param {char} *topic_name，话题名称
 * @param {uint8_t} publish_mode，发布模式（指针或复制）
 * @param {uint8_t} data_len，话题数据长度
 * @return {pubsub_topic_t*} 返回创建的话题指针，失败返回NULL
 * @note: 话题名称长度限制为PUBSUB_MAX_TOPIC_NAME_LEN，数据长度限制为PUBSUB_MAX_TOPIC_DATA_LEN
 */
pubsub_topic_t* pubsub_create_topic(const char *topic_name, uint8_t publish_mode, uint8_t data_len);
/**
 * @description: 删除一个话题
 * @param {pubsub_topic_t*} topic 要删除的话题指针
 * @return {int} 返回状态码，0表示成功，-1表示失败
 */
int pubsub_delete_topic(pubsub_topic_t *topic);
/**
 * @description: 获取话题数量
 * @return {int} 返回话题数量
 */
int pubsub_get_topic_count(void);
/**
 * @description: 创建一个订阅者
 * @param {char*} topic_name 订阅者要订阅的话题名称
 * @param {uint8_t} expected_data_len 订阅者期望的数据长度
 * @return {pubsub_subscriber_t*} 返回创建的订阅者指针，失败返回NULL
 * @note: 如果话题不存在，将会延迟绑定
 */
pubsub_subscriber_t* pubsub_create_subscriber(const char *topic_name, uint8_t expected_data_len);
/**
 * @description: 删除一个订阅者
 * @param {pubsub_subscriber_t*} subscriber 订阅者指针
 * @return {int} 删除成功返回0，失败返回-1
 */
int pubsub_delete_subscriber(pubsub_subscriber_t *subscriber);
/**
 * @description: 发布消息到指定话题
 * @param {pubsub_topic_t*} topic 要发布的目标话题
 * @param {void*} data 要发布的数据指针
 * @param {uint8_t} data_len 数据长度
 * @return {int} 返回状态码，0表示成功，-1表示失败
 * @note: 该函数会检查数据长度是否与话题规定的长度匹配
 */
int pubsub_publish(pubsub_topic_t *topic, void *data, uint8_t data_len);
/**
 * @description: 接收消息 - 指针模式
 * @param {pubsub_subscriber_t*} subscriber 订阅者指针
 * @param {void**} data 接收的数据指针（指针模式）
 * @return {int} 返回状态码，0表示成功，-1表示失败
 * @note: 该函数会直接从无锁队列中接收数据，无需查找
 */
int pubsub_receive_ptr(pubsub_subscriber_t *subscriber, void **data);      
/**
 * @description: 接收消息 - 复制模式
 * @param {pubsub_subscriber_t*} subscriber 订阅者指针
 * @param {void*} data 接收的数据指针（复制模式）
 * @return {int} 返回状态码，0表示成功，-1表示失败
 * @note: 该函数会直接从无锁队列中接收数据，无需查找
 */
int pubsub_receive_copy(pubsub_subscriber_t *subscriber, void *data);    
/**
 * @description: 调试打印函数
 * @param {pubsub_debug_type_t} type 调试类型
 * @param {char*} topic_name 话题名称（可选）
 * @return {int} 返回状态码，0表示成功，-1表示失败
 */
int pubsub_debug_print(pubsub_debug_type_t type, const char *topic_name);

#endif // SUBPUB_H