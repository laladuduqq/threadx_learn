# 创建 tools 静态库
add_library(tools STATIC
    ulog/ulog.c
    ulog/ulog_port.c
)

# 设置包含目录
target_include_directories(tools
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/ulog
)

# 链接必要的库
target_link_libraries(tools stm32cubemx ThreadX applications bsp)

# 将 tools 添加到主项目
target_link_libraries(${PROJECT_NAME} tools)
