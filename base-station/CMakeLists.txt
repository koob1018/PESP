cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(base_station)

target_sources(app PRIVATE
    src/main.c
    src/driver/sensor_node_driver.c
)

target_include_directories(app PRIVATE src)
