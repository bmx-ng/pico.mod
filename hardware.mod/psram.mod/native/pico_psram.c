#include <stdint.h>

#include "pico.h"

#if !PICO_RP2040
#include "hardware/psram.h"
#endif

int32_t bmx_pico_psram_available(void) {
#if PICO_RP2040
    return 0;
#else
    return psram_is_available() ? 1 : 0;
#endif
}

uint32_t bmx_pico_psram_capacity(void) {
#if PICO_RP2040
    return 0;
#else
    return (uint32_t)psram_get_size();
#endif
}

int32_t bmx_pico_psram_contains(void *address) {
#if PICO_RP2040
    (void)address;
    return 0;
#else
    return address && psram_check_address(address) ? 1 : 0;
#endif
}
