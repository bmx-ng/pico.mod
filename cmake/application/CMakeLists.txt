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

target_link_libraries(${BLITZMAX_OUTPUT_NAME} pico_stdlib hardware_adc hardware_dma hardware_i2c hardware_pio hardware_pwm hardware_spi hardware_uart)
if (BLITZMAX_PICO_NATIVE_LINK_OPTIONS)
    target_link_options(${BLITZMAX_OUTPUT_NAME} PRIVATE ${BLITZMAX_PICO_NATIVE_LINK_OPTIONS})
endif()
target_compile_definitions(${BLITZMAX_OUTPUT_NAME} PRIVATE
    RYU_OPTIMIZE_SIZE=1
    BMX_PICO_ARENA_SIZE=${BLITZMAX_PICO_ARENA_SIZE}u
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
