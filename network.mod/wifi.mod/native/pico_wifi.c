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

typedef struct BMXEmbeddedWiFiEvent {
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
} BMXEmbeddedWiFiEvent;

static BMXEmbeddedWiFiEvent bmx_pico_wifi_events[BMX_PICO_WIFI_EVENT_CAPACITY];
static volatile uint32_t bmx_pico_wifi_event_put;
static volatile uint32_t bmx_pico_wifi_event_get;
static volatile uint32_t bmx_pico_wifi_dropped_event_count;
static bool bmx_pico_wifi_is_initialized;
static bool bmx_pico_wifi_claimed;
static bool bmx_pico_ble_claimed;
static bool bmx_pico_wifi_ap_active;
static bool bmx_pico_wifi_scan_requested;
static int32_t bmx_pico_wifi_last_link_status;

extern uint32_t bmx_embedded_net_active_socket_count(void) __attribute__((weak));

static uint32_t bmx_pico_wifi_authentication(uint32_t authentication) {
    switch (authentication) {
        case 0: return CYW43_AUTH_OPEN;
        case 1: return CYW43_AUTH_WPA_TKIP_PSK;
        case 2: return CYW43_AUTH_WPA2_AES_PSK;
        case 3: return CYW43_AUTH_WPA2_MIXED_PSK;
        case 4: return CYW43_AUTH_WPA3_SAE_AES_PSK;
        case 5: return CYW43_AUTH_WPA3_WPA2_AES_PSK;
        default: return UINT32_MAX;
    }
}

static uint32_t bmx_pico_wifi_pack_address(const ip4_addr_t *address) {
    return (uint32_t)ip4_addr1(address) |
        ((uint32_t)ip4_addr2(address) << 8) |
        ((uint32_t)ip4_addr3(address) << 16) |
        ((uint32_t)ip4_addr4(address) << 24);
}

static bool bmx_pico_wifi_queue_event(const BMXEmbeddedWiFiEvent *event) {
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
    BMXEmbeddedWiFiEvent event = {0};
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

static int32_t bmx_pico_radio_initialize(uint32_t country) {
    if (bmx_pico_wifi_is_initialized) return PICO_OK;

    int32_t result = cyw43_arch_init_with_country(country);
    if (result != PICO_OK) return result;
    bmx_pico_wifi_event_put = 0;
    bmx_pico_wifi_event_get = 0;
    bmx_pico_wifi_dropped_event_count = 0;
    bmx_pico_wifi_scan_requested = false;
    bmx_pico_wifi_last_link_status = CYW43_LINK_DOWN;
    bmx_pico_wifi_is_initialized = true;
    bmx_pico_wifi_ap_active = false;
    return PICO_OK;
}

int32_t bmx_embedded_wifi_initialize(uint32_t country) {
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    int32_t result = bmx_pico_radio_initialize(country);
    if (result == PICO_OK && !bmx_pico_wifi_claimed) {
        bmx_pico_wifi_event_put = bmx_pico_wifi_event_get = 0;
        bmx_pico_wifi_dropped_event_count = 0;
        bmx_pico_wifi_scan_requested = false;
        bmx_pico_wifi_last_link_status = CYW43_LINK_DOWN;
        cyw43_arch_enable_sta_mode();
        bmx_pico_wifi_claimed = true;
    }
    return result;
}

int32_t bmx_pico_radio_acquire_ble(void) {
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    int32_t result = bmx_pico_radio_initialize(CYW43_COUNTRY_WORLDWIDE);
    if (result == PICO_OK) bmx_pico_ble_claimed = true;
    return result;
}

static int32_t bmx_pico_radio_deinitialize(void) {
    if (!bmx_pico_wifi_is_initialized) return PICO_OK;
    if (bmx_pico_wifi_claimed || bmx_pico_ble_claimed) return PICO_OK;
    if (bmx_pico_wifi_ap_active) return PICO_ERROR_RESOURCE_IN_USE;
    if (bmx_pico_wifi_scan_requested || cyw43_wifi_scan_active(&cyw43_state))
        return PICO_ERROR_RESOURCE_IN_USE;
    if (bmx_embedded_net_active_socket_count &&
            bmx_embedded_net_active_socket_count())
        return PICO_ERROR_RESOURCE_IN_USE;
    cyw43_arch_deinit();
    bmx_pico_wifi_is_initialized = false;
    bmx_pico_wifi_ap_active = false;
    bmx_pico_wifi_event_put = 0;
    bmx_pico_wifi_event_get = 0;
    return PICO_OK;
}

int32_t bmx_embedded_wifi_deinitialize(void) {
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    if (!bmx_pico_wifi_claimed) return PICO_OK;
    if (bmx_pico_wifi_ap_active) return PICO_ERROR_RESOURCE_IN_USE;
    if (bmx_pico_wifi_scan_requested || cyw43_wifi_scan_active(&cyw43_state))
        return PICO_ERROR_RESOURCE_IN_USE;
    if (bmx_embedded_net_active_socket_count &&
            bmx_embedded_net_active_socket_count())
        return PICO_ERROR_RESOURCE_IN_USE;
    if (bmx_pico_ble_claimed) cyw43_arch_disable_sta_mode();
    bmx_pico_wifi_claimed = false;
    bmx_pico_wifi_event_put = bmx_pico_wifi_event_get = 0;
    return bmx_pico_radio_deinitialize();
}

int32_t bmx_embedded_wifi_initialized(void) {
    return bmx_pico_wifi_claimed;
}

int32_t bmx_pico_radio_release_ble(void) {
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    bmx_pico_ble_claimed = false;
    return bmx_pico_radio_deinitialize();
}

int32_t bmx_pico_radio_ble_release_allowed(void) {
    if (!bmx_pico_wifi_claimed && bmx_embedded_net_active_socket_count &&
            bmx_embedded_net_active_socket_count()) return PICO_ERROR_RESOURCE_IN_USE;
    return PICO_OK;
}

int32_t bmx_pico_radio_is_initialized(void) {
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

int32_t bmx_embedded_wifi_start_scan(void) {
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    if (!bmx_pico_wifi_claimed) return PICO_ERROR_INVALID_STATE;
    if (bmx_pico_wifi_scan_requested || cyw43_wifi_scan_active(&cyw43_state))
        return PICO_ERROR_RESOURCE_IN_USE;

    cyw43_wifi_scan_options_t options = {0};
    int32_t result = cyw43_wifi_scan(&cyw43_state, &options, NULL,
        bmx_pico_wifi_scan_callback);
    if (result == PICO_OK) bmx_pico_wifi_scan_requested = true;
    return result;
}

int32_t bmx_embedded_wifi_connect(const uint8_t *ssid, uint32_t ssid_length,
        const uint8_t *password, uint32_t password_length, uint32_t authentication) {
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    if (!bmx_pico_wifi_claimed) return PICO_ERROR_INVALID_STATE;
    if (!ssid || !ssid_length || ssid_length > 32u ||
            (password_length && !password)) return PICO_ERROR_INVALID_ARG;
    if (bmx_pico_wifi_scan_requested || cyw43_wifi_scan_active(&cyw43_state))
        return PICO_ERROR_RESOURCE_IN_USE;
    authentication = !password_length ? CYW43_AUTH_OPEN :
        bmx_pico_wifi_authentication(authentication);
    if (authentication == UINT32_MAX) return PICO_ERROR_INVALID_ARG;
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

int32_t bmx_embedded_wifi_disconnect(void) {
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    if (!bmx_pico_wifi_claimed) return PICO_ERROR_INVALID_STATE;
    return cyw43_wifi_leave(&cyw43_state, CYW43_ITF_STA);
}

int32_t bmx_embedded_wifi_scan_active(void) {
    return bmx_pico_wifi_claimed &&
        (bmx_pico_wifi_scan_requested || cyw43_wifi_scan_active(&cyw43_state));
}

int32_t bmx_embedded_wifi_link_status(void) {
    if (!bmx_pico_wifi_claimed) return CYW43_LINK_DOWN;
    cyw43_arch_lwip_begin();
    int32_t status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
    cyw43_arch_lwip_end();
    return status;
}

void bmx_embedded_wifi_service(void) {
    if (!bmx_pico_wifi_claimed) return;
    if (bmx_pico_wifi_scan_requested && !cyw43_wifi_scan_active(&cyw43_state)) {
        BMXEmbeddedWiFiEvent event = {0};
        event.kind = BMX_PICO_WIFI_EVENT_SCAN_COMPLETE;
        if (bmx_pico_wifi_queue_event(&event)) bmx_pico_wifi_scan_requested = false;
    }

    BMXEmbeddedWiFiEvent link_event = {0};
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

int32_t bmx_embedded_wifi_take_event(int32_t *kind, uint8_t *ssid,
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
    BMXEmbeddedWiFiEvent event = bmx_pico_wifi_events[get & BMX_PICO_WIFI_EVENT_MASK];
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
    if (!bmx_pico_wifi_claimed || get_core_num() != 0) return 0;
    cyw43_arch_lwip_begin();
    struct netif *interface = &cyw43_state.netif[CYW43_ITF_STA];
    const ip4_addr_t *value = selector == 0 ? netif_ip4_addr(interface) :
        selector == 1 ? netif_ip4_netmask(interface) : netif_ip4_gw(interface);
    uint32_t result = bmx_pico_wifi_pack_address(value);
    cyw43_arch_lwip_end();
    return result;
}

uint32_t bmx_embedded_wifi_ipv4_address(void) {
    return bmx_pico_wifi_live_address(0);
}

uint32_t bmx_embedded_wifi_ipv4_netmask(void) {
    return bmx_pico_wifi_live_address(1);
}

uint32_t bmx_embedded_wifi_ipv4_gateway(void) {
    return bmx_pico_wifi_live_address(2);
}

uint32_t bmx_embedded_wifi_dropped_events(void) {
    return bmx_pico_wifi_dropped_event_count;
}

int32_t bmx_pico_wifi_start_access_point(const uint8_t *ssid,
        uint32_t ssid_length, const uint8_t *password, uint32_t password_length,
        uint32_t authentication, uint32_t channel) {
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    if (!bmx_pico_wifi_claimed) return PICO_ERROR_INVALID_STATE;
    if (bmx_pico_wifi_ap_active) return PICO_ERROR_RESOURCE_IN_USE;
    if (!ssid || !ssid_length || ssid_length > 32u ||
            (password_length && !password) || channel < 1u || channel > 11u)
        return PICO_ERROR_INVALID_ARG;
    uint32_t auth = bmx_pico_wifi_authentication(authentication);
    if (password_length == 0u) {
        if (authentication != 0u) return PICO_ERROR_INVALID_ARG;
        auth = CYW43_AUTH_OPEN;
    } else if (password_length < 8u || password_length > 63u ||
            (auth != CYW43_AUTH_WPA_TKIP_PSK &&
             auth != CYW43_AUTH_WPA2_AES_PSK &&
             auth != CYW43_AUTH_WPA2_MIXED_PSK)) {
        return PICO_ERROR_INVALID_ARG;
    }
    char ssid_string[33];
    char password_string[64];
    if (memchr(ssid, 0, ssid_length) ||
            (password_length && memchr(password, 0, password_length)))
        return PICO_ERROR_INVALID_ARG;
    memcpy(ssid_string, ssid, ssid_length);
    ssid_string[ssid_length] = 0;
    if (password_length) {
        memcpy(password_string, password, password_length);
        password_string[password_length] = 0;
    }
    cyw43_wifi_ap_set_channel(&cyw43_state, channel);
    cyw43_arch_enable_ap_mode(ssid_string,
        password_length ? password_string : NULL, auth);
    memset(password_string, 0, sizeof(password_string));
    if (!(cyw43_state.itf_state & (1u << CYW43_ITF_AP)))
        return PICO_ERROR_GENERIC;
    bmx_pico_wifi_ap_active = true;
    return PICO_OK;
}

int32_t bmx_pico_wifi_stop_access_point(void) {
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    if (!bmx_pico_wifi_claimed) return PICO_ERROR_INVALID_STATE;
    if (!bmx_pico_wifi_ap_active) return PICO_OK;
    if (bmx_embedded_net_active_socket_count &&
            bmx_embedded_net_active_socket_count()) return PICO_ERROR_RESOURCE_IN_USE;
    cyw43_arch_disable_ap_mode();
    bmx_pico_wifi_ap_active = false;
    return PICO_OK;
}

int32_t bmx_pico_wifi_access_point_active(void) {
    return bmx_pico_wifi_claimed && bmx_pico_wifi_ap_active;
}

int32_t bmx_pico_wifi_access_point_client_count(uint32_t *count) {
    if (!count) return PICO_ERROR_INVALID_ARG;
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    if (!bmx_pico_wifi_access_point_active()) return PICO_ERROR_INVALID_STATE;
    int number = 16;
    uint8_t macs[16 * 6];
    cyw43_wifi_ap_get_stas(&cyw43_state, &number, macs);
    *count = number < 0 ? 0u : (uint32_t)number;
    return PICO_OK;
}

static uint32_t bmx_pico_wifi_ap_address(int selector) {
    if (!bmx_pico_wifi_access_point_active() || get_core_num() != 0) return 0;
    cyw43_arch_lwip_begin();
    struct netif *interface = &cyw43_state.netif[CYW43_ITF_AP];
    const ip4_addr_t *value = selector == 0 ? netif_ip4_addr(interface) :
        selector == 1 ? netif_ip4_netmask(interface) : netif_ip4_gw(interface);
    uint32_t result = bmx_pico_wifi_pack_address(value);
    cyw43_arch_lwip_end();
    return result;
}

uint32_t bmx_pico_wifi_access_point_ipv4_address(void) { return bmx_pico_wifi_ap_address(0); }
uint32_t bmx_pico_wifi_access_point_ipv4_netmask(void) { return bmx_pico_wifi_ap_address(1); }
uint32_t bmx_pico_wifi_access_point_ipv4_gateway(void) { return bmx_pico_wifi_ap_address(2); }
