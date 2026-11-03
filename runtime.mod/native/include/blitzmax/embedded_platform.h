#ifndef BLITZMAX_EMBEDDED_PLATFORM_H
#define BLITZMAX_EMBEDDED_PLATFORM_H

#include "pico/platform/sections.h"
#include "pico/stdlib.h"
#include <stdio.h>

/* Target contract consumed by the shared embedded runtime. These operations
   deliberately remain macros so the Pico implementation retains its existing
   code shape and does not add a call on allocation or exception paths. */
#if defined(BMX_EMBEDDED_MULTICORE) && BMX_EMBEDDED_MULTICORE
int32_t bmx_pico_managed_gc_context_valid(void);
int32_t bmx_pico_managed_gc_bind(uint32_t slot);
void bmx_pico_managed_gc_unbind(uint32_t slot);
#define BMX_EMBEDDED_PLATFORM_CONTEXT_VALID() bmx_pico_managed_gc_context_valid()
#define BMX_EMBEDDED_PLATFORM_GC_BIND(slot) bmx_pico_managed_gc_bind(slot)
#define BMX_EMBEDDED_PLATFORM_GC_UNBIND(slot) bmx_pico_managed_gc_unbind(slot)
#define BMX_EMBEDDED_PLATFORM_GC_WAIT() tight_loop_contents()
#else
#define BMX_EMBEDDED_PLATFORM_CONTEXT_VALID() \
    (get_core_num() == 0 && __get_current_exception() == 0)
#endif
#define BMX_EMBEDDED_PLATFORM_CONTEXT_INDEX() get_core_num()
#define BMX_EMBEDDED_PLATFORM_PANIC(message) do { \
    puts(message); \
    panic("BlitzMax runtime error"); \
} while (0)

#if BMX_EMBEDDED_ARENA_IN_PSRAM
#define BMX_EMBEDDED_ARENA_STORAGE(name) __uninitialized_psram(#name) name
#else
#define BMX_EMBEDDED_ARENA_STORAGE(name) name
#endif

#endif
