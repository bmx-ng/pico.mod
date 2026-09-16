#ifndef BLITZMAX_EMBEDDED_PLATFORM_H
#define BLITZMAX_EMBEDDED_PLATFORM_H

#include "pico/platform/sections.h"
#include "pico/stdlib.h"
#include <stdio.h>

/* Target contract consumed by the shared embedded runtime. These operations
   deliberately remain macros so the Pico implementation retains its existing
   code shape and does not add a call on allocation or exception paths. */
#define BMX_EMBEDDED_PLATFORM_CONTEXT_VALID() \
    (get_core_num() == 0 && __get_current_exception() == 0)
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
