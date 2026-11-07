/* Copyright (c) 2026 Bruce A Henderson and contributors. SPDX-License-Identifier: Zlib */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "lwip/apps/mqtt.h"
#include "lwip/dns.h"
extern int32_t bmx_embedded_wifi_initialized(void) __attribute__((weak));

#define EVENT_COUNT 8u
#define TOPIC_MAX 127u
#define PAYLOAD_MAX 1024u
typedef struct {
    int32_t kind, code, qos, retained;
    uint32_t topic_len, payload_len;
    char topic[TOPIC_MAX + 1];
    uint8_t payload[PAYLOAD_MAX];
} mqtt_event_t;
static mqtt_event_t events[EVENT_COUNT], incoming;
static uint32_t put, get, dropped;
static mqtt_client_t *client;
static struct mqtt_connect_client_info_t info;
static char host[256], client_id[128], user[128], password[128];
static uint16_t port;
static int32_t state;
static bool wanted, receiving, overflow;
static uintptr_t generation;

static void queue(const mqtt_event_t *event) {
    if (put - get == EVENT_COUNT) {
        if (dropped != UINT32_MAX) ++dropped;
        ++get;
    }
    events[put % EVENT_COUNT] = *event;
    ++put;
}
static void status(mqtt_client_t *c, void *arg, mqtt_connection_status_t result) {
    (void)c; (void)arg;
    mqtt_event_t event = {0};
    event.kind = result == MQTT_CONNECT_ACCEPTED ? 1 : 2;
    event.code = (int32_t)result;
    state = result == MQTT_CONNECT_ACCEPTED ? 2 : 0;
    if (wanted) queue(&event);
}
static void data(void *arg, const u8_t *bytes, u16_t length, u8_t flags) {
    (void)arg;
    if (!receiving) return;
    if (incoming.payload_len + length > PAYLOAD_MAX) overflow = true;
    else if (!overflow && length) {
        memcpy(incoming.payload + incoming.payload_len, bytes, length);
        incoming.payload_len += length;
    }
    if (flags & MQTT_DATA_FLAG_LAST) {
        if (!overflow) queue(&incoming);
        else { mqtt_event_t error = {.kind = 4, .code = -4}; queue(&error); }
        receiving = false;
    }
}
static void publish_in(void *arg, const char *topic, u32_t length) {
    (void)arg;
    memset(&incoming, 0, sizeof(incoming));
    incoming.kind = 3;
    incoming.topic_len = strlen(topic);
    overflow = incoming.topic_len > TOPIC_MAX || length > PAYLOAD_MAX;
    if (!overflow) memcpy(incoming.topic, topic, incoming.topic_len + 1);
    receiving = true;
    if (length == 0) {
        if (!overflow) queue(&incoming);
        else { mqtt_event_t error = {.kind = 4, .code = -4}; queue(&error); }
        receiving = false;
    }
}
static void resolved(const char *name, const ip_addr_t *address, void *arg) {
    (void)name;
    if (!wanted || (uintptr_t)arg != generation) return;
    if (!address) {
        state = 0;
        mqtt_event_t event = {.kind = 2, .code = -2}; queue(&event);
        return;
    }
    err_t result = mqtt_client_connect(client, address, port, status, NULL, &info);
    if (result != ERR_OK) {
        state = 0;
        mqtt_event_t event = {.kind = 2, .code = result}; queue(&event);
    }
}
static int copy(char *dest, size_t size, const uint8_t *source, bool required) {
    if (!source) { dest[0] = 0; return required ? -1 : 0; }
    size_t length = strnlen((const char *)source, size);
    if (length == size || (required && !length)) return -1;
    memcpy(dest, source, length + 1);
    return 0;
}
int32_t bmx_embedded_mqtt_start(const uint8_t *new_host, int32_t new_port,
        const uint8_t *new_id, const uint8_t *new_user, const uint8_t *new_password,
        int32_t keep_alive) {
    if (new_port < 1 || new_port > 65535 || keep_alive < 0 || keep_alive > 65535 ||
        copy(host, sizeof(host), new_host, true) ||
        copy(client_id, sizeof(client_id), new_id, true) ||
        copy(user, sizeof(user), new_user, false) ||
        copy(password, sizeof(password), new_password, false)) return -1;
    if (get_core_num() != 0 || !bmx_embedded_wifi_initialized ||
        !bmx_embedded_wifi_initialized()) return -3;
    cyw43_arch_lwip_begin();
    if (!client) client = mqtt_client_new();
    if (!client) { cyw43_arch_lwip_end(); return ERR_MEM; }
    if (state == 2) mqtt_disconnect(client);
    wanted = true;
    ++generation;
    state = 1;
    port = (uint16_t)new_port;
    memset(&info, 0, sizeof(info));
    info.client_id = client_id;
    info.client_user = new_user ? user : NULL;
    info.client_pass = new_password ? password : NULL;
    info.keep_alive = (uint16_t)keep_alive;
    mqtt_set_inpub_callback(client, publish_in, data, NULL);
    ip_addr_t address;
    err_t result = dns_gethostbyname(host, &address, resolved, (void *)generation);
    if (result == ERR_OK) resolved(host, &address, (void *)generation);
    else if (result != ERR_INPROGRESS) { state = 0; wanted = false; }
    cyw43_arch_lwip_end();
    return result == ERR_OK || result == ERR_INPROGRESS ? 0 : result;
}
void bmx_embedded_mqtt_stop(void) {
    if (!bmx_embedded_wifi_initialized || !bmx_embedded_wifi_initialized()) {
        wanted = false; state = 0; ++generation; put = get; return;
    }
    cyw43_arch_lwip_begin();
    wanted = false;
    ++generation;
    if (client && state == 2) mqtt_disconnect(client);
    state = 0;
    put = get;
    cyw43_arch_lwip_end();
}
int32_t bmx_embedded_mqtt_state(void) { return wanted && !state ? 1 : state; }
int32_t bmx_embedded_mqtt_publish(const uint8_t *topic, const uint8_t *payload,
        int32_t length, int32_t qos, int32_t retained) {
    if (!topic || length < 0 || length > UINT16_MAX || (length && !payload) || qos < 0 || qos > 2) return -1;
    if (!bmx_embedded_wifi_initialized || !bmx_embedded_wifi_initialized()) return -3;
    cyw43_arch_lwip_begin();
    err_t result = state == 2 ? mqtt_publish(client, (const char *)topic, payload,
        (u16_t)length, (u8_t)qos, !!retained, NULL, NULL) : ERR_CONN;
    cyw43_arch_lwip_end();
    return result;
}
int32_t bmx_embedded_mqtt_subscribe(const uint8_t *topic, int32_t qos) {
    if (!topic || qos < 0 || qos > 2) return -1;
    if (!bmx_embedded_wifi_initialized || !bmx_embedded_wifi_initialized()) return -3;
    cyw43_arch_lwip_begin();
    err_t result = state == 2 ? mqtt_subscribe(client, (const char *)topic,
        (u8_t)qos, NULL, NULL) : ERR_CONN;
    cyw43_arch_lwip_end();
    return result;
}
int32_t bmx_embedded_mqtt_take_event(int32_t *kind, int32_t *code, uint8_t *topic,
        int32_t *topic_length, uint8_t *payload, int32_t *payload_length,
        int32_t *qos, int32_t *retained) {
    if (!bmx_embedded_wifi_initialized || !bmx_embedded_wifi_initialized()) return 0;
    cyw43_arch_lwip_begin();
    if (get == put) { cyw43_arch_lwip_end(); return 0; }
    mqtt_event_t *event = &events[get % EVENT_COUNT];
    *kind = event->kind; *code = event->code;
    *topic_length = (int32_t)event->topic_len;
    *payload_length = (int32_t)event->payload_len;
    *qos = event->qos; *retained = event->retained;
    memcpy(topic, event->topic, event->topic_len);
    memcpy(payload, event->payload, event->payload_len);
    ++get;
    cyw43_arch_lwip_end();
    return 1;
}
uint32_t bmx_embedded_mqtt_dropped_events(void) { return dropped; }
