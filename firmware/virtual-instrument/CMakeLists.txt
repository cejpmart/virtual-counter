cmake_minimum_required(VERSION 3.2)
project(virtual-instrument C ASM)

file(GLOB_RECURSE USER_SOURCES "src/*.c")

add_library(${PROJECT_NAME} ${USER_SOURCES})

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wno-unused -Werror)

target_include_directories(${PROJECT_NAME} PUBLIC include)
target_include_directories(${PROJECT_NAME} PRIVATE src)
