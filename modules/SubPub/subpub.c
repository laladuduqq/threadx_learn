/*
 * @Author: laladuduqq 17503181697@163.com
 * @Date: 2025-06-28 11:56:06
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-07 09:45:51
 * @FilePath: /threadx_learn/modules/SubPub/subpub.c
 * @Description: 无锁发布订阅消息系统 - 针对1对1场景
 * 
 */
#include "subpub.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define LOG_TAG              "subpub"
#include <ulog.h>

// 全局变量
static pubsub_topic_t *topic_list = NULL;
static pubsub_subscriber_t *subscriber_list = NULL;

// 内部函数声明
static int find_topic(const char *topic_name, pubsub_topic_t **topic);
static int add_topic_to_list(pubsub_topic_t *topic);
static int remove_topic_from_list(pubsub_topic_t *topic);
static int add_subscriber_to_list(pubsub_subscriber_t *subscriber);
static int remove_subscriber_from_list(pubsub_subscriber_t *subscriber);
static int try_bind_subscriber_to_topic(pubsub_subscriber_t *subscriber);

// 使用编译器内置原子操作
#if defined(__GNUC__)
    #define ATOMIC_LOAD(ptr) __atomic_load_n(&(ptr), __ATOMIC_ACQUIRE)
    #define ATOMIC_STORE(ptr, val) __atomic_store_n(&(ptr), val, __ATOMIC_RELEASE)
    #define ATOMIC_EXCHANGE(ptr, val) __atomic_exchange_n(&(ptr), val, __ATOMIC_ACQ_REL)
    #define ATOMIC_COMPARE_EXCHANGE(ptr, expected, desired) __atomic_compare_exchange_n(&(ptr), &(expected), desired, false, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED)
#elif defined(__ICCARM__) || defined(__CC_ARM) || defined(__ARMCC_VERSION)
    // ARMCC 或 IAR 编译器的原子操作实现
    #define ATOMIC_LOAD(ptr) __LDREXW(&(ptr))
    #define ATOMIC_STORE(ptr, val) __STREXW(val, &(ptr))
    // 其他编译器需要特定实现
#else
    // 回退到 CAS 实现
    #define ATOMIC_LOAD(ptr) (ptr)
    #define ATOMIC_STORE(ptr, val) do { (ptr) = (val); } while(0)
    #define ATOMIC_EXCHANGE(ptr, val) __sync_lock_test_and_set(&(ptr), val)
    #define ATOMIC_COMPARE_EXCHANGE(ptr, expected, desired) __sync_bool_compare_and_swap(&(ptr), expected, desired)
#endif

// 队列初始化函数
static inline int lockfree_queue_init(pubsub_lockfree_queue_t *queue, uint8_t mode) {
    ATOMIC_STORE(queue->head, 0);
    ATOMIC_STORE(queue->tail, 0);
    queue->mode = mode;
    
    if (mode == PUBSUB_MODE_POINTER) {
        // 指针模式：初始化指针数组
        for (int i = 0; i < PUBSUB_QUEUE_SIZE; i++) {
            queue->storage.pointers[i] = NULL;
        }
    } else {
        // 复制模式：初始化数据缓冲区
        memset(queue->storage.data, 0, sizeof(queue->storage.data));
    }
    
    memset(queue->data_len, 0, sizeof(queue->data_len));
    return PUBSUB_SUCCESS;
}

// 获取队列中消息数量
static inline uint8_t lockfree_queue_get_count(pubsub_lockfree_queue_t *queue) {
    uint32_t head = ATOMIC_LOAD(queue->head);
    uint32_t tail = ATOMIC_LOAD(queue->tail);
    return (head - tail) & (PUBSUB_QUEUE_SIZE - 1);
}

// 初始化发布订阅系统
int pubsub_init(void) {
    topic_list = NULL;
    subscriber_list = NULL;
    
    ULOG_TAG_INFO("PubSub system initialized successfully");
    return PUBSUB_SUCCESS;
}

// 反初始化发布订阅系统
int pubsub_deinit(void) {
    // 删除所有话题
    pubsub_topic_t *topic = topic_list;
    while (topic != NULL) {
        pubsub_topic_t *next = topic->next;
        pubsub_delete_topic(topic);
        topic = next;
    }
    
    topic_list = NULL;
    subscriber_list = NULL;
    
    ULOG_TAG_INFO("PubSub system deinitialized successfully");
    return PUBSUB_SUCCESS;
}

// 创建话题
pubsub_topic_t* pubsub_create_topic(const char *topic_name, uint8_t publish_mode, uint8_t data_len) {
    if (topic_name == NULL || strlen(topic_name) >= PUBSUB_MAX_TOPIC_NAME_LEN ||
        (publish_mode != PUBSUB_MODE_POINTER && publish_mode != PUBSUB_MODE_COPY) ||
        data_len == 0 || data_len > PUBSUB_MAX_DATA_SIZE) {
        ULOG_TAG_ERROR("Invalid topic parameters: name=%p, mode=%d, len=%d", topic_name, publish_mode, data_len);
        return NULL;
    }
    
    // 检查话题是否已存在
    pubsub_topic_t *existing_topic;
    if (find_topic(topic_name, &existing_topic) == PUBSUB_SUCCESS) {
        ULOG_TAG_WARNING("Topic '%s' already exists", topic_name);
        return existing_topic;
    }
    
    // 创建新话题
    pubsub_topic_t *topic = (pubsub_topic_t*)threadx_malloc(sizeof(pubsub_topic_t));
    if (topic == NULL) {
        ULOG_TAG_ERROR("Failed to allocate memory for topic");
        return NULL;
    }
    
    // 初始化话题
    memset(topic, 0, sizeof(pubsub_topic_t));
    strcpy(topic->name, topic_name);
    topic->is_active = 1;
    topic->subscriber_count = 0;
    topic->publish_mode = publish_mode;
    topic->publisher_data_len = data_len;
    
    // 添加到话题列表
    add_topic_to_list(topic);
    
    // 尝试绑定等待的订阅者
    pubsub_subscriber_t *subscriber = subscriber_list;
    while (subscriber != NULL) {
        if (!subscriber->topic_cache_valid && 
            strcmp(subscriber->target_topic_name, topic_name) == 0) {
            
            // 检查数据长度是否匹配
            if (subscriber->expected_data_len == data_len) {
                // 重新初始化队列以匹配话题模式
                lockfree_queue_init(&subscriber->queue, publish_mode);
                
                // 缓存话题指针
                subscriber->cached_topic_ptr = topic;
                subscriber->topic_cache_valid = 1;
                
                // 将订阅者添加到话题的订阅者列表
                if (topic->subscriber_count < PUBSUB_MAX_SUBSCRIBERS_PER_TOPIC) {
                    topic->subscribers[topic->subscriber_count] = subscriber;
                    topic->subscriber_count++;
                    
                    ULOG_TAG_INFO("Auto-bound waiting subscriber '%s' to new topic '%s'", 
                          subscriber->name, topic_name);
                }
            } else {
                ULOG_TAG_WARNING("Data length mismatch for auto-binding: topic=%d, subscriber=%d", 
                      data_len, subscriber->expected_data_len);
            }
        }
        subscriber = subscriber->next;
    }
    
    ULOG_TAG_INFO("Topic '%s' created successfully (mode: %s, data_len: %d, auto-bound subscribers: %d)", 
           topic_name, publish_mode == PUBSUB_MODE_POINTER ? "POINTER" : "COPY", 
           data_len, topic->subscriber_count);
    
    return topic;
}

// 删除话题
int pubsub_delete_topic(pubsub_topic_t *topic) {
    if (topic == NULL) {
        return PUBSUB_ERROR_INVALID_PARAM;
    }
    
    // 删除所有订阅者
    for (int i = 0; i < topic->subscriber_count; i++) {
        pubsub_subscriber_t *subscriber = topic->subscribers[i];
        if (subscriber != NULL) {
            // 从全局订阅者列表移除
            remove_subscriber_from_list(subscriber);
            
            // 释放订阅者内存
            threadx_free(subscriber);
        }
    }
    topic->subscriber_count = 0;
    
    // 从列表中移除话题
    remove_topic_from_list(topic);
    
    // 释放话题内存
    threadx_free(topic);
    
    ULOG_TAG_INFO("Topic deleted successfully\n");
    return PUBSUB_SUCCESS;
}

// 获取话题数量
int pubsub_get_topic_count(void) {    
    int count = 0;
    pubsub_topic_t *topic = topic_list;
    while (topic != NULL) {
        count++;
        topic = topic->next;
    }
    return count;
}

// 创建订阅者 - 支持延迟绑定
pubsub_subscriber_t* pubsub_create_subscriber(const char *topic_name, uint8_t expected_data_len) {
    if (topic_name == NULL || expected_data_len == 0 || expected_data_len > PUBSUB_MAX_DATA_SIZE) {
        ULOG_TAG_ERROR("Invalid create_subscriber parameters: topic=%p, expected_len=%d\n", topic_name, expected_data_len);
        return NULL;
    }
    
    // 查找话题
    pubsub_topic_t *topic;
    int topic_found = find_topic(topic_name, &topic);
    
    // 创建订阅者
    pubsub_subscriber_t *subscriber = (pubsub_subscriber_t*)threadx_malloc(sizeof(pubsub_subscriber_t));
    if (subscriber == NULL) {
        ULOG_TAG_ERROR("Failed to allocate memory for subscriber");
        return NULL;
    }
    
    // 初始化订阅者
    memset(subscriber, 0, sizeof(pubsub_subscriber_t));
    snprintf(subscriber->name, PUBSUB_MAX_SUBSCRIBER_NAME_LEN, "sub_%s_%d", topic_name, 
             topic_found == PUBSUB_SUCCESS ? topic->subscriber_count : 0);
    subscriber->is_active = 1;
    subscriber->expected_data_len = expected_data_len;
    
    // 保存目标话题名称，用于延迟绑定
    strcpy(subscriber->target_topic_name, topic_name);
    subscriber->cached_topic_ptr = NULL;
    subscriber->topic_cache_valid = 0;
    
    if (topic_found == PUBSUB_SUCCESS) {
        // 话题已存在，立即绑定
        if (expected_data_len != topic->publisher_data_len) {
            threadx_free(subscriber);
            ULOG_TAG_ERROR("Data length mismatch: publisher=%d, subscriber=%d for topic '%s'", 
                  topic->publisher_data_len, expected_data_len, topic_name);
            return NULL;
        }
        
        if (topic->subscriber_count >= PUBSUB_MAX_SUBSCRIBERS_PER_TOPIC) {
            threadx_free(subscriber);
            ULOG_TAG_ERROR("Topic '%s' has reached maximum subscribers", topic_name);
            return NULL;
        }
        
        // 初始化无锁队列
        lockfree_queue_init(&subscriber->queue, topic->publish_mode);
        
        // 缓存话题指针
        subscriber->cached_topic_ptr = topic;
        subscriber->topic_cache_valid = 1;
        
        // 将订阅者添加到话题的订阅者列表
        topic->subscribers[topic->subscriber_count] = subscriber;
        topic->subscriber_count++;
        
        ULOG_TAG_INFO("Subscriber '%s' created and bound to topic '%s' (data_len: %d)", 
              subscriber->name, topic_name, expected_data_len);
    } else {
        // 话题不存在，延迟绑定
        // 使用默认的复制模式初始化队列，后续会重新初始化
        lockfree_queue_init(&subscriber->queue, PUBSUB_MODE_COPY);
        
        ULOG_TAG_INFO("Subscriber '%s' created with delayed binding to topic '%s' (data_len: %d)", 
              subscriber->name, topic_name, expected_data_len);
    }
    
    // 添加到全局订阅者列表
    add_subscriber_to_list(subscriber);
    
    return subscriber;
}

// 删除订阅者
int pubsub_delete_subscriber(pubsub_subscriber_t *subscriber) {
    if (subscriber == NULL) {
        return PUBSUB_ERROR_INVALID_PARAM;
    }
    
    // 如果已绑定到话题，从话题的订阅者列表中移除
    if (subscriber->topic_cache_valid && subscriber->cached_topic_ptr != NULL) {
        pubsub_topic_t *topic = subscriber->cached_topic_ptr;
        
        for (int i = 0; i < topic->subscriber_count; i++) {
            if (topic->subscribers[i] == subscriber) {
                // 移除订阅者，保持数组连续
                for (int j = i; j < topic->subscriber_count - 1; j++) {
                    topic->subscribers[j] = topic->subscribers[j + 1];
                }
                topic->subscribers[topic->subscriber_count - 1] = NULL;
                topic->subscriber_count--;
                break;
            }
        }
    }
    
    // 从全局订阅者列表移除
    remove_subscriber_from_list(subscriber);
    
    // 释放订阅者内存
    threadx_free(subscriber);
    
    ULOG_TAG_INFO("Subscriber deleted successfully");
    return PUBSUB_SUCCESS;
}

// 尝试绑定订阅者到话题 - 内部函数
static int try_bind_subscriber_to_topic(pubsub_subscriber_t *subscriber) {
    if (subscriber->topic_cache_valid) {
        return PUBSUB_SUCCESS; // 已经绑定
    }
    
    // 查找话题
    pubsub_topic_t *topic;
    if (find_topic(subscriber->target_topic_name, &topic) != PUBSUB_SUCCESS) {
        return PUBSUB_ERROR_TOPIC_NOT_FOUND; // 话题仍不存在
    }
    
    // 检查数据长度是否匹配
    if (subscriber->expected_data_len != topic->publisher_data_len) {
        ULOG_TAG_ERROR("Data length mismatch during binding: publisher=%d, subscriber=%d for topic '%s'", 
              topic->publisher_data_len, subscriber->expected_data_len, subscriber->target_topic_name);
        return PUBSUB_ERROR_INVALID_PARAM;
    }
    
    // 检查话题的订阅者数量限制
    if (topic->subscriber_count >= PUBSUB_MAX_SUBSCRIBERS_PER_TOPIC) {
        ULOG_TAG_ERROR("Topic '%s' has reached maximum subscribers during binding", subscriber->target_topic_name);
        return PUBSUB_ERROR_INVALID_PARAM;
    }
    
    // 重新初始化队列以匹配话题模式
    lockfree_queue_init(&subscriber->queue, topic->publish_mode);
    
    // 缓存话题指针
    subscriber->cached_topic_ptr = topic;
    subscriber->topic_cache_valid = 1;
    
    // 将订阅者添加到话题的订阅者列表
    topic->subscribers[topic->subscriber_count] = subscriber;
    topic->subscriber_count++;
    
    ULOG_TAG_INFO("Subscriber '%s' successfully bound to topic '%s' (delayed binding)", 
          subscriber->name, subscriber->target_topic_name);
    
    return PUBSUB_SUCCESS;
}

// 发布消息 - 高度优化版本（针对1对1场景）
int pubsub_publish(pubsub_topic_t *topic, void *data, uint8_t data_len) {
    // 快速参数检查
    if (!topic || !data || !data_len || data_len > PUBSUB_MAX_DATA_SIZE || !topic->is_active) {
        return PUBSUB_ERROR_INVALID_PARAM;
    }
    
    // 检查数据长度是否匹配
    if (data_len != topic->publisher_data_len) {
        ULOG_TAG_ERROR("Data length mismatch: expected=%d, actual=%d for topic '%s'", 
              topic->publisher_data_len, data_len, topic->name);
        return PUBSUB_ERROR_INVALID_PARAM;
    }
    
    // 由于PUBSUB_MAX_SUBSCRIBERS_PER_TOPIC=1，我们只需要处理一个订阅者
    if (topic->subscriber_count > 0) {
        pubsub_subscriber_t *subscriber = topic->subscribers[0];
        if (subscriber && subscriber->is_active) {
            // 直接操作队列（完全内联优化）
            pubsub_lockfree_queue_t *queue = &subscriber->queue;
            uint32_t head = ATOMIC_LOAD(queue->head);
            uint32_t next_head = (head + 1) & (PUBSUB_QUEUE_SIZE - 1);
            uint32_t tail = ATOMIC_LOAD(queue->tail);
            
            // 检查队列是否已满
            if (next_head == tail) {
                return PUBSUB_ERROR_QUEUE_FULL;
            }
            
            // 写入数据（内联优化）
            if (queue->mode == PUBSUB_MODE_POINTER) {
                queue->storage.pointers[head] = data;
                queue->data_len[head] = sizeof(void*);
            } else {
                if (data_len <= sizeof(uint32_t)) {
                    // 对于非常小的数据，直接按字节复制
                    uint8_t *src = (uint8_t*)data;
                    uint8_t *dst = queue->storage.data[head];
                    for (int i = 0; i < data_len; i++) {
                        dst[i] = src[i];
                    }
                } else {
                    // 对于较大数据，使用memcpy
                    memcpy(queue->storage.data[head], data, data_len);
                }
                queue->data_len[head] = data_len;
            }
            
            // 原子更新head指针
            ATOMIC_STORE(queue->head, next_head);
            return PUBSUB_SUCCESS;
        }
    }
    
    return PUBSUB_ERROR_QUEUE_FULL;
}

// 指针模式接收消息
int pubsub_receive_ptr(pubsub_subscriber_t *subscriber, void **data) {
    // 快速参数检查
    if (!subscriber || !data || !subscriber->is_active) {
        return PUBSUB_ERROR_INVALID_PARAM;
    }
    
    // 尝试延迟绑定（仅在必要时）
    if (!subscriber->topic_cache_valid) {
        int bind_result = try_bind_subscriber_to_topic(subscriber);
        if (bind_result != PUBSUB_SUCCESS) {
            return PUBSUB_ERROR_QUEUE_EMPTY;
        }
    }
    
    // 检查模式是否匹配
    if (subscriber->queue.mode != PUBSUB_MODE_POINTER) {
        ULOG_TAG_ERROR("Mode mismatch: subscriber '%s' is not in POINTER mode", subscriber->name);
        return PUBSUB_ERROR_MODE_MISMATCH;
    }
    
    // 直接访问队列（内联优化）
    pubsub_lockfree_queue_t *queue = &subscriber->queue;
    uint32_t tail = ATOMIC_LOAD(queue->tail);
    uint32_t head = ATOMIC_LOAD(queue->head);
    
    // 检查队列是否为空
    if (head == tail) {
        return PUBSUB_ERROR_QUEUE_EMPTY;
    }
    
    // 读取数据
    *data = queue->storage.pointers[tail];
    
    // 原子更新tail指针
    uint32_t next_tail = (tail + 1) & (PUBSUB_QUEUE_SIZE - 1);
    ATOMIC_STORE(queue->tail, next_tail);
    return PUBSUB_SUCCESS;
}

// 复制模式接收消息 
int pubsub_receive_copy(pubsub_subscriber_t *subscriber, void *data) {
    // 快速参数检查
    if (!subscriber || !data || !subscriber->is_active) {
        return PUBSUB_ERROR_INVALID_PARAM;
    }
    
    // 尝试延迟绑定（仅在必要时）
    if (!subscriber->topic_cache_valid) {
        int bind_result = try_bind_subscriber_to_topic(subscriber);
        if (bind_result != PUBSUB_SUCCESS) {
            return PUBSUB_ERROR_QUEUE_EMPTY;
        }
    }
    
    // 检查模式是否匹配
    if (subscriber->queue.mode != PUBSUB_MODE_COPY) {
        ULOG_TAG_ERROR("Mode mismatch: subscriber '%s' is not in COPY mode", subscriber->name);
        return PUBSUB_ERROR_MODE_MISMATCH;
    }
    
    // 直接访问队列（内联优化）
    pubsub_lockfree_queue_t *queue = &subscriber->queue;
    uint32_t tail = ATOMIC_LOAD(queue->tail);
    uint32_t head = ATOMIC_LOAD(queue->head);
    
    // 检查队列是否为空
    if (head == tail) {
        return PUBSUB_ERROR_QUEUE_EMPTY;
    }
    
    uint8_t data_len = queue->data_len[tail];
    if (data_len <= sizeof(uint32_t)) {
        // 对于非常小的数据，直接按字节复制
        uint8_t *src = queue->storage.data[tail];
        uint8_t *dst = (uint8_t*)data;
        for (int i = 0; i < data_len; i++) {
            dst[i] = src[i];
        }
    } else {
        // 对于较大数据，使用memcpy
        memcpy(data, queue->storage.data[tail], data_len);
    }
    
    // 原子更新tail指针
    uint32_t next_tail = (tail + 1) & (PUBSUB_QUEUE_SIZE - 1);
    ATOMIC_STORE(queue->tail, next_tail);
    return PUBSUB_SUCCESS;
}

// 内部函数实现

// 查找话题
static int find_topic(const char *topic_name, pubsub_topic_t **topic) {
    pubsub_topic_t *current = topic_list;
    while (current != NULL) {
        if (strcmp(current->name, topic_name) == 0) {
            *topic = current;
            return PUBSUB_SUCCESS;
        }
        current = current->next;
    }
    return PUBSUB_ERROR_TOPIC_NOT_FOUND;
}

// 添加话题到列表
static int add_topic_to_list(pubsub_topic_t *topic) {
    topic->next = topic_list;
    topic_list = topic;
    return PUBSUB_SUCCESS;
}

// 从列表中移除话题
static int remove_topic_from_list(pubsub_topic_t *topic) {
    if (topic_list == topic) {
        topic_list = topic->next;
        return PUBSUB_SUCCESS;
    }
    
    pubsub_topic_t *current = topic_list;
    while (current != NULL && current->next != topic) {
        current = current->next;
    }
    
    if (current != NULL) {
        current->next = topic->next;
        return PUBSUB_SUCCESS;
    }
    
    return PUBSUB_ERROR_TOPIC_NOT_FOUND;
}

// 添加订阅者到列表
static int add_subscriber_to_list(pubsub_subscriber_t *subscriber) {
    subscriber->next = subscriber_list;
    subscriber_list = subscriber;
    return PUBSUB_SUCCESS;
}

// 从列表中移除订阅者
static int remove_subscriber_from_list(pubsub_subscriber_t *subscriber) {
    if (subscriber_list == subscriber) {
        subscriber_list = subscriber->next;
        return PUBSUB_SUCCESS;
    }
    
    pubsub_subscriber_t *current = subscriber_list;
    while (current != NULL && current->next != subscriber) {
        current = current->next;
    }
    
    if (current != NULL) {
        current->next = subscriber->next;
        return PUBSUB_SUCCESS;
    }
    
    return PUBSUB_ERROR_SUBSCRIBER_NOT_FOUND;
}

// 调试和状态查询函数实现
int pubsub_debug_print(pubsub_debug_type_t type, const char *topic_name) {
    switch (type) {
        case PUBSUB_DEBUG_TOPIC_LIST: {
            ULOG_TAG_INFO("=== Topic List ===");
            if (topic_list == NULL) {
                ULOG_TAG_INFO("No topics found");
                break;
            }
            pubsub_topic_t *topic = topic_list;
            uint8_t count = 0;
            while (topic != NULL) {
                ULOG_TAG_INFO("[%d] Topic: '%s' (Mode: %s, Data Len: %d, Subscribers: %d, Active: %s)", 
                      count++,
                      topic->name,
                      topic->publish_mode == PUBSUB_MODE_POINTER ? "POINTER" : "COPY",
                      topic->publisher_data_len,
                      topic->subscriber_count,
                      topic->is_active ? "YES" : "NO");
                topic = topic->next;
            }
            ULOG_TAG_INFO("Total topics: %d", count);
            ULOG_TAG_INFO("================");
            break;
        }
        case PUBSUB_DEBUG_SUBSCRIBER_LIST: {
            ULOG_TAG_INFO("=== Subscriber List ===");
            if (subscriber_list == NULL) {
                ULOG_TAG_INFO("No subscribers found");
                break;
            }
            pubsub_subscriber_t *subscriber = subscriber_list;
            int count = 0;
            while (subscriber != NULL) {
                uint8_t messages_in_queue = lockfree_queue_get_count(&subscriber->queue);
                ULOG_TAG_INFO("[%d] Subscriber: '%s' (Data Len: %d, Active: %s, Queue Messages: %d)", 
                      count++,
                      subscriber->name,
                      subscriber->expected_data_len,
                      subscriber->is_active ? "YES" : "NO",
                      messages_in_queue);
                subscriber = subscriber->next;
            }
            ULOG_TAG_INFO("Total subscribers: %d", count);
            ULOG_TAG_INFO("=====================");
            break;
        }
        case PUBSUB_DEBUG_TOPIC_SUBSCRIBERS: {
            if (topic_name == NULL) return PUBSUB_ERROR_INVALID_PARAM;
            pubsub_topic_t *topic;
            if (find_topic(topic_name, &topic) != PUBSUB_SUCCESS) {
                ULOG_TAG_ERROR("Topic '%s' not found", topic_name);
                return PUBSUB_ERROR_TOPIC_NOT_FOUND;
            }
            ULOG_TAG_INFO("=== Topic '%s' Subscribers ===", topic_name);
            ULOG_TAG_INFO("Mode: %s, Data Len: %d, Active: %s", 
                  topic->publish_mode == PUBSUB_MODE_POINTER ? "POINTER" : "COPY",
                  topic->publisher_data_len,
                  topic->is_active ? "YES" : "NO");
            if (topic->subscriber_count == 0) {
                ULOG_TAG_INFO("No subscribers");
            } else {
                for (int i = 0; i < topic->subscriber_count; i++) {
                    pubsub_subscriber_t *subscriber = topic->subscribers[i];
                    if (subscriber != NULL) {
                        uint8_t messages_in_queue = lockfree_queue_get_count(&subscriber->queue);
                        ULOG_TAG_INFO("[%d] Subscriber: '%s' (Data Len: %d, Active: %s, Queue Messages: %d)", 
                              i,
                              subscriber->name,
                              subscriber->expected_data_len,
                              subscriber->is_active ? "YES" : "NO",
                              messages_in_queue);
                    }
                }
            }
            ULOG_TAG_INFO("Total subscribers: %d", topic->subscriber_count);
            ULOG_TAG_INFO("==========================");
            break;
        }
        case PUBSUB_DEBUG_SYSTEM_STATUS: {
            ULOG_TAG_INFO("=== PubSub System Status ===");
            int topic_count = pubsub_get_topic_count();
            int subscriber_count = 0;
            pubsub_subscriber_t *subscriber = subscriber_list;
            while (subscriber != NULL) {
                subscriber_count++;
                subscriber = subscriber->next;
            }
            ULOG_TAG_INFO("Topics: %d", topic_count);
            ULOG_TAG_INFO("Subscribers: %d", subscriber_count);
            int total_subscriptions = 0;
            pubsub_topic_t *topic = topic_list;
            while (topic != NULL) {
                total_subscriptions += topic->subscriber_count;
                topic = topic->next;
            }
            ULOG_TAG_INFO("Total subscriptions: %d", total_subscriptions);
            size_t estimated_memory = 0;
            topic = topic_list;
            while (topic != NULL) {
                estimated_memory += sizeof(pubsub_topic_t);
                topic = topic->next;
            }
            subscriber = subscriber_list;
            while (subscriber != NULL) {
                estimated_memory += sizeof(pubsub_subscriber_t);
                subscriber = subscriber->next;
            }
            ULOG_TAG_INFO("Estimated memory usage: %zu bytes", estimated_memory);
            ULOG_TAG_INFO("=====================================");
            break;
        }
        default:
            return PUBSUB_ERROR_INVALID_PARAM;
    }
    return PUBSUB_SUCCESS;
}