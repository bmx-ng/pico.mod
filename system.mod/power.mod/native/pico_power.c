#include <stdint.h>

#include "blitzmax/pico_runtime.h"
#include "hardware/sync.h"
#include "pico/low_power.h"
#include "pico/stdlib.h"

uint32_t bmx_pico_power_capabilities(void) {
    uint32_t capabilities = 1u | 2u | 4u | 8u | 32u;
#if !PICO_RP2040
    capabilities |= 16u;
#endif
    return capabilities;
}

void bmx_pico_power_idle(void) {
    __wfi();
}

int32_t bmx_pico_power_sleep_until_interrupt(void) {
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    return low_power_sleep_until_irq(NULL);
}

int32_t bmx_pico_power_sleep_for_ms(uint32_t milliseconds, int32_t exclusive) {
    if (!milliseconds) return PICO_ERROR_INVALID_ARG;
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    return low_power_sleep_for_ms(milliseconds, NULL, exclusive != 0);
}

int32_t bmx_pico_power_dormant_for_ms(uint32_t milliseconds) {
#if !PICO_RP2040
    if (!milliseconds) return PICO_ERROR_INVALID_ARG;
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    int32_t result = low_power_dormant_for_ms(milliseconds,
        DORMANT_CLOCK_SOURCE_DEFAULT, NULL);
    /* SDK 2.3.0 leaves the POWMAN wake condition armed after a timed
       dormant period. Clear it so a later GPIO dormant really waits for GPIO. */
    if (result == PICO_OK) powman_disable_alarm_wakeup();
    return result;
#else
    (void)milliseconds;
    return PICO_ERROR_PRECONDITION_NOT_MET;
#endif
}

int32_t bmx_pico_power_dormant_until_gpio(uint32_t gpio, int32_t edge, int32_t high) {
    if (gpio >= NUM_BANK0_GPIOS) return PICO_ERROR_INVALID_ARG;
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    return low_power_dormant_until_gpio_pin_state(gpio, edge != 0, high != 0,
        DORMANT_CLOCK_SOURCE_ROSC, NULL);
}

int32_t bmx_pico_power_set_unused_pins_low_leakage(uint64_t exclude_mask) {
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    low_power_set_pins_low_leakage_exclude_mask64(exclude_mask);
    return PICO_OK;
}
