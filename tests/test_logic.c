#include "logic.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void set_record(HashDatabase *db, size_t i, const char *name, uint32_t hash, uint32_t size) {
    strcpy(db->records[i].path, name);
    db->records[i].hash = hash;
    db->records[i].size = size;
}

static void test_duplicate_detection() {
    HashDatabase db;
    db.count = 4;

    set_record(&db, 0, "file1.sub", 100, 10);
    set_record(&db, 1, "file2.sub", 200, 10);
    set_record(&db, 2, "file3.sub", 100, 10);
    set_record(&db, 3, "file4.sub", 300, 10);

    process_duplicates(&db);

    assert(db.num_groups == 1);
    assert(db.groups[0].hash == 100);
    assert(db.groups[0].count == 2);

    printf("Test passed: Duplicate detection works.\n");
}

static void test_same_hash_different_size_not_grouped() {
    HashDatabase db;
    db.count = 2;

    set_record(&db, 0, "file1.sub", 100, 10);
    set_record(&db, 1, "file2.sub", 100, 20);

    process_duplicates(&db);

    assert(db.num_groups == 0);

    printf("Test passed: same CRC, different size is not a duplicate.\n");
}

static void test_two_empty_files_are_duplicates() {
    HashDatabase db;
    db.count = 2;

    set_record(&db, 0, "file1.sub", 0, 0);
    set_record(&db, 1, "file2.sub", 0, 0);

    process_duplicates(&db);

    assert(db.num_groups == 1);
    assert(db.groups[0].count == 2);
    assert(db.groups[0].size == 0);

    printf("Test passed: two empty files form a duplicate group.\n");
}

static void test_db_remove_record() {
    HashDatabase db;
    db.count = 3;

    set_record(&db, 0, "file1.sub", 100, 10);
    set_record(&db, 1, "file2.sub", 200, 10);
    set_record(&db, 2, "file3.sub", 300, 10);

    db_remove_record(&db, "file2.sub");

    assert(db.count == 2);
    assert(strcmp(db.records[0].path, "file1.sub") == 0);
    assert(db.records[0].hash == 100);
    assert(strcmp(db.records[1].path, "file3.sub") == 0);
    assert(db.records[1].hash == 300);

    printf("Test passed: db_remove_record works.\n");
}

static void test_db_remove_record_not_found() {
    HashDatabase db;
    db.count = 2;

    set_record(&db, 0, "file1.sub", 100, 10);
    set_record(&db, 1, "file2.sub", 200, 10);

    db_remove_record(&db, "nonexistent.sub");

    assert(db.count == 2);
    assert(strcmp(db.records[0].path, "file1.sub") == 0);
    assert(strcmp(db.records[1].path, "file2.sub") == 0);

    printf("Test passed: db_remove_record handles missing file.\n");
}

static void test_db_remove_record_last() {
    HashDatabase db;
    db.count = 1;

    set_record(&db, 0, "only.sub", 100, 10);

    db_remove_record(&db, "only.sub");

    assert(db.count == 0);

    printf("Test passed: db_remove_record removes last record.\n");
}

static void test_scan_add_unreadable_never_grouped() {
    HashDatabase db = {0};
    ScanStats stats = {0};

    assert(scan_add_file(&db, &stats, "broken.sub", 0, 0, false) == ScanAddOk);

    assert(db.count == 0);
    assert(stats.unreadable == 1);
    assert(stats.added == 0);

    printf("Test passed: an unreadable file is never added, and is counted.\n");
}

static void test_scan_add_hits_limit_without_dropping() {
    HashDatabase db = {0};
    ScanStats stats = {0};

    for (size_t i = 0; i < MAX_FILES; i++) {
        char name[APP_MAX_PATH_LEN];
        snprintf(name, sizeof(name), "file%zu.sub", i);
        ScanAdd result = scan_add_file(&db, &stats, name, 10, (uint32_t)i, true);
        assert(result == ScanAddOk);
    }

    assert(db.count == MAX_FILES);
    assert(stats.hit_limit == false);
    assert(strcmp(db.records[MAX_FILES - 1].path, "file127.sub") == 0);
    assert(db.records[MAX_FILES - 1].hash == MAX_FILES - 1);

    ScanAdd result = scan_add_file(&db, &stats, "one_too_many.sub", 10, 999, true);

    assert(result == ScanAddStop);
    assert(stats.hit_limit == true);
    assert(db.count == MAX_FILES);
    assert(strcmp(db.records[MAX_FILES - 1].path, "file127.sub") == 0);
    assert(db.records[MAX_FILES - 1].hash == MAX_FILES - 1);

    printf("Test passed: the 129th file stops the scan without dropping any record.\n");
}

static void test_scan_add_name_length_boundary() {
    HashDatabase db = {0};
    ScanStats stats = {0};

    char name63[APP_MAX_PATH_LEN];
    memset(name63, 'a', APP_MAX_PATH_LEN - 1 - 4);
    strcpy(name63 + APP_MAX_PATH_LEN - 1 - 4, ".sub");
    assert(strlen(name63) == APP_MAX_PATH_LEN - 1);

    assert(scan_add_file(&db, &stats, name63, 10, 1, true) == ScanAddOk);
    assert(db.count == 1);
    assert(stats.name_too_long == 0);

    char name64[APP_MAX_PATH_LEN + 1];
    memset(name64, 'a', APP_MAX_PATH_LEN - 4);
    strcpy(name64 + APP_MAX_PATH_LEN - 4, ".sub");
    assert(strlen(name64) == APP_MAX_PATH_LEN);

    assert(scan_add_file(&db, &stats, name64, 10, 2, true) == ScanAddOk);
    assert(db.count == 1);
    assert(stats.name_too_long == 1);

    printf("Test passed: a name of exactly APP_MAX_PATH_LEN - 1 fits, one longer doesn't.\n");
}

static void test_scan_add_long_name_takes_priority_over_unreadable() {
    HashDatabase db = {0};
    ScanStats stats = {0};

    char long_name[APP_MAX_PATH_LEN + 10];
    memset(long_name, 'a', sizeof(long_name) - 5);
    strcpy(long_name + sizeof(long_name) - 5, ".sub");

    ScanAdd result = scan_add_file(&db, &stats, long_name, 0, 0, false);

    assert(result == ScanAddOk);
    assert(stats.name_too_long == 1);
    assert(stats.unreadable == 0);

    printf("Test passed: a too-long name is counted as such even when also unreadable.\n");
}

static void test_scan_add_long_name_counted_not_truncated() {
    HashDatabase db = {0};
    ScanStats stats = {0};

    char long_name[APP_MAX_PATH_LEN + 10];
    memset(long_name, 'a', sizeof(long_name) - 5);
    strcpy(long_name + sizeof(long_name) - 5, ".sub");

    ScanAdd result = scan_add_file(&db, &stats, long_name, 10, 1, true);

    assert(result == ScanAddOk);
    assert(db.count == 0);
    assert(stats.name_too_long == 1);

    printf("Test passed: an over-long name is counted, never truncated into the db.\n");
}

static void test_scan_add_accepts_uppercase_sub_ignores_txt() {
    HashDatabase db = {0};
    ScanStats stats = {0};

    assert(scan_add_file(&db, &stats, "FILE.SUB", 10, 1, true) == ScanAddOk);
    assert(db.count == 1);

    assert(scan_add_file(&db, &stats, "notes.txt", 10, 2, true) == ScanAddOk);
    assert(db.count == 1);
    assert(stats.unreadable == 0);
    assert(stats.name_too_long == 0);

    printf("Test passed: .SUB is accepted and .txt is filtered silently.\n");
}

static void test_path_join_refuses_overflow() {
    char out[16];

    assert(path_join(out, sizeof(out), "/ext", "short.sub") == true);
    assert(strcmp(out, "/ext/short.sub") == 0);

    assert(path_join(out, sizeof(out), "/ext/subghz", "short.sub") == false);

    printf("Test passed: path_join refuses to silently truncate.\n");
}

static void test_scan_dir_is_valid_table() {
    assert(scan_dir_is_valid("/ext/subghz") == true);
    assert(scan_dir_is_valid("/ext") == true);
    assert(scan_dir_is_valid("/ext/subghz/nested") == true);
    assert(scan_dir_is_valid("/extra") == false);
    assert(scan_dir_is_valid("/sd/subghz") == false);
    assert(scan_dir_is_valid("/ext/../root") == false);
    assert(scan_dir_is_valid("") == false);
    assert(scan_dir_is_valid(NULL) == false);

    printf("Test passed: scan_dir_is_valid table holds.\n");
}

int main() {
    test_duplicate_detection();
    test_same_hash_different_size_not_grouped();
    test_two_empty_files_are_duplicates();
    test_db_remove_record();
    test_db_remove_record_not_found();
    test_db_remove_record_last();
    test_scan_add_unreadable_never_grouped();
    test_scan_add_hits_limit_without_dropping();
    test_scan_add_name_length_boundary();
    test_scan_add_long_name_takes_priority_over_unreadable();
    test_scan_add_long_name_counted_not_truncated();
    test_scan_add_accepts_uppercase_sub_ignores_txt();
    test_path_join_refuses_overflow();
    test_scan_dir_is_valid_table();
    return 0;
}
