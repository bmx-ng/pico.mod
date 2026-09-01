#ifndef BLITZMAX_PICO_RUNTIME_H
#define BLITZMAX_PICO_RUNTIME_H

#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Scalar BRL.Blitz intrinsics. Functions, rather than macros, preserve the
   BlitzMax rule that each argument expression is evaluated exactly once. */
#define BMX_PICO_MINMAX(type, suffix) \
    static inline type bmx_pico_min_##suffix(type a, type b) { return a > b ? b : a; } \
    static inline type bmx_pico_max_##suffix(type a, type b) { return a < b ? b : a; }

BMX_PICO_MINMAX(int32_t, i32)
BMX_PICO_MINMAX(int64_t, i64)
BMX_PICO_MINMAX(float, f32)
BMX_PICO_MINMAX(double, f64)
BMX_PICO_MINMAX(uint8_t, u8)
BMX_PICO_MINMAX(uint16_t, u16)
BMX_PICO_MINMAX(uint32_t, u32)
BMX_PICO_MINMAX(uint64_t, u64)
BMX_PICO_MINMAX(size_t, size)
BMX_PICO_MINMAX(long, long)
BMX_PICO_MINMAX(unsigned long, ulong)

#undef BMX_PICO_MINMAX

static inline int32_t bmx_pico_abs_i32(int32_t value) { return value >= 0 ? value : -value; }
static inline int64_t bmx_pico_abs_i64(int64_t value) { return value >= 0 ? value : -value; }
static inline float bmx_pico_abs_f32(float value) { return __builtin_fabsf(value); }
static inline double bmx_pico_abs_f64(double value) { return __builtin_fabs(value); }
static inline int32_t bmx_pico_sgn_i32(int32_t value) { return value == 0 ? 0 : (value > 0 ? 1 : -1); }
static inline int64_t bmx_pico_sgn_i64(int64_t value) { return value == 0 ? 0 : (value > 0 ? 1 : -1); }
static inline float bmx_pico_sgn_f32(float value) { return value == 0 ? 0.0f : (value > 0 ? 1.0f : -1.0f); }
static inline double bmx_pico_sgn_f64(double value) { return value == 0 ? 0.0 : (value > 0 ? 1.0 : -1.0); }

typedef struct BMXPicoString {
    int32_t length;
    const uint16_t *buf;
} BMXPicoString;

typedef struct BMXPicoValueDescriptor BMXPicoValueDescriptor;
typedef void (*BMXPicoArrayInitializer)(void *element);

typedef struct BMXPicoArray {
    int32_t length;
    uint32_t element_size;
    uint16_t element_kind;
    uint16_t reserved;
    BMXPicoArrayInitializer initializer;
    const BMXPicoValueDescriptor *element_descriptor;
} BMXPicoArray;

/* Standard BlitzMax native ABI spelling retained for shared BRL code. */
typedef BMXPicoArray *BBARRAY;

typedef struct BMXPicoEnumDescriptor {
    const char *name;
    const uint64_t *values;
    const BMXPicoString *const *names;
    uint16_t count;
    uint16_t element_size;
    uint16_t flags;
} BMXPicoEnumDescriptor;

#define BMX_PICO_ENUM_FLAG_FLAGS 0x0001u

#define BMX_PICO_ARRAY_ELEMENT_VALUE 0u
#define BMX_PICO_ARRAY_ELEMENT_STRING 1u
#define BMX_PICO_ARRAY_ELEMENT_OBJECT 2u

typedef struct BMXPicoValueField {
    uint32_t offset;
    uint32_t stride;
    uint32_t count;
    uint16_t kind;
    const BMXPicoValueDescriptor *descriptor;
} BMXPicoValueField;

struct BMXPicoValueDescriptor {
    const char *name;
    uint32_t size;
    const BMXPicoValueField *fields;
    uint32_t field_count;
};

#define BMX_PICO_VALUE_OBJECT 1u
#define BMX_PICO_VALUE_ARRAY 2u
#define BMX_PICO_VALUE_STRING 3u
#define BMX_PICO_VALUE_STRUCT 4u

typedef void (*BMXPicoTraceVisitor)(void *reference, void *context);
typedef void (*BMXPicoTraceFunction)(void *object, BMXPicoTraceVisitor visitor, void *context);
typedef void (*BMXPicoFinalizer)(void *object);
typedef void (*BMXPicoMethod)(void);
typedef struct BMXPicoInterfaceDescriptor {
    const char *name;
    const char *abi_name;
} BMXPicoInterfaceDescriptor;

typedef struct BMXPicoInterfaceEntry {
    const BMXPicoInterfaceDescriptor *interface_type;
    const BMXPicoMethod *methods;
    uint32_t method_count;
} BMXPicoInterfaceEntry;
typedef int32_t (*BMXPicoObjectCompare)(void *object, void *other);
typedef uint32_t (*BMXPicoObjectHashCode)(void *object);
typedef int32_t (*BMXPicoObjectEquals)(void *object, void *other);

typedef struct BMXPicoTypeDescriptor {
    const char *name;
    const char *abi_name;
    uint32_t instance_size;
    const struct BMXPicoTypeDescriptor *super;
    const BMXPicoMethod *methods;
    uint32_t method_count;
    const BMXPicoInterfaceEntry *interfaces;
    uint32_t interface_count;
    const uint32_t *reference_offsets;
    uint32_t reference_count;
    const uint32_t *array_offsets;
    uint32_t array_count;
    const uint32_t *string_offsets;
    uint32_t string_count;
    const BMXPicoValueField *value_fields;
    uint32_t value_field_count;
    uint16_t flags;
    BMXPicoTraceFunction trace;
    BMXPicoFinalizer finalizer;
    BMXPicoObjectCompare compare;
    BMXPicoObjectHashCode hash_code;
    BMXPicoObjectEquals equals;
} BMXPicoTypeDescriptor;

typedef struct BMXPicoObject {
    const BMXPicoTypeDescriptor *type;
} BMXPicoObject;

typedef struct BMXPicoClosure {
    BMXPicoObject object;
    BMXPicoMethod invoke;
    BMXPicoObject *environment;
} BMXPicoClosure;

#define BMX_PICO_EXCEPTION_NONE 0u
#define BMX_PICO_EXCEPTION_OBJECT 1u
#define BMX_PICO_EXCEPTION_ARRAY 2u
#define BMX_PICO_EXCEPTION_STRING 3u

typedef struct BMXPicoException {
    void *value;
    uint16_t kind;
    uint16_t reserved;
} BMXPicoException;

typedef struct BMXPicoRootSlot {
    void *address;
    uint16_t kind;
    const BMXPicoValueDescriptor *descriptor;
} BMXPicoRootSlot;

#define BMX_PICO_ROOT_OBJECT 1u
#define BMX_PICO_ROOT_ARRAY 2u
#define BMX_PICO_ROOT_STRING 3u
#define BMX_PICO_ROOT_STRUCT 4u
#define BMX_PICO_ROOT_EXCEPTION 5u

typedef struct BMXPicoRootFrame {
    struct BMXPicoRootFrame *previous;
    BMXPicoRootSlot *slots;
    uint16_t slot_count;
} BMXPicoRootFrame;

typedef struct BMXPicoExceptionFrame {
    struct BMXPicoExceptionFrame *previous;
    BMXPicoRootFrame *root_snapshot;
    uint32_t root_frame_count;
    uint32_t root_slot_count;
    jmp_buf buffer;
} BMXPicoExceptionFrame;

#define BMX_PICO_TYPE_FLAG_CUSTOM_TRACE 0x0001u
#define BMX_PICO_TYPE_FLAG_HAS_FINALIZER 0x0002u

extern const BMXPicoString bmx_pico_empty_string;
extern BMXPicoArray bmx_pico_empty_array;
extern BMXPicoObject bmx_pico_null_object;

int32_t bmx_pico_string_compare(const BMXPicoString *left, const BMXPicoString *right);
int32_t bmx_pico_string_equals(const BMXPicoString *left, const BMXPicoString *right);
uint32_t bmx_pico_string_hash(const BMXPicoString *text);
int32_t bmx_pico_string_compare_case(const BMXPicoString *left, const BMXPicoString *right, int32_t case_sensitive);
int32_t bmx_pico_string_equals_case(const BMXPicoString *left, const BMXPicoString *right, int32_t case_sensitive);
uint32_t bmx_pico_string_hash_case(const BMXPicoString *text, int32_t case_sensitive);
typedef const BMXPicoString *(*BMXPicoStringCaseTransform)(const BMXPicoString *text);
typedef uint16_t (*BMXPicoCharacterCaseFold)(uint16_t character);
void bmx_pico_string_install_unicode_case(BMXPicoStringCaseTransform lower,
    BMXPicoStringCaseTransform upper, BMXPicoCharacterCaseFold fold);
const BMXPicoString *bmx_pico_string_to_string(const BMXPicoString *text);
int32_t bmx_pico_string_find(const BMXPicoString *text, const BMXPicoString *substring, int32_t start);
int32_t bmx_pico_string_find_last(const BMXPicoString *text, const BMXPicoString *substring, int32_t start);
const BMXPicoString *bmx_pico_string_trim(const BMXPicoString *text);
const BMXPicoString *bmx_pico_string_replace(const BMXPicoString *text, const BMXPicoString *substring, const BMXPicoString *replacement);
const BMXPicoString *bmx_pico_string_to_lower(const BMXPicoString *text);
const BMXPicoString *bmx_pico_string_to_upper(const BMXPicoString *text);
int32_t bmx_pico_string_starts_with(const BMXPicoString *text, const BMXPicoString *substring);
int32_t bmx_pico_string_ends_with(const BMXPicoString *text, const BMXPicoString *substring);
int32_t bmx_pico_string_contains(const BMXPicoString *text, const BMXPicoString *substring);
const BMXPicoString *bmx_pico_string_replicate(const BMXPicoString *text, int32_t count);
BMXPicoArray *bmx_pico_string_split(const BMXPicoString *text, const BMXPicoString *separator);
const BMXPicoString *bmx_pico_string_join(const BMXPicoString *separator, BMXPicoArray *parts);
const BMXPicoString *bmx_pico_string_from_bytes(const uint8_t *bytes, int32_t count);
const BMXPicoString *bmx_pico_string_from_shorts(const uint16_t *characters, int32_t count);
const BMXPicoString *bmx_pico_string_from_c_string(const uint8_t *bytes);
const BMXPicoString *bmx_pico_string_from_w_string(const uint16_t *characters);
const BMXPicoString *bmx_pico_string_from_ascii(const char *bytes, int32_t count);
const BMXPicoString *bmx_pico_string_from_utf8_string(const uint8_t *bytes);
const BMXPicoString *bmx_pico_string_from_utf8_bytes(const uint8_t *bytes, int32_t count);
uint8_t *bmx_pico_string_to_c_string(const BMXPicoString *text);
uint16_t *bmx_pico_string_to_w_string(const BMXPicoString *text);
uint16_t *bmx_pico_string_to_w_string_buffer(const BMXPicoString *text, uint16_t *buffer, size_t *length);
uint8_t *bmx_pico_string_to_utf8_string(const BMXPicoString *text);
uint8_t *bmx_pico_string_to_utf8_string_len(const BMXPicoString *text, size_t *length);
uint8_t *bmx_pico_string_to_utf8_string_buffer(const BMXPicoString *text, uint8_t *buffer, size_t *length);
uint32_t *bmx_pico_string_to_utf32_string(const BMXPicoString *text);
const BMXPicoString *bmx_pico_string_from_utf32_string(const uint32_t *characters);
const BMXPicoString *bmx_pico_string_from_utf32_bytes(const uint32_t *characters, size_t count);
const BMXPicoString *bmx_pico_string_from_bytes_as_hex(const uint8_t *bytes, int32_t length, int32_t upper_case);
int32_t bmx_pico_string_to_bytes_from_hex(const BMXPicoString *text, uint8_t *bytes, int32_t length);
int32_t bmx_pico_string_to_bytes_from_hex_ex(const BMXPicoString *text, int32_t offset, int32_t count, uint8_t *bytes, int32_t length);
const BMXPicoString *bmx_pico_stream_url_string(BMXPicoObject *value);
const BMXPicoString *bmx_pico_string_concat(const BMXPicoString *left, const BMXPicoString *right);
const BMXPicoString *bmx_pico_string_slice(const BMXPicoString *text, int32_t begin, int32_t end);
const BMXPicoString *bmx_pico_string_from_char(int32_t character);
int32_t bmx_pico_string_asc(const BMXPicoString *text);
int32_t bmx_pico_put_string(const BMXPicoString *text);
int32_t bmx_pico_stdio_init_all(void);
void bmx_pico_debug_stop(void);
int64_t bmx_pico_stdio_read(void *buffer, int64_t count);
int64_t bmx_pico_stdio_write(void *buffer, int64_t count);
void bmx_pico_stdio_flush(void);
int32_t bmx_pico_putchar_raw(int32_t character);
void bmx_pico_delay(int32_t milliseconds);
void bmx_pico_udelay(int32_t microseconds);
uint32_t bmx_pico_string_failure_count(void);
uint32_t bmx_pico_string_allocation_count(void);
uint32_t bmx_pico_string_allocated_bytes(void);
uint32_t bmx_pico_string_live_count(void);
uint32_t bmx_pico_string_live_bytes(void);
uint32_t bmx_pico_reachable_string_count(void);
uint32_t bmx_pico_unreachable_string_count(void);

const BMXPicoString *bmx_pico_string_from_int32(int32_t value);
const BMXPicoString *bmx_pico_string_from_uint32(uint32_t value);
const BMXPicoString *bmx_pico_string_from_int64(int64_t value);
const BMXPicoString *bmx_pico_string_from_uint64(uint64_t value);
const BMXPicoString *bmx_pico_string_from_size(size_t value);
const BMXPicoString *bmx_pico_string_from_long(long value);
const BMXPicoString *bmx_pico_string_from_ulong(unsigned long value);
BMXPicoString *bmx_pico_string_allocate(int32_t length);
int32_t bmx_pico_string_to_int32(const BMXPicoString *text);
uint32_t bmx_pico_string_to_uint32(const BMXPicoString *text);
int64_t bmx_pico_string_to_int64(const BMXPicoString *text);
uint64_t bmx_pico_string_to_uint64(const BMXPicoString *text);
size_t bmx_pico_string_to_size(const BMXPicoString *text);
long bmx_pico_string_to_long(const BMXPicoString *text);
unsigned long bmx_pico_string_to_ulong(const BMXPicoString *text);
BMXPicoArray *bmx_pico_string_split_ints(const BMXPicoString *text, const BMXPicoString *separator);
BMXPicoArray *bmx_pico_string_split_bytes(const BMXPicoString *text, const BMXPicoString *separator);
BMXPicoArray *bmx_pico_string_split_shorts(const BMXPicoString *text, const BMXPicoString *separator);
BMXPicoArray *bmx_pico_string_split_uints(const BMXPicoString *text, const BMXPicoString *separator);
BMXPicoArray *bmx_pico_string_split_longs(const BMXPicoString *text, const BMXPicoString *separator);
BMXPicoArray *bmx_pico_string_split_ulongs(const BMXPicoString *text, const BMXPicoString *separator);
BMXPicoArray *bmx_pico_string_split_sizes(const BMXPicoString *text, const BMXPicoString *separator);
BMXPicoArray *bmx_pico_string_split_long_ints(const BMXPicoString *text, const BMXPicoString *separator);
BMXPicoArray *bmx_pico_string_split_ulong_ints(const BMXPicoString *text, const BMXPicoString *separator);
const BMXPicoString *bmx_pico_string_join_ints(const BMXPicoString *separator, BMXPicoArray *values);
const BMXPicoString *bmx_pico_string_join_bytes(const BMXPicoString *separator, BMXPicoArray *values);
const BMXPicoString *bmx_pico_string_join_shorts(const BMXPicoString *separator, BMXPicoArray *values);
const BMXPicoString *bmx_pico_string_join_uints(const BMXPicoString *separator, BMXPicoArray *values);
const BMXPicoString *bmx_pico_string_join_longs(const BMXPicoString *separator, BMXPicoArray *values);
const BMXPicoString *bmx_pico_string_join_ulongs(const BMXPicoString *separator, BMXPicoArray *values);
const BMXPicoString *bmx_pico_string_join_sizes(const BMXPicoString *separator, BMXPicoArray *values);
const BMXPicoString *bmx_pico_string_join_long_ints(const BMXPicoString *separator, BMXPicoArray *values);
const BMXPicoString *bmx_pico_string_join_ulong_ints(const BMXPicoString *separator, BMXPicoArray *values);
BMXPicoArray *bmx_pico_string_split_floats(const BMXPicoString *text, const BMXPicoString *separator);
BMXPicoArray *bmx_pico_string_split_doubles(const BMXPicoString *text, const BMXPicoString *separator);
const BMXPicoString *bmx_pico_string_join_floats_default(const BMXPicoString *separator, BMXPicoArray *values);
const BMXPicoString *bmx_pico_string_join_floats_fixed(const BMXPicoString *separator, BMXPicoArray *values);
const BMXPicoString *bmx_pico_string_join_doubles_default(const BMXPicoString *separator, BMXPicoArray *values);
const BMXPicoString *bmx_pico_string_join_doubles_fixed(const BMXPicoString *separator, BMXPicoArray *values);
static inline const BMXPicoString *bmx_pico_string_join_floats(const BMXPicoString *separator,
    BMXPicoArray *values, int32_t fixed) {
    return fixed ? bmx_pico_string_join_floats_fixed(separator, values) :
        bmx_pico_string_join_floats_default(separator, values);
}
static inline const BMXPicoString *bmx_pico_string_join_doubles(const BMXPicoString *separator,
    BMXPicoArray *values, int32_t fixed) {
    return fixed ? bmx_pico_string_join_doubles_fixed(separator, values) :
        bmx_pico_string_join_doubles_default(separator, values);
}
const BMXPicoString *bmx_pico_string_from_float_default(float value);
const BMXPicoString *bmx_pico_string_from_double_default(double value);
const BMXPicoString *bmx_pico_string_from_float_fixed(float value);
const BMXPicoString *bmx_pico_string_from_double_fixed(double value);
static inline const BMXPicoString *bmx_pico_string_from_float(float value, int32_t fixed) {
    return fixed ? bmx_pico_string_from_float_fixed(value) : bmx_pico_string_from_float_default(value);
}
static inline const BMXPicoString *bmx_pico_string_from_double(double value, int32_t fixed) {
    return fixed ? bmx_pico_string_from_double_fixed(value) : bmx_pico_string_from_double_default(value);
}
float bmx_pico_string_to_float(const BMXPicoString *text);
double bmx_pico_string_to_double(const BMXPicoString *text);

BMXPicoArray *bmx_pico_enum_values(const BMXPicoEnumDescriptor *descriptor);
const BMXPicoString *bmx_pico_enum_to_string(const BMXPicoEnumDescriptor *descriptor, uint64_t value);
int32_t bmx_pico_enum_try_convert(const BMXPicoEnumDescriptor *descriptor, uint64_t value, void *result);
uint64_t bmx_pico_enum_from_string(const BMXPicoEnumDescriptor *descriptor, const BMXPicoString *name);
uint32_t bmx_pico_enum_failure_count(void);

BMXPicoArray *bmx_pico_array_new_1d(int32_t length, uint32_t element_size, uint16_t element_kind, BMXPicoArrayInitializer initializer, const BMXPicoValueDescriptor *element_descriptor);
BMXPicoArray *bmx_pico_array_concat(BMXPicoArray *left, BMXPicoArray *right);
void bbArrayCopy(BBARRAY src, int src_pos, BBARRAY dst, int dst_pos, int length);
BMXPicoArray *bmx_pico_array_slice(BMXPicoArray *array, int32_t begin, int32_t end,
    uint32_t element_size, uint16_t element_kind, BMXPicoArrayInitializer initializer,
    const BMXPicoValueDescriptor *element_descriptor);
void *bmx_pico_array_element(BMXPicoArray *array, int32_t index, uint32_t element_size);
void *bmx_pico_array_data(BMXPicoArray *array);

void *bbMemAlloc(size_t size);
void bbMemFree(void *memory);
void *bbMemExtend(void *memory, size_t size, size_t new_size);
void *bbMemAllocCollectable(size_t size);
void bbMemFreeCollectable(void *memory);
void *bbMemExtendCollectable(void *memory, size_t size, size_t new_size);
void bbMemClear(void *memory, size_t size);
void bbMemCopy(void *destination, const void *source, size_t size);
void bbMemMove(void *destination, const void *source, size_t size);

int32_t bbIncbinAdd(const BMXPicoString *path, const void *data, int32_t size);
void *bbIncbinPtr(const BMXPicoString *path);
int32_t bbIncbinLen(const BMXPicoString *path);

void *brl_blitz_MemAlloc__Bsize_t__Bint(size_t size, int32_t collectable);
void brl_blitz_MemFree__PBbyte__Bint(void *memory, int32_t collectable);
void *brl_blitz_MemExtend__PBbyte__Bsize_t__Bsize_t__Bint(void *memory, size_t size, size_t new_size, int32_t collectable);
uint32_t bmx_pico_array_failure_count(void);
uint32_t bmx_pico_array_allocation_count(void);
uint32_t bmx_pico_array_allocated_bytes(void);
uint32_t bmx_pico_array_live_count(void);
uint32_t bmx_pico_array_live_bytes(void);
uint32_t bmx_pico_reachable_array_count(void);
uint32_t bmx_pico_unreachable_array_count(void);

void *bmx_pico_object_allocate(const BMXPicoTypeDescriptor *type);
void *bmx_pico_object_assert(void *object);
void *bmx_pico_object_null_failure(void);
static inline void *bmx_pico_object_not_null(void *object) {
    if (!object || object == &bmx_pico_null_object) return bmx_pico_object_null_failure();
    return object;
}
int32_t bmx_pico_object_compare(void *object, void *other);
uint32_t bmx_pico_object_hash_code(void *object);
int32_t bmx_pico_object_equals(void *object, void *other);
void *bmx_pico_object_cast(void *object, const BMXPicoTypeDescriptor *target);
const BMXPicoMethod *bmx_pico_type_methods(void *object, const BMXPicoTypeDescriptor *target, uint32_t method_count);
void *bmx_pico_interface_cast(void *object, const BMXPicoInterfaceDescriptor *target);
const BMXPicoMethod *bmx_pico_interface_methods(void *object, const BMXPicoInterfaceDescriptor *target, uint32_t method_count);
BMXPicoClosure *bmx_pico_closure_allocate(BMXPicoMethod invoke, BMXPicoObject *environment);
BMXPicoClosure *bmx_pico_closure_assert(void *closure);
uint32_t bmx_pico_object_failure_count(void);
uint32_t bmx_pico_object_allocation_count(void);
uint32_t bmx_pico_object_allocated_bytes(void);
uint32_t bmx_pico_object_live_count(void);
uint32_t bmx_pico_object_live_bytes(void);
uint32_t bmx_pico_object_root_retain(BMXPicoObject *object);
void bmx_pico_object_root_release(uint32_t token);
uint32_t bmx_pico_object_root_count(void);
uint32_t bmx_pico_reachability_audit(void);
uint32_t bmx_pico_reachable_object_count(void);
uint32_t bmx_pico_unreachable_object_count(void);
uint32_t bmx_pico_invalid_reference_count(void);
uint32_t bmx_pico_collect_objects(void);
uint32_t bmx_pico_collection_count(void);
uint32_t bmx_pico_automatic_collection_count(void);
uint32_t bmx_pico_last_reclaimed_object_count(void);
uint32_t bmx_pico_last_reclaimed_bytes(void);
uint32_t bmx_pico_last_reclaimed_array_count(void);
uint32_t bmx_pico_last_reclaimed_array_bytes(void);
uint32_t bmx_pico_last_reclaimed_string_count(void);
uint32_t bmx_pico_last_reclaimed_string_bytes(void);
uint32_t bmx_pico_finalizer_pending_count(void);
uint32_t bmx_pico_finalizer_invocation_count(void);
uint32_t bmx_pico_last_finalized_object_count(void);
uint32_t bmx_pico_heap_reusable_bytes(void);
uint32_t bmx_pico_heap_largest_free_block(void);
void bmx_pico_root_frame_enter(BMXPicoRootFrame *frame, BMXPicoRootSlot *slots, uint16_t slot_count);
void bmx_pico_root_frame_leave(BMXPicoRootFrame *frame);
uint32_t bmx_pico_root_frame_count(void);
uint32_t bmx_pico_root_slot_count(void);
void bmx_pico_exception_enter(BMXPicoExceptionFrame *frame);
void bmx_pico_exception_leave(void);
BMXPicoException bmx_pico_exception_object(BMXPicoObject *value);
BMXPicoException bmx_pico_exception_array(BMXPicoArray *value);
BMXPicoException bmx_pico_exception_string(const BMXPicoString *value);
BMXPicoException bmx_pico_exception_catch(void);
void bmx_pico_exception_throw(BMXPicoException exception);
uint32_t bmx_pico_exception_depth(void);
uint32_t bmx_pico_exception_throw_count(void);
uint32_t bmx_pico_exception_catch_count(void);
uint32_t bmx_pico_exception_max_depth(void);
uint32_t bmx_pico_exception_unhandled_count(void);

void *bmx_pico_arena_allocate(uint32_t bytes);
uint32_t bmx_pico_arena_capacity(void);
uint32_t bmx_pico_arena_used(void);
uint32_t bmx_pico_arena_remaining(void);
uint32_t bmx_pico_arena_high_water(void);
uint32_t bmx_pico_arena_allocation_count(void);
uint32_t bmx_pico_arena_failure_count(void);

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
int32_t bmx_pico_gpio_set_irq_enabled(uint32_t gpio, uint32_t event_mask, int32_t enabled);
uint32_t bmx_pico_gpio_pending_irq_events(uint32_t gpio);
uint32_t bmx_pico_gpio_take_irq_events(uint32_t gpio);

int32_t bmx_pico_millisecs(void);
uint64_t bmx_pico_time_microseconds(void);
uint32_t bmx_pico_system_clock_hz(void);
uint64_t bmx_pico_time_milliseconds(void);
void bmx_pico_sleep_us(uint64_t microseconds);
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

typedef int32_t (*BMXPicoPIOProgramInitializer)(void *instance, uint32_t state_machine,
    uint32_t offset);

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
int32_t bmx_pico_pio_sm_set_consecutive_pin_directions(int32_t controller,
    uint32_t state_machine, uint32_t pin_base, uint32_t pin_count, int32_t output);
int32_t bmx_pico_pio_sm_init(int32_t controller, uint32_t state_machine, uint32_t initial_pc);
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
int32_t bmx_pico_pio_sm_restart(int32_t controller, uint32_t state_machine);
int32_t bmx_pico_pio_sm_restart_clock_divider(int32_t controller, uint32_t state_machine);
int32_t bmx_pico_pio_sm_clear_fifos(int32_t controller, uint32_t state_machine);
int32_t bmx_pico_pio_sm_execute(int32_t controller, uint32_t state_machine,
    uint32_t instruction);
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
uint32_t bmx_pico_pio_irq_enabled_sources(int32_t controller, uint32_t irq_line);
uint32_t bmx_pico_pio_irq_armed_sources(int32_t controller, uint32_t irq_line);
uint32_t bmx_pico_pio_irq_pending_events(int32_t controller, uint32_t irq_line);
uint32_t bmx_pico_pio_irq_take_events(int32_t controller, uint32_t irq_line);
int32_t bmx_pico_pio_irq_rearm_sources(int32_t controller, uint32_t irq_line,
    uint32_t source_mask);
int32_t bmx_pico_pio_interrupt_is_set(int32_t controller, uint32_t interrupt_number);
int32_t bmx_pico_pio_interrupt_clear(int32_t controller, uint32_t interrupt_number);

uint32_t bmx_pico_dma_channel_count(void);
uint32_t bmx_pico_dma_irq_line_count(void);
uint32_t bmx_pico_dma_force_dreq(void);
int32_t bmx_pico_dma_claim_unused_channel(void);
int32_t bmx_pico_dma_channel_is_claimed(uint32_t channel);
int32_t bmx_pico_dma_unclaim_channel(uint32_t channel);
int32_t bmx_pico_dma_configure(uint32_t channel, void *read_address, void *write_address,
    uint32_t transfer_count, uint32_t data_size, int32_t read_increment,
    int32_t write_increment, uint32_t dreq, int32_t start);
int32_t bmx_pico_dma_start(uint32_t channel);
int32_t bmx_pico_dma_abort(uint32_t channel);
int32_t bmx_pico_dma_busy(uint32_t channel);
uint32_t bmx_pico_dma_remaining(uint32_t channel);
int32_t bmx_pico_dma_set_irq_enabled(uint32_t channel, uint32_t irq_line,
    int32_t enabled);
uint32_t bmx_pico_dma_pending_completion_events(uint32_t channel, uint32_t irq_line);
uint32_t bmx_pico_dma_take_completion_events(uint32_t channel, uint32_t irq_line);
void *bmx_pico_pio_find_program(const BMXPicoString *name);
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
