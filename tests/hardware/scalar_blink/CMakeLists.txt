cmake_minimum_required(VERSION 3.13)

if (NOT DEFINED ENV{PICO_SDK_PATH})
    message(FATAL_ERROR "PICO_SDK_PATH must point to a Raspberry Pi Pico SDK checkout")
endif()

set(PICO_BOARD pico2 CACHE STRING "Pico SDK board definition")
set(PICO_PLATFORM rp2350-arm-s CACHE STRING "Pico SDK platform")

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(pico_scalar_blink C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

get_filename_component(
    BLITZMAX_PICO_SDK_DEFAULT
    "${CMAKE_CURRENT_LIST_DIR}/../../../../.."
    ABSOLUTE
)
set(
    BLITZMAX_PICO_SDK
    "${BLITZMAX_PICO_SDK_DEFAULT}"
    CACHE PATH
    "BlitzMax SDK containing the Pico-enabled bcc2 executable"
)
set(BCC_EXECUTABLE "${BLITZMAX_PICO_SDK}/bin/bcc")

if (NOT EXISTS "${BCC_EXECUTABLE}")
    message(FATAL_ERROR "Pico-enabled bcc2 was not found at ${BCC_EXECUTABLE}")
endif()

set(GENERATED_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(GENERATED_C "${GENERATED_DIRECTORY}/scalar_blink.c")

add_custom_command(
    OUTPUT "${GENERATED_C}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${GENERATED_DIRECTORY}"
    COMMAND
        "${BCC_EXECUTABLE}"
        --emit-c
        --sdk "${BLITZMAX_PICO_SDK}"
        --platform pico
        --arch arm
        --release
        --single-threaded
        --no-runtime
        -o "${GENERATED_C}"
        "${CMAKE_CURRENT_LIST_DIR}/scalar_blink.bmx"
    DEPENDS
        "${CMAKE_CURRENT_LIST_DIR}/scalar_blink.bmx"
        "${BCC_EXECUTABLE}"
    COMMENT "Generating scalar Pico C with bcc2"
    VERBATIM
)

set_source_files_properties("${GENERATED_C}" PROPERTIES GENERATED TRUE)

add_executable(pico_scalar_blink
    "${GENERATED_C}"
    ../../../runtime.mod/native/pico_runtime.c
)

pico_set_program_name(pico_scalar_blink "BlitzMax scalar blink")
pico_set_program_version(pico_scalar_blink "0.1")

target_link_libraries(pico_scalar_blink
    pico_stdlib
)

target_include_directories(pico_scalar_blink PRIVATE
    ../../../runtime.mod/native/include
)

pico_enable_stdio_uart(pico_scalar_blink 1)
pico_enable_stdio_usb(pico_scalar_blink 1)
pico_add_extra_outputs(pico_scalar_blink)
