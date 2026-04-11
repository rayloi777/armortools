# CastleDB C 測試程式設計

## Overview

為 castle_c library 建立獨立測試程式，驗證寫入（cdb_writer）和輸出（cdb_reader）功能。

**目標：** 確保 cdb_save() / cdb_load() 往返正確，所有類型、邊界條件、錯誤處理皆覆蓋。

## 測試環境

```
castle_c/test/
├── test_cdb.c    # 主測試程式
└── Makefile      # 編譯腳本
```

**編譯方式：** `cd castle_c/test && make`

**依賴：**
- `castle_c/sources/lib/cdb.c`
- `castle_c/sources/lib/cdb_reader.c`
- `castle_c/sources/lib/cdb_writer.c`
- `castle_c/sources/lib/cdb_internal.h`
- `castle_c/sources/lib/cdb.h`
- `base/sources/libs/jsmn.h` + `jsmn.c`

## 測試框架

使用 minimal 自製框架，無外部依賴：

```c
#define TEST(name) void test_##name(void)
#define ASSERT(cond, msg) if (!(cond)) { printf("FAIL: %s\n", msg); failures++; }
#define RUN(test) do { printf("Running %s... ", #test); test_##test(); if (failures == last_failures) printf("OK\n"); last_failures = failures; } while(0)

extern int failures;
```

## 測試案例

### 群組 1: 基本流程（Basic Flow）

| 案例 | 描述 |
|------|------|
| basic_create_save_load | cdb_create() → 新增 sheet/column/row → set values → cdb_save() → cdb_load() → verify |
| basic_empty_db | 創建空資料庫，確認 sheet_count = 0 |
| basic_two_sheets | 兩個 sheet 的往返測試 |

### 群組 2: 基本類型（Primitive Types）

| 案例 | 描述 |
|------|------|
| type_int | set_int / get_int，含邊界值（0, 1, -1, INT_MAX, INT_MIN） |
| type_float | set_float / get_float，含邊界值（0.0, -1.5, FLT_MAX） |
| type_bool | set_bool / get_bool，true/false 往返 |
| type_string | set_string / get_string，含空字串、特殊字元 |

### 群組 3: 複雜類型（Complex Types）

| 案例 | 描述 |
|------|------|
| type_enum | enum column（typeStr="5:val1,val2,val3"），set_int / get_int |
| type_flags | flags column（typeStr="10:a,b,c"），set_int / get_int |
| type_color | color column（typeStr="11"），整數儲存 RGBA |
| type_optional | optional column，NULL 值處理 |

### 群組 4: 邊界條件（Edge Cases）

| 案例 | 描述 |
|------|------|
| edge_null_value | 設定 NULL（空字串）值，驗證 cdb_value_is_null() |
| edge_empty_sheet | 無 column 的 sheet |
| edge_special_chars | 欄位名含特殊字元：`,` `"` `\` `\n` |
| edge_large_data | 1000 rows × 10 columns，大量資料效能 |
| edge_unicode | UTF-8 中文字測試 |

### 群組 5: 錯誤處理（Error Handling）

| 案例 | 描述 |
|------|------|
| error_file_not_found | cdb_load() 讀取不存在的檔案，回傳 CDB_ERR_FILE_NOT_FOUND |
| error_invalid_json | 讀取無效 JSON，cdb_get_error() = CDB_ERR_JSON_PARSE |
| error_corrupt_cdb | 讀取格式錯誤的 CDB（缺 sheets 陣列） |

### 群組 6: Sheet 操作（Sheet Operations）

| 案例 | 描述 |
|------|------|
| ops_add_sheet | 新增多個 sheet，驗證名稱和數量 |
| ops_delete_sheet | 刪除 sheet，確認陣列正確移位 |
| ops_add_column | 新增 column，確認新 column 值為 NULL |
| ops_delete_column | 刪除 column，確認其他 column 資料不變 |
| ops_add_row | 新增 row，驗證可正常存取 |
| ops_delete_row | 刪除 row，確認其餘 rows 正確 |

## 測試資料檔案

測試產生的暫存檔案放在 `/tmp/castle_test_*.cdb`，測試結束後自動刪除。

## 預期產出

編譯後執行 `test_cdb`：
```
=== CastleDB C Test Suite ===
Running basic_create_save_load... OK
Running basic_empty_db... OK
Running type_int... OK
Running type_float... OK
...
=== 23 tests, 0 failures ===
```

## 編譯（Makefile）

```makefile
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I../../sources/lib -I../../../../base/sources/libs
LDFLAGS =

SRC = test_cdb.c \
      ../../sources/lib/cdb.c \
      ../../sources/lib/cdb_reader.c \
      ../../sources/lib/cdb_writer.c \
      ../../../../base/sources/libs/jsmn.c

test: $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDFLAGS)

clean:
	rm -f test_cdb
```

## 實作順序

1. 建立 `castle_c/test/` 目錄
2. 撰寫 `test_cdb.c`（框架 + 全部 23 個測試案例）
3. 建立 `Makefile`
4. 編譯並執行測試
5. 修復失敗的案例
6. 確認全部通過
