# CastleDB C 測試整合進 base/tests 設計

## Overview

將 castle_c 的 cdb_save/cdb_load 測試整合進 armortools 的 base/tests 架構，使用 project.js 構建系統。

## 目標

- castle_c 測試服從 `base/make` 構建系統
- 測試 binary 位於 `base/tests/castle_c/build/`
- 移除獨立的 `castle_c/test/` 目錄（已廢棄）
- 保持 castle_c library 原始位置（`castle_c/sources/lib/`）

## 架構

### 目錄結構

```
armortools/
├── castle_c/                         # Library（保持原位）
│   └── sources/lib/
│       ├── cdb.c
│       ├── cdb_reader.c
│       ├── cdb_writer.c
│       ├── cdb.h
│       └── cdb_internal.h
│
└── base/tests/castle_c/              # NEW - 測試位置
    ├── project.js                     # 構建配置
    ├── test_cdb.c                     # 測試程式（遷移並修改）
    └── (build/)                       # 輸出目錄
```

### Build 流程

```bash
# 從 armortools/ 根目錄
cd base/tests/castle_c
../../../make --run

# 或從任意位置（推薦）
cd base && ./make --target castle_c_test
```

## 實作細節

### project.js

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

### test_cdb.c 修改

1. **Include 路徑更新**：
   - 舊: `#include "../sources/lib/cdb.h"` (from `castle_c/test/`)
   - 新: `#include "cdb.h"` (已透過 project.js 的 include_dir 設定)

2. **不使用 Iron**：
   - 純 C `main()` 入口，不調用 `_kickstart()`
   - 獨立的 command-line 測試程式

### 移除舊測試目錄

測試遷移完成後，刪除 `castle_c/test/`：
```bash
rm -rf castle_c/test/
```

## 執行方式

```bash
# Build
cd base/tests/castle_c
../../../make

# Run
./build/build/Release/CastleCDBTest
```

## 輸出 binary

- Debug: `base/tests/castle_c/build/build/Release/CastleCDBTest`
- Release: 同上（base 預設 Release）

## 遷移 checklist

1. 建立 `base/tests/castle_c/` 目錄
2. 建立 `base/tests/castle_c/project.js`
3. 複製並修改 `castle_c/test/test_cdb.c` → `base/tests/castle_c/test_cdb.c`
4. 測試編譯和執行
5. 確認 25 tests, 0 failures
6. 刪除 `castle_c/test/`
7. 更新 `castle_c/README.md`（如有）
