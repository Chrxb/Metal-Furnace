add_library(dfrobot_max31855
        dfrobot_max31855.cpp)


target_link_libraries(dfrobot_max31855
        hardware_i2c
        pico_stdlib
        )
target_include_directories (dfrobot_max31855 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})