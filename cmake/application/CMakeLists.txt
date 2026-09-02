cmake_minimum_required(VERSION 3.13)

foreach(required_variable BLITZMAX_PICO_SDK BLITZMAX_APPLICATION_ROOT BLITZMAX_GENERATED_C BLITZMAX_OUTPUT_NAME BLITZMAX_PICO_ARENA_SIZE)
    if (NOT DEFINED ${required_variable})
        message(FATAL_ERROR "${required_variable} must be supplied by bmk")
    endif()
endforeach()

if (NOT DEFINED PICO_BOARD)
    set(PICO_BOARD pico2 CACHE STRING "Pico SDK board definition")
endif()

include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(blitzmax_pico_application C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

if (PICO_PLATFORM STREQUAL "rp2040")
    math(EXPR BLITZMAX_PICO_RAM_BYTES "256 * 1024")
    math(EXPR BLITZMAX_PICO_AUTO_ARENA_SIZE "192 * 1024")
elseif (PICO_PLATFORM MATCHES "^rp2350-arm-")
    math(EXPR BLITZMAX_PICO_RAM_BYTES "512 * 1024")
    math(EXPR BLITZMAX_PICO_AUTO_ARENA_SIZE "384 * 1024")
else()
    message(FATAL_ERROR
        "The BlitzMax Pico target requires an ARM RP2040 or RP2350 board; "
        "PICO_BOARD='${PICO_BOARD}' selected PICO_PLATFORM='${PICO_PLATFORM}'")
endif()

if (NOT DEFINED PICO_FLASH_SIZE_BYTES)
    message(FATAL_ERROR
        "Pico SDK board '${PICO_BOARD}' does not define PICO_FLASH_SIZE_BYTES")
endif()
math(EXPR BLITZMAX_PICO_FLASH_BYTES "${PICO_FLASH_SIZE_BYTES}")

if (BLITZMAX_PICO_ARENA_SIZE STREQUAL "auto")
    set(BLITZMAX_PICO_RESOLVED_ARENA_SIZE ${BLITZMAX_PICO_AUTO_ARENA_SIZE})
else()
    math(EXPR BLITZMAX_PICO_RESOLVED_ARENA_SIZE "${BLITZMAX_PICO_ARENA_SIZE}")
endif()
if (BLITZMAX_PICO_RESOLVED_ARENA_SIZE LESS 1024)
    message(FATAL_ERROR "The BlitzMax Pico managed heap must be at least 1 KiB")
endif()
if (BLITZMAX_PICO_RESOLVED_ARENA_SIZE GREATER_EQUAL BLITZMAX_PICO_RAM_BYTES)
    message(FATAL_ERROR
        "The BlitzMax Pico managed heap must leave room for application and SDK RAM")
endif()

set(BLITZMAX_PICO_RAM_BYTES ${BLITZMAX_PICO_RAM_BYTES}
    CACHE INTERNAL "BlitzMax Pico internal SRAM budget" FORCE)
set(BLITZMAX_PICO_FLASH_BYTES ${BLITZMAX_PICO_FLASH_BYTES}
    CACHE INTERNAL "BlitzMax Pico board flash capacity" FORCE)
set(BLITZMAX_PICO_RESOLVED_ARENA_SIZE ${BLITZMAX_PICO_RESOLVED_ARENA_SIZE}
    CACHE INTERNAL "Resolved BlitzMax Pico managed heap size" FORCE)

set(PICO_MODULE_ROOT "${BLITZMAX_PICO_SDK}/mod/pico.mod")
set(BRL_BLITZ_ROOT "${BLITZMAX_PICO_SDK}/mod/brl.mod/blitz.mod")
set(GENERATED_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(GENERATED_BOOTSTRAP_C "${GENERATED_DIRECTORY}/blitzmax_pico_modules.c")

file(MAKE_DIRECTORY "${GENERATED_DIRECTORY}")
file(WRITE "${GENERATED_BOOTSTRAP_C}" "/* Generated Pico module initialization order. */\n")
foreach(module_initializer IN LISTS BLITZMAX_PICO_MODULE_INITIALIZERS)
    file(APPEND "${GENERATED_BOOTSTRAP_C}" "extern int ${module_initializer}(void);\n")
endforeach()
file(APPEND "${GENERATED_BOOTSTRAP_C}" "void bmx_pico_modules_init(void) {\n")
foreach(module_initializer IN LISTS BLITZMAX_PICO_MODULE_INITIALIZERS)
    file(APPEND "${GENERATED_BOOTSTRAP_C}" "    (void)${module_initializer}();\n")
endforeach()
file(APPEND "${GENERATED_BOOTSTRAP_C}" "}\n")

if (DEFINED BLITZMAX_PICO_NATIVE_CMAKE)
    include("${BLITZMAX_PICO_NATIVE_CMAKE}")
endif()

set_source_files_properties(${BLITZMAX_GENERATED_C} ${BLITZMAX_PICO_GENERATED_SOURCES} "${GENERATED_BOOTSTRAP_C}" PROPERTIES GENERATED TRUE)

add_executable(${BLITZMAX_OUTPUT_NAME}
    ${BLITZMAX_GENERATED_C}
    ${BLITZMAX_PICO_GENERATED_SOURCES}
    ${BLITZMAX_PICO_NATIVE_SOURCES}
    "${GENERATED_BOOTSTRAP_C}"
    "${PICO_MODULE_ROOT}/runtime.mod/native/pico_runtime.c"
    "${PICO_MODULE_ROOT}/runtime.mod/native/pico_string_integer.c"
    "${PICO_MODULE_ROOT}/runtime.mod/native/pico_string_float.cpp"
    "${PICO_MODULE_ROOT}/runtime.mod/native/pico_unicode.c"
    "${BRL_BLITZ_ROOT}/ryu/ryu/f2s.c"
    "${BRL_BLITZ_ROOT}/ryu/ryu/d2s.c"
)

if (DEFINED BLITZMAX_PICO_PIO_CMAKE)
    include("${BLITZMAX_PICO_PIO_CMAKE}")
endif()

target_link_libraries(${BLITZMAX_OUTPUT_NAME} pico_stdlib pico_rand pico_unique_id hardware_adc hardware_dma hardware_i2c hardware_pio hardware_pwm hardware_spi hardware_uart hardware_watchdog)
if (BLITZMAX_PICO_NATIVE_LINK_OPTIONS)
    target_link_options(${BLITZMAX_OUTPUT_NAME} PRIVATE ${BLITZMAX_PICO_NATIVE_LINK_OPTIONS})
endif()
target_compile_definitions(${BLITZMAX_OUTPUT_NAME} PRIVATE
    RYU_OPTIMIZE_SIZE=1
    BMX_PICO_ARENA_SIZE=${BLITZMAX_PICO_RESOLVED_ARENA_SIZE}u
)
target_include_directories(${BLITZMAX_OUTPUT_NAME} PRIVATE
    "${PICO_MODULE_ROOT}/runtime.mod/native/include"
    "${BLITZMAX_PICO_SDK}/mod"
    "${BLITZMAX_APPLICATION_ROOT}"
    "${BRL_BLITZ_ROOT}"
    "${BRL_BLITZ_ROOT}/ryu"
)
pico_enable_stdio_uart(${BLITZMAX_OUTPUT_NAME} 1)
pico_enable_stdio_usb(${BLITZMAX_OUTPUT_NAME} 1)
pico_add_extra_outputs(${BLITZMAX_OUTPUT_NAME})
