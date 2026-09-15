#ifndef BLITZMAX_PICO_RUNTIME_H
#define BLITZMAX_PICO_RUNTIME_H

#include "blitzmax/embedded_runtime.h"
#include "blitzmax/embedded_events.h"
#include "blitzmax/embedded_uart.h"
#include "blitzmax/embedded_system.h"

/* Binary-compatible with Pub.Time.SDateTime. Pico.System.Calendar keeps the
   native shape here without introducing a module dependency cycle. */
typedef struct BMXPicoCalendarDateTime {
    int32_t year;
    int32_t month;
    int32_t day;
    int32_t hour;
    int32_t minute;
    int32_t second;
    int32_t millisecond;
    int32_t utc;
    int32_t offset;
    int32_t dst;
} BMXPicoCalendarDateTime;

#ifdef __cplusplus
extern "C" {
#endif

int32_t bmx_pico_put_string(const BMXEmbeddedString *text);
int32_t bmx_pico_stdio_init_all(void);
int64_t bmx_pico_stdio_read(void *buffer, int64_t count);
int64_t bmx_pico_stdio_write(void *buffer, int64_t count);
void bmx_pico_stdio_flush(void);
int32_t bmx_pico_putchar_raw(int32_t character);
void bmx_pico_system_wait(void);
int32_t bmx_pico_event_post_from_irq(uint32_t token, uint32_t event_data,
    uint32_t detail);
int32_t bmx_pico_event_post_from_irq_ex(uint32_t token, uint32_t event_data,
    uint32_t event_mods, uint32_t event_x, uint32_t event_y);
int32_t bmx_pico_event_take(uint32_t *token, uint32_t *event_data,
    uint32_t *event_mods, uint32_t *event_x, uint32_t *event_y);
uint32_t bmx_pico_event_pending(void);
uint32_t bmx_pico_event_dropped(void);
int32_t bmx_pico_psram_available(void);
uint32_t bmx_pico_psram_capacity(void);
int32_t bmx_pico_psram_contains(void *address);

uint32_t bmx_pico_watchdog_maximum_delay_ms(void);
int32_t bmx_pico_watchdog_enable(uint32_t delay_ms, int32_t pause_on_debug);
void bmx_pico_watchdog_disable(void);
void bmx_pico_watchdog_feed(void);
int32_t bmx_pico_watchdog_caused_reboot(void);
int32_t bmx_pico_watchdog_enable_caused_reboot(void);
uint32_t bmx_pico_watchdog_time_remaining_us(void);
uint32_t bmx_pico_watchdog_time_remaining_ms(void);
int32_t bmx_pico_watchdog_reboot(uint32_t delay_ms);

const BMXEmbeddedString *bmx_pico_unique_board_id(void);
BMXEmbeddedArray *bmx_pico_unique_board_id_bytes(void);
int32_t bmx_pico_bootsel_button_pressed(void);
int32_t bmx_pico_device_reboot(uint32_t delay_ms);
int32_t bmx_pico_device_reboot_to_bootsel(int32_t activity_pin, int32_t activity_pin_active_low,
    int32_t disable_mass_storage, int32_t disable_picoboot);

uint32_t bmx_pico_flash_storage_physical_size(void);
uint32_t bmx_pico_flash_storage_offset(void);
uint32_t bmx_pico_flash_storage_size(void);
uint32_t bmx_pico_flash_storage_tail_reserved_size(void);
uint32_t bmx_pico_flash_storage_read_size(void);
uint32_t bmx_pico_flash_storage_program_size(void);
uint32_t bmx_pico_flash_storage_erase_size(void);
int32_t bmx_pico_flash_storage_read(uint32_t offset, void *destination, uint32_t count);
int32_t bmx_pico_flash_storage_is_erased(uint32_t offset, uint32_t count);
int32_t bmx_pico_flash_storage_program(uint32_t offset, void *source, uint32_t count,
    uint32_t timeout_ms);
int32_t bmx_pico_flash_storage_erase(uint32_t offset, uint32_t count, uint32_t timeout_ms);

void bmx_pico_gpio_init(uint32_t gpio);
void bmx_pico_gpio_set_function(uint32_t gpio, int32_t function);
int32_t bmx_pico_gpio_get_function(uint32_t gpio);
void bmx_pico_gpio_set_direction(uint32_t gpio, int32_t direction);
int32_t bmx_pico_gpio_get_direction(uint32_t gpio);
void bmx_pico_gpio_set_input(uint32_t gpio);
void bmx_pico_gpio_set_output(uint32_t gpio);
int32_t bmx_pico_gpio_get(uint32_t gpio);
void bmx_pico_gpio_put(uint32_t gpio, int32_t value);
int32_t bmx_pico_gpio_get_output(uint32_t gpio);
void bmx_pico_gpio_set_pulls(uint32_t gpio, int32_t pull_up, int32_t pull_down);
void bmx_pico_gpio_pull_up(uint32_t gpio);
void bmx_pico_gpio_pull_down(uint32_t gpio);
void bmx_pico_gpio_disable_pulls(uint32_t gpio);
int32_t bmx_pico_gpio_is_pulled_up(uint32_t gpio);
int32_t bmx_pico_gpio_is_pulled_down(uint32_t gpio);
void bmx_pico_gpio_set_input_enabled(uint32_t gpio, int32_t enabled);
void bmx_pico_gpio_set_input_hysteresis_enabled(uint32_t gpio, int32_t enabled);
void bmx_pico_gpio_set_slew_rate(uint32_t gpio, int32_t slew_rate);
int32_t bmx_pico_gpio_get_slew_rate(uint32_t gpio);
void bmx_pico_gpio_set_drive_strength(uint32_t gpio, int32_t drive_strength);
int32_t bmx_pico_gpio_get_drive_strength(uint32_t gpio);
int32_t bmx_embedded_gpio_is_valid(uint32_t gpio);
int32_t bmx_embedded_gpio_is_output_capable(uint32_t gpio);
int32_t bmx_embedded_gpio_is_pull_capable(uint32_t gpio);
int32_t bmx_embedded_gpio_init(uint32_t gpio);
int32_t bmx_embedded_gpio_set_direction(uint32_t gpio, int32_t direction);
int32_t bmx_embedded_gpio_get_direction(uint32_t gpio);
int32_t bmx_embedded_gpio_set_input(uint32_t gpio);
int32_t bmx_embedded_gpio_set_output(uint32_t gpio);
int32_t bmx_embedded_gpio_get(uint32_t gpio);
int32_t bmx_embedded_gpio_put(uint32_t gpio, int32_t value);
int32_t bmx_embedded_gpio_get_output(uint32_t gpio);
int32_t bmx_embedded_gpio_set_pulls(uint32_t gpio, int32_t pull_up, int32_t pull_down);
int32_t bmx_embedded_gpio_pull_up(uint32_t gpio);
int32_t bmx_embedded_gpio_pull_down(uint32_t gpio);
int32_t bmx_embedded_gpio_disable_pulls(uint32_t gpio);
int32_t bmx_embedded_gpio_is_pulled_up(uint32_t gpio);
int32_t bmx_embedded_gpio_is_pulled_down(uint32_t gpio);
int32_t bmx_embedded_gpio_set_drive_strength(uint32_t gpio, int32_t drive_strength);
int32_t bmx_embedded_gpio_get_drive_strength(uint32_t gpio);
int32_t bmx_pico_gpio_set_irq_enabled(uint32_t gpio, uint32_t event_mask, int32_t enabled);
int32_t bmx_pico_gpio_set_event_token(uint32_t gpio, uint32_t token);
uint32_t bmx_pico_gpio_pending_irq_events(uint32_t gpio);
uint32_t bmx_pico_gpio_take_irq_events(uint32_t gpio);

uint64_t bmx_pico_time_microseconds(void);
uint32_t bmx_pico_system_clock_hz(void);
uint64_t bmx_pico_time_milliseconds(void);
void bmx_pico_sleep_us(uint64_t microseconds);
uint64_t bmx_embedded_time_microseconds(void);
uint64_t bmx_embedded_time_milliseconds(void);
void bmx_embedded_sleep_milliseconds(uint32_t milliseconds);
void bmx_embedded_sleep_microseconds(uint64_t microseconds);
uint32_t bmx_pico_power_capabilities(void);
void bmx_pico_power_idle(void);
int32_t bmx_pico_power_sleep_until_interrupt(void);
int32_t bmx_pico_power_sleep_for_ms(uint32_t milliseconds, int32_t exclusive);
int32_t bmx_pico_power_dormant_for_ms(uint32_t milliseconds);
int32_t bmx_pico_power_dormant_until_gpio(uint32_t gpio, int32_t edge, int32_t high);
int32_t bmx_pico_power_set_unused_pins_low_leakage(uint64_t exclude_mask);
int32_t bmx_pico_alarm_after_ms(uint32_t milliseconds);
int32_t bmx_pico_alarm_after_us(uint64_t microseconds);
int32_t bmx_pico_repeating_alarm_ms(uint32_t milliseconds);
int32_t bmx_pico_repeating_alarm_us(uint64_t microseconds);
int32_t bmx_pico_alarm_cancel(int32_t handle);
int32_t bmx_pico_alarm_active(int32_t handle);
uint32_t bmx_pico_alarm_pending_events(int32_t handle);
uint32_t bmx_pico_alarm_take_events(int32_t handle);
int64_t bmx_pico_alarm_remaining_us(int32_t handle);
int32_t bmx_pico_alarm_remaining_ms(int32_t handle);

int32_t bmx_pico_datetime_to_epoch(const BMXPicoCalendarDateTime *date_time,
    int64_t *seconds, int32_t *milliseconds);
int32_t bmx_pico_datetime_from_epoch(int64_t seconds, int32_t milliseconds,
    BMXPicoCalendarDateTime *date_time);
void bmx_pico_calendar_stop(void);
int32_t bmx_pico_calendar_is_running(void);
uint64_t bmx_pico_calendar_resolution_nanoseconds(void);
void bmx_pico_calendar_disable_alarm(void);
uint32_t bmx_pico_calendar_pending_alarm_events(void);
uint32_t bmx_pico_calendar_take_alarm_events(void);

uint32_t bmx_pico_pwm_init_gpio(uint32_t gpio);
uint32_t bmx_pico_pwm_slice_for_gpio(uint32_t gpio);
uint32_t bmx_pico_pwm_channel_for_gpio(uint32_t gpio);
void bmx_pico_pwm_set_wrap(uint32_t slice, uint32_t wrap);
uint32_t bmx_pico_pwm_get_wrap(uint32_t slice);
void bmx_pico_pwm_set_channel_level(uint32_t slice, uint32_t channel, uint32_t level);
uint32_t bmx_pico_pwm_get_channel_level(uint32_t slice, uint32_t channel);
void bmx_pico_pwm_set_both_levels(uint32_t slice, uint32_t level_a, uint32_t level_b);
void bmx_pico_pwm_set_gpio_level(uint32_t gpio, uint32_t level);
uint32_t bmx_pico_pwm_get_counter(uint32_t slice);
void bmx_pico_pwm_set_counter(uint32_t slice, uint32_t counter);
void bmx_pico_pwm_set_clock_divider(uint32_t slice, float divider);
void bmx_pico_pwm_set_clock_divider_int_frac(uint32_t slice, uint32_t integer, uint32_t fraction);
void bmx_pico_pwm_set_divider_mode(uint32_t slice, uint32_t mode);
void bmx_pico_pwm_set_output_polarity(uint32_t slice, int32_t invert_a, int32_t invert_b);
void bmx_pico_pwm_set_phase_correct(uint32_t slice, int32_t enabled);
void bmx_pico_pwm_set_enabled(uint32_t slice, int32_t enabled);
uint32_t bmx_pico_pwm_set_frequency(uint32_t slice, uint32_t frequency);
int32_t bmx_pico_pwm_set_irq_enabled(uint32_t slice, int32_t enabled);
uint32_t bmx_pico_pwm_pending_wrap_events(uint32_t slice);
uint32_t bmx_pico_pwm_take_wrap_events(uint32_t slice);

void bmx_pico_adc_init(void);
void bmx_pico_adc_gpio_init(uint32_t gpio);
int32_t bmx_pico_adc_input_for_gpio(uint32_t gpio);
void bmx_pico_adc_select_input(uint32_t input);
uint32_t bmx_pico_adc_get_selected_input(void);
void bmx_pico_adc_set_round_robin(uint32_t input_mask);
void bmx_pico_adc_set_temperature_sensor_enabled(int32_t enabled);
uint32_t bmx_pico_adc_read(void);
uint32_t bmx_pico_adc_read_input(uint32_t input);
void bmx_pico_adc_run(int32_t enabled);
void bmx_pico_adc_set_clock_divider(float divider);
void bmx_pico_adc_fifo_setup(int32_t enabled, int32_t dma_request_enabled,
    uint32_t dma_request_threshold, int32_t error_in_fifo, int32_t byte_shift);
int32_t bmx_pico_adc_fifo_is_empty(void);
uint32_t bmx_pico_adc_fifo_level(void);
uint32_t bmx_pico_adc_fifo_get(void);
uint32_t bmx_pico_adc_fifo_get_blocking(void);
void bmx_pico_adc_fifo_drain(void);
void *bmx_pico_adc_fifo_address(void);
uint32_t bmx_pico_adc_dreq(void);

int32_t bmx_pico_i2c_default_controller(void);
uint32_t bmx_pico_i2c_default_sda_pin(void);
uint32_t bmx_pico_i2c_default_scl_pin(void);
int32_t bmx_pico_i2c_configure_pins(int32_t controller, uint32_t sda_pin,
    uint32_t scl_pin, int32_t pull_ups);
uint32_t bmx_pico_i2c_init(int32_t controller, uint32_t baudrate);
void bmx_pico_i2c_deinit(int32_t controller);
uint32_t bmx_pico_i2c_set_baudrate(int32_t controller, uint32_t baudrate);
int32_t bmx_pico_i2c_set_slave_mode(int32_t controller, int32_t enabled, uint32_t address);
int32_t bmx_pico_i2c_write_blocking(int32_t controller, uint32_t address,
    void *data, int32_t length, int32_t no_stop);
int32_t bmx_pico_i2c_read_blocking(int32_t controller, uint32_t address,
    void *data, int32_t length, int32_t no_stop);
int32_t bmx_pico_i2c_write_timeout_us(int32_t controller, uint32_t address,
    void *data, int32_t length, int32_t no_stop, uint32_t timeout_us);
int32_t bmx_pico_i2c_read_timeout_us(int32_t controller, uint32_t address,
    void *data, int32_t length, int32_t no_stop, uint32_t timeout_us);
uint32_t bmx_pico_i2c_write_available(int32_t controller);
uint32_t bmx_pico_i2c_read_available(int32_t controller);
int32_t bmx_pico_i2c_write_raw_blocking(int32_t controller,
    void *data, int32_t length);
int32_t bmx_pico_i2c_read_raw_blocking(int32_t controller,
    void *data, int32_t length);

int32_t bmx_pico_spi_default_controller(void);
uint32_t bmx_pico_spi_default_rx_pin(void);
uint32_t bmx_pico_spi_default_tx_pin(void);
uint32_t bmx_pico_spi_default_sck_pin(void);
uint32_t bmx_pico_spi_default_csn_pin(void);
int32_t bmx_pico_spi_configure_pins(int32_t controller, uint32_t rx_pin,
    uint32_t tx_pin, uint32_t sck_pin);
uint32_t bmx_pico_spi_init(int32_t controller, uint32_t baudrate);
void bmx_pico_spi_deinit(int32_t controller);
uint32_t bmx_pico_spi_set_baudrate(int32_t controller, uint32_t baudrate);
uint32_t bmx_pico_spi_get_baudrate(int32_t controller);
int32_t bmx_pico_spi_set_format(int32_t controller, uint32_t data_bits,
    uint32_t polarity, uint32_t phase, uint32_t bit_order);
int32_t bmx_pico_spi_set_peripheral_mode(int32_t controller, int32_t enabled);
int32_t bmx_pico_spi_is_writable(int32_t controller);
int32_t bmx_pico_spi_is_readable(int32_t controller);
int32_t bmx_pico_spi_is_busy(int32_t controller);
int32_t bmx_pico_spi_write_read_blocking(int32_t controller, void *source,
    void *destination, int32_t length);
int32_t bmx_pico_spi_write_blocking(int32_t controller, void *source, int32_t length);
int32_t bmx_pico_spi_read_blocking(int32_t controller, uint32_t repeated_data,
    void *destination, int32_t length);
int32_t bmx_pico_spi_write16_read16_blocking(int32_t controller, uint16_t *source,
    uint16_t *destination, int32_t length);
int32_t bmx_pico_spi_write16_blocking(int32_t controller, uint16_t *source, int32_t length);
int32_t bmx_pico_spi_read16_blocking(int32_t controller, uint32_t repeated_data,
    uint16_t *destination, int32_t length);
void *bmx_pico_spi_data_register_address(int32_t controller);
uint32_t bmx_pico_spi_tx_dreq(int32_t controller);
uint32_t bmx_pico_spi_rx_dreq(int32_t controller);

int32_t bmx_pico_uart_default_controller(void);
uint32_t bmx_pico_uart_default_tx_pin(void);
uint32_t bmx_pico_uart_default_rx_pin(void);
uint32_t bmx_pico_uart_default_baudrate(void);
int32_t bmx_pico_uart_supports_auxiliary_pin_mappings(void);
int32_t bmx_pico_uart_configure_pins(int32_t controller, uint32_t tx_pin, uint32_t rx_pin);
int32_t bmx_pico_uart_configure_flow_pins(int32_t controller, uint32_t cts_pin, uint32_t rts_pin);
uint32_t bmx_pico_uart_init(int32_t controller, uint32_t baudrate);
void bmx_pico_uart_deinit(int32_t controller);
uint32_t bmx_pico_uart_set_baudrate(int32_t controller, uint32_t baudrate);
int32_t bmx_pico_uart_set_format(int32_t controller, uint32_t data_bits,
    uint32_t stop_bits, uint32_t parity);
int32_t bmx_pico_uart_set_flow_control(int32_t controller, int32_t cts, int32_t rts);
int32_t bmx_pico_uart_set_fifo_enabled(int32_t controller, int32_t enabled);
int32_t bmx_pico_uart_is_enabled(int32_t controller);
int32_t bmx_pico_uart_is_writable(int32_t controller);
int32_t bmx_pico_uart_is_readable(int32_t controller);
int32_t bmx_pico_uart_is_readable_within_us(int32_t controller, uint32_t timeout_us);
int32_t bmx_pico_uart_write_blocking(int32_t controller, void *source, int32_t length);
int32_t bmx_pico_uart_read_blocking(int32_t controller, void *destination, int32_t length);
int32_t bmx_pico_uart_read_timeout_us(int32_t controller, void *destination,
    int32_t length, uint32_t timeout_us);
int32_t bmx_pico_uart_read_available(int32_t controller, void *destination, int32_t capacity);
int32_t bmx_pico_uart_put_byte(int32_t controller, uint32_t value);
void bmx_pico_uart_tx_wait_blocking(int32_t controller);
int32_t bmx_pico_uart_set_break(int32_t controller, int32_t enabled);
int32_t bmx_pico_uart_set_translate_crlf(int32_t controller, int32_t enabled);
uint32_t bmx_pico_uart_get_errors(int32_t controller);
void bmx_pico_uart_clear_errors(int32_t controller);
int32_t bmx_pico_uart_async_open(int32_t controller, uint32_t rx_capacity,
    uint32_t tx_capacity, uint32_t rx_token, uint32_t tx_token,
    uint32_t error_token);
int32_t bmx_pico_uart_async_close(int32_t controller);
int32_t bmx_pico_uart_async_is_open(int32_t controller);
int32_t bmx_pico_uart_async_read(int32_t controller, void *destination,
    int32_t capacity);
int32_t bmx_pico_uart_async_write(int32_t controller, void *source,
    int32_t length);
uint32_t bmx_pico_uart_async_read_available(int32_t controller);
uint32_t bmx_pico_uart_async_write_available(int32_t controller);
uint32_t bmx_pico_uart_async_rx_dropped(int32_t controller);
int32_t bmx_pico_uart_async_tx_idle(int32_t controller);

#define BMX_PICO_PIO_CONFIG_CLOCK_DIVIDER (1u << 0)
#define BMX_PICO_PIO_CONFIG_OUT_PINS       (1u << 1)
#define BMX_PICO_PIO_CONFIG_SET_PINS       (1u << 2)
#define BMX_PICO_PIO_CONFIG_IN_PINS        (1u << 3)
#define BMX_PICO_PIO_CONFIG_SIDESET_PINS   (1u << 4)
#define BMX_PICO_PIO_CONFIG_JUMP_PIN       (1u << 5)
#define BMX_PICO_PIO_CONFIG_IN_SHIFT       (1u << 6)
#define BMX_PICO_PIO_CONFIG_OUT_SHIFT      (1u << 7)
#define BMX_PICO_PIO_CONFIG_FIFO_JOIN      (1u << 8)
#define BMX_PICO_PIO_CONFIG_SIDESET        (1u << 9)
#define BMX_PICO_PIO_CONFIG_OUT_SPECIAL    (1u << 10)
#define BMX_PICO_PIO_CONFIG_MOV_STATUS     (1u << 11)

typedef struct BMXPicoPIOStateMachineConfig {
    uint32_t overrides;
    float clock_divider;
    uint32_t out_pin_base;
    uint32_t out_pin_count;
    uint32_t set_pin_base;
    uint32_t set_pin_count;
    uint32_t in_pin_base;
    uint32_t sideset_pin_base;
    uint32_t jump_pin;
    uint32_t in_shift_right;
    uint32_t autopush;
    uint32_t push_threshold;
    uint32_t out_shift_right;
    uint32_t autopull;
    uint32_t pull_threshold;
    uint32_t fifo_join;
    uint32_t sideset_bit_count;
    uint32_t sideset_optional;
    uint32_t sideset_pindirs;
    uint32_t out_sticky;
    uint32_t out_has_enable_pin;
    uint32_t out_enable_bit_index;
    uint32_t mov_status_type;
    uint32_t mov_status_threshold;
} BMXPicoPIOStateMachineConfig;

int32_t bmx_pico_pio_apply_config_overrides(void *config,
    const BMXPicoPIOStateMachineConfig *overrides);

typedef int32_t (*BMXPicoPIOProgramInitializer)(void *instance, uint32_t state_machine,
    uint32_t offset, const BMXPicoPIOStateMachineConfig *overrides);

typedef struct BMXPicoPIOProgramDescriptor {
    const char *name;
    const uint16_t *instructions;
    uint32_t length;
    int32_t origin;
    uint32_t version;
    uint32_t used_gpio_ranges;
    uint32_t wrap_target;
    uint32_t wrap;
    BMXPicoPIOProgramInitializer initialize;
} BMXPicoPIOProgramDescriptor;

extern const BMXPicoPIOProgramDescriptor bmx_pico_imported_pio_programs[];
extern const uint32_t bmx_pico_imported_pio_program_count;

uint32_t bmx_pico_pio_count(void);
uint32_t bmx_pico_pio_version(void);
uint32_t bmx_pico_pio_state_machine_count(void);
uint32_t bmx_pico_pio_instruction_capacity(void);
int32_t bmx_pico_pio_add_program(int32_t controller, uint16_t *instructions,
    uint32_t length, int32_t origin, uint32_t version, uint32_t used_gpio_ranges);
int32_t bmx_pico_pio_can_add_program(int32_t controller, uint16_t *instructions,
    uint32_t length, int32_t origin, uint32_t version, uint32_t used_gpio_ranges);
int32_t bmx_pico_pio_remove_program(int32_t controller, uint32_t length, uint32_t offset);
int32_t bmx_pico_pio_claim_unused_state_machine(int32_t controller);
int32_t bmx_pico_pio_unclaim_state_machine(int32_t controller, uint32_t state_machine);
int32_t bmx_pico_pio_state_machine_is_claimed(int32_t controller, uint32_t state_machine);
int32_t bmx_pico_pio_gpio_init(int32_t controller, uint32_t pin);
uint32_t bmx_pico_pio_gpio_base(int32_t controller);
int32_t bmx_pico_pio_set_gpio_base(int32_t controller, uint32_t gpio_base);
int32_t bmx_pico_pio_sm_set_consecutive_pin_directions(int32_t controller,
    uint32_t state_machine, uint32_t pin_base, uint32_t pin_count, int32_t output);
int32_t bmx_pico_pio_sm_init(int32_t controller, uint32_t state_machine, uint32_t initial_pc);
int32_t bmx_pico_pio_sm_init_configured(int32_t controller, uint32_t state_machine,
    uint32_t initial_pc, void *config);
int32_t bmx_pico_pio_sm_set_wrap(int32_t controller, uint32_t state_machine,
    uint32_t wrap_target, uint32_t wrap);
int32_t bmx_pico_pio_sm_set_out_pins(int32_t controller, uint32_t state_machine,
    uint32_t pin_base, uint32_t pin_count);
int32_t bmx_pico_pio_sm_set_set_pins(int32_t controller, uint32_t state_machine,
    uint32_t pin_base, uint32_t pin_count);
int32_t bmx_pico_pio_sm_set_in_pins(int32_t controller, uint32_t state_machine,
    uint32_t pin_base);
int32_t bmx_pico_pio_sm_set_sideset_pins(int32_t controller, uint32_t state_machine,
    uint32_t pin_base);
int32_t bmx_pico_pio_sm_set_jump_pin(int32_t controller, uint32_t state_machine,
    uint32_t pin);
int32_t bmx_pico_pio_sm_set_clock_divider(int32_t controller, uint32_t state_machine,
    float divider);
int32_t bmx_pico_pio_sm_set_enabled(int32_t controller, uint32_t state_machine,
    int32_t enabled);
int32_t bmx_pico_pio_sm_mask_set_enabled(int32_t controller, uint32_t state_machine_mask,
    int32_t enabled);
int32_t bmx_pico_pio_sm_mask_restart(int32_t controller, uint32_t state_machine_mask);
int32_t bmx_pico_pio_sm_mask_restart_clock_divider(int32_t controller,
    uint32_t state_machine_mask);
int32_t bmx_pico_pio_sm_mask_enable_synchronized(int32_t controller,
    uint32_t state_machine_mask);
int32_t bmx_pico_pio_sm_restart(int32_t controller, uint32_t state_machine);
int32_t bmx_pico_pio_sm_restart_clock_divider(int32_t controller, uint32_t state_machine);
int32_t bmx_pico_pio_sm_clear_fifos(int32_t controller, uint32_t state_machine);
int32_t bmx_pico_pio_sm_execute(int32_t controller, uint32_t state_machine,
    uint32_t instruction);
int32_t bmx_pico_pio_sm_execute_blocking(int32_t controller, uint32_t state_machine,
    uint32_t instruction);
int32_t bmx_pico_pio_sm_execute_stalled(int32_t controller, uint32_t state_machine);
int32_t bmx_pico_pio_sm_drain_tx_fifo(int32_t controller, uint32_t state_machine);
int32_t bmx_pico_pio_sm_set_pins_masked(int32_t controller, uint32_t state_machine,
    uint64_t pin_values, uint64_t pin_mask);
int32_t bmx_pico_pio_sm_set_pin_directions_masked(int32_t controller,
    uint32_t state_machine, uint64_t pin_directions, uint64_t pin_mask);
uint32_t bmx_pico_pio_sm_program_counter(int32_t controller, uint32_t state_machine);
int32_t bmx_pico_pio_sm_tx_full(int32_t controller, uint32_t state_machine);
int32_t bmx_pico_pio_sm_tx_empty(int32_t controller, uint32_t state_machine);
int32_t bmx_pico_pio_sm_rx_full(int32_t controller, uint32_t state_machine);
int32_t bmx_pico_pio_sm_rx_empty(int32_t controller, uint32_t state_machine);
uint32_t bmx_pico_pio_sm_tx_level(int32_t controller, uint32_t state_machine);
uint32_t bmx_pico_pio_sm_rx_level(int32_t controller, uint32_t state_machine);
int32_t bmx_pico_pio_sm_put(int32_t controller, uint32_t state_machine, uint32_t value);
int32_t bmx_pico_pio_sm_put_blocking(int32_t controller, uint32_t state_machine,
    uint32_t value);
uint32_t bmx_pico_pio_sm_get(int32_t controller, uint32_t state_machine);
uint32_t bmx_pico_pio_sm_get_blocking(int32_t controller, uint32_t state_machine);
void *bmx_pico_pio_sm_tx_fifo_address(int32_t controller, uint32_t state_machine);
void *bmx_pico_pio_sm_rx_fifo_address(int32_t controller, uint32_t state_machine);
uint32_t bmx_pico_pio_sm_dreq(int32_t controller, uint32_t state_machine,
    int32_t transmit);
uint32_t bmx_pico_pio_interrupt_count(void);
uint32_t bmx_pico_pio_irq_supported_sources(void);
int32_t bmx_pico_pio_irq_set_sources_enabled(int32_t controller, uint32_t irq_line,
    uint32_t source_mask, int32_t enabled);
int32_t bmx_pico_pio_irq_set_event_token(int32_t controller, uint32_t irq_line,
    uint32_t source_mask, uint32_t token);
uint32_t bmx_pico_pio_irq_enabled_sources(int32_t controller, uint32_t irq_line);
uint32_t bmx_pico_pio_irq_armed_sources(int32_t controller, uint32_t irq_line);
uint32_t bmx_pico_pio_irq_pending_events(int32_t controller, uint32_t irq_line);
uint32_t bmx_pico_pio_irq_take_events(int32_t controller, uint32_t irq_line);
uint32_t bmx_pico_pio_irq_take_events_masked(int32_t controller, uint32_t irq_line,
    uint32_t source_mask);
int32_t bmx_pico_pio_irq_rearm_sources(int32_t controller, uint32_t irq_line,
    uint32_t source_mask);
int32_t bmx_pico_pio_interrupt_is_set(int32_t controller, uint32_t interrupt_number);
int32_t bmx_pico_pio_interrupt_clear(int32_t controller, uint32_t interrupt_number);
int32_t bmx_pico_pio_sm_init_imported_program_configured(int32_t controller,
    uint32_t state_machine, void *handle, uint32_t offset, void *config);

uint32_t bmx_pico_dma_channel_count(void);
uint32_t bmx_pico_dma_irq_line_count(void);
uint32_t bmx_pico_dma_force_dreq(void);
int32_t bmx_pico_dma_claim_unused_channel(void);
int32_t bmx_pico_dma_channel_is_claimed(uint32_t channel);
int32_t bmx_pico_dma_unclaim_channel(uint32_t channel);
int32_t bmx_pico_dma_configure(uint32_t channel, void *read_address, void *write_address,
    uint32_t transfer_count, uint32_t data_size, int32_t read_increment,
    int32_t write_increment, uint32_t dreq, int32_t start);
int32_t bmx_pico_dma_configure_advanced(uint32_t channel, void *read_address,
    void *write_address, uint32_t transfer_count, uint32_t data_size,
    int32_t read_increment, int32_t write_increment, uint32_t dreq,
    int32_t high_priority, int32_t byte_swap, int32_t quiet_irq,
    uint32_t ring_size_bits, int32_t ring_on_write, int32_t chain_to,
    int32_t start);
int32_t bmx_pico_dma_start(uint32_t channel);
int32_t bmx_pico_dma_abort(uint32_t channel);
int32_t bmx_pico_dma_busy(uint32_t channel);
uint32_t bmx_pico_dma_remaining(uint32_t channel);
int32_t bmx_pico_dma_set_irq_enabled(uint32_t channel, uint32_t irq_line,
    int32_t enabled);
int32_t bmx_pico_dma_set_event_token(uint32_t channel, uint32_t irq_line,
    uint32_t token);
uint32_t bmx_pico_dma_pending_completion_events(uint32_t channel, uint32_t irq_line);
uint32_t bmx_pico_dma_take_completion_events(uint32_t channel, uint32_t irq_line);
void *bmx_pico_dma_buffer_allocate(uint32_t size, uint32_t alignment);
void bmx_pico_dma_buffer_free(void *buffer);
uint32_t bmx_pico_dma_timer_count(void);
int32_t bmx_pico_dma_claim_unused_timer(void);
int32_t bmx_pico_dma_timer_is_claimed(uint32_t timer);
int32_t bmx_pico_dma_timer_configure(uint32_t timer, uint32_t numerator,
    uint32_t denominator);
int32_t bmx_pico_dma_timer_unclaim(uint32_t timer);
uint32_t bmx_pico_dma_timer_dreq(uint32_t timer);
void *bmx_pico_pio_find_program(const BMXEmbeddedString *name);
uint16_t *bmx_pico_pio_program_instructions(void *program);
uint32_t bmx_pico_pio_program_length(void *program);
int32_t bmx_pico_pio_program_origin(void *program);
uint32_t bmx_pico_pio_program_version(void *program);
uint32_t bmx_pico_pio_program_used_gpio_ranges(void *program);
uint32_t bmx_pico_pio_program_wrap_target(void *program);
uint32_t bmx_pico_pio_program_wrap(void *program);
int32_t bmx_pico_pio_can_add_imported_program(int32_t controller, void *program);
int32_t bmx_pico_pio_add_imported_program(int32_t controller, void *program);
int32_t bmx_pico_pio_sm_init_imported_program(int32_t controller, uint32_t state_machine,
    void *program, uint32_t offset);

#ifdef __cplusplus
}
#endif

#endif
