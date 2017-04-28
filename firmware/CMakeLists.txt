cmake_minimum_required(VERSION 3.6)
project(VirtualInstrument C ASM)

set(TARGET_CPU "" CACHE STRING "Target CPU")

if (TARGET_CPU STREQUAL "STM32F042F6")
	add_definitions(-DSTM32F042F6)
	add_definitions(-DSTM32F042x6)
elseif (TARGET_CPU STREQUAL "STM32F042K6")
	add_definitions(-DSTM32F042x6)
else()
	message(FATAL_ERROR "Invalid or undefined TARGET_CPU")
endif()

file(GLOB_RECURSE USER_SOURCES "Src/*.c")
file(GLOB_RECURSE HAL_SOURCES "Drivers/STM32F0xx_HAL_Driver/Src/*.c"
        "Middlewares/ST/STM32_USB_Device_Library/*.c")

add_library(CMSIS
        Drivers/CMSIS/Device/ST/STM32F0xx/Source/Templates/system_stm32f0xx.c
        Drivers/CMSIS/Device/ST/STM32F0xx/Source/Templates/gcc/startup_stm32f042x6.s)

include_directories(Inc)
include_directories(Drivers/STM32F0xx_HAL_Driver/Inc)
include_directories(Drivers/CMSIS/Include)
include_directories(Drivers/CMSIS/Device/ST/STM32F0xx/Include)
include_directories(Middlewares/ST/STM32_USB_Device_Library/Core/Inc)
include_directories(Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc)

add_executable(${PROJECT_NAME}.elf ${USER_SOURCES} ${HAL_SOURCES} ${LINKER_SCRIPT})

target_link_libraries(${PROJECT_NAME}.elf CMSIS)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Map=${PROJECT_SOURCE_DIR}/build/${PROJECT_NAME}.map")

set(HEX_FILE ${PROJECT_SOURCE_DIR}/build/${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_SOURCE_DIR}/build/${PROJECT_NAME}.bin)
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
        COMMENT "Building ${HEX_FILE} \nBuilding ${BIN_FILE}")

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}.elf>
        )

add_custom_target(flash
        COMMAND dfu-util -a 0 -D ${BIN_FILE} --dfuse-address 0x08000000
        COMMENT "Uploading ${BIN_FILE}")
