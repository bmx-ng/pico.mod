#include "native_value.h"

#ifndef BMX_PICO_NATIVE_COMMON
#error "Module CC_OPTS were not applied to the imported C source"
#endif

#ifndef BMX_PICO_NATIVE_C_ONLY
#error "Module C_OPTS were not applied to the imported C source"
#endif

int32_t bmx_pico_native_c_value(int32_t value) {
    return value + BMX_PICO_NATIVE_COMMON + BMX_PICO_NATIVE_C_ONLY + BMX_PICO_NATIVE_HEADER_VALUE;
}
