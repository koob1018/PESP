cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(sensor_node C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(sensor_node
    src/main.c
    src/bme680_min.c
    src/veml7700_min.c
)

target_link_libraries(sensor_node
    pico_stdlib
)

# enable ADC hardware include/library for reading GPIO26/ADC0
target_link_libraries(sensor_node
    hardware_adc
)

# enable I2C hardware
target_link_libraries(sensor_node
    hardware_i2c
)

# TODO: Select USB/UART stdio according to the final board debug setup.
pico_enable_stdio_usb(sensor_node 1)
pico_enable_stdio_uart(sensor_node 0)

pico_add_extra_outputs(sensor_node)
