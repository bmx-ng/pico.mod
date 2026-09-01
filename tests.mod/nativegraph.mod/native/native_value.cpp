#include "native_value.h"

#ifndef BMX_PICO_NATIVE_COMMON
#error "Module CC_OPTS were not applied to the imported C++ source"
#endif

#ifndef BMX_PICO_NATIVE_CPP_ONLY
#error "Module CPP_OPTS were not applied to the imported C++ source"
#endif

#ifndef BMX_PICO_NATIVE_LEXICAL
#error "Lexical compiler options were not applied to the imported C++ source"
#endif

extern "C" int32_t bmx_pico_native_cpp_value(int32_t value) {
    return value * 2 + BMX_PICO_NATIVE_COMMON + BMX_PICO_NATIVE_CPP_ONLY + BMX_PICO_NATIVE_LEXICAL + BMX_PICO_NATIVE_HEADER_VALUE;
}
