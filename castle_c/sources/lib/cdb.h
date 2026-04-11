#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CDB_VERSION "1.0.0"

typedef enum {
    CDB_TID = 0,
    CDB_TSTRING = 1,
    CDB_TBOOL = 2,
    CDB_TINT = 3,
    CDB_TFLOAT = 4,
    CDB_TENUM = 5,
    CDB_TREF = 6,
    CDB_TIMAGE = 7,
    CDB_TLIST = 8,
    CDB_TCUSTOM = 9,
    CDB_TFLAGS = 10,
    CDB_TCOLOR = 11,
    CDB_TLAYER = 12,
    CDB_TFILE = 13,
    CDB_TTILEPOS = 14,
    CDB_TTILELAYER = 15,
    CDB_TDYNAMIC = 16,
    CDB_TPROPERTIES = 17,
    CDB_TGRADIENT = 18,
    CDB_TCURVE = 19,
    CDB_TGUID = 20,
    CDB_TPOLYMORPH = 21
} CDBColumnType;

typedef enum {
    CDB_OK,
    CDB_ERR_FILE_NOT_FOUND,
    CDB_ERR_FILE_READ,
    CDB_ERR_FILE_WRITE,
    CDB_ERR_JSON_PARSE,
    CDB_ERR_INVALID_CDB,
    CDB_ERR_OUT_OF_MEMORY,
    CDB_ERR_NOT_FOUND,
    CDB_ERR_TYPE_MISMATCH
} CDBError;

typedef struct CDB CDB;
typedef struct CDBSheet CDBSheet;
typedef struct CDBColumn CDBColumn;

// Lifecycle
CDB *cdb_create(void);
int cdb_load(CDB *db, const char *filepath);
int cdb_save(CDB *db, const char *filepath);
void cdb_close(CDB *db);
CDBError cdb_get_error(CDB *db);
const char *cdb_error_string(CDBError err);

// Database operations
int cdb_get_sheet_count(CDB *db);
CDBSheet *cdb_get_sheet(CDB *db, int index);
CDBSheet *cdb_get_sheet_by_name(CDB *db, const char *name);
int cdb_create_sheet(CDB *db, const char *name);
int cdb_delete_sheet(CDB *db, const char *name);

// Sheet operations
const char *cdb_sheet_name(CDBSheet *sheet);
int cdb_sheet_column_count(CDBSheet *sheet);
CDBColumn *cdb_sheet_column(CDBSheet *sheet, int index);
CDBColumn *cdb_sheet_column_by_name(CDBSheet *sheet, const char *name);
int cdb_sheet_add_column(CDBSheet *sheet, CDBColumn *col, int position);
int cdb_sheet_delete_column(CDBSheet *sheet, const char *column_name);
int cdb_sheet_row_count(CDBSheet *sheet);
int cdb_sheet_add_row(CDBSheet *sheet, int position);
int cdb_sheet_delete_row(CDBSheet *sheet, int row_index);
int cdb_sheet_move_row(CDBSheet *sheet, int from, int to);

// Column operations
CDBColumn *cdb_column_create(const char *name, CDBColumnType type);
void cdb_column_destroy(CDBColumn *col);
const char *cdb_column_name(CDBColumn *col);
CDBColumnType cdb_column_type(CDBColumn *col);
const char *cdb_column_type_str(CDBColumn *col);  // e.g., "5:value1,value2"
int cdb_column_is_optional(CDBColumn *col);
void cdb_column_set_optional(CDBColumn *col, int optional);

// Value access - string
const char *cdb_get_string(CDBSheet *sheet, int row, const char *column);
int cdb_set_string(CDBSheet *sheet, int row, const char *column, const char *value);

// Value access - int
int cdb_get_int(CDBSheet *sheet, int row, const char *column);
int cdb_set_int(CDBSheet *sheet, int row, const char *column, int value);

// Value access - float
float cdb_get_float(CDBSheet *sheet, int row, const char *column);
int cdb_set_float(CDBSheet *sheet, int row, const char *column, float value);

// Value access - bool
int cdb_get_bool(CDBSheet *sheet, int row, const char *column);
int cdb_set_bool(CDBSheet *sheet, int row, const char *column, int value);

// Value access - raw (for complex types)
const char *cdb_get_raw(CDBSheet *sheet, int row, const char *column);
int cdb_set_raw(CDBSheet *sheet, int row, const char *column, const char *value);

// Value null check
int cdb_value_is_null(CDBSheet *sheet, int row, const char *column);

#ifdef __cplusplus
}
#endif