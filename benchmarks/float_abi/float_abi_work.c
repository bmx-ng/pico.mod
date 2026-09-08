/* Copyright (c) 2026 Bruce A Henderson and contributors
   SPDX-License-Identifier: Zlib */

#include <stdint.h>

#if defined(__GNUC__)
#define BMX_NOINLINE __attribute__((noinline))
#else
#define BMX_NOINLINE
#endif

const char *bmx_float_abi_name(void) {
#if BMX_PICO_HARD_FLOAT_ABI
    return "hard";
#else
    return "softfp";
#endif
}

BMX_NOINLINE uint32_t bmx_float_abi_integer_call4(
        uint32_t first, uint32_t second, uint32_t third, uint32_t fourth) {
    return first * 33u + second + third - fourth;
}

BMX_NOINLINE float bmx_float_abi_float_call4(
        float first, float second, float third, float fourth) {
    return first * 0.99991f + second * 0.00009f + third - fourth;
}

BMX_NOINLINE double bmx_float_abi_double_call4(
        double first, double second, double third, double fourth) {
    return first * 0.99991 + second * 0.00009 + third - fourth;
}
