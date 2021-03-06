cmake_minimum_required(VERSION 3.2)

set(TARGET_CPU "" CACHE STRING "Target CPU")
project(fw-${TARGET_CPU} C ASM)

if (TARGET_CPU STREQUAL "STM32F303RE")
	add_definitions(-DSTM32F303RE)
	add_definitions(-DSTM32F303xE)
else()
	message(FATAL_ERROR "Invalid or undefined TARGET_CPU")
endif()

add_subdirectory(../virtual-instrument ${CMAKE_BINARY_DIR}/build-virtual-instrument)

file(GLOB_RECURSE USER_SOURCES
		"Src/*.c"
		"../stm32-shared/*.c"
		)

file(GLOB_RECURSE HAL_SOURCES "Drivers/STM32F3xx_HAL_Driver/Src/*.c"
        )

include_directories(Inc)
include_directories(Drivers/STM32F3xx_HAL_Driver/Inc)
include_directories(Drivers/CMSIS/Include)
include_directories(Drivers/CMSIS/Device/ST/STM32F3xx/Include)

add_executable(${PROJECT_NAME}.elf
		${USER_SOURCES}
		${HAL_SOURCES}
		${LINKER_SCRIPT}
		startup/startup_stm32f303xe.s
		)

target_include_directories(${PROJECT_NAME}.elf PRIVATE
		../stm32-shared
		)

target_link_libraries(${PROJECT_NAME}.elf
		virtual-instrument
		)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Map=${CMAKE_BINARY_DIR}/${PROJECT_NAME}.map")

set(HEX_FILE ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin)
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
        COMMENT "Building ${HEX_FILE} \nBuilding ${BIN_FILE}")

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}.elf>
        )
