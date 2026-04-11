#include "cdb_internal.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int cdb_parse_type_str(CDBColumn *col, const char *type_str) {
    if (!col || !type_str) {
        return -1;
    }

    // Make a copy of the type string to work with
    char *copy = strdup(type_str);
    if (!copy) {
        return -1;
    }

    // Find the colon separator (if any)
    char *colon = strchr(copy, ':');
    char *value_part = NULL;

    if (colon) {
        *colon = '\0';
        value_part = colon + 1;
    }

    // Parse the integer type prefix
    int type_id = atoi(copy);
    col->type = (CDBColumnType)type_id;

    // Store the original type string
    col->type_str = strdup(type_str);
    if (!col->type_str) {
        free(copy);
        return -1;
    }

    // Handle types with extra data
    if (value_part && *value_part) {
        if (type_id == CDB_TENUM || type_id == CDB_TFLAGS) {
            // Split comma-separated values
            char *token = strtok(value_part, ",");
            int capacity = 4;
            col->enum_values = (char **)malloc(sizeof(char *) * capacity);
            if (!col->enum_values) {
                free(copy);
                return -1;
            }
            col->enum_count = 0;

            while (token) {
                if (col->enum_count >= capacity) {
                    capacity *= 2;
                    char **tmp = (char **)realloc(col->enum_values, sizeof(char *) * capacity);
                    if (!tmp) {
                        for (int i = 0; i < col->enum_count; i++) {
                            free(col->enum_values[i]);
                        }
                        free(col->enum_values);
                        free(copy);
                        return -1;
                    }
                    col->enum_values = tmp;
                }
                col->enum_values[col->enum_count] = strdup(token);
                if (!col->enum_values[col->enum_count]) {
                    for (int i = 0; i < col->enum_count; i++) {
                        free(col->enum_values[i]);
                    }
                    free(col->enum_values);
                    free(copy);
                    return -1;
                }
                col->enum_count++;
                token = strtok(NULL, ",");
            }
        } else if (type_id == CDB_TREF) {
            // Extract ref_sheet name
            col->ref_sheet = strdup(value_part);
            if (!col->ref_sheet) {
                free(copy);
                return -1;
            }
        }
    }

    free(copy);
    return 0;
}
