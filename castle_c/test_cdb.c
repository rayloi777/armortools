/*
 * CastleDB C - CLI Test Tool
 */

#include "sources/lib/cdb.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    const char *filepath = "/Users/rayloi/Documents/GitHub/castle/test/item.cdb";

    if (argc > 1) {
        filepath = argv[1];
    }

    printf("=== CastleDB CLI Test ===\n");
    printf("Opening: %s\n\n", filepath);

    // Create database
    CDB *db = cdb_create();
    if (!db) {
        printf("ERROR: Failed to create database\n");
        return 1;
    }

    // Load CDB file
    int result = cdb_load(db, filepath);
    if (result != 0) {
        printf("ERROR: Failed to load CDB file (error code: %d)\n", cdb_get_error(db));
        printf("Error string: %s\n", cdb_error_string(cdb_get_error(db)));
        cdb_close(db);
        return 1;
    }

    printf("SUCCESS: Loaded CDB file\n");
    printf("Sheet count: %d\n\n", cdb_get_sheet_count(db));

    // List all sheets
    printf("=== Sheets ===\n");
    for (int i = 0; i < cdb_get_sheet_count(db); i++) {
        CDBSheet *sheet = cdb_get_sheet(db, i);
        if (sheet) {
            printf("  [%d] %s (columns: %d, rows: %d)\n",
                   i, cdb_sheet_name(sheet),
                   cdb_sheet_column_count(sheet),
                   cdb_sheet_row_count(sheet));
        }
    }
    printf("\n");

    // Get first sheet
    CDBSheet *sheet = cdb_get_sheet(db, 0);
    if (!sheet) {
        printf("ERROR: No sheets found\n");
        cdb_close(db);
        return 1;
    }

    printf("=== First Sheet: %s ===\n", cdb_sheet_name(sheet));

    // List columns
    printf("Columns:\n");
    for (int i = 0; i < cdb_sheet_column_count(sheet); i++) {
        CDBColumn *col = cdb_sheet_column(sheet, i);
        if (col) {
            printf("  [%d] %s (type: %d)\n",
                   i, cdb_column_name(col), cdb_column_type(col));
        }
    }
    printf("\n");

    // Print first few rows
    int row_count = cdb_sheet_row_count(sheet);
    printf("Rows: %d (showing first 5)\n", row_count);
    for (int r = 0; r < (row_count < 5 ? row_count : 5); r++) {
        printf("  Row %d:", r);
        for (int c = 0; c < cdb_sheet_column_count(sheet); c++) {
            CDBColumn *col = cdb_sheet_column(sheet, c);
            const char *val = cdb_get_string(sheet, r, cdb_column_name(col));
            printf(" | %s", val ? val : "(null)");
        }
        printf("\n");
    }

    cdb_close(db);
    printf("\n=== Test Complete ===\n");
    return 0;
}
