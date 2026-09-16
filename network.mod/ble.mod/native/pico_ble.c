#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "btstack.h"
#include "ble/att_db_util.h"
#include "hardware/sync.h"
#include "pico/cyw43_arch.h"
#include "pico/error.h"
#include "pico/stdlib.h"

extern int32_t bmx_pico_radio_acquire_ble(void);
extern int32_t bmx_pico_radio_release_ble(void);
extern int32_t bmx_pico_radio_is_initialized(void);
extern int32_t bmx_pico_radio_ble_release_allowed(void);

#define BLE_QUEUE_SIZE 8u
#define BLE_QUEUE_MASK (BLE_QUEUE_SIZE - 1u)
#define BLE_UNSUPPORTED (-100)

typedef struct {
    int32_t kind, status, address_type, event_type, rssi;
    int32_t data_length, connection_handle, attribute_id;
    int32_t notifications, indications, attribute_end, parent_attribute, properties;
    uint8_t address[6];
    uint8_t data[512];
} BLEEvent;

static BLEEvent events[BLE_QUEUE_SIZE];
static volatile uint32_t event_put, event_get, dropped_events;
static BLEEvent critical_events[4];
static volatile uint32_t critical_put, critical_get;
static btstack_packet_callback_registration_t packet_registration;
static bool initialized, ready, scanning, advertising, connecting;
static bool scan_complete_pending, advertising_complete_pending, connect_timeout_pending;
static bool stack_configured;
static bool advertising_auto_restart, advertising_resume_pending;
static uint32_t advertising_duration;
static uint32_t scan_deadline, advertising_deadline, connect_deadline;
static char device_name[30];
static hci_con_handle_t connection_handle = HCI_CON_HANDLE_INVALID;
static uint8_t connection_address[6];
static int connection_address_type, connection_role;
static int connection_interval, connection_latency, connection_timeout;
static uint8_t connecting_address[6];
static int connecting_address_type;
enum { QUERY_NONE, QUERY_SERVICES, QUERY_CHARACTERISTICS, QUERY_DESCRIPTORS,
    QUERY_READ, QUERY_WRITE };
static int gatt_query, gatt_parent, gatt_attribute;
static bool gatt_read_received;
static uint8_t gatt_write_value[512];

#define MAX_GATT_SERVICES 8
#define MAX_GATT_CHARACTERISTICS 16
typedef struct {
    uint8_t uuid[16];
    uint8_t uuid_length;
} BLEService;
typedef struct {
    uint8_t uuid[16];
    uint8_t uuid_length;
    uint8_t value[512];
    uint16_t value_length, capacity, value_handle, ccc_handle;
    uint16_t flags;
    uint8_t service_id, subscriptions;
} BLECharacteristic;
static BLEService services[MAX_GATT_SERVICES];
static BLECharacteristic characteristics[MAX_GATT_CHARACTERISTICS];
static uint8_t service_count, characteristic_count;
static bool att_server_configured;
static bool queue_event(const BLEEvent *event);
static bool queue_critical_event(const BLEEvent *event);

static int parse_uuid(const uint8_t *text, uint32_t length, uint8_t *uuid) {
    if (!text) return 0;
    int digits = 0;
    for (uint32_t i = 0; i < length; ++i) if (text[i] != '-') ++digits;
    if (digits != 4 && digits != 32) return 0;
    int position = 0, high = -1;
    for (uint32_t i = 0; i < length; ++i) {
        int nibble;
        uint8_t ch = text[i];
        if (ch == '-') continue;
        if (ch >= '0' && ch <= '9') nibble = ch - '0';
        else if (ch >= 'a' && ch <= 'f') nibble = ch - 'a' + 10;
        else if (ch >= 'A' && ch <= 'F') nibble = ch - 'A' + 10;
        else return 0;
        if (high < 0) high = nibble;
        else {
            uuid[position++] = (high << 4) | nibble;
            high = -1;
        }
    }
    return digits / 2;
}

static BLECharacteristic *characteristic_by_handle(uint16_t handle,
        bool *is_ccc, int *identifier) {
    for (int index = 0; index < characteristic_count; ++index) {
        BLECharacteristic *item = &characteristics[index];
        if (handle == item->value_handle || handle == item->ccc_handle) {
            if (is_ccc) *is_ccc = handle == item->ccc_handle;
            if (identifier) *identifier = index + 1;
            return item;
        }
    }
    return NULL;
}

static uint16_t server_read(hci_con_handle_t handle, uint16_t attribute,
        uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
    (void)handle;
    bool ccc = false;
    BLECharacteristic *item = characteristic_by_handle(attribute, &ccc, NULL);
    if (!item) return 0;
    if (ccc) {
        uint8_t bits[2] = {item->subscriptions, 0};
        return att_read_callback_handle_blob(bits, 2, offset, buffer, buffer_size);
    }
    return att_read_callback_handle_blob(item->value, item->value_length,
        offset, buffer, buffer_size);
}

static int server_write(hci_con_handle_t handle, uint16_t attribute,
        uint16_t mode, uint16_t offset, uint8_t *buffer, uint16_t length) {
    bool ccc = false;
    int identifier = 0;
    BLECharacteristic *item = characteristic_by_handle(attribute, &ccc, &identifier);
    if (!item) return ATT_ERROR_WRITE_NOT_PERMITTED;
    if (mode != ATT_TRANSACTION_MODE_NONE) return ATT_ERROR_REQUEST_NOT_SUPPORTED;
    if (offset != 0) return ATT_ERROR_INVALID_OFFSET;
    BLEEvent event = {.connection_handle = handle, .attribute_id = identifier};
    if (ccc) {
        if (length != 2) return ATT_ERROR_INVALID_ATTRIBUTE_VALUE_LENGTH;
        item->subscriptions = buffer[0] & 3;
        event.kind = 9;
        event.notifications = (item->subscriptions & 1u) != 0;
        event.indications = (item->subscriptions & 2u) != 0;
    } else {
        if (length > item->capacity) return ATT_ERROR_INVALID_ATTRIBUTE_VALUE_LENGTH;
        memcpy(item->value, buffer, length);
        item->value_length = length;
        event.kind = 8;
        event.data_length = length;
        memcpy(event.data, buffer, length);
    }
    queue_event(&event);
    return 0;
}

static void server_packet_handler(uint8_t packet_type, uint16_t channel,
        uint8_t *packet, uint16_t size) {
    (void)channel;
    (void)size;
    if (packet_type != HCI_EVENT_PACKET || !initialized) return;
    BLEEvent event = {0};
    switch (hci_event_packet_get_type(packet)) {
        case ATT_EVENT_MTU_EXCHANGE_COMPLETE:
            event.kind = 19;
            event.connection_handle = att_event_mtu_exchange_complete_get_handle(packet);
            event.attribute_end = att_event_mtu_exchange_complete_get_MTU(packet);
            queue_event(&event);
            break;
        case ATT_EVENT_HANDLE_VALUE_INDICATION_COMPLETE:
            event.kind = 20;
            event.connection_handle = att_event_handle_value_indication_complete_get_conn_handle(packet);
            event.status = att_event_handle_value_indication_complete_get_status(packet);
            int identifier = 0;
            characteristic_by_handle(
                att_event_handle_value_indication_complete_get_attribute_handle(packet),
                NULL, &identifier);
            event.attribute_id = identifier;
            event.indications = 1;
            event.properties = event.status == 0;
            queue_critical_event(&event);
            break;
    }
}

static void configure_att_server(void) {
    att_db_util_init();
    for (int service_index = 0; service_index < service_count; ++service_index) {
        BLEService *service = &services[service_index];
        if (service->uuid_length == 2) {
            att_db_util_add_service_uuid16((service->uuid[0] << 8) | service->uuid[1]);
        } else {
            att_db_util_add_service_uuid128(service->uuid);
        }
        for (int index = 0; index < characteristic_count; ++index) {
            BLECharacteristic *item = &characteristics[index];
            if (item->service_id != service_index + 1) continue;
            uint16_t properties = ATT_PROPERTY_DYNAMIC;
            if (item->flags & 1u) properties |= ATT_PROPERTY_READ;
            if (item->flags & 2u) properties |= ATT_PROPERTY_WRITE;
            if (item->flags & 4u) properties |= ATT_PROPERTY_NOTIFY;
            if (item->flags & 8u) properties |= ATT_PROPERTY_INDICATE;
            if (item->flags & 16u) properties |= ATT_PROPERTY_WRITE_WITHOUT_RESPONSE;
            if (item->uuid_length == 2) {
                item->value_handle = att_db_util_add_characteristic_uuid16(
                    (item->uuid[0] << 8) | item->uuid[1], properties,
                    ATT_SECURITY_NONE, ATT_SECURITY_NONE, NULL, 0);
            } else {
                item->value_handle = att_db_util_add_characteristic_uuid128(
                    item->uuid, properties, ATT_SECURITY_NONE, ATT_SECURITY_NONE, NULL, 0);
            }
            if (item->flags & (4u | 8u)) item->ccc_handle = item->value_handle + 1u;
        }
    }
    if (att_server_configured) {
        att_set_db(att_db_util_get_address());
    } else {
        att_server_init(att_db_util_get_address(), server_read, server_write);
        att_server_register_packet_handler(server_packet_handler);
    }
    att_server_configured = true;
}

static bool queue_event(const BLEEvent *event) {
    uint32_t irq = save_and_disable_interrupts();
    uint32_t put = event_put;
    if (put - event_get == BLE_QUEUE_SIZE) {
        if (dropped_events != UINT32_MAX) ++dropped_events;
        restore_interrupts(irq);
        return false;
    }
    events[put & BLE_QUEUE_MASK] = *event;
    __dmb();
    event_put = put + 1u;
    restore_interrupts(irq);
    __sev();
    return true;
}

static bool queue_critical_event(const BLEEvent *event) {
    uint32_t irq = save_and_disable_interrupts();
    uint32_t put = critical_put;
    if (put - critical_get == 4u) {
        if (dropped_events != UINT32_MAX) ++dropped_events;
        restore_interrupts(irq);
        return false;
    }
    critical_events[put & 3u] = *event;
    __dmb();
    critical_put = put + 1u;
    restore_interrupts(irq);
    __sev();
    return true;
}

static void encode_uuid(BLEEvent *event, uint16_t uuid16, const uint8_t *uuid128) {
    if (uuid16) {
        snprintf((char *)event->data, sizeof(event->data), "%04x", uuid16);
    } else {
        strncpy((char *)event->data, uuid128_to_str(uuid128), sizeof(event->data) - 1u);
    }
    event->data_length = strlen((char *)event->data);
}

static void gatt_packet_handler(uint8_t packet_type, uint16_t channel,
        uint8_t *packet, uint16_t size) {
    (void)channel;
    (void)size;
    if (packet_type != HCI_EVENT_PACKET || !initialized) return;
    BLEEvent event = {0};
    event.connection_handle = connection_handle;
    switch (hci_event_packet_get_type(packet)) {
        case GATT_EVENT_MTU:
            event.kind = 19;
            event.connection_handle = gatt_event_mtu_get_handle(packet);
            event.attribute_end = gatt_event_mtu_get_MTU(packet);
            queue_event(&event);
            break;
        case GATT_EVENT_SERVICE_QUERY_RESULT: {
            gatt_client_service_t service;
            gatt_event_service_query_result_get_service(packet, &service);
            event.kind = 10;
            event.attribute_id = service.start_group_handle;
            event.attribute_end = service.end_group_handle;
            encode_uuid(&event, service.uuid16, service.uuid128);
            queue_event(&event);
            break;
        }
        case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT: {
            gatt_client_characteristic_t characteristic;
            gatt_event_characteristic_query_result_get_characteristic(packet, &characteristic);
            event.kind = 12;
            event.attribute_id = characteristic.start_handle;
            event.attribute_end = characteristic.value_handle;
            event.parent_attribute = gatt_parent;
            event.properties = characteristic.properties;
            encode_uuid(&event, characteristic.uuid16, characteristic.uuid128);
            queue_event(&event);
            break;
        }
        case GATT_EVENT_ALL_CHARACTERISTIC_DESCRIPTORS_QUERY_RESULT: {
            gatt_client_characteristic_descriptor_t descriptor;
            gatt_event_all_characteristic_descriptors_query_result_get_characteristic_descriptor(
                packet, &descriptor);
            event.kind = 14;
            event.attribute_id = descriptor.handle;
            event.parent_attribute = gatt_parent;
            encode_uuid(&event, descriptor.uuid16, descriptor.uuid128);
            queue_event(&event);
            break;
        }
        case GATT_EVENT_CHARACTERISTIC_VALUE_QUERY_RESULT:
            if (gatt_query != QUERY_READ) break;
            event.kind = 16;
            event.attribute_id = gatt_event_characteristic_value_query_result_get_value_handle(packet);
            event.data_length = gatt_event_characteristic_value_query_result_get_value_length(packet);
            if (event.data_length > (int)sizeof(event.data)) event.data_length = sizeof(event.data);
            memcpy(event.data, gatt_event_characteristic_value_query_result_get_value(packet),
                event.data_length);
            gatt_read_received = queue_event(&event);
            break;
        case GATT_EVENT_QUERY_COMPLETE:
            event.status = gatt_event_query_complete_get_att_status(packet);
            event.parent_attribute = gatt_parent;
            event.attribute_id = gatt_attribute;
            switch (gatt_query) {
                case QUERY_SERVICES: event.kind = 11; break;
                case QUERY_CHARACTERISTICS: event.kind = 13; break;
                case QUERY_DESCRIPTORS: event.kind = 15; break;
                case QUERY_READ:
                    if (event.status || !gatt_read_received) {
                        event.kind = 16;
                        if (!event.status) event.status = PICO_ERROR_INSUFFICIENT_RESOURCES;
                    }
                    break;
                case QUERY_WRITE: event.kind = 17; break;
            }
            gatt_query = QUERY_NONE;
            if (event.kind) queue_critical_event(&event);
            break;
    }
}

static void packet_handler(uint8_t packet_type, uint16_t channel,
        uint8_t *packet, uint16_t size) {
    (void)channel;
    (void)size;
    if (packet_type != HCI_EVENT_PACKET || !initialized) return;
    BLEEvent event = {0};
    switch (hci_event_packet_get_type(packet)) {
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING && !ready) {
                ready = true;
                event.kind = 1;
                queue_critical_event(&event);
            }
            break;
        case GAP_EVENT_ADVERTISING_REPORT:
            if (!scanning) break;
            event.kind = 2;
            event.event_type = gap_event_advertising_report_get_advertising_event_type(packet);
            event.address_type = gap_event_advertising_report_get_address_type(packet);
            gap_event_advertising_report_get_address(packet, event.address);
            event.rssi = gap_event_advertising_report_get_rssi(packet);
            event.data_length = gap_event_advertising_report_get_data_length(packet);
            if (event.data_length > (int)sizeof(event.data))
                event.data_length = sizeof(event.data);
            memcpy(event.data, gap_event_advertising_report_get_data(packet), event.data_length);
            queue_event(&event);
            break;
        case HCI_EVENT_META_GAP:
            if (hci_event_gap_meta_get_subevent_code(packet) != GAP_SUBEVENT_LE_CONNECTION_COMPLETE)
                break;
            event.kind = 5;
            event.status = gap_subevent_le_connection_complete_get_status(packet);
            if (event.status && !connecting) break;
            connecting = false;
            connect_timeout_pending = false;
            event.connection_handle = gap_subevent_le_connection_complete_get_connection_handle(packet);
            event.properties = gap_subevent_le_connection_complete_get_role(packet);
            event.address_type = gap_subevent_le_connection_complete_get_peer_address_type(packet);
            gap_subevent_le_connection_complete_get_peer_address(packet, event.address);
            if (!event.status) {
                if (advertising && event.properties == 1) {
                    advertising = false;
                    advertising_resume_pending = advertising_auto_restart;
                    if (!advertising_resume_pending) advertising_complete_pending = true;
                }
                connection_handle = event.connection_handle;
                connection_address_type = event.address_type;
                connection_role = event.properties;
                memcpy(connection_address, event.address, 6);
                connection_interval = gap_subevent_le_connection_complete_get_conn_interval(packet);
                connection_latency = gap_subevent_le_connection_complete_get_conn_latency(packet);
                connection_timeout = gap_subevent_le_connection_complete_get_supervision_timeout(packet);
            }
            queue_critical_event(&event);
            break;
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            event.connection_handle = hci_event_disconnection_complete_get_connection_handle(packet);
            if (event.connection_handle != connection_handle) break;
            event.kind = 6;
            event.status = hci_event_disconnection_complete_get_reason(packet);
            event.address_type = connection_address_type;
            event.properties = connection_role;
            memcpy(event.address, connection_address, 6);
            connection_handle = HCI_CON_HANDLE_INVALID;
            gatt_query = QUERY_NONE;
            for (int index = 0; index < characteristic_count; ++index)
                characteristics[index].subscriptions = 0;
            if (advertising_resume_pending) {
                gap_advertisements_enable(1);
                advertising = true;
                advertising_resume_pending = false;
                advertising_deadline = advertising_duration ?
                    to_ms_since_boot(get_absolute_time()) + advertising_duration : 0;
            }
            queue_critical_event(&event);
            break;
        default:
            break;
    }
}

int32_t bmx_embedded_ble_initialize(const uint8_t *name, uint32_t name_length) {
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    if (initialized) return PICO_OK;
    if (name_length > sizeof(device_name) - 1u || (name_length && !name))
        return PICO_ERROR_INVALID_ARG;
    int32_t result = bmx_pico_radio_acquire_ble();
    if (result != PICO_OK) return result;
    if (name_length) memcpy(device_name, name, name_length);
    device_name[name_length] = 0;
    event_put = event_get = critical_put = critical_get = dropped_events = 0;
    ready = scanning = advertising = connecting = false;
    scan_complete_pending = advertising_complete_pending = connect_timeout_pending = false;
    advertising_resume_pending = false;
    connection_handle = HCI_CON_HANDLE_INVALID;
    gatt_query = QUERY_NONE;
    initialized = true;
    cyw43_arch_lwip_begin();
    if (!stack_configured) {
        l2cap_init();
        sm_init();
        gatt_client_init();
        stack_configured = true;
    }
    configure_att_server();
    packet_registration.callback = packet_handler;
    hci_add_event_handler(&packet_registration);
    result = hci_power_control(HCI_POWER_ON);
    cyw43_arch_lwip_end();
    if (result) {
        cyw43_arch_lwip_begin();
        hci_remove_event_handler(&packet_registration);
        cyw43_arch_lwip_end();
        initialized = false;
        bmx_pico_radio_release_ble();
        if (!bmx_pico_radio_is_initialized()) {
            stack_configured = false;
            att_server_configured = false;
        }
    }
    return result;
}

int32_t bmx_embedded_ble_deinitialize(void) {
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    if (!initialized) return PICO_OK;
    if (connection_handle != HCI_CON_HANDLE_INVALID || connecting)
        return PICO_ERROR_RESOURCE_IN_USE;
    int32_t allowed = bmx_pico_radio_ble_release_allowed();
    if (allowed != PICO_OK) return allowed;
    cyw43_arch_lwip_begin();
    if (scanning) gap_stop_scan();
    if (advertising) gap_advertisements_enable(0);
    hci_remove_event_handler(&packet_registration);
    hci_power_control(HCI_POWER_OFF);
    cyw43_arch_lwip_end();
    initialized = ready = scanning = advertising = false;
    event_put = event_get = critical_put = critical_get = 0;
    int32_t result = bmx_pico_radio_release_ble();
    if (!bmx_pico_radio_is_initialized()) {
        stack_configured = false;
        att_server_configured = false;
    }
    return result;
}

int32_t bmx_embedded_ble_initialized(void) { return initialized; }
int32_t bmx_embedded_ble_ready(void) { return ready; }

int32_t bmx_embedded_ble_start_scan(uint32_t duration, int32_t active,
        int32_t filter_duplicates) {
    (void)filter_duplicates;
    if (!initialized || !ready) return PICO_ERROR_INVALID_STATE;
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    if (scanning) return PICO_ERROR_RESOURCE_IN_USE;
    cyw43_arch_lwip_begin();
    gap_set_scan_parameters(active != 0, 0x30, 0x30);
    gap_start_scan();
    cyw43_arch_lwip_end();
    scanning = true;
    scan_deadline = duration ? to_ms_since_boot(get_absolute_time()) + duration : 0;
    return PICO_OK;
}

int32_t bmx_embedded_ble_stop_scan(void) {
    if (!initialized) return PICO_ERROR_INVALID_STATE;
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    if (!scanning) return PICO_OK;
    cyw43_arch_lwip_begin();
    gap_stop_scan();
    cyw43_arch_lwip_end();
    scanning = false;
    scan_complete_pending = true;
    return PICO_OK;
}
int32_t bmx_embedded_ble_scan_active(void) { return scanning; }

int32_t bmx_embedded_ble_start_advertising(int32_t service_id, uint32_t duration,
        int32_t connectable, int32_t auto_restart) {
    if (!initialized || !ready) return PICO_ERROR_INVALID_STATE;
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    if (service_id < 0 || service_id > service_count) return PICO_ERROR_INVALID_ARG;
    if (advertising) return PICO_ERROR_RESOURCE_IN_USE;
    uint8_t data[31] = {2, 1, 6};
    uint8_t length = 3;
    if (service_id) {
        const BLEService *service = &services[service_id - 1];
        data[length++] = service->uuid_length + 1;
        data[length++] = service->uuid_length == 2 ? 3 : 7;
        for (int index = service->uuid_length - 1; index >= 0; --index)
            data[length++] = service->uuid[index];
    }
    size_t name_length = strlen(device_name);
    if (name_length > sizeof(data) - length - 2u)
        name_length = sizeof(data) - length - 2u;
    if (name_length) {
        data[length++] = name_length + 1;
        data[length++] = 9;
        memcpy(data + length, device_name, name_length);
        length += name_length;
    }
    bd_addr_t null_address = {0};
    cyw43_arch_lwip_begin();
    gap_advertisements_set_params(0x30, 0x60, connectable ? 0 : 3,
        0, null_address, 7, 0);
    gap_advertisements_set_data(length, data);
    gap_advertisements_enable(1);
    cyw43_arch_lwip_end();
    advertising = true;
    advertising_auto_restart = auto_restart != 0;
    advertising_duration = duration;
    advertising_deadline = duration ? to_ms_since_boot(get_absolute_time()) + duration : 0;
    return PICO_OK;
}
int32_t bmx_embedded_ble_stop_advertising(void) {
    if (!initialized) return PICO_ERROR_INVALID_STATE;
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    if (!advertising) {
        if (advertising_resume_pending) {
            advertising_resume_pending = false;
            advertising_complete_pending = true;
        }
        return PICO_OK;
    }
    cyw43_arch_lwip_begin();
    gap_advertisements_enable(0);
    cyw43_arch_lwip_end();
    advertising = false;
    advertising_complete_pending = true;
    return PICO_OK;
}
int32_t bmx_embedded_ble_advertising_active(void) { return advertising; }

int32_t bmx_embedded_ble_connect(int32_t address_type, const uint8_t *address,
        uint32_t timeout) {
    if (!initialized || !ready) return PICO_ERROR_INVALID_STATE;
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    if (!address || address_type < 0 || address_type > 3) return PICO_ERROR_INVALID_ARG;
    if (connecting || connection_handle != HCI_CON_HANDLE_INVALID)
        return PICO_ERROR_RESOURCE_IN_USE;
    cyw43_arch_lwip_begin();
    int32_t result = gap_connect(address, address_type);
    cyw43_arch_lwip_end();
    if (result == 0) {
        connecting = true;
        memcpy(connecting_address, address, 6);
        connecting_address_type = address_type;
        connect_deadline = timeout ? to_ms_since_boot(get_absolute_time()) + timeout : 0;
    }
    return result;
}
int32_t bmx_embedded_ble_cancel_connect(void) {
    if (!initialized) return PICO_ERROR_INVALID_STATE;
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    if (!connecting) return PICO_OK;
    cyw43_arch_lwip_begin();
    int32_t result = gap_connect_cancel();
    cyw43_arch_lwip_end();
    if (!result) connecting = false;
    return result;
}
int32_t bmx_embedded_ble_disconnect(int32_t handle) {
    if (!initialized || handle != connection_handle) return PICO_ERROR_INVALID_ARG;
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    cyw43_arch_lwip_begin();
    int32_t result = gap_disconnect(handle);
    cyw43_arch_lwip_end();
    return result;
}

int32_t bmx_embedded_ble_take_event(int32_t *kind, int32_t *status,
        int32_t *address_type, uint8_t *address, int32_t *event_type,
        int32_t *rssi, uint8_t *data, int32_t *data_length,
        int32_t *handle, int32_t *attribute_id, int32_t *notifications,
        int32_t *indications, int32_t *attribute_end, int32_t *parent_attribute,
        int32_t *properties) {
    if (!kind || !status || !address_type || !address || !event_type || !rssi ||
            !data || !data_length || !handle || !attribute_id || !notifications ||
            !indications || !attribute_end || !parent_attribute || !properties ||
            get_core_num() != 0) return 0;
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (scanning && scan_deadline && (int32_t)(now - scan_deadline) >= 0)
        bmx_embedded_ble_stop_scan();
    if (advertising && advertising_deadline && (int32_t)(now - advertising_deadline) >= 0)
        bmx_embedded_ble_stop_advertising();
    if (connecting && connect_deadline && (int32_t)(now - connect_deadline) >= 0) {
        if (bmx_embedded_ble_cancel_connect() == 0) connect_timeout_pending = true;
    }
    uint32_t irq = save_and_disable_interrupts();
    uint32_t get = event_get;
    BLEEvent event = {0};
    if (get != event_put) {
        event = events[get & BLE_QUEUE_MASK];
        event_get = get + 1u;
    } else if (critical_get != critical_put) {
        event = critical_events[critical_get & 3u];
        critical_get += 1u;
    } else if (scan_complete_pending) {
        scan_complete_pending = false;
        event.kind = 3;
    } else if (advertising_complete_pending) {
        advertising_complete_pending = false;
        event.kind = 7;
    } else if (connect_timeout_pending) {
        connect_timeout_pending = false;
        event.kind = 5;
        event.status = PICO_ERROR_TIMEOUT;
        event.address_type = connecting_address_type;
        memcpy(event.address, connecting_address, 6);
    } else {
        restore_interrupts(irq);
        return 0;
    }
    restore_interrupts(irq);
    *kind = event.kind;
    *status = event.status;
    *address_type = event.address_type;
    memcpy(address, event.address, 6);
    *event_type = event.event_type;
    *rssi = event.rssi;
    *data_length = event.data_length;
    memcpy(data, event.data, event.data_length);
    *handle = event.connection_handle;
    *attribute_id = event.attribute_id;
    *notifications = event.notifications;
    *indications = event.indications;
    *attribute_end = event.attribute_end;
    *parent_attribute = event.parent_attribute;
    *properties = event.properties;
    return 1;
}
uint32_t bmx_embedded_ble_dropped_events(void) { return dropped_events; }

/* Unsupported controls return a distinct error until their BTstack paths
   exist. Never report a successful controller operation without performing it. */
#define UNSUPPORTED(name, args) int32_t name args { return BLE_UNSUPPORTED; }
int32_t bmx_embedded_ble_exchange_mtu(int32_t handle) {
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    if (!initialized || !ready) return PICO_ERROR_INVALID_STATE;
    if (handle != connection_handle) return PICO_ERROR_INVALID_ARG;
    cyw43_arch_lwip_begin();
    gatt_client_send_mtu_negotiation(gatt_packet_handler, handle);
    cyw43_arch_lwip_end();
    return PICO_OK;
}
int32_t bmx_embedded_ble_connection_mtu(int32_t handle) {
    if (handle != connection_handle) return PICO_ERROR_INVALID_ARG;
    uint16_t mtu = 23;
    cyw43_arch_lwip_begin();
    if (gatt_client_get_mtu(handle, &mtu) != 0) mtu = att_server_get_mtu(handle);
    cyw43_arch_lwip_end();
    return mtu;
}
int32_t bmx_embedded_ble_connection_count(void) {
    return connection_handle == HCI_CON_HANDLE_INVALID ? 0 : 1;
}
int32_t bmx_embedded_ble_connection_handle(int32_t index, int32_t *handle) {
    if (!handle || index != 0 || connection_handle == HCI_CON_HANDLE_INVALID)
        return PICO_ERROR_INVALID_ARG;
    *handle = connection_handle;
    return PICO_OK;
}
int32_t bmx_embedded_ble_connection_info(int32_t handle, int32_t *role,
        int32_t *address_type, uint8_t *address, int32_t *interval,
        int32_t *latency, int32_t *timeout) {
    if (handle != connection_handle || !role || !address_type || !address ||
            !interval || !latency || !timeout) return PICO_ERROR_INVALID_ARG;
    *role = connection_role;
    *address_type = connection_address_type;
    memcpy(address, connection_address, 6);
    *interval = connection_interval;
    *latency = connection_latency;
    *timeout = connection_timeout;
    return PICO_OK;
}
UNSUPPORTED(bmx_embedded_ble_connection_rssi, (int32_t handle, int32_t *rssi))
UNSUPPORTED(bmx_embedded_ble_connection_phy, (int32_t handle, int32_t *tx, int32_t *rx))
UNSUPPORTED(bmx_embedded_ble_update_connection_parameters,
    (int32_t handle, int32_t min, int32_t max, int32_t latency, int32_t timeout))
UNSUPPORTED(bmx_embedded_ble_set_preferred_phy,
    (int32_t handle, int32_t tx, int32_t rx, int32_t coded))
UNSUPPORTED(bmx_embedded_ble_configure_security,
    (int32_t io, int32_t bonding, int32_t auth, int32_t secure, int32_t secure_only))
UNSUPPORTED(bmx_embedded_ble_secure_connection, (int32_t handle))
UNSUPPORTED(bmx_embedded_ble_security_state,
    (int32_t handle, int32_t *encrypted, int32_t *authenticated,
     int32_t *bonded, int32_t *key_size))
UNSUPPORTED(bmx_embedded_ble_provide_passkey,
    (int32_t handle, int32_t action, int32_t passkey))
UNSUPPORTED(bmx_embedded_ble_confirm_passkey, (int32_t handle, int32_t accept))
int32_t bmx_embedded_ble_bond_count(void) { return BLE_UNSUPPORTED; }
UNSUPPORTED(bmx_embedded_ble_bond,
    (int32_t index, int32_t *address_type, uint8_t *address))
UNSUPPORTED(bmx_embedded_ble_forget_bond,
    (int32_t address_type, const uint8_t *address))
UNSUPPORTED(bmx_embedded_ble_forget_all_bonds, (void))
static void start_gatt_query(int query, int parent, int attribute) {
    gatt_query = query;
    gatt_parent = parent;
    gatt_attribute = attribute;
}

int32_t bmx_embedded_ble_discover_services(int32_t handle) {
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    if (handle != connection_handle) return PICO_ERROR_INVALID_ARG;
    if (gatt_query != QUERY_NONE) return PICO_ERROR_RESOURCE_IN_USE;
    start_gatt_query(QUERY_SERVICES, 0, 0);
    cyw43_arch_lwip_begin();
    uint8_t result = gatt_client_discover_primary_services(gatt_packet_handler, handle);
    cyw43_arch_lwip_end();
    if (result) gatt_query = QUERY_NONE;
    return result;
}
int32_t bmx_embedded_ble_discover_characteristics(int32_t handle,
        int32_t start, int32_t end) {
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    if (handle != connection_handle || start <= 0 || end < start || end > 65535)
        return PICO_ERROR_INVALID_ARG;
    if (gatt_query != QUERY_NONE) return PICO_ERROR_RESOURCE_IN_USE;
    start_gatt_query(QUERY_CHARACTERISTICS, start, 0);
    gatt_client_service_t service = {0};
    service.start_group_handle = start;
    service.end_group_handle = end;
    cyw43_arch_lwip_begin();
    uint8_t result = gatt_client_discover_characteristics_for_service(
        gatt_packet_handler, handle, &service);
    cyw43_arch_lwip_end();
    if (result) gatt_query = QUERY_NONE;
    return result;
}
int32_t bmx_embedded_ble_discover_descriptors(int32_t handle,
        int32_t value, int32_t end) {
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    if (handle != connection_handle || value <= 0 || end <= value || end > 65535)
        return PICO_ERROR_INVALID_ARG;
    if (gatt_query != QUERY_NONE) return PICO_ERROR_RESOURCE_IN_USE;
    start_gatt_query(QUERY_DESCRIPTORS, value, 0);
    gatt_client_characteristic_t characteristic = {0};
    characteristic.value_handle = value;
    characteristic.end_handle = end;
    cyw43_arch_lwip_begin();
    uint8_t result = gatt_client_discover_characteristic_descriptors(
        gatt_packet_handler, handle, &characteristic);
    cyw43_arch_lwip_end();
    if (result) gatt_query = QUERY_NONE;
    return result;
}
int32_t bmx_embedded_ble_client_read(int32_t handle,
        int32_t attribute, uint32_t offset) {
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    if (handle != connection_handle || attribute <= 0 || attribute > 65535)
        return PICO_ERROR_INVALID_ARG;
    if (offset) return BLE_UNSUPPORTED;
    if (gatt_query != QUERY_NONE) return PICO_ERROR_RESOURCE_IN_USE;
    gatt_read_received = false;
    start_gatt_query(QUERY_READ, 0, attribute);
    cyw43_arch_lwip_begin();
    uint8_t result = gatt_client_read_value_of_characteristic_using_value_handle(
        gatt_packet_handler, handle, attribute);
    cyw43_arch_lwip_end();
    if (result) gatt_query = QUERY_NONE;
    return result;
}
int32_t bmx_embedded_ble_client_write(int32_t handle, int32_t attribute,
        const uint8_t *value, uint32_t length, int32_t response) {
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    if (handle != connection_handle || attribute <= 0 || attribute > 65535 ||
            length > sizeof(gatt_write_value) || (length && !value))
        return PICO_ERROR_INVALID_ARG;
    if (gatt_query != QUERY_NONE) return PICO_ERROR_RESOURCE_IN_USE;
    if (length) memcpy(gatt_write_value, value, length);
    if (response) start_gatt_query(QUERY_WRITE, 0, attribute);
    cyw43_arch_lwip_begin();
    uint8_t result = response ? gatt_client_write_value_of_characteristic(
        gatt_packet_handler, handle, attribute, length, gatt_write_value) :
        gatt_client_write_value_of_characteristic_without_response(
            handle, attribute, length, gatt_write_value);
    cyw43_arch_lwip_end();
    if (result == 0 && !response) {
        BLEEvent event = {.kind = 17, .connection_handle = handle,
            .attribute_id = attribute};
        queue_event(&event);
    }
    if (result && response) gatt_query = QUERY_NONE;
    return result;
}
int32_t bmx_embedded_ble_gatt_reset(void) {
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    if (initialized) return PICO_ERROR_RESOURCE_IN_USE;
    memset(services, 0, sizeof(services));
    memset(characteristics, 0, sizeof(characteristics));
    service_count = characteristic_count = 0;
    return PICO_OK;
}
int32_t bmx_embedded_ble_gatt_add_service(const uint8_t *uuid,
        uint32_t length, int32_t *service_id) {
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    if (initialized) return PICO_ERROR_RESOURCE_IN_USE;
    if (!service_id) return PICO_ERROR_INVALID_ARG;
    if (service_count == MAX_GATT_SERVICES) return PICO_ERROR_INSUFFICIENT_RESOURCES;
    BLEService *item = &services[service_count];
    int parsed = parse_uuid(uuid, length, item->uuid);
    if (!parsed) return PICO_ERROR_INVALID_ARG;
    item->uuid_length = parsed;
    *service_id = ++service_count;
    return PICO_OK;
}
int32_t bmx_embedded_ble_gatt_add_characteristic(int32_t service_id,
        const uint8_t *uuid, uint32_t uuid_length, uint32_t flags,
        const uint8_t *value, uint32_t value_length, uint32_t capacity,
        int32_t *characteristic_id) {
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    if (initialized) return PICO_ERROR_RESOURCE_IN_USE;
    if (!characteristic_id || service_id < 1 || service_id > service_count ||
            !capacity || capacity > 512 || value_length > capacity ||
            (value_length && !value)) return PICO_ERROR_INVALID_ARG;
    if (flags & ~31u) return BLE_UNSUPPORTED;
    if (characteristic_count == MAX_GATT_CHARACTERISTICS)
        return PICO_ERROR_INSUFFICIENT_RESOURCES;
    BLECharacteristic *item = &characteristics[characteristic_count];
    int parsed = parse_uuid(uuid, uuid_length, item->uuid);
    if (!parsed) return PICO_ERROR_INVALID_ARG;
    item->uuid_length = parsed;
    item->service_id = service_id;
    item->capacity = capacity;
    item->value_length = value_length;
    item->flags = flags;
    if (value_length) memcpy(item->value, value, value_length);
    *characteristic_id = ++characteristic_count;
    return PICO_OK;
}

int32_t bmx_embedded_ble_gatt_notify(int32_t characteristic_id,
        int32_t handle, int32_t indication) {
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    if (!initialized || !ready) return PICO_ERROR_INVALID_STATE;
    if (characteristic_id < 1 || characteristic_id > characteristic_count)
        return PICO_ERROR_INVALID_ARG;
    if (handle == -1) handle = connection_handle;
    if (handle != connection_handle) return PICO_ERROR_INVALID_ARG;
    BLECharacteristic *item = &characteristics[characteristic_id - 1];
    if (!item->value_handle ||
            (indication ? !(item->flags & 8u) : !(item->flags & 4u)))
        return PICO_ERROR_INVALID_STATE;
    if (!(item->subscriptions & (indication ? 2u : 1u)))
        return PICO_ERROR_PRECONDITION_NOT_MET;
    cyw43_arch_lwip_begin();
    uint8_t result = indication ? att_server_indicate(handle, item->value_handle,
        item->value, item->value_length) : att_server_notify(handle,
        item->value_handle, item->value, item->value_length);
    cyw43_arch_lwip_end();
    if (!result && !indication) {
        BLEEvent event = {.kind = 20, .connection_handle = handle,
            .attribute_id = characteristic_id};
        queue_event(&event);
    }
    return result;
}

int32_t bmx_embedded_ble_gatt_set_value(int32_t characteristic_id,
        const uint8_t *value, uint32_t length, int32_t notify_subscribers) {
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    if (characteristic_id < 1 || characteristic_id > characteristic_count ||
            (length && !value)) return PICO_ERROR_INVALID_ARG;
    BLECharacteristic *item = &characteristics[characteristic_id - 1];
    if (length > item->capacity) return PICO_ERROR_BUFFER_TOO_SMALL;
    if (initialized) cyw43_arch_lwip_begin();
    if (length) memcpy(item->value, value, length);
    item->value_length = length;
    if (initialized) cyw43_arch_lwip_end();
    if (notify_subscribers && connection_handle != HCI_CON_HANDLE_INVALID &&
            (item->subscriptions & 1u))
        return bmx_embedded_ble_gatt_notify(characteristic_id,
            connection_handle, 0);
    return PICO_OK;
}
int32_t bmx_embedded_ble_gatt_get_value(int32_t characteristic_id,
        uint8_t *value, uint32_t capacity, int32_t *value_length) {
    if (get_core_num() != 0) return PICO_ERROR_NOT_PERMITTED;
    if (characteristic_id < 1 || characteristic_id > characteristic_count ||
            !value_length) return PICO_ERROR_INVALID_ARG;
    BLECharacteristic *item = &characteristics[characteristic_id - 1];
    if (initialized) cyw43_arch_lwip_begin();
    *value_length = item->value_length;
    int32_t result = PICO_OK;
    if (value) {
        if (capacity < item->value_length) result = PICO_ERROR_BUFFER_TOO_SMALL;
        else if (item->value_length) memcpy(value, item->value, item->value_length);
    }
    if (initialized) cyw43_arch_lwip_end();
    return result;
}
