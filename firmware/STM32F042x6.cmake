include (CMakeForceCompiler)
SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_SYSTEM_VERSION 1)

SET(ARM_NONE_EABI_GCC_BIN /opt/gcc-arm-none-eabi-6-2017-q1-update/bin)

# specify the cross compiler
SET(CMAKE_C_COMPILER_WORKS 1)
SET(CMAKE_C_COMPILER ${ARM_NONE_EABI_GCC_BIN}/arm-none-eabi-gcc)
SET(CMAKE_CXX_COMPILER ${ARM_NONE_EABI_GCC_BIN}/arm-none-eabi-g++)
set(CMAKE_SIZE ${ARM_NONE_EABI_GCC_BIN}/arm-none-eabi-size)

SET(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/STM32F042K6Tx_FLASH.ld)
SET(COMMON_FLAGS "-mcpu=cortex-m0 -mthumb -mfloat-abi=soft -ffunction-sections -fdata-sections -Og -g3 -fno-common -fmessage-length=0 -Wall")
SET(CMAKE_CXX_FLAGS "${COMMON_FLAGS} -std=c++11")
SET(CMAKE_C_FLAGS "${COMMON_FLAGS} -std=gnu99")
SET(CMAKE_EXE_LINKER_FLAGS "-specs=nosys.specs -specs=nano.specs -Wl,-gc-sections -T ${LINKER_SCRIPT}")
