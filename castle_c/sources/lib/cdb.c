/*
 * CastleDB C Library
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "cdb_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static const char *error_strings[] = {
    "OK",
    "File not found",
    "File write error",
    "JSON parse error",
    "Invalid CDB file",
    "Out of memory",
    "Not found",
    "Type mismatch"
};

const char *cdb_error_string(CDBError err) {
    if (err >= 0 && err <= CDB_ERR_TYPE_MISMATCH) {
        return error_strings[err];
    }
    return "Unknown error";
}

CDB *cdb_create(void) {
    CDB *db = calloc(1, sizeof(CDB));
    if (!db) return NULL;
    db->error = CDB_OK;
    return db;
}

void cdb_close(CDB *db) {
    if (!db) return;

    if (db->sheets) {
        for (int i = 0; i < db->sheet_count; i++) {
            CDBSheet *sheet = &db->sheets[i];

            if (sheet->columns) {
                for (int j = 0; j < sheet->column_count; j++) {
                    cdb_column_destroy(&sheet->columns[j]);
                }
                free(sheet->columns);
            }

            if (sheet->lines) {
                for (int j = 0; j < sheet->line_count; j++) {
                    if (sheet->lines[j]) {
                        for (int k = 0; k < sheet->column_count; k++) {
                            free(sheet->lines[j][k]);
                        }
                        free(sheet->lines[j]);
                    }
                }
                free(sheet->lines);
            }

            free(sheet->name);
        }
        free(db->sheets);
    }

    free(db->filepath);
    free(db);
}

CDBError cdb_get_error(CDB *db) {
    return db ? db->error : CDB_ERR_INVALID_CDB;
}

CDBColumn *cdb_column_create(const char *name, CDBColumnType type) {
    CDBColumn *col = calloc(1, sizeof(CDBColumn));
    if (!col) return NULL;
    col->name = strdup(name);
    col->type = type;
    return col;
}

void cdb_column_destroy(CDBColumn *col) {
    if (!col) return;
    free(col->name);
    free(col->type_str);
    if (col->enum_values) {
        for (int i = 0; i < col->enum_count; i++) {
            free(col->enum_values[i]);
        }
        free(col->enum_values);
    }
    free(col->ref_sheet);
}

const char *cdb_column_name(CDBColumn *col) {
    return col ? col->name : NULL;
}

CDBColumnType cdb_column_type(CDBColumn *col) {
    return col ? col->type : CDB_TSTRING;
}

const char *cdb_column_type_str(CDBColumn *col) {
    return col ? col->type_str : NULL;
}

int cdb_column_is_optional(CDBColumn *col) {
    return col ? col->is_optional : 0;
}

void cdb_column_set_optional(CDBColumn *col, int optional) {
    if (col) col->is_optional = optional;
}

// Database operations
int cdb_get_sheet_count(CDB *db) {
    return db ? db->sheet_count : 0;
}

CDBSheet *cdb_get_sheet(CDB *db, int index) {
    if (!db || index < 0 || index >= db->sheet_count) return NULL;
    return &db->sheets[index];
}

CDBSheet *cdb_get_sheet_by_name(CDB *db, const char *name) {
    if (!db || !name) return NULL;
    for (int i = 0; i < db->sheet_count; i++) {
        if (db->sheets[i].name && strcmp(db->sheets[i].name, name) == 0) {
            return &db->sheets[i];
        }
    }
    return NULL;
}

int cdb_create_sheet(CDB *db, const char *name) {
    if (!db || !name) return -1;

    // Check if name already exists
    for (int i = 0; i < db->sheet_count; i++) {
        if (strcmp(db->sheets[i].name, name) == 0) {
            return -1;
        }
    }

    CDBSheet *new_sheets = realloc(db->sheets, (db->sheet_count + 1) * sizeof(CDBSheet));
    if (!new_sheets) return -1;
    db->sheets = new_sheets;

    CDBSheet *sheet = &db->sheets[db->sheet_count];
    memset(sheet, 0, sizeof(CDBSheet));
    sheet->name = strdup(name);
    db->sheet_count++;

    return 0;
}

int cdb_delete_sheet(CDB *db, const char *name) {
    if (!db || !name) return -1;

    int found = -1;
    for (int i = 0; i < db->sheet_count; i++) {
        if (strcmp(db->sheets[i].name, name) == 0) {
            found = i;
            break;
        }
    }

    if (found < 0) return -1;

    // Free the sheet
    CDBSheet *sheet = &db->sheets[found];
    if (sheet->columns) {
        for (int j = 0; j < sheet->column_count; j++) {
            cdb_column_destroy(&sheet->columns[j]);
        }
        free(sheet->columns);
    }
    if (sheet->lines) {
        for (int j = 0; j < sheet->line_count; j++) {
            if (sheet->lines[j]) {
                for (int k = 0; k < sheet->column_count; k++) {
                    free(sheet->lines[j][k]);
                }
                free(sheet->lines[j]);
            }
        }
        free(sheet->lines);
    }
    free(sheet->name);

    // Remove from array
    for (int i = found; i < db->sheet_count - 1; i++) {
        db->sheets[i] = db->sheets[i + 1];
    }
    db->sheet_count--;

    return 0;
}

// Sheet operations
const char *cdb_sheet_name(CDBSheet *sheet) {
    return sheet ? sheet->name : NULL;
}

int cdb_sheet_column_count(CDBSheet *sheet) {
    return sheet ? sheet->column_count : 0;
}

CDBColumn *cdb_sheet_column(CDBSheet *sheet, int index) {
    if (!sheet || index < 0 || index >= sheet->column_count) return NULL;
    return &sheet->columns[index];
}

CDBColumn *cdb_sheet_column_by_name(CDBSheet *sheet, const char *name) {
    if (!sheet || !name) return NULL;
    for (int i = 0; i < sheet->column_count; i++) {
        if (sheet->columns[i].name && strcmp(sheet->columns[i].name, name) == 0) {
            return &sheet->columns[i];
        }
    }
    return NULL;
}

int cdb_sheet_row_count(CDBSheet *sheet) {
    return sheet ? sheet->line_count : 0;
}

static int find_column_index(CDBSheet *sheet, const char *column) {
    if (!sheet || !column) return -1;
    for (int i = 0; i < sheet->column_count; i++) {
        if (sheet->columns[i].name && strcmp(sheet->columns[i].name, column) == 0) {
            return i;
        }
    }
    return -1;
}

const char *cdb_get_string(CDBSheet *sheet, int row, const char *column) {
    if (!sheet || !column) return NULL;
    int col_idx = find_column_index(sheet, column);
    if (col_idx < 0 || row < 0 || row >= sheet->line_count) return NULL;
    return sheet->lines[row][col_idx];
}

int cdb_set_string(CDBSheet *sheet, int row, const char *column, const char *value) {
    if (!sheet || !column) return -1;
    int col_idx = find_column_index(sheet, column);
    if (col_idx < 0 || row < 0 || row >= sheet->line_count) return -1;

    free(sheet->lines[row][col_idx]);
    sheet->lines[row][col_idx] = value ? strdup(value) : NULL;
    return 0;
}

int cdb_get_int(CDBSheet *sheet, int row, const char *column) {
    const char *val = cdb_get_string(sheet, row, column);
    return val ? atoi(val) : 0;
}

int cdb_set_int(CDBSheet *sheet, int row, const char *column, int value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", value);
    return cdb_set_string(sheet, row, column, buf);
}

float cdb_get_float(CDBSheet *sheet, int row, const char *column) {
    const char *val = cdb_get_string(sheet, row, column);
    return val ? (float)atof(val) : 0.0f;
}

int cdb_set_float(CDBSheet *sheet, int row, const char *column, float value) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%g", value);
    return cdb_set_string(sheet, row, column, buf);
}

int cdb_get_bool(CDBSheet *sheet, int row, const char *column) {
    const char *val = cdb_get_string(sheet, row, column);
    return val && (strcmp(val, "true") == 0 || strcmp(val, "1") == 0);
}

int cdb_set_bool(CDBSheet *sheet, int row, const char *column, int value) {
    return cdb_set_string(sheet, row, column, value ? "true" : "false");
}

const char *cdb_get_raw(CDBSheet *sheet, int row, const char *column) {
    return cdb_get_string(sheet, row, column);
}

int cdb_set_raw(CDBSheet *sheet, int row, const char *column, const char *value) {
    return cdb_set_string(sheet, row, column, value);
}

int cdb_value_is_null(CDBSheet *sheet, int row, const char *column) {
    const char *val = cdb_get_string(sheet, row, column);
    return val == NULL || strlen(val) == 0;
}

// Sheet column operations
int cdb_sheet_add_column(CDBSheet *sheet, CDBColumn *col, int position) {
    if (!sheet || !col) return -1;

    // Check if column name already exists
    if (cdb_sheet_column_by_name(sheet, col->name) != NULL) {
        return -1;
    }

    int new_count = sheet->column_count + 1;
    CDBColumn *new_cols = realloc(sheet->columns, new_count * sizeof(CDBColumn));
    if (!new_cols) return -1;
    sheet->columns = new_cols;

    int insert_pos = (position >= 0 && position < sheet->column_count) ? position : sheet->column_count;

    // Shift columns after insert position
    for (int i = sheet->column_count; i > insert_pos; i--) {
        sheet->columns[i] = sheet->columns[i - 1];
    }

    // Initialize new column
    memset(&sheet->columns[insert_pos], 0, sizeof(CDBColumn));
    sheet->columns[insert_pos].name = strdup(col->name);
    sheet->columns[insert_pos].type = col->type;
    sheet->columns[insert_pos].type_str = col->type_str ? strdup(col->type_str) : NULL;
    sheet->columns[insert_pos].is_optional = col->is_optional;
    sheet->columns[insert_pos].enum_values = col->enum_values;
    sheet->columns[insert_pos].enum_count = col->enum_count;
    sheet->columns[insert_pos].ref_sheet = col->ref_sheet ? strdup(col->ref_sheet) : NULL;

    sheet->column_count = new_count;

    // Initialize new column values for existing rows
    for (int i = 0; i < sheet->line_count; i++) {
        char **new_line = realloc(sheet->lines[i], new_count * sizeof(char*));
        if (!new_line) return -1;
        sheet->lines[i] = new_line;
        // Shift values after insert position
        for (int j = sheet->line_count - 1; j > insert_pos; j--) {
            sheet->lines[i][j] = sheet->lines[i][j - 1];
        }
        sheet->lines[i][insert_pos] = NULL;
    }

    return 0;
}

int cdb_sheet_delete_column(CDBSheet *sheet, const char *column_name) {
    if (!sheet || !column_name) return -1;

    int col_idx = find_column_index(sheet, column_name);
    if (col_idx < 0) return -1;

    // Free column data
    cdb_column_destroy(&sheet->columns[col_idx]);

    // Shift columns
    for (int i = col_idx; i < sheet->column_count - 1; i++) {
        sheet->columns[i] = sheet->columns[i + 1];
    }
    sheet->column_count--;

    // Shift values in all rows
    for (int i = 0; i < sheet->line_count; i++) {
        free(sheet->lines[i][col_idx]);
        for (int j = col_idx; j < sheet->column_count; j++) {
            sheet->lines[i][j] = sheet->lines[i][j + 1];
        }
    }

    return 0;
}

// Sheet row operations
int cdb_sheet_add_row(CDBSheet *sheet, int position) {
    if (!sheet) return -1;

    int new_count = sheet->line_count + 1;
    char ***new_lines = realloc(sheet->lines, new_count * sizeof(char**));
    if (!new_lines) return -1;
    sheet->lines = new_lines;

    int insert_pos = (position >= 0 && position < sheet->line_count) ? position : sheet->line_count;

    // Shift rows after insert position
    for (int i = sheet->line_count; i > insert_pos; i--) {
        sheet->lines[i] = sheet->lines[i - 1];
    }

    // Initialize new row
    sheet->lines[insert_pos] = calloc(sheet->column_count, sizeof(char*));
    if (!sheet->lines[insert_pos]) return -1;

    sheet->line_count = new_count;
    return 0;
}

int cdb_sheet_delete_row(CDBSheet *sheet, int row_index) {
    if (!sheet || row_index < 0 || row_index >= sheet->line_count) return -1;

    // Free row data
    for (int j = 0; j < sheet->column_count; j++) {
        free(sheet->lines[row_index][j]);
    }
    free(sheet->lines[row_index]);

    // Shift rows
    for (int i = row_index; i < sheet->line_count - 1; i++) {
        sheet->lines[i] = sheet->lines[i + 1];
    }
    sheet->line_count--;

    return 0;
}

int cdb_sheet_move_row(CDBSheet *sheet, int from, int to) {
    if (!sheet || from < 0 || from >= sheet->line_count || to < 0 || to >= sheet->line_count) return -1;
    if (from == to) return 0;

    // Swap rows
    char **temp = sheet->lines[from];
    sheet->lines[from] = sheet->lines[to];
    sheet->lines[to] = temp;

    return 0;
}