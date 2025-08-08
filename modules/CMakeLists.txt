# 创建 modules 静态库
add_library(modules STATIC
        RGB/RGB.c 
        algorithm/kalman_filter.c
        algorithm/controller.c 
        algorithm/LQR.c 
        algorithm/QuaternionEKF.c 
        algorithm/user_lib.c 
        BMI088/BMI088.c
        systemwatch/systemwatch.c
        BEEP/beep.c
        OFFLINE/offline.c
        IMU/imu.c
        IMU/imu_task.c
        referee/referee.c
        referee/crc_rm.c
        referee/referee_UI.c
        REMOTE/remote.c
        REMOTE/dt7.c
        REMOTE/sbus.c
        REMOTE/vt03.c
        REMOTE/vt02.c
        board_com/board_com.c
        SubPub/subpub.c
        DM_IMU/dm_imu.c
        MOTOR/motor_task.c
        MOTOR/DJI/dji.c
)

# 设置包含目录
target_include_directories(modules
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/RGB
        ${CMAKE_CURRENT_SOURCE_DIR}/algorithm
        ${CMAKE_CURRENT_SOURCE_DIR}/BMI088
        ${CMAKE_CURRENT_SOURCE_DIR}/systemwatch
        ${CMAKE_CURRENT_SOURCE_DIR}/BEEP
        ${CMAKE_CURRENT_SOURCE_DIR}/OFFLINE
        ${CMAKE_CURRENT_SOURCE_DIR}/IMU
        ${CMAKE_CURRENT_SOURCE_DIR}/referee
        ${CMAKE_CURRENT_SOURCE_DIR}/REMOTE
        ${CMAKE_CURRENT_SOURCE_DIR}/board_com
        ${CMAKE_CURRENT_SOURCE_DIR}/SubPub
        ${CMAKE_CURRENT_SOURCE_DIR}/DM_IMU
        ${CMAKE_CURRENT_SOURCE_DIR}/MOTOR
        ${CMAKE_CURRENT_SOURCE_DIR}/MOTOR/DJI
)

# 链接必要的库
target_link_libraries(modules stm32cubemx ThreadX tools bsp applications)
target_link_libraries(modules -l:libarm_cortexM4lf_math.a) #链接dsp
target_link_libraries(modules m) #链接数学库

# 将 modules 添加到主项目
target_link_libraries(${PROJECT_NAME} modules)