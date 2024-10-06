cmake_minimum_required(VERSION 3.13)
 
# initialize the SDK directly
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)
 
project(my_project)
 
# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# add_subdirectory(libs/zlib/zlib-1.3.1)
# add_subdirectory(libs/png/libpng-1.6.44)
add_subdirectory(app)

