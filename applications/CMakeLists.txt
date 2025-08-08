# 创建 applications 静态库
add_library(applications STATIC
    compoent_config.c
    robot_init.c
    robot_control/robot_control.c
    robot_control/robot_task.c
    gimbal/gimbalcmd.c
    shoot/shootcmd.c
)

# 设置包含目录
target_include_directories(applications
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/robot_control
        ${CMAKE_CURRENT_SOURCE_DIR}/gimbal
        ${CMAKE_CURRENT_SOURCE_DIR}/shoot
)

# 链接必要的库
target_link_libraries(applications stm32cubemx ThreadX tools modules bsp)

# 将 applications 添加到主项目
target_link_libraries(${PROJECT_NAME} applications)
