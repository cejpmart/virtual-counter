cmake_minimum_required(VERSION 3.6)
project(lib-virtual-instrument C ASM)

#set(TARGET_CPU "" CACHE STRING "Target CPU")
#
#if (TARGET_CPU STREQUAL "STM32F042F6")
#	add_definitions(-DSTM32F042F6)
#	add_definitions(-DSTM32F042x6)
#elseif (TARGET_CPU STREQUAL "STM32F042K6")
#	add_definitions(-DSTM32F042x6)
#else()
#	message(FATAL_ERROR "Invalid or undefined TARGET_CPU")
#endif()

file(GLOB_RECURSE USER_SOURCES "src/*.c")

add_library(virtual-instrument ${USER_SOURCES})

target_include_directories(virtual-instrument PUBLIC include)
target_include_directories(virtual-instrument PRIVATE src)
