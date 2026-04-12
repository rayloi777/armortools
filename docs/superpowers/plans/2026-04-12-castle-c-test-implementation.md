# CastleDB C 測試程式實作計畫

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 建立獨立測試程式 `test_cdb.c`，驗證 castle_c library 的 cdb_save() / cdb_load() 往返正確。

**Architecture:** 純 C 測試，使用自製 minimal unit test 框架（巨集 `ASSERT`、`TEST`），無外部依賴。測試個體隔離，每個 TEST function 創建/銷毀自己的 CDB instance。

**Tech Stack:** C99、標準 C library（stdio/stdlib/string）、JSMN、 castle_c sources

---

## 檔案結構

```
castle_c/test/
├── test_cdb.c    # 主測試程式（框架 + 23 個測試案例）
└── Makefile      # 編譯腳本
```

**依賴關係：**
```
test_cdb.c
├── cdb.h (castle_c/sources/lib/cdb.h)
├── cdb_internal.h (castle_c/sources/lib/cdb_internal.h)
├── cdb.c (castle_c/sources/lib/cdb.c)
├── cdb_reader.c (castle_c/sources/lib/cdb_reader.c)
├── cdb_writer.c (castle_c/sources/lib/cdb_writer.c)
└── jsmn.h + jsmn.c (base/sources/libs/jsmn.h + jsmn.c)
```

---

## Task 1: 建立測試目錄

**Files:**
- Create: `castle_c/test/` 目錄

- [ ] **Step 1: 建立目錄**

```bash
mkdir -p castle_c/test
```

---

## Task 2: 撰寫測試框架巨集和輔助函數

**Files:**
- Modify: `castle_c/test/test_cdb.c`（新建，全域框架）

- [ ] **Step 1: 撰寫框架巨集和測試框架**

```c
/*
 * CastleDB C Library - Test Suite
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../../sources/lib/cdb.h"
#include "../../sources/lib/cdb_internal.h"

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
```

---

## Task 3: 撰寫 Group 1 - 基本流程測試（3 案例）

**Files:**
- Modify: `castle_c/test/test_cdb.c`

- [ ] **Step 1: 撰寫 basic_create_save_load**

```c
TEST(basic_create_save_load) {
    CDB *db = cdb_create();
    ASSERT(db != NULL, "cdb_create failed");

    // Create sheet
    ASSERT_INT(cdb_create_sheet(db, "TestSheet"), 0, "create_sheet failed");
    CDBSheet *sheet = cdb_get_sheet_by_name(db, "TestSheet");
    ASSERT(sheet != NULL, "get_sheet_by_name failed");

    // Add column
    CDBColumn *col = cdb_column_create("name", CDB_TSTRING);
    ASSERT_INT(cdb_sheet_add_column(sheet, col, -1), 0, "add_column failed");

    // Add row
    ASSERT_INT(cdb_sheet_add_row(sheet, -1), 0, "add_row failed");

    // Set value
    ASSERT_INT(cdb_set_string(sheet, 0, "name", "Hello World"), 0, "set_string failed");

    // Save
    char path[256];
    temp_path(path, sizeof(path), "basic_create");
    ASSERT_INT(cdb_save(db, path), 0, "cdb_save failed");

    // Load
    CDB *db2 = cdb_create();
    ASSERT_INT(cdb_load(db2, path), 0, "cdb_load failed");

    // Verify
    ASSERT_INT(cdb_get_sheet_count(db2), 1, "sheet count after load mismatch");
    CDBSheet *sheet2 = cdb_get_sheet_by_name(db2, "TestSheet");
    ASSERT(sheet2 != NULL, "sheet not found after load");
    ASSERT_STR(cdb_get_string(sheet2, 0, "name"), "Hello World", "value mismatch after load");

    // Cleanup
    cdb_close(db2);
    cdb_close(db);
    remove(path);
}
```

- [ ] **Step 2: 撰寫 basic_empty_db**

```c
TEST(basic_empty_db) {
    CDB *db = cdb_create();
    ASSERT(db != NULL, "cdb_create failed");
    ASSERT_INT(cdb_get_sheet_count(db), 0, "empty db should have 0 sheets");

    char path[256];
    temp_path(path, sizeof(path), "basic_empty");
    ASSERT_INT(cdb_save(db, path), 0, "cdb_save empty db failed");

    CDB *db2 = cdb_create();
    ASSERT_INT(cdb_load(db2, path), 0, "cdb_load empty db failed");
    ASSERT_INT(cdb_get_sheet_count(db2), 0, "loaded empty db should have 0 sheets");

    cdb_close(db2);
    cdb_close(db);
    remove(path);
}
```

- [ ] **Step 3: 撰寫 basic_two_sheets**

```c
TEST(basic_two_sheets) {
    CDB *db = cdb_create();
    cdb_create_sheet(db, "Sheet1");
    cdb_create_sheet(db, "Sheet2");

    CDBSheet *s1 = cdb_get_sheet_by_name(db, "Sheet1");
    CDBSheet *s2 = cdb_get_sheet_by_name(db, "Sheet2");
    CDBColumn *col = cdb_column_create("id", CDB_TINT);
    cdb_sheet_add_column(s1, col, -1);
    col = cdb_column_create("name", CDB_TSTRING);
    cdb_sheet_add_column(s2, col, -1);

    cdb_sheet_add_row(s1, -1);
    cdb_sheet_add_row(s2, -1);
    cdb_set_int(s1, 0, "id", 42);
    cdb_set_string(s2, 0, "name", "Two Sheets");

    char path[256];
    temp_path(path, sizeof(path), "basic_two_sheets");
    cdb_save(db, path);

    CDB *db2 = cdb_create();
    cdb_load(db2, path);

    CDBSheet *s1r = cdb_get_sheet_by_name(db2, "Sheet1");
    CDBSheet *s2r = cdb_get_sheet_by_name(db2, "Sheet2");
    ASSERT_INT(cdb_get_int(s1r, 0, "id"), 42, "Sheet1 id mismatch");
    ASSERT_STR(cdb_get_string(s2r, 0, "name"), "Two Sheets", "Sheet2 name mismatch");

    cdb_close(db2);
    cdb_close(db);
    remove(path);
}
```

---

## Task 4: 撰寫 Group 2 - 基本類型測試（4 案例）

**Files:**
- Modify: `castle_c/test/test_cdb.c`

- [ ] **Step 1: 撰寫 type_int**

```c
TEST(type_int) {
    CDB *db = cdb_create();
    CDBSheet *sheet;
    cdb_create_sheet(db, "Sheet");
    sheet = cdb_get_sheet_by_name(db, "Sheet");
    CDBColumn *col = cdb_column_create("val", CDB_TINT);
    cdb_sheet_add_column(sheet, col, -1);
    cdb_sheet_add_row(sheet, -1);

    // Test values
    cdb_set_int(sheet, 0, "val", 0);
    ASSERT_INT(cdb_get_int(sheet, 0, "val"), 0, "int 0 mismatch");
    cdb_set_int(sheet, 0, "val", 1);
    ASSERT_INT(cdb_get_int(sheet, 0, "val"), 1, "int 1 mismatch");
    cdb_set_int(sheet, 0, "val", -1);
    ASSERT_INT(cdb_get_int(sheet, 0, "val"), -1, "int -1 mismatch");
    cdb_set_int(sheet, 0, "val", 123456);
    ASSERT_INT(cdb_get_int(sheet, 0, "val"), 123456, "int 123456 mismatch");

    // Save and reload
    char path[256];
    temp_path(path, sizeof(path), "type_int");
    cdb_save(db, path);
    CDB *db2 = cdb_create();
    cdb_load(db2, path);
    CDBSheet *sheet2 = cdb_get_sheet_by_name(db2, "Sheet");
    ASSERT_INT(cdb_get_int(sheet2, 0, "val"), 123456, "int after reload mismatch");

    cdb_close(db2);
    cdb_close(db);
    remove(path);
}
```

- [ ] **Step 2: 撰寫 type_float**

```c
TEST(type_float) {
    CDB *db = cdb_create();
    CDBSheet *sheet;
    cdb_create_sheet(db, "Sheet");
    sheet = cdb_get_sheet_by_name(db, "Sheet");
    CDBColumn *col = cdb_column_create("val", CDB_TFLOAT);
    cdb_sheet_add_column(sheet, col, -1);
    cdb_sheet_add_row(sheet, -1);

    cdb_set_float(sheet, 0, "val", 0.0f);
    ASSERT_INT(cdb_get_float(sheet, 0, "val") == 0.0f, 1, "float 0.0 mismatch");
    cdb_set_float(sheet, 0, "val", -1.5f);
    ASSERT_INT(cdb_get_float(sheet, 0, "val") == -1.5f, 1, "float -1.5 mismatch");
    cdb_set_float(sheet, 0, "val", 3.14159f);
    ASSERT_INT(cdb_get_float(sheet, 0, "val") == 3.14159f, 1, "float pi mismatch");

    char path[256];
    temp_path(path, sizeof(path), "type_float");
    cdb_save(db, path);
    CDB *db2 = cdb_create();
    cdb_load(db2, path);
    CDBSheet *sheet2 = cdb_get_sheet_by_name(db2, "Sheet");
    ASSERT_INT(cdb_get_float(sheet2, 0, "val") == 3.14159f, 1, "float after reload mismatch");

    cdb_close(db2);
    cdb_close(db);
    remove(path);
}
```

- [ ] **Step 3: 撰寫 type_bool**

```c
TEST(type_bool) {
    CDB *db = cdb_create();
    CDBSheet *sheet;
    cdb_create_sheet(db, "Sheet");
    sheet = cdb_get_sheet_by_name(db, "Sheet");
    CDBColumn *col = cdb_column_create("val", CDB_TBOOL);
    cdb_sheet_add_column(sheet, col, -1);
    cdb_sheet_add_row(sheet, -1);

    cdb_set_bool(sheet, 0, "val", 1);
    ASSERT_INT(cdb_get_bool(sheet, 0, "val"), 1, "bool true mismatch");
    cdb_set_bool(sheet, 0, "val", 0);
    ASSERT_INT(cdb_get_bool(sheet, 0, "val"), 0, "bool false mismatch");

    char path[256];
    temp_path(path, sizeof(path), "type_bool");
    cdb_save(db, path);
    CDB *db2 = cdb_create();
    cdb_load(db2, path);
    CDBSheet *sheet2 = cdb_get_sheet_by_name(db2, "Sheet");
    ASSERT_INT(cdb_get_bool(sheet2, 0, "val"), 0, "bool after reload mismatch");

    cdb_close(db2);
    cdb_close(db);
    remove(path);
}
```

- [ ] **Step 4: 撰寫 type_string**

```c
TEST(type_string) {
    CDB *db = cdb_create();
    CDBSheet *sheet;
    cdb_create_sheet(db, "Sheet");
    sheet = cdb_get_sheet_by_name(db, "Sheet");
    CDBColumn *col = cdb_column_create("val", CDB_TSTRING);
    cdb_sheet_add_column(sheet, col, -1);
    cdb_sheet_add_row(sheet, -1);

    cdb_set_string(sheet, 0, "val", "hello");
    ASSERT_STR(cdb_get_string(sheet, 0, "val"), "hello", "string hello mismatch");
    cdb_set_string(sheet, 0, "val", "");
    ASSERT_STR(cdb_get_string(sheet, 0, "val"), "", "empty string mismatch");
    cdb_set_string(sheet, 0, "val", "hello world with spaces");
    ASSERT_STR(cdb_get_string(sheet, 0, "val"), "hello world with spaces", "string with spaces mismatch");

    char path[256];
    temp_path(path, sizeof(path), "type_string");
    cdb_save(db, path);
    CDB *db2 = cdb_create();
    cdb_load(db2, path);
    CDBSheet *sheet2 = cdb_get_sheet_by_name(db2, "Sheet");
    ASSERT_STR(cdb_get_string(sheet2, 0, "val"), "hello world with spaces", "string after reload mismatch");

    cdb_close(db2);
    cdb_close(db);
    remove(path);
}
```

---

## Task 5: 撰寫 Group 3 - 複雜類型測試（4 案例）

**Files:**
- Modify: `castle_c/test/test_cdb.c`

- [ ] **Step 1: 撰寫 type_enum**

```c
TEST(type_enum) {
    CDB *db = cdb_create();
    CDBSheet *sheet;
    cdb_create_sheet(db, "Sheet");
    sheet = cdb_get_sheet_by_name(db, "Sheet");

    // Column with typeStr = "5:val1,val2,val3" (5 = CDB_TENUM)
    CDBColumn *col = cdb_column_create("status", CDB_TENUM);
    col->type_str = strdup("5:val1,val2,val3");
    // Parse enum values
    col->enum_count = 3;
    col->enum_values = malloc(3 * sizeof(char*));
    col->enum_values[0] = strdup("val1");
    col->enum_values[1] = strdup("val2");
    col->enum_values[2] = strdup("val3");
    cdb_sheet_add_column(sheet, col, -1);
    cdb_sheet_add_row(sheet, -1);

    cdb_set_int(sheet, 0, "status", 1);  // Set to val2
    ASSERT_INT(cdb_get_int(sheet, 0, "status"), 1, "enum value mismatch");

    char path[256];
    temp_path(path, sizeof(path), "type_enum");
    cdb_save(db, path);
    CDB *db2 = cdb_create();
    cdb_load(db2, path);
    CDBSheet *sheet2 = cdb_get_sheet_by_name(db2, "Sheet");
    ASSERT_INT(cdb_get_int(sheet2, 0, "status"), 1, "enum after reload mismatch");

    cdb_close(db2);
    cdb_close(db);
    remove(path);
}
```

- [ ] **Step 2: 撰寫 type_flags**

```c
TEST(type_flags) {
    CDB *db = cdb_create();
    CDBSheet *sheet;
    cdb_create_sheet(db, "Sheet");
    sheet = cdb_get_sheet_by_name(db, "Sheet");

    CDBColumn *col = cdb_column_create("perms", CDB_TFLAGS);
    col->type_str = strdup("10:read,write,execute");
    col->enum_count = 3;
    col->enum_values = malloc(3 * sizeof(char*));
    col->enum_values[0] = strdup("read");
    col->enum_values[1] = strdup("write");
    col->enum_values[2] = strdup("execute");
    cdb_sheet_add_column(sheet, col, -1);
    cdb_sheet_add_row(sheet, -1);

    cdb_set_int(sheet, 0, "perms", 5);  // read + write (1 + 4)
    ASSERT_INT(cdb_get_int(sheet, 0, "perms"), 5, "flags value mismatch");

    char path[256];
    temp_path(path, sizeof(path), "type_flags");
    cdb_save(db, path);
    CDB *db2 = cdb_create();
    cdb_load(db2, path);
    CDBSheet *sheet2 = cdb_get_sheet_by_name(db2, "Sheet");
    ASSERT_INT(cdb_get_int(sheet2, 0, "perms"), 5, "flags after reload mismatch");

    cdb_close(db2);
    cdb_close(db);
    remove(path);
}
```

- [ ] **Step 3: 撰寫 type_color**

```c
TEST(type_color) {
    CDB *db = cdb_create();
    CDBSheet *sheet;
    cdb_create_sheet(db, "Sheet");
    sheet = cdb_get_sheet_by_name(db, "Sheet");
    CDBColumn *col = cdb_column_create("color", CDB_TCOLOR);
    col->type_str = strdup("11");
    cdb_sheet_add_column(sheet, col, -1);
    cdb_sheet_add_row(sheet, -1);

    // RGBA: 0xAABBCCDD
    cdb_set_int(sheet, 0, "color", 0xAABBCCDD);
    ASSERT_INT(cdb_get_int(sheet, 0, "color"), 0xAABBCCDD, "color value mismatch");

    char path[256];
    temp_path(path, sizeof(path), "type_color");
    cdb_save(db, path);
    CDB *db2 = cdb_create();
    cdb_load(db2, path);
    CDBSheet *sheet2 = cdb_get_sheet_by_name(db2, "Sheet");
    ASSERT_INT(cdb_get_int(sheet2, 0, "color"), 0xAABBCCDD, "color after reload mismatch");

    cdb_close(db2);
    cdb_close(db);
    remove(path);
}
```

- [ ] **Step 4: 撰寫 type_optional**

```c
TEST(type_optional) {
    CDB *db = cdb_create();
    CDBSheet *sheet;
    cdb_create_sheet(db, "Sheet");
    sheet = cdb_get_sheet_by_name(db, "Sheet");
    CDBColumn *col = cdb_column_create("opt", CDB_TSTRING);
    col->is_optional = 1;
    cdb_sheet_add_column(sheet, col, -1);
    cdb_sheet_add_row(sheet, -1);

    // New row, value should be NULL/empty
    ASSERT_INT(cdb_value_is_null(sheet, 0, "opt"), 1, "new row should be null");

    cdb_set_string(sheet, 0, "opt", "value");
    ASSERT_INT(cdb_value_is_null(sheet, 0, "opt"), 0, "should not be null after set");

    char path[256];
    temp_path(path, sizeof(path), "type_optional");
    cdb_save(db, path);
    CDB *db2 = cdb_create();
    cdb_load(db2, path);
    CDBSheet *sheet2 = cdb_get_sheet_by_name(db2, "Sheet");
    ASSERT_INT(cdb_value_is_null(sheet2, 0, "opt"), 0, "optional after reload should have value");

    cdb_close(db2);
    cdb_close(db);
    remove(path);
}
```

---

## Task 6: 撰寫 Group 4 - 邊界條件測試（5 案例）

**Files:**
- Modify: `castle_c/test/test_cdb.c`

- [ ] **Step 1: 撰寫 edge_null_value**

```c
TEST(edge_null_value) {
    CDB *db = cdb_create();
    CDBSheet *sheet;
    cdb_create_sheet(db, "Sheet");
    sheet = cdb_get_sheet_by_name(db, "Sheet");
    CDBColumn *col = cdb_column_create("val", CDB_TSTRING);
    cdb_sheet_add_column(sheet, col, -1);
    cdb_sheet_add_row(sheet, -1);

    // Set empty string
    cdb_set_string(sheet, 0, "val", "");
    ASSERT_INT(cdb_value_is_null(sheet, 0, "val"), 1, "empty string should be null");

    char path[256];
    temp_path(path, sizeof(path), "edge_null");
    cdb_save(db, path);
    CDB *db2 = cdb_create();
    cdb_load(db2, path);
    CDBSheet *sheet2 = cdb_get_sheet_by_name(db2, "Sheet");
    ASSERT_INT(cdb_value_is_null(sheet2, 0, "val"), 1, "null after reload mismatch");

    cdb_close(db2);
    cdb_close(db);
    remove(path);
}
```

- [ ] **Step 2: 撰寫 edge_empty_sheet**

```c
TEST(edge_empty_sheet) {
    CDB *db = cdb_create();
    cdb_create_sheet(db, "EmptySheet");
    CDBSheet *sheet = cdb_get_sheet_by_name(db, "EmptySheet");
    ASSERT_INT(cdb_sheet_column_count(sheet), 0, "empty sheet should have 0 columns");
    ASSERT_INT(cdb_sheet_row_count(sheet), 0, "empty sheet should have 0 rows");

    char path[256];
    temp_path(path, sizeof(path), "edge_empty_sheet");
    cdb_save(db, path);
    CDB *db2 = cdb_create();
    cdb_load(db2, path);
    CDBSheet *sheet2 = cdb_get_sheet_by_name(db2, "EmptySheet");
    ASSERT_INT(cdb_sheet_column_count(sheet2), 0, "loaded empty sheet columns mismatch");
    ASSERT_INT(cdb_sheet_row_count(sheet2), 0, "loaded empty sheet rows mismatch");

    cdb_close(db2);
    cdb_close(db);
    remove(path);
}
```

- [ ] **Step 3: 撰寫 edge_special_chars**

```c
TEST(edge_special_chars) {
    CDB *db = cdb_create();
    CDBSheet *sheet;
    cdb_create_sheet(db, "Sheet");
    sheet = cdb_get_sheet_by_name(db, "Sheet");
    CDBColumn *col = cdb_column_create("val", CDB_TSTRING);
    cdb_sheet_add_column(sheet, col, -1);
    cdb_sheet_add_row(sheet, -1);

    // Test JSON special characters
    cdb_set_string(sheet, 0, "val", "quote\"slash\\newline\ntab\treturn\r");
    ASSERT_STR(cdb_get_string(sheet, 0, "val"), "quote\"slash\\newline\ntab\treturn\r", "special chars mismatch");

    char path[256];
    temp_path(path, sizeof(path), "edge_special");
    cdb_save(db, path);
    CDB *db2 = cdb_create();
    cdb_load(db2, path);
    CDBSheet *sheet2 = cdb_get_sheet_by_name(db2, "Sheet");
    ASSERT_STR(cdb_get_string(sheet2, 0, "val"), "quote\"slash\\newline\ntab\treturn\r", "special chars after reload mismatch");

    cdb_close(db2);
    cdb_close(db);
    remove(path);
}
```

- [ ] **Step 4: 撰寫 edge_large_data**

```c
TEST(edge_large_data) {
    CDB *db = cdb_create();
    CDBSheet *sheet;
    cdb_create_sheet(db, "Sheet");
    sheet = cdb_get_sheet_by_name(db, "Sheet");

    // Add 10 columns
    for (int i = 0; i < 10; i++) {
        char colname[32];
        snprintf(colname, sizeof(colname), "col%d", i);
        CDBColumn *col = cdb_column_create(colname, CDB_TINT);
        cdb_sheet_add_column(sheet, col, -1);
    }

    // Add 1000 rows
    for (int r = 0; r < 1000; r++) {
        cdb_sheet_add_row(sheet, -1);
        for (int c = 0; c < 10; c++) {
            char colname[32];
            snprintf(colname, sizeof(colname), "col%d", c);
            cdb_set_int(sheet, r, colname, r * 10 + c);
        }
    }

    ASSERT_INT(cdb_sheet_row_count(sheet), 1000, "should have 1000 rows");

    char path[256];
    temp_path(path, sizeof(path), "edge_large");
    cdb_save(db, path);
    CDB *db2 = cdb_create();
    cdb_load(db2, path);
    CDBSheet *sheet2 = cdb_get_sheet_by_name(db2, "Sheet");
    ASSERT_INT(cdb_sheet_row_count(sheet2), 1000, "loaded should have 1000 rows");
    ASSERT_INT(cdb_get_int(sheet2, 999, "col9"), 999 * 10 + 9, "last row col9 mismatch");

    cdb_close(db2);
    cdb_close(db);
    remove(path);
}
```

- [ ] **Step 5: 撰寫 edge_unicode**

```c
TEST(edge_unicode) {
    CDB *db = cdb_create();
    CDBSheet *sheet;
    cdb_create_sheet(db, "Sheet");
    sheet = cdb_get_sheet_by_name(db, "Sheet");
    CDBColumn *col = cdb_column_create("name", CDB_TSTRING);
    cdb_sheet_add_column(sheet, col, -1);
    cdb_sheet_add_row(sheet, -1);

    cdb_set_string(sheet, 0, "name", "中文測試 русский 한국어");
    ASSERT_STR(cdb_get_string(sheet, 0, "name"), "中文測試 русский 한국어", "unicode mismatch");

    char path[256];
    temp_path(path, sizeof(path), "edge_unicode");
    cdb_save(db, path);
    CDB *db2 = cdb_create();
    cdb_load(db2, path);
    CDBSheet *sheet2 = cdb_get_sheet_by_name(db2, "Sheet");
    ASSERT_STR(cdb_get_string(sheet2, 0, "name"), "中文測試 русский 한국어", "unicode after reload mismatch");

    cdb_close(db2);
    cdb_close(db);
    remove(path);
}
```

---

## Task 7: 撰寫 Group 5 - 錯誤處理測試（3 案例）

**Files:**
- Modify: `castle_c/test/test_cdb.c`

- [ ] **Step 1: 撰寫 error_file_not_found**

```c
TEST(error_file_not_found) {
    CDB *db = cdb_create();
    int result = cdb_load(db, "/nonexistent/path/castle_test_nofile.cdb");
    ASSERT_INT(result, -1, "load nonexistent file should return -1");
    ASSERT_INT(cdb_get_error(db), CDB_ERR_FILE_NOT_FOUND, "error should be FILE_NOT_FOUND");
    cdb_close(db);
}
```

- [ ] **Step 2: 撰寫 error_invalid_json**

```c
TEST(error_invalid_json) {
    // Write invalid JSON to temp file
    char path[256];
    temp_path(path, sizeof(path), "error_invalid");
    FILE *f = fopen(path, "w");
    fprintf(f, "{ invalid json content }");
    fclose(f);

    CDB *db = cdb_create();
    int result = cdb_load(db, path);
    ASSERT_INT(result, -1, "load invalid JSON should return -1");
    ASSERT_INT(cdb_get_error(db), CDB_ERR_JSON_PARSE, "error should be JSON_PARSE");
    cdb_close(db);
    remove(path);
}
```

- [ ] **Step 3: 撰寫 error_corrupt_cdb**

```c
TEST(error_corrupt_cdb) {
    char path[256];
    temp_path(path, sizeof(path), "error_corrupt");
    FILE *f = fopen(path, "w");
    // Valid JSON but not a CDB file (no sheets array)
    fprintf(f, "{\"name\": \"not a cdb\", \"data\": 123}");
    fclose(f);

    CDB *db = cdb_create();
    int result = cdb_load(db, path);
    ASSERT_INT(result, -1, "load corrupt CDB should return -1");
    // Error should be either INVALID_CDB or JSON_PARSE depending on parsing stage
    ASSERT(cdb_get_error(db) == CDB_ERR_INVALID_CDB || cdb_get_error(db) == CDB_ERR_JSON_PARSE,
           "error should be INVALID_CDB or JSON_PARSE");
    cdb_close(db);
    remove(path);
}
```

---

## Task 8: 撰寫 Group 6 - Sheet 操作測試（6 案例）

**Files:**
- Modify: `castle_c/test/test_cdb.c`

- [ ] **Step 1: 撰寫 ops_add_sheet**

```c
TEST(ops_add_sheet) {
    CDB *db = cdb_create();
    ASSERT_INT(cdb_get_sheet_count(db), 0, "initial sheet count should be 0");
    cdb_create_sheet(db, "First");
    ASSERT_INT(cdb_get_sheet_count(db), 1, "should have 1 sheet");
    cdb_create_sheet(db, "Second");
    ASSERT_INT(cdb_get_sheet_count(db), 2, "should have 2 sheets");
    ASSERT(cdb_get_sheet_by_name(db, "First") != NULL, "First sheet should exist");
    ASSERT(cdb_get_sheet_by_name(db, "Second") != NULL, "Second sheet should exist");
    ASSERT(cdb_get_sheet_by_name(db, "Third") == NULL, "Third sheet should not exist");

    char path[256];
    temp_path(path, sizeof(path), "ops_add_sheet");
    cdb_save(db, path);
    CDB *db2 = cdb_create();
    cdb_load(db2, path);
    ASSERT_INT(cdb_get_sheet_count(db2), 2, "loaded should have 2 sheets");
    cdb_close(db2);
    cdb_close(db);
    remove(path);
}
```

- [ ] **Step 2: 撰寫 ops_delete_sheet**

```c
TEST(ops_delete_sheet) {
    CDB *db = cdb_create();
    cdb_create_sheet(db, "Keep");
    cdb_create_sheet(db, "Delete");
    cdb_create_sheet(db, "AlsoKeep");

    ASSERT_INT(cdb_get_sheet_count(db), 3, "should have 3 sheets");
    cdb_delete_sheet(db, "Delete");
    ASSERT_INT(cdb_get_sheet_count(db), 2, "should have 2 sheets after delete");
    ASSERT(cdb_get_sheet_by_name(db, "Keep") != NULL, "Keep should exist");
    ASSERT(cdb_get_sheet_by_name(db, "Delete") == NULL, "Delete should not exist");
    ASSERT(cdb_get_sheet_by_name(db, "AlsoKeep") != NULL, "AlsoKeep should exist");

    char path[256];
    temp_path(path, sizeof(path), "ops_delete_sheet");
    cdb_save(db, path);
    CDB *db2 = cdb_create();
    cdb_load(db2, path);
    ASSERT_INT(cdb_get_sheet_count(db2), 2, "loaded should have 2 sheets");
    cdb_close(db2);
    cdb_close(db);
    remove(path);
}
```

- [ ] **Step 3: 撰寫 ops_add_column**

```c
TEST(ops_add_column) {
    CDB *db = cdb_create();
    CDBSheet *sheet;
    cdb_create_sheet(db, "Sheet");
    sheet = cdb_get_sheet_by_name(db, "Sheet");
    cdb_sheet_add_row(sheet, -1);

    CDBColumn *col1 = cdb_column_create("col1", CDB_TINT);
    cdb_sheet_add_column(sheet, col1, -1);
    ASSERT_INT(cdb_sheet_column_count(sheet), 1, "should have 1 column");
    cdb_set_int(sheet, 0, "col1", 100);

    CDBColumn *col2 = cdb_column_create("col2", CDB_TSTRING);
    cdb_sheet_add_column(sheet, col2, -1);
    ASSERT_INT(cdb_sheet_column_count(sheet), 2, "should have 2 columns");

    // col1 value should still be 100
    ASSERT_INT(cdb_get_int(sheet, 0, "col1"), 100, "col1 value should persist");
    // col2 value should be null
    ASSERT_INT(cdb_value_is_null(sheet, 0, "col2"), 1, "new column value should be null");

    char path[256];
    temp_path(path, sizeof(path), "ops_add_column");
    cdb_save(db, path);
    CDB *db2 = cdb_create();
    cdb_load(db2, path);
    CDBSheet *sheet2 = cdb_get_sheet_by_name(db2, "Sheet");
    ASSERT_INT(cdb_sheet_column_count(sheet2), 2, "loaded should have 2 columns");
    ASSERT_INT(cdb_get_int(sheet2, 0, "col1"), 100, "col1 after reload mismatch");
    cdb_close(db2);
    cdb_close(db);
    remove(path);
}
```

- [ ] **Step 4: 撰寫 ops_delete_column**

```c
TEST(ops_delete_column) {
    CDB *db = cdb_create();
    CDBSheet *sheet;
    cdb_create_sheet(db, "Sheet");
    sheet = cdb_get_sheet_by_name(db, "Sheet");

    CDBColumn *col1 = cdb_column_create("keep", CDB_TINT);
    cdb_sheet_add_column(sheet, col1, -1);
    CDBColumn *col2 = cdb_column_create("delete", CDB_TSTRING);
    cdb_sheet_add_column(sheet, col2, -1);
    CDBColumn *col3 = cdb_column_create("also_keep", CDB_TBOOL);
    cdb_sheet_add_column(sheet, col3, -1);

    cdb_sheet_add_row(sheet, -1);
    cdb_set_int(sheet, 0, "keep", 42);
    cdb_set_string(sheet, 0, "delete", "to_delete");
    cdb_set_bool(sheet, 0, "also_keep", 1);

    cdb_sheet_delete_column(sheet, "delete");
    ASSERT_INT(cdb_sheet_column_count(sheet), 2, "should have 2 columns after delete");
    ASSERT_INT(cdb_get_int(sheet, 0, "keep"), 42, "keep value should persist");
    ASSERT_INT(cdb_get_bool(sheet, 0, "also_keep"), 1, "also_keep value should persist");
    ASSERT(cdb_sheet_column_by_name(sheet, "delete") == NULL, "delete column should be gone");

    char path[256];
    temp_path(path, sizeof(path), "ops_delete_column");
    cdb_save(db, path);
    CDB *db2 = cdb_create();
    cdb_load(db2, path);
    CDBSheet *sheet2 = cdb_get_sheet_by_name(db2, "Sheet");
    ASSERT_INT(cdb_sheet_column_count(sheet2), 2, "loaded should have 2 columns");
    cdb_close(db2);
    cdb_close(db);
    remove(path);
}
```

- [ ] **Step 5: 撰寫 ops_add_row**

```c
TEST(ops_add_row) {
    CDB *db = cdb_create();
    CDBSheet *sheet;
    cdb_create_sheet(db, "Sheet");
    sheet = cdb_get_sheet_by_name(db, "Sheet");
    CDBColumn *col = cdb_column_create("val", CDB_TINT);
    cdb_sheet_add_column(sheet, col, -1);

    ASSERT_INT(cdb_sheet_row_count(sheet), 0, "initial row count should be 0");
    cdb_sheet_add_row(sheet, -1);
    cdb_set_int(sheet, 0, "val", 10);
    ASSERT_INT(cdb_sheet_row_count(sheet), 1, "should have 1 row");

    cdb_sheet_add_row(sheet, -1);
    cdb_set_int(sheet, 1, "val", 20);
    ASSERT_INT(cdb_sheet_row_count(sheet), 2, "should have 2 rows");
    ASSERT_INT(cdb_get_int(sheet, 0, "val"), 10, "first row value mismatch");
    ASSERT_INT(cdb_get_int(sheet, 1, "val"), 20, "second row value mismatch");

    char path[256];
    temp_path(path, sizeof(path), "ops_add_row");
    cdb_save(db, path);
    CDB *db2 = cdb_create();
    cdb_load(db2, path);
    CDBSheet *sheet2 = cdb_get_sheet_by_name(db2, "Sheet");
    ASSERT_INT(cdb_sheet_row_count(sheet2), 2, "loaded should have 2 rows");
    ASSERT_INT(cdb_get_int(sheet2, 0, "val"), 10, "row 0 after reload mismatch");
    ASSERT_INT(cdb_get_int(sheet2, 1, "val"), 20, "row 1 after reload mismatch");
    cdb_close(db2);
    cdb_close(db);
    remove(path);
}
```

- [ ] **Step 6: 撰寫 ops_delete_row**

```c
TEST(ops_delete_row) {
    CDB *db = cdb_create();
    CDBSheet *sheet;
    cdb_create_sheet(db, "Sheet");
    sheet = cdb_get_sheet_by_name(db, "Sheet");
    CDBColumn *col = cdb_column_create("val", CDB_TINT);
    cdb_sheet_add_column(sheet, col, -1);

    cdb_sheet_add_row(sheet, -1);
    cdb_set_int(sheet, 0, "val", 10);
    cdb_sheet_add_row(sheet, -1);
    cdb_set_int(sheet, 1, "val", 20);
    cdb_sheet_add_row(sheet, -1);
    cdb_set_int(sheet, 2, "val", 30);

    ASSERT_INT(cdb_sheet_row_count(sheet), 3, "should have 3 rows");
    cdb_sheet_delete_row(sheet, 1);  // Delete middle row
    ASSERT_INT(cdb_sheet_row_count(sheet), 2, "should have 2 rows after delete");
    ASSERT_INT(cdb_get_int(sheet, 0, "val"), 10, "first row should still be 10");
    ASSERT_INT(cdb_get_int(sheet, 1, "val"), 30, "last row should now be 30 (was 20 deleted)");

    char path[256];
    temp_path(path, sizeof(path), "ops_delete_row");
    cdb_save(db, path);
    CDB *db2 = cdb_create();
    cdb_load(db2, path);
    CDBSheet *sheet2 = cdb_get_sheet_by_name(db2, "Sheet");
    ASSERT_INT(cdb_sheet_row_count(sheet2), 2, "loaded should have 2 rows");
    cdb_close(db2);
    cdb_close(db);
    remove(path);
}
```

---

## Task 9: 撰寫 main() 和執行所有測試

**Files:**
- Modify: `castle_c/test/test_cdb.c`

- [ ] **Step 1: 撰寫 main()**

```c
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
           3 + 4 + 4 + 5 + 3 + 6, failures);
    return failures > 0 ? 1 : 0;
}
```

---

## Task 10: 建立 Makefile

**Files:**
- Create: `castle_c/test/Makefile`

- [ ] **Step 1: 建立 Makefile**

```makefile
CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c99 -I../../sources/lib -I../../../base/sources/libs
LDFLAGS =

SOURCES = test_cdb.c \
           ../../sources/lib/cdb.c \
           ../../sources/lib/cdb_reader.c \
           ../../sources/lib/cdb_writer.c \
           ../../../base/sources/libs/jsmn.c

TARGET = test_cdb

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $(SOURCES) $(LDFLAGS)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
```

---

## Task 11: 編譯並執行測試

- [ ] **Step 1: 編譯**

```bash
cd castle_c/test && make clean && make
```

- [ ] **Step 2: 執行**

```bash
./test_cdb
```

- [ ] **Step 3: 如有失敗，根據輸出修復問題**

- [ ] **Step 4: 全部通過後 commit**

```bash
git add castle_c/test/
git commit -m "test(castle_c): add CDB save/load test suite

- 23 test cases across 6 groups
- Basic flow, primitive types, complex types, edge cases, error handling, sheet operations

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>"
```

---

## 實作順序

1. Task 1: 建立測試目錄
2. Task 2: 撰寫測試框架巨集
3. Task 3: Group 1 - 基本流程（3 案例）
4. Task 4: Group 2 - 基本類型（4 案例）
5. Task 5: Group 3 - 複雜類型（4 案例）
6. Task 6: Group 4 - 邊界條件（5 案例）
7. Task 7: Group 5 - 錯誤處理（3 案例）
8. Task 8: Group 6 - Sheet 操作（6 案例）
9. Task 9: 撰寫 main()
10. Task 10: 建立 Makefile
11. Task 11: 編譯、執行、修復、commit
