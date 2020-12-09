#!/bin/bash

if [[ $# -ne 2 ]] ; then
    echo 'usage: deploy_all.sh DESTDIR GUI_BUILD_ZIP'
    exit 1
fi

set -e

# TODO: auto-grab GUI build from AppVeyor ?
OUTDIR=$1
ZIPPED=$2
FW_DIR=${OUTDIR}/Firmware

ARM_NONE_EABI_GCC=/opt/gcc-arm-none-eabi-8-2018-q4-major

MAKE_FLAGS=-j5

rm -rf build-stm32f042f6-usb_cdc && mkdir build-stm32f042f6-usb_cdc && cd build-stm32f042f6-usb_cdc
cmake -DCMAKE_TOOLCHAIN_FILE=../firmware/fw-stm32f042/STM32F042x6.cmake -DARM_NONE_EABI_GCC_BIN=${ARM_NONE_EABI_GCC}/bin/ -DTARGET_CPU=STM32F042F6 -DCOMM_INTERFACE=USB_CDC ../firmware/fw-stm32f042
make ${MAKE_FLAGS} clean default_target
cd ..

rm -rf build-stm32f042k6-vcp && mkdir build-stm32f042k6-vcp && cd build-stm32f042k6-vcp
cmake -DCMAKE_TOOLCHAIN_FILE=../firmware/fw-stm32f042/STM32F042x6.cmake -DARM_NONE_EABI_GCC_BIN=${ARM_NONE_EABI_GCC}/bin/ -DTARGET_CPU=STM32F042K6 -DCOMM_INTERFACE=VCP ../firmware/fw-stm32f042
make ${MAKE_FLAGS} clean default_target
cd ..

rm -rf build-stm32f303re && mkdir build-stm32f303re && cd build-stm32f303re
cmake -DCMAKE_TOOLCHAIN_FILE=../firmware/fw-stm32f303/STM32F303xE.cmake -DARM_NONE_EABI_GCC_BIN=${ARM_NONE_EABI_GCC}/bin/ -DTARGET_CPU=STM32F303RE ../firmware/fw-stm32f303
make ${MAKE_FLAGS} clean default_target
cd ..

mkdir ${OUTDIR}

unzip ${ZIPPED} -d ${OUTDIR}

#cp doc/manual.pdf ${OUTDIR}/

mkdir ${FW_DIR}
cp build-stm32f042f6-usb_cdc/*.bin ${FW_DIR}
cp build-stm32f042f6-usb_cdc/*.elf ${FW_DIR}
cp build-stm32f042f6-usb_cdc/*.hex ${FW_DIR}

cp build-stm32f042k6-vcp/*.bin ${FW_DIR}
cp build-stm32f042k6-vcp/*.elf ${FW_DIR}
cp build-stm32f042k6-vcp/*.hex ${FW_DIR}

cp build-stm32f303re/*.bin ${FW_DIR}
cp build-stm32f303re/*.elf ${FW_DIR}
cp build-stm32f303re/*.hex ${FW_DIR}
