#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "hardware/sync.h"
#include "lwip/ip4_addr.h"
#include "lwip/netif.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#define BMX_PICO_WIFI_EVENT_CAPACITY 32u
#define BMX_PICO_WIFI_EVENT_MASK (BMX_PICO_WIFI_EVENT_CAPACITY - 1u)
#define BMX_PICO_WIFI_EVENT_SCAN_RESULT 1
#define BMX_PICO_WIFI_EVENT_SCAN_COMPLETE 2
#define BMX_PICO_WIFI_EVENT_LINK_STATE 3
#define BMX_PICO_WIFI_WPA_MAX_PASSWORD_LENGTH 64u
#define BMX_PICO_WIFI_WPA_SAE_MAX_PASSWORD_LENGTH 128u

typedef struct BMXPicoWiFiEvent {
    int32_t kind;
    uint8_t ssid_length;
    uint8_t ssid[32];
    uint8_t bssid[6];
    uint16_t channel;
    int16_t rssi;
    uint8_t security;
    int32_t link_status;
    uint32_t address;
    uint32_t netmask;
    uint32_t gateway;
} BMXPicoWiFiEvent;

static BMXPicoWiFiEvent bmx_pico_wifi_events[BMX_PICO_WIFI_EVENT_CAPACITY];
static volatile uint32_t bmx_pico_wifi_event_put;
static volatile uint32_t bmx_pico_wifi_event_get;
static volatile uint32_t bmx_pico_wifi_dropped_event_count;
static bool bmx_pico_wifi_is_initialized;
static bool bmx_pico_wifi_scan_requested;
static int32_t bmx_pico_wifi_last_link_status;

static uint32_t bmx_pico_wifi_pack_address(const ip4_addr_t *address) {
    return (uint32_t)ip4_addr1(address) |
        ((uint32_t)ip4_addr2(address) << 8) |
        ((uint32_t)ip4_addr3(address) << 16) |
        ((uint32_t)ip4_addr4(address) << 24);
}

static bool bmx_pico_wifi_queue_event(const BMXPicoWiFiEvent *event) {
    uint32_t interrupt_state = save_and_disable_interrupts();
    uint32_t put = bmx_pico_wifi_event_put;
    if (put - bmx_pico_wifi_event_get >= BMX_PICO_WIFI_EVENT_CAPACITY) {
        if (bmx_pico_wifi_dropped_event_count != UINT32_MAX)
            ++bmx_pico_wifi_dropped_event_count;
        restore_interrupts(interrupt_state);
        __sev();
        return false;
    }
    bmx_pico_wifi_events[put & BMX_PICO_WIFI_EVENT_MASK] = *event;
    __dmb();
    bmx_pico_wifi_event_put = put + 1u;
    restore_interrupts(interrupt_state);
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
    bmx_pico_wifi_dropped_event_count = 0;
    bmx_pico_wifi_scan_requested = false;
    bmx_pico_wifi_last_link_status = CYW43_LINK_DOWN;
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

int32_t bmx_pico_wifi_connect(const uint8_t *ssid, uint32_t ssid_length,
        const uint8_t *password, uint32_t password_length, uint32_t authentication) {
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    if (!bmx_pico_wifi_is_initialized) return PICO_ERROR_INVALID_STATE;
    if (!ssid || !ssid_length || ssid_length > 32u ||
            (password_length && !password)) return PICO_ERROR_INVALID_ARG;
    if (bmx_pico_wifi_scan_requested || cyw43_wifi_scan_active(&cyw43_state))
        return PICO_ERROR_RESOURCE_IN_USE;
    if (!password_length) authentication = CYW43_AUTH_OPEN;
    /* Match cyw43_ll_wifi_join's current limits exactly. In particular the
       transitional WPA3/WPA2 mode still uses the 64-byte WPA key path. */
    if ((authentication == CYW43_AUTH_WPA3_SAE_AES_PSK &&
            password_length > BMX_PICO_WIFI_WPA_SAE_MAX_PASSWORD_LENGTH) ||
            (authentication != CYW43_AUTH_OPEN &&
             authentication != CYW43_AUTH_WPA3_SAE_AES_PSK &&
             password_length > BMX_PICO_WIFI_WPA_MAX_PASSWORD_LENGTH))
        return PICO_ERROR_INVALID_ARG;
    return cyw43_wifi_join(&cyw43_state, ssid_length, ssid, password_length,
        password, authentication, NULL, CYW43_CHANNEL_NONE);
}

int32_t bmx_pico_wifi_disconnect(void) {
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    if (!bmx_pico_wifi_is_initialized) return PICO_ERROR_INVALID_STATE;
    return cyw43_wifi_leave(&cyw43_state, CYW43_ITF_STA);
}

int32_t bmx_pico_wifi_scan_active(void) {
    return bmx_pico_wifi_is_initialized &&
        (bmx_pico_wifi_scan_requested || cyw43_wifi_scan_active(&cyw43_state));
}

int32_t bmx_pico_wifi_link_status(void) {
    if (!bmx_pico_wifi_is_initialized) return CYW43_LINK_DOWN;
    cyw43_arch_lwip_begin();
    int32_t status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
    cyw43_arch_lwip_end();
    return status;
}

void bmx_pico_wifi_service(void) {
    if (!bmx_pico_wifi_is_initialized) return;
    if (bmx_pico_wifi_scan_requested && !cyw43_wifi_scan_active(&cyw43_state)) {
        BMXPicoWiFiEvent event = {0};
        event.kind = BMX_PICO_WIFI_EVENT_SCAN_COMPLETE;
        if (bmx_pico_wifi_queue_event(&event)) bmx_pico_wifi_scan_requested = false;
    }

    BMXPicoWiFiEvent link_event = {0};
    cyw43_arch_lwip_begin();
    link_event.link_status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
    if (link_event.link_status != bmx_pico_wifi_last_link_status) {
        struct netif *interface = &cyw43_state.netif[CYW43_ITF_STA];
        link_event.kind = BMX_PICO_WIFI_EVENT_LINK_STATE;
        link_event.address = bmx_pico_wifi_pack_address(netif_ip4_addr(interface));
        link_event.netmask = bmx_pico_wifi_pack_address(netif_ip4_netmask(interface));
        link_event.gateway = bmx_pico_wifi_pack_address(netif_ip4_gw(interface));
    }
    cyw43_arch_lwip_end();
    if (link_event.kind && bmx_pico_wifi_queue_event(&link_event))
        bmx_pico_wifi_last_link_status = link_event.link_status;
}

int32_t bmx_pico_wifi_take_event(int32_t *kind, uint8_t *ssid,
        int32_t *ssid_length, uint8_t *bssid, int32_t *channel,
        int32_t *rssi, int32_t *security, int32_t *link_status,
        uint32_t *address, uint32_t *netmask, uint32_t *gateway) {
    if (!kind || !ssid || !ssid_length || !bssid || !channel || !rssi ||
            !security || !link_status || !address || !netmask || !gateway ||
            get_core_num() != 0) return 0;
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
    *link_status = event.link_status;
    *address = event.address;
    *netmask = event.netmask;
    *gateway = event.gateway;
    return 1;
}

static uint32_t bmx_pico_wifi_live_address(int selector) {
    if (!bmx_pico_wifi_is_initialized || get_core_num() != 0) return 0;
    cyw43_arch_lwip_begin();
    struct netif *interface = &cyw43_state.netif[CYW43_ITF_STA];
    const ip4_addr_t *value = selector == 0 ? netif_ip4_addr(interface) :
        selector == 1 ? netif_ip4_netmask(interface) : netif_ip4_gw(interface);
    uint32_t result = bmx_pico_wifi_pack_address(value);
    cyw43_arch_lwip_end();
    return result;
}

uint32_t bmx_pico_wifi_ipv4_address(void) {
    return bmx_pico_wifi_live_address(0);
}

uint32_t bmx_pico_wifi_ipv4_netmask(void) {
    return bmx_pico_wifi_live_address(1);
}

uint32_t bmx_pico_wifi_ipv4_gateway(void) {
    return bmx_pico_wifi_live_address(2);
}

uint32_t bmx_pico_wifi_dropped_events(void) {
    return bmx_pico_wifi_dropped_event_count;
}
