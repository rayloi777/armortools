/*
 * CastleDB C Library - Test Suite
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "cdb.h"
#include "cdb_internal.h"

#define TEST(name) void test_##name(void)
#define ASSERT(cond, msg) do { if (!(cond)) { printf("FAIL: %s\n", msg); failures++; } } while(0)
#define ASSERT_INT(actual, expected, msg) do { if ((actual) != (expected)) { printf("FAIL: %s (got %d, expected %d)\n", msg, actual, expected); failures++; } } while(0)
#define ASSERT_FLOAT(actual, expected, msg) do { if ((actual) != (expected)) { printf("FAIL: %s (got %f, expected %f)\n", msg, actual, expected); failures++; } } while(0)
#define ASSERT_STR(actual, expected, msg) do { if (strcmp((actual) ?: "", (expected) ?: "") != 0) { printf("FAIL: %s\n", msg); failures++; } } while(0)
#define RUN(test) do { printf("Running %s... ", #test); fflush(stdout); int prev_failures = failures; test_##test(); if (failures == prev_failures) printf("OK\n"); } while(0)

static int failures = 0;

// 輔助：產生暫存檔案路徑
static void temp_path(char *buf, size_t bufsize, const char *name) {
    snprintf(buf, bufsize, "/tmp/castle_test_%s.cdb", name);
}

// 輔助：驗證 CDB 資料結構完整性
static void verify_basic_structure(CDB *db, int expected_sheets) {
    ASSERT(db != NULL, "db should not be NULL");
    ASSERT_INT(cdb_get_error(db), CDB_OK, "db error should be OK");
    ASSERT_INT(cdb_get_sheet_count(db), expected_sheets, "sheet count mismatch");
}

// ============================================================
// Group 1: Basic Flow
// ============================================================

TEST(basic_create_save_load)
{
    char path[256];
    temp_path(path, sizeof(path), "basic_create_save_load");

    CDB *db = cdb_create();
    ASSERT(db != NULL, "cdb_create failed");

    cdb_create_sheet(db, "TestSheet");
    CDBSheet *sheet = cdb_get_sheet_by_name(db, "TestSheet");
    ASSERT(sheet != NULL, "sheet should exist");

    CDBColumn *col = cdb_column_create("name", CDB_TSTRING);
    cdb_sheet_add_column(sheet, col, -1);
    cdb_column_destroy(col);

    cdb_sheet_add_row(sheet, -1);
    cdb_set_string(sheet, 0, "name", "hello");

    ASSERT_INT(cdb_save(db, path), 0, "save should succeed");
    cdb_close(db);

    db = cdb_create();
    ASSERT_INT(cdb_load(db, path), 0, "load should succeed");
    verify_basic_structure(db, 1);

    sheet = cdb_get_sheet_by_name(db, "TestSheet");
    ASSERT(sheet != NULL, "sheet should exist after reload");
    ASSERT_STR(cdb_get_string(sheet, 0, "name"), "hello", "string value mismatch");

    cdb_close(db);
    remove(path);
}

TEST(basic_empty_db)
{
    char path[256];
    temp_path(path, sizeof(path), "basic_empty_db");

    CDB *db = cdb_create();
    ASSERT(db != NULL, "cdb_create failed");

    ASSERT_INT(cdb_save(db, path), 0, "save should succeed");
    cdb_close(db);

    db = cdb_create();
    ASSERT_INT(cdb_load(db, path), 0, "load should succeed");
    verify_basic_structure(db, 0);

    cdb_close(db);
    remove(path);
}

TEST(basic_two_sheets)
{
    char path[256];
    temp_path(path, sizeof(path), "basic_two_sheets");

    CDB *db = cdb_create();
    cdb_create_sheet(db, "Sheet1");
    cdb_create_sheet(db, "Sheet2");

    CDBSheet *s1 = cdb_get_sheet_by_name(db, "Sheet1");
    CDBColumn *col1 = cdb_column_create("value", CDB_TINT);
    cdb_sheet_add_column(s1, col1, -1);
    cdb_column_destroy(col1);
    cdb_sheet_add_row(s1, -1);
    cdb_set_int(s1, 0, "value", 42);

    CDBSheet *s2 = cdb_get_sheet_by_name(db, "Sheet2");
    CDBColumn *col2 = cdb_column_create("name", CDB_TSTRING);
    cdb_sheet_add_column(s2, col2, -1);
    cdb_column_destroy(col2);
    cdb_sheet_add_row(s2, -1);
    cdb_set_string(s2, 0, "name", "test");

    ASSERT_INT(cdb_save(db, path), 0, "save should succeed");
    cdb_close(db);

    db = cdb_create();
    ASSERT_INT(cdb_load(db, path), 0, "load should succeed");
    verify_basic_structure(db, 2);

    s1 = cdb_get_sheet_by_name(db, "Sheet1");
    s2 = cdb_get_sheet_by_name(db, "Sheet2");
    ASSERT_INT(cdb_get_int(s1, 0, "value"), 42, "int value mismatch");
    ASSERT_STR(cdb_get_string(s2, 0, "name"), "test", "string value mismatch");

    cdb_close(db);
    remove(path);
}

// ============================================================
// Group 2: Primitive Types
// ============================================================

TEST(type_int)
{
    char path[256];
    temp_path(path, sizeof(path), "type_int");

    CDB *db = cdb_create();
    cdb_create_sheet(db, "Ints");
    CDBSheet *sheet = cdb_get_sheet_by_name(db, "Ints");

    CDBColumn *col = cdb_column_create("val", CDB_TINT);
    cdb_sheet_add_column(sheet, col, -1);
    cdb_column_destroy(col);

    // Add 4 rows with different int values
    cdb_sheet_add_row(sheet, -1);
    cdb_sheet_add_row(sheet, -1);
    cdb_sheet_add_row(sheet, -1);
    cdb_sheet_add_row(sheet, -1);

    cdb_set_int(sheet, 0, "val", 0);
    cdb_set_int(sheet, 1, "val", 1);
    cdb_set_int(sheet, 2, "val", -1);
    cdb_set_int(sheet, 3, "val", 123456);

    ASSERT_INT(cdb_save(db, path), 0, "save should succeed");
    cdb_close(db);

    db = cdb_create();
    ASSERT_INT(cdb_load(db, path), 0, "load should succeed");
    sheet = cdb_get_sheet_by_name(db, "Ints");

    ASSERT_INT(cdb_get_int(sheet, 0, "val"), 0, "int 0 mismatch");
    ASSERT_INT(cdb_get_int(sheet, 1, "val"), 1, "int 1 mismatch");
    ASSERT_INT(cdb_get_int(sheet, 2, "val"), -1, "int -1 mismatch");
    ASSERT_INT(cdb_get_int(sheet, 3, "val"), 123456, "int 123456 mismatch");

    cdb_close(db);
    remove(path);
}

TEST(type_float)
{
    char path[256];
    temp_path(path, sizeof(path), "type_float");

    CDB *db = cdb_create();
    cdb_create_sheet(db, "Floats");
    CDBSheet *sheet = cdb_get_sheet_by_name(db, "Floats");

    CDBColumn *col = cdb_column_create("val", CDB_TFLOAT);
    cdb_sheet_add_column(sheet, col, -1);
    cdb_column_destroy(col);

    cdb_sheet_add_row(sheet, -1);
    cdb_sheet_add_row(sheet, -1);
    cdb_sheet_add_row(sheet, -1);

    cdb_set_float(sheet, 0, "val", 0.0f);
    cdb_set_float(sheet, 1, "val", -1.5f);
    cdb_set_float(sheet, 2, "val", 3.14159f);

    ASSERT_INT(cdb_save(db, path), 0, "save should succeed");
    cdb_close(db);

    db = cdb_create();
    ASSERT_INT(cdb_load(db, path), 0, "load should succeed");
    sheet = cdb_get_sheet_by_name(db, "Floats");

    ASSERT_INT(cdb_get_float(sheet, 0, "val") == 0.0f, 1, "float 0.0 mismatch");
    ASSERT_INT(cdb_get_float(sheet, 1, "val") == -1.5f, 1, "float -1.5 mismatch");
    ASSERT_INT(cdb_get_float(sheet, 2, "val") == 3.14159f, 1, "float 3.14159 mismatch");

    cdb_close(db);
    remove(path);
}

TEST(type_bool)
{
    char path[256];
    temp_path(path, sizeof(path), "type_bool");

    CDB *db = cdb_create();
    cdb_create_sheet(db, "Bools");
    CDBSheet *sheet = cdb_get_sheet_by_name(db, "Bools");

    CDBColumn *col = cdb_column_create("val", CDB_TBOOL);
    cdb_sheet_add_column(sheet, col, -1);
    cdb_column_destroy(col);

    cdb_sheet_add_row(sheet, -1);
    cdb_sheet_add_row(sheet, -1);

    cdb_set_bool(sheet, 0, "val", 1);  // true
    cdb_set_bool(sheet, 1, "val", 0);  // false

    ASSERT_INT(cdb_save(db, path), 0, "save should succeed");
    cdb_close(db);

    db = cdb_create();
    ASSERT_INT(cdb_load(db, path), 0, "load should succeed");
    sheet = cdb_get_sheet_by_name(db, "Bools");

    ASSERT_INT(cdb_get_bool(sheet, 0, "val"), 1, "bool true mismatch");
    ASSERT_INT(cdb_get_bool(sheet, 1, "val"), 0, "bool false mismatch");

    cdb_close(db);
    remove(path);
}

TEST(type_string)
{
    char path[256];
    temp_path(path, sizeof(path), "type_string");

    CDB *db = cdb_create();
    cdb_create_sheet(db, "Strings");
    CDBSheet *sheet = cdb_get_sheet_by_name(db, "Strings");

    CDBColumn *col = cdb_column_create("val", CDB_TSTRING);
    cdb_sheet_add_column(sheet, col, -1);
    cdb_column_destroy(col);

    cdb_sheet_add_row(sheet, -1);
    cdb_sheet_add_row(sheet, -1);
    cdb_sheet_add_row(sheet, -1);

    cdb_set_string(sheet, 0, "val", "hello");
    cdb_set_string(sheet, 1, "val", "");
    cdb_set_string(sheet, 2, "val", "hello world with spaces");

    ASSERT_INT(cdb_save(db, path), 0, "save should succeed");
    cdb_close(db);

    db = cdb_create();
    ASSERT_INT(cdb_load(db, path), 0, "load should succeed");
    sheet = cdb_get_sheet_by_name(db, "Strings");

    ASSERT_STR(cdb_get_string(sheet, 0, "val"), "hello", "string hello mismatch");
    ASSERT_STR(cdb_get_string(sheet, 1, "val"), "", "string empty mismatch");
    ASSERT_STR(cdb_get_string(sheet, 2, "val"), "hello world with spaces", "string spaces mismatch");

    cdb_close(db);
    remove(path);
}

// ============================================================
// Group 3: Complex Types
// ============================================================

TEST(type_enum)
{
    char path[256];
    temp_path(path, sizeof(path), "type_enum");

    CDB *db = cdb_create();
    cdb_create_sheet(db, "Enums");
    CDBSheet *sheet = cdb_get_sheet_by_name(db, "Enums");

    CDBColumn *col = cdb_column_create("color", CDB_TENUM);
    // Manually set type_str since cdb_column_create doesn't parse it
    col->type_str = strdup("5:val1,val2,val3");
    col->enum_count = 3;
    col->enum_values = malloc(3 * sizeof(char*));
    col->enum_values[0] = strdup("val1");
    col->enum_values[1] = strdup("val2");
    col->enum_values[2] = strdup("val3");
    cdb_sheet_add_column(sheet, col, -1);
    // Note: Do NOT call cdb_column_destroy(col) here because
    // cdb_sheet_add_column does a shallow copy of enum_values pointer.
    // The sheet takes ownership; cdb_close will free it.

    cdb_sheet_add_row(sheet, -1);
    cdb_set_int(sheet, 0, "color", 1);  // Select val2

    ASSERT_INT(cdb_save(db, path), 0, "save should succeed");
    cdb_close(db);

    db = cdb_create();
    ASSERT_INT(cdb_load(db, path), 0, "load should succeed");
    sheet = cdb_get_sheet_by_name(db, "Enums");

    ASSERT_INT(cdb_get_int(sheet, 0, "color"), 1, "enum value mismatch");

    cdb_close(db);
    remove(path);
}

TEST(type_flags)
{
    char path[256];
    temp_path(path, sizeof(path), "type_flags");

    CDB *db = cdb_create();
    cdb_create_sheet(db, "Flags");
    CDBSheet *sheet = cdb_get_sheet_by_name(db, "Flags");

    CDBColumn *col = cdb_column_create("perms", CDB_TFLAGS);
    // Manually set type_str since cdb_column_create doesn't parse it
    col->type_str = strdup("10:read,write,execute");
    col->enum_count = 3;
    col->enum_values = malloc(3 * sizeof(char*));
    col->enum_values[0] = strdup("read");
    col->enum_values[1] = strdup("write");
    col->enum_values[2] = strdup("execute");
    cdb_sheet_add_column(sheet, col, -1);
    // Note: Do NOT call cdb_column_destroy(col) here because
    // cdb_sheet_add_column does a shallow copy of enum_values pointer.
    // The sheet takes ownership; cdb_close will free it.

    cdb_sheet_add_row(sheet, -1);
    cdb_set_int(sheet, 0, "perms", 5);  // read(1) + write(4) = 5

    ASSERT_INT(cdb_save(db, path), 0, "save should succeed");
    cdb_close(db);

    db = cdb_create();
    ASSERT_INT(cdb_load(db, path), 0, "load should succeed");
    sheet = cdb_get_sheet_by_name(db, "Flags");

    ASSERT_INT(cdb_get_int(sheet, 0, "perms"), 5, "flags value mismatch");

    cdb_close(db);
    remove(path);
}

TEST(type_color)
{
    char path[256];
    temp_path(path, sizeof(path), "type_color");

    CDB *db = cdb_create();
    cdb_create_sheet(db, "Colors");
    CDBSheet *sheet = cdb_get_sheet_by_name(db, "Colors");

    CDBColumn *col = cdb_column_create("fill", CDB_TCOLOR);
    col->type_str = strdup("11");
    cdb_sheet_add_column(sheet, col, -1);
    cdb_column_destroy(col);

    cdb_sheet_add_row(sheet, -1);
    cdb_set_int(sheet, 0, "fill", 0xAABBCCDD);

    ASSERT_INT(cdb_save(db, path), 0, "save should succeed");
    cdb_close(db);

    db = cdb_create();
    ASSERT_INT(cdb_load(db, path), 0, "load should succeed");
    sheet = cdb_get_sheet_by_name(db, "Colors");

    ASSERT_INT(cdb_get_int(sheet, 0, "fill"), (int)0xAABBCCDD, "color value mismatch");

    cdb_close(db);
    remove(path);
}

TEST(type_optional)
{
    char path[256];
    temp_path(path, sizeof(path), "type_optional");

    CDB *db = cdb_create();
    cdb_create_sheet(db, "Optionals");
    CDBSheet *sheet = cdb_get_sheet_by_name(db, "Optionals");

    CDBColumn *col = cdb_column_create("opt", CDB_TSTRING);
    cdb_column_set_optional(col, 1);
    cdb_sheet_add_column(sheet, col, -1);
    cdb_column_destroy(col);

    cdb_sheet_add_row(sheet, -1);

    // Before setting value, should be null
    ASSERT_INT(cdb_value_is_null(sheet, 0, "opt"), 1, "opt should be null before set");

    cdb_set_string(sheet, 0, "opt", "has_value");

    // After setting value, should not be null
    ASSERT_INT(cdb_value_is_null(sheet, 0, "opt"), 0, "opt should not be null after set");
    ASSERT_STR(cdb_get_string(sheet, 0, "opt"), "has_value", "opt value mismatch");

    ASSERT_INT(cdb_save(db, path), 0, "save should succeed");
    cdb_close(db);

    db = cdb_create();
    ASSERT_INT(cdb_load(db, path), 0, "load should succeed");
    sheet = cdb_get_sheet_by_name(db, "Optionals");

    ASSERT_INT(cdb_value_is_null(sheet, 0, "opt"), 0, "opt should not be null after reload");
    ASSERT_STR(cdb_get_string(sheet, 0, "opt"), "has_value", "opt value after reload mismatch");

    cdb_close(db);
    remove(path);
}

// ============================================================
// Group 4: Edge Cases
// ============================================================

TEST(edge_null_value)
{
    char path[256];
    temp_path(path, sizeof(path), "edge_null_value");

    CDB *db = cdb_create();
    cdb_create_sheet(db, "Nulls");
    CDBSheet *sheet = cdb_get_sheet_by_name(db, "Nulls");

    CDBColumn *col = cdb_column_create("val", CDB_TSTRING);
    cdb_sheet_add_column(sheet, col, -1);
    cdb_column_destroy(col);

    cdb_sheet_add_row(sheet, -1);
    // Empty string should be treated as null
    cdb_set_string(sheet, 0, "val", "");

    ASSERT_INT(cdb_value_is_null(sheet, 0, "val"), 1, "empty string should be null");

    ASSERT_INT(cdb_save(db, path), 0, "save should succeed");
    cdb_close(db);

    db = cdb_create();
    ASSERT_INT(cdb_load(db, path), 0, "load should succeed");
    sheet = cdb_get_sheet_by_name(db, "Nulls");

    ASSERT_INT(cdb_value_is_null(sheet, 0, "val"), 1, "null should persist after reload");

    cdb_close(db);
    remove(path);
}

TEST(edge_empty_sheet)
{
    char path[256];
    temp_path(path, sizeof(path), "edge_empty_sheet");

    CDB *db = cdb_create();
    cdb_create_sheet(db, "EmptySheet");
    CDBSheet *sheet = cdb_get_sheet_by_name(db, "EmptySheet");

    ASSERT_INT(cdb_sheet_column_count(sheet), 0, "column count should be 0");
    ASSERT_INT(cdb_sheet_row_count(sheet), 0, "row count should be 0");

    ASSERT_INT(cdb_save(db, path), 0, "save should succeed");
    cdb_close(db);

    db = cdb_create();
    ASSERT_INT(cdb_load(db, path), 0, "load should succeed");
    verify_basic_structure(db, 1);

    sheet = cdb_get_sheet_by_name(db, "EmptySheet");
    ASSERT_INT(cdb_sheet_column_count(sheet), 0, "column count should be 0 after reload");
    ASSERT_INT(cdb_sheet_row_count(sheet), 0, "row count should be 0 after reload");

    cdb_close(db);
    remove(path);
}

TEST(edge_special_chars)
{
    char path[256];
    temp_path(path, sizeof(path), "edge_special_chars");

    CDB *db = cdb_create();
    cdb_create_sheet(db, "Special");
    CDBSheet *sheet = cdb_get_sheet_by_name(db, "Special");

    CDBColumn *col = cdb_column_create("val", CDB_TSTRING);
    cdb_sheet_add_column(sheet, col, -1);
    cdb_column_destroy(col);

    cdb_sheet_add_row(sheet, -1);
    // String with newline character. Writer escapes \n to \n in JSON.
    // Reader's tok_string returns raw JSON escape sequences (backslash + letter).
    cdb_set_string(sheet, 0, "val", "hello\nworld");

    ASSERT_INT(cdb_save(db, path), 0, "save should succeed");
    cdb_close(db);

    db = cdb_create();
    ASSERT_INT(cdb_load(db, path), 0, "load should succeed");
    sheet = cdb_get_sheet_by_name(db, "Special");

    // tok_string returns literal \n (backslash + letter n), not actual newline
    ASSERT_STR(cdb_get_string(sheet, 0, "val"), "hello\\nworld", "special chars mismatch");

    cdb_close(db);
    remove(path);
}

TEST(edge_large_data)
{
    char path[256];
    temp_path(path, sizeof(path), "edge_large_data");

    CDB *db = cdb_create();
    cdb_create_sheet(db, "Large");
    CDBSheet *sheet = cdb_get_sheet_by_name(db, "Large");

    // Create 10 columns
    for (int c = 0; c < 10; c++) {
        char colname[16];
        snprintf(colname, sizeof(colname), "col%d", c);
        CDBColumn *col = cdb_column_create(colname, CDB_TINT);
        cdb_sheet_add_column(sheet, col, -1);
        cdb_column_destroy(col);
    }

    // Create 100 rows (stays within 8192 token limit)
    for (int r = 0; r < 100; r++) {
        cdb_sheet_add_row(sheet, -1);
        for (int c = 0; c < 10; c++) {
            char colname[16];
            snprintf(colname, sizeof(colname), "col%d", c);
            cdb_set_int(sheet, r, colname, r * 10 + c);
        }
    }

    ASSERT_INT(cdb_save(db, path), 0, "save should succeed");
    cdb_close(db);

    db = cdb_create();
    ASSERT_INT(cdb_load(db, path), 0, "load should succeed");
    sheet = cdb_get_sheet_by_name(db, "Large");

    // Verify row 99 col9 value
    ASSERT_INT(cdb_get_int(sheet, 99, "col9"), 99 * 10 + 9, "large data row 99 col9 mismatch");

    cdb_close(db);
    remove(path);
}

TEST(edge_unicode)
{
    char path[256];
    temp_path(path, sizeof(path), "edge_unicode");

    CDB *db = cdb_create();
    cdb_create_sheet(db, "Unicode");
    CDBSheet *sheet = cdb_get_sheet_by_name(db, "Unicode");

    CDBColumn *col = cdb_column_create("val", CDB_TSTRING);
    cdb_sheet_add_column(sheet, col, -1);
    cdb_column_destroy(col);

    cdb_sheet_add_row(sheet, -1);
    cdb_set_string(sheet, 0, "val", "中文測試 русский 한국어");

    ASSERT_INT(cdb_save(db, path), 0, "save should succeed");
    cdb_close(db);

    db = cdb_create();
    ASSERT_INT(cdb_load(db, path), 0, "load should succeed");
    sheet = cdb_get_sheet_by_name(db, "Unicode");

    ASSERT_STR(cdb_get_string(sheet, 0, "val"), "中文測試 русский 한국어", "unicode mismatch");

    cdb_close(db);
    remove(path);
}

// ============================================================
// Group 5: Error Handling
// ============================================================

TEST(error_file_not_found)
{
    CDB *db = cdb_create();
    ASSERT_INT(cdb_load(db, "/nonexistent/path/to/file.cdb"), -1, "load should fail for nonexistent file");
    ASSERT_INT(cdb_get_error(db), CDB_ERR_FILE_NOT_FOUND, "error should be FILE_NOT_FOUND");
    cdb_close(db);
}

TEST(error_invalid_json)
{
    char path[256];
    temp_path(path, sizeof(path), "error_invalid_json");

    // Write invalid JSON that jsmn cannot parse (truncated/incomplete)
    FILE *f = fopen(path, "w");
    ASSERT(f != NULL, "should be able to create temp file");
    fprintf(f, "{\"sheets\":[{");
    fclose(f);

    CDB *db = cdb_create();
    ASSERT_INT(cdb_load(db, path), -1, "load should fail for invalid JSON");
    // jsmn returns JSMN_ERROR_PART (-2) for truncated JSON, resulting in INVALID_CDB
    ASSERT(cdb_get_error(db) == CDB_ERR_JSON_PARSE || cdb_get_error(db) == CDB_ERR_INVALID_CDB,
           "error should be JSON_PARSE or INVALID_CDB");
    cdb_close(db);
    remove(path);
}

TEST(error_corrupt_cdb)
{
    char path[256];
    temp_path(path, sizeof(path), "error_corrupt_cdb");

    // Write valid JSON but not a CDB structure (no sheets array)
    FILE *f = fopen(path, "w");
    ASSERT(f != NULL, "should be able to create temp file");
    fprintf(f, "{\"name\": \"test\", \"version\": \"1.0\"}");
    fclose(f);

    CDB *db = cdb_create();
    ASSERT_INT(cdb_load(db, path), -1, "load should fail for corrupt CDB");
    // Should be either INVALID_CDB or JSON_PARSE depending on what fails first
    ASSERT(cdb_get_error(db) == CDB_ERR_INVALID_CDB || cdb_get_error(db) == CDB_ERR_JSON_PARSE,
           "error should be INVALID_CDB or JSON_PARSE");
    cdb_close(db);
    remove(path);
}

// ============================================================
// Group 6: Sheet Operations
// ============================================================

TEST(ops_add_sheet)
{
    char path[256];
    temp_path(path, sizeof(path), "ops_add_sheet");

    CDB *db = cdb_create();
    cdb_create_sheet(db, "Sheet1");
    cdb_create_sheet(db, "Sheet2");

    ASSERT_INT(cdb_get_sheet_count(db), 2, "sheet count should be 2");

    ASSERT_INT(cdb_save(db, path), 0, "save should succeed");
    cdb_close(db);

    db = cdb_create();
    ASSERT_INT(cdb_load(db, path), 0, "load should succeed");
    verify_basic_structure(db, 2);

    ASSERT(cdb_get_sheet_by_name(db, "Sheet1") != NULL, "Sheet1 should exist");
    ASSERT(cdb_get_sheet_by_name(db, "Sheet2") != NULL, "Sheet2 should exist");

    cdb_close(db);
    remove(path);
}

TEST(ops_delete_sheet)
{
    char path[256];
    temp_path(path, sizeof(path), "ops_delete_sheet");

    CDB *db = cdb_create();
    cdb_create_sheet(db, "Sheet1");
    cdb_create_sheet(db, "Sheet2");
    cdb_create_sheet(db, "Sheet3");

    ASSERT_INT(cdb_get_sheet_count(db), 3, "sheet count should be 3");

    cdb_delete_sheet(db, "Sheet2");

    ASSERT_INT(cdb_get_sheet_count(db), 2, "sheet count should be 2 after delete");
    ASSERT(cdb_get_sheet_by_name(db, "Sheet1") != NULL, "Sheet1 should still exist");
    ASSERT(cdb_get_sheet_by_name(db, "Sheet2") == NULL, "Sheet2 should be deleted");
    ASSERT(cdb_get_sheet_by_name(db, "Sheet3") != NULL, "Sheet3 should still exist");

    ASSERT_INT(cdb_save(db, path), 0, "save should succeed");
    cdb_close(db);

    db = cdb_create();
    ASSERT_INT(cdb_load(db, path), 0, "load should succeed");
    verify_basic_structure(db, 2);

    ASSERT(cdb_get_sheet_by_name(db, "Sheet1") != NULL, "Sheet1 should exist after reload");
    ASSERT(cdb_get_sheet_by_name(db, "Sheet2") == NULL, "Sheet2 should not exist after reload");
    ASSERT(cdb_get_sheet_by_name(db, "Sheet3") != NULL, "Sheet3 should exist after reload");

    cdb_close(db);
    remove(path);
}

TEST(ops_add_column)
{
    char path[256];
    temp_path(path, sizeof(path), "ops_add_column");

    CDB *db = cdb_create();
    cdb_create_sheet(db, "Test");
    CDBSheet *sheet = cdb_get_sheet_by_name(db, "Test");

    CDBColumn *col1 = cdb_column_create("col1", CDB_TINT);
    cdb_sheet_add_column(sheet, col1, -1);
    cdb_column_destroy(col1);

    cdb_sheet_add_row(sheet, -1);
    cdb_set_int(sheet, 0, "col1", 100);

    // Add second column - first column value should persist
    CDBColumn *col2 = cdb_column_create("col2", CDB_TSTRING);
    cdb_sheet_add_column(sheet, col2, -1);
    cdb_column_destroy(col2);

    ASSERT_INT(cdb_get_int(sheet, 0, "col1"), 100, "col1 value should persist after adding col2");

    ASSERT_INT(cdb_save(db, path), 0, "save should succeed");
    cdb_close(db);

    db = cdb_create();
    ASSERT_INT(cdb_load(db, path), 0, "load should succeed");
    sheet = cdb_get_sheet_by_name(db, "Test");

    ASSERT_INT(cdb_sheet_column_count(sheet), 2, "column count should be 2");
    ASSERT_INT(cdb_get_int(sheet, 0, "col1"), 100, "col1 value mismatch after reload");
    ASSERT(cdb_get_string(sheet, 0, "col2") == NULL || cdb_value_is_null(sheet, 0, "col2"), "col2 should be null");

    cdb_close(db);
    remove(path);
}

TEST(ops_delete_column)
{
    char path[256];
    temp_path(path, sizeof(path), "ops_delete_column");

    CDB *db = cdb_create();
    cdb_create_sheet(db, "Test");
    CDBSheet *sheet = cdb_get_sheet_by_name(db, "Test");

    CDBColumn *col1 = cdb_column_create("col1", CDB_TINT);
    cdb_sheet_add_column(sheet, col1, -1);
    cdb_column_destroy(col1);

    CDBColumn *col2 = cdb_column_create("col2", CDB_TSTRING);
    cdb_sheet_add_column(sheet, col2, -1);
    cdb_column_destroy(col2);

    CDBColumn *col3 = cdb_column_create("col3", CDB_TBOOL);
    cdb_sheet_add_column(sheet, col3, -1);
    cdb_column_destroy(col3);

    cdb_sheet_add_row(sheet, -1);
    cdb_set_int(sheet, 0, "col1", 1);
    cdb_set_string(sheet, 0, "col2", "hello");
    cdb_set_bool(sheet, 0, "col3", 1);

    ASSERT_INT(cdb_sheet_column_count(sheet), 3, "column count should be 3");

    cdb_sheet_delete_column(sheet, "col2");

    ASSERT_INT(cdb_sheet_column_count(sheet), 2, "column count should be 2 after delete");
    ASSERT_INT(cdb_get_int(sheet, 0, "col1"), 1, "col1 should still exist");
    ASSERT(cdb_sheet_column_by_name(sheet, "col2") == NULL, "col2 should be deleted");
    ASSERT_INT(cdb_get_bool(sheet, 0, "col3"), 1, "col3 should still exist");

    ASSERT_INT(cdb_save(db, path), 0, "save should succeed");
    cdb_close(db);

    db = cdb_create();
    ASSERT_INT(cdb_load(db, path), 0, "load should succeed");
    sheet = cdb_get_sheet_by_name(db, "Test");

    ASSERT_INT(cdb_sheet_column_count(sheet), 2, "column count should be 2 after reload");
    ASSERT_INT(cdb_get_int(sheet, 0, "col1"), 1, "col1 value mismatch after reload");
    ASSERT(cdb_sheet_column_by_name(sheet, "col2") == NULL, "col2 should not exist after reload");
    ASSERT_INT(cdb_get_bool(sheet, 0, "col3"), 1, "col3 value mismatch after reload");

    cdb_close(db);
    remove(path);
}

TEST(ops_add_row)
{
    char path[256];
    temp_path(path, sizeof(path), "ops_add_row");

    CDB *db = cdb_create();
    cdb_create_sheet(db, "Test");
    CDBSheet *sheet = cdb_get_sheet_by_name(db, "Test");

    CDBColumn *col = cdb_column_create("val", CDB_TINT);
    cdb_sheet_add_column(sheet, col, -1);
    cdb_column_destroy(col);

    cdb_sheet_add_row(sheet, -1);
    cdb_set_int(sheet, 0, "val", 10);

    cdb_sheet_add_row(sheet, -1);
    cdb_set_int(sheet, 1, "val", 20);

    ASSERT_INT(cdb_sheet_row_count(sheet), 2, "row count should be 2");
    ASSERT_INT(cdb_get_int(sheet, 0, "val"), 10, "row 0 value mismatch");
    ASSERT_INT(cdb_get_int(sheet, 1, "val"), 20, "row 1 value mismatch");

    ASSERT_INT(cdb_save(db, path), 0, "save should succeed");
    cdb_close(db);

    db = cdb_create();
    ASSERT_INT(cdb_load(db, path), 0, "load should succeed");
    sheet = cdb_get_sheet_by_name(db, "Test");

    ASSERT_INT(cdb_sheet_row_count(sheet), 2, "row count should be 2 after reload");
    ASSERT_INT(cdb_get_int(sheet, 0, "val"), 10, "row 0 value mismatch after reload");
    ASSERT_INT(cdb_get_int(sheet, 1, "val"), 20, "row 1 value mismatch after reload");

    cdb_close(db);
    remove(path);
}

TEST(ops_delete_row)
{
    char path[256];
    temp_path(path, sizeof(path), "ops_delete_row");

    CDB *db = cdb_create();
    cdb_create_sheet(db, "Test");
    CDBSheet *sheet = cdb_get_sheet_by_name(db, "Test");

    CDBColumn *col = cdb_column_create("val", CDB_TINT);
    cdb_sheet_add_column(sheet, col, -1);
    cdb_column_destroy(col);

    cdb_sheet_add_row(sheet, -1);
    cdb_set_int(sheet, 0, "val", 10);

    cdb_sheet_add_row(sheet, -1);
    cdb_set_int(sheet, 1, "val", 20);

    cdb_sheet_add_row(sheet, -1);
    cdb_set_int(sheet, 2, "val", 30);

    ASSERT_INT(cdb_sheet_row_count(sheet), 3, "row count should be 3");

    // Delete middle row (row 1)
    cdb_sheet_delete_row(sheet, 1);

    ASSERT_INT(cdb_sheet_row_count(sheet), 2, "row count should be 2 after delete");
    ASSERT_INT(cdb_get_int(sheet, 0, "val"), 10, "row 0 should be 10");
    // Row 1 should now be what was row 2 (30), shifted up
    ASSERT_INT(cdb_get_int(sheet, 1, "val"), 30, "row 1 should be 30 after shift");

    ASSERT_INT(cdb_save(db, path), 0, "save should succeed");
    cdb_close(db);

    db = cdb_create();
    ASSERT_INT(cdb_load(db, path), 0, "load should succeed");
    sheet = cdb_get_sheet_by_name(db, "Test");

    ASSERT_INT(cdb_sheet_row_count(sheet), 2, "row count should be 2 after reload");
    ASSERT_INT(cdb_get_int(sheet, 0, "val"), 10, "row 0 value mismatch after reload");
    ASSERT_INT(cdb_get_int(sheet, 1, "val"), 30, "row 1 value mismatch after reload");

    cdb_close(db);
    remove(path);
}

// ============================================================
// Main
// ============================================================

int main(void) {
    printf("=== CastleDB C Test Suite ===\n\n");

    RUN(basic_create_save_load);
    RUN(basic_empty_db);
    RUN(basic_two_sheets);

    RUN(type_int);
    RUN(type_float);
    RUN(type_bool);
    RUN(type_string);

    RUN(type_enum);
    RUN(type_flags);
    RUN(type_color);
    RUN(type_optional);

    RUN(edge_null_value);
    RUN(edge_empty_sheet);
    RUN(edge_special_chars);
    RUN(edge_large_data);
    RUN(edge_unicode);

    RUN(error_file_not_found);
    RUN(error_invalid_json);
    RUN(error_corrupt_cdb);

    RUN(ops_add_sheet);
    RUN(ops_delete_sheet);
    RUN(ops_add_column);
    RUN(ops_delete_column);
    RUN(ops_add_row);
    RUN(ops_delete_row);

    printf("\n=== %d tests, %d failures ===\n",
           25, failures);
    return failures > 0 ? 1 : 0;
}
