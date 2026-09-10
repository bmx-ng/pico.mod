cmake_minimum_required(VERSION 3.13)

foreach(required_variable BLITZMAX_PICO_SDK BLITZMAX_APPLICATION_ROOT BLITZMAX_GENERATED_C BLITZMAX_OUTPUT_NAME BLITZMAX_PICO_ARENA_SIZE BLITZMAX_PICO_ARENA_REGION BLITZMAX_PICO_STORAGE_SIZE BLITZMAX_PICO_FLOAT_ABI)
    if (NOT DEFINED ${required_variable})
        message(FATAL_ERROR "${required_variable} must be supplied by bmk")
    endif()
endforeach()

if (NOT DEFINED PICO_BOARD)
    set(PICO_BOARD pico2 CACHE STRING "Pico SDK board definition")
endif()

include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

if (NOT DEFINED PICO_SDK_VERSION_STRING OR
        PICO_SDK_VERSION_STRING VERSION_LESS "2.3.0")
    message(FATAL_ERROR
        "The BlitzMax Pico target requires Raspberry Pi Pico SDK 2.3.0 or newer; "
        "selected SDK version is '${PICO_SDK_VERSION_STRING}'")
endif()

if (BLITZMAX_PICO_FLOAT_ABI STREQUAL "hard")
    if (NOT PICO_PLATFORM MATCHES "^rp2350-arm-")
        message(FATAL_ERROR
            "The hard-float ABI is supported only by ARM RP2350 boards; "
            "PICO_BOARD='${PICO_BOARD}' selected PICO_PLATFORM='${PICO_PLATFORM}'")
    endif()
    set(PICO_HARD_FLOAT_ABI 1)
elseif (BLITZMAX_PICO_FLOAT_ABI STREQUAL "auto")
    set(PICO_HARD_FLOAT_ABI 0)
else()
    message(FATAL_ERROR
        "BLITZMAX_PICO_FLOAT_ABI must be 'auto' or 'hard'")
endif()

project(blitzmax_pico_application C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

list(FIND BLITZMAX_PICO_MODULE_INITIALIZERS "__bb_pico_system_power_power" BLITZMAX_PICO_POWER_MODULE_INDEX)
set(BLITZMAX_PICO_POWER_MODULE_ENABLED FALSE)
if (NOT BLITZMAX_PICO_POWER_MODULE_INDEX EQUAL -1)
    if (NOT TARGET pico_low_power)
        message(FATAL_ERROR
            "Pico.System.Power requires the Pico SDK pico_low_power library")
    endif()
    set(BLITZMAX_PICO_POWER_MODULE_ENABLED TRUE)
    set(BLITZMAX_PICO_POWER_SOURCE
        "${BLITZMAX_PICO_SDK}/mod/pico.mod/system.mod/power.mod/native/pico_power.c")
endif()

list(FIND BLITZMAX_PICO_MODULE_INITIALIZERS "__bb_pico_hardware_psram_psram" BLITZMAX_PICO_PSRAM_MODULE_INDEX)
set(BLITZMAX_PICO_PSRAM_MODULE_ENABLED FALSE)
if (NOT BLITZMAX_PICO_PSRAM_MODULE_INDEX EQUAL -1)
    set(BLITZMAX_PICO_PSRAM_MODULE_ENABLED TRUE)
    set(BLITZMAX_PICO_PSRAM_SOURCE
        "${BLITZMAX_PICO_SDK}/mod/pico.mod/hardware.mod/psram.mod/native/pico_psram.c")
endif()

list(FIND BLITZMAX_PICO_MODULE_INITIALIZERS "__bb_pico_network_wifi_wifi" BLITZMAX_PICO_WIFI_MODULE_INDEX)
list(FIND BLITZMAX_PICO_MODULE_INITIALIZERS "__bb_pub_net_net" BLITZMAX_PICO_NET_MODULE_INDEX)
set(BLITZMAX_PICO_WIFI_MODULE_ENABLED FALSE)
set(BLITZMAX_PICO_NETWORK_STACK_ENABLED FALSE)
if (NOT BLITZMAX_PICO_WIFI_MODULE_INDEX EQUAL -1 OR
        NOT BLITZMAX_PICO_NET_MODULE_INDEX EQUAL -1)
    if (NOT PICO_CYW43_SUPPORTED)
        message(FATAL_ERROR
            "Pico Wi-Fi and socket networking require a Pico SDK board definition with CYW43 wireless support; "
            "PICO_BOARD='${PICO_BOARD}' does not provide it")
    endif()
    if (NOT TARGET pico_cyw43_arch_lwip_threadsafe_background)
        message(FATAL_ERROR
            "Pico networking requires the Pico SDK thread-safe background lwIP library")
    endif()
    set(BLITZMAX_PICO_NETWORK_STACK_ENABLED TRUE)
endif()
if (NOT BLITZMAX_PICO_WIFI_MODULE_INDEX EQUAL -1)
    set(BLITZMAX_PICO_WIFI_MODULE_ENABLED TRUE)
    set(BLITZMAX_PICO_WIFI_SOURCE
        "${BLITZMAX_PICO_SDK}/mod/pico.mod/network.mod/wifi.mod/native/pico_wifi.c")
endif()

if (PICO_PLATFORM STREQUAL "rp2040")
    set(BLITZMAX_PICO_RESOLVED_FLOAT_ABI "soft")
    math(EXPR BLITZMAX_PICO_RAM_BYTES "256 * 1024")
    math(EXPR BLITZMAX_PICO_AUTO_ARENA_SIZE "192 * 1024")
elseif (PICO_PLATFORM MATCHES "^rp2350-arm-")
    if (BLITZMAX_PICO_FLOAT_ABI STREQUAL "hard")
        set(BLITZMAX_PICO_RESOLVED_FLOAT_ABI "hard")
    else()
        set(BLITZMAX_PICO_RESOLVED_FLOAT_ABI "softfp")
    endif()
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

set(BLITZMAX_PICO_PSRAM_BYTES 0)
if (DEFINED PICO_PSRAM_SIZE_BYTES)
    math(EXPR BLITZMAX_PICO_PSRAM_BYTES "${PICO_PSRAM_SIZE_BYTES}")
endif()

set(BLITZMAX_PICO_RESOLVED_ARENA_REGION "${BLITZMAX_PICO_ARENA_REGION}")
set(BLITZMAX_PICO_PSRAM_RESERVED_BYTES 0)
set(BLITZMAX_PICO_ARENA_IN_PSRAM 0)
if (BLITZMAX_PICO_RESOLVED_ARENA_REGION STREQUAL "psram")
    set(BLITZMAX_PICO_ARENA_IN_PSRAM 1)
    math(EXPR BLITZMAX_PICO_PSRAM_RESERVED_BYTES "64 * 1024")
    if (BLITZMAX_PICO_ARENA_SIZE STREQUAL "auto")
        if (BLITZMAX_PICO_PSRAM_BYTES GREATER BLITZMAX_PICO_PSRAM_RESERVED_BYTES)
            math(EXPR BLITZMAX_PICO_RESOLVED_ARENA_SIZE
                "${BLITZMAX_PICO_PSRAM_BYTES} - ${BLITZMAX_PICO_PSRAM_RESERVED_BYTES}")
        else()
            set(BLITZMAX_PICO_RESOLVED_ARENA_SIZE 0)
        endif()
    else()
        math(EXPR BLITZMAX_PICO_RESOLVED_ARENA_SIZE "${BLITZMAX_PICO_ARENA_SIZE}")
        math(EXPR BLITZMAX_PICO_PSRAM_RESERVED_BYTES
            "${BLITZMAX_PICO_PSRAM_BYTES} - ${BLITZMAX_PICO_RESOLVED_ARENA_SIZE}")
        if (BLITZMAX_PICO_PSRAM_RESERVED_BYTES LESS 0)
            set(BLITZMAX_PICO_PSRAM_RESERVED_BYTES 0)
        endif()
    endif()
endif()

math(EXPR BLITZMAX_PICO_STORAGE_BYTES "${BLITZMAX_PICO_STORAGE_SIZE}")
if (BLITZMAX_PICO_STORAGE_BYTES LESS 0)
    message(FATAL_ERROR "The BlitzMax Pico persistent storage size cannot be negative")
endif()
if (BLITZMAX_PICO_STORAGE_BYTES GREATER 0)
    math(EXPR _blitzmax_storage_remainder "${BLITZMAX_PICO_STORAGE_BYTES} % 4096")
    if (NOT _blitzmax_storage_remainder EQUAL 0)
        message(FATAL_ERROR "The BlitzMax Pico persistent storage size must be a multiple of 4096 bytes")
    endif()
    if (BLITZMAX_PICO_STORAGE_BYTES LESS 8192)
        message(FATAL_ERROR "The BlitzMax Pico persistent storage region must contain at least two flash sectors")
    endif()
endif()

# RP2350 A2-compatible builds keep the last sector unused when persistent
# storage is enabled. This follows the Pico SDK's own flash-bank placement and
# leaves room for the RP2350-E10 workaround without shifting the filesystem.
set(BLITZMAX_PICO_TAIL_RESERVED_BYTES 0)
if (BLITZMAX_PICO_STORAGE_BYTES GREATER 0 AND
        PICO_PLATFORM MATCHES "^rp2350-arm-" AND PICO_RP2350_A2_SUPPORTED)
    set(BLITZMAX_PICO_TAIL_RESERVED_BYTES 4096)
endif()
math(EXPR BLITZMAX_PICO_STORAGE_OFFSET
    "${BLITZMAX_PICO_FLASH_BYTES} - ${BLITZMAX_PICO_TAIL_RESERVED_BYTES} - ${BLITZMAX_PICO_STORAGE_BYTES}")
set(BLITZMAX_PICO_APPLICATION_FLASH_BYTES ${BLITZMAX_PICO_STORAGE_OFFSET})
if (BLITZMAX_PICO_APPLICATION_FLASH_BYTES LESS 65536)
    message(FATAL_ERROR
        "The requested Pico persistent storage leaves less than 64 KiB for the application")
endif()

# pico_standard_link has already generated this include using the physical
# flash capacity. Replace only the linker's FLASH region length: runtime flash
# APIs must continue to see the board's real PICO_FLASH_SIZE_BYTES value.
file(WRITE "${CMAKE_BINARY_DIR}/pico_flash_region.ld"
    "FLASH(rx) : ORIGIN = 0x10000000, LENGTH = ${BLITZMAX_PICO_APPLICATION_FLASH_BYTES}\n")

if (BLITZMAX_PICO_RESOLVED_ARENA_REGION STREQUAL "sram")
    if (BLITZMAX_PICO_ARENA_SIZE STREQUAL "auto")
        set(BLITZMAX_PICO_RESOLVED_ARENA_SIZE ${BLITZMAX_PICO_AUTO_ARENA_SIZE})
    else()
        math(EXPR BLITZMAX_PICO_RESOLVED_ARENA_SIZE "${BLITZMAX_PICO_ARENA_SIZE}")
    endif()
endif()
if (BLITZMAX_PICO_RESOLVED_ARENA_REGION STREQUAL "sram" AND BLITZMAX_PICO_RESOLVED_ARENA_SIZE LESS 1024)
    message(FATAL_ERROR "The BlitzMax Pico managed heap must be at least 1 KiB")
endif()
if (BLITZMAX_PICO_RESOLVED_ARENA_REGION STREQUAL "sram" AND BLITZMAX_PICO_RESOLVED_ARENA_SIZE GREATER_EQUAL BLITZMAX_PICO_RAM_BYTES)
    message(FATAL_ERROR
        "The BlitzMax Pico managed heap must leave room for application and SDK RAM")
endif()

set(BLITZMAX_PICO_RAM_BYTES ${BLITZMAX_PICO_RAM_BYTES}
    CACHE INTERNAL "BlitzMax Pico internal SRAM budget" FORCE)
set(BLITZMAX_PICO_RESOLVED_FLOAT_ABI ${BLITZMAX_PICO_RESOLVED_FLOAT_ABI}
    CACHE INTERNAL "Resolved BlitzMax Pico floating-point ABI" FORCE)
set(BLITZMAX_PICO_FLASH_BYTES ${BLITZMAX_PICO_FLASH_BYTES}
    CACHE INTERNAL "BlitzMax Pico board flash capacity" FORCE)
set(BLITZMAX_PICO_PSRAM_BYTES ${BLITZMAX_PICO_PSRAM_BYTES}
    CACHE INTERNAL "BlitzMax Pico board fixed PSRAM capacity" FORCE)
set(BLITZMAX_PICO_RESOLVED_ARENA_REGION ${BLITZMAX_PICO_RESOLVED_ARENA_REGION}
    CACHE INTERNAL "Resolved BlitzMax Pico managed heap region" FORCE)
set(BLITZMAX_PICO_PSRAM_RESERVED_BYTES ${BLITZMAX_PICO_PSRAM_RESERVED_BYTES}
    CACHE INTERNAL "PSRAM not occupied by the BlitzMax managed heap" FORCE)
set(BLITZMAX_PICO_APPLICATION_FLASH_BYTES ${BLITZMAX_PICO_APPLICATION_FLASH_BYTES}
    CACHE INTERNAL "BlitzMax Pico application flash capacity" FORCE)
set(BLITZMAX_PICO_STORAGE_OFFSET ${BLITZMAX_PICO_STORAGE_OFFSET}
    CACHE INTERNAL "BlitzMax Pico persistent storage flash offset" FORCE)
set(BLITZMAX_PICO_STORAGE_BYTES ${BLITZMAX_PICO_STORAGE_BYTES}
    CACHE INTERNAL "BlitzMax Pico persistent storage size" FORCE)
set(BLITZMAX_PICO_TAIL_RESERVED_BYTES ${BLITZMAX_PICO_TAIL_RESERVED_BYTES}
    CACHE INTERNAL "BlitzMax Pico protected flash tail size" FORCE)
set(BLITZMAX_PICO_RESOLVED_ARENA_SIZE ${BLITZMAX_PICO_RESOLVED_ARENA_SIZE}
    CACHE INTERNAL "Resolved BlitzMax Pico managed heap size" FORCE)

set(PICO_MODULE_ROOT "${BLITZMAX_PICO_SDK}/mod/pico.mod")
set(EMBEDDED_MODULE_ROOT "${BLITZMAX_PICO_SDK}/mod/embedded.mod")
set(BRL_BLITZ_ROOT "${BLITZMAX_PICO_SDK}/mod/brl.mod/blitz.mod")
set(GENERATED_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(GENERATED_BOOTSTRAP_C "${GENERATED_DIRECTORY}/blitzmax_pico_modules.c")

file(MAKE_DIRECTORY "${GENERATED_DIRECTORY}")
file(WRITE "${GENERATED_BOOTSTRAP_C}" "/* Generated Pico module initialization order. */\n")
foreach(module_initializer IN LISTS BLITZMAX_PICO_MODULE_INITIALIZERS)
    file(APPEND "${GENERATED_BOOTSTRAP_C}" "extern int ${module_initializer}(void);\n")
endforeach()
file(APPEND "${GENERATED_BOOTSTRAP_C}" "void bmx_embedded_modules_init(void) {\n")
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
    "${EMBEDDED_MODULE_ROOT}/runtime.mod/native/embedded_runtime.c"
    "${PICO_MODULE_ROOT}/runtime.mod/native/pico_runtime.c"
    "${EMBEDDED_MODULE_ROOT}/runtime.mod/native/embedded_string_integer.c"
    "${EMBEDDED_MODULE_ROOT}/runtime.mod/native/embedded_string_float.cpp"
    "${EMBEDDED_MODULE_ROOT}/runtime.mod/native/embedded_unicode.c"
    ${BLITZMAX_PICO_POWER_SOURCE}
    ${BLITZMAX_PICO_PSRAM_SOURCE}
    ${BLITZMAX_PICO_WIFI_SOURCE}
    "${BRL_BLITZ_ROOT}/ryu/ryu/f2s.c"
    "${BRL_BLITZ_ROOT}/ryu/ryu/d2s.c"
)

if (DEFINED BLITZMAX_PICO_PIO_CMAKE)
    include("${BLITZMAX_PICO_PIO_CMAKE}")
endif()

target_link_libraries(${BLITZMAX_OUTPUT_NAME} pico_stdlib pico_aon_timer pico_flash pico_rand pico_unique_id hardware_adc hardware_dma hardware_flash hardware_i2c hardware_pio hardware_pwm hardware_spi hardware_uart hardware_watchdog)
if (BLITZMAX_PICO_POWER_MODULE_ENABLED)
    target_link_libraries(${BLITZMAX_OUTPUT_NAME} pico_low_power)
endif()
if (BLITZMAX_PICO_NETWORK_STACK_ENABLED)
    target_link_libraries(${BLITZMAX_OUTPUT_NAME} pico_cyw43_arch_lwip_threadsafe_background)
    target_include_directories(${BLITZMAX_OUTPUT_NAME} PRIVATE
        "${BLITZMAX_PICO_SDK}/mod/pico.mod/network.mod/wifi.mod/native")
endif()
if ((BLITZMAX_PICO_PSRAM_MODULE_ENABLED OR BLITZMAX_PICO_ARENA_IN_PSRAM) AND
        NOT PICO_PLATFORM STREQUAL "rp2040")
    if (NOT TARGET hardware_psram)
        message(FATAL_ERROR "Pico PSRAM support requires the Pico SDK hardware_psram library")
    endif()
    target_link_libraries(${BLITZMAX_OUTPUT_NAME} hardware_psram)
endif()
if (BLITZMAX_PICO_NATIVE_LINK_OPTIONS)
    target_link_options(${BLITZMAX_OUTPUT_NAME} PRIVATE ${BLITZMAX_PICO_NATIVE_LINK_OPTIONS})
endif()
target_compile_definitions(${BLITZMAX_OUTPUT_NAME} PRIVATE
    RYU_OPTIMIZE_SIZE=1
    BMX_PICO_HARD_FLOAT_ABI=$<BOOL:${PICO_HARD_FLOAT_ABI}>
    BMX_EMBEDDED_ARENA_SIZE=${BLITZMAX_PICO_RESOLVED_ARENA_SIZE}u
    BMX_EMBEDDED_ARENA_IN_PSRAM=${BLITZMAX_PICO_ARENA_IN_PSRAM}
    BMX_PICO_FLASH_BYTES=${BLITZMAX_PICO_FLASH_BYTES}u
    BMX_PICO_STORAGE_OFFSET=${BLITZMAX_PICO_STORAGE_OFFSET}u
    BMX_PICO_STORAGE_SIZE=${BLITZMAX_PICO_STORAGE_BYTES}u
    BMX_PICO_TAIL_RESERVED_SIZE=${BLITZMAX_PICO_TAIL_RESERVED_BYTES}u
)
target_include_directories(${BLITZMAX_OUTPUT_NAME} PRIVATE
    "${EMBEDDED_MODULE_ROOT}/runtime.mod/native/include"
    "${PICO_MODULE_ROOT}/runtime.mod/native/include"
    "${BLITZMAX_PICO_SDK}/mod"
    "${BLITZMAX_APPLICATION_ROOT}"
    "${BRL_BLITZ_ROOT}"
    "${BRL_BLITZ_ROOT}/ryu"
)
pico_enable_stdio_uart(${BLITZMAX_OUTPUT_NAME} 1)
pico_enable_stdio_usb(${BLITZMAX_OUTPUT_NAME} 1)
pico_add_extra_outputs(${BLITZMAX_OUTPUT_NAME})
