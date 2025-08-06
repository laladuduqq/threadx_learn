# remote

这里是遥控和图传，自定义控制器(后续更新)

## sbus介绍：

[通信中常用的SBUS协议详解-云社区-华为云](https://bbs.huaweicloud.com/blogs/357019)

## dt7介绍：

[Robomaster大疆遥控器DT7DR16介绍与C型开发板通信教程_dt7遥控器-CSD博客](https://blog.csdn.net/2302_80641606/article/details/141711957)

## 图传：

### vt03：

[vt03图传](https://www.robomaster.com/zh-CN/products/components/detail/6137)

### vt02：

[vt02图传](https://www.robomaster.com/zh-CN/products/components/detail/2596)



## 使用说明：

在robot_config里配置使用遥控器

```c
//remote 在这里决定控制来源 注意键鼠控制部分 图传优先遥控器 uart部分c板只有 huart1/huart3/huart6
//遥控器选择 sbus 0 dt7 1 none 2
#define REMOTE_SOURCE    3
#define REMOTE_UART      huart3
//图传选择 vt02 0 vt03 1  
#define REMOTE_VT_SOURCE 1
#define REMOTE_VT_UART   huart6  
```

使用说明

> 在这里完善对应的遥控器控制，根据需要修改

```c
static void RemoteControlSet(Chassis_Ctrl_Cmd_s *Chassis_Ctrl,Shoot_Ctrl_Cmd_s *Shoot_Ctrl,Gimbal_Ctrl_Cmd_s *Gimbal_Ctrl)
```


