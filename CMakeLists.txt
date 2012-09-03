cmake_minimum_required(VERSION 2.8)
project(flash32w)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})
find_package(libusb-1.0 REQUIRED)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall" )

include_directories(include ${LIBUSB_1_INCLUDE_DIR})

add_executable(flash32w main.c stm32w.c stm32f.c stm32f_usb.c flash32w.h)
target_link_libraries(flash32w ${LIBUSB_1_LIBRARY})

