#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "blitzmax/pico_runtime.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
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
#include "pico/stdlib.h"
#include "pico/unique_id.h"

#ifndef BMX_PICO_ARENA_SIZE
#define BMX_PICO_ARENA_SIZE (16u * 1024u)
#endif

#ifndef BMX_PICO_ROOT_CAPACITY
#define BMX_PICO_ROOT_CAPACITY 64u
#endif

#ifndef BMX_PICO_ALARM_CAPACITY
#define BMX_PICO_ALARM_CAPACITY 8u
#endif

#define BMX_PICO_MEMORY_ALIGNMENT 16u

_Static_assert(BMX_PICO_ALARM_CAPACITY > 0u && BMX_PICO_ALARM_CAPACITY <= 255u,
    "Pico alarm handles reserve one byte for the slot number");

typedef union BMXPicoHeapBlock BMXPicoHeapBlock;

union BMXPicoHeapBlock {
    max_align_t alignment;
    struct {
        BMXPicoHeapBlock *previous;
        BMXPicoHeapBlock *next;
        BMXPicoHeapBlock *free_next;
        uint32_t capacity;
        uint32_t requested_size;
        uint32_t flags;
        uint32_t mark_epoch;
        uint32_t scan_epoch;
    } state;
};

_Static_assert(sizeof(BMXPicoHeapBlock) % BMX_PICO_MEMORY_ALIGNMENT == 0,
    "Pico heap headers must preserve manual-memory alignment");

#define BMX_PICO_HEAP_BLOCK_FREE 0x0001u
#define BMX_PICO_HEAP_BLOCK_OBJECT 0x0002u
#define BMX_PICO_HEAP_BLOCK_ARRAY 0x0004u
#define BMX_PICO_HEAP_BLOCK_FINALIZER_PENDING 0x0008u
#define BMX_PICO_HEAP_BLOCK_FINALIZED 0x0010u
#define BMX_PICO_HEAP_BLOCK_STRING 0x0020u
#define BMX_PICO_HEAP_BLOCK_RAW 0x0040u

static _Alignas(BMX_PICO_MEMORY_ALIGNMENT) uint8_t bmx_pico_arena[BMX_PICO_ARENA_SIZE];

static uint32_t bmx_pico_arena_offset;
static uint32_t bmx_pico_arena_high_water_mark;
static uint32_t bmx_pico_arena_allocations;
static uint32_t bmx_pico_arena_failures;
static uint32_t bmx_pico_array_failures;
static uint32_t bmx_pico_array_allocations;
static uint32_t bmx_pico_array_bytes;
static uint32_t bmx_pico_live_arrays;
static uint32_t bmx_pico_live_array_bytes;
static uint32_t bmx_pico_string_failures;
static uint32_t bmx_pico_string_allocations;
static uint32_t bmx_pico_string_bytes;
static uint32_t bmx_pico_live_strings;
static uint32_t bmx_pico_live_string_bytes;
static uint32_t bmx_pico_enum_failures;
static uint32_t bmx_pico_object_failures;
static uint32_t bmx_pico_object_allocations;
static uint32_t bmx_pico_object_bytes;
static uint32_t bmx_pico_live_objects;
static uint32_t bmx_pico_live_object_bytes;
static BMXPicoHeapBlock *bmx_pico_heap_first;
static BMXPicoHeapBlock *bmx_pico_heap_last;
static BMXPicoHeapBlock *bmx_pico_heap_free;
static void *bmx_pico_object_roots[BMX_PICO_ROOT_CAPACITY];
static uint32_t bmx_pico_object_root_total;
static uint32_t bmx_pico_reachability_epoch;
static uint32_t bmx_pico_reachable_objects;
static uint32_t bmx_pico_unreachable_objects;
static uint32_t bmx_pico_invalid_references;
static uint32_t bmx_pico_reachable_arrays;
static uint32_t bmx_pico_unreachable_arrays;
static uint32_t bmx_pico_reachable_strings;
static uint32_t bmx_pico_unreachable_strings;
static BMXPicoRootFrame *bmx_pico_root_frames;
static uint32_t bmx_pico_root_frame_total;
static uint32_t bmx_pico_root_slot_total;
static BMXPicoExceptionFrame *bmx_pico_exception_frames;
static BMXPicoException bmx_pico_exception_value;
static uint32_t bmx_pico_exception_depth_total;
static uint32_t bmx_pico_exception_throw_total;
static uint32_t bmx_pico_exception_catch_total;
static uint32_t bmx_pico_exception_max_depth_total;
static uint32_t bmx_pico_exception_unhandled_total;
static uint32_t bmx_pico_collection_total;
static uint32_t bmx_pico_automatic_collection_total;
static uint32_t bmx_pico_collection_active;
static uint32_t bmx_pico_last_reclaimed_objects;
static uint32_t bmx_pico_last_reclaimed_byte_total;
static uint32_t bmx_pico_last_reclaimed_arrays;
static uint32_t bmx_pico_last_reclaimed_array_byte_total;
static uint32_t bmx_pico_last_reclaimed_strings;
static uint32_t bmx_pico_last_reclaimed_string_byte_total;
static uint32_t bmx_pico_finalizer_pending_objects;
static uint32_t bmx_pico_finalizer_invocation_total;
static uint32_t bmx_pico_last_finalized_objects;

const BMXPicoString bmx_pico_empty_string = {0, NULL};
static const uint16_t bmx_pico_invalid_utf16_data[] = {
    'F','a','i','l','e','d',' ','t','o',' ','c','r','e','a','t','e',' ','U','T','F','3','2','.',
    ' ','I','n','v','a','l','i','d',' ','U','T','F','-','1','6',' ','s','u','r','r','o','g','a','t','e','.'
};
static const BMXPicoString bmx_pico_invalid_utf16_string = {
    (int32_t)(sizeof(bmx_pico_invalid_utf16_data) / sizeof(bmx_pico_invalid_utf16_data[0])),
    bmx_pico_invalid_utf16_data
};
BMXPicoArray bmx_pico_empty_array = {0, 0, BMX_PICO_ARRAY_ELEMENT_VALUE, 0, NULL, NULL};
BMXPicoObject bmx_pico_null_object = {NULL};

static uint32_t bmx_pico_align_size(uint32_t bytes) {
    const uint32_t alignment = BMX_PICO_MEMORY_ALIGNMENT;
    return (bytes + alignment - 1u) & ~(alignment - 1u);
}

static void bmx_pico_record_arena_failure(void) {
    __atomic_fetch_add(&bmx_pico_arena_failures, 1u, __ATOMIC_RELAXED);
}

static void bmx_pico_heap_free_remove(BMXPicoHeapBlock *block) {
    BMXPicoHeapBlock **link = &bmx_pico_heap_free;
    while (*link && *link != block) link = &(*link)->state.free_next;
    if (*link) *link = block->state.free_next;
    block->state.free_next = NULL;
}

static void bmx_pico_heap_free_add(BMXPicoHeapBlock *block) {
    block->state.free_next = bmx_pico_heap_free;
    bmx_pico_heap_free = block;
}

static BMXPicoHeapBlock *bmx_pico_heap_release(BMXPicoHeapBlock *block) {
    block->state.requested_size = 0;
    block->state.flags = BMX_PICO_HEAP_BLOCK_FREE;
    block->state.mark_epoch = 0;
    block->state.scan_epoch = 0;

    BMXPicoHeapBlock *next = block->state.next;
    if (next && (next->state.flags & BMX_PICO_HEAP_BLOCK_FREE)) {
        bmx_pico_heap_free_remove(next);
        block->state.capacity += (uint32_t)sizeof(BMXPicoHeapBlock) + next->state.capacity;
        block->state.next = next->state.next;
        if (block->state.next) block->state.next->state.previous = block;
        else bmx_pico_heap_last = block;
    }

    BMXPicoHeapBlock *previous = block->state.previous;
    if (previous && (previous->state.flags & BMX_PICO_HEAP_BLOCK_FREE)) {
        bmx_pico_heap_free_remove(previous);
        previous->state.capacity += (uint32_t)sizeof(BMXPicoHeapBlock) + block->state.capacity;
        previous->state.next = block->state.next;
        if (previous->state.next) previous->state.next->state.previous = previous;
        else bmx_pico_heap_last = previous;
        block = previous;
    }

    bmx_pico_heap_free_add(block);
    return block;
}

static void *bmx_pico_heap_allocate(uint32_t bytes, uint32_t flags) {
    if (!bytes) return NULL;
    const uint32_t aligned_bytes = bmx_pico_align_size(bytes);
    if (aligned_bytes < bytes) return NULL;

    BMXPicoHeapBlock *block = bmx_pico_heap_free;
    while (block && block->state.capacity < aligned_bytes) block = block->state.free_next;
    if (block) {
        bmx_pico_heap_free_remove(block);
        const uint32_t minimum_remainder = (uint32_t)sizeof(BMXPicoHeapBlock) + (uint32_t)_Alignof(max_align_t);
        if (block->state.capacity >= aligned_bytes + minimum_remainder) {
            BMXPicoHeapBlock *remainder = (BMXPicoHeapBlock *)((uint8_t *)(block + 1) + aligned_bytes);
            remainder->state.previous = block;
            remainder->state.next = block->state.next;
            if (remainder->state.next) remainder->state.next->state.previous = remainder;
            else bmx_pico_heap_last = remainder;
            remainder->state.free_next = NULL;
            remainder->state.capacity = block->state.capacity - aligned_bytes - (uint32_t)sizeof(BMXPicoHeapBlock);
            remainder->state.requested_size = 0;
            remainder->state.flags = BMX_PICO_HEAP_BLOCK_FREE;
            remainder->state.mark_epoch = 0;
            remainder->state.scan_epoch = 0;
            block->state.next = remainder;
            block->state.capacity = aligned_bytes;
            bmx_pico_heap_free_add(remainder);
        }
    } else {
        if (aligned_bytes > UINT32_MAX - sizeof(BMXPicoHeapBlock)) return NULL;
        const uint32_t total_size = (uint32_t)sizeof(BMXPicoHeapBlock) + aligned_bytes;
        if (total_size > BMX_PICO_ARENA_SIZE - bmx_pico_arena_offset) return NULL;
        block = (BMXPicoHeapBlock *)&bmx_pico_arena[bmx_pico_arena_offset];
        bmx_pico_arena_offset += total_size;
        block->state.previous = bmx_pico_heap_last;
        block->state.next = NULL;
        if (bmx_pico_heap_last) bmx_pico_heap_last->state.next = block;
        else bmx_pico_heap_first = block;
        bmx_pico_heap_last = block;
        block->state.capacity = aligned_bytes;
    }
    block->state.free_next = NULL;
    block->state.requested_size = bytes;
    block->state.flags = flags;
    block->state.mark_epoch = 0;
    block->state.scan_epoch = 0;
    bmx_pico_arena_allocations += 1u;
    if (bmx_pico_arena_offset > bmx_pico_arena_high_water_mark) bmx_pico_arena_high_water_mark = bmx_pico_arena_offset;
    return block + 1;
}

static void *bmx_pico_heap_allocate_with_collection(uint32_t bytes, uint32_t flags) {
    void *result = bmx_pico_heap_allocate(bytes, flags);
    if (!result && !bmx_pico_collection_active) {
        bmx_pico_automatic_collection_total += 1u;
        bmx_pico_collect_objects();
        if (bmx_pico_last_finalized_objects) {
            bmx_pico_automatic_collection_total += 1u;
            bmx_pico_collect_objects();
        }
        result = bmx_pico_heap_allocate(bytes, flags);
    }
    return result;
}

void *bmx_pico_arena_allocate(uint32_t bytes) {
    if (!bytes || get_core_num() != 0 || __get_current_exception() != 0) {
        bmx_pico_record_arena_failure();
        return NULL;
    }
    void *result = bmx_pico_heap_allocate(bytes, BMX_PICO_HEAP_BLOCK_RAW);
    if (!result) bmx_pico_record_arena_failure();
    return result;
}

uint32_t bmx_pico_arena_capacity(void) {
    return BMX_PICO_ARENA_SIZE;
}

uint32_t bmx_pico_arena_used(void) {
    return bmx_pico_arena_offset;
}

uint32_t bmx_pico_arena_remaining(void) {
    return BMX_PICO_ARENA_SIZE - bmx_pico_arena_offset;
}

uint32_t bmx_pico_arena_high_water(void) {
    return bmx_pico_arena_high_water_mark;
}

uint32_t bmx_pico_arena_allocation_count(void) {
    return bmx_pico_arena_allocations;
}

uint32_t bmx_pico_arena_failure_count(void) {
    return __atomic_load_n(&bmx_pico_arena_failures, __ATOMIC_RELAXED);
}

static void bmx_pico_record_array_failure(void) {
    __atomic_fetch_add(&bmx_pico_array_failures, 1u, __ATOMIC_RELAXED);
}

BMXPicoArray *bmx_pico_array_new_1d(int32_t length, uint32_t element_size, uint16_t element_kind, BMXPicoArrayInitializer initializer, const BMXPicoValueDescriptor *element_descriptor) {
    if (length < 0 || !element_size || element_kind > BMX_PICO_ARRAY_ELEMENT_OBJECT ||
        (initializer && element_kind != BMX_PICO_ARRAY_ELEMENT_VALUE) ||
        (element_descriptor && element_kind != BMX_PICO_ARRAY_ELEMENT_VALUE) ||
        (element_descriptor && element_descriptor->size != element_size) ||
        (element_kind != BMX_PICO_ARRAY_ELEMENT_VALUE && element_size != sizeof(void *))) {
        bmx_pico_record_array_failure();
        return &bmx_pico_empty_array;
    }
    if (!length) return &bmx_pico_empty_array;

    const uint32_t header_size = bmx_pico_align_size((uint32_t)sizeof(BMXPicoArray));
    const uint32_t count = (uint32_t)length;
    if (count > (UINT32_MAX - header_size) / element_size) {
        bmx_pico_record_array_failure();
        return &bmx_pico_empty_array;
    }

    const uint32_t total_size = header_size + count * element_size;
    if (get_core_num() != 0 || __get_current_exception() != 0) {
        bmx_pico_record_arena_failure();
        bmx_pico_record_array_failure();
        return &bmx_pico_empty_array;
    }
    BMXPicoArray *array = (BMXPicoArray *)bmx_pico_heap_allocate_with_collection(total_size, BMX_PICO_HEAP_BLOCK_ARRAY);
    if (!array) {
        bmx_pico_record_arena_failure();
        bmx_pico_record_array_failure();
        return &bmx_pico_empty_array;
    }
    array->length = length;
    array->element_size = element_size;
    array->element_kind = element_kind;
    array->reserved = 0;
    array->initializer = initializer;
    array->element_descriptor = element_descriptor;
    void *elements = (uint8_t *)array + header_size;
    if (element_kind == BMX_PICO_ARRAY_ELEMENT_OBJECT) {
        void **references = (void **)elements;
        for (uint32_t index = 0; index < count; ++index) references[index] = &bmx_pico_null_object;
    } else if (element_kind == BMX_PICO_ARRAY_ELEMENT_STRING) {
        const BMXPicoString **references = (const BMXPicoString **)elements;
        for (uint32_t index = 0; index < count; ++index) references[index] = &bmx_pico_empty_string;
    } else {
        memset(elements, 0, count * element_size);
        if (initializer) {
            BMXPicoArray *array_root = array;
            BMXPicoRootSlot slot = {(void *)&array_root, BMX_PICO_ROOT_ARRAY, NULL};
            BMXPicoRootFrame frame;
            bmx_pico_root_frame_enter(&frame, &slot, 1);
            for (uint32_t index = 0; index < count; ++index) initializer((uint8_t *)elements + index * element_size);
            bmx_pico_root_frame_leave(&frame);
        }
    }
    bmx_pico_array_allocations += 1u;
    bmx_pico_array_bytes += total_size;
    bmx_pico_live_arrays += 1u;
    bmx_pico_live_array_bytes += total_size;
    return array;
}

BMXPicoArray *bmx_pico_array_from_data(int32_t length, uint32_t element_size,
    uint16_t element_kind, BMXPicoArrayInitializer initializer,
    const BMXPicoValueDescriptor *element_descriptor, const void *data) {
    if (length > 0 && !data) {
        bmx_pico_record_array_failure();
        return &bmx_pico_empty_array;
    }
    BMXPicoArray *array = bmx_pico_array_new_1d(length, element_size, element_kind,
        initializer, element_descriptor);
    if (array != &bmx_pico_empty_array && length > 0) {
        memcpy(bmx_pico_array_data(array), data, (size_t)length * element_size);
    }
    return array;
}

BMXPicoArray *bmx_pico_array_concat(BMXPicoArray *left, BMXPicoArray *right) {
    if (!left || !right) {
        bmx_pico_record_array_failure();
        return &bmx_pico_empty_array;
    }
    if (!left->length && !right->length) return &bmx_pico_empty_array;

    BMXPicoArray *shape = left->length ? left : right;
    if ((left->length && (left->element_size != shape->element_size ||
            left->element_kind != shape->element_kind ||
            left->element_descriptor != shape->element_descriptor)) ||
        (right->length && (right->element_size != shape->element_size ||
            right->element_kind != shape->element_kind ||
            right->element_descriptor != shape->element_descriptor)) ||
        left->length > INT32_MAX - right->length) {
        bmx_pico_record_array_failure();
        return &bmx_pico_empty_array;
    }

    BMXPicoArray *left_root = left;
    BMXPicoArray *right_root = right;
    BMXPicoRootSlot slots[2] = {
        {(void *)&left_root, BMX_PICO_ROOT_ARRAY, NULL},
        {(void *)&right_root, BMX_PICO_ROOT_ARRAY, NULL}
    };
    BMXPicoRootFrame frame;
    bmx_pico_root_frame_enter(&frame, slots, 2);
    BMXPicoArray *result = bmx_pico_array_new_1d(left->length + right->length,
        shape->element_size, shape->element_kind, NULL, shape->element_descriptor);
    if (result != &bmx_pico_empty_array) {
        result->initializer = shape->initializer;
        uint8_t *destination = (uint8_t *)bmx_pico_array_data(result);
        if (left->length) memcpy(destination, bmx_pico_array_data(left),
            (size_t)left->length * shape->element_size);
        if (right->length) memcpy(destination + (size_t)left->length * shape->element_size,
            bmx_pico_array_data(right), (size_t)right->length * shape->element_size);
    }
    bmx_pico_root_frame_leave(&frame);
    return result;
}

BMXPicoArray *bmx_pico_array_slice(BMXPicoArray *array, int32_t begin, int32_t end,
    uint32_t element_size, uint16_t element_kind, BMXPicoArrayInitializer initializer,
    const BMXPicoValueDescriptor *element_descriptor) {
    if (!array || end <= begin) return &bmx_pico_empty_array;
    if (array->length && (array->element_size != element_size || array->element_kind != element_kind ||
            array->initializer != initializer || array->element_descriptor != element_descriptor)) {
        bmx_pico_record_array_failure();
        return &bmx_pico_empty_array;
    }
    int64_t length64 = (int64_t)end - begin;
    if (length64 > INT32_MAX) {
        bmx_pico_record_array_failure();
        return &bmx_pico_empty_array;
    }
    BMXPicoArray *array_root = array;
    BMXPicoRootSlot slot = {(void *)&array_root, BMX_PICO_ROOT_ARRAY, NULL};
    BMXPicoRootFrame frame;
    bmx_pico_root_frame_enter(&frame, &slot, 1);
    BMXPicoArray *result = bmx_pico_array_new_1d((int32_t)length64, element_size,
        element_kind, initializer, element_descriptor);
    if (result != &bmx_pico_empty_array) {
        int64_t copy_begin = begin;
        int64_t copy_end = end;
        if (copy_begin < 0) copy_begin = 0;
        if (copy_end > array->length) copy_end = array->length;
        if (copy_end > copy_begin) {
            uint32_t copied = (uint32_t)(copy_end - copy_begin);
            uint32_t destination = (uint32_t)(copy_begin - begin);
            memcpy((uint8_t *)bmx_pico_array_data(result) + (size_t)destination * element_size,
                (uint8_t *)bmx_pico_array_data(array) + (size_t)copy_begin * element_size,
                (size_t)copied * element_size);
        }
    }
    bmx_pico_root_frame_leave(&frame);
    return result;
}

void bbArrayCopy(BBARRAY src, int src_pos, BBARRAY dst, int dst_pos, int length) {
    if (!src || !dst || src_pos < 0 || dst_pos < 0 || length < 0 ||
        src_pos > src->length - length || dst_pos > dst->length - length ||
        src->element_size != dst->element_size || src->element_kind != dst->element_kind ||
        src->element_descriptor != dst->element_descriptor) {
        bmx_pico_record_array_failure();
        panic("BlitzMax Pico ArrayCopy range or element type mismatch");
    }
    if (!length) return;
    memmove((uint8_t *)bmx_pico_array_data(dst) + (uint32_t)dst_pos * dst->element_size,
        (uint8_t *)bmx_pico_array_data(src) + (uint32_t)src_pos * src->element_size,
        (size_t)length * src->element_size);
}

void *bmx_pico_array_element(BMXPicoArray *array, int32_t index, uint32_t element_size) {
    if (!array || index < 0 || index >= array->length || element_size != array->element_size) {
        bmx_pico_record_array_failure();
        panic("BlitzMax Pico Array index or element type mismatch");
    }
    const uint32_t header_size = bmx_pico_align_size((uint32_t)sizeof(BMXPicoArray));
    return (uint8_t *)array + header_size + (uint32_t)index * element_size;
}

void *bmx_pico_array_data(BMXPicoArray *array) {
    if (!array) return NULL;
    return (uint8_t *)array + bmx_pico_align_size((uint32_t)sizeof(BMXPicoArray));
}

static BMXPicoHeapBlock *bmx_pico_raw_block(void *memory) {
    if (!memory) return NULL;
    for (BMXPicoHeapBlock *block = bmx_pico_heap_first; block; block = block->state.next) {
        if (!(block->state.flags & BMX_PICO_HEAP_BLOCK_FREE) &&
            (block->state.flags & BMX_PICO_HEAP_BLOCK_RAW) &&
            (void *)(block + 1) == memory) return block;
    }
    return NULL;
}

void *bbMemAlloc(size_t size) {
    if (!size || size > UINT32_MAX || get_core_num() != 0 || __get_current_exception() != 0) {
        bmx_pico_record_arena_failure();
        return NULL;
    }
    void *result = bmx_pico_heap_allocate_with_collection((uint32_t)size, BMX_PICO_HEAP_BLOCK_RAW);
    if (!result) bmx_pico_record_arena_failure();
    return result;
}

void bbMemFree(void *memory) {
    if (!memory) return;
    if (get_core_num() != 0 || __get_current_exception() != 0) {
        bmx_pico_record_arena_failure();
        return;
    }
    BMXPicoHeapBlock *block = bmx_pico_raw_block(memory);
    if (!block) panic("BlitzMax Pico MemFree received an invalid pointer");
    bmx_pico_heap_release(block);
}

void *bbMemExtend(void *memory, size_t size, size_t new_size) {
    if (!memory) return bbMemAlloc(new_size);
    if (!new_size) {
        bbMemFree(memory);
        return NULL;
    }
    BMXPicoHeapBlock *block = bmx_pico_raw_block(memory);
    if (!block) panic("BlitzMax Pico MemExtend received an invalid pointer");
    void *extended = bbMemAlloc(new_size);
    if (!extended) return NULL;
    size_t copied = size;
    if (copied > block->state.requested_size) copied = block->state.requested_size;
    if (copied > new_size) copied = new_size;
    memcpy(extended, memory, copied);
    bbMemFree(memory);
    return extended;
}

void *bbMemAllocCollectable(size_t size) {
    (void)size;
    panic("BlitzMax Pico collectable raw memory is not supported");
    return NULL;
}

void bbMemFreeCollectable(void *memory) {
    (void)memory;
    panic("BlitzMax Pico collectable raw memory is not supported");
}

typedef struct BMXPicoIncbinEntry {
    struct BMXPicoIncbinEntry *next;
    const BMXPicoString *path;
    const void *data;
    int32_t size;
} BMXPicoIncbinEntry;

static BMXPicoIncbinEntry *bmx_pico_incbin_entries;

int32_t bbIncbinAdd(const BMXPicoString *path, const void *data, int32_t size) {
    if (!path || !data || size < 0) return 0;
    for (BMXPicoIncbinEntry *entry = bmx_pico_incbin_entries; entry; entry = entry->next) {
        if (bmx_pico_string_compare(entry->path, path) == 0) return 0;
    }
    BMXPicoIncbinEntry *entry = (BMXPicoIncbinEntry *)bbMemAlloc(sizeof(BMXPicoIncbinEntry));
    if (!entry) return 0;
    entry->path = path;
    entry->data = data;
    entry->size = size;
    entry->next = bmx_pico_incbin_entries;
    bmx_pico_incbin_entries = entry;
    return 0;
}

void *bbIncbinPtr(const BMXPicoString *path) {
    if (!path) return NULL;
    for (BMXPicoIncbinEntry *entry = bmx_pico_incbin_entries; entry; entry = entry->next) {
        if (bmx_pico_string_compare(entry->path, path) == 0) return (void *)entry->data;
    }
    return NULL;
}

int32_t bbIncbinLen(const BMXPicoString *path) {
    if (!path) return 0;
    for (BMXPicoIncbinEntry *entry = bmx_pico_incbin_entries; entry; entry = entry->next) {
        if (bmx_pico_string_compare(entry->path, path) == 0) return entry->size;
    }
    return 0;
}

void *bbMemExtendCollectable(void *memory, size_t size, size_t new_size) {
    (void)memory;
    (void)size;
    (void)new_size;
    panic("BlitzMax Pico collectable raw memory is not supported");
    return NULL;
}

void bbMemClear(void *memory, size_t size) {
    memset(memory, 0, size);
}

void bbMemCopy(void *destination, const void *source, size_t size) {
    memcpy(destination, source, size);
}

void bbMemMove(void *destination, const void *source, size_t size) {
    memmove(destination, source, size);
}

uint32_t bmx_pico_array_failure_count(void) {
    return __atomic_load_n(&bmx_pico_array_failures, __ATOMIC_RELAXED);
}

uint32_t bmx_pico_array_allocation_count(void) {
    return bmx_pico_array_allocations;
}

uint32_t bmx_pico_array_allocated_bytes(void) {
    return bmx_pico_array_bytes;
}

uint32_t bmx_pico_array_live_count(void) {
    return bmx_pico_live_arrays;
}

uint32_t bmx_pico_array_live_bytes(void) {
    return bmx_pico_live_array_bytes;
}

static void bmx_pico_record_object_failure(void) {
    __atomic_fetch_add(&bmx_pico_object_failures, 1u, __ATOMIC_RELAXED);
}

static int bmx_pico_value_field_valid(const BMXPicoValueField *field, uint32_t container_size) {
    if (!field || !field->count || field->offset >= container_size) return 0;
    if (field->kind < BMX_PICO_VALUE_OBJECT || field->kind > BMX_PICO_VALUE_STRUCT) return 0;
    if (field->kind == BMX_PICO_VALUE_STRUCT && !field->descriptor) return 0;
    if (field->kind != BMX_PICO_VALUE_STRUCT && field->descriptor) return 0;
    uint32_t referenced_size = field->kind == BMX_PICO_VALUE_STRUCT ?
        field->descriptor->size : (uint32_t)sizeof(void *);
    if (!referenced_size || referenced_size > container_size) return 0;
    if (field->count == 1u) return field->offset <= container_size - referenced_size;
    if (!field->stride || field->stride > UINT32_MAX / (field->count - 1u)) return 0;
    uint32_t span = (field->count - 1u) * field->stride;
    return field->offset <= UINT32_MAX - span && field->offset + span <= container_size - referenced_size;
}

static int bmx_pico_value_descriptor_valid(const BMXPicoValueDescriptor *descriptor) {
    if (!descriptor || !descriptor->name || !descriptor->size ||
        (descriptor->field_count && !descriptor->fields)) return 0;
    for (uint32_t index = 0; index < descriptor->field_count; ++index) {
        if (!bmx_pico_value_field_valid(&descriptor->fields[index], descriptor->size)) return 0;
    }
    return 1;
}

static int bmx_pico_type_descriptor_valid(const BMXPicoTypeDescriptor *type) {
    if (!type || !type->name || type->instance_size < sizeof(BMXPicoObject)) return 0;
    if (type->method_count && !type->methods) return 0;
    if (type->interface_count && !type->interfaces) return 0;
    if (type->reference_count && !type->reference_offsets) return 0;
    if (type->array_count && !type->array_offsets) return 0;
    if (type->string_count && !type->string_offsets) return 0;
    if (type->value_field_count && !type->value_fields) return 0;
    if (type->flags & ~(BMX_PICO_TYPE_FLAG_CUSTOM_TRACE | BMX_PICO_TYPE_FLAG_HAS_FINALIZER)) return 0;
    if ((type->flags & BMX_PICO_TYPE_FLAG_CUSTOM_TRACE) && !type->trace) return 0;
    if ((type->flags & BMX_PICO_TYPE_FLAG_HAS_FINALIZER) && !type->finalizer) return 0;
    for (uint32_t index = 0; index < type->reference_count; ++index) {
        uint32_t offset = type->reference_offsets[index];
        if (offset < sizeof(BMXPicoObject) || offset > type->instance_size - sizeof(void *) || offset % _Alignof(void *)) return 0;
    }
    for (uint32_t index = 0; index < type->array_count; ++index) {
        uint32_t offset = type->array_offsets[index];
        if (offset < sizeof(BMXPicoObject) || offset > type->instance_size - sizeof(void *) || offset % _Alignof(void *)) return 0;
    }
    for (uint32_t index = 0; index < type->string_count; ++index) {
        uint32_t offset = type->string_offsets[index];
        if (offset < sizeof(BMXPicoObject) || offset > type->instance_size - sizeof(void *) || offset % _Alignof(void *)) return 0;
    }
    for (uint32_t index = 0; index < type->value_field_count; ++index) {
        if (!bmx_pico_value_field_valid(&type->value_fields[index], type->instance_size)) return 0;
    }
    return 1;
}

static BMXPicoHeapBlock *bmx_pico_object_allocation(void *object) {
    for (BMXPicoHeapBlock *block = bmx_pico_heap_first; block; block = block->state.next) {
        if (!(block->state.flags & BMX_PICO_HEAP_BLOCK_FREE) && (block->state.flags & BMX_PICO_HEAP_BLOCK_OBJECT) && (void *)(block + 1) == object) return block;
    }
    return NULL;
}

static BMXPicoHeapBlock *bmx_pico_array_allocation(void *array) {
    for (BMXPicoHeapBlock *block = bmx_pico_heap_first; block; block = block->state.next) {
        if (!(block->state.flags & BMX_PICO_HEAP_BLOCK_FREE) && (block->state.flags & BMX_PICO_HEAP_BLOCK_ARRAY) && (void *)(block + 1) == array) return block;
    }
    return NULL;
}

static BMXPicoHeapBlock *bmx_pico_string_allocation(const void *string) {
    for (BMXPicoHeapBlock *block = bmx_pico_heap_first; block; block = block->state.next) {
        if (!(block->state.flags & BMX_PICO_HEAP_BLOCK_FREE) && (block->state.flags & BMX_PICO_HEAP_BLOCK_STRING) && (const void *)(block + 1) == string) return block;
    }
    return NULL;
}

const BMXPicoString *bmx_pico_stream_url_string(BMXPicoObject *value) {
    if (!value || value == &bmx_pico_null_object) return &bmx_pico_empty_string;
    if ((const void *)value == (const void *)&bmx_pico_empty_string || bmx_pico_string_allocation(value)) {
        return (const BMXPicoString *)value;
    }
    if (bmx_pico_object_allocation(value) || bmx_pico_array_allocation(value)) {
        return &bmx_pico_empty_string;
    }
    const BMXPicoString *text = (const BMXPicoString *)value;
    if (text->length < 0 || (text->length > 0 && !text->buf)) return &bmx_pico_empty_string;
    return text;
}

void *bmx_pico_object_allocate(const BMXPicoTypeDescriptor *type) {
    if (get_core_num() != 0 || __get_current_exception() != 0) {
        bmx_pico_record_arena_failure();
        bmx_pico_record_object_failure();
        return &bmx_pico_null_object;
    }
    if (!bmx_pico_type_descriptor_valid(type)) {
        bmx_pico_record_object_failure();
        return &bmx_pico_null_object;
    }
    BMXPicoObject *object = (BMXPicoObject *)bmx_pico_heap_allocate_with_collection(type->instance_size, BMX_PICO_HEAP_BLOCK_OBJECT);
    if (!object) {
        bmx_pico_record_arena_failure();
        bmx_pico_record_object_failure();
        return &bmx_pico_null_object;
    }
    memset(object, 0, type->instance_size);
    object->type = type;
    bmx_pico_object_allocations += 1u;
    bmx_pico_object_bytes += type->instance_size;
    bmx_pico_live_objects += 1u;
    bmx_pico_live_object_bytes += type->instance_size;
    return object;
}

static const uint32_t bmx_pico_closure_reference_offsets[] = {
    (uint32_t)offsetof(BMXPicoClosure, environment)
};

static const BMXPicoTypeDescriptor bmx_pico_closure_type = {
    .name = "Closure",
    .abi_name = "",
    .instance_size = (uint32_t)sizeof(BMXPicoClosure),
    .super = NULL,
    .methods = NULL,
    .method_count = 0,
    .interfaces = NULL,
    .interface_count = 0,
    .reference_offsets = bmx_pico_closure_reference_offsets,
    .reference_count = 1,
    .array_offsets = NULL,
    .array_count = 0,
    .string_offsets = NULL,
    .string_count = 0,
    .value_fields = NULL,
    .value_field_count = 0,
    .flags = 0,
    .trace = NULL,
    .finalizer = NULL,
    .compare = NULL,
    .hash_code = NULL,
    .equals = NULL
};

BMXPicoClosure *bmx_pico_closure_allocate(BMXPicoMethod invoke, BMXPicoObject *environment) {
    if (get_core_num() != 0 || __get_current_exception() != 0) {
        bmx_pico_record_arena_failure();
        bmx_pico_record_object_failure();
        return (BMXPicoClosure *)&bmx_pico_null_object;
    }
    if (!invoke) {
        bmx_pico_record_object_failure();
        return (BMXPicoClosure *)&bmx_pico_null_object;
    }
    BMXPicoObject *environment_root = environment ? environment : &bmx_pico_null_object;
    BMXPicoRootSlot slot = {(void *)&environment_root, BMX_PICO_ROOT_OBJECT, NULL};
    BMXPicoRootFrame frame;
    bmx_pico_root_frame_enter(&frame, &slot, 1);
    BMXPicoClosure *closure = (BMXPicoClosure *)bmx_pico_object_allocate(&bmx_pico_closure_type);
    if ((void *)closure != (void *)&bmx_pico_null_object) {
        closure->invoke = invoke;
        closure->environment = environment_root;
    }
    bmx_pico_root_frame_leave(&frame);
    return closure;
}

BMXPicoClosure *bmx_pico_closure_assert(void *closure) {
    BMXPicoClosure *value = (BMXPicoClosure *)bmx_pico_object_assert(closure);
    if (value->object.type != &bmx_pico_closure_type || !value->invoke) {
        bmx_pico_record_object_failure();
        panic("BlitzMax Pico invalid Closure invocation");
    }
    return value;
}

void *bmx_pico_object_assert(void *object) {
    if (!object || object == &bmx_pico_null_object || !bmx_pico_object_allocation(object) ||
        !bmx_pico_type_descriptor_valid(((BMXPicoObject *)object)->type)) {
        bmx_pico_record_object_failure();
        panic("BlitzMax Pico Null object access");
    }
    return object;
}

void *bmx_pico_object_null_failure(void) {
    bmx_pico_record_object_failure();
    panic("BlitzMax Pico Null object access");
    return &bmx_pico_null_object;
}

int32_t bmx_pico_object_compare(void *object, void *other) {
    BMXPicoObject *left = (BMXPicoObject *)bmx_pico_object_assert(object);
    if (left->type->compare) return left->type->compare(left, other);
    uintptr_t left_value = (uintptr_t)object;
    uintptr_t right_value = (uintptr_t)other;
    return (left_value > right_value) - (left_value < right_value);
}

static uint32_t bmx_pico_mix32(uint32_t value) {
    value ^= value >> 16;
    value *= 0x85ebca6bu;
    value ^= value >> 13;
    value *= 0xc2b2ae35u;
    value ^= value >> 16;
    return value;
}

uint32_t bmx_pico_object_hash_code(void *object) {
    BMXPicoObject *value = (BMXPicoObject *)bmx_pico_object_assert(object);
    if (value->type->hash_code) return value->type->hash_code(value);
    return bmx_pico_mix32((uint32_t)(uintptr_t)object);
}

int32_t bmx_pico_object_equals(void *object, void *other) {
    BMXPicoObject *left = (BMXPicoObject *)bmx_pico_object_assert(object);
    if (left->type->equals) return left->type->equals(left, other);
    return object == other;
}

void *bmx_pico_object_cast(void *object, const BMXPicoTypeDescriptor *target) {
    if (!object || object == &bmx_pico_null_object) return &bmx_pico_null_object;
    BMXPicoHeapBlock *block = bmx_pico_object_allocation(object);
    if (!block || !target || !bmx_pico_type_descriptor_valid(((BMXPicoObject *)object)->type)) {
        bmx_pico_record_object_failure();
        return &bmx_pico_null_object;
    }
    for (const BMXPicoTypeDescriptor *type = ((BMXPicoObject *)object)->type; type; type = type->super) {
        if (type == target) return object;
        if (type->abi_name && target->abi_name && type->abi_name[0] && target->abi_name[0] &&
                strcmp(type->abi_name, target->abi_name) == 0) return object;
    }
    return &bmx_pico_null_object;
}

const BMXPicoMethod *bmx_pico_type_methods(void *object, const BMXPicoTypeDescriptor *target, uint32_t method_count) {
    BMXPicoObject *value = (BMXPicoObject *)bmx_pico_object_assert(object);
    const BMXPicoTypeDescriptor *dynamic_type = value->type;
    int matched = target == NULL;
    for (const BMXPicoTypeDescriptor *type = dynamic_type; target && type; type = type->super) {
        if (type == target || (type->abi_name && target->abi_name && type->abi_name[0] && target->abi_name[0] &&
                strcmp(type->abi_name, target->abi_name) == 0)) {
            matched = 1;
            break;
        }
    }
    if (!matched || dynamic_type->method_count < method_count || (method_count && !dynamic_type->methods)) {
        panic("invalid BlitzMax Pico Type dispatch");
    }
    return dynamic_type->methods;
}

static const BMXPicoInterfaceEntry *bmx_pico_find_interface(void *object, const BMXPicoInterfaceDescriptor *target) {
    if (!object || object == &bmx_pico_null_object || !target) return NULL;
    BMXPicoHeapBlock *block = bmx_pico_object_allocation(object);
    if (!block || !bmx_pico_type_descriptor_valid(((BMXPicoObject *)object)->type)) return NULL;
    for (const BMXPicoTypeDescriptor *type = ((BMXPicoObject *)object)->type; type; type = type->super) {
        for (uint32_t index = 0; index < type->interface_count; ++index) {
            const BMXPicoInterfaceDescriptor *candidate = type->interfaces[index].interface_type;
            if (candidate == target) return &type->interfaces[index];
            if (candidate && candidate->abi_name && target->abi_name &&
                    candidate->abi_name[0] && target->abi_name[0] &&
                    strcmp(candidate->abi_name, target->abi_name) == 0) {
                return &type->interfaces[index];
            }
        }
    }
    return NULL;
}

void *bmx_pico_interface_cast(void *object, const BMXPicoInterfaceDescriptor *target) {
    return bmx_pico_find_interface(object, target) ? object : &bmx_pico_null_object;
}

const BMXPicoMethod *bmx_pico_interface_methods(void *object, const BMXPicoInterfaceDescriptor *target, uint32_t method_count) {
    BMXPicoObject *value = (BMXPicoObject *)bmx_pico_object_assert(object);
    const BMXPicoInterfaceEntry *entry = bmx_pico_find_interface(value, target);
    if (!entry || entry->method_count < method_count || (method_count && !entry->methods)) {
        bmx_pico_record_object_failure();
        panic("BlitzMax Pico interface dispatch failure");
    }
    return entry->methods;
}

uint32_t bmx_pico_object_failure_count(void) {
    return __atomic_load_n(&bmx_pico_object_failures, __ATOMIC_RELAXED);
}

uint32_t bmx_pico_object_allocation_count(void) {
    return bmx_pico_object_allocations;
}

uint32_t bmx_pico_object_allocated_bytes(void) {
    return bmx_pico_object_bytes;
}

uint32_t bmx_pico_object_live_count(void) {
    return bmx_pico_live_objects;
}

uint32_t bmx_pico_object_live_bytes(void) {
    return bmx_pico_live_object_bytes;
}

uint32_t bmx_pico_object_root_retain(BMXPicoObject *object) {
    if (!object || object == &bmx_pico_null_object) return 0;
    if (get_core_num() != 0 || __get_current_exception() != 0 || !bmx_pico_object_allocation(object)) {
        bmx_pico_record_object_failure();
        return 0;
    }
    for (uint32_t index = 0; index < BMX_PICO_ROOT_CAPACITY; ++index) {
        if (!bmx_pico_object_roots[index]) {
            bmx_pico_object_roots[index] = object;
            bmx_pico_object_root_total += 1u;
            return index + 1u;
        }
    }
    bmx_pico_record_object_failure();
    return 0;
}

void bmx_pico_object_root_release(uint32_t token) {
    if (!token) return;
    if (get_core_num() != 0 || __get_current_exception() != 0 || token > BMX_PICO_ROOT_CAPACITY || !bmx_pico_object_roots[token - 1u]) {
        bmx_pico_record_object_failure();
        return;
    }
    bmx_pico_object_roots[token - 1u] = NULL;
    bmx_pico_object_root_total -= 1u;
}

uint32_t bmx_pico_object_root_count(void) {
    return bmx_pico_object_root_total;
}

void bmx_pico_root_frame_enter(BMXPicoRootFrame *frame, BMXPicoRootSlot *slots, uint16_t slot_count) {
    if (!frame || (slot_count && !slots) || get_core_num() != 0 || __get_current_exception() != 0) {
        bmx_pico_record_object_failure();
        return;
    }
    frame->previous = bmx_pico_root_frames;
    frame->slots = slots;
    frame->slot_count = slot_count;
    bmx_pico_root_frames = frame;
    bmx_pico_root_frame_total += 1u;
    bmx_pico_root_slot_total += slot_count;
}

void bmx_pico_root_frame_leave(BMXPicoRootFrame *frame) {
    if (!frame || frame != bmx_pico_root_frames || get_core_num() != 0 || __get_current_exception() != 0) {
        bmx_pico_record_object_failure();
        return;
    }
    bmx_pico_root_frames = frame->previous;
    bmx_pico_root_frame_total -= 1u;
    bmx_pico_root_slot_total -= frame->slot_count;
    frame->previous = NULL;
    frame->slots = NULL;
    frame->slot_count = 0;
}

uint32_t bmx_pico_root_frame_count(void) {
    return bmx_pico_root_frame_total;
}

uint32_t bmx_pico_root_slot_count(void) {
    return bmx_pico_root_slot_total;
}

void bmx_pico_exception_enter(BMXPicoExceptionFrame *frame) {
    if (!frame || get_core_num() != 0 || __get_current_exception() != 0) {
        bmx_pico_record_object_failure();
        panic("BlitzMax Pico exception handlers require core 0 thread context");
    }
    frame->previous = bmx_pico_exception_frames;
    frame->root_snapshot = bmx_pico_root_frames;
    frame->root_frame_count = bmx_pico_root_frame_total;
    frame->root_slot_count = bmx_pico_root_slot_total;
    bmx_pico_exception_frames = frame;
    bmx_pico_exception_depth_total += 1u;
    if (bmx_pico_exception_depth_total > bmx_pico_exception_max_depth_total) {
        bmx_pico_exception_max_depth_total = bmx_pico_exception_depth_total;
    }
}

void bmx_pico_exception_leave(void) {
    BMXPicoExceptionFrame *frame = bmx_pico_exception_frames;
    if (!frame || get_core_num() != 0 || __get_current_exception() != 0) {
        bmx_pico_record_object_failure();
        panic("BlitzMax Pico exception frame imbalance");
    }
    bmx_pico_exception_frames = frame->previous;
    bmx_pico_exception_depth_total -= 1u;
    frame->previous = NULL;
}

BMXPicoException bmx_pico_exception_object(BMXPicoObject *value) {
    BMXPicoException exception = {value, BMX_PICO_EXCEPTION_OBJECT, 0};
    return exception;
}

BMXPicoException bmx_pico_exception_array(BMXPicoArray *value) {
    BMXPicoException exception = {value, BMX_PICO_EXCEPTION_ARRAY, 0};
    return exception;
}

BMXPicoException bmx_pico_exception_string(const BMXPicoString *value) {
    BMXPicoException exception = {(void *)value, BMX_PICO_EXCEPTION_STRING, 0};
    return exception;
}

BMXPicoException bmx_pico_exception_catch(void) {
    BMXPicoException exception = bmx_pico_exception_value;
    bmx_pico_exception_value.value = NULL;
    bmx_pico_exception_value.kind = BMX_PICO_EXCEPTION_NONE;
    bmx_pico_exception_catch_total += 1u;
    return exception;
}

void bmx_pico_exception_throw(BMXPicoException exception) {
    bmx_pico_exception_throw_total += 1u;
    int valid = exception.value && get_core_num() == 0 && __get_current_exception() == 0;
    if (valid && exception.kind == BMX_PICO_EXCEPTION_OBJECT) {
        valid = exception.value != &bmx_pico_null_object && bmx_pico_object_allocation(exception.value) != NULL;
    } else if (valid && exception.kind == BMX_PICO_EXCEPTION_ARRAY) {
        valid = exception.value == &bmx_pico_empty_array || bmx_pico_array_allocation(exception.value) != NULL;
    } else if (valid && exception.kind == BMX_PICO_EXCEPTION_STRING) {
        /* Compiler literals live in flash and are trusted, while dynamic
           Strings are validated when the collector marks the rooted carrier. */
    } else {
        valid = 0;
    }
    if (!valid) {
        bmx_pico_record_object_failure();
        panic("BlitzMax Pico invalid exception value or context");
    }
    BMXPicoExceptionFrame *frame = bmx_pico_exception_frames;
    if (!frame) {
        bmx_pico_exception_unhandled_total += 1u;
        panic("Unhandled BlitzMax Pico exception");
    }
    bmx_pico_exception_value = exception;
    bmx_pico_root_frames = frame->root_snapshot;
    bmx_pico_root_frame_total = frame->root_frame_count;
    bmx_pico_root_slot_total = frame->root_slot_count;
    bmx_pico_exception_frames = frame->previous;
    bmx_pico_exception_depth_total -= 1u;
    longjmp(frame->buffer, 1);
}

uint32_t bmx_pico_exception_depth(void) { return bmx_pico_exception_depth_total; }
uint32_t bmx_pico_exception_throw_count(void) { return bmx_pico_exception_throw_total; }
uint32_t bmx_pico_exception_catch_count(void) { return bmx_pico_exception_catch_total; }
uint32_t bmx_pico_exception_max_depth(void) { return bmx_pico_exception_max_depth_total; }
uint32_t bmx_pico_exception_unhandled_count(void) { return bmx_pico_exception_unhandled_total; }

typedef struct BMXPicoReachabilityContext {
    uint32_t epoch;
    uint32_t reachable;
    uint32_t reachable_arrays;
    uint32_t reachable_strings;
    uint32_t invalid;
} BMXPicoReachabilityContext;

static void bmx_pico_mark_reference(void *reference, void *context_value) {
    if (!reference || reference == &bmx_pico_null_object) return;
    BMXPicoReachabilityContext *context = (BMXPicoReachabilityContext *)context_value;
    BMXPicoHeapBlock *block = bmx_pico_object_allocation(reference);
    if (!block || !bmx_pico_type_descriptor_valid(((BMXPicoObject *)reference)->type)) {
        context->invalid += 1u;
        return;
    }
    if (block->state.mark_epoch != context->epoch) {
        block->state.mark_epoch = context->epoch;
        context->reachable += 1u;
    }
}

static void bmx_pico_mark_array(void *reference, void *context_value) {
    if (!reference || reference == &bmx_pico_empty_array) return;
    BMXPicoReachabilityContext *context = (BMXPicoReachabilityContext *)context_value;
    BMXPicoHeapBlock *block = bmx_pico_array_allocation(reference);
    BMXPicoArray *array = (BMXPicoArray *)reference;
    if (!block || array->element_kind > BMX_PICO_ARRAY_ELEMENT_OBJECT ||
        (array->element_kind != BMX_PICO_ARRAY_ELEMENT_VALUE && array->element_size != sizeof(void *))) {
        context->invalid += 1u;
        return;
    }
    if (block->state.mark_epoch != context->epoch) {
        block->state.mark_epoch = context->epoch;
        context->reachable_arrays += 1u;
    }
}

static void bmx_pico_mark_string(const void *reference, void *context_value) {
    if (!reference || reference == &bmx_pico_empty_string) return;
    BMXPicoHeapBlock *block = bmx_pico_string_allocation(reference);
    /* Compiler-emitted literals live in flash rather than the managed heap and
       are permanent. Only heap-backed Strings participate in marking. */
    if (!block) return;
    BMXPicoReachabilityContext *context = (BMXPicoReachabilityContext *)context_value;
    if (block->state.mark_epoch != context->epoch) {
        block->state.mark_epoch = context->epoch;
        context->reachable_strings += 1u;
    }
}

static void bmx_pico_mark_value(const void *value, const BMXPicoValueDescriptor *descriptor, BMXPicoReachabilityContext *context) {
    if (!value || !bmx_pico_value_descriptor_valid(descriptor)) {
        context->invalid += 1u;
        return;
    }
    for (uint32_t field_index = 0; field_index < descriptor->field_count; ++field_index) {
        const BMXPicoValueField *field = &descriptor->fields[field_index];
        for (uint32_t item = 0; item < field->count; ++item) {
            const uint8_t *address = (const uint8_t *)value + field->offset + (uint32_t)item * field->stride;
            if (field->kind == BMX_PICO_VALUE_OBJECT) bmx_pico_mark_reference(*(void *const *)address, context);
            else if (field->kind == BMX_PICO_VALUE_ARRAY) bmx_pico_mark_array(*(void *const *)address, context);
            else if (field->kind == BMX_PICO_VALUE_STRING) bmx_pico_mark_string(*(const void *const *)address, context);
            else if (field->kind == BMX_PICO_VALUE_STRUCT && field->descriptor) bmx_pico_mark_value(address, field->descriptor, context);
            else context->invalid += 1u;
        }
    }
}

uint32_t bmx_pico_reachability_audit(void) {
    if (get_core_num() != 0 || __get_current_exception() != 0) {
        bmx_pico_record_object_failure();
        return 0;
    }
    bmx_pico_reachability_epoch += 1u;
    if (!bmx_pico_reachability_epoch) {
        for (BMXPicoHeapBlock *block = bmx_pico_heap_first; block; block = block->state.next) {
            block->state.mark_epoch = 0;
            block->state.scan_epoch = 0;
        }
        bmx_pico_reachability_epoch = 1u;
    }

    BMXPicoReachabilityContext context = {bmx_pico_reachability_epoch, 0, 0, 0, 0};
    for (uint32_t index = 0; index < BMX_PICO_ROOT_CAPACITY; ++index) {
        if (bmx_pico_object_roots[index]) bmx_pico_mark_reference(bmx_pico_object_roots[index], &context);
    }
    for (BMXPicoRootFrame *frame = bmx_pico_root_frames; frame; frame = frame->previous) {
        for (uint16_t index = 0; index < frame->slot_count; ++index) {
            BMXPicoRootSlot *slot = &frame->slots[index];
            if (!slot->address) continue;
            if (slot->kind == BMX_PICO_ROOT_OBJECT) bmx_pico_mark_reference(*(void **)slot->address, &context);
            else if (slot->kind == BMX_PICO_ROOT_ARRAY) bmx_pico_mark_array(*(void **)slot->address, &context);
            else if (slot->kind == BMX_PICO_ROOT_STRING) bmx_pico_mark_string(*(void **)slot->address, &context);
            else if (slot->kind == BMX_PICO_ROOT_STRUCT) bmx_pico_mark_value(slot->address, slot->descriptor, &context);
            else if (slot->kind == BMX_PICO_ROOT_EXCEPTION) {
                BMXPicoException *exception = (BMXPicoException *)slot->address;
                if (exception->kind == BMX_PICO_EXCEPTION_OBJECT) bmx_pico_mark_reference(exception->value, &context);
                else if (exception->kind == BMX_PICO_EXCEPTION_ARRAY) bmx_pico_mark_array(exception->value, &context);
                else if (exception->kind == BMX_PICO_EXCEPTION_STRING) bmx_pico_mark_string(exception->value, &context);
                else if (exception->kind != BMX_PICO_EXCEPTION_NONE) context.invalid += 1u;
            }
            else context.invalid += 1u;
        }
    }

    int pending;
    do {
        pending = 0;
        for (BMXPicoHeapBlock *block = bmx_pico_heap_first; block; block = block->state.next) {
            if ((block->state.flags & BMX_PICO_HEAP_BLOCK_FREE) || !(block->state.flags & BMX_PICO_HEAP_BLOCK_OBJECT)) continue;
            if (block->state.mark_epoch != context.epoch || block->state.scan_epoch == context.epoch) continue;
            block->state.scan_epoch = context.epoch;
            BMXPicoObject *object = (BMXPicoObject *)(block + 1);
            const BMXPicoTypeDescriptor *type = object->type;
            for (uint32_t index = 0; index < type->reference_count; ++index) {
                void *reference = *(void **)((uint8_t *)object + type->reference_offsets[index]);
                bmx_pico_mark_reference(reference, &context);
            }
            for (uint32_t index = 0; index < type->array_count; ++index) {
                void *reference = *(void **)((uint8_t *)object + type->array_offsets[index]);
                bmx_pico_mark_array(reference, &context);
            }
            for (uint32_t index = 0; index < type->string_count; ++index) {
                const void *reference = *(const void **)((uint8_t *)object + type->string_offsets[index]);
                bmx_pico_mark_string(reference, &context);
            }
            for (uint32_t index = 0; index < type->value_field_count; ++index) {
                const BMXPicoValueField *field = &type->value_fields[index];
                for (uint32_t item = 0; item < field->count; ++item) {
                    const uint8_t *address = (const uint8_t *)object + field->offset + (uint32_t)item * field->stride;
                    bmx_pico_mark_value(address, field->descriptor, &context);
                }
            }
            if (type->flags & BMX_PICO_TYPE_FLAG_CUSTOM_TRACE) type->trace(object, bmx_pico_mark_reference, &context);
            pending = 1;
        }
        for (BMXPicoHeapBlock *block = bmx_pico_heap_first; block; block = block->state.next) {
            if ((block->state.flags & BMX_PICO_HEAP_BLOCK_FREE) || !(block->state.flags & BMX_PICO_HEAP_BLOCK_ARRAY)) continue;
            if (block->state.mark_epoch != context.epoch || block->state.scan_epoch == context.epoch) continue;
            block->state.scan_epoch = context.epoch;
            BMXPicoArray *array = (BMXPicoArray *)(block + 1);
            if (array->element_kind == BMX_PICO_ARRAY_ELEMENT_OBJECT) {
                const uint32_t header_size = bmx_pico_align_size((uint32_t)sizeof(BMXPicoArray));
                void **elements = (void **)((uint8_t *)array + header_size);
                for (int32_t index = 0; index < array->length; ++index) bmx_pico_mark_reference(elements[index], &context);
            } else if (array->element_kind == BMX_PICO_ARRAY_ELEMENT_STRING) {
                const uint32_t header_size = bmx_pico_align_size((uint32_t)sizeof(BMXPicoArray));
                const void **elements = (const void **)((uint8_t *)array + header_size);
                for (int32_t index = 0; index < array->length; ++index) bmx_pico_mark_string(elements[index], &context);
            } else if (array->element_descriptor) {
                const uint32_t header_size = bmx_pico_align_size((uint32_t)sizeof(BMXPicoArray));
                const uint8_t *elements = (const uint8_t *)array + header_size;
                for (int32_t index = 0; index < array->length; ++index) {
                    bmx_pico_mark_value(elements + (uint32_t)index * array->element_size, array->element_descriptor, &context);
                }
            }
            pending = 1;
        }
    } while (pending);

    bmx_pico_reachable_objects = context.reachable;
    bmx_pico_unreachable_objects = bmx_pico_live_objects - context.reachable;
    bmx_pico_invalid_references = context.invalid;
    bmx_pico_reachable_arrays = context.reachable_arrays;
    bmx_pico_unreachable_arrays = bmx_pico_live_arrays - context.reachable_arrays;
    bmx_pico_reachable_strings = context.reachable_strings;
    bmx_pico_unreachable_strings = bmx_pico_live_strings - context.reachable_strings;
    return context.reachable;
}

uint32_t bmx_pico_reachable_object_count(void) {
    return bmx_pico_reachable_objects;
}

uint32_t bmx_pico_unreachable_object_count(void) {
    return bmx_pico_unreachable_objects;
}

uint32_t bmx_pico_invalid_reference_count(void) {
    return bmx_pico_invalid_references;
}

uint32_t bmx_pico_reachable_array_count(void) {
    return bmx_pico_reachable_arrays;
}

uint32_t bmx_pico_unreachable_array_count(void) {
    return bmx_pico_unreachable_arrays;
}

uint32_t bmx_pico_reachable_string_count(void) {
    return bmx_pico_reachable_strings;
}

uint32_t bmx_pico_unreachable_string_count(void) {
    return bmx_pico_unreachable_strings;
}

uint32_t bmx_pico_collect_objects(void) {
    if (get_core_num() != 0 || __get_current_exception() != 0 || bmx_pico_collection_active) {
        bmx_pico_record_object_failure();
        return 0;
    }

    bmx_pico_collection_active = 1u;
    bmx_pico_collection_total += 1u;
    bmx_pico_last_reclaimed_objects = 0;
    bmx_pico_last_reclaimed_byte_total = 0;
    bmx_pico_last_reclaimed_arrays = 0;
    bmx_pico_last_reclaimed_array_byte_total = 0;
    bmx_pico_last_reclaimed_strings = 0;
    bmx_pico_last_reclaimed_string_byte_total = 0;
    bmx_pico_finalizer_pending_objects = 0;
    bmx_pico_last_finalized_objects = 0;
    bmx_pico_reachability_audit();
    if (bmx_pico_invalid_references) {
        bmx_pico_collection_active = 0;
        bmx_pico_record_object_failure();
        return 0;
    }

    /* Queue every newly unreachable finalizable Object before invoking any
       user code. Heap order is intentionally the only ordering guarantee. */
    for (BMXPicoHeapBlock *block = bmx_pico_heap_first; block; block = block->state.next) {
        if ((block->state.flags & BMX_PICO_HEAP_BLOCK_FREE) || !(block->state.flags & BMX_PICO_HEAP_BLOCK_OBJECT)) continue;
        if (block->state.mark_epoch == bmx_pico_reachability_epoch) continue;
        const BMXPicoObject *object = (const BMXPicoObject *)(block + 1);
        if ((object->type->flags & BMX_PICO_TYPE_FLAG_HAS_FINALIZER) && !(block->state.flags & BMX_PICO_HEAP_BLOCK_FINALIZED)) {
            block->state.flags |= BMX_PICO_HEAP_BLOCK_FINALIZER_PENDING;
            bmx_pico_finalizer_pending_objects += 1u;
        }
    }
    if (bmx_pico_finalizer_pending_objects) {
        for (BMXPicoHeapBlock *block = bmx_pico_heap_first; block; block = block->state.next) {
            if (!(block->state.flags & BMX_PICO_HEAP_BLOCK_FINALIZER_PENDING)) continue;
            BMXPicoObject *object = (BMXPicoObject *)(block + 1);
            block->state.flags &= ~BMX_PICO_HEAP_BLOCK_FINALIZER_PENDING;
            block->state.flags |= BMX_PICO_HEAP_BLOCK_FINALIZED;
            bmx_pico_finalizer_pending_objects -= 1u;
            bmx_pico_finalizer_invocation_total += 1u;
            bmx_pico_last_finalized_objects += 1u;
            object->type->finalizer(object);
        }
        /* Do not sweep during a finalizer cycle. A later collection starts a
           fresh reachability epoch, observes resurrection and field changes,
           and can reclaim only Objects whose finalizer has already run. */
        bmx_pico_collection_active = 0;
        return 0;
    }

    BMXPicoHeapBlock *block = bmx_pico_heap_first;
    while (block) {
        if (!(block->state.flags & BMX_PICO_HEAP_BLOCK_FREE) &&
            block->state.mark_epoch != bmx_pico_reachability_epoch &&
            (block->state.flags & (BMX_PICO_HEAP_BLOCK_OBJECT | BMX_PICO_HEAP_BLOCK_ARRAY | BMX_PICO_HEAP_BLOCK_STRING))) {
            const uint32_t reclaimed_bytes = block->state.requested_size;
            if (block->state.flags & BMX_PICO_HEAP_BLOCK_OBJECT) {
                bmx_pico_last_reclaimed_objects += 1u;
                bmx_pico_last_reclaimed_byte_total += reclaimed_bytes;
                bmx_pico_live_objects -= 1u;
                bmx_pico_live_object_bytes -= reclaimed_bytes;
            } else if (block->state.flags & BMX_PICO_HEAP_BLOCK_ARRAY) {
                bmx_pico_last_reclaimed_arrays += 1u;
                bmx_pico_last_reclaimed_array_byte_total += reclaimed_bytes;
                bmx_pico_live_arrays -= 1u;
                bmx_pico_live_array_bytes -= reclaimed_bytes;
            } else {
                bmx_pico_last_reclaimed_strings += 1u;
                bmx_pico_last_reclaimed_string_byte_total += reclaimed_bytes;
                bmx_pico_live_strings -= 1u;
                bmx_pico_live_string_bytes -= reclaimed_bytes;
            }
            block = bmx_pico_heap_release(block)->state.next;
        } else {
            block = block->state.next;
        }
    }

    bmx_pico_reachable_objects = bmx_pico_live_objects;
    bmx_pico_unreachable_objects = 0;
    bmx_pico_reachable_arrays = bmx_pico_live_arrays;
    bmx_pico_unreachable_arrays = 0;
    bmx_pico_reachable_strings = bmx_pico_live_strings;
    bmx_pico_unreachable_strings = 0;
    bmx_pico_collection_active = 0;
    return bmx_pico_last_reclaimed_objects;
}

uint32_t bmx_pico_collection_count(void) {
    return bmx_pico_collection_total;
}

uint32_t bmx_pico_automatic_collection_count(void) {
    return bmx_pico_automatic_collection_total;
}

uint32_t bmx_pico_last_reclaimed_object_count(void) {
    return bmx_pico_last_reclaimed_objects;
}

uint32_t bmx_pico_last_reclaimed_bytes(void) {
    return bmx_pico_last_reclaimed_byte_total;
}

uint32_t bmx_pico_last_reclaimed_array_count(void) {
    return bmx_pico_last_reclaimed_arrays;
}

uint32_t bmx_pico_last_reclaimed_array_bytes(void) {
    return bmx_pico_last_reclaimed_array_byte_total;
}

uint32_t bmx_pico_last_reclaimed_string_count(void) {
    return bmx_pico_last_reclaimed_strings;
}

uint32_t bmx_pico_last_reclaimed_string_bytes(void) {
    return bmx_pico_last_reclaimed_string_byte_total;
}

uint32_t bmx_pico_finalizer_pending_count(void) {
    return bmx_pico_finalizer_pending_objects;
}

uint32_t bmx_pico_finalizer_invocation_count(void) {
    return bmx_pico_finalizer_invocation_total;
}

uint32_t bmx_pico_last_finalized_object_count(void) {
    return bmx_pico_last_finalized_objects;
}

uint32_t bmx_pico_heap_reusable_bytes(void) {
    uint32_t bytes = 0;
    for (BMXPicoHeapBlock *block = bmx_pico_heap_free; block; block = block->state.free_next) bytes += block->state.capacity;
    return bytes;
}

uint32_t bmx_pico_heap_largest_free_block(void) {
    uint32_t largest = 0;
    for (BMXPicoHeapBlock *block = bmx_pico_heap_free; block; block = block->state.free_next) {
        if (block->state.capacity > largest) largest = block->state.capacity;
    }
    return largest;
}

static void bmx_pico_record_string_failure(void) {
    __atomic_fetch_add(&bmx_pico_string_failures, 1u, __ATOMIC_RELAXED);
}

static BMXPicoString *bmx_pico_string_new(int32_t length) {
    if (length <= 0) return (BMXPicoString *)&bmx_pico_empty_string;
    const uint32_t header_size = bmx_pico_align_size((uint32_t)sizeof(BMXPicoString));
    const uint32_t count = (uint32_t)length;
    if (count > (UINT32_MAX - header_size) / sizeof(uint16_t) || get_core_num() != 0 || __get_current_exception() != 0) {
        bmx_pico_record_arena_failure();
        bmx_pico_record_string_failure();
        return (BMXPicoString *)&bmx_pico_empty_string;
    }
    const uint32_t total_size = header_size + count * (uint32_t)sizeof(uint16_t);
    BMXPicoString *text = (BMXPicoString *)bmx_pico_heap_allocate_with_collection(total_size, BMX_PICO_HEAP_BLOCK_STRING);
    if (!text) {
        bmx_pico_record_arena_failure();
        bmx_pico_record_string_failure();
        return (BMXPicoString *)&bmx_pico_empty_string;
    }
    text->length = length;
    text->buf = (const uint16_t *)((uint8_t *)text + header_size);
    bmx_pico_string_allocations += 1u;
    bmx_pico_string_bytes += total_size;
    bmx_pico_live_strings += 1u;
    bmx_pico_live_string_bytes += total_size;
    return text;
}

BMXPicoString *bmx_pico_string_allocate(int32_t length) {
    return bmx_pico_string_new(length);
}

uint32_t bmx_pico_string_failure_count(void) {
    return __atomic_load_n(&bmx_pico_string_failures, __ATOMIC_RELAXED);
}

uint32_t bmx_pico_string_allocation_count(void) {
    return bmx_pico_string_allocations;
}

uint32_t bmx_pico_string_allocated_bytes(void) {
    return bmx_pico_string_bytes;
}

uint32_t bmx_pico_string_live_count(void) {
    return bmx_pico_live_strings;
}

uint32_t bmx_pico_string_live_bytes(void) {
    return bmx_pico_live_string_bytes;
}

int32_t bmx_pico_string_compare(const BMXPicoString *left, const BMXPicoString *right) {
    int32_t common = left->length < right->length ? left->length : right->length;
    for (int32_t index = 0; index < common; ++index) {
        if (left->buf[index] < right->buf[index]) return -1;
        if (left->buf[index] > right->buf[index]) return 1;
    }
    return (left->length > right->length) - (left->length < right->length);
}

int32_t bmx_pico_string_equals(const BMXPicoString *left, const BMXPicoString *right) {
    return bmx_pico_string_compare(left, right) == 0;
}

uint32_t bmx_pico_string_hash(const BMXPicoString *text) {
    uint32_t hash = 2166136261u;
    for (int32_t index = 0; index < text->length; ++index) {
        hash ^= text->buf[index];
        hash *= 16777619u;
    }
    return hash;
}

static BMXPicoStringCaseTransform bmx_pico_unicode_lower;
static BMXPicoStringCaseTransform bmx_pico_unicode_upper;
static BMXPicoCharacterCaseFold bmx_pico_unicode_fold;

static uint16_t bmx_pico_ascii_case_fold(uint16_t character) {
    return character >= (uint16_t)'A' && character <= (uint16_t)'Z' ?
        character + ((uint16_t)'a' - (uint16_t)'A') : character;
}

void bmx_pico_string_install_unicode_case(BMXPicoStringCaseTransform lower,
    BMXPicoStringCaseTransform upper, BMXPicoCharacterCaseFold fold) {
    bmx_pico_unicode_lower = lower;
    bmx_pico_unicode_upper = upper;
    bmx_pico_unicode_fold = fold;
}

int32_t bmx_pico_string_compare_case(const BMXPicoString *left, const BMXPicoString *right,
    int32_t case_sensitive) {
    if (case_sensitive) return bmx_pico_string_compare(left, right);
    int32_t common = left->length < right->length ? left->length : right->length;
    for (int32_t index = 0; index < common; ++index) {
        uint16_t left_character = left->buf[index];
        uint16_t right_character = right->buf[index];
        if (left_character == right_character) continue;
        left_character = bmx_pico_unicode_fold ? bmx_pico_unicode_fold(left_character) :
            bmx_pico_ascii_case_fold(left_character);
        right_character = bmx_pico_unicode_fold ? bmx_pico_unicode_fold(right_character) :
            bmx_pico_ascii_case_fold(right_character);
        if (left_character != right_character) return (int32_t)left_character - right_character;
    }
    return left->length - right->length;
}

int32_t bmx_pico_string_equals_case(const BMXPicoString *left, const BMXPicoString *right,
    int32_t case_sensitive) {
    if (left->length != right->length) return 0;
    return bmx_pico_string_compare_case(left, right, case_sensitive) == 0;
}

uint32_t bmx_pico_string_hash_case(const BMXPicoString *text, int32_t case_sensitive) {
    if (case_sensitive) return bmx_pico_string_hash(text);
    uint32_t hash = 2166136261u;
    for (int32_t index = 0; index < text->length; ++index) {
        uint16_t character = bmx_pico_unicode_fold ? bmx_pico_unicode_fold(text->buf[index]) :
            bmx_pico_ascii_case_fold(text->buf[index]);
        hash ^= character;
        hash *= 16777619u;
    }
    return hash;
}

const BMXPicoString *bmx_pico_string_to_string(const BMXPicoString *text) {
    return text;
}

int32_t bmx_pico_string_find(const BMXPicoString *text, const BMXPicoString *substring, int32_t start) {
    if (!text || !substring) return -1;
    if (start < 0) start = 0;
    if (substring->length == 0) return start <= text->length ? start : -1;
    if (start > text->length - substring->length) return -1;
    for (int32_t index = start; index <= text->length - substring->length; ++index) {
        if (!memcmp(text->buf + index, substring->buf, (size_t)substring->length * sizeof(uint16_t))) return index;
    }
    return -1;
}

int32_t bmx_pico_string_find_last(const BMXPicoString *text, const BMXPicoString *substring, int32_t start) {
    if (!text || !substring || start < 0) return -1;
    int32_t index = text->length - start;
    if (index > text->length - substring->length) index = text->length - substring->length;
    while (index >= 0) {
        if (!memcmp(text->buf + index, substring->buf, (size_t)substring->length * sizeof(uint16_t))) return index;
        --index;
    }
    return -1;
}

const BMXPicoString *bmx_pico_string_trim(const BMXPicoString *text) {
    if (!text || !text->length) return &bmx_pico_empty_string;
    int32_t begin = 0;
    int32_t end = text->length;
    while (begin < end && text->buf[begin] <= (uint16_t)' ') ++begin;
    if (begin == end) return &bmx_pico_empty_string;
    while (text->buf[end - 1] <= (uint16_t)' ') --end;
    if (begin == 0 && end == text->length) return text;
    BMXPicoString *result = bmx_pico_string_new(end - begin);
    if (result != &bmx_pico_empty_string) {
        memcpy((void *)(uintptr_t)result->buf, text->buf + begin, (size_t)(end - begin) * sizeof(uint16_t));
    }
    return result;
}

const BMXPicoString *bmx_pico_string_replace(const BMXPicoString *text, const BMXPicoString *substring, const BMXPicoString *replacement) {
    if (!text || !substring || !replacement || !substring->length) return text ? text : &bmx_pico_empty_string;
    int32_t index = 0;
    int32_t match_count = 0;
    while ((index = bmx_pico_string_find(text, substring, index)) != -1) {
        index += substring->length;
        ++match_count;
    }
    if (!match_count) return text;
    int64_t result_length = (int64_t)text->length + (int64_t)(replacement->length - substring->length) * match_count;
    if (result_length < 0 || result_length > INT32_MAX) {
        bmx_pico_record_string_failure();
        return &bmx_pico_empty_string;
    }
    BMXPicoString *result = bmx_pico_string_new((int32_t)result_length);
    if (result == &bmx_pico_empty_string) return result;
    uint16_t *output = (uint16_t *)(uintptr_t)result->buf;
    int32_t source = 0;
    int32_t destination = 0;
    int32_t match;
    while ((match = bmx_pico_string_find(text, substring, source)) != -1) {
        int32_t prefix_length = match - source;
        if (prefix_length) {
            memcpy(output + destination, text->buf + source, (size_t)prefix_length * sizeof(uint16_t));
            destination += prefix_length;
        }
        if (replacement->length) {
            memcpy(output + destination, replacement->buf, (size_t)replacement->length * sizeof(uint16_t));
            destination += replacement->length;
        }
        source = match + substring->length;
    }
    if (source < text->length) {
        memcpy(output + destination, text->buf + source, (size_t)(text->length - source) * sizeof(uint16_t));
    }
    return result;
}

const BMXPicoString *bmx_pico_string_to_lower(const BMXPicoString *text) {
    if (bmx_pico_unicode_lower) return bmx_pico_unicode_lower(text);
    if (!text || !text->length) return &bmx_pico_empty_string;
    int changed = 0;
    for (int32_t index = 0; index < text->length; ++index) {
        if (text->buf[index] >= (uint16_t)'A' && text->buf[index] <= (uint16_t)'Z') {
            changed = 1;
            break;
        }
    }
    if (!changed) return text;
    BMXPicoString *result = bmx_pico_string_new(text->length);
    if (result == &bmx_pico_empty_string) return result;
    uint16_t *buffer = (uint16_t *)(uintptr_t)result->buf;
    for (int32_t index = 0; index < text->length; ++index) {
        uint16_t character = text->buf[index];
        if (character >= (uint16_t)'A' && character <= (uint16_t)'Z') character += (uint16_t)'a' - (uint16_t)'A';
        buffer[index] = character;
    }
    return result;
}

const BMXPicoString *bmx_pico_string_to_upper(const BMXPicoString *text) {
    if (bmx_pico_unicode_upper) return bmx_pico_unicode_upper(text);
    if (!text || !text->length) return &bmx_pico_empty_string;
    int changed = 0;
    for (int32_t index = 0; index < text->length; ++index) {
        if (text->buf[index] >= (uint16_t)'a' && text->buf[index] <= (uint16_t)'z') {
            changed = 1;
            break;
        }
    }
    if (!changed) return text;
    BMXPicoString *result = bmx_pico_string_new(text->length);
    if (result == &bmx_pico_empty_string) return result;
    uint16_t *buffer = (uint16_t *)(uintptr_t)result->buf;
    for (int32_t index = 0; index < text->length; ++index) {
        uint16_t character = text->buf[index];
        if (character >= (uint16_t)'a' && character <= (uint16_t)'z') character -= (uint16_t)'a' - (uint16_t)'A';
        buffer[index] = character;
    }
    return result;
}

int32_t bmx_pico_string_starts_with(const BMXPicoString *text, const BMXPicoString *substring) {
    if (!text || !substring || text->length < substring->length) return 0;
    return !memcmp(text->buf, substring->buf, (size_t)substring->length * sizeof(uint16_t));
}

int32_t bmx_pico_string_ends_with(const BMXPicoString *text, const BMXPicoString *substring) {
    if (!text || !substring || text->length < substring->length) return 0;
    return !memcmp(text->buf + text->length - substring->length, substring->buf,
        (size_t)substring->length * sizeof(uint16_t));
}

int32_t bmx_pico_string_contains(const BMXPicoString *text, const BMXPicoString *substring) {
    return bmx_pico_string_find(text, substring, 0) != -1;
}

const BMXPicoString *bmx_pico_string_replicate(const BMXPicoString *text, int32_t count) {
    if (!text || !text->length || count <= 0) return &bmx_pico_empty_string;
    if (text->length > INT32_MAX / count) {
        bmx_pico_record_string_failure();
        return &bmx_pico_empty_string;
    }
    BMXPicoString *result = bmx_pico_string_new(text->length * count);
    if (result == &bmx_pico_empty_string) return result;
    uint16_t *output = (uint16_t *)(uintptr_t)result->buf;
    for (int32_t index = 0; index < count; ++index) {
        memcpy(output + index * text->length, text->buf, (size_t)text->length * sizeof(uint16_t));
    }
    return result;
}

BMXPicoArray *bmx_pico_string_split(const BMXPicoString *text, const BMXPicoString *separator) {
    if (!text) text = &bmx_pico_empty_string;
    if (!separator) separator = &bmx_pico_empty_string;
    int32_t part_count = 0;
    if (separator->length) {
        part_count = 1;
        int32_t index = 0;
        while ((index = bmx_pico_string_find(text, separator, index)) != -1) {
            if (part_count == INT32_MAX) {
                bmx_pico_record_array_failure();
                return &bmx_pico_empty_array;
            }
            ++part_count;
            index += separator->length;
        }
    } else {
        int32_t index = 0;
        while (index < text->length) {
            while (index < text->length && text->buf[index] < 33u) ++index;
            if (index == text->length) break;
            ++part_count;
            while (index < text->length && text->buf[index] > 32u) ++index;
        }
        if (!part_count) return &bmx_pico_empty_array;
    }

    BMXPicoArray *parts = bmx_pico_array_new_1d(part_count, (uint32_t)sizeof(const BMXPicoString *),
        BMX_PICO_ARRAY_ELEMENT_STRING, NULL, NULL);
    if (parts == &bmx_pico_empty_array) return parts;
    BMXPicoArray *parts_root = parts;
    BMXPicoRootSlot slot = {(void *)&parts_root, BMX_PICO_ROOT_ARRAY, NULL};
    BMXPicoRootFrame frame;
    bmx_pico_root_frame_enter(&frame, &slot, 1);
    const BMXPicoString **output = (const BMXPicoString **)bmx_pico_array_data(parts);

    if (separator->length) {
        int32_t begin = 0;
        for (int32_t part = 0; part < part_count; ++part) {
            int32_t end = bmx_pico_string_find(text, separator, begin);
            if (end == -1) end = text->length;
            output[part] = bmx_pico_string_slice(text, begin, end);
            begin = end + separator->length;
        }
    } else {
        int32_t index = 0;
        for (int32_t part = 0; part < part_count; ++part) {
            while (text->buf[index] < 33u) ++index;
            int32_t begin = index;
            while (index < text->length && text->buf[index] > 32u) ++index;
            output[part] = bmx_pico_string_slice(text, begin, index);
        }
    }
    bmx_pico_root_frame_leave(&frame);
    return parts;
}

const BMXPicoString *bmx_pico_string_join(const BMXPicoString *separator, BMXPicoArray *parts) {
    if (!separator) separator = &bmx_pico_empty_string;
    if (!parts || parts == &bmx_pico_empty_array || !parts->length) return &bmx_pico_empty_string;
    if (parts->element_kind != BMX_PICO_ARRAY_ELEMENT_STRING || parts->element_size != sizeof(const BMXPicoString *)) {
        bmx_pico_record_array_failure();
        return &bmx_pico_empty_string;
    }
    const BMXPicoString *const *values = (const BMXPicoString *const *)bmx_pico_array_data(parts);
    int64_t length = (int64_t)(parts->length - 1) * separator->length;
    for (int32_t index = 0; index < parts->length; ++index) {
        const BMXPicoString *part = values[index] ? values[index] : &bmx_pico_empty_string;
        length += part->length;
        if (length > INT32_MAX) {
            bmx_pico_record_string_failure();
            return &bmx_pico_empty_string;
        }
    }
    BMXPicoString *result = bmx_pico_string_new((int32_t)length);
    if (result == &bmx_pico_empty_string) return result;
    uint16_t *output = (uint16_t *)(uintptr_t)result->buf;
    for (int32_t index = 0; index < parts->length; ++index) {
        if (index && separator->length) {
            memcpy(output, separator->buf, (size_t)separator->length * sizeof(uint16_t));
            output += separator->length;
        }
        const BMXPicoString *part = values[index] ? values[index] : &bmx_pico_empty_string;
        if (part->length) {
            memcpy(output, part->buf, (size_t)part->length * sizeof(uint16_t));
            output += part->length;
        }
    }
    return result;
}

const BMXPicoString *bmx_pico_string_from_bytes(const uint8_t *bytes, int32_t count) {
    if (!bytes || count <= 0) return &bmx_pico_empty_string;
    BMXPicoString *result = bmx_pico_string_new(count);
    if (result == &bmx_pico_empty_string) return result;
    uint16_t *buffer = (uint16_t *)(uintptr_t)result->buf;
    for (int32_t index = 0; index < count; ++index) buffer[index] = bytes[index];
    return result;
}

const BMXPicoString *bmx_pico_string_from_shorts(const uint16_t *characters, int32_t count) {
    if (!characters || count <= 0) return &bmx_pico_empty_string;
    BMXPicoString *result = bmx_pico_string_new(count);
    if (result != &bmx_pico_empty_string) {
        memcpy((void *)(uintptr_t)result->buf, characters, (size_t)count * sizeof(uint16_t));
    }
    return result;
}

const BMXPicoString *bmx_pico_string_from_c_string(const uint8_t *bytes) {
    if (!bytes) return &bmx_pico_empty_string;
    size_t count = strlen((const char *)bytes);
    if (count > INT32_MAX) {
        bmx_pico_record_string_failure();
        return &bmx_pico_empty_string;
    }
    return bmx_pico_string_from_bytes(bytes, (int32_t)count);
}

const BMXPicoString *bmx_pico_string_from_w_string(const uint16_t *characters) {
    if (!characters) return &bmx_pico_empty_string;
    size_t count = 0;
    while (characters[count]) {
        if (count == INT32_MAX) {
            bmx_pico_record_string_failure();
            return &bmx_pico_empty_string;
        }
        ++count;
    }
    return bmx_pico_string_from_shorts(characters, (int32_t)count);
}

const BMXPicoString *bmx_pico_string_from_ascii(const char *bytes, int32_t count) {
    return bmx_pico_string_from_bytes((const uint8_t *)bytes, count);
}

const BMXPicoString *bmx_pico_string_from_utf8_string(const uint8_t *bytes) {
    if (!bytes) return &bmx_pico_empty_string;
    size_t count = strlen((const char *)bytes);
    if (count > INT32_MAX) {
        bmx_pico_record_string_failure();
        return &bmx_pico_empty_string;
    }
    return bmx_pico_string_from_utf8_bytes(bytes, (int32_t)count);
}

static uint32_t bmx_pico_decode_utf8(const uint8_t *bytes, int32_t count, int32_t *index) {
    uint32_t first = bytes[(*index)++];
    if (first < 0x80u) return first;
    int needed = first >= 0xf0u ? 3 : first >= 0xe0u ? 2 : first >= 0xc2u ? 1 : 0;
    if (!needed || *index + needed > count) return 0xfffdu;
    uint32_t value = first & (needed == 1 ? 0x1fu : needed == 2 ? 0x0fu : 0x07u);
    int32_t continuation = *index;
    for (int offset = 0; offset < needed; ++offset) {
        uint32_t next = bytes[continuation + offset];
        if ((next & 0xc0u) != 0x80u) return 0xfffdu;
        value = (value << 6) | (next & 0x3fu);
    }
    if ((needed == 2 && value < 0x800u) || (needed == 3 && value < 0x10000u) ||
        (value >= 0xd800u && value <= 0xdfffu) || value > 0x10ffffu) return 0xfffdu;
    *index += needed;
    return value;
}

const BMXPicoString *bmx_pico_string_from_utf8_bytes(const uint8_t *bytes, int32_t count) {
    if (!bytes || count <= 0) return &bmx_pico_empty_string;
    int32_t units = 0;
    for (int32_t index = 0; index < count;) {
        uint32_t code_point = bmx_pico_decode_utf8(bytes, count, &index);
        units += code_point > 0xffffu ? 2 : 1;
    }
    BMXPicoString *result = bmx_pico_string_new(units);
    if (result == &bmx_pico_empty_string) return result;
    uint16_t *buffer = (uint16_t *)(uintptr_t)result->buf;
    int32_t output = 0;
    for (int32_t index = 0; index < count;) {
        uint32_t code_point = bmx_pico_decode_utf8(bytes, count, &index);
        if (code_point <= 0xffffu) {
            buffer[output++] = (uint16_t)code_point;
        } else {
            code_point -= 0x10000u;
            buffer[output++] = (uint16_t)(0xd800u | (code_point >> 10));
            buffer[output++] = (uint16_t)(0xdc00u | (code_point & 0x3ffu));
        }
    }
    return result;
}

uint8_t *bmx_pico_string_to_c_string(const BMXPicoString *text) {
    int32_t count = text ? text->length : 0;
    uint8_t *result = (uint8_t *)bbMemAlloc((size_t)count + 1u);
    if (!result) return NULL;
    for (int32_t index = 0; index < count; ++index) result[index] = (uint8_t)text->buf[index];
    result[count] = 0;
    return result;
}

uint16_t *bmx_pico_string_to_w_string_buffer(const BMXPicoString *text, uint16_t *buffer, size_t *length) {
    if (!buffer || !length) return buffer;
    size_t capacity = *length;
    if (!capacity) return buffer;
    size_t copied = text && text->length > 0 ? (size_t)text->length : 0u;
    if (copied >= capacity) copied = capacity - 1u;
    if (copied) memcpy(buffer, text->buf, copied * sizeof(uint16_t));
    buffer[copied] = 0;
    *length = copied;
    return buffer;
}

uint16_t *bmx_pico_string_to_w_string(const BMXPicoString *text) {
    size_t capacity = (size_t)(text && text->length > 0 ? text->length : 0) + 1u;
    if (capacity > SIZE_MAX / sizeof(uint16_t)) return NULL;
    uint16_t *result = (uint16_t *)bbMemAlloc(capacity * sizeof(uint16_t));
    if (!result) return NULL;
    return bmx_pico_string_to_w_string_buffer(text, result, &capacity);
}

static size_t bmx_pico_utf8_length(const BMXPicoString *text) {
    size_t length = 0;
    for (int32_t index = 0; text && index < text->length; ++index) {
        uint32_t code_point = text->buf[index];
        if (code_point >= 0xd800u && code_point <= 0xdbffu && index + 1 < text->length) {
            uint32_t low = text->buf[index + 1];
            if (low >= 0xdc00u && low <= 0xdfffu) {
                code_point = 0x10000u + ((code_point - 0xd800u) << 10) + (low - 0xdc00u);
                ++index;
            } else code_point = 0xfffdu;
        } else if (code_point >= 0xdc00u && code_point <= 0xdfffu) code_point = 0xfffdu;
        length += code_point <= 0x7fu ? 1u : code_point <= 0x7ffu ? 2u : code_point <= 0xffffu ? 3u : 4u;
    }
    return length;
}

static uint8_t *bmx_pico_encode_utf8(uint8_t *output, uint32_t code_point) {
    if (code_point <= 0x7fu) {
        *output++ = (uint8_t)code_point;
    } else if (code_point <= 0x7ffu) {
        *output++ = (uint8_t)(0xc0u | (code_point >> 6));
        *output++ = (uint8_t)(0x80u | (code_point & 0x3fu));
    } else if (code_point <= 0xffffu) {
        *output++ = (uint8_t)(0xe0u | (code_point >> 12));
        *output++ = (uint8_t)(0x80u | ((code_point >> 6) & 0x3fu));
        *output++ = (uint8_t)(0x80u | (code_point & 0x3fu));
    } else {
        *output++ = (uint8_t)(0xf0u | (code_point >> 18));
        *output++ = (uint8_t)(0x80u | ((code_point >> 12) & 0x3fu));
        *output++ = (uint8_t)(0x80u | ((code_point >> 6) & 0x3fu));
        *output++ = (uint8_t)(0x80u | (code_point & 0x3fu));
    }
    return output;
}

uint8_t *bmx_pico_string_to_utf8_string_buffer(const BMXPicoString *text, uint8_t *buffer, size_t *length) {
    if (!buffer || !length) return buffer;
    size_t capacity = *length;
    if (!capacity) return buffer;
    uint8_t *output = buffer;
    size_t remaining = capacity - 1u;
    for (int32_t index = 0; text && index < text->length; ++index) {
        uint32_t code_point = text->buf[index];
        if (code_point >= 0xd800u && code_point <= 0xdbffu && index + 1 < text->length) {
            uint32_t low = text->buf[index + 1];
            if (low >= 0xdc00u && low <= 0xdfffu) {
                code_point = 0x10000u + ((code_point - 0xd800u) << 10) + (low - 0xdc00u);
                ++index;
            } else code_point = 0xfffdu;
        } else if (code_point >= 0xdc00u && code_point <= 0xdfffu) code_point = 0xfffdu;
        size_t encoded = code_point <= 0x7fu ? 1u : code_point <= 0x7ffu ? 2u : code_point <= 0xffffu ? 3u : 4u;
        if (encoded > remaining) break;
        output = bmx_pico_encode_utf8(output, code_point);
        remaining -= encoded;
    }
    *output = 0;
    *length = (size_t)(output - buffer);
    return buffer;
}

uint8_t *bmx_pico_string_to_utf8_string_len(const BMXPicoString *text, size_t *length) {
    size_t required = bmx_pico_utf8_length(text);
    uint8_t *result = (uint8_t *)bbMemAlloc(required + 1u);
    if (!result) {
        if (length) *length = 0;
        return NULL;
    }
    uint8_t *output = result;
    for (int32_t index = 0; text && index < text->length; ++index) {
        uint32_t code_point = text->buf[index];
        if (code_point >= 0xd800u && code_point <= 0xdbffu && index + 1 < text->length) {
            uint32_t low = text->buf[index + 1];
            if (low >= 0xdc00u && low <= 0xdfffu) {
                code_point = 0x10000u + ((code_point - 0xd800u) << 10) + (low - 0xdc00u);
                ++index;
            } else code_point = 0xfffdu;
        } else if (code_point >= 0xdc00u && code_point <= 0xdfffu) code_point = 0xfffdu;
        output = bmx_pico_encode_utf8(output, code_point);
    }
    *output = 0;
    if (length) *length = required;
    return result;
}

uint8_t *bmx_pico_string_to_utf8_string(const BMXPicoString *text) {
    return bmx_pico_string_to_utf8_string_len(text, NULL);
}

uint32_t *bmx_pico_string_to_utf32_string(const BMXPicoString *text) {
    size_t capacity = (size_t)(text && text->length > 0 ? text->length : 0) + 1u;
    if (capacity > SIZE_MAX / sizeof(uint32_t)) return NULL;
    uint32_t *result = (uint32_t *)bbMemAlloc(capacity * sizeof(uint32_t));
    if (!result) return NULL;
    uint32_t *output = result;
    for (int32_t index = 0; text && index < text->length; ++index) {
        uint32_t code_point = text->buf[index];
        if (code_point >= 0xd800u && code_point <= 0xdbffu) {
            if (index + 1 >= text->length || text->buf[index + 1] < 0xdc00u || text->buf[index + 1] > 0xdfffu) {
                bbMemFree(result);
                bmx_pico_exception_throw(bmx_pico_exception_string(&bmx_pico_invalid_utf16_string));
            }
            code_point = 0x10000u + ((code_point - 0xd800u) << 10) + (text->buf[++index] - 0xdc00u);
        } else if (code_point >= 0xdc00u && code_point <= 0xdfffu) {
            bbMemFree(result);
            bmx_pico_exception_throw(bmx_pico_exception_string(&bmx_pico_invalid_utf16_string));
        }
        *output++ = code_point;
    }
    *output = 0;
    return result;
}

const BMXPicoString *bmx_pico_string_from_utf32_bytes(const uint32_t *characters, size_t count) {
    if (!characters || !count) return &bmx_pico_empty_string;
    if (count > (size_t)INT32_MAX / 2u) {
        bmx_pico_record_string_failure();
        return &bmx_pico_empty_string;
    }
    size_t units = 0;
    for (size_t index = 0; index < count; ++index) units += characters[index] > 0xffffu && characters[index] <= 0x10ffffu ? 2u : 1u;
    if (units > INT32_MAX) {
        bmx_pico_record_string_failure();
        return &bmx_pico_empty_string;
    }
    BMXPicoString *result = bmx_pico_string_new((int32_t)units);
    if (result == &bmx_pico_empty_string) return result;
    uint16_t *output = (uint16_t *)(uintptr_t)result->buf;
    for (size_t index = 0; index < count; ++index) {
        uint32_t code_point = characters[index];
        if (code_point >= 0xd800u && code_point <= 0xdfffu) code_point = 0xfffdu;
        else if (code_point > 0x10ffffu) code_point = 0xfffdu;
        if (code_point <= 0xffffu) *output++ = (uint16_t)code_point;
        else {
            code_point -= 0x10000u;
            *output++ = (uint16_t)(0xd800u + (code_point >> 10));
            *output++ = (uint16_t)(0xdc00u + (code_point & 0x3ffu));
        }
    }
    return result;
}

const BMXPicoString *bmx_pico_string_from_utf32_string(const uint32_t *characters) {
    if (!characters) return &bmx_pico_empty_string;
    size_t count = 0;
    while (characters[count]) {
        if (count == INT32_MAX) {
            bmx_pico_record_string_failure();
            return &bmx_pico_empty_string;
        }
        ++count;
    }
    return bmx_pico_string_from_utf32_bytes(characters, count);
}

const BMXPicoString *bmx_pico_string_from_bytes_as_hex(const uint8_t *bytes, int32_t length, int32_t upper_case) {
    if (!bytes || length <= 0) return &bmx_pico_empty_string;
    if (length > INT32_MAX / 2) {
        bmx_pico_record_string_failure();
        return &bmx_pico_empty_string;
    }
    static const char lower[] = "0123456789abcdef";
    static const char upper[] = "0123456789ABCDEF";
    const char *digits = upper_case ? upper : lower;
    BMXPicoString *result = bmx_pico_string_new(length * 2);
    if (result == &bmx_pico_empty_string) return result;
    uint16_t *output = (uint16_t *)(uintptr_t)result->buf;
    for (int32_t index = 0; index < length; ++index) {
        output[index * 2] = (uint16_t)digits[bytes[index] >> 4];
        output[index * 2 + 1] = (uint16_t)digits[bytes[index] & 15u];
    }
    return result;
}

static int32_t bmx_pico_hex_value(uint16_t character) {
    if (character >= '0' && character <= '9') return character - '0';
    if (character >= 'a' && character <= 'f') return character - 'a' + 10;
    if (character >= 'A' && character <= 'F') return character - 'A' + 10;
    return -1;
}

int32_t bmx_pico_string_to_bytes_from_hex_ex(const BMXPicoString *text, int32_t offset, int32_t count,
        uint8_t *bytes, int32_t length) {
    if (!text || !text->length || offset < 0 || count < 0 || offset > text->length) return 0;
    if (!bytes || length <= 0) return -1;
    int32_t end = text->length;
    if (count < end - offset) end = offset + count;
    int32_t written = 0;
    for (int32_t index = offset; index + 1 < end; index += 2) {
        int32_t high = bmx_pico_hex_value(text->buf[index]);
        int32_t low = bmx_pico_hex_value(text->buf[index + 1]);
        if (high < 0 || low < 0) break;
        if (written >= length) return -1;
        bytes[written++] = (uint8_t)((high << 4) | low);
    }
    return written;
}

int32_t bmx_pico_string_to_bytes_from_hex(const BMXPicoString *text, uint8_t *bytes, int32_t length) {
    return bmx_pico_string_to_bytes_from_hex_ex(text, 0, text ? text->length : 0, bytes, length);
}

const BMXPicoString *bmx_pico_string_concat(const BMXPicoString *left, const BMXPicoString *right) {
    if (!left || !right || left->length < 0 || right->length < 0 || left->length > INT32_MAX - right->length) {
        bmx_pico_record_string_failure();
        return &bmx_pico_empty_string;
    }
    if (get_core_num() != 0 || __get_current_exception() != 0) {
        bmx_pico_record_arena_failure();
        bmx_pico_record_string_failure();
        return &bmx_pico_empty_string;
    }
    const BMXPicoString *left_root = left;
    const BMXPicoString *right_root = right;
    BMXPicoRootSlot slots[2] = {
        {(void *)&left_root, BMX_PICO_ROOT_STRING, NULL},
        {(void *)&right_root, BMX_PICO_ROOT_STRING, NULL}
    };
    BMXPicoRootFrame frame;
    bmx_pico_root_frame_enter(&frame, slots, 2);
    BMXPicoString *result = bmx_pico_string_new(left->length + right->length);
    if (result != &bmx_pico_empty_string) {
        uint16_t *buffer = (uint16_t *)(uintptr_t)result->buf;
        memcpy(buffer, left->buf, (size_t)left->length * sizeof(uint16_t));
        memcpy(buffer + left->length, right->buf, (size_t)right->length * sizeof(uint16_t));
    }
    bmx_pico_root_frame_leave(&frame);
    return result;
}

const BMXPicoString *bmx_pico_string_slice(const BMXPicoString *text, int32_t begin, int32_t end) {
    if (!text || text->length < 0 || end <= begin) return &bmx_pico_empty_string;
    int64_t length64 = (int64_t)end - begin;
    if (length64 > INT32_MAX) {
        bmx_pico_record_string_failure();
        return &bmx_pico_empty_string;
    }
    if (get_core_num() != 0 || __get_current_exception() != 0) {
        bmx_pico_record_arena_failure();
        bmx_pico_record_string_failure();
        return &bmx_pico_empty_string;
    }
    const BMXPicoString *text_root = text;
    BMXPicoRootSlot slot = {(void *)&text_root, BMX_PICO_ROOT_STRING, NULL};
    BMXPicoRootFrame frame;
    bmx_pico_root_frame_enter(&frame, &slot, 1);
    BMXPicoString *result = bmx_pico_string_new((int32_t)length64);
    if (result != &bmx_pico_empty_string) {
        uint16_t *buffer = (uint16_t *)(uintptr_t)result->buf;
        for (int32_t index = 0; index < result->length; ++index) {
            int64_t source = (int64_t)begin + index;
            buffer[index] = source >= 0 && source < text->length ? text->buf[source] : (uint16_t)' ';
        }
    }
    bmx_pico_root_frame_leave(&frame);
    return result;
}

const BMXPicoString *bmx_pico_string_from_char(int32_t character) {
    BMXPicoString *result = bmx_pico_string_new(1);
    if (result != &bmx_pico_empty_string) ((uint16_t *)(uintptr_t)result->buf)[0] = (uint16_t)character;
    return result;
}

static int bmx_pico_enum_descriptor_valid(const BMXPicoEnumDescriptor *descriptor) {
    if (!descriptor || !descriptor->name || (descriptor->count && (!descriptor->values || !descriptor->names))) return 0;
    return descriptor->element_size == 1u || descriptor->element_size == 2u || descriptor->element_size == 4u || descriptor->element_size == 8u;
}

static void bmx_pico_record_enum_failure(void) {
    __atomic_fetch_add(&bmx_pico_enum_failures, 1u, __ATOMIC_RELAXED);
}

static void bmx_pico_enum_store(void *result, uint16_t size, uint64_t value) {
    if (size == 1u) *(uint8_t *)result = (uint8_t)value;
    else if (size == 2u) *(uint16_t *)result = (uint16_t)value;
    else if (size == 4u) *(uint32_t *)result = (uint32_t)value;
    else *(uint64_t *)result = value;
}

static uint16_t bmx_pico_ascii_fold(uint16_t character) {
    if (character >= (uint16_t)'A' && character <= (uint16_t)'Z') return character + ((uint16_t)'a' - (uint16_t)'A');
    return character;
}

static int bmx_pico_enum_name_equals(const BMXPicoString *candidate, const uint16_t *text, int32_t length) {
    if (!candidate || candidate->length != length) return 0;
    for (int32_t index = 0; index < length; ++index) {
        if (bmx_pico_ascii_fold(candidate->buf[index]) != bmx_pico_ascii_fold(text[index])) return 0;
    }
    return 1;
}

BMXPicoArray *bmx_pico_enum_values(const BMXPicoEnumDescriptor *descriptor) {
    if (!bmx_pico_enum_descriptor_valid(descriptor)) {
        bmx_pico_record_enum_failure();
        return &bmx_pico_empty_array;
    }
    BMXPicoArray *result = bmx_pico_array_new_1d(descriptor->count, descriptor->element_size, BMX_PICO_ARRAY_ELEMENT_VALUE, NULL, NULL);
    if (descriptor->count && result == &bmx_pico_empty_array) {
        bmx_pico_record_enum_failure();
        return result;
    }
    for (uint16_t index = 0; index < descriptor->count; ++index) {
        bmx_pico_enum_store(bmx_pico_array_element(result, index, descriptor->element_size), descriptor->element_size, descriptor->values[index]);
    }
    return result;
}

const BMXPicoString *bmx_pico_enum_to_string(const BMXPicoEnumDescriptor *descriptor, uint64_t value) {
    if (!bmx_pico_enum_descriptor_valid(descriptor)) {
        bmx_pico_record_enum_failure();
        return &bmx_pico_empty_string;
    }
    if (!(descriptor->flags & BMX_PICO_ENUM_FLAG_FLAGS)) {
        for (uint16_t index = 0; index < descriptor->count; ++index) {
            if (descriptor->values[index] == value) return descriptor->names[index];
        }
        return &bmx_pico_empty_string;
    }

    uint64_t remaining = value;
    uint32_t length = 0;
    uint16_t part_count = 0;
    for (uint16_t index = 0; index < descriptor->count; ++index) {
        uint64_t item = descriptor->values[index];
        int selected = item ? (remaining & item) == item : value == 0 && !part_count;
        if (!selected) continue;
        if (part_count) length += 1u;
        length += (uint32_t)descriptor->names[index]->length;
        part_count += 1u;
        if (item) remaining &= ~item;
    }
    if (!part_count || length > INT32_MAX) return &bmx_pico_empty_string;
    BMXPicoString *result = bmx_pico_string_new((int32_t)length);
    if (result == &bmx_pico_empty_string) {
        bmx_pico_record_enum_failure();
        return result;
    }
    uint16_t *buffer = (uint16_t *)(uintptr_t)result->buf;
    uint32_t offset = 0;
    remaining = value;
    part_count = 0;
    for (uint16_t index = 0; index < descriptor->count; ++index) {
        uint64_t item = descriptor->values[index];
        int selected = item ? (remaining & item) == item : value == 0 && !part_count;
        if (!selected) continue;
        if (part_count) buffer[offset++] = (uint16_t)'|';
        const BMXPicoString *name = descriptor->names[index];
        memcpy(buffer + offset, name->buf, (size_t)name->length * sizeof(uint16_t));
        offset += (uint32_t)name->length;
        part_count += 1u;
        if (item) remaining &= ~item;
    }
    return result;
}

int32_t bmx_pico_enum_try_convert(const BMXPicoEnumDescriptor *descriptor, uint64_t value, void *result) {
    if (!bmx_pico_enum_descriptor_valid(descriptor) || !result) {
        bmx_pico_record_enum_failure();
        return 0;
    }
    if (!(descriptor->flags & BMX_PICO_ENUM_FLAG_FLAGS)) {
        for (uint16_t index = 0; index < descriptor->count; ++index) {
            if (descriptor->values[index] == value) {
                bmx_pico_enum_store(result, descriptor->element_size, value);
                return 1;
            }
        }
        return 0;
    }
    uint64_t remaining = value;
    int zero_declared = 0;
    for (uint16_t index = 0; index < descriptor->count; ++index) {
        uint64_t item = descriptor->values[index];
        if (!item) zero_declared = 1;
        else if ((remaining & item) == item) remaining &= ~item;
    }
    if (remaining || (!value && !zero_declared)) return 0;
    bmx_pico_enum_store(result, descriptor->element_size, value);
    return 1;
}

uint64_t bmx_pico_enum_from_string(const BMXPicoEnumDescriptor *descriptor, const BMXPicoString *name) {
    if (!bmx_pico_enum_descriptor_valid(descriptor) || !name || !name->length) {
        bmx_pico_record_enum_failure();
        return 0;
    }
    if (!(descriptor->flags & BMX_PICO_ENUM_FLAG_FLAGS)) {
        for (uint16_t index = 0; index < descriptor->count; ++index) {
            if (bmx_pico_enum_name_equals(descriptor->names[index], name->buf, name->length)) return descriptor->values[index];
        }
        bmx_pico_record_enum_failure();
        return 0;
    }
    uint64_t result = 0;
    int32_t segment_start = 0;
    for (int32_t index = 0; index <= name->length; ++index) {
        if (index != name->length && name->buf[index] != (uint16_t)'|') continue;
        int32_t segment_length = index - segment_start;
        int matched = 0;
        if (segment_length > 0) {
            for (uint16_t candidate = 0; candidate < descriptor->count; ++candidate) {
                if (bmx_pico_enum_name_equals(descriptor->names[candidate], name->buf + segment_start, segment_length)) {
                    result |= descriptor->values[candidate];
                    matched = 1;
                    break;
                }
            }
        }
        if (!matched) {
            bmx_pico_record_enum_failure();
            return 0;
        }
        segment_start = index + 1;
    }
    return result;
}

uint32_t bmx_pico_enum_failure_count(void) {
    return __atomic_load_n(&bmx_pico_enum_failures, __ATOMIC_RELAXED);
}

int32_t bmx_pico_string_asc(const BMXPicoString *text) {
    return text->length ? text->buf[0] : -1;
}

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

int32_t bmx_pico_put_string(const BMXPicoString *text) {
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

void __attribute__((noinline, used)) bmx_pico_debug_stop(void) {
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

const BMXPicoString *bmx_pico_unique_board_id(void) {
    char identifier[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2u + 1u];
    pico_get_unique_board_id_string(identifier, sizeof(identifier));
    return bmx_pico_string_from_ascii(identifier, PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2);
}

BMXPicoArray *bmx_pico_unique_board_id_bytes(void) {
    pico_unique_board_id_t identifier;
    pico_get_unique_board_id(&identifier);
    BMXPicoArray *result = bmx_pico_array_new_1d(PICO_UNIQUE_BOARD_ID_SIZE_BYTES,
        sizeof(uint8_t), BMX_PICO_ARRAY_ELEMENT_VALUE, NULL, NULL);
    if (result != &bmx_pico_empty_array) {
        memcpy(bmx_pico_array_data(result), identifier.id, PICO_UNIQUE_BOARD_ID_SIZE_BYTES);
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
static int bmx_pico_gpio_irq_callback_installed;

static void bmx_pico_gpio_irq_callback(uint gpio, uint32_t events) {
    if (gpio < NUM_BANK0_GPIOS) bmx_pico_gpio_irq_events[gpio] |= events;
}

int32_t bmx_pico_gpio_set_irq_enabled(uint32_t gpio, uint32_t event_mask, int32_t enabled) {
    if (get_core_num() != 0) return 0;
    if (enabled && !bmx_pico_gpio_irq_callback_installed) {
        gpio_set_irq_enabled_with_callback(gpio, event_mask, true, bmx_pico_gpio_irq_callback);
        bmx_pico_gpio_irq_callback_installed = 1;
    } else {
        gpio_set_irq_enabled(gpio, event_mask, enabled != 0);
    }
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

int32_t bmx_pico_millisecs(void) {
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

void *bmx_pico_pio_find_program(const BMXPicoString *name) {
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
    return program->initialize(pio, state_machine, offset);
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
static uint8_t bmx_pico_pio_irq_installed[NUM_PIOS][NUM_PIO_IRQS];

static void bmx_pico_pio_irq_handler(uint32_t controller, uint32_t irq_line) {
    PIO pio = pio_get_instance(controller);
    uint32_t pending = pio->irq_ctrl[irq_line].ints &
        bmx_pico_pio_irq_source_masks[controller][irq_line];
    if (!pending) return;
    hw_clear_bits(&pio->irq_ctrl[irq_line].inte, pending);
    bmx_pico_pio_irq_events[controller][irq_line] |= pending;
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

static volatile uint32_t bmx_pico_dma_completion_events[2][NUM_DMA_CHANNELS];
static volatile uint32_t bmx_pico_dma_irq_channel_masks[2];
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
    }
    dma_channel_cleanup(channel);
    dma_channel_unclaim(channel);
    return 1;
}

int32_t bmx_pico_dma_configure(uint32_t channel, void *read_address,
        void *write_address, uint32_t transfer_count, uint32_t data_size,
        int32_t read_increment, int32_t write_increment, uint32_t dreq, int32_t start) {
    if (get_core_num() != 0 || !bmx_pico_dma_channel_valid(channel) ||
            !dma_channel_is_claimed(channel) || !read_address || !write_address ||
            !transfer_count || data_size > DMA_SIZE_32 || dreq > DREQ_FORCE ||
            dma_channel_is_busy(channel)) return 0;
#if !PICO_RP2040
    if (transfer_count & ~DMA_CH0_TRANS_COUNT_COUNT_BITS) return 0;
#endif
    dma_channel_config config = dma_channel_get_default_config(channel);
    channel_config_set_transfer_data_size(&config, (enum dma_channel_transfer_size)data_size);
    channel_config_set_read_increment(&config, read_increment != 0);
    channel_config_set_write_increment(&config, write_increment != 0);
    channel_config_set_dreq(&config, dreq);
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

void bmx_pico_delay(int32_t milliseconds) {
    if (milliseconds > 0) sleep_ms((uint32_t)milliseconds);
}

void bmx_pico_udelay(int32_t microseconds) {
    if (microseconds > 0) sleep_us((uint64_t)(uint32_t)microseconds);
}
