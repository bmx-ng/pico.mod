#include <stdbool.h>
#include <stdio.h>

#include "pico/stdlib.h"

#ifndef PICO_DEFAULT_LED_PIN
#error "The selected board does not define PICO_DEFAULT_LED_PIN"
#endif

int main(void) {
    stdio_init_all();

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    bool led_on = false;
    unsigned int tick = 0;

    while (true) {
        led_on = !led_on;
        gpio_put(PICO_DEFAULT_LED_PIN, led_on);
        printf("BlitzMax Pico 2 baseline: tick=%u led=%s\n",
               tick++, led_on ? "on" : "off");
        sleep_ms(500);
    }
}
