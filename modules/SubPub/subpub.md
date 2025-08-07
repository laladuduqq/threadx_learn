<!--
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-08-07 08:30:01
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2025-08-07 10:06:53
 * @FilePath: /threadx_learn/modules/SubPub/subpub.md
 * @Description: SubPub模块完整文档
-->

# SubPub - 高性能无锁发布订阅消息系统

## 概述

SubPub是一个专为嵌入式实时系统设计的高性能无锁发布订阅消息系统。它采用原子操作实现无锁队列，支持指针模式和复制模式两种数据传输方式，针对1对1通信场景进行了优化。

## 核心特性

- **高性能**: 基于原子操作的无锁设计，支持高频率消息传递
- **双模式支持**: 指针模式（高效）和复制模式（安全）
- **延迟绑定**: 支持订阅者在发布者之前注册，自动绑定
- **类型安全**: 分离的接收函数，编译时类型检查
- **内存友好**: 静态内存布局，最小化内存碎片
- **实时性**: 无中断禁用，不影响系统实时性
- **跨平台**: 支持多种编译器和ARM Cortex-M系列处理器

# 

## 工作原理

### 1. 原子操作无锁队列原理

SubPub使用原子操作实现无锁队列，避免了传统锁机制的开销：

```c
// 原子操作实现
#if defined(__GNUC__)
    #define ATOMIC_LOAD(ptr) __atomic_load_n(&(ptr), __ATOMIC_ACQUIRE)
    #define ATOMIC_STORE(ptr, val) __atomic_store_n(&(ptr), val, __ATOMIC_RELEASE)
#elif defined(__ICCARM__) || defined(__CC_ARM) || defined(__ARMCC_VERSION)
    #define ATOMIC_LOAD(ptr) __LDREXW(&(ptr))
    #define ATOMIC_STORE(ptr, val) __STREXW(val, &(ptr))
#endif

// 发送消息流程
static inline int lockfree_queue_send(pubsub_lockfree_queue_t *queue, void *data, uint8_t data_len) {
    uint32_t head = ATOMIC_LOAD(queue->head);
    uint32_t next_head = (head + 1) & (PUBSUB_QUEUE_SIZE - 1);
    uint32_t tail = ATOMIC_LOAD(queue->tail);

    // 检查队列是否已满
    if (next_head == tail) {
        return PUBSUB_ERROR_QUEUE_FULL;
    }

    // 写入数据
    if (queue->mode == PUBSUB_MODE_POINTER) {
        queue->storage.pointers[head] = data;
        queue->data_len[head] = sizeof(void*);
    } else {
        memcpy(queue->storage.data[head], data, data_len);
        queue->data_len[head] = data_len;
    }

    // 原子更新head指针
    ATOMIC_STORE(queue->head, next_head);
    return PUBSUB_SUCCESS;
}
```

### 2. 延迟绑定机制

SubPub支持订阅者在发布者之前创建，系统会在发布者创建时自动绑定：

```c
// 订阅者可以先于发布者创建
pubsub_subscriber_t *sub = pubsub_create_subscriber("future_topic", sizeof(MyData_t));
// 此时 sub->topic_cache_valid = 0

// 后续创建发布者时自动绑定
pubsub_topic_t *topic = pubsub_create_topic("future_topic", PUBSUB_MODE_POINTER, sizeof(MyData_t));
// 自动设置 sub->topic_cache_valid = 1, sub->cached_topic_ptr = topic
```

### 3. 双模式数据传输

**指针模式**:

- 适用于大数据结构（>16字节）
- 只传递数据指针，性能最高
- 需要管理数据生命周期

```c
// 发送：存储数据指针
queue->storage.pointers[head] = data;

// 接收：返回数据指针
*data = queue->storage.pointers[tail];
```

**复制模式**:

- 适用于小数据结构（≤16字节）
- 复制数据内容，数据安全
- 有数据复制开销

```c
// 发送：复制数据内容
memcpy(queue->storage.data[head], data, data_len);

// 接收：复制数据内容
memcpy(data, queue->storage.data[tail], data_len);
```

## 配置参数

```c
#define PUBSUB_MAX_TOPICS 16                    // 最大话题数量
#define PUBSUB_MAX_SUBSCRIBERS_PER_TOPIC 1      // 每个话题最大订阅者数量（针对1对1优化）
#define PUBSUB_MAX_DATA_SIZE 64                 // 最大数据大小
#define PUBSUB_MAX_TOPIC_NAME_LEN 32            // 最大话题名称长度
#define PUBSUB_MAX_SUBSCRIBER_NAME_LEN 32       // 最大订阅者名称长度
#define PUBSUB_QUEUE_SIZE 4                     // 无锁队列大小（必须是2的幂）
```

## 使用说明

### 1. 基本使用流程

```c
#include "subpub.h"

// 1. 初始化系统
pubsub_init();

// 2. 创建话题（发布者）
pubsub_topic_t *gimbal_cmd_topic = pubsub_create_topic("gimbal_cmd", 
                                                       PUBSUB_MODE_POINTER, 
                                                       sizeof(Gimbal_Ctrl_Cmd_s));

// 3. 创建订阅者
pubsub_subscriber_t *gimbal_sub = pubsub_create_subscriber("gimbal_cmd", 
                                                          sizeof(Gimbal_Ctrl_Cmd_s));

// 4. 发布消息
Gimbal_Ctrl_Cmd_s gimbal_cmd_send = {0};
gimbal_cmd_send.gimbal_mode = GIMBAL_AUTO_MODE;
gimbal_cmd_send.pitch = 10.0f;
pubsub_publish(gimbal_cmd_topic, &gimbal_cmd_send, sizeof(Gimbal_Ctrl_Cmd_s));

// 5. 接收消息（指针模式）
Gimbal_Ctrl_Cmd_s *gimbal_cmd_recv_ptr = NULL;
int status = PUBSUB_RECEIVE_PTR(gimbal_sub,&gimbal_cmd_recv_ptr);
if (status == PUBSUB_SUCCESS && gimbal_cmd_recv_ptr != NULL) {
    printf("Received mode: %d, pitch: %f\n", 
           gimbal_cmd_recv_ptr->gimbal_mode, 
           gimbal_cmd_recv_ptr->pitch);
}
```

### 2. 模式选择

**指针模式** (PUBSUB_MODE_POINTER):

```c
typedef struct {
    float data[16];
    uint32_t timestamp;
    uint8_t flags;
} LargeData_t;

pubsub_topic_t *large_topic = pubsub_create_topic("large_data", 
                                                  PUBSUB_MODE_POINTER, 
                                                  sizeof(LargeData_t));
```

**复制模式** (PUBSUB_MODE_COPY):

```c
typedef struct {
    uint8_t cmd;
    uint8_t data[8];
} SmallData_t;

pubsub_topic_t *small_topic = pubsub_create_topic("small_data", 
                                                  PUBSUB_MODE_COPY, 
                                                  sizeof(SmallData_t));
```

### 3. 错误处理

```c
int status = pubsub_receive_ptr(subscriber, &data_ptr);
switch (status) {
    case PUBSUB_SUCCESS:
        // 成功接收
        break;
    case PUBSUB_ERROR_QUEUE_EMPTY:
        // 队列为空，正常情况
        break;
    case PUBSUB_ERROR_MODE_MISMATCH:
        // 模式不匹配，检查订阅者模式
        log_e("Mode mismatch detected\n");
        break;
    case PUBSUB_ERROR_INVALID_PARAM:
        // 参数错误
        log_e("Invalid parameters\n");
        break;
    default:
        // 其他错误
        log_e("Unknown error: %d\n", status);
        break;
}
```

### 4. 调试功能

```c
// 打印系统状态
pubsub_debug_print(PUBSUB_DEBUG_SYSTEM_STATUS, NULL);

// 打印话题列表
pubsub_debug_print(PUBSUB_DEBUG_TOPIC_LIST, NULL);

// 打印订阅者列表
pubsub_debug_print(PUBSUB_DEBUG_SUBSCRIBER_LIST, NULL);

// 打印指定话题的订阅者
pubsub_debug_print(PUBSUB_DEBUG_TOPIC_SUBSCRIBERS, "topic_name");
```

## 注意事项

1. **数据生命周期**: 指针模式下，确保数据在接收者使用期间保持有效
2. **订阅者数量**：目前设计的是1对1,所以订阅者数量恒为一
3. **队列溢出**: 高频发布时注意队列大小配置，避免溢出
4. **模式一致性**: 同一话题的所有订阅者必须使用相同的接收模式
5. **线程安全**: 系统设计为无锁，但话题和订阅者的创建/删除需要外部同步
6. **内存对齐**: 结构体已优化为8字节对齐，确保最佳性能
7. **编译器支持**: 需要支持原子操作的编译器
8. **接收宏定义**:实现编译时类型检查
