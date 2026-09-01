cmake_minimum_required(VERSION 3.13)

if (NOT DEFINED ENV{PICO_SDK_PATH})
    message(FATAL_ERROR "PICO_SDK_PATH must point to a Raspberry Pi Pico SDK checkout")
endif()

set(PICO_BOARD pico2 CACHE STRING "Pico SDK board definition")
set(PICO_PLATFORM rp2350-arm-s CACHE STRING "Pico SDK platform")

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(pico2_baseline C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(pico2_baseline
    main.c
)

pico_set_program_name(pico2_baseline "BlitzMax Pico 2 baseline")
pico_set_program_version(pico2_baseline "0.1")

target_link_libraries(pico2_baseline
    pico_stdlib
)

pico_enable_stdio_uart(pico2_baseline 1)
pico_enable_stdio_usb(pico2_baseline 1)
pico_add_extra_outputs(pico2_baseline)
