#include "littlefs/lfs.h"
#include "blitzmax/pico_runtime.h"

#include <stdint.h>
#include <string.h>

#ifndef BMX_PICO_STORAGE_SIZE
#define BMX_PICO_STORAGE_SIZE 0u
#endif

#if BMX_PICO_STORAGE_SIZE == 0
#error "Pico.Storage.LittleFS requires a reserved flash region; build with -storage <size>"
#endif

#define BMX_LFS_BLOCK_SIZE 4096u
#define BMX_LFS_CACHE_SIZE 256u
#define BMX_LFS_LOOKAHEAD_SIZE 32u
#define BMX_LFS_TIMEOUT_MS 1000u
#define BMX_LFS_TIME_ATTRIBUTE 0xb1u
#define BMX_LFS_TIME_ATTRIBUTE_SIZE 32u

extern uint64_t bmx_current_unix_time(void);

typedef struct BMXPicoLittleFSFile {
    lfs_file_t file;
    struct lfs_file_config config;
    uint8_t cache[BMX_LFS_CACHE_SIZE];
    char *path;
    int32_t write_mode;
} BMXPicoLittleFSFile;

typedef struct BMXPicoLittleFSDirectory {
    lfs_dir_t directory;
} BMXPicoLittleFSDirectory;

static lfs_t bmx_lfs;
static struct lfs_config bmx_lfs_config;
static uint8_t bmx_lfs_read_buffer[BMX_LFS_CACHE_SIZE];
static uint8_t bmx_lfs_program_buffer[BMX_LFS_CACHE_SIZE];
static uint8_t bmx_lfs_lookahead_buffer[BMX_LFS_LOOKAHEAD_SIZE];
static int32_t bmx_lfs_mounted;
static int32_t bmx_lfs_last_error;

typedef struct BMXPicoLittleFSTimes {
    int64_t created;
    int64_t modified;
    int64_t accessed;
} BMXPicoLittleFSTimes;

static int64_t bmx_lfs_decode_i64(const uint8_t *source) {
    uint64_t value = 0;
    for (uint32_t index = 0; index < 8u; ++index) {
        value |= (uint64_t)source[index] << (index * 8u);
    }
    return (int64_t)value;
}

static void bmx_lfs_encode_i64(uint8_t *destination, int64_t signed_value) {
    uint64_t value = (uint64_t)signed_value;
    for (uint32_t index = 0; index < 8u; ++index) {
        destination[index] = (uint8_t)(value >> (index * 8u));
    }
}

static int bmx_lfs_read_times(const char *path, BMXPicoLittleFSTimes *times) {
    uint8_t data[BMX_LFS_TIME_ATTRIBUTE_SIZE];
    memset(times, 0, sizeof(*times));
    lfs_ssize_t size = lfs_getattr(&bmx_lfs, path, BMX_LFS_TIME_ATTRIBUTE,
        data, sizeof(data));
    if (size == LFS_ERR_NOATTR) return LFS_ERR_OK;
    if (size < 0) return size;
    if (size != (lfs_ssize_t)sizeof(data) || data[0] != 'B' || data[1] != 'M' ||
        data[2] != 'X' || data[3] != 'T' || data[4] != 1u) return LFS_ERR_OK;
    times->created = bmx_lfs_decode_i64(data + 8);
    times->modified = bmx_lfs_decode_i64(data + 16);
    times->accessed = bmx_lfs_decode_i64(data + 24);
    return LFS_ERR_OK;
}

static int bmx_lfs_write_times(const char *path, const BMXPicoLittleFSTimes *times) {
    uint8_t data[BMX_LFS_TIME_ATTRIBUTE_SIZE] = {'B', 'M', 'X', 'T', 1u};
    bmx_lfs_encode_i64(data + 8, times->created);
    bmx_lfs_encode_i64(data + 16, times->modified);
    bmx_lfs_encode_i64(data + 24, times->accessed);
    return lfs_setattr(&bmx_lfs, path, BMX_LFS_TIME_ATTRIBUTE, data, sizeof(data));
}

static int64_t bmx_lfs_now(void) {
    return (int64_t)(bmx_current_unix_time() / 1000u);
}

static int bmx_lfs_touch_created(const char *path) {
    BMXPicoLittleFSTimes times;
    int result = bmx_lfs_read_times(path, &times);
    if (result) return result;
    int64_t now = bmx_lfs_now();
    times.created = now;
    times.modified = now;
    return bmx_lfs_write_times(path, &times);
}

static int bmx_lfs_touch_modified(const char *path) {
    BMXPicoLittleFSTimes times;
    int result = bmx_lfs_read_times(path, &times);
    if (result) return result;
    times.modified = bmx_lfs_now();
    return bmx_lfs_write_times(path, &times);
}

static int bmx_lfs_set_result(int result) {
    bmx_lfs_last_error = result;
    return result;
}

static int bmx_lfs_read(const struct lfs_config *config, lfs_block_t block,
        lfs_off_t offset, void *buffer, lfs_size_t size) {
    uint32_t address = block * config->block_size + offset;
    return bmx_pico_flash_storage_read(address, buffer, size) == 0
        ? LFS_ERR_OK : LFS_ERR_IO;
}

static int bmx_lfs_program(const struct lfs_config *config, lfs_block_t block,
        lfs_off_t offset, const void *buffer, lfs_size_t size) {
    uint32_t address = block * config->block_size + offset;
    return bmx_pico_flash_storage_program(address, (void *)buffer, size,
        BMX_LFS_TIMEOUT_MS) == 0 ? LFS_ERR_OK : LFS_ERR_IO;
}

static int bmx_lfs_erase(const struct lfs_config *config, lfs_block_t block) {
    uint32_t address = block * config->block_size;
    return bmx_pico_flash_storage_erase(address, config->block_size,
        BMX_LFS_TIMEOUT_MS) == 0 ? LFS_ERR_OK : LFS_ERR_IO;
}

static int bmx_lfs_sync(const struct lfs_config *config) {
    (void)config;
    return LFS_ERR_OK;
}

static void bmx_lfs_initialize_config(void) {
    if (bmx_lfs_config.block_size) return;

    memset(&bmx_lfs_config, 0, sizeof(bmx_lfs_config));
    bmx_lfs_config.read = bmx_lfs_read;
    bmx_lfs_config.prog = bmx_lfs_program;
    bmx_lfs_config.erase = bmx_lfs_erase;
    bmx_lfs_config.sync = bmx_lfs_sync;
    bmx_lfs_config.read_size = 1u;
    bmx_lfs_config.prog_size = BMX_LFS_CACHE_SIZE;
    bmx_lfs_config.block_size = BMX_LFS_BLOCK_SIZE;
    bmx_lfs_config.block_count = bmx_pico_flash_storage_size() / BMX_LFS_BLOCK_SIZE;
    bmx_lfs_config.block_cycles = 500;
    bmx_lfs_config.cache_size = BMX_LFS_CACHE_SIZE;
    bmx_lfs_config.lookahead_size = BMX_LFS_LOOKAHEAD_SIZE;
    bmx_lfs_config.read_buffer = bmx_lfs_read_buffer;
    bmx_lfs_config.prog_buffer = bmx_lfs_program_buffer;
    bmx_lfs_config.lookahead_buffer = bmx_lfs_lookahead_buffer;
}

static int bmx_lfs_storage_is_blank(void) {
    uint8_t buffer[BMX_LFS_CACHE_SIZE];
    uint32_t size = bmx_pico_flash_storage_size();
    for (uint32_t offset = 0; offset < size; offset += sizeof(buffer)) {
        uint32_t count = size - offset;
        if (count > sizeof(buffer)) count = sizeof(buffer);
        if (bmx_pico_flash_storage_read(offset, buffer, count) != 0) return 0;
        for (uint32_t index = 0; index < count; ++index) {
            if (buffer[index] != 0xffu) return 0;
        }
    }
    return 1;
}

int32_t bmx_pico_littlefs_mount(int32_t format_blank) {
    if (bmx_lfs_mounted) return bmx_lfs_set_result(LFS_ERR_OK);
    bmx_lfs_initialize_config();
    if (bmx_lfs_config.block_count < 2u) return bmx_lfs_set_result(LFS_ERR_INVAL);

    int result = lfs_mount(&bmx_lfs, &bmx_lfs_config);
    if (result && format_blank && bmx_lfs_storage_is_blank()) {
        result = lfs_format(&bmx_lfs, &bmx_lfs_config);
        if (!result) result = lfs_mount(&bmx_lfs, &bmx_lfs_config);
    }
    bmx_lfs_mounted = result == LFS_ERR_OK;
    return bmx_lfs_set_result(result);
}

int32_t bmx_pico_littlefs_unmount(void) {
    if (!bmx_lfs_mounted) return bmx_lfs_set_result(LFS_ERR_OK);
    int result = lfs_unmount(&bmx_lfs);
    if (!result) bmx_lfs_mounted = 0;
    return bmx_lfs_set_result(result);
}

int32_t bmx_pico_littlefs_format(void) {
    if (bmx_lfs_mounted) {
        int result = lfs_unmount(&bmx_lfs);
        if (result) return bmx_lfs_set_result(result);
        bmx_lfs_mounted = 0;
    }
    bmx_lfs_initialize_config();
    int result = lfs_format(&bmx_lfs, &bmx_lfs_config);
    if (!result) {
        result = lfs_mount(&bmx_lfs, &bmx_lfs_config);
        bmx_lfs_mounted = result == LFS_ERR_OK;
    }
    return bmx_lfs_set_result(result);
}

int32_t bmx_pico_littlefs_is_mounted(void) {
    return bmx_lfs_mounted;
}

int32_t bmx_pico_littlefs_last_error(void) {
    return bmx_lfs_last_error;
}

uint32_t bmx_pico_littlefs_capacity(void) {
    bmx_lfs_initialize_config();
    return bmx_lfs_config.block_count * bmx_lfs_config.block_size;
}

int64_t bmx_pico_littlefs_used(void) {
    if (!bmx_lfs_mounted) return bmx_lfs_set_result(LFS_ERR_BADF);
    lfs_ssize_t blocks = lfs_fs_size(&bmx_lfs);
    if (blocks < 0) return bmx_lfs_set_result(blocks);
    bmx_lfs_set_result(LFS_ERR_OK);
    return (int64_t)blocks * bmx_lfs_config.block_size;
}

static char *bmx_lfs_path(const BMXPicoString *path) {
    return (char *)bmx_pico_string_to_utf8_string(path);
}

void *bmx_pico_littlefs_open(const BMXPicoString *path, int32_t readable,
        int32_t write_mode) {
    if (!bmx_lfs_mounted) {
        bmx_lfs_set_result(LFS_ERR_BADF);
        return NULL;
    }

    int flags;
    if (readable && write_mode == 1) {
        flags = LFS_O_RDWR;
    } else if (readable && write_mode == 2) {
        flags = LFS_O_RDWR | LFS_O_CREAT | LFS_O_APPEND;
    } else if (write_mode == 1) {
        flags = LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC;
    } else if (write_mode == 2) {
        flags = LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND;
    } else {
        flags = LFS_O_RDONLY;
    }

    BMXPicoLittleFSFile *handle = (BMXPicoLittleFSFile *)bbMemAlloc(sizeof(*handle));
    if (!handle) {
        bmx_lfs_set_result(LFS_ERR_NOMEM);
        return NULL;
    }
    memset(handle, 0, sizeof(*handle));
    handle->config.buffer = handle->cache;

    char *native_path = bmx_lfs_path(path);
    if (!native_path) {
        bbMemFree(handle);
        bmx_lfs_set_result(LFS_ERR_NOMEM);
        return NULL;
    }
    struct lfs_info existing_info;
    int existed = lfs_stat(&bmx_lfs, native_path, &existing_info) == LFS_ERR_OK;
    int result = lfs_file_opencfg(&bmx_lfs, &handle->file, native_path, flags,
        &handle->config);
    if (result) {
        bbMemFree(native_path);
        bbMemFree(handle);
        bmx_lfs_set_result(result);
        return NULL;
    }
    handle->path = native_path;
    handle->write_mode = write_mode;
    if (!existed && write_mode) {
        result = bmx_lfs_touch_created(native_path);
        if (result) {
            lfs_file_close(&bmx_lfs, &handle->file);
            bbMemFree(native_path);
            bbMemFree(handle);
            bmx_lfs_set_result(result);
            return NULL;
        }
    }
    bmx_lfs_set_result(LFS_ERR_OK);
    return handle;
}

int32_t bmx_pico_littlefs_close(void *opaque) {
    if (!opaque) return bmx_lfs_set_result(LFS_ERR_OK);
    BMXPicoLittleFSFile *handle = (BMXPicoLittleFSFile *)opaque;
    int result = lfs_file_close(&bmx_lfs, &handle->file);
    if (!result && handle->write_mode) result = bmx_lfs_touch_modified(handle->path);
    bbMemFree(handle->path);
    bbMemFree(handle);
    return bmx_lfs_set_result(result);
}

int64_t bmx_pico_littlefs_read(void *opaque, void *buffer, int64_t count) {
    if (!opaque || !buffer || count < 0 || count > LFS_FILE_MAX) {
        return bmx_lfs_set_result(LFS_ERR_INVAL);
    }
    lfs_ssize_t result = lfs_file_read(&bmx_lfs,
        &((BMXPicoLittleFSFile *)opaque)->file, buffer, (lfs_size_t)count);
    bmx_lfs_set_result(result < 0 ? result : LFS_ERR_OK);
    return result;
}

int64_t bmx_pico_littlefs_write(void *opaque, void *buffer, int64_t count) {
    if (!opaque || !buffer || count < 0 || count > LFS_FILE_MAX) {
        return bmx_lfs_set_result(LFS_ERR_INVAL);
    }
    lfs_ssize_t result = lfs_file_write(&bmx_lfs,
        &((BMXPicoLittleFSFile *)opaque)->file, buffer, (lfs_size_t)count);
    bmx_lfs_set_result(result < 0 ? result : LFS_ERR_OK);
    return result;
}

int64_t bmx_pico_littlefs_position(void *opaque) {
    if (!opaque) return bmx_lfs_set_result(LFS_ERR_BADF);
    lfs_soff_t result = lfs_file_tell(&bmx_lfs,
        &((BMXPicoLittleFSFile *)opaque)->file);
    bmx_lfs_set_result(result < 0 ? result : LFS_ERR_OK);
    return result;
}

int64_t bmx_pico_littlefs_size(void *opaque) {
    if (!opaque) return bmx_lfs_set_result(LFS_ERR_BADF);
    lfs_soff_t result = lfs_file_size(&bmx_lfs,
        &((BMXPicoLittleFSFile *)opaque)->file);
    bmx_lfs_set_result(result < 0 ? result : LFS_ERR_OK);
    return result;
}

int64_t bmx_pico_littlefs_seek(void *opaque, int64_t offset, int32_t whence) {
    if (!opaque || offset < INT32_MIN || offset > INT32_MAX || whence < 0 || whence > 2) {
        return bmx_lfs_set_result(LFS_ERR_INVAL);
    }
    lfs_soff_t result = lfs_file_seek(&bmx_lfs,
        &((BMXPicoLittleFSFile *)opaque)->file, (lfs_soff_t)offset, whence);
    bmx_lfs_set_result(result < 0 ? result : LFS_ERR_OK);
    return result;
}

int32_t bmx_pico_littlefs_resize(void *opaque, int64_t size) {
    if (!opaque || size < 0 || size > LFS_FILE_MAX) {
        return bmx_lfs_set_result(LFS_ERR_INVAL);
    }
    return bmx_lfs_set_result(lfs_file_truncate(&bmx_lfs,
        &((BMXPicoLittleFSFile *)opaque)->file, (lfs_off_t)size));
}

int32_t bmx_pico_littlefs_flush(void *opaque) {
    if (!opaque) return bmx_lfs_set_result(LFS_ERR_BADF);
    return bmx_lfs_set_result(lfs_file_sync(&bmx_lfs,
        &((BMXPicoLittleFSFile *)opaque)->file));
}

int32_t bmx_pico_littlefs_stat(const BMXPicoString *path, int32_t *type,
        int64_t *size, int64_t *modified, int64_t *created, int64_t *accessed) {
    if (!bmx_lfs_mounted) return bmx_lfs_set_result(LFS_ERR_BADF);
    char *native_path = bmx_lfs_path(path);
    if (!native_path) return bmx_lfs_set_result(LFS_ERR_NOMEM);
    struct lfs_info info;
    int result = lfs_stat(&bmx_lfs, native_path, &info);
    if (!result) {
        if (type) *type = info.type == LFS_TYPE_DIR ? 2 : 1;
        if (size) *size = info.size;
        BMXPicoLittleFSTimes times;
        result = bmx_lfs_read_times(native_path, &times);
        if (!result) {
            if (modified) *modified = times.modified;
            if (created) *created = times.created;
            if (accessed) *accessed = times.accessed;
        }
    }
    bbMemFree(native_path);
    return bmx_lfs_set_result(result);
}

int32_t bmx_pico_littlefs_set_time(const BMXPicoString *path, int64_t time,
        int32_t time_type) {
    if (!bmx_lfs_mounted || time_type < 0 || time_type > 2) {
        return bmx_lfs_set_result(LFS_ERR_INVAL);
    }
    char *native_path = bmx_lfs_path(path);
    if (!native_path) return bmx_lfs_set_result(LFS_ERR_NOMEM);
    BMXPicoLittleFSTimes times;
    int result = bmx_lfs_read_times(native_path, &times);
    if (!result) {
        if (time_type == 0) times.modified = time;
        else if (time_type == 1) times.created = time;
        else times.accessed = time;
        result = bmx_lfs_write_times(native_path, &times);
    }
    bbMemFree(native_path);
    return bmx_lfs_set_result(result);
}

static int bmx_lfs_path_operation(const BMXPicoString *path,
        int (*operation)(lfs_t *, const char *)) {
    if (!bmx_lfs_mounted) return bmx_lfs_set_result(LFS_ERR_BADF);
    char *native_path = bmx_lfs_path(path);
    if (!native_path) return bmx_lfs_set_result(LFS_ERR_NOMEM);
    int result = operation(&bmx_lfs, native_path);
    bbMemFree(native_path);
    return bmx_lfs_set_result(result);
}

int32_t bmx_pico_littlefs_mkdir(const BMXPicoString *path) {
    if (!bmx_lfs_mounted) return bmx_lfs_set_result(LFS_ERR_BADF);
    char *native_path = bmx_lfs_path(path);
    if (!native_path) return bmx_lfs_set_result(LFS_ERR_NOMEM);
    int result = lfs_mkdir(&bmx_lfs, native_path);
    if (!result) result = bmx_lfs_touch_created(native_path);
    bbMemFree(native_path);
    return bmx_lfs_set_result(result);
}

int32_t bmx_pico_littlefs_remove(const BMXPicoString *path) {
    return bmx_lfs_path_operation(path, lfs_remove);
}

int32_t bmx_pico_littlefs_rename(const BMXPicoString *old_path,
        const BMXPicoString *new_path) {
    if (!bmx_lfs_mounted) return bmx_lfs_set_result(LFS_ERR_BADF);
    char *native_old_path = bmx_lfs_path(old_path);
    char *native_new_path = bmx_lfs_path(new_path);
    if (!native_old_path || !native_new_path) {
        if (native_old_path) bbMemFree(native_old_path);
        if (native_new_path) bbMemFree(native_new_path);
        return bmx_lfs_set_result(LFS_ERR_NOMEM);
    }
    int result = lfs_rename(&bmx_lfs, native_old_path, native_new_path);
    bbMemFree(native_old_path);
    bbMemFree(native_new_path);
    return bmx_lfs_set_result(result);
}

void *bmx_pico_littlefs_directory_open(const BMXPicoString *path) {
    if (!bmx_lfs_mounted) {
        bmx_lfs_set_result(LFS_ERR_BADF);
        return NULL;
    }
    BMXPicoLittleFSDirectory *handle =
        (BMXPicoLittleFSDirectory *)bbMemAlloc(sizeof(*handle));
    if (!handle) {
        bmx_lfs_set_result(LFS_ERR_NOMEM);
        return NULL;
    }
    memset(handle, 0, sizeof(*handle));
    char *native_path = bmx_lfs_path(path);
    if (!native_path) {
        bbMemFree(handle);
        bmx_lfs_set_result(LFS_ERR_NOMEM);
        return NULL;
    }
    int result = lfs_dir_open(&bmx_lfs, &handle->directory, native_path);
    bbMemFree(native_path);
    if (result) {
        bbMemFree(handle);
        bmx_lfs_set_result(result);
        return NULL;
    }
    bmx_lfs_set_result(LFS_ERR_OK);
    return handle;
}

const BMXPicoString *bmx_pico_littlefs_directory_next(void *opaque) {
    if (!opaque) {
        bmx_lfs_set_result(LFS_ERR_BADF);
        return &bmx_pico_empty_string;
    }
    struct lfs_info info;
    int result = lfs_dir_read(&bmx_lfs,
        &((BMXPicoLittleFSDirectory *)opaque)->directory, &info);
    if (result <= 0) {
        bmx_lfs_set_result(result < 0 ? result : LFS_ERR_OK);
        return &bmx_pico_empty_string;
    }
    bmx_lfs_set_result(LFS_ERR_OK);
    return bmx_pico_string_from_utf8_string((const uint8_t *)info.name);
}

int32_t bmx_pico_littlefs_directory_close(void *opaque) {
    if (!opaque) return bmx_lfs_set_result(LFS_ERR_OK);
    BMXPicoLittleFSDirectory *handle = (BMXPicoLittleFSDirectory *)opaque;
    int result = lfs_dir_close(&bmx_lfs, &handle->directory);
    bbMemFree(handle);
    return bmx_lfs_set_result(result);
}
