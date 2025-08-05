# 创建 modules 静态库
add_library(modules STATIC
        RGB/RGB.c 
        algorithm/kalman_filter.c
        algorithm/controller.c 
        algorithm/LQR.c 
        algorithm/QuaternionEKF.c 
        algorithm/user_lib.c 
)

# 设置包含目录
target_include_directories(modules
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/RGB
        ${CMAKE_CURRENT_SOURCE_DIR}/algorithm
)

# 链接必要的库
target_link_libraries(modules stm32cubemx ThreadX tools bsp applications)
target_link_libraries(modules -l:libarm_cortexM4lf_math.a) #链接dsp

# 将 modules 添加到主项目
target_link_libraries(${PROJECT_NAME} modules)