#include "fixture_value.h"

#ifndef BMX_PICO_GRAPH_MODULE_VALUE
#error "User module compiler options were not applied"
#endif

int32_t bmx_pico_graph_native_value(int32_t value) {
    return value + BMX_PICO_GRAPH_MODULE_VALUE + BMX_PICO_GRAPH_HEADER_VALUE;
}
