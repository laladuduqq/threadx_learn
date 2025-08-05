# DWT (Data Watchpoint and Trace) 模块文档

## 简介

DWT (Data Watchpoint and Trace) 是ARM Cortex-M系列处理器中的一个调试和跟踪单元。本模块提供了一个高精度的定时器功能，可以用于精确的时间测量和延时操作。与传统的基于SysTick的延时方法相比，DWT不受中断状态的影响，可以在关闭中断的情况下正常工作。

## DWT模块原理

### DWT硬件结构

DWT (Data Watchpoint and Trace) 是ARM Cortex-M系列内核的一个调试外设，它提供了多种调试和跟踪功能：

1. **CYCCNT寄存器**：32位的cycle counter，用于计算处理器时钟周期数
2. **Comparator寄存器**：用于数据观察点比较
3. **Mask寄存器**：用于地址比较掩码
4. **Function寄存器**：定义比较器功能
   
   ### CYCCNT计数器工作原理
   
   CYCCNT是DWT模块中的核心计数器，具有以下特点：
- 32位宽度，以CPU主频连续计数
- 计数范围：0x00000000 到 0xFFFFFFFF
- 溢出后自动从0开始重新计数
- 可通过DWT->CTRL寄存器使能/禁用
  
  ### 溢出处理机制
  
  由于CYCCNT是32位计数器，在高频CPU下会快速溢出：
- 在168MHz下，约25.56秒溢出一次
- 在180MHz下，约23.86秒溢出一次
  为了处理溢出问题，本模块采用以下策略：
1. 使用一个软件计数器(CYCCNT_RountCount)记录溢出次数
2. 每次读取CYCCNT时检查是否发生溢出
3. 通过比较当前值与上次值判断是否溢出
4. 构建64位时间戳：`CYCCNT64 = CYCCNT_RountCount * UINT32_MAX + CYCCNT`
   
   ### 精度分析
   
   DWT计数精度直接取决于CPU时钟频率：
- 精度 = 1 / CPU_Freq_Hz 秒
- 在168MHz下，精度约为5.95纳秒
- 在180MHz下，精度约为5.56纳秒
  
  ### 与SysTick比较
  
  | 特性    | DWT      | SysTick      |
  | ----- | -------- | ------------ |
  | 精度    | CPU时钟周期级 | SysTick重载值决定 |
  | 中断依赖  | 不依赖      | 依赖SysTick中断  |
  | 关中断使用 | 可以       | 不可以          |
  | 实现复杂度 | 简单       | 需要中断处理       |
  
  ## 工作原理详解
  
  ### 初始化过程
1. 使能DWT外设：设置CoreDebug->DEMCR寄存器
2. 清零CYCCNT计数器
3. 使能CYCCNT计数：设置DWT->CTRL寄存器
4. 计算频率相关常量，用于时间换算
   
   ### 时间计算原理
   
   时间计算基于以下公式：时间(秒) = 计数差值 / CPU频率
   
   > 例如：
   > - CPU频率为168MHz (168,000,000 Hz)
   > - 计数差值为168,000
   > - 时间 = 168,000 / 168,000,000 = 0.001秒 = 1毫秒
   
   ### 时间轴更新机制
   
   为了提供系统运行时间，模块维护一个DWT_Time_t结构体：
   
   ```c
   typedef struct {
   uint32_t s; // 秒
   uint16_t ms; // 毫秒
   uint16_t us; // 微秒
   } DWT_Time_t;
   ```
   
   > 更新过程：
   > 
   > 1. 获取64位时间戳
   > 2. 通过除法和取余运算分别计算秒、毫秒、微秒部分
   > 3. 存储在SysTime结构体中
   
   ### 延时实现原理
   
   DWT_Delay函数基于循环等待实现：
   
   ```c
   while ((DWT->CYCCNT - tickstart) < wait * (float)CPU_FREQ_Hz);
   ```

> 该方法：
> 
> 1. 记录起始时间戳
> 2. 不断检查当前时间戳与起始时间戳的差值
> 3. 当差值达到所需延时对应的CPU周期数时退出循环
> 
> 由于不依赖中断，即使在关中断状态下也能正常工作。

## API 说明

### DWT_Init

```c
void DWT_Init(uint32_t CPU_Freq_mHz)
```

> 初始化DWT模块，设置CPU频率参数。
> 
> 参数：
> 
> - `CPU_Freq_mHz`：CPU频率，单位为MHz

### DWT_GetDeltaT

```c
float DWT_GetDeltaT(uint32_t *cnt_last)
```

> 获取两次调用之间的时间间隔，单位为秒。
> 
> 参数：
> 
> - `cnt_last`：上一次调用的时间戳指针
> 
> 返回值：
> 
> - 两次调用之间的时间间隔，单位为秒

### DWT_GetDeltaT64

```c
double DWT_GetDeltaT64(uint32_t *cnt_last)
```

> 获取两次调用之间的时间间隔，单位为秒，使用double类型提供更高精度。
> 
> 参数：
> 
> - `cnt_last`：上一次调用的时间戳指针
> 
> 返回值：
> 
> - 两次调用之间的时间间隔，单位为秒

### DWT_GetTimeline_s

```c
float DWT_GetTimeline_s(void)
```

> 获取当前系统时间，单位为秒。
> 
> 返回值：
> 
> - 当前系统时间，单位为秒

### DWT_GetTimeline_ms

```c
float DWT_GetTimeline_ms(void)
```

> 获取当前系统时间，单位为毫秒。
> 
> 返回值：
> 
> - 当前系统时间，单位为毫秒

### DWT_GetTimeline_us

```c
uint64_t DWT_GetTimeline_us(void)
```

> 获取当前系统时间，单位为微秒。
> 
> 返回值：
> 
> - 当前系统时间，单位为微秒

### DWT_Delay

```c
void DWT_Delay(float Delay)
```

> 实现精确延时功能，不受中断状态影响。
> 
> 参数：
> 
> - `Delay`：延时时间，单位为秒

### DWT_SysTimeUpdate

```c
void DWT_SysTimeUpdate(void)
```

> 更新系统时间轴，此函数会被时间轴获取函数自动调用，一般不需要手动调用。



## 使用示例

```c
// 初始化DWT，c板CPU频率为168MHz
DWT_Init(168);


// 测量某段代码执行时间
uint32_t last_time = 0;
float execution_time;

// 开始测量
last_time = DWT->CYCCNT;

// 执行需要测量的代码
do_something();

// 获取执行时间
execution_time = DWT_GetDeltaT(&last_time);
printf("Execution time: %f seconds\n", execution_time);
```

## 注意事项

1. **溢出处理**：DWT模块内部处理了CYCCNT计数器的溢出问题，用户无需关心溢出处理。

2. **中断安全**：DWT延时函数`DWT_Delay`不受中断状态影响，可在关闭中断时使用，而`HAL_Delay`在此情况下无法工作。

3. **精度**：DWT提供基于CPU时钟的高精度时间测量，精度可达CPU时钟周期。

4. **定期更新**：如果长时间不调用时间轴获取函数，需要手动调用DWT_SysTimeUpdate()以确保时间轴的准确性。

5. **CPU频率设置**：初始化时必须正确设置CPU频率，否则时间计算会出现错误。
