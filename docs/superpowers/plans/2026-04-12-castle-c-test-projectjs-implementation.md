# CastleDB C 測試整合進 base/tests 實作計畫

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 將 castle_c 測試整合進 base/tests/castle_c/，使用 project.js 構建系統。

**Architecture:** 測試作為 base/tests/ 的子項目，透過 project.js 引用 castle_c/sources/lib/ 的 C 程式碼。純 C main() 入口，不使用 Iron。

**Tech Stack:** C99、project.js 構建系統、castle_c library

---

## 檔案結構

```
base/tests/castle_c/                    # NEW - 測試位置
├── project.js                         # 構建配置
├── test_cdb.c                        # 測試程式
└── (build/)                          # 輸出目錄（自動產生）

castle_c/sources/lib/                  # 不變
├── cdb.h
├── cdb.c
├── cdb_reader.c
├── cdb_writer.c
├── cdb_internal.h
└── jsmn.h / jsmn.c
```

---

## Task 1: 建立目錄

**Files:**
- Create: `base/tests/castle_c/` 目錄

- [ ] **Step 1: 建立目錄**

```bash
mkdir -p base/tests/castle_c
ls -la base/tests/castle_c/
```

Expected output: `total 8 drwxr-xr-x 9 rayloi staff 288 12 Apr 00:XX base/tests/castle_c/`

---

## Task 2: 建立 project.js

**Files:**
- Create: `base/tests/castle_c/project.js`

- [ ] **Step 1: 撰寫 project.js**

```javascript
let flags = globalThis.flags;

let project = new Project("CastleCDBTest");
project.add_project("../../");  // 包含 base

// Add castle_c library sources
// 相對路徑: base/tests/castle_c/ -> castle_c/sources/lib/
project.add_include_dir("../../../castle_c/sources/lib");
project.add_cfiles("../../../castle_c/sources/lib/cdb.c");
project.add_cfiles("../../../castle_c/sources/lib/cdb_reader.c");
project.add_cfiles("../../../castle_c/sources/lib/cdb_writer.c");

// Test entry point
project.add_cfiles("test_cdb.c");

return project;
```

---

## Task 3: 遷移並修改 test_cdb.c

**Files:**
- Create: `base/tests/castle_c/test_cdb.c`（從 `castle_c/test/test_cdb.c` 複製並修改）

- [ ] **Step 1: 複製現有測試檔案**

```bash
cp castle_c/test/test_cdb.c base/tests/castle_c/test_cdb.c
```

- [ ] **Step 2: 修改 include 路徑**

將：
```c
#include "../sources/lib/cdb.h"
#include "../sources/lib/cdb_internal.h"
```

改為：
```c
#include "cdb.h"
#include "cdb_internal.h"
```

（因為 project.js 已透過 `project.add_include_dir("../../../castle_c/sources/lib")` 設定 include 路徑）

---

## Task 4: 測試編譯

- [ ] **Step 1: 嘗試編譯**

```bash
cd base/tests/castle_c
../../../make 2>&1
```

Expected: 成功編譯，產生 `base/tests/castle_c/build/build/Release/CastleCDBTest`

- [ ] **Step 2: 若編譯失敗，根據錯誤修復**

常見問題：
- Include path 錯誤 → 確認 project.js 的 `add_include_dir` 路徑正確
- JSMN 找不到 → 確認 `jsmn.h` 位於 `castle_c/sources/lib/`（是的）
- 符號未定義 → 確認所有 cdb library .c 檔案都已加入

---

## Task 5: 執行測試並驗證

- [ ] **Step 1: 執行測試**

```bash
./build/build/Release/CastleCDBTest
```

Expected output:
```
=== CastleDB C Test Suite ===

Running basic_create_save_load... OK
Running basic_empty_db... OK
Running basic_two_sheets... OK
Running type_int... OK
Running type_float... OK
Running type_bool... OK
Running type_string... OK
Running type_enum... OK
Running type_flags... OK
Running type_color... OK
Running type_optional... OK
Running edge_null_value... OK
Running edge_empty_sheet... OK
Running edge_special_chars... OK
Running edge_large_data... OK
Running edge_unicode... OK
Running error_file_not_found... OK
Running error_invalid_json... OK
Running error_corrupt_cdb... OK
Running ops_add_sheet... OK
Running ops_delete_sheet... OK
Running ops_add_column... OK
Running ops_delete_column... OK
Running ops_add_row... OK
Running ops_delete_row... OK

=== 25 tests, 0 failures ===
```

- [ ] **Step 2: 若有失敗，根據輸出修復問題**

常見失敗原因：
- 路徑相關（temp file）→ 檢查 `temp_path()` 函數
- 資料比對失敗 → 檢查 ASSERT_* 巨集

---

## Task 6: 刪除舊測試目錄

- [ ] **Step 1: 刪除 `castle_c/test/`**

```bash
rm -rf castle_c/test/
ls -la castle_c/
```

確認 `castle_c/test/` 目錄已不存在。

- [ ] **Step 2: 確認 git 狀態**

```bash
git status
```

預期：`castle_c/test/` 刪除、`base/tests/castle_c/` 新增

---

## Task 7: 更新 castle_c/README.md

**Files:**
- Modify: `castle_c/README.md`

- [ ] **Step 1: 在 README.md 適當位置加入測試說明**

在現有的 "Project Structure" 區塊或新增 "Testing" 區塊：

```markdown
## Testing

The CDB library is tested using a dedicated test suite located in `base/tests/castle_c/`.

```bash
# Build and run tests
cd base/tests/castle_c
../../../make
./build/build/Release/CastleCDBTest
```

Expected: `=== 25 tests, 0 failures ===`
```

---

## Task 8: Commit

- [ ] **Step 1: Git add 並 commit**

```bash
git add base/tests/castle_c/
git add castle_c/README.md
git rm -r castle_c/test/
git status
git commit -m "$(cat <<'EOF'
test(castle_c): integrate tests into base/tests via project.js

- Move test suite from castle_c/test/ to base/tests/castle_c/
- Add project.js for armortools build system integration
- Update README with test location and run instructions

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>
EOF
)"
```

---

## 實作順序

1. Task 1: 建立 `base/tests/castle_c/` 目錄
2. Task 2: 建立 `base/tests/castle_c/project.js`
3. Task 3: 遷移並修改 test_cdb.c
4. Task 4: 測試編譯
5. Task 5: 執行測試並驗證（25 tests, 0 failures）
6. Task 6: 刪除舊 `castle_c/test/` 目錄
7. Task 7: 更新 `castle_c/README.md`
8. Task 8: Commit
