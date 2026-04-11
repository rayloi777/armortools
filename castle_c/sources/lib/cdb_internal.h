#pragma once

#include "cdb.h"

// Internal column structure
struct CDBColumn {
    char *name;
    CDBColumnType type;
    char *type_str;        // Original type string e.g., "5:value1,value2"
    char **enum_values;    // For TEnum, TFlags
    int enum_count;
    char *ref_sheet;       // For TRef
    int is_optional;
};

// Internal sheet structure
struct CDBSheet {
    char *name;
    CDBColumn *columns;
    int column_count;
    char ***lines;         // 2D array: lines[row][col]
    int line_count;
    void *props;           // Sheet properties (displayColumn, etc.)
    int hidden;            // For sub-sheets (hide flag)
};

// Internal database structure
struct CDB {
    CDBError error;
    char *filepath;
    CDBSheet *sheets;
    int sheet_count;
    int compress;          // LZ4 compression enabled
    void *custom_types;    // For TCustom support
};
