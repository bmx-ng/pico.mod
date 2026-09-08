#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "hardware/sync.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#define BMX_PICO_WIFI_EVENT_CAPACITY 32u
#define BMX_PICO_WIFI_EVENT_MASK (BMX_PICO_WIFI_EVENT_CAPACITY - 1u)
#define BMX_PICO_WIFI_EVENT_SCAN_RESULT 1
#define BMX_PICO_WIFI_EVENT_SCAN_COMPLETE 2

typedef struct BMXPicoWiFiEvent {
    int32_t kind;
    uint8_t ssid_length;
    uint8_t ssid[32];
    uint8_t bssid[6];
    uint16_t channel;
    int16_t rssi;
    uint8_t security;
} BMXPicoWiFiEvent;

static BMXPicoWiFiEvent bmx_pico_wifi_events[BMX_PICO_WIFI_EVENT_CAPACITY];
static volatile uint32_t bmx_pico_wifi_event_put;
static volatile uint32_t bmx_pico_wifi_event_get;
static volatile uint32_t bmx_pico_wifi_dropped;
static bool bmx_pico_wifi_is_initialized;
static bool bmx_pico_wifi_scan_requested;

static bool bmx_pico_wifi_queue_event(const BMXPicoWiFiEvent *event) {
    uint32_t put = bmx_pico_wifi_event_put;
    if (put - bmx_pico_wifi_event_get >= BMX_PICO_WIFI_EVENT_CAPACITY) {
        if (bmx_pico_wifi_dropped != UINT32_MAX) ++bmx_pico_wifi_dropped;
        __sev();
        return false;
    }
    bmx_pico_wifi_events[put & BMX_PICO_WIFI_EVENT_MASK] = *event;
    __dmb();
    bmx_pico_wifi_event_put = put + 1u;
    __sev();
    return true;
}

static int bmx_pico_wifi_scan_callback(void *environment,
        const cyw43_ev_scan_result_t *result) {
    (void)environment;
    BMXPicoWiFiEvent event = {0};
    event.kind = BMX_PICO_WIFI_EVENT_SCAN_RESULT;
    event.ssid_length = result->ssid_len <= sizeof(event.ssid) ?
        result->ssid_len : sizeof(event.ssid);
    memcpy(event.ssid, result->ssid, event.ssid_length);
    memcpy(event.bssid, result->bssid, sizeof(event.bssid));
    event.channel = result->channel;
    event.rssi = result->rssi;
    event.security = result->auth_mode;
    bmx_pico_wifi_queue_event(&event);
    return 0;
}

int32_t bmx_pico_wifi_initialize(uint32_t country) {
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    if (bmx_pico_wifi_is_initialized) return PICO_OK;

    int32_t result = cyw43_arch_init_with_country(country);
    if (result != PICO_OK) return result;
    cyw43_arch_enable_sta_mode();
    bmx_pico_wifi_event_put = 0;
    bmx_pico_wifi_event_get = 0;
    bmx_pico_wifi_dropped = 0;
    bmx_pico_wifi_scan_requested = false;
    bmx_pico_wifi_is_initialized = true;
    return PICO_OK;
}

int32_t bmx_pico_wifi_deinitialize(void) {
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    if (!bmx_pico_wifi_is_initialized) return PICO_OK;
    if (bmx_pico_wifi_scan_requested || cyw43_wifi_scan_active(&cyw43_state))
        return PICO_ERROR_RESOURCE_IN_USE;
    cyw43_arch_deinit();
    bmx_pico_wifi_is_initialized = false;
    bmx_pico_wifi_event_put = 0;
    bmx_pico_wifi_event_get = 0;
    return PICO_OK;
}

int32_t bmx_pico_wifi_initialized(void) {
    return bmx_pico_wifi_is_initialized;
}

int32_t bmx_pico_wifi_set_led(int32_t value) {
    if (!bmx_pico_wifi_is_initialized || get_core_num() != 0) return 0;
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, value != 0);
    return 1;
}

int32_t bmx_pico_wifi_get_led(void) {
    if (!bmx_pico_wifi_is_initialized || get_core_num() != 0) return -1;
    return cyw43_arch_gpio_get(CYW43_WL_GPIO_LED_PIN);
}

int32_t bmx_pico_wifi_start_scan(void) {
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    if (!bmx_pico_wifi_is_initialized) return PICO_ERROR_INVALID_STATE;
    if (bmx_pico_wifi_scan_requested || cyw43_wifi_scan_active(&cyw43_state))
        return PICO_ERROR_RESOURCE_IN_USE;

    cyw43_wifi_scan_options_t options = {0};
    int32_t result = cyw43_wifi_scan(&cyw43_state, &options, NULL,
        bmx_pico_wifi_scan_callback);
    if (result == PICO_OK) bmx_pico_wifi_scan_requested = true;
    return result;
}

int32_t bmx_pico_wifi_scan_active(void) {
    return bmx_pico_wifi_is_initialized &&
        (bmx_pico_wifi_scan_requested || cyw43_wifi_scan_active(&cyw43_state));
}

int32_t bmx_pico_wifi_link_status(void) {
    if (!bmx_pico_wifi_is_initialized) return CYW43_LINK_DOWN;
    return cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
}

void bmx_pico_wifi_service(void) {
    if (!bmx_pico_wifi_is_initialized || !bmx_pico_wifi_scan_requested ||
            cyw43_wifi_scan_active(&cyw43_state)) return;
    bmx_pico_wifi_scan_requested = false;
    BMXPicoWiFiEvent event = {0};
    event.kind = BMX_PICO_WIFI_EVENT_SCAN_COMPLETE;
    bmx_pico_wifi_queue_event(&event);
}

int32_t bmx_pico_wifi_take_event(int32_t *kind, uint8_t *ssid,
        int32_t *ssid_length, uint8_t *bssid, int32_t *channel,
        int32_t *rssi, int32_t *security) {
    if (!kind || !ssid || !ssid_length || !bssid || !channel || !rssi ||
            !security || get_core_num() != 0) return 0;
    uint32_t interrupt_state = save_and_disable_interrupts();
    uint32_t get = bmx_pico_wifi_event_get;
    if (get == bmx_pico_wifi_event_put) {
        restore_interrupts(interrupt_state);
        return 0;
    }
    BMXPicoWiFiEvent event = bmx_pico_wifi_events[get & BMX_PICO_WIFI_EVENT_MASK];
    bmx_pico_wifi_event_get = get + 1u;
    restore_interrupts(interrupt_state);

    *kind = event.kind;
    *ssid_length = event.ssid_length;
    memcpy(ssid, event.ssid, sizeof(event.ssid));
    memcpy(bssid, event.bssid, sizeof(event.bssid));
    *channel = event.channel;
    *rssi = event.rssi;
    *security = event.security;
    return 1;
}

uint32_t bmx_pico_wifi_dropped_scan_results(void) {
    return bmx_pico_wifi_dropped;
}
