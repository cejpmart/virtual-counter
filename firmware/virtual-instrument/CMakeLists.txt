cmake_minimum_required(VERSION 3.6)
project(virtual-instrument C ASM)

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

add_library(${PROJECT_NAME} ${USER_SOURCES})

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wno-unused -Werror)

target_include_directories(${PROJECT_NAME} PUBLIC include)
target_include_directories(${PROJECT_NAME} PRIVATE src)
