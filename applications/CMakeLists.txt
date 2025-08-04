# 创建 applications 静态库
add_library(applications STATIC
    compoent_config.c
)

# 设置包含目录
target_include_directories(applications
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 链接必要的库
target_link_libraries(applications stm32cubemx ThreadX tools)

# 将 applications 添加到主项目
target_link_libraries(${PROJECT_NAME} applications)
