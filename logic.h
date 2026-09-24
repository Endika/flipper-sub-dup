#ifndef LOGIC_H
#define LOGIC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MAX_FILES 128
#define APP_MAX_PATH_LEN 64
#define FULL_PATH_LEN 280

typedef struct {
    char path[APP_MAX_PATH_LEN];
    uint32_t hash;
    uint32_t size;
} FileRecord;

typedef struct {
    uint32_t hash;
    uint32_t size;
    size_t start_index;
    size_t count;
} DuplicateGroup;

typedef struct {
    FileRecord records[MAX_FILES];
    size_t count;
    DuplicateGroup groups[MAX_FILES];
    size_t num_groups;
} HashDatabase;

typedef struct {
    size_t added;
    size_t unreadable;
    size_t name_too_long;
    bool hit_limit;
} ScanStats;

typedef enum {
    ScanAddOk,
    ScanAddStop,
} ScanAdd;

uint32_t calculate_crc32(uint32_t crc, const uint8_t *data, size_t size);
void process_duplicates(HashDatabase *db);
void db_remove_record(HashDatabase *db, const char *filename);

bool is_sub_file(const char *name);
bool path_join(char *out, size_t cap, const char *dir, const char *name);
bool scan_dir_is_valid(const char *path);
ScanAdd scan_add_file(HashDatabase *db, ScanStats *stats, const char *name, uint32_t size,
                      uint32_t hash, bool read_ok);

#endif
