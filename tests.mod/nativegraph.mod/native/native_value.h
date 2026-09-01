#ifndef BMX_PICO_NATIVE_VALUE_H
#define BMX_PICO_NATIVE_VALUE_H

#include <stdint.h>

#define BMX_PICO_NATIVE_HEADER_VALUE 3

#ifdef __cplusplus
extern "C" {
#endif

int32_t bmx_pico_native_c_value(int32_t value);
int32_t bmx_pico_native_cpp_value(int32_t value);

#ifdef __cplusplus
}
#endif

#endif
