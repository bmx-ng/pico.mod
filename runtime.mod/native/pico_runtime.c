#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "blitzmax/pico_runtime.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/flash.h"
#include "hardware/irq.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "hardware/spi.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"
#include "hardware/structs/timer.h"
#include "hardware/sync.h"
#include "hardware/watchdog.h"
#include "pico/bootrom.h"
#include "pico/aon_timer.h"
#include "pico/flash.h"
#include "pico/platform/sections.h"
#include "pico/stdlib.h"
#include "pico/unique_id.h"

#ifndef BMX_EMBEDDED_ARENA_SIZE
#define BMX_EMBEDDED_ARENA_SIZE (16u * 1024u)
#endif

#ifndef BMX_EMBEDDED_ARENA_IN_PSRAM
#define BMX_EMBEDDED_ARENA_IN_PSRAM 0
#endif

#ifndef BMX_PICO_FLASH_BYTES
#define BMX_PICO_FLASH_BYTES 0u
#endif

#ifndef BMX_PICO_STORAGE_OFFSET
#define BMX_PICO_STORAGE_OFFSET 0u
#endif

#ifndef BMX_PICO_STORAGE_SIZE
#define BMX_PICO_STORAGE_SIZE 0u
#endif

#ifndef BMX_PICO_TAIL_RESERVED_SIZE
#define BMX_PICO_TAIL_RESERVED_SIZE 0u
#endif

#ifndef BMX_EMBEDDED_ROOT_CAPACITY
#define BMX_EMBEDDED_ROOT_CAPACITY 64u
#endif

#ifndef BMX_PICO_ALARM_CAPACITY
#define BMX_PICO_ALARM_CAPACITY 8u
#endif

#define BMX_EMBEDDED_MEMORY_ALIGNMENT 16u

_Static_assert(BMX_PICO_ALARM_CAPACITY > 0u && BMX_PICO_ALARM_CAPACITY <= 255u,
    "Pico alarm handles reserve one byte for the slot number");

static int32_t bmx_pico_put_utf8(uint32_t code_point) {
    int32_t written = 0;
    if (code_point <= 0x7fu) {
        written += stdio_putchar_raw((int)code_point) >= 0;
    } else if (code_point <= 0x7ffu) {
        written += stdio_putchar_raw((int)(0xc0u | (code_point >> 6))) >= 0;
        written += stdio_putchar_raw((int)(0x80u | (code_point & 0x3fu))) >= 0;
    } else if (code_point <= 0xffffu) {
        written += stdio_putchar_raw((int)(0xe0u | (code_point >> 12))) >= 0;
        written += stdio_putchar_raw((int)(0x80u | ((code_point >> 6) & 0x3fu))) >= 0;
        written += stdio_putchar_raw((int)(0x80u | (code_point & 0x3fu))) >= 0;
    } else {
        written += stdio_putchar_raw((int)(0xf0u | (code_point >> 18))) >= 0;
        written += stdio_putchar_raw((int)(0x80u | ((code_point >> 12) & 0x3fu))) >= 0;
        written += stdio_putchar_raw((int)(0x80u | ((code_point >> 6) & 0x3fu))) >= 0;
        written += stdio_putchar_raw((int)(0x80u | (code_point & 0x3fu))) >= 0;
    }
    return written;
}

int32_t bmx_pico_put_string(const BMXEmbeddedString *text) {
    int32_t written = 0;
    for (int32_t index = 0; index < text->length; ++index) {
        uint32_t code_point = text->buf[index];
        if (code_point >= 0xd800u && code_point <= 0xdbffu && index + 1 < text->length) {
            uint32_t low = text->buf[index + 1];
            if (low >= 0xdc00u && low <= 0xdfffu) {
                code_point = 0x10000u + ((code_point - 0xd800u) << 10) + (low - 0xdc00u);
                index += 1;
            }
        }
        written += bmx_pico_put_utf8(code_point);
    }
    return written;
}

int32_t bmx_pico_stdio_init_all(void) {
    return stdio_init_all();
}

void __attribute__((noinline, used)) bmx_embedded_debug_stop(void) {
    __asm volatile ("" ::: "memory");
}

int64_t bmx_pico_stdio_read(void *buffer, int64_t count) {
    if (!buffer || count <= 0) return 0;
    uint8_t *bytes = (uint8_t *)buffer;
    int64_t read = 0;
    while (read < count) {
        int character = stdio_getchar();
        if (character < 0) break;
        bytes[read++] = (uint8_t)character;
    }
    return read;
}

int64_t bmx_pico_stdio_write(void *buffer, int64_t count) {
    if (!buffer || count <= 0) return 0;
    const uint8_t *bytes = (const uint8_t *)buffer;
    int64_t written = 0;
    while (written < count && stdio_putchar_raw(bytes[written]) >= 0) ++written;
    return written;
}

void bmx_pico_stdio_flush(void) {
    stdio_flush();
}

uint32_t bmx_pico_default_led_pin(void) {
#ifdef PICO_DEFAULT_LED_PIN
    return PICO_DEFAULT_LED_PIN;
#else
    return UINT32_MAX;
#endif
}

uint32_t bmx_pico_watchdog_maximum_delay_ms(void) {
#if PICO_RP2040
    return WATCHDOG_LOAD_BITS / 2000u;
#else
    return WATCHDOG_LOAD_BITS / 1000u;
#endif
}

int32_t bmx_pico_watchdog_enable(uint32_t delay_ms, int32_t pause_on_debug) {
    if (!delay_ms || delay_ms > bmx_pico_watchdog_maximum_delay_ms()) return 0;
    watchdog_enable(delay_ms, pause_on_debug != 0);
    return 1;
}

void bmx_pico_watchdog_disable(void) {
    watchdog_disable();
}

void bmx_pico_watchdog_feed(void) {
    watchdog_update();
}

int32_t bmx_pico_watchdog_caused_reboot(void) {
    return watchdog_caused_reboot() != 0;
}

int32_t bmx_pico_watchdog_enable_caused_reboot(void) {
    return watchdog_enable_caused_reboot() != 0;
}

uint32_t bmx_pico_watchdog_time_remaining_us(void) {
    return watchdog_get_time_remaining_us();
}

uint32_t bmx_pico_watchdog_time_remaining_ms(void) {
    return watchdog_get_time_remaining_ms();
}

int32_t bmx_pico_watchdog_reboot(uint32_t delay_ms) {
    if (delay_ms > bmx_pico_watchdog_maximum_delay_ms()) return 0;
    watchdog_reboot(0, 0, delay_ms);
    return 1;
}

const BMXEmbeddedString *bmx_pico_unique_board_id(void) {
    char identifier[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2u + 1u];
    pico_get_unique_board_id_string(identifier, sizeof(identifier));
    return bmx_embedded_string_from_ascii(identifier, PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2);
}

BMXEmbeddedArray *bmx_pico_unique_board_id_bytes(void) {
    pico_unique_board_id_t identifier;
    pico_get_unique_board_id(&identifier);
    BMXEmbeddedArray *result = bmx_embedded_array_new_1d(PICO_UNIQUE_BOARD_ID_SIZE_BYTES,
        sizeof(uint8_t), BMX_EMBEDDED_ARRAY_ELEMENT_VALUE, NULL, NULL);
    if (result != &bmx_embedded_empty_array) {
        memcpy(bmx_embedded_array_data(result), identifier.id, PICO_UNIQUE_BOARD_ID_SIZE_BYTES);
    }
    return result;
}

#define BMX_PICO_BOOTSEL_CS_PIN_INDEX 1u
#if PICO_RP2040
#define BMX_PICO_BOOTSEL_CS_BIT (1u << BMX_PICO_BOOTSEL_CS_PIN_INDEX)
#else
#define BMX_PICO_BOOTSEL_CS_BIT SIO_GPIO_HI_IN_QSPI_CSN_BITS
#endif

int32_t __no_inline_not_in_flash_func(bmx_pico_bootsel_button_pressed)(void) {
    if (get_core_num() != 0) return 0;
    uint32_t interrupt_state = save_and_disable_interrupts();
    hw_write_masked(&ioqspi_hw->io[BMX_PICO_BOOTSEL_CS_PIN_INDEX].ctrl,
        GPIO_OVERRIDE_LOW << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
        IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);
    uint32_t start = timer_hw->timerawl;
    while ((uint32_t)(timer_hw->timerawl - start) <= 8u) {
        tight_loop_contents();
    }
    int32_t pressed = (sio_hw->gpio_hi_in & BMX_PICO_BOOTSEL_CS_BIT) == 0;
    hw_write_masked(&ioqspi_hw->io[BMX_PICO_BOOTSEL_CS_PIN_INDEX].ctrl,
        GPIO_OVERRIDE_NORMAL << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
        IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);
    restore_interrupts(interrupt_state);
    return pressed;
}

int32_t bmx_pico_device_reboot(uint32_t delay_ms) {
    return bmx_pico_watchdog_reboot(delay_ms);
}

int32_t bmx_pico_device_reboot_to_bootsel(int32_t activity_pin, int32_t activity_pin_active_low,
        int32_t disable_mass_storage, int32_t disable_picoboot) {
    if (activity_pin < -1 || activity_pin >= (int32_t)NUM_BANK0_GPIOS || get_core_num() != 0) return 0;
    uint32_t disable_mask = (disable_mass_storage ? 1u : 0u) | (disable_picoboot ? 2u : 0u);
    rom_reset_usb_boot_extra(activity_pin, disable_mask, activity_pin_active_low != 0);
    return 1;
}

uint32_t bmx_pico_flash_storage_physical_size(void) {
    return BMX_PICO_FLASH_BYTES;
}

uint32_t bmx_pico_flash_storage_offset(void) {
    return BMX_PICO_STORAGE_OFFSET;
}

uint32_t bmx_pico_flash_storage_size(void) {
    return BMX_PICO_STORAGE_SIZE;
}

uint32_t bmx_pico_flash_storage_tail_reserved_size(void) {
    return BMX_PICO_TAIL_RESERVED_SIZE;
}

uint32_t bmx_pico_flash_storage_read_size(void) {
    return 1u;
}

uint32_t bmx_pico_flash_storage_program_size(void) {
    return FLASH_PAGE_SIZE;
}

uint32_t bmx_pico_flash_storage_erase_size(void) {
    return FLASH_SECTOR_SIZE;
}

static int32_t bmx_pico_flash_storage_range_valid(uint32_t offset, uint32_t count) {
    return BMX_PICO_STORAGE_SIZE && offset <= BMX_PICO_STORAGE_SIZE &&
        count <= BMX_PICO_STORAGE_SIZE - offset;
}

int32_t bmx_pico_flash_storage_read(uint32_t offset, void *destination, uint32_t count) {
    if ((!destination && count) || !bmx_pico_flash_storage_range_valid(offset, count)) {
        return PICO_ERROR_INVALID_ARG;
    }
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    if (count) {
        memcpy(destination, (const void *)(uintptr_t)(XIP_BASE + BMX_PICO_STORAGE_OFFSET + offset), count);
    }
    return PICO_OK;
}

int32_t bmx_pico_flash_storage_is_erased(uint32_t offset, uint32_t count) {
    if (!bmx_pico_flash_storage_range_valid(offset, count)) return PICO_ERROR_INVALID_ARG;
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    const uint8_t *source = (const uint8_t *)(uintptr_t)(XIP_BASE + BMX_PICO_STORAGE_OFFSET + offset);
    for (uint32_t index = 0; index < count; ++index) {
        if (source[index] != 0xffu) return 0;
    }
    return 1;
}

typedef struct BMXPicoFlashOperation {
    uint32_t offset;
    const uint8_t *data;
    uint32_t count;
} BMXPicoFlashOperation;

static void __no_inline_not_in_flash_func(bmx_pico_flash_storage_program_callback)(void *parameter) {
    BMXPicoFlashOperation *operation = (BMXPicoFlashOperation *)parameter;
    flash_range_program(BMX_PICO_STORAGE_OFFSET + operation->offset, operation->data, operation->count);
}

static void __no_inline_not_in_flash_func(bmx_pico_flash_storage_erase_callback)(void *parameter) {
    BMXPicoFlashOperation *operation = (BMXPicoFlashOperation *)parameter;
    flash_range_erase(BMX_PICO_STORAGE_OFFSET + operation->offset, operation->count);
}

int32_t bmx_pico_flash_storage_program(uint32_t offset, void *source, uint32_t count,
        uint32_t timeout_ms) {
    if ((!source && count) || !bmx_pico_flash_storage_range_valid(offset, count)) {
        return PICO_ERROR_INVALID_ARG;
    }
    if ((offset & (FLASH_PAGE_SIZE - 1u)) || (count & (FLASH_PAGE_SIZE - 1u))) {
        return PICO_ERROR_BAD_ALIGNMENT;
    }
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;

    const uint8_t *bytes = (const uint8_t *)source;
    const uint8_t *existing = (const uint8_t *)(uintptr_t)(XIP_BASE + BMX_PICO_STORAGE_OFFSET + offset);
    for (uint32_t index = 0; index < count; ++index) {
        if ((existing[index] & bytes[index]) != bytes[index]) {
            return PICO_ERROR_UNSUPPORTED_MODIFICATION;
        }
    }

    uint8_t page[FLASH_PAGE_SIZE];
    for (uint32_t position = 0; position < count; position += FLASH_PAGE_SIZE) {
        memcpy(page, bytes + position, FLASH_PAGE_SIZE);
        BMXPicoFlashOperation operation = {offset + position, page, FLASH_PAGE_SIZE};
        int result = flash_safe_execute(bmx_pico_flash_storage_program_callback, &operation, timeout_ms);
        if (result != PICO_OK) return result;
    }
    return PICO_OK;
}

int32_t bmx_pico_flash_storage_erase(uint32_t offset, uint32_t count, uint32_t timeout_ms) {
    if (!bmx_pico_flash_storage_range_valid(offset, count)) return PICO_ERROR_INVALID_ARG;
    if ((offset & (FLASH_SECTOR_SIZE - 1u)) || (count & (FLASH_SECTOR_SIZE - 1u))) {
        return PICO_ERROR_BAD_ALIGNMENT;
    }
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    BMXPicoFlashOperation operation = {offset, NULL, count};
    return flash_safe_execute(bmx_pico_flash_storage_erase_callback, &operation, timeout_ms);
}

void bmx_pico_gpio_init(uint32_t gpio) {
    gpio_init(gpio);
}

void bmx_pico_gpio_set_function(uint32_t gpio, int32_t function) {
    gpio_set_function(gpio, (gpio_function_t)function);
}

int32_t bmx_pico_gpio_get_function(uint32_t gpio) {
    return (int32_t)gpio_get_function(gpio);
}

void bmx_pico_gpio_set_direction(uint32_t gpio, int32_t direction) {
    gpio_set_dir(gpio, direction != 0);
}

int32_t bmx_pico_gpio_get_direction(uint32_t gpio) {
    return gpio_get_dir(gpio) != 0;
}

void bmx_pico_gpio_set_input(uint32_t gpio) {
    gpio_set_dir(gpio, GPIO_IN);
}

void bmx_pico_gpio_set_output(uint32_t gpio) {
    gpio_set_dir(gpio, GPIO_OUT);
}

int32_t bmx_pico_gpio_get(uint32_t gpio) {
    return gpio_get(gpio) != 0;
}

void bmx_pico_gpio_put(uint32_t gpio, int32_t value) {
    gpio_put(gpio, value != 0);
}

int32_t bmx_pico_gpio_get_output(uint32_t gpio) {
    return gpio_get_out_level(gpio) != 0;
}

void bmx_pico_gpio_set_pulls(uint32_t gpio, int32_t pull_up, int32_t pull_down) {
    gpio_set_pulls(gpio, pull_up != 0, pull_down != 0);
}

void bmx_pico_gpio_pull_up(uint32_t gpio) {
    gpio_pull_up(gpio);
}

void bmx_pico_gpio_pull_down(uint32_t gpio) {
    gpio_pull_down(gpio);
}

void bmx_pico_gpio_disable_pulls(uint32_t gpio) {
    gpio_disable_pulls(gpio);
}

int32_t bmx_pico_gpio_is_pulled_up(uint32_t gpio) {
    return gpio_is_pulled_up(gpio) != 0;
}

int32_t bmx_pico_gpio_is_pulled_down(uint32_t gpio) {
    return gpio_is_pulled_down(gpio) != 0;
}

void bmx_pico_gpio_set_input_enabled(uint32_t gpio, int32_t enabled) {
    gpio_set_input_enabled(gpio, enabled != 0);
}

void bmx_pico_gpio_set_input_hysteresis_enabled(uint32_t gpio, int32_t enabled) {
    gpio_set_input_hysteresis_enabled(gpio, enabled != 0);
}

void bmx_pico_gpio_set_slew_rate(uint32_t gpio, int32_t slew_rate) {
    gpio_set_slew_rate(gpio, slew_rate ? GPIO_SLEW_RATE_FAST : GPIO_SLEW_RATE_SLOW);
}

int32_t bmx_pico_gpio_get_slew_rate(uint32_t gpio) {
    return (int32_t)gpio_get_slew_rate(gpio);
}

void bmx_pico_gpio_set_drive_strength(uint32_t gpio, int32_t drive_strength) {
    gpio_set_drive_strength(gpio, (enum gpio_drive_strength)drive_strength);
}

int32_t bmx_pico_gpio_get_drive_strength(uint32_t gpio) {
    return (int32_t)gpio_get_drive_strength(gpio);
}

static volatile uint32_t bmx_pico_gpio_irq_events[NUM_BANK0_GPIOS];
static volatile uint32_t bmx_pico_gpio_event_tokens[NUM_BANK0_GPIOS];
static int bmx_pico_gpio_irq_callback_installed;

static void bmx_pico_gpio_irq_callback(uint gpio, uint32_t events) {
    if (gpio < NUM_BANK0_GPIOS) {
        uint64_t captured_time = time_us_64();
        bmx_pico_gpio_irq_events[gpio] |= events;
        bmx_pico_event_post_from_irq_ex(bmx_pico_gpio_event_tokens[gpio], events,
            gpio, (uint32_t)captured_time, (uint32_t)(captured_time >> 32u));
    }
}

int32_t bmx_pico_gpio_set_irq_enabled(uint32_t gpio, uint32_t event_mask, int32_t enabled) {
    if (get_core_num() != 0 || gpio >= NUM_BANK0_GPIOS ||
            !event_mask || (event_mask & ~0x0fu)) return 0;
    if (enabled && !bmx_pico_gpio_irq_callback_installed) {
        gpio_set_irq_enabled_with_callback(gpio, event_mask, true, bmx_pico_gpio_irq_callback);
        bmx_pico_gpio_irq_callback_installed = 1;
    } else {
        gpio_set_irq_enabled(gpio, event_mask, enabled != 0);
    }
    return 1;
}

int32_t bmx_pico_gpio_set_event_token(uint32_t gpio, uint32_t token) {
    if (get_core_num() != 0 || gpio >= NUM_BANK0_GPIOS) return 0;
    uint32_t interrupt_state = save_and_disable_interrupts();
    if (token && bmx_pico_gpio_event_tokens[gpio] &&
            bmx_pico_gpio_event_tokens[gpio] != token) {
        restore_interrupts(interrupt_state);
        return 0;
    }
    bmx_pico_gpio_event_tokens[gpio] = token;
    restore_interrupts(interrupt_state);
    return 1;
}

uint32_t bmx_pico_gpio_pending_irq_events(uint32_t gpio) {
    if (gpio >= NUM_BANK0_GPIOS) return 0;
    return bmx_pico_gpio_irq_events[gpio];
}

uint32_t bmx_pico_gpio_take_irq_events(uint32_t gpio) {
    if (gpio >= NUM_BANK0_GPIOS || get_core_num() != 0) return 0;
    uint32_t interrupt_state = save_and_disable_interrupts();
    uint32_t events = bmx_pico_gpio_irq_events[gpio];
    bmx_pico_gpio_irq_events[gpio] = 0;
    restore_interrupts(interrupt_state);
    return events;
}

int32_t bmx_pico_putchar_raw(int32_t character) {
    return stdio_putchar_raw(character);
}

void bmx_pico_sleep_ms(uint32_t milliseconds) {
    sleep_ms(milliseconds);
}

int32_t bmx_embedded_millisecs(void) {
    return (int32_t)(uint32_t)(time_us_64() / 1000u);
}

uint64_t bmx_pico_time_microseconds(void) {
    return time_us_64();
}

uint32_t bmx_pico_system_clock_hz(void) {
    return clock_get_hz(clk_sys);
}

uint64_t bmx_pico_time_milliseconds(void) {
    return time_us_64() / 1000u;
}

void bmx_pico_sleep_us(uint64_t microseconds) {
    sleep_us(microseconds);
}

enum {
    BMX_PICO_ALARM_FREE = 0,
    BMX_PICO_ALARM_ARMED = 1,
    BMX_PICO_ALARM_FIRED = 2
};

typedef struct BMXPicoAlarmSlot {
    volatile uint32_t state;
    volatile uint32_t pending;
    uint32_t generation;
    alarm_id_t alarm_id;
    uint64_t interval_us;
} BMXPicoAlarmSlot;

static BMXPicoAlarmSlot bmx_pico_alarm_slots[BMX_PICO_ALARM_CAPACITY];

static int32_t bmx_pico_alarm_handle(uint32_t index, uint32_t generation) {
    return (int32_t)((generation << 8u) | (index + 1u));
}

static BMXPicoAlarmSlot *bmx_pico_alarm_slot(int32_t handle) {
    uint32_t encoded = (uint32_t)handle;
    uint32_t slot_number = encoded & 0xffu;
    uint32_t generation = encoded >> 8u;
    if (!slot_number || slot_number > BMX_PICO_ALARM_CAPACITY || !generation) return NULL;
    BMXPicoAlarmSlot *slot = &bmx_pico_alarm_slots[slot_number - 1u];
    if (slot->generation != generation || slot->state == BMX_PICO_ALARM_FREE) return NULL;
    return slot;
}

static int64_t bmx_pico_alarm_callback(alarm_id_t id, void *user_data) {
    BMXPicoAlarmSlot *slot = (BMXPicoAlarmSlot *)user_data;
    if (!slot || slot->state != BMX_PICO_ALARM_ARMED || slot->alarm_id != id) return 0;
    if (slot->pending != UINT32_MAX) ++slot->pending;
    if (slot->interval_us) return -(int64_t)slot->interval_us;
    slot->alarm_id = 0;
    slot->state = BMX_PICO_ALARM_FIRED;
    return 0;
}

static int32_t bmx_pico_alarm_schedule(uint64_t delay_us, uint64_t interval_us) {
    if (!delay_us || delay_us > INT64_MAX || interval_us > INT64_MAX || get_core_num() != 0) return 0;
    uint32_t interrupt_state = save_and_disable_interrupts();
    uint32_t index = BMX_PICO_ALARM_CAPACITY;
    for (uint32_t candidate = 0; candidate < BMX_PICO_ALARM_CAPACITY; ++candidate) {
        if (bmx_pico_alarm_slots[candidate].state == BMX_PICO_ALARM_FREE) {
            index = candidate;
            break;
        }
    }
    if (index == BMX_PICO_ALARM_CAPACITY) {
        restore_interrupts(interrupt_state);
        return 0;
    }
    BMXPicoAlarmSlot *slot = &bmx_pico_alarm_slots[index];
    uint32_t generation = slot->generation + 1u;
    if (!generation || generation > 0x7fffffu) generation = 1u;
    slot->generation = generation;
    slot->pending = 0;
    slot->interval_us = interval_us;
    slot->alarm_id = 0;
    slot->state = BMX_PICO_ALARM_ARMED;
    alarm_id_t id = add_alarm_in_us(delay_us, bmx_pico_alarm_callback, slot, false);
    if (id <= 0) {
        slot->state = BMX_PICO_ALARM_FREE;
        restore_interrupts(interrupt_state);
        return 0;
    }
    slot->alarm_id = id;
    int32_t handle = bmx_pico_alarm_handle(index, generation);
    restore_interrupts(interrupt_state);
    return handle;
}

int32_t bmx_pico_alarm_after_ms(uint32_t milliseconds) {
    return bmx_pico_alarm_schedule((uint64_t)milliseconds * 1000u, 0);
}

int32_t bmx_pico_alarm_after_us(uint64_t microseconds) {
    return bmx_pico_alarm_schedule(microseconds, 0);
}

int32_t bmx_pico_repeating_alarm_ms(uint32_t milliseconds) {
    uint64_t interval = (uint64_t)milliseconds * 1000u;
    return bmx_pico_alarm_schedule(interval, interval);
}

int32_t bmx_pico_repeating_alarm_us(uint64_t microseconds) {
    return bmx_pico_alarm_schedule(microseconds, microseconds);
}

int32_t bmx_pico_alarm_cancel(int32_t handle) {
    if (get_core_num() != 0) return 0;
    uint32_t interrupt_state = save_and_disable_interrupts();
    BMXPicoAlarmSlot *slot = bmx_pico_alarm_slot(handle);
    if (!slot) {
        restore_interrupts(interrupt_state);
        return 0;
    }
    if (slot->state == BMX_PICO_ALARM_FIRED) {
        slot->state = BMX_PICO_ALARM_FREE;
        restore_interrupts(interrupt_state);
        return 1;
    }
    int cancelled = cancel_alarm(slot->alarm_id);
    if (cancelled) slot->state = BMX_PICO_ALARM_FREE;
    restore_interrupts(interrupt_state);
    return cancelled != 0;
}

int32_t bmx_pico_alarm_active(int32_t handle) {
    if (get_core_num() != 0) return 0;
    uint32_t interrupt_state = save_and_disable_interrupts();
    BMXPicoAlarmSlot *slot = bmx_pico_alarm_slot(handle);
    int32_t active = slot && slot->state == BMX_PICO_ALARM_ARMED;
    restore_interrupts(interrupt_state);
    return active;
}

uint32_t bmx_pico_alarm_pending_events(int32_t handle) {
    if (get_core_num() != 0) return 0;
    uint32_t interrupt_state = save_and_disable_interrupts();
    BMXPicoAlarmSlot *slot = bmx_pico_alarm_slot(handle);
    uint32_t pending = slot ? slot->pending : 0;
    restore_interrupts(interrupt_state);
    return pending;
}

uint32_t bmx_pico_alarm_take_events(int32_t handle) {
    if (get_core_num() != 0) return 0;
    uint32_t interrupt_state = save_and_disable_interrupts();
    BMXPicoAlarmSlot *slot = bmx_pico_alarm_slot(handle);
    uint32_t pending = slot ? slot->pending : 0;
    if (slot) {
        slot->pending = 0;
        if (slot->state == BMX_PICO_ALARM_FIRED) slot->state = BMX_PICO_ALARM_FREE;
    }
    restore_interrupts(interrupt_state);
    return pending;
}

int64_t bmx_pico_alarm_remaining_us(int32_t handle) {
    if (get_core_num() != 0) return -1;
    uint32_t interrupt_state = save_and_disable_interrupts();
    BMXPicoAlarmSlot *slot = bmx_pico_alarm_slot(handle);
    alarm_id_t id = slot && slot->state == BMX_PICO_ALARM_ARMED ? slot->alarm_id : 0;
    restore_interrupts(interrupt_state);
    return id ? remaining_alarm_time_us(id) : -1;
}

int32_t bmx_pico_alarm_remaining_ms(int32_t handle) {
    if (get_core_num() != 0) return -1;
    uint32_t interrupt_state = save_and_disable_interrupts();
    BMXPicoAlarmSlot *slot = bmx_pico_alarm_slot(handle);
    alarm_id_t id = slot && slot->state == BMX_PICO_ALARM_ARMED ? slot->alarm_id : 0;
    restore_interrupts(interrupt_state);
    return id ? remaining_alarm_time_ms(id) : -1;
}

static int32_t bmx_pico_calendar_days_in_month(int32_t year, int32_t month) {
    static const uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int32_t result = days[month - 1];
    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) ++result;
    return result;
}

static int32_t bmx_pico_calendar_valid(const BMXPicoCalendarDateTime *date_time) {
    if (!date_time || date_time->year < 1 || date_time->year > 9999 ||
        date_time->month < 1 || date_time->month > 12 ||
        date_time->day < 1 || date_time->day > bmx_pico_calendar_days_in_month(date_time->year, date_time->month) ||
        date_time->hour < 0 || date_time->hour > 23 || date_time->minute < 0 || date_time->minute > 59 ||
        date_time->second < 0 || date_time->second > 59 ||
        date_time->millisecond < 0 || date_time->millisecond > 999) return 0;
    return 1;
}

static int64_t bmx_pico_calendar_days_from_civil(int32_t year, uint32_t month, uint32_t day) {
    year -= month <= 2;
    int32_t era = (year >= 0 ? year : year - 399) / 400;
    uint32_t year_of_era = (uint32_t)(year - era * 400);
    uint32_t day_of_year = (153u * (month + (month > 2 ? (uint32_t)-3 : 9u)) + 2u) / 5u + day - 1u;
    uint32_t day_of_era = year_of_era * 365u + year_of_era / 4u - year_of_era / 100u + day_of_year;
    return (int64_t)era * 146097 + (int64_t)day_of_era - 719468;
}

static void bmx_pico_calendar_civil_from_days(int64_t days, int32_t *year, int32_t *month, int32_t *day) {
    days += 719468;
    int64_t era = (days >= 0 ? days : days - 146096) / 146097;
    uint32_t day_of_era = (uint32_t)(days - era * 146097);
    uint32_t year_of_era = (day_of_era - day_of_era / 1460u + day_of_era / 36524u - day_of_era / 146096u) / 365u;
    int32_t y = (int32_t)year_of_era + (int32_t)era * 400;
    uint32_t day_of_year = day_of_era - (365u * year_of_era + year_of_era / 4u - year_of_era / 100u);
    uint32_t month_prime = (5u * day_of_year + 2u) / 153u;
    uint32_t d = day_of_year - (153u * month_prime + 2u) / 5u + 1u;
    uint32_t m = month_prime + (month_prime < 10u ? 3u : (uint32_t)-9);
    y += m <= 2u;
    *year = y;
    *month = (int32_t)m;
    *day = (int32_t)d;
}

int32_t bmx_pico_datetime_to_epoch(const BMXPicoCalendarDateTime *date_time,
    int64_t *seconds, int32_t *milliseconds) {
    if (!seconds || !milliseconds || !bmx_pico_calendar_valid(date_time)) return 0;
    int64_t value = bmx_pico_calendar_days_from_civil(date_time->year,
        (uint32_t)date_time->month, (uint32_t)date_time->day) * 86400;
    value += date_time->hour * 3600 + date_time->minute * 60 + date_time->second;
    if (!date_time->utc) {
        value -= (int64_t)date_time->offset * 60;
        if (date_time->dst == 1) value -= 3600;
    }
    *seconds = value;
    *milliseconds = date_time->millisecond;
    return 1;
}

int32_t bmx_pico_datetime_from_epoch(int64_t seconds, int32_t milliseconds,
    BMXPicoCalendarDateTime *date_time) {
    if (!date_time || milliseconds < 0 || milliseconds > 999) return 0;
    int64_t days = seconds / 86400;
    int64_t within_day = seconds % 86400;
    if (within_day < 0) {
        within_day += 86400;
        --days;
    }
    int32_t year, month, day;
    bmx_pico_calendar_civil_from_days(days, &year, &month, &day);
    if (year < 1 || year > 9999) return 0;
    date_time->year = year;
    date_time->month = month;
    date_time->day = day;
    date_time->hour = (int32_t)(within_day / 3600);
    date_time->minute = (int32_t)((within_day / 60) % 60);
    date_time->second = (int32_t)(within_day % 60);
    date_time->millisecond = milliseconds;
    date_time->utc = 1;
    date_time->offset = 0;
    date_time->dst = 0;
    return 1;
}

static int32_t bmx_pico_calendar_to_hardware(const BMXPicoCalendarDateTime *date_time,
    int64_t *seconds, int32_t *milliseconds, struct tm *calendar) {
    if (!bmx_pico_datetime_to_epoch(date_time, seconds, milliseconds) || *seconds < 0) return 0;
    BMXPicoCalendarDateTime utc;
    if (!bmx_pico_datetime_from_epoch(*seconds, *milliseconds, &utc) || utc.year > 4095) return 0;
    memset(calendar, 0, sizeof(*calendar));
    calendar->tm_year = utc.year - 1900;
    calendar->tm_mon = utc.month - 1;
    calendar->tm_mday = utc.day;
    calendar->tm_hour = utc.hour;
    calendar->tm_min = utc.minute;
    calendar->tm_sec = utc.second;
    calendar->tm_wday = (int)((*seconds / 86400 + 4) % 7);
    calendar->tm_yday = (int)(bmx_pico_calendar_days_from_civil(utc.year, (uint32_t)utc.month,
        (uint32_t)utc.day) - bmx_pico_calendar_days_from_civil(utc.year, 1u, 1u));
    return 1;
}

static volatile uint32_t bmx_pico_calendar_alarm_events;

static void bmx_pico_calendar_alarm_handler(void) {
    /* Calendar alarms are one-shot at the BlitzMax API. Explicitly disable the
       source before publishing the event: on RP2040 an exact calendar match
       otherwise remains asserted throughout the matching second. */
    aon_timer_disable_alarm();
    if (bmx_pico_calendar_alarm_events != UINT32_MAX) ++bmx_pico_calendar_alarm_events;
}

int32_t bmx_pico_calendar_start(const BMXPicoCalendarDateTime *date_time) {
    if (get_core_num() != 0) return 0;
    int64_t seconds;
    int32_t milliseconds;
    struct tm calendar;
    if (!bmx_pico_calendar_to_hardware(date_time, &seconds, &milliseconds, &calendar)) return 0;
#if HAS_RP2040_RTC
    return aon_timer_start_calendar(&calendar);
#else
    struct timespec value = { .tv_sec = (time_t)seconds, .tv_nsec = milliseconds * 1000000L };
    return aon_timer_start(&value);
#endif
}

void bmx_pico_calendar_stop(void) {
    if (get_core_num() != 0) return;
    aon_timer_disable_alarm();
    aon_timer_stop();
    while (aon_timer_is_running()) tight_loop_contents();
    uint32_t interrupt_state = save_and_disable_interrupts();
    bmx_pico_calendar_alarm_events = 0;
    restore_interrupts(interrupt_state);
}

int32_t bmx_pico_calendar_set(const BMXPicoCalendarDateTime *date_time) {
    if (get_core_num() != 0) return 0;
    int64_t seconds;
    int32_t milliseconds;
    struct tm calendar;
    if (!bmx_pico_calendar_to_hardware(date_time, &seconds, &milliseconds, &calendar)) return 0;
#if HAS_RP2040_RTC
    return aon_timer_set_time_calendar(&calendar);
#else
    struct timespec value = { .tv_sec = (time_t)seconds, .tv_nsec = milliseconds * 1000000L };
    return aon_timer_set_time(&value);
#endif
}

int32_t bmx_pico_calendar_get(BMXPicoCalendarDateTime *date_time) {
    if (!date_time || get_core_num() != 0 || !aon_timer_is_running()) return 0;
#if HAS_RP2040_RTC
    struct tm calendar;
    if (!aon_timer_get_time_calendar(&calendar)) return 0;
    BMXPicoCalendarDateTime value = {
        .year = calendar.tm_year + 1900, .month = calendar.tm_mon + 1, .day = calendar.tm_mday,
        .hour = calendar.tm_hour, .minute = calendar.tm_min, .second = calendar.tm_sec,
        .millisecond = 0, .utc = 1, .offset = 0, .dst = 0
    };
    *date_time = value;
    return 1;
#else
    struct timespec value;
    if (!aon_timer_get_time(&value)) return 0;
    return bmx_pico_datetime_from_epoch((int64_t)value.tv_sec, (int32_t)(value.tv_nsec / 1000000L), date_time);
#endif
}

int32_t bmx_pico_calendar_is_running(void) {
    return aon_timer_is_running();
}

uint64_t bmx_pico_calendar_resolution_nanoseconds(void) {
    struct timespec resolution;
    aon_timer_get_resolution(&resolution);
    return (uint64_t)resolution.tv_sec * 1000000000u + (uint64_t)resolution.tv_nsec;
}

int32_t bmx_pico_calendar_set_alarm(const BMXPicoCalendarDateTime *date_time,
    int32_t wake_from_low_power) {
    if (get_core_num() != 0) return 0;
    int64_t seconds;
    int32_t milliseconds;
    struct tm calendar;
    if (!bmx_pico_calendar_to_hardware(date_time, &seconds, &milliseconds, &calendar)) return 0;
    bmx_pico_calendar_alarm_events = 0;
#if HAS_RP2040_RTC
    aon_timer_alarm_handler_t previous = aon_timer_enable_alarm_calendar(&calendar,
        bmx_pico_calendar_alarm_handler, wake_from_low_power != 0);
#else
    struct timespec value = { .tv_sec = (time_t)seconds, .tv_nsec = milliseconds * 1000000L };
    aon_timer_alarm_handler_t previous = aon_timer_enable_alarm(&value,
        bmx_pico_calendar_alarm_handler, wake_from_low_power != 0);
#endif
    return (intptr_t)previous != PICO_ERROR_INVALID_ARG;
}

void bmx_pico_calendar_disable_alarm(void) {
    if (get_core_num() != 0) return;
    aon_timer_disable_alarm();
    uint32_t interrupt_state = save_and_disable_interrupts();
    bmx_pico_calendar_alarm_events = 0;
    restore_interrupts(interrupt_state);
}

uint32_t bmx_pico_calendar_pending_alarm_events(void) {
    if (get_core_num() != 0) return 0;
    uint32_t interrupt_state = save_and_disable_interrupts();
    uint32_t events = bmx_pico_calendar_alarm_events;
    restore_interrupts(interrupt_state);
    return events;
}

uint32_t bmx_pico_calendar_take_alarm_events(void) {
    if (get_core_num() != 0) return 0;
    uint32_t interrupt_state = save_and_disable_interrupts();
    uint32_t events = bmx_pico_calendar_alarm_events;
    bmx_pico_calendar_alarm_events = 0;
    restore_interrupts(interrupt_state);
    return events;
}

uint32_t bmx_pico_pwm_init_gpio(uint32_t gpio) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    return pwm_gpio_to_slice_num(gpio);
}

uint32_t bmx_pico_pwm_slice_for_gpio(uint32_t gpio) {
    return pwm_gpio_to_slice_num(gpio);
}

uint32_t bmx_pico_pwm_channel_for_gpio(uint32_t gpio) {
    return pwm_gpio_to_channel(gpio);
}

void bmx_pico_pwm_set_wrap(uint32_t slice, uint32_t wrap) {
    pwm_set_wrap(slice, (uint16_t)wrap);
}

uint32_t bmx_pico_pwm_get_wrap(uint32_t slice) {
    check_slice_num_param(slice);
    return pwm_hw->slice[slice].top;
}

void bmx_pico_pwm_set_channel_level(uint32_t slice, uint32_t channel, uint32_t level) {
    pwm_set_chan_level(slice, channel, (uint16_t)level);
}

uint32_t bmx_pico_pwm_get_channel_level(uint32_t slice, uint32_t channel) {
    check_slice_num_param(slice);
    uint32_t compare = pwm_hw->slice[slice].cc;
    return channel ? (compare >> PWM_CH0_CC_B_LSB) & 0xffffu : compare & 0xffffu;
}

void bmx_pico_pwm_set_both_levels(uint32_t slice, uint32_t level_a, uint32_t level_b) {
    pwm_set_both_levels(slice, (uint16_t)level_a, (uint16_t)level_b);
}

void bmx_pico_pwm_set_gpio_level(uint32_t gpio, uint32_t level) {
    pwm_set_gpio_level(gpio, (uint16_t)level);
}

uint32_t bmx_pico_pwm_get_counter(uint32_t slice) {
    return pwm_get_counter(slice);
}

void bmx_pico_pwm_set_counter(uint32_t slice, uint32_t counter) {
    pwm_set_counter(slice, (uint16_t)counter);
}

void bmx_pico_pwm_set_clock_divider(uint32_t slice, float divider) {
    pwm_set_clkdiv(slice, divider);
}

void bmx_pico_pwm_set_clock_divider_int_frac(uint32_t slice, uint32_t integer, uint32_t fraction) {
    pwm_set_clkdiv_int_frac4(slice, (uint8_t)integer, (uint8_t)fraction);
}

void bmx_pico_pwm_set_divider_mode(uint32_t slice, uint32_t mode) {
    pwm_set_clkdiv_mode(slice, (enum pwm_clkdiv_mode)mode);
}

void bmx_pico_pwm_set_output_polarity(uint32_t slice, int32_t invert_a, int32_t invert_b) {
    pwm_set_output_polarity(slice, invert_a != 0, invert_b != 0);
}

void bmx_pico_pwm_set_phase_correct(uint32_t slice, int32_t enabled) {
    pwm_set_phase_correct(slice, enabled != 0);
}

void bmx_pico_pwm_set_enabled(uint32_t slice, int32_t enabled) {
    pwm_set_enabled(slice, enabled != 0);
}

uint32_t bmx_pico_pwm_set_frequency(uint32_t slice, uint32_t frequency) {
    check_slice_num_param(slice);
    if (!frequency) return 0;
    uint64_t system_frequency = clock_get_hz(clk_sys);
    uint64_t denominator = (uint64_t)frequency * 65536u;
    uint32_t divider16 = (uint32_t)((system_frequency * 16u + denominator - 1u) / denominator);
    if (divider16 < 16u) divider16 = 16u;
    if (divider16 > 4095u) divider16 = 4095u;
    uint64_t wrap_plus_one = (system_frequency * 16u) / ((uint64_t)frequency * divider16);
    if (!wrap_plus_one) wrap_plus_one = 1u;
    if (wrap_plus_one > 65536u) wrap_plus_one = 65536u;
    pwm_set_clkdiv_int_frac4(slice, divider16 >> 4u, divider16 & 0x0fu);
    pwm_set_wrap(slice, (uint16_t)(wrap_plus_one - 1u));
    return (uint32_t)((system_frequency * 16u) / ((uint64_t)divider16 * wrap_plus_one));
}

static volatile uint32_t bmx_pico_pwm_wrap_events[NUM_PWM_SLICES];
static int bmx_pico_pwm_irq_installed;

static void bmx_pico_pwm_irq_handler(void) {
    uint32_t pending = pwm_get_irq_status_mask();
    for (uint32_t slice = 0; slice < NUM_PWM_SLICES; ++slice) {
        if (pending & (1u << slice)) {
            pwm_clear_irq(slice);
            if (bmx_pico_pwm_wrap_events[slice] != UINT32_MAX) ++bmx_pico_pwm_wrap_events[slice];
        }
    }
}

int32_t bmx_pico_pwm_set_irq_enabled(uint32_t slice, int32_t enabled) {
    if (get_core_num() != 0 || slice >= NUM_PWM_SLICES) return 0;
    uint32_t interrupt_state = save_and_disable_interrupts();
    if (enabled && !bmx_pico_pwm_irq_installed) {
        irq_add_shared_handler(PWM_DEFAULT_IRQ_NUM(), bmx_pico_pwm_irq_handler,
            PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
        irq_set_enabled(PWM_DEFAULT_IRQ_NUM(), true);
        bmx_pico_pwm_irq_installed = 1;
    }
    pwm_clear_irq(slice);
    bmx_pico_pwm_wrap_events[slice] = 0;
    pwm_set_irq_enabled(slice, enabled != 0);
    restore_interrupts(interrupt_state);
    return 1;
}

uint32_t bmx_pico_pwm_pending_wrap_events(uint32_t slice) {
    if (get_core_num() != 0 || slice >= NUM_PWM_SLICES) return 0;
    return bmx_pico_pwm_wrap_events[slice];
}

uint32_t bmx_pico_pwm_take_wrap_events(uint32_t slice) {
    if (get_core_num() != 0 || slice >= NUM_PWM_SLICES) return 0;
    uint32_t interrupt_state = save_and_disable_interrupts();
    uint32_t events = bmx_pico_pwm_wrap_events[slice];
    bmx_pico_pwm_wrap_events[slice] = 0;
    restore_interrupts(interrupt_state);
    return events;
}

void bmx_pico_adc_init(void) {
    adc_init();
}

void bmx_pico_adc_gpio_init(uint32_t gpio) {
    adc_gpio_init(gpio);
}

int32_t bmx_pico_adc_input_for_gpio(uint32_t gpio) {
    if (gpio < ADC_BASE_PIN || gpio >= ADC_BASE_PIN + NUM_ADC_CHANNELS - 1) return -1;
    return (int32_t)(gpio - ADC_BASE_PIN);
}

void bmx_pico_adc_select_input(uint32_t input) {
    adc_select_input(input);
}

uint32_t bmx_pico_adc_get_selected_input(void) {
    return adc_get_selected_input();
}

void bmx_pico_adc_set_round_robin(uint32_t input_mask) {
    adc_set_round_robin(input_mask);
}

void bmx_pico_adc_set_temperature_sensor_enabled(int32_t enabled) {
    adc_set_temp_sensor_enabled(enabled != 0);
}

uint32_t bmx_pico_adc_read(void) {
    return adc_read();
}

uint32_t bmx_pico_adc_read_input(uint32_t input) {
    adc_select_input(input);
    return adc_read();
}

void bmx_pico_adc_run(int32_t enabled) {
    adc_run(enabled != 0);
}

void bmx_pico_adc_set_clock_divider(float divider) {
    adc_set_clkdiv(divider);
}

void bmx_pico_adc_fifo_setup(int32_t enabled, int32_t dma_request_enabled,
        uint32_t dma_request_threshold, int32_t error_in_fifo, int32_t byte_shift) {
    adc_fifo_setup(enabled != 0, dma_request_enabled != 0,
        (uint16_t)dma_request_threshold, error_in_fifo != 0, byte_shift != 0);
}

int32_t bmx_pico_adc_fifo_is_empty(void) {
    return adc_fifo_is_empty();
}

uint32_t bmx_pico_adc_fifo_level(void) {
    return adc_fifo_get_level();
}

uint32_t bmx_pico_adc_fifo_get(void) {
    return adc_fifo_get();
}

uint32_t bmx_pico_adc_fifo_get_blocking(void) {
    return adc_fifo_get_blocking();
}

void bmx_pico_adc_fifo_drain(void) {
    adc_fifo_drain();
}

void *bmx_pico_adc_fifo_address(void) {
    return (void *)&adc_hw->fifo;
}

uint32_t bmx_pico_adc_dreq(void) {
    return DREQ_ADC;
}

static i2c_inst_t *bmx_pico_i2c_instance(int32_t controller) {
    if (controller < 0 || controller >= NUM_I2CS) return NULL;
    return i2c_get_instance((uint)controller);
}

int32_t bmx_pico_i2c_default_controller(void) {
#ifdef PICO_DEFAULT_I2C
    return PICO_DEFAULT_I2C;
#else
    return -1;
#endif
}

uint32_t bmx_pico_i2c_default_sda_pin(void) {
#ifdef PICO_DEFAULT_I2C_SDA_PIN
    return PICO_DEFAULT_I2C_SDA_PIN;
#else
    return UINT32_MAX;
#endif
}

uint32_t bmx_pico_i2c_default_scl_pin(void) {
#ifdef PICO_DEFAULT_I2C_SCL_PIN
    return PICO_DEFAULT_I2C_SCL_PIN;
#else
    return UINT32_MAX;
#endif
}

int32_t bmx_pico_i2c_configure_pins(int32_t controller, uint32_t sda_pin,
        uint32_t scl_pin, int32_t pull_ups) {
    if (!bmx_pico_i2c_instance(controller) || sda_pin >= NUM_BANK0_GPIOS ||
            scl_pin >= NUM_BANK0_GPIOS) return 0;
    uint32_t sda_pattern = controller ? 2u : 0u;
    if ((sda_pin & 3u) != sda_pattern || (scl_pin & 3u) != sda_pattern + 1u) return 0;
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    if (pull_ups) {
        gpio_pull_up(sda_pin);
        gpio_pull_up(scl_pin);
    } else {
        gpio_disable_pulls(sda_pin);
        gpio_disable_pulls(scl_pin);
    }
    return 1;
}

uint32_t bmx_pico_i2c_init(int32_t controller, uint32_t baudrate) {
    i2c_inst_t *i2c = bmx_pico_i2c_instance(controller);
    return i2c && baudrate ? i2c_init(i2c, baudrate) : 0;
}

void bmx_pico_i2c_deinit(int32_t controller) {
    i2c_inst_t *i2c = bmx_pico_i2c_instance(controller);
    if (i2c) i2c_deinit(i2c);
}

uint32_t bmx_pico_i2c_set_baudrate(int32_t controller, uint32_t baudrate) {
    i2c_inst_t *i2c = bmx_pico_i2c_instance(controller);
    return i2c && baudrate ? i2c_set_baudrate(i2c, baudrate) : 0;
}

int32_t bmx_pico_i2c_set_slave_mode(int32_t controller, int32_t enabled, uint32_t address) {
    i2c_inst_t *i2c = bmx_pico_i2c_instance(controller);
    if (!i2c || address > 0x7fu) return 0;
    i2c_set_slave_mode(i2c, enabled != 0, (uint8_t)address);
    return 1;
}

static int32_t bmx_pico_i2c_transfer_valid(i2c_inst_t *i2c, uint32_t address,
        const void *data, int32_t length) {
    return i2c && address <= 0x7fu && length >= 0 && (length == 0 || data);
}

int32_t bmx_pico_i2c_write_blocking(int32_t controller, uint32_t address,
        void *data, int32_t length, int32_t no_stop) {
    i2c_inst_t *i2c = bmx_pico_i2c_instance(controller);
    if (!bmx_pico_i2c_transfer_valid(i2c, address, data, length)) return PICO_ERROR_INVALID_ARG;
    return i2c_write_blocking(i2c, (uint8_t)address, (const uint8_t *)data,
        (size_t)length, no_stop != 0);
}

int32_t bmx_pico_i2c_read_blocking(int32_t controller, uint32_t address,
        void *data, int32_t length, int32_t no_stop) {
    i2c_inst_t *i2c = bmx_pico_i2c_instance(controller);
    if (!bmx_pico_i2c_transfer_valid(i2c, address, data, length)) return PICO_ERROR_INVALID_ARG;
    return i2c_read_blocking(i2c, (uint8_t)address, (uint8_t *)data,
        (size_t)length, no_stop != 0);
}

int32_t bmx_pico_i2c_write_timeout_us(int32_t controller, uint32_t address,
        void *data, int32_t length, int32_t no_stop, uint32_t timeout_us) {
    i2c_inst_t *i2c = bmx_pico_i2c_instance(controller);
    if (!bmx_pico_i2c_transfer_valid(i2c, address, data, length)) return PICO_ERROR_INVALID_ARG;
    return i2c_write_timeout_us(i2c, (uint8_t)address, (const uint8_t *)data,
        (size_t)length, no_stop != 0, timeout_us);
}

int32_t bmx_pico_i2c_read_timeout_us(int32_t controller, uint32_t address,
        void *data, int32_t length, int32_t no_stop, uint32_t timeout_us) {
    i2c_inst_t *i2c = bmx_pico_i2c_instance(controller);
    if (!bmx_pico_i2c_transfer_valid(i2c, address, data, length)) return PICO_ERROR_INVALID_ARG;
    return i2c_read_timeout_us(i2c, (uint8_t)address, (uint8_t *)data,
        (size_t)length, no_stop != 0, timeout_us);
}

uint32_t bmx_pico_i2c_write_available(int32_t controller) {
    i2c_inst_t *i2c = bmx_pico_i2c_instance(controller);
    return i2c ? (uint32_t)i2c_get_write_available(i2c) : 0;
}

uint32_t bmx_pico_i2c_read_available(int32_t controller) {
    i2c_inst_t *i2c = bmx_pico_i2c_instance(controller);
    return i2c ? (uint32_t)i2c_get_read_available(i2c) : 0;
}

int32_t bmx_pico_i2c_write_raw_blocking(int32_t controller,
        void *data, int32_t length) {
    i2c_inst_t *i2c = bmx_pico_i2c_instance(controller);
    if (!bmx_pico_i2c_transfer_valid(i2c, 0, data, length)) return PICO_ERROR_INVALID_ARG;
    i2c_write_raw_blocking(i2c, (const uint8_t *)data, (size_t)length);
    return length;
}

int32_t bmx_pico_i2c_read_raw_blocking(int32_t controller,
        void *data, int32_t length) {
    i2c_inst_t *i2c = bmx_pico_i2c_instance(controller);
    if (!bmx_pico_i2c_transfer_valid(i2c, 0, data, length)) return PICO_ERROR_INVALID_ARG;
    i2c_read_raw_blocking(i2c, (uint8_t *)data, (size_t)length);
    return length;
}

static spi_inst_t *bmx_pico_spi_instance(int32_t controller) {
    if (controller < 0 || controller >= NUM_SPIS) return NULL;
    return SPI_INSTANCE((uint)controller);
}

int32_t bmx_pico_spi_default_controller(void) {
#ifdef PICO_DEFAULT_SPI
    return PICO_DEFAULT_SPI;
#else
    return -1;
#endif
}

uint32_t bmx_pico_spi_default_rx_pin(void) {
#ifdef PICO_DEFAULT_SPI_RX_PIN
    return PICO_DEFAULT_SPI_RX_PIN;
#else
    return UINT32_MAX;
#endif
}

uint32_t bmx_pico_spi_default_tx_pin(void) {
#ifdef PICO_DEFAULT_SPI_TX_PIN
    return PICO_DEFAULT_SPI_TX_PIN;
#else
    return UINT32_MAX;
#endif
}

uint32_t bmx_pico_spi_default_sck_pin(void) {
#ifdef PICO_DEFAULT_SPI_SCK_PIN
    return PICO_DEFAULT_SPI_SCK_PIN;
#else
    return UINT32_MAX;
#endif
}

uint32_t bmx_pico_spi_default_csn_pin(void) {
#ifdef PICO_DEFAULT_SPI_CSN_PIN
    return PICO_DEFAULT_SPI_CSN_PIN;
#else
    return UINT32_MAX;
#endif
}

static int32_t bmx_pico_spi_pin_valid(int32_t controller, uint32_t pin, uint32_t role) {
    return pin < NUM_BANK0_GPIOS && (int32_t)((pin >> 3u) & 1u) == controller &&
        (pin & 3u) == role;
}

int32_t bmx_pico_spi_configure_pins(int32_t controller, uint32_t rx_pin,
        uint32_t tx_pin, uint32_t sck_pin) {
    if (!bmx_pico_spi_instance(controller) ||
            !bmx_pico_spi_pin_valid(controller, rx_pin, 0) ||
            !bmx_pico_spi_pin_valid(controller, tx_pin, 3) ||
            !bmx_pico_spi_pin_valid(controller, sck_pin, 2)) return 0;
    gpio_set_function(rx_pin, GPIO_FUNC_SPI);
    gpio_set_function(tx_pin, GPIO_FUNC_SPI);
    gpio_set_function(sck_pin, GPIO_FUNC_SPI);
    gpio_disable_pulls(rx_pin);
    gpio_disable_pulls(tx_pin);
    gpio_disable_pulls(sck_pin);
    return 1;
}

uint32_t bmx_pico_spi_init(int32_t controller, uint32_t baudrate) {
    spi_inst_t *spi = bmx_pico_spi_instance(controller);
    return spi && baudrate ? spi_init(spi, baudrate) : 0;
}

void bmx_pico_spi_deinit(int32_t controller) {
    spi_inst_t *spi = bmx_pico_spi_instance(controller);
    if (spi) spi_deinit(spi);
}

uint32_t bmx_pico_spi_set_baudrate(int32_t controller, uint32_t baudrate) {
    spi_inst_t *spi = bmx_pico_spi_instance(controller);
    return spi && baudrate ? spi_set_baudrate(spi, baudrate) : 0;
}

uint32_t bmx_pico_spi_get_baudrate(int32_t controller) {
    spi_inst_t *spi = bmx_pico_spi_instance(controller);
    return spi ? spi_get_baudrate(spi) : 0;
}

int32_t bmx_pico_spi_set_format(int32_t controller, uint32_t data_bits,
        uint32_t polarity, uint32_t phase, uint32_t bit_order) {
    spi_inst_t *spi = bmx_pico_spi_instance(controller);
    if (!spi || data_bits < 4 || data_bits > 16 || polarity > 1 || phase > 1 ||
            bit_order != SPI_MSB_FIRST) return 0;
    spi_set_format(spi, data_bits, (spi_cpol_t)polarity, (spi_cpha_t)phase,
        (spi_order_t)bit_order);
    return 1;
}

int32_t bmx_pico_spi_set_peripheral_mode(int32_t controller, int32_t enabled) {
    spi_inst_t *spi = bmx_pico_spi_instance(controller);
    if (!spi) return 0;
    spi_set_slave(spi, enabled != 0);
    return 1;
}

int32_t bmx_pico_spi_is_writable(int32_t controller) {
    spi_inst_t *spi = bmx_pico_spi_instance(controller);
    return spi && spi_is_writable(spi);
}

int32_t bmx_pico_spi_is_readable(int32_t controller) {
    spi_inst_t *spi = bmx_pico_spi_instance(controller);
    return spi && spi_is_readable(spi);
}

int32_t bmx_pico_spi_is_busy(int32_t controller) {
    spi_inst_t *spi = bmx_pico_spi_instance(controller);
    return spi && spi_is_busy(spi);
}

static int32_t bmx_pico_spi_transfer_valid(spi_inst_t *spi, const void *source,
        const void *destination, int32_t length) {
    return spi && length >= 0 && (length == 0 || source || destination);
}

int32_t bmx_pico_spi_write_read_blocking(int32_t controller, void *source,
        void *destination, int32_t length) {
    spi_inst_t *spi = bmx_pico_spi_instance(controller);
    if (!spi || length < 0 || (length && (!source || !destination))) return PICO_ERROR_INVALID_ARG;
    return spi_write_read_blocking(spi, (const uint8_t *)source,
        (uint8_t *)destination, (size_t)length);
}

int32_t bmx_pico_spi_write_blocking(int32_t controller, void *source, int32_t length) {
    spi_inst_t *spi = bmx_pico_spi_instance(controller);
    if (!bmx_pico_spi_transfer_valid(spi, source, NULL, length) || (length && !source))
        return PICO_ERROR_INVALID_ARG;
    return spi_write_blocking(spi, (const uint8_t *)source, (size_t)length);
}

int32_t bmx_pico_spi_read_blocking(int32_t controller, uint32_t repeated_data,
        void *destination, int32_t length) {
    spi_inst_t *spi = bmx_pico_spi_instance(controller);
    if (!bmx_pico_spi_transfer_valid(spi, NULL, destination, length) ||
            repeated_data > 0xffu || (length && !destination)) return PICO_ERROR_INVALID_ARG;
    return spi_read_blocking(spi, (uint8_t)repeated_data,
        (uint8_t *)destination, (size_t)length);
}

int32_t bmx_pico_spi_write16_read16_blocking(int32_t controller, uint16_t *source,
        uint16_t *destination, int32_t length) {
    spi_inst_t *spi = bmx_pico_spi_instance(controller);
    if (!spi || length < 0 || (length && (!source || !destination))) return PICO_ERROR_INVALID_ARG;
    return spi_write16_read16_blocking(spi, source, destination, (size_t)length);
}

int32_t bmx_pico_spi_write16_blocking(int32_t controller, uint16_t *source, int32_t length) {
    spi_inst_t *spi = bmx_pico_spi_instance(controller);
    if (!bmx_pico_spi_transfer_valid(spi, source, NULL, length) || (length && !source))
        return PICO_ERROR_INVALID_ARG;
    return spi_write16_blocking(spi, source, (size_t)length);
}

int32_t bmx_pico_spi_read16_blocking(int32_t controller, uint32_t repeated_data,
        uint16_t *destination, int32_t length) {
    spi_inst_t *spi = bmx_pico_spi_instance(controller);
    if (!bmx_pico_spi_transfer_valid(spi, NULL, destination, length) ||
            repeated_data > 0xffffu || (length && !destination)) return PICO_ERROR_INVALID_ARG;
    return spi_read16_blocking(spi, (uint16_t)repeated_data, destination, (size_t)length);
}

void *bmx_pico_spi_data_register_address(int32_t controller) {
    spi_inst_t *spi = bmx_pico_spi_instance(controller);
    return spi ? (void *)&spi_get_hw(spi)->dr : NULL;
}

uint32_t bmx_pico_spi_tx_dreq(int32_t controller) {
    spi_inst_t *spi = bmx_pico_spi_instance(controller);
    return spi ? spi_get_dreq(spi, true) : UINT32_MAX;
}

uint32_t bmx_pico_spi_rx_dreq(int32_t controller) {
    spi_inst_t *spi = bmx_pico_spi_instance(controller);
    return spi ? spi_get_dreq(spi, false) : UINT32_MAX;
}

static uart_inst_t *bmx_pico_uart_instance(int32_t controller) {
    if (controller < 0 || controller >= NUM_UARTS) return NULL;
    return uart_get_instance((uint)controller);
}

int32_t bmx_pico_uart_default_controller(void) {
#ifdef PICO_DEFAULT_UART
    return PICO_DEFAULT_UART;
#else
    return -1;
#endif
}

uint32_t bmx_pico_uart_default_tx_pin(void) {
#ifdef PICO_DEFAULT_UART_TX_PIN
    return PICO_DEFAULT_UART_TX_PIN;
#else
    return UINT32_MAX;
#endif
}

uint32_t bmx_pico_uart_default_rx_pin(void) {
#ifdef PICO_DEFAULT_UART_RX_PIN
    return PICO_DEFAULT_UART_RX_PIN;
#else
    return UINT32_MAX;
#endif
}

uint32_t bmx_pico_uart_default_baudrate(void) {
    return PICO_DEFAULT_UART_BAUD_RATE;
}

int32_t bmx_pico_uart_supports_auxiliary_pin_mappings(void) {
#if PICO_RP2350
    return 1;
#else
    return 0;
#endif
}

static int32_t bmx_pico_uart_pin_controller(uint32_t pin) {
    uint32_t group = pin >> 2u;
    return (int32_t)((group ^ (group >> 1u)) & 1u);
}

int32_t bmx_pico_uart_configure_pins(int32_t controller, uint32_t tx_pin, uint32_t rx_pin) {
    uart_inst_t *uart = bmx_pico_uart_instance(controller);
    if (!uart || tx_pin >= NUM_BANK0_GPIOS || rx_pin >= NUM_BANK0_GPIOS ||
            (tx_pin & 1u) || !(rx_pin & 1u) ||
            bmx_pico_uart_pin_controller(tx_pin) != controller ||
            bmx_pico_uart_pin_controller(rx_pin) != controller) return 0;
    gpio_set_function(tx_pin, UART_FUNCSEL_NUM(uart, tx_pin));
    gpio_set_function(rx_pin, UART_FUNCSEL_NUM(uart, rx_pin));
    gpio_disable_pulls(tx_pin);
    gpio_disable_pulls(rx_pin);
    return 1;
}

int32_t bmx_pico_uart_configure_flow_pins(int32_t controller,
        uint32_t cts_pin, uint32_t rts_pin) {
    if (!bmx_pico_uart_instance(controller) || cts_pin >= NUM_BANK0_GPIOS ||
            rts_pin >= NUM_BANK0_GPIOS || (cts_pin & 1u) || !(rts_pin & 1u) ||
            bmx_pico_uart_pin_controller(cts_pin) != controller ||
            bmx_pico_uart_pin_controller(rts_pin) != controller) return 0;
#if PICO_RP2350
    gpio_set_function(cts_pin, (cts_pin & 2u) ? GPIO_FUNC_UART : GPIO_FUNC_UART_AUX);
    gpio_set_function(rts_pin, (rts_pin & 2u) ? GPIO_FUNC_UART : GPIO_FUNC_UART_AUX);
#else
    if ((cts_pin & 3u) != 2u || (rts_pin & 3u) != 3u) return 0;
    gpio_set_function(cts_pin, GPIO_FUNC_UART);
    gpio_set_function(rts_pin, GPIO_FUNC_UART);
#endif
    gpio_disable_pulls(cts_pin);
    gpio_disable_pulls(rts_pin);
    return 1;
}

uint32_t bmx_pico_uart_init(int32_t controller, uint32_t baudrate) {
    uart_inst_t *uart = bmx_pico_uart_instance(controller);
    return uart && baudrate ? uart_init(uart, baudrate) : 0;
}

void bmx_pico_uart_deinit(int32_t controller) {
    uart_inst_t *uart = bmx_pico_uart_instance(controller);
    if (uart) uart_deinit(uart);
}

uint32_t bmx_pico_uart_set_baudrate(int32_t controller, uint32_t baudrate) {
    uart_inst_t *uart = bmx_pico_uart_instance(controller);
    return uart && baudrate ? uart_set_baudrate(uart, baudrate) : 0;
}

int32_t bmx_pico_uart_set_format(int32_t controller, uint32_t data_bits,
        uint32_t stop_bits, uint32_t parity) {
    uart_inst_t *uart = bmx_pico_uart_instance(controller);
    if (!uart || data_bits < 5 || data_bits > 8 || stop_bits < 1 || stop_bits > 2 ||
            parity > UART_PARITY_ODD) return 0;
    uart_set_format(uart, data_bits, stop_bits, (uart_parity_t)parity);
    return 1;
}

int32_t bmx_pico_uart_set_flow_control(int32_t controller, int32_t cts, int32_t rts) {
    uart_inst_t *uart = bmx_pico_uart_instance(controller);
    if (!uart) return 0;
    uart_set_hw_flow(uart, cts != 0, rts != 0);
    return 1;
}

int32_t bmx_pico_uart_set_fifo_enabled(int32_t controller, int32_t enabled) {
    uart_inst_t *uart = bmx_pico_uart_instance(controller);
    if (!uart) return 0;
    uart_set_fifo_enabled(uart, enabled != 0);
    return 1;
}

int32_t bmx_pico_uart_is_enabled(int32_t controller) {
    uart_inst_t *uart = bmx_pico_uart_instance(controller);
    return uart && uart_is_enabled(uart);
}

int32_t bmx_pico_uart_is_writable(int32_t controller) {
    uart_inst_t *uart = bmx_pico_uart_instance(controller);
    return uart && uart_is_writable(uart);
}

int32_t bmx_pico_uart_is_readable(int32_t controller) {
    uart_inst_t *uart = bmx_pico_uart_instance(controller);
    return uart && uart_is_readable(uart);
}

int32_t bmx_pico_uart_is_readable_within_us(int32_t controller, uint32_t timeout_us) {
    uart_inst_t *uart = bmx_pico_uart_instance(controller);
    return uart && uart_is_readable_within_us(uart, timeout_us);
}

static int32_t bmx_pico_uart_buffer_valid(uart_inst_t *uart, const void *buffer,
        int32_t length) {
    return uart && length >= 0 && (length == 0 || buffer);
}

int32_t bmx_pico_uart_write_blocking(int32_t controller, void *source, int32_t length) {
    uart_inst_t *uart = bmx_pico_uart_instance(controller);
    if (!bmx_pico_uart_buffer_valid(uart, source, length)) return PICO_ERROR_INVALID_ARG;
    uart_write_blocking(uart, (const uint8_t *)source, (size_t)length);
    return length;
}

int32_t bmx_pico_uart_read_blocking(int32_t controller, void *destination, int32_t length) {
    uart_inst_t *uart = bmx_pico_uart_instance(controller);
    if (!bmx_pico_uart_buffer_valid(uart, destination, length)) return PICO_ERROR_INVALID_ARG;
    uart_read_blocking(uart, (uint8_t *)destination, (size_t)length);
    return length;
}

int32_t bmx_pico_uart_read_timeout_us(int32_t controller, void *destination,
        int32_t length, uint32_t timeout_us) {
    uart_inst_t *uart = bmx_pico_uart_instance(controller);
    if (!bmx_pico_uart_buffer_valid(uart, destination, length)) return PICO_ERROR_INVALID_ARG;
    uint8_t *bytes = (uint8_t *)destination;
    uint64_t deadline = time_us_64() + timeout_us;
    int32_t count = 0;
    while (count < length) {
        while (!uart_is_readable(uart)) {
            if (time_us_64() >= deadline) return count;
            tight_loop_contents();
        }
        bytes[count++] = (uint8_t)uart_get_hw(uart)->dr;
    }
    return count;
}

int32_t bmx_pico_uart_read_available(int32_t controller, void *destination, int32_t capacity) {
    uart_inst_t *uart = bmx_pico_uart_instance(controller);
    if (!bmx_pico_uart_buffer_valid(uart, destination, capacity)) return PICO_ERROR_INVALID_ARG;
    uint8_t *bytes = (uint8_t *)destination;
    int32_t count = 0;
    while (count < capacity && uart_is_readable(uart))
        bytes[count++] = (uint8_t)uart_get_hw(uart)->dr;
    return count;
}

int32_t bmx_pico_uart_put_byte(int32_t controller, uint32_t value) {
    uart_inst_t *uart = bmx_pico_uart_instance(controller);
    if (!uart || value > 0xffu) return 0;
    uart_putc_raw(uart, (char)value);
    return 1;
}

void bmx_pico_uart_tx_wait_blocking(int32_t controller) {
    uart_inst_t *uart = bmx_pico_uart_instance(controller);
    if (uart) uart_tx_wait_blocking(uart);
}

int32_t bmx_pico_uart_set_break(int32_t controller, int32_t enabled) {
    uart_inst_t *uart = bmx_pico_uart_instance(controller);
    if (!uart) return 0;
    uart_set_break(uart, enabled != 0);
    return 1;
}

int32_t bmx_pico_uart_set_translate_crlf(int32_t controller, int32_t enabled) {
    uart_inst_t *uart = bmx_pico_uart_instance(controller);
    if (!uart) return 0;
    uart_set_translate_crlf(uart, enabled != 0);
    return 1;
}

uint32_t bmx_pico_uart_get_errors(int32_t controller) {
    uart_inst_t *uart = bmx_pico_uart_instance(controller);
    return uart ? uart_get_hw(uart)->rsr & 0x0fu : 0;
}

void bmx_pico_uart_clear_errors(int32_t controller) {
    uart_inst_t *uart = bmx_pico_uart_instance(controller);
    if (uart) uart_get_hw(uart)->rsr = 0xffffffffu;
}

typedef struct BMXPicoBufferedUART {
    uint8_t *rx_buffer;
    uint8_t *tx_buffer;
    uint32_t rx_capacity;
    uint32_t tx_capacity;
    volatile uint32_t rx_put;
    volatile uint32_t rx_get;
    volatile uint32_t tx_put;
    volatile uint32_t tx_get;
    volatile uint32_t rx_dropped;
    uint32_t rx_token;
    uint32_t tx_token;
    uint32_t error_token;
    uint8_t open;
} BMXPicoBufferedUART;

static BMXPicoBufferedUART bmx_pico_buffered_uarts[NUM_UARTS];
static uint8_t bmx_pico_uart_irq_installed[NUM_UARTS];

static void bmx_pico_uart_async_irq(uint32_t controller) {
    uart_inst_t *uart = bmx_pico_uart_instance((int32_t)controller);
    BMXPicoBufferedUART *state = &bmx_pico_buffered_uarts[controller];
    if (!uart || !state->open) return;

    uint32_t errors = uart_get_hw(uart)->rsr & 0x0fu;
    if (errors) {
        uart_get_hw(uart)->rsr = 0xffffffffu;
        bmx_pico_event_post_from_irq_ex(state->error_token, errors, controller, 0, 0);
    }

    bool rx_was_empty = state->rx_put == state->rx_get;
    while (uart_is_readable(uart)) {
        uint8_t value = (uint8_t)uart_get_hw(uart)->dr;
        uint32_t put = state->rx_put;
        if (put - state->rx_get >= state->rx_capacity) {
            if (state->rx_dropped != UINT32_MAX) ++state->rx_dropped;
        } else {
            state->rx_buffer[put & (state->rx_capacity - 1u)] = value;
            state->rx_put = put + 1u;
        }
    }
    if (rx_was_empty && state->rx_put != state->rx_get)
        bmx_pico_event_post_from_irq_ex(state->rx_token,
            state->rx_put - state->rx_get, controller, 0, 0);

    bool tx_had_data = state->tx_put != state->tx_get;
    while (state->tx_get != state->tx_put && uart_is_writable(uart)) {
        uart_get_hw(uart)->dr = state->tx_buffer[state->tx_get & (state->tx_capacity - 1u)];
        ++state->tx_get;
    }
    bool tx_empty = state->tx_get == state->tx_put;
    uart_set_irqs_enabled(uart, true, !tx_empty);
    if (tx_had_data && tx_empty)
        bmx_pico_event_post_from_irq_ex(state->tx_token, 0, controller, 0, 0);
}

static void bmx_pico_uart0_async_irq(void) { bmx_pico_uart_async_irq(0); }
#if NUM_UARTS > 1
static void bmx_pico_uart1_async_irq(void) { bmx_pico_uart_async_irq(1); }
#endif

static irq_handler_t bmx_pico_uart_async_handler(uint32_t controller) {
    if (controller == 0) return bmx_pico_uart0_async_irq;
#if NUM_UARTS > 1
    if (controller == 1) return bmx_pico_uart1_async_irq;
#endif
    return NULL;
}

int32_t bmx_pico_uart_async_open(int32_t controller, uint32_t rx_capacity,
        uint32_t tx_capacity, uint32_t rx_token, uint32_t tx_token,
        uint32_t error_token) {
    uart_inst_t *uart = bmx_pico_uart_instance(controller);
    if (get_core_num() != 0 || !uart || !uart_is_enabled(uart) ||
            rx_capacity < 2u || tx_capacity < 2u ||
            (rx_capacity & (rx_capacity - 1u)) != 0u ||
            (tx_capacity & (tx_capacity - 1u)) != 0u ||
            !rx_token || !tx_token || !error_token) return 0;
    BMXPicoBufferedUART *state = &bmx_pico_buffered_uarts[controller];
    if (state->open) return 0;
    uint8_t *rx_buffer = (uint8_t *)bbMemAlloc(rx_capacity);
    if (!rx_buffer) return 0;
    uint8_t *tx_buffer = (uint8_t *)bbMemAlloc(tx_capacity);
    if (!tx_buffer) {
        bbMemFree(rx_buffer);
        return 0;
    }
    memset(state, 0, sizeof(*state));
    state->rx_buffer = rx_buffer;
    state->tx_buffer = tx_buffer;
    state->rx_capacity = rx_capacity;
    state->tx_capacity = tx_capacity;
    state->rx_token = rx_token;
    state->tx_token = tx_token;
    state->error_token = error_token;
    state->open = 1;
    if (!bmx_pico_uart_irq_installed[controller]) {
        irq_add_shared_handler(UART_IRQ_NUM(uart),
            bmx_pico_uart_async_handler((uint32_t)controller),
            PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
        irq_set_enabled(UART_IRQ_NUM(uart), true);
        bmx_pico_uart_irq_installed[controller] = 1;
    }
    uart_set_irqs_enabled(uart, true, false);
    return 1;
}

int32_t bmx_pico_uart_async_close(int32_t controller) {
    uart_inst_t *uart = bmx_pico_uart_instance(controller);
    if (get_core_num() != 0 || !uart) return 0;
    BMXPicoBufferedUART *state = &bmx_pico_buffered_uarts[controller];
    if (!state->open) return 1;
    uart_set_irqs_enabled(uart, false, false);
    uint8_t *rx_buffer = state->rx_buffer;
    uint8_t *tx_buffer = state->tx_buffer;
    memset(state, 0, sizeof(*state));
    bbMemFree(rx_buffer);
    bbMemFree(tx_buffer);
    return 1;
}

int32_t bmx_pico_uart_async_is_open(int32_t controller) {
    return controller >= 0 && controller < NUM_UARTS &&
        bmx_pico_buffered_uarts[controller].open;
}

int32_t bmx_pico_uart_async_read(int32_t controller, void *destination,
        int32_t capacity) {
    if (get_core_num() != 0 || controller < 0 || controller >= NUM_UARTS ||
            capacity < 0 || (capacity && !destination)) return PICO_ERROR_INVALID_ARG;
    BMXPicoBufferedUART *state = &bmx_pico_buffered_uarts[controller];
    if (!state->open) return PICO_ERROR_INVALID_STATE;
    uint8_t *bytes = (uint8_t *)destination;
    uint32_t interrupt_state = save_and_disable_interrupts();
    int32_t count = 0;
    while (count < capacity && state->rx_get != state->rx_put) {
        bytes[count++] = state->rx_buffer[state->rx_get & (state->rx_capacity - 1u)];
        ++state->rx_get;
    }
    restore_interrupts(interrupt_state);
    return count;
}

int32_t bmx_pico_uart_async_write(int32_t controller, void *source,
        int32_t length) {
    uart_inst_t *uart = bmx_pico_uart_instance(controller);
    if (get_core_num() != 0 || !uart || length < 0 || (length && !source))
        return PICO_ERROR_INVALID_ARG;
    BMXPicoBufferedUART *state = &bmx_pico_buffered_uarts[controller];
    if (!state->open) return PICO_ERROR_INVALID_STATE;
    const uint8_t *bytes = (const uint8_t *)source;
    uint32_t interrupt_state = save_and_disable_interrupts();
    int32_t count = 0;
    while (count < length && state->tx_put - state->tx_get < state->tx_capacity) {
        state->tx_buffer[state->tx_put & (state->tx_capacity - 1u)] = bytes[count++];
        ++state->tx_put;
    }
    while (state->tx_get != state->tx_put && uart_is_writable(uart)) {
        uart_get_hw(uart)->dr = state->tx_buffer[state->tx_get & (state->tx_capacity - 1u)];
        ++state->tx_get;
    }
    bool tx_empty = state->tx_get == state->tx_put;
    uart_set_irqs_enabled(uart, true, !tx_empty);
    if (count && tx_empty)
        bmx_pico_event_post_from_irq_ex(state->tx_token, 0, (uint32_t)controller, 0, 0);
    restore_interrupts(interrupt_state);
    return count;
}

uint32_t bmx_pico_uart_async_read_available(int32_t controller) {
    if (controller < 0 || controller >= NUM_UARTS) return 0;
    BMXPicoBufferedUART *state = &bmx_pico_buffered_uarts[controller];
    return state->open ? state->rx_put - state->rx_get : 0;
}

uint32_t bmx_pico_uart_async_write_available(int32_t controller) {
    if (controller < 0 || controller >= NUM_UARTS) return 0;
    BMXPicoBufferedUART *state = &bmx_pico_buffered_uarts[controller];
    return state->open ? state->tx_capacity - (state->tx_put - state->tx_get) : 0;
}

uint32_t bmx_pico_uart_async_rx_dropped(int32_t controller) {
    if (controller < 0 || controller >= NUM_UARTS) return 0;
    return bmx_pico_buffered_uarts[controller].rx_dropped;
}

int32_t bmx_pico_uart_async_tx_idle(int32_t controller) {
    uart_inst_t *uart = bmx_pico_uart_instance(controller);
    if (!uart || controller < 0 || controller >= NUM_UARTS) return 1;
    BMXPicoBufferedUART *state = &bmx_pico_buffered_uarts[controller];
    return !state->open ||
        (state->tx_put == state->tx_get &&
            !(uart_get_hw(uart)->fr & UART_UARTFR_BUSY_BITS));
}

static PIO bmx_pico_pio_instance(int32_t controller) {
    if (controller == 0) return pio0;
    if (controller == 1) return pio1;
#if NUM_PIOS > 2
    if (controller == 2) return pio2;
#endif
    return NULL;
}

static int32_t bmx_pico_pio_state_machine_valid(PIO pio, uint32_t state_machine) {
    return pio && state_machine < NUM_PIO_STATE_MACHINES;
}

static int32_t bmx_pico_pio_pin_span_valid(uint32_t pin_base, uint32_t pin_count) {
    return pin_count > 0 && pin_count <= 32 && pin_base < NUM_BANK0_GPIOS &&
        pin_count <= NUM_BANK0_GPIOS - pin_base;
}

int32_t bmx_pico_pio_apply_config_overrides(void *config_pointer,
        const BMXPicoPIOStateMachineConfig *settings) {
    pio_sm_config *config = config_pointer;
    if (!config) return 0;
    if (!settings) return 1;
    const uint32_t supported = (BMX_PICO_PIO_CONFIG_MOV_STATUS << 1) - 1u;
    if (settings->overrides & ~supported) return 0;
    if (settings->overrides & BMX_PICO_PIO_CONFIG_CLOCK_DIVIDER) {
        if (!(settings->clock_divider >= 1.0f && settings->clock_divider <= 65536.0f))
            return 0;
        sm_config_set_clkdiv(config, settings->clock_divider);
    }
    if (settings->overrides & BMX_PICO_PIO_CONFIG_OUT_PINS) {
        if (!bmx_pico_pio_pin_span_valid(settings->out_pin_base,
                settings->out_pin_count)) return 0;
        sm_config_set_out_pins(config, settings->out_pin_base, settings->out_pin_count);
    }
    if (settings->overrides & BMX_PICO_PIO_CONFIG_SET_PINS) {
        if (!bmx_pico_pio_pin_span_valid(settings->set_pin_base,
                settings->set_pin_count) || settings->set_pin_count > 5) return 0;
        sm_config_set_set_pins(config, settings->set_pin_base, settings->set_pin_count);
    }
    if (settings->overrides & BMX_PICO_PIO_CONFIG_IN_PINS) {
        if (settings->in_pin_base >= NUM_BANK0_GPIOS) return 0;
        sm_config_set_in_pins(config, settings->in_pin_base);
    }
    if (settings->overrides & BMX_PICO_PIO_CONFIG_SIDESET_PINS) {
        if (settings->sideset_pin_base >= NUM_BANK0_GPIOS) return 0;
        sm_config_set_sideset_pins(config, settings->sideset_pin_base);
    }
    if (settings->overrides & BMX_PICO_PIO_CONFIG_JUMP_PIN) {
        if (settings->jump_pin >= NUM_BANK0_GPIOS) return 0;
        sm_config_set_jmp_pin(config, settings->jump_pin);
    }
    if (settings->overrides & BMX_PICO_PIO_CONFIG_IN_SHIFT) {
        if (settings->push_threshold > 32) return 0;
        sm_config_set_in_shift(config, settings->in_shift_right != 0,
            settings->autopush != 0, settings->push_threshold);
    }
    if (settings->overrides & BMX_PICO_PIO_CONFIG_OUT_SHIFT) {
        if (settings->pull_threshold > 32) return 0;
        sm_config_set_out_shift(config, settings->out_shift_right != 0,
            settings->autopull != 0, settings->pull_threshold);
    }
    if (settings->overrides & BMX_PICO_PIO_CONFIG_FIFO_JOIN) {
        if (settings->fifo_join != PIO_FIFO_JOIN_NONE &&
                settings->fifo_join != PIO_FIFO_JOIN_TX &&
                settings->fifo_join != PIO_FIFO_JOIN_RX
#if PICO_PIO_VERSION > 0
                && settings->fifo_join != PIO_FIFO_JOIN_TXGET
                && settings->fifo_join != PIO_FIFO_JOIN_TXPUT
                && settings->fifo_join != PIO_FIFO_JOIN_PUTGET
#endif
                ) return 0;
        sm_config_set_fifo_join(config, (enum pio_fifo_join)settings->fifo_join);
    }
    if (settings->overrides & BMX_PICO_PIO_CONFIG_SIDESET) {
        if (settings->sideset_bit_count > 5 ||
                (settings->sideset_optional && !settings->sideset_bit_count)) return 0;
        sm_config_set_sideset(config, settings->sideset_bit_count,
            settings->sideset_optional != 0, settings->sideset_pindirs != 0);
    }
    if (settings->overrides & BMX_PICO_PIO_CONFIG_OUT_SPECIAL) {
        if (settings->out_enable_bit_index > 31) return 0;
        sm_config_set_out_special(config, settings->out_sticky != 0,
            settings->out_has_enable_pin != 0, settings->out_enable_bit_index);
    }
    if (settings->overrides & BMX_PICO_PIO_CONFIG_MOV_STATUS) {
        if (settings->mov_status_threshold > 31 ||
                (settings->mov_status_type != STATUS_TX_LESSTHAN &&
                 settings->mov_status_type != STATUS_RX_LESSTHAN
#if PICO_PIO_VERSION > 0
                 && settings->mov_status_type != STATUS_IRQ_SET
#endif
                 )) return 0;
        sm_config_set_mov_status(config,
            (enum pio_mov_status_type)settings->mov_status_type,
            settings->mov_status_threshold);
    }
    return 1;
}

static uint32_t bmx_pico_pio_loaded_programs[NUM_PIOS];

static uint32_t bmx_pico_pio_program_mask(uint32_t length, uint32_t offset) {
    uint32_t bits = length == 32 ? UINT32_MAX : ((1u << length) - 1u);
    return bits << offset;
}

static int32_t bmx_pico_pio_program_init(pio_program_t *program, uint16_t *instructions,
        uint32_t length, int32_t origin, uint32_t version, uint32_t used_gpio_ranges) {
    if (!program || !instructions || !length || length > PIO_INSTRUCTION_COUNT ||
            origin < -1 || origin >= (int32_t)PIO_INSTRUCTION_COUNT ||
            (origin >= 0 && length > PIO_INSTRUCTION_COUNT - (uint32_t)origin) ||
            version > PICO_PIO_VERSION || used_gpio_ranges > UINT8_MAX) return 0;
#if PICO_PIO_VERSION == 0
    if (used_gpio_ranges) return 0;
#endif
    program->instructions = instructions;
    program->length = (uint8_t)length;
    program->origin = (int8_t)origin;
    program->pio_version = (uint8_t)version;
#if PICO_PIO_VERSION > 0
    program->used_gpio_ranges = (uint8_t)used_gpio_ranges;
#else
    (void)used_gpio_ranges;
#endif
    return 1;
}

uint32_t bmx_pico_pio_count(void) { return NUM_PIOS; }
uint32_t bmx_pico_pio_version(void) { return PICO_PIO_VERSION; }
uint32_t bmx_pico_pio_state_machine_count(void) { return NUM_PIO_STATE_MACHINES; }
uint32_t bmx_pico_pio_instruction_capacity(void) { return PIO_INSTRUCTION_COUNT; }

void *bmx_pico_pio_find_program(const BMXEmbeddedString *name) {
    if (!name || name->length <= 0) return NULL;
    for (uint32_t index = 0; index < bmx_pico_imported_pio_program_count; ++index) {
        const BMXPicoPIOProgramDescriptor *program = &bmx_pico_imported_pio_programs[index];
        uint32_t length = 0;
        while (program->name[length]) ++length;
        if (length != (uint32_t)name->length) continue;
        uint32_t character = 0;
        while (character < length && name->buf[character] == (uint8_t)program->name[character])
            ++character;
        if (character == length) return (void *)program;
    }
    return NULL;
}

uint16_t *bmx_pico_pio_program_instructions(void *handle) {
    const BMXPicoPIOProgramDescriptor *program = handle;
    return program ? (uint16_t *)program->instructions : NULL;
}

uint32_t bmx_pico_pio_program_length(void *handle) {
    const BMXPicoPIOProgramDescriptor *program = handle;
    return program ? program->length : 0;
}

int32_t bmx_pico_pio_program_origin(void *handle) {
    const BMXPicoPIOProgramDescriptor *program = handle;
    return program ? program->origin : PICO_ERROR_INVALID_ARG;
}

uint32_t bmx_pico_pio_program_version(void *handle) {
    const BMXPicoPIOProgramDescriptor *program = handle;
    return program ? program->version : UINT32_MAX;
}

uint32_t bmx_pico_pio_program_used_gpio_ranges(void *handle) {
    const BMXPicoPIOProgramDescriptor *program = handle;
    return program ? program->used_gpio_ranges : 0;
}

uint32_t bmx_pico_pio_program_wrap_target(void *handle) {
    const BMXPicoPIOProgramDescriptor *program = handle;
    return program ? program->wrap_target : 0;
}

uint32_t bmx_pico_pio_program_wrap(void *handle) {
    const BMXPicoPIOProgramDescriptor *program = handle;
    return program ? program->wrap : 0;
}

int32_t bmx_pico_pio_can_add_program(int32_t controller, uint16_t *instructions,
        uint32_t length, int32_t origin, uint32_t version, uint32_t used_gpio_ranges) {
    PIO pio = bmx_pico_pio_instance(controller);
    pio_program_t program;
    if (!pio || !bmx_pico_pio_program_init(&program, instructions, length, origin,
            version, used_gpio_ranges)) return 0;
    if (origin >= 0) return pio_can_add_program_at_offset(pio, &program, (uint32_t)origin);
    return pio_can_add_program(pio, &program);
}

int32_t bmx_pico_pio_add_program(int32_t controller, uint16_t *instructions,
        uint32_t length, int32_t origin, uint32_t version, uint32_t used_gpio_ranges) {
    PIO pio = bmx_pico_pio_instance(controller);
    pio_program_t program;
    if (!pio || !bmx_pico_pio_program_init(&program, instructions, length, origin,
            version, used_gpio_ranges)) return PICO_ERROR_INVALID_ARG;
    int offset = origin >= 0 ? pio_add_program_at_offset(pio, &program, (uint32_t)origin) :
        pio_add_program(pio, &program);
    if (offset >= 0) {
        bmx_pico_pio_loaded_programs[controller] |=
            bmx_pico_pio_program_mask(length, (uint32_t)offset);
    }
    return offset;
}

int32_t bmx_pico_pio_can_add_imported_program(int32_t controller, void *handle) {
    const BMXPicoPIOProgramDescriptor *program = handle;
    if (!program) return 0;
    return bmx_pico_pio_can_add_program(controller, (uint16_t *)program->instructions, program->length,
        program->origin, program->version, program->used_gpio_ranges);
}

int32_t bmx_pico_pio_add_imported_program(int32_t controller, void *handle) {
    const BMXPicoPIOProgramDescriptor *program = handle;
    if (!program) return PICO_ERROR_INVALID_ARG;
    return bmx_pico_pio_add_program(controller, (uint16_t *)program->instructions, program->length,
        program->origin, program->version, program->used_gpio_ranges);
}

int32_t bmx_pico_pio_sm_init_imported_program(int32_t controller, uint32_t state_machine,
        void *handle, uint32_t offset) {
    const BMXPicoPIOProgramDescriptor *program = handle;
    PIO pio = bmx_pico_pio_instance(controller);
    if (!bmx_pico_pio_state_machine_valid(pio, state_machine) || !program ||
            !program->initialize || offset >= PIO_INSTRUCTION_COUNT ||
            program->length > PIO_INSTRUCTION_COUNT - offset) return PICO_ERROR_INVALID_ARG;
    return program->initialize(pio, state_machine, offset, NULL);
}

int32_t bmx_pico_pio_sm_init_imported_program_configured(int32_t controller,
        uint32_t state_machine, void *handle, uint32_t offset, void *config_pointer) {
    const BMXPicoPIOProgramDescriptor *program = handle;
    const BMXPicoPIOStateMachineConfig *config = config_pointer;
    PIO pio = bmx_pico_pio_instance(controller);
    if (!bmx_pico_pio_state_machine_valid(pio, state_machine) || !program ||
            !program->initialize || !config || offset >= PIO_INSTRUCTION_COUNT ||
            program->length > PIO_INSTRUCTION_COUNT - offset)
        return PICO_ERROR_INVALID_ARG;
    return program->initialize(pio, state_machine, offset, config);
}

int32_t bmx_pico_pio_remove_program(int32_t controller, uint32_t length, uint32_t offset) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (!pio || !length || length > PIO_INSTRUCTION_COUNT ||
            offset >= PIO_INSTRUCTION_COUNT || length > PIO_INSTRUCTION_COUNT - offset)
        return 0;
    uint32_t mask = bmx_pico_pio_program_mask(length, offset);
    if ((bmx_pico_pio_loaded_programs[controller] & mask) != mask) return 0;
    pio_program_t program = {.instructions = NULL, .length = (uint8_t)length,
        .origin = -1, .pio_version = 0
#if PICO_PIO_VERSION > 0
        , .used_gpio_ranges = 0
#endif
    };
    pio_remove_program(pio, &program, offset);
    bmx_pico_pio_loaded_programs[controller] &= ~mask;
    return 1;
}

int32_t bmx_pico_pio_claim_unused_state_machine(int32_t controller) {
    PIO pio = bmx_pico_pio_instance(controller);
    return pio ? pio_claim_unused_sm(pio, false) : PICO_ERROR_INVALID_ARG;
}

int32_t bmx_pico_pio_unclaim_state_machine(int32_t controller, uint32_t state_machine) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (!bmx_pico_pio_state_machine_valid(pio, state_machine) ||
            !pio_sm_is_claimed(pio, state_machine)) return 0;
    pio_sm_unclaim(pio, state_machine);
    return 1;
}

int32_t bmx_pico_pio_state_machine_is_claimed(int32_t controller, uint32_t state_machine) {
    PIO pio = bmx_pico_pio_instance(controller);
    return bmx_pico_pio_state_machine_valid(pio, state_machine) &&
        pio_sm_is_claimed(pio, state_machine);
}

int32_t bmx_pico_pio_gpio_init(int32_t controller, uint32_t pin) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (!pio || pin >= NUM_BANK0_GPIOS) return 0;
    pio_gpio_init(pio, pin);
    return 1;
}

uint32_t bmx_pico_pio_gpio_base(int32_t controller) {
    PIO pio = bmx_pico_pio_instance(controller);
    return pio ? pio_get_gpio_base(pio) : UINT32_MAX;
}

int32_t bmx_pico_pio_set_gpio_base(int32_t controller, uint32_t gpio_base) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (!pio || (gpio_base != 0 && gpio_base != 16)) return 0;
    return pio_set_gpio_base(pio, gpio_base) == PICO_OK;
}

int32_t bmx_pico_pio_sm_set_consecutive_pin_directions(int32_t controller,
        uint32_t state_machine, uint32_t pin_base, uint32_t pin_count, int32_t output) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (!bmx_pico_pio_state_machine_valid(pio, state_machine) ||
            !bmx_pico_pio_pin_span_valid(pin_base, pin_count)) return PICO_ERROR_INVALID_ARG;
    return pio_sm_set_consecutive_pindirs(pio, state_machine, pin_base, pin_count,
        output != 0);
}

int32_t bmx_pico_pio_sm_init(int32_t controller, uint32_t state_machine,
        uint32_t initial_pc) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (!bmx_pico_pio_state_machine_valid(pio, state_machine) ||
            initial_pc >= PIO_INSTRUCTION_COUNT) return PICO_ERROR_INVALID_ARG;
    return pio_sm_init(pio, state_machine, initial_pc, NULL);
}

int32_t bmx_pico_pio_sm_init_configured(int32_t controller, uint32_t state_machine,
        uint32_t initial_pc, void *settings_pointer) {
    PIO pio = bmx_pico_pio_instance(controller);
    const BMXPicoPIOStateMachineConfig *settings = settings_pointer;
    if (!bmx_pico_pio_state_machine_valid(pio, state_machine) || !settings ||
            initial_pc >= PIO_INSTRUCTION_COUNT) return PICO_ERROR_INVALID_ARG;
    pio_sm_config config = pio_get_default_sm_config();
    if (!bmx_pico_pio_apply_config_overrides(&config, settings))
        return PICO_ERROR_INVALID_ARG;
    return pio_sm_init(pio, state_machine, initial_pc, &config);
}

int32_t bmx_pico_pio_sm_set_wrap(int32_t controller, uint32_t state_machine,
        uint32_t wrap_target, uint32_t wrap) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (!bmx_pico_pio_state_machine_valid(pio, state_machine) ||
            wrap_target >= PIO_INSTRUCTION_COUNT || wrap >= PIO_INSTRUCTION_COUNT ||
            wrap_target > wrap) return 0;
    pio_sm_set_wrap(pio, state_machine, wrap_target, wrap);
    return 1;
}

int32_t bmx_pico_pio_sm_set_out_pins(int32_t controller, uint32_t state_machine,
        uint32_t pin_base, uint32_t pin_count) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (!bmx_pico_pio_state_machine_valid(pio, state_machine) ||
            !bmx_pico_pio_pin_span_valid(pin_base, pin_count)) return 0;
    pio_sm_set_out_pins(pio, state_machine, pin_base, pin_count);
    return 1;
}

int32_t bmx_pico_pio_sm_set_set_pins(int32_t controller, uint32_t state_machine,
        uint32_t pin_base, uint32_t pin_count) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (!bmx_pico_pio_state_machine_valid(pio, state_machine) ||
            !bmx_pico_pio_pin_span_valid(pin_base, pin_count) || pin_count > 5) return 0;
    pio_sm_set_set_pins(pio, state_machine, pin_base, pin_count);
    return 1;
}

int32_t bmx_pico_pio_sm_set_in_pins(int32_t controller, uint32_t state_machine,
        uint32_t pin_base) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (!bmx_pico_pio_state_machine_valid(pio, state_machine) ||
            pin_base >= NUM_BANK0_GPIOS) return 0;
    pio_sm_set_in_pins(pio, state_machine, pin_base);
    return 1;
}

int32_t bmx_pico_pio_sm_set_sideset_pins(int32_t controller, uint32_t state_machine,
        uint32_t pin_base) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (!bmx_pico_pio_state_machine_valid(pio, state_machine) ||
            pin_base >= NUM_BANK0_GPIOS) return 0;
    pio_sm_set_sideset_pins(pio, state_machine, pin_base);
    return 1;
}

int32_t bmx_pico_pio_sm_set_jump_pin(int32_t controller, uint32_t state_machine,
        uint32_t pin) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (!bmx_pico_pio_state_machine_valid(pio, state_machine) || pin >= NUM_BANK0_GPIOS)
        return 0;
    pio_sm_set_jmp_pin(pio, state_machine, pin);
    return 1;
}

int32_t bmx_pico_pio_sm_set_clock_divider(int32_t controller, uint32_t state_machine,
        float divider) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (!bmx_pico_pio_state_machine_valid(pio, state_machine) || divider < 1.0f ||
            divider > 65536.0f) return 0;
    pio_sm_set_clkdiv(pio, state_machine, divider);
    return 1;
}

int32_t bmx_pico_pio_sm_set_enabled(int32_t controller, uint32_t state_machine,
        int32_t enabled) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (!bmx_pico_pio_state_machine_valid(pio, state_machine)) return 0;
    pio_sm_set_enabled(pio, state_machine, enabled != 0);
    return 1;
}

static int32_t bmx_pico_pio_sm_mask_valid(uint32_t state_machine_mask) {
    uint32_t supported = (1u << NUM_PIO_STATE_MACHINES) - 1u;
    return state_machine_mask && !(state_machine_mask & ~supported);
}

int32_t bmx_pico_pio_sm_mask_set_enabled(int32_t controller,
        uint32_t state_machine_mask, int32_t enabled) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (!pio || !bmx_pico_pio_sm_mask_valid(state_machine_mask)) return 0;
    pio_set_sm_mask_enabled(pio, state_machine_mask, enabled != 0);
    return 1;
}

int32_t bmx_pico_pio_sm_mask_restart(int32_t controller,
        uint32_t state_machine_mask) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (!pio || !bmx_pico_pio_sm_mask_valid(state_machine_mask)) return 0;
    pio_restart_sm_mask(pio, state_machine_mask);
    return 1;
}

int32_t bmx_pico_pio_sm_mask_restart_clock_divider(int32_t controller,
        uint32_t state_machine_mask) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (!pio || !bmx_pico_pio_sm_mask_valid(state_machine_mask)) return 0;
    pio_clkdiv_restart_sm_mask(pio, state_machine_mask);
    return 1;
}

int32_t bmx_pico_pio_sm_mask_enable_synchronized(int32_t controller,
        uint32_t state_machine_mask) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (!pio || !bmx_pico_pio_sm_mask_valid(state_machine_mask)) return 0;
    pio_enable_sm_mask_in_sync(pio, state_machine_mask);
    return 1;
}

int32_t bmx_pico_pio_sm_restart(int32_t controller, uint32_t state_machine) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (!bmx_pico_pio_state_machine_valid(pio, state_machine)) return 0;
    pio_sm_restart(pio, state_machine);
    return 1;
}

int32_t bmx_pico_pio_sm_restart_clock_divider(int32_t controller,
        uint32_t state_machine) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (!bmx_pico_pio_state_machine_valid(pio, state_machine)) return 0;
    pio_sm_clkdiv_restart(pio, state_machine);
    return 1;
}

int32_t bmx_pico_pio_sm_clear_fifos(int32_t controller, uint32_t state_machine) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (!bmx_pico_pio_state_machine_valid(pio, state_machine)) return 0;
    pio_sm_clear_fifos(pio, state_machine);
    return 1;
}

int32_t bmx_pico_pio_sm_execute(int32_t controller, uint32_t state_machine,
        uint32_t instruction) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (!bmx_pico_pio_state_machine_valid(pio, state_machine) || instruction > UINT16_MAX)
        return 0;
    pio_sm_exec(pio, state_machine, instruction);
    return 1;
}

int32_t bmx_pico_pio_sm_execute_blocking(int32_t controller, uint32_t state_machine,
        uint32_t instruction) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (!bmx_pico_pio_state_machine_valid(pio, state_machine) || instruction > UINT16_MAX)
        return 0;
    pio_sm_exec_wait_blocking(pio, state_machine, instruction);
    return 1;
}

int32_t bmx_pico_pio_sm_execute_stalled(int32_t controller, uint32_t state_machine) {
    PIO pio = bmx_pico_pio_instance(controller);
    return bmx_pico_pio_state_machine_valid(pio, state_machine) &&
        pio_sm_is_exec_stalled(pio, state_machine);
}

int32_t bmx_pico_pio_sm_drain_tx_fifo(int32_t controller, uint32_t state_machine) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (!bmx_pico_pio_state_machine_valid(pio, state_machine)) return 0;
    pio_sm_drain_tx_fifo(pio, state_machine);
    return 1;
}

int32_t bmx_pico_pio_sm_set_pins_masked(int32_t controller, uint32_t state_machine,
        uint64_t pin_values, uint64_t pin_mask) {
    PIO pio = bmx_pico_pio_instance(controller);
    uint64_t valid_mask = NUM_BANK0_GPIOS == 64 ? UINT64_MAX :
        ((1ull << NUM_BANK0_GPIOS) - 1ull);
    if (!bmx_pico_pio_state_machine_valid(pio, state_machine) || !pin_mask ||
            (pin_mask & ~valid_mask)) return 0;
    pio_sm_set_pins_with_mask64(pio, state_machine, pin_values, pin_mask);
    return 1;
}

int32_t bmx_pico_pio_sm_set_pin_directions_masked(int32_t controller,
        uint32_t state_machine, uint64_t pin_directions, uint64_t pin_mask) {
    PIO pio = bmx_pico_pio_instance(controller);
    uint64_t valid_mask = NUM_BANK0_GPIOS == 64 ? UINT64_MAX :
        ((1ull << NUM_BANK0_GPIOS) - 1ull);
    if (!bmx_pico_pio_state_machine_valid(pio, state_machine) || !pin_mask ||
            (pin_mask & ~valid_mask)) return 0;
    pio_sm_set_pindirs_with_mask64(pio, state_machine, pin_directions, pin_mask);
    return 1;
}

uint32_t bmx_pico_pio_sm_program_counter(int32_t controller, uint32_t state_machine) {
    PIO pio = bmx_pico_pio_instance(controller);
    return bmx_pico_pio_state_machine_valid(pio, state_machine) ?
        pio_sm_get_pc(pio, state_machine) : UINT32_MAX;
}

int32_t bmx_pico_pio_sm_tx_full(int32_t controller, uint32_t state_machine) {
    PIO pio = bmx_pico_pio_instance(controller);
    return bmx_pico_pio_state_machine_valid(pio, state_machine) &&
        pio_sm_is_tx_fifo_full(pio, state_machine);
}

int32_t bmx_pico_pio_sm_tx_empty(int32_t controller, uint32_t state_machine) {
    PIO pio = bmx_pico_pio_instance(controller);
    return bmx_pico_pio_state_machine_valid(pio, state_machine) &&
        pio_sm_is_tx_fifo_empty(pio, state_machine);
}

int32_t bmx_pico_pio_sm_rx_full(int32_t controller, uint32_t state_machine) {
    PIO pio = bmx_pico_pio_instance(controller);
    return bmx_pico_pio_state_machine_valid(pio, state_machine) &&
        pio_sm_is_rx_fifo_full(pio, state_machine);
}

int32_t bmx_pico_pio_sm_rx_empty(int32_t controller, uint32_t state_machine) {
    PIO pio = bmx_pico_pio_instance(controller);
    return bmx_pico_pio_state_machine_valid(pio, state_machine) &&
        pio_sm_is_rx_fifo_empty(pio, state_machine);
}

uint32_t bmx_pico_pio_sm_tx_level(int32_t controller, uint32_t state_machine) {
    PIO pio = bmx_pico_pio_instance(controller);
    return bmx_pico_pio_state_machine_valid(pio, state_machine) ?
        pio_sm_get_tx_fifo_level(pio, state_machine) : 0;
}

uint32_t bmx_pico_pio_sm_rx_level(int32_t controller, uint32_t state_machine) {
    PIO pio = bmx_pico_pio_instance(controller);
    return bmx_pico_pio_state_machine_valid(pio, state_machine) ?
        pio_sm_get_rx_fifo_level(pio, state_machine) : 0;
}

int32_t bmx_pico_pio_sm_put(int32_t controller, uint32_t state_machine, uint32_t value) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (!bmx_pico_pio_state_machine_valid(pio, state_machine) ||
            pio_sm_is_tx_fifo_full(pio, state_machine)) return 0;
    pio_sm_put(pio, state_machine, value);
    return 1;
}

int32_t bmx_pico_pio_sm_put_blocking(int32_t controller, uint32_t state_machine,
        uint32_t value) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (!bmx_pico_pio_state_machine_valid(pio, state_machine)) return 0;
    pio_sm_put_blocking(pio, state_machine, value);
    return 1;
}

uint32_t bmx_pico_pio_sm_get(int32_t controller, uint32_t state_machine) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (!bmx_pico_pio_state_machine_valid(pio, state_machine) ||
            pio_sm_is_rx_fifo_empty(pio, state_machine)) return 0;
    return pio_sm_get(pio, state_machine);
}

uint32_t bmx_pico_pio_sm_get_blocking(int32_t controller, uint32_t state_machine) {
    PIO pio = bmx_pico_pio_instance(controller);
    return bmx_pico_pio_state_machine_valid(pio, state_machine) ?
        pio_sm_get_blocking(pio, state_machine) : 0;
}

void *bmx_pico_pio_sm_tx_fifo_address(int32_t controller, uint32_t state_machine) {
    PIO pio = bmx_pico_pio_instance(controller);
    return bmx_pico_pio_state_machine_valid(pio, state_machine) ?
        (void *)&pio->txf[state_machine] : NULL;
}

void *bmx_pico_pio_sm_rx_fifo_address(int32_t controller, uint32_t state_machine) {
    PIO pio = bmx_pico_pio_instance(controller);
    return bmx_pico_pio_state_machine_valid(pio, state_machine) ?
        (void *)&pio->rxf[state_machine] : NULL;
}

uint32_t bmx_pico_pio_sm_dreq(int32_t controller, uint32_t state_machine,
        int32_t transmit) {
    PIO pio = bmx_pico_pio_instance(controller);
    return bmx_pico_pio_state_machine_valid(pio, state_machine) ?
        pio_get_dreq(pio, state_machine, transmit != 0) : UINT32_MAX;
}

static volatile uint32_t bmx_pico_pio_irq_events[NUM_PIOS][NUM_PIO_IRQS];
static volatile uint32_t bmx_pico_pio_irq_source_masks[NUM_PIOS][NUM_PIO_IRQS];
static volatile uint32_t bmx_pico_pio_irq_event_tokens[NUM_PIOS][NUM_PIO_IRQS];
static volatile uint32_t bmx_pico_pio_irq_event_source_masks[NUM_PIOS][NUM_PIO_IRQS];
static uint8_t bmx_pico_pio_irq_installed[NUM_PIOS][NUM_PIO_IRQS];

static void bmx_pico_pio_irq_handler(uint32_t controller, uint32_t irq_line) {
    PIO pio = pio_get_instance(controller);
    uint32_t pending = pio->irq_ctrl[irq_line].ints &
        bmx_pico_pio_irq_source_masks[controller][irq_line];
    if (!pending) return;
    hw_clear_bits(&pio->irq_ctrl[irq_line].inte, pending);
    bmx_pico_pio_irq_events[controller][irq_line] |= pending;
    uint32_t event_pending = pending &
        bmx_pico_pio_irq_event_source_masks[controller][irq_line];
    if (event_pending) {
        uint64_t captured_at = time_us_64();
        bmx_pico_event_post_from_irq_ex(
            bmx_pico_pio_irq_event_tokens[controller][irq_line], event_pending, controller,
            (uint32_t)captured_at, (uint32_t)(captured_at >> 32));
    }
}

static void bmx_pico_pio0_irq0_handler(void) { bmx_pico_pio_irq_handler(0, 0); }
static void bmx_pico_pio0_irq1_handler(void) { bmx_pico_pio_irq_handler(0, 1); }
static void bmx_pico_pio1_irq0_handler(void) { bmx_pico_pio_irq_handler(1, 0); }
static void bmx_pico_pio1_irq1_handler(void) { bmx_pico_pio_irq_handler(1, 1); }
#if NUM_PIOS > 2
static void bmx_pico_pio2_irq0_handler(void) { bmx_pico_pio_irq_handler(2, 0); }
static void bmx_pico_pio2_irq1_handler(void) { bmx_pico_pio_irq_handler(2, 1); }
#endif

static irq_handler_t bmx_pico_pio_irq_handler_for(uint32_t controller, uint32_t irq_line) {
    static irq_handler_t const handlers[2][NUM_PIO_IRQS] = {
        {bmx_pico_pio0_irq0_handler, bmx_pico_pio0_irq1_handler},
        {bmx_pico_pio1_irq0_handler, bmx_pico_pio1_irq1_handler}
    };
    if (controller < 2) return handlers[controller][irq_line];
#if NUM_PIOS > 2
    return irq_line ? bmx_pico_pio2_irq1_handler : bmx_pico_pio2_irq0_handler;
#else
    return NULL;
#endif
}

uint32_t bmx_pico_pio_interrupt_count(void) {
    return PICO_PIO_VERSION > 0 ? 8u : 4u;
}

uint32_t bmx_pico_pio_irq_supported_sources(void) {
    return PIO_INTR_BITS;
}

int32_t bmx_pico_pio_irq_set_sources_enabled(int32_t controller, uint32_t irq_line,
        uint32_t source_mask, int32_t enabled) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (get_core_num() != 0 || !pio || irq_line >= NUM_PIO_IRQS || !source_mask ||
            (source_mask & ~PIO_INTR_BITS)) return 0;
    uint32_t instance = PIO_NUM(pio);
    uint32_t interrupt_state = save_and_disable_interrupts();
    if (enabled && !bmx_pico_pio_irq_installed[instance][irq_line]) {
        irq_add_shared_handler(pio_get_irq_num(pio, irq_line),
            bmx_pico_pio_irq_handler_for(instance, irq_line),
            PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
        irq_set_enabled(pio_get_irq_num(pio, irq_line), true);
        bmx_pico_pio_irq_installed[instance][irq_line] = 1;
    }
    if (enabled) {
        bmx_pico_pio_irq_events[instance][irq_line] &= ~source_mask;
        bmx_pico_pio_irq_source_masks[instance][irq_line] |= source_mask;
    } else {
        bmx_pico_pio_irq_source_masks[instance][irq_line] &= ~source_mask;
        bmx_pico_pio_irq_events[instance][irq_line] &= ~source_mask;
    }
    pio_set_irqn_source_mask_enabled(pio, irq_line, source_mask, enabled != 0);
    restore_interrupts(interrupt_state);
    return 1;
}

int32_t bmx_pico_pio_irq_set_event_token(int32_t controller, uint32_t irq_line,
        uint32_t source_mask, uint32_t token) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (get_core_num() != 0 || !pio || irq_line >= NUM_PIO_IRQS ||
            (source_mask & ~PIO_INTR_BITS) || (token && !source_mask)) return 0;
    uint32_t instance = PIO_NUM(pio);
    uint32_t interrupt_state = save_and_disable_interrupts();
    if (token && bmx_pico_pio_irq_event_tokens[instance][irq_line] &&
            bmx_pico_pio_irq_event_tokens[instance][irq_line] != token) {
        restore_interrupts(interrupt_state);
        return 0;
    }
    bmx_pico_pio_irq_event_tokens[instance][irq_line] = token;
    bmx_pico_pio_irq_event_source_masks[instance][irq_line] = token ? source_mask : 0;
    restore_interrupts(interrupt_state);
    return 1;
}

uint32_t bmx_pico_pio_irq_enabled_sources(int32_t controller, uint32_t irq_line) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (get_core_num() != 0 || !pio || irq_line >= NUM_PIO_IRQS) return 0;
    return bmx_pico_pio_irq_source_masks[PIO_NUM(pio)][irq_line];
}

uint32_t bmx_pico_pio_irq_armed_sources(int32_t controller, uint32_t irq_line) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (get_core_num() != 0 || !pio || irq_line >= NUM_PIO_IRQS) return 0;
    return pio->irq_ctrl[irq_line].inte &
        bmx_pico_pio_irq_source_masks[PIO_NUM(pio)][irq_line];
}

uint32_t bmx_pico_pio_irq_pending_events(int32_t controller, uint32_t irq_line) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (get_core_num() != 0 || !pio || irq_line >= NUM_PIO_IRQS) return 0;
    return bmx_pico_pio_irq_events[PIO_NUM(pio)][irq_line];
}

uint32_t bmx_pico_pio_irq_take_events(int32_t controller, uint32_t irq_line) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (get_core_num() != 0 || !pio || irq_line >= NUM_PIO_IRQS) return 0;
    uint32_t interrupt_state = save_and_disable_interrupts();
    uint32_t events = bmx_pico_pio_irq_events[PIO_NUM(pio)][irq_line];
    bmx_pico_pio_irq_events[PIO_NUM(pio)][irq_line] = 0;
    restore_interrupts(interrupt_state);
    return events;
}

uint32_t bmx_pico_pio_irq_take_events_masked(int32_t controller, uint32_t irq_line,
        uint32_t source_mask) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (get_core_num() != 0 || !pio || irq_line >= NUM_PIO_IRQS || !source_mask ||
            (source_mask & ~PIO_INTR_BITS)) return 0;
    uint32_t interrupt_state = save_and_disable_interrupts();
    uint32_t instance = PIO_NUM(pio);
    uint32_t events = bmx_pico_pio_irq_events[instance][irq_line] & source_mask;
    bmx_pico_pio_irq_events[instance][irq_line] &= ~source_mask;
    restore_interrupts(interrupt_state);
    return events;
}

int32_t bmx_pico_pio_irq_rearm_sources(int32_t controller, uint32_t irq_line,
        uint32_t source_mask) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (get_core_num() != 0 || !pio || irq_line >= NUM_PIO_IRQS || !source_mask ||
            (source_mask & ~PIO_INTR_BITS)) return 0;
    uint32_t instance = PIO_NUM(pio);
    uint32_t interrupt_state = save_and_disable_interrupts();
    if (source_mask & ~bmx_pico_pio_irq_source_masks[instance][irq_line]) {
        restore_interrupts(interrupt_state);
        return 0;
    }
    pio_set_irqn_source_mask_enabled(pio, irq_line, source_mask, true);
    restore_interrupts(interrupt_state);
    return 1;
}

int32_t bmx_pico_pio_interrupt_is_set(int32_t controller, uint32_t interrupt_number) {
    PIO pio = bmx_pico_pio_instance(controller);
    return pio && interrupt_number < bmx_pico_pio_interrupt_count() &&
        pio_interrupt_get(pio, interrupt_number);
}

int32_t bmx_pico_pio_interrupt_clear(int32_t controller, uint32_t interrupt_number) {
    PIO pio = bmx_pico_pio_instance(controller);
    if (get_core_num() != 0 || !pio ||
            interrupt_number >= bmx_pico_pio_interrupt_count()) return 0;
    pio_interrupt_clear(pio, interrupt_number);
    return 1;
}

#define BMX_PICO_EVENT_CAPACITY 32u
#define BMX_PICO_EVENT_MASK (BMX_PICO_EVENT_CAPACITY - 1u)

typedef struct BMXPicoDeferredEvent {
    uint32_t token;
    uint32_t event_data;
    uint32_t event_mods;
    uint32_t event_x;
    uint32_t event_y;
} BMXPicoDeferredEvent;

static BMXPicoDeferredEvent bmx_pico_deferred_events[BMX_PICO_EVENT_CAPACITY];
static volatile uint32_t bmx_pico_deferred_event_put;
static volatile uint32_t bmx_pico_deferred_event_get;
static volatile uint32_t bmx_pico_deferred_event_dropped;

int32_t bmx_pico_event_post_from_irq(uint32_t token, uint32_t event_data,
        uint32_t detail) {
    return bmx_pico_event_post_from_irq_ex(token, event_data, 0, detail, 0);
}

int32_t bmx_pico_event_post_from_irq_ex(uint32_t token, uint32_t event_data,
        uint32_t event_mods, uint32_t event_x, uint32_t event_y) {
    if (!token) return 0;
    uint32_t put = bmx_pico_deferred_event_put;
    if (put - bmx_pico_deferred_event_get >= BMX_PICO_EVENT_CAPACITY) {
        if (bmx_pico_deferred_event_dropped != UINT32_MAX)
            ++bmx_pico_deferred_event_dropped;
        __sev();
        return 0;
    }
    BMXPicoDeferredEvent *event = &bmx_pico_deferred_events[put & BMX_PICO_EVENT_MASK];
    event->token = token;
    event->event_data = event_data;
    event->event_mods = event_mods;
    event->event_x = event_x;
    event->event_y = event_y;
    __dmb();
    bmx_pico_deferred_event_put = put + 1u;
    __sev();
    return 1;
}

int32_t bmx_pico_event_take(uint32_t *token, uint32_t *event_data,
        uint32_t *event_mods, uint32_t *event_x, uint32_t *event_y) {
    if (!token || !event_data || !event_mods || !event_x || !event_y ||
            get_core_num() != 0) return 0;
    uint32_t interrupt_state = save_and_disable_interrupts();
    uint32_t get = bmx_pico_deferred_event_get;
    if (get == bmx_pico_deferred_event_put) {
        restore_interrupts(interrupt_state);
        return 0;
    }
    BMXPicoDeferredEvent event = bmx_pico_deferred_events[get & BMX_PICO_EVENT_MASK];
    bmx_pico_deferred_event_get = get + 1u;
    restore_interrupts(interrupt_state);
    *token = event.token;
    *event_data = event.event_data;
    *event_mods = event.event_mods;
    *event_x = event.event_x;
    *event_y = event.event_y;
    return 1;
}

uint32_t bmx_pico_event_pending(void) {
    uint32_t interrupt_state = save_and_disable_interrupts();
    uint32_t pending = bmx_pico_deferred_event_put - bmx_pico_deferred_event_get;
    restore_interrupts(interrupt_state);
    return pending;
}

uint32_t bmx_pico_event_dropped(void) {
    return bmx_pico_deferred_event_dropped;
}

static volatile uint32_t bmx_pico_dma_completion_events[2][NUM_DMA_CHANNELS];
static volatile uint32_t bmx_pico_dma_irq_channel_masks[2];
static volatile uint32_t bmx_pico_dma_event_tokens[2][NUM_DMA_CHANNELS];
static uint8_t bmx_pico_dma_irq_installed[2];

static int32_t bmx_pico_dma_channel_valid(uint32_t channel) {
    return channel < NUM_DMA_CHANNELS;
}

static void bmx_pico_dma_irq_handler(uint32_t irq_line) {
    uint32_t pending = dma_hw->irq_ctrl[irq_line].ints &
        bmx_pico_dma_irq_channel_masks[irq_line];
    while (pending) {
        uint32_t channel = (uint32_t)__builtin_ctz(pending);
        uint32_t bit = 1u << channel;
        dma_irqn_acknowledge_channel(irq_line, channel);
        if (bmx_pico_dma_completion_events[irq_line][channel] != UINT32_MAX)
            ++bmx_pico_dma_completion_events[irq_line][channel];
        bmx_pico_event_post_from_irq(bmx_pico_dma_event_tokens[irq_line][channel],
            channel, dma_hw->ch[channel].ctrl_trig);
        pending &= ~bit;
    }
}

static void bmx_pico_dma_irq0_handler(void) { bmx_pico_dma_irq_handler(0); }
static void bmx_pico_dma_irq1_handler(void) { bmx_pico_dma_irq_handler(1); }

uint32_t bmx_pico_dma_channel_count(void) {
    return NUM_DMA_CHANNELS;
}

uint32_t bmx_pico_dma_irq_line_count(void) {
    return 2;
}

uint32_t bmx_pico_dma_force_dreq(void) {
    return DREQ_FORCE;
}

int32_t bmx_pico_dma_claim_unused_channel(void) {
    return get_core_num() == 0 ? dma_claim_unused_channel(false) : -1;
}

int32_t bmx_pico_dma_channel_is_claimed(uint32_t channel) {
    return bmx_pico_dma_channel_valid(channel) && dma_channel_is_claimed(channel);
}

int32_t bmx_pico_dma_unclaim_channel(uint32_t channel) {
    if (get_core_num() != 0 || !bmx_pico_dma_channel_valid(channel) ||
            !dma_channel_is_claimed(channel) || dma_channel_is_busy(channel)) return 0;
    for (uint32_t irq_line = 0; irq_line < 2; ++irq_line) {
        dma_irqn_set_channel_enabled(irq_line, channel, false);
        dma_irqn_acknowledge_channel(irq_line, channel);
        bmx_pico_dma_irq_channel_masks[irq_line] &= ~(1u << channel);
        bmx_pico_dma_completion_events[irq_line][channel] = 0;
        bmx_pico_dma_event_tokens[irq_line][channel] = 0;
    }
    dma_channel_cleanup(channel);
    dma_channel_unclaim(channel);
    return 1;
}

int32_t bmx_pico_dma_configure(uint32_t channel, void *read_address,
        void *write_address, uint32_t transfer_count, uint32_t data_size,
        int32_t read_increment, int32_t write_increment, uint32_t dreq, int32_t start) {
    return bmx_pico_dma_configure_advanced(channel, read_address, write_address,
        transfer_count, data_size, read_increment, write_increment, dreq,
        0, 0, 0, 0, 0, -1, start);
}

int32_t bmx_pico_dma_configure_advanced(uint32_t channel, void *read_address,
        void *write_address, uint32_t transfer_count, uint32_t data_size,
        int32_t read_increment, int32_t write_increment, uint32_t dreq,
        int32_t high_priority, int32_t byte_swap, int32_t quiet_irq,
        uint32_t ring_size_bits, int32_t ring_on_write, int32_t chain_to,
        int32_t start) {
    if (get_core_num() != 0 || !bmx_pico_dma_channel_valid(channel) ||
            !dma_channel_is_claimed(channel) || !read_address || !write_address ||
            !transfer_count || data_size > DMA_SIZE_32 || dreq > DREQ_FORCE ||
            ring_size_bits > 15u || chain_to >= (int32_t)NUM_DMA_CHANNELS ||
            dma_channel_is_busy(channel)) return 0;
#if !PICO_RP2040
    if (transfer_count & ~DMA_CH0_TRANS_COUNT_COUNT_BITS) return 0;
#endif
    if (chain_to >= 0 && !dma_channel_is_claimed((uint32_t)chain_to)) return 0;
    if (ring_size_bits) {
        uintptr_t ring_address = (uintptr_t)(ring_on_write ? write_address : read_address);
        uintptr_t ring_mask = ((uintptr_t)1u << ring_size_bits) - 1u;
        if (ring_address & ring_mask) return 0;
    }
    dma_channel_config config = dma_channel_get_default_config(channel);
    channel_config_set_transfer_data_size(&config, (enum dma_channel_transfer_size)data_size);
    channel_config_set_read_increment(&config, read_increment != 0);
    channel_config_set_write_increment(&config, write_increment != 0);
    channel_config_set_dreq(&config, dreq);
    channel_config_set_high_priority(&config, high_priority != 0);
    channel_config_set_bswap(&config, byte_swap != 0);
    channel_config_set_irq_quiet(&config, quiet_irq != 0);
    if (ring_size_bits)
        channel_config_set_ring(&config, ring_on_write != 0, ring_size_bits);
    if (chain_to >= 0)
        channel_config_set_chain_to(&config, (uint32_t)chain_to);
    dma_channel_configure(channel, &config, write_address, read_address,
        dma_encode_transfer_count(transfer_count), start != 0);
    return 1;
}

int32_t bmx_pico_dma_start(uint32_t channel) {
    if (get_core_num() != 0 || !bmx_pico_dma_channel_valid(channel) ||
            !dma_channel_is_claimed(channel) || dma_channel_is_busy(channel)) return 0;
    dma_channel_start(channel);
    return 1;
}

int32_t bmx_pico_dma_abort(uint32_t channel) {
    if (get_core_num() != 0 || !bmx_pico_dma_channel_valid(channel) ||
            !dma_channel_is_claimed(channel)) return 0;
    uint32_t enabled_lines = 0;
    for (uint32_t irq_line = 0; irq_line < 2; ++irq_line) {
        if (bmx_pico_dma_irq_channel_masks[irq_line] & (1u << channel)) {
            enabled_lines |= 1u << irq_line;
            dma_irqn_set_channel_enabled(irq_line, channel, false);
        }
    }
    hw_clear_bits(&dma_hw->ch[channel].ctrl_trig, DMA_CH0_CTRL_TRIG_EN_BITS);
    dma_channel_abort(channel);
    for (uint32_t irq_line = 0; irq_line < 2; ++irq_line) {
        dma_irqn_acknowledge_channel(irq_line, channel);
        bmx_pico_dma_completion_events[irq_line][channel] = 0;
        bmx_pico_dma_event_tokens[irq_line][channel] = 0;
        if (enabled_lines & (1u << irq_line))
            dma_irqn_set_channel_enabled(irq_line, channel, true);
    }
    return 1;
}

int32_t bmx_pico_dma_busy(uint32_t channel) {
    return bmx_pico_dma_channel_valid(channel) && dma_channel_is_claimed(channel) &&
        dma_channel_is_busy(channel);
}

uint32_t bmx_pico_dma_remaining(uint32_t channel) {
    if (!bmx_pico_dma_channel_valid(channel) || !dma_channel_is_claimed(channel)) return 0;
#if PICO_RP2040
    return dma_hw->ch[channel].transfer_count;
#else
    return dma_hw->ch[channel].transfer_count & DMA_CH0_TRANS_COUNT_COUNT_BITS;
#endif
}

int32_t bmx_pico_dma_set_irq_enabled(uint32_t channel, uint32_t irq_line,
        int32_t enabled) {
    if (get_core_num() != 0 || !bmx_pico_dma_channel_valid(channel) ||
            !dma_channel_is_claimed(channel) || irq_line >= 2) return 0;
    uint32_t interrupt_state = save_and_disable_interrupts();
    if (enabled && !bmx_pico_dma_irq_installed[irq_line]) {
        irq_add_shared_handler(dma_get_irq_num(irq_line),
            irq_line ? bmx_pico_dma_irq1_handler : bmx_pico_dma_irq0_handler,
            PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
        irq_set_enabled(dma_get_irq_num(irq_line), true);
        bmx_pico_dma_irq_installed[irq_line] = 1;
    }
    dma_irqn_acknowledge_channel(irq_line, channel);
    bmx_pico_dma_completion_events[irq_line][channel] = 0;
    if (enabled)
        bmx_pico_dma_irq_channel_masks[irq_line] |= 1u << channel;
    else
        bmx_pico_dma_irq_channel_masks[irq_line] &= ~(1u << channel);
    dma_irqn_set_channel_enabled(irq_line, channel, enabled != 0);
    restore_interrupts(interrupt_state);
    return 1;
}

int32_t bmx_pico_dma_set_event_token(uint32_t channel, uint32_t irq_line,
        uint32_t token) {
    if (get_core_num() != 0 || !bmx_pico_dma_channel_valid(channel) ||
            !dma_channel_is_claimed(channel) || irq_line >= 2) return 0;
    uint32_t interrupt_state = save_and_disable_interrupts();
    bmx_pico_dma_event_tokens[irq_line][channel] = token;
    restore_interrupts(interrupt_state);
    return 1;
}

uint32_t bmx_pico_dma_pending_completion_events(uint32_t channel, uint32_t irq_line) {
    if (get_core_num() != 0 || !bmx_pico_dma_channel_valid(channel) || irq_line >= 2)
        return 0;
    return bmx_pico_dma_completion_events[irq_line][channel];
}

uint32_t bmx_pico_dma_take_completion_events(uint32_t channel, uint32_t irq_line) {
    if (get_core_num() != 0 || !bmx_pico_dma_channel_valid(channel) || irq_line >= 2)
        return 0;
    uint32_t interrupt_state = save_and_disable_interrupts();
    uint32_t events = bmx_pico_dma_completion_events[irq_line][channel];
    bmx_pico_dma_completion_events[irq_line][channel] = 0;
    restore_interrupts(interrupt_state);
    return events;
}

typedef struct BMXPicoDMABufferHeader {
    void *allocation;
    uint32_t marker;
} BMXPicoDMABufferHeader;

#define BMX_PICO_DMA_BUFFER_MARKER 0x444d4142u

void *bmx_pico_dma_buffer_allocate(uint32_t size, uint32_t alignment) {
    if (!size || alignment < BMX_EMBEDDED_MEMORY_ALIGNMENT || alignment > 32768u ||
            (alignment & (alignment - 1u)) != 0u ||
            size > UINT32_MAX - alignment - sizeof(BMXPicoDMABufferHeader)) return NULL;
    void *allocation = bbMemAlloc(size + alignment - 1u + sizeof(BMXPicoDMABufferHeader));
    if (!allocation) return NULL;
    uintptr_t first = (uintptr_t)allocation + sizeof(BMXPicoDMABufferHeader);
    uintptr_t aligned = (first + alignment - 1u) & ~((uintptr_t)alignment - 1u);
    BMXPicoDMABufferHeader *header = (BMXPicoDMABufferHeader *)aligned - 1;
    header->allocation = allocation;
    header->marker = BMX_PICO_DMA_BUFFER_MARKER;
    return (void *)aligned;
}

void bmx_pico_dma_buffer_free(void *buffer) {
    if (!buffer) return;
    BMXPicoDMABufferHeader *header = (BMXPicoDMABufferHeader *)buffer - 1;
    if (header->marker != BMX_PICO_DMA_BUFFER_MARKER || !header->allocation)
        panic("BlitzMax Pico DMA buffer received an invalid pointer");
    void *allocation = header->allocation;
    header->marker = 0;
    header->allocation = NULL;
    bbMemFree(allocation);
}

uint32_t bmx_pico_dma_timer_count(void) {
    return NUM_DMA_TIMERS;
}

int32_t bmx_pico_dma_claim_unused_timer(void) {
    return get_core_num() == 0 ? dma_claim_unused_timer(false) : -1;
}

int32_t bmx_pico_dma_timer_is_claimed(uint32_t timer) {
    return timer < NUM_DMA_TIMERS && dma_timer_is_claimed(timer);
}

int32_t bmx_pico_dma_timer_configure(uint32_t timer, uint32_t numerator,
        uint32_t denominator) {
    if (get_core_num() != 0 || timer >= NUM_DMA_TIMERS ||
            !dma_timer_is_claimed(timer) || !numerator || !denominator ||
            numerator > UINT16_MAX || denominator > UINT16_MAX ||
            numerator > denominator) return 0;
    dma_timer_set_fraction(timer, (uint16_t)numerator, (uint16_t)denominator);
    return 1;
}

int32_t bmx_pico_dma_timer_unclaim(uint32_t timer) {
    if (get_core_num() != 0 || timer >= NUM_DMA_TIMERS ||
            !dma_timer_is_claimed(timer)) return 0;
    dma_timer_set_fraction(timer, 0, 1);
    dma_timer_unclaim(timer);
    return 1;
}

uint32_t bmx_pico_dma_timer_dreq(uint32_t timer) {
    return timer < NUM_DMA_TIMERS ? dma_get_timer_dreq(timer) : UINT32_MAX;
}

void bmx_embedded_delay(int32_t milliseconds) {
    if (milliseconds > 0) sleep_ms((uint32_t)milliseconds);
}

void bmx_embedded_udelay(int32_t microseconds) {
    if (microseconds > 0) sleep_us((uint64_t)(uint32_t)microseconds);
}

void bmx_pico_system_wait(void) {
    if (bmx_pico_event_pending()) return;
    __wfe();
}
