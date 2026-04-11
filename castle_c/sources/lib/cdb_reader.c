/*
 * CastleDB C JSON Reader - Simplified JSMN-based parser
 */

#include "cdb_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define JSMN_HEADER
#include "jsmn.h"

static int parse_type_string(CDBColumn *col, const char *type_str);
static int parse_enum_values(const char *enum_str, char ***values, int *count);

// Forward declarations
static int skip_to_end(int idx, jsmntok_t *tokens, int token_count);
static int find_in_object(int obj_idx, const char *key, jsmntok_t *tokens, int token_count, const char *json);

// Skip to end of container (object or array)
static int skip_to_end(int idx, jsmntok_t *tokens, int token_count) {
    if (idx < 0 || idx >= token_count) return idx + 1;

    jsmntype_t type = tokens[idx].type;

    if (type == JSMN_STRING || type == JSMN_PRIMITIVE) {
        return idx + 1;
    }
    if (type == JSMN_OBJECT) {
        int pairs = tokens[idx].size;
        int i = idx + 1;
        for (int p = 0; p < pairs && i < token_count; p++) {
            // Skip key (usually STRING)
            i = skip_to_end(i, tokens, token_count);
            if (i >= token_count) return token_count;
            // Skip value
            i = skip_to_end(i, tokens, token_count);
        }
        return i;
    }
    if (type == JSMN_ARRAY) {
        int elems = tokens[idx].size;
        int i = idx + 1;
        for (int e = 0; e < elems && i < token_count; e++) {
            i = skip_to_end(i, tokens, token_count);
        }
        // Array ends at the next token after last element
        // JSMN doesn't have explicit ] token in some cases
        return i;
    }
    return idx + 1;
}

// Find key's value index in object
static int find_in_object(int obj_idx, const char *key, jsmntok_t *tokens, int token_count, const char *json) {
    if (tokens[obj_idx].type != JSMN_OBJECT) return -1;

    int pairs = tokens[obj_idx].size;
    int i = obj_idx + 1;

    for (int p = 0; p < pairs && i < token_count; p++) {
        // tokens[i] is the key (typically STRING)
        if (tokens[i].type == JSMN_STRING) {
            int key_len = tokens[i].end - tokens[i].start;
            if ((int)strlen(key) == key_len && strncmp(json + tokens[i].start, key, key_len) == 0) {
                // Found it - return the NEXT token index (the value)
                return i + 1;
            }
        }
        // Skip key
        i = skip_to_end(i, tokens, token_count);
        if (i >= token_count) return -1;
        // Skip value
        i = skip_to_end(i, tokens, token_count);
        if (i >= token_count) return -1;
    }
    return -1;
}

// Get string from STRING token
static char *tok_string(int idx, const char *json, jsmntok_t *tokens) {
    if (idx < 0 || tokens[idx].type != JSMN_STRING) return NULL;
    int len = tokens[idx].end - tokens[idx].start;
    if (len <= 0) return NULL;
    char *s = malloc(len + 1);
    if (s) {
        memcpy(s, json + tokens[idx].start, len);
        s[len] = '\0';
    }
    return s;
}

// Get primitive as string
static char *tok_primitive(int idx, const char *json, jsmntok_t *tokens) {
    if (idx < 0 || tokens[idx].type != JSMN_PRIMITIVE) return NULL;
    int len = tokens[idx].end - tokens[idx].start;
    if (len <= 0) return NULL;
    char *s = malloc(len + 1);
    if (s) {
        memcpy(s, json + tokens[idx].start, len);
        s[len] = '\0';
    }
    return s;
}

// Parse column
static void parse_column(CDBColumn *col, int obj_idx, jsmntok_t *tokens, int token_count, const char *json) {
    col->name = NULL;
    col->type = CDB_TSTRING;
    col->type_str = NULL;
    col->enum_values = NULL;
    col->enum_count = 0;
    col->ref_sheet = NULL;
    col->is_optional = 0;

    int v = find_in_object(obj_idx, "name", tokens, token_count, json);
    if (v >= 0) col->name = tok_string(v, json, tokens);

    v = find_in_object(obj_idx, "typeStr", tokens, token_count, json);
    if (v >= 0) {
        col->type_str = tok_string(v, json, tokens);
        if (col->type_str) parse_type_string(col, col->type_str);
    }

    v = find_in_object(obj_idx, "opt", tokens, token_count, json);
    if (v >= 0) {
        char *opt = tok_primitive(v, json, tokens);
        if (opt) {
            col->is_optional = (strcmp(opt, "true") == 0 || strcmp(opt, "1") == 0);
            free(opt);
        }
    }
}

// Parse line (row)
static void parse_line(CDBSheet *sheet, int obj_idx, int line_idx, jsmntok_t *tokens, int token_count, const char *json) {
    sheet->lines[line_idx] = calloc(sheet->column_count, sizeof(char*));

    for (int c = 0; c < sheet->column_count; c++) {
        if (!sheet->columns[c].name) continue;
        int v = find_in_object(obj_idx, sheet->columns[c].name, tokens, token_count, json);
        if (v >= 0) {
            if (tokens[v].type == JSMN_STRING) {
                sheet->lines[line_idx][c] = tok_string(v, json, tokens);
            } else if (tokens[v].type == JSMN_PRIMITIVE) {
                sheet->lines[line_idx][c] = tok_primitive(v, json, tokens);
            }
        }
    }
}

// Parse sheet
static void parse_sheet(CDBSheet *sheet, int obj_idx, jsmntok_t *tokens, int token_count, const char *json) {
    sheet->name = NULL;
    sheet->columns = NULL;
    sheet->column_count = 0;
    sheet->lines = NULL;
    sheet->line_count = 0;
    sheet->props = NULL;
    sheet->hidden = 0;

    // Name
    int v = find_in_object(obj_idx, "name", tokens, token_count, json);
    if (v >= 0) sheet->name = tok_string(v, json, tokens);

    // Columns array
    v = find_in_object(obj_idx, "columns", tokens, token_count, json);
    if (v >= 0 && tokens[v].type == JSMN_ARRAY) {
        int num_cols = tokens[v].size;
        sheet->column_count = num_cols;
        sheet->columns = calloc(num_cols, sizeof(CDBColumn));

        int i = v + 1;
        for (int c = 0; c < num_cols && i < token_count; c++) {
            // Find next object
            while (i < token_count && tokens[i].type != JSMN_OBJECT) i++;
            if (i >= token_count) break;
            parse_column(&sheet->columns[c], i, tokens, token_count, json);
            i = skip_to_end(i, tokens, token_count); // skip column object
        }
    }

    // Lines array
    v = find_in_object(obj_idx, "lines", tokens, token_count, json);
    if (v >= 0 && tokens[v].type == JSMN_ARRAY) {
        int num_lines = tokens[v].size;
        sheet->line_count = num_lines;
        if (num_lines > 0) {
            sheet->lines = calloc(num_lines, sizeof(char**));
        }

        int i = v + 1;
        for (int l = 0; l < num_lines && i < token_count; l++) {
            // Find next object
            while (i < token_count && tokens[i].type != JSMN_OBJECT) i++;
            if (i >= token_count) break;
            parse_line(sheet, i, l, tokens, token_count, json);
            i = skip_to_end(i, tokens, token_count); // skip line object
        }
    }
}

int cdb_load(CDB *db, const char *filepath) {
    if (!db || !filepath) return -1;

    FILE *f = fopen(filepath, "rb");
    if (!f) { db->error = CDB_ERR_FILE_NOT_FOUND; return -1; }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size == 0) { fclose(f); db->error = CDB_ERR_FILE_READ; return -1; }

    char *json = malloc(size + 1);
    if (!json) { fclose(f); db->error = CDB_ERR_OUT_OF_MEMORY; return -1; }

    size_t read_size = fread(json, 1, size, f);
    fclose(f);
    if (read_size != (size_t)size) { free(json); db->error = CDB_ERR_FILE_READ; return -1; }
    json[size] = '\0';

    jsmn_parser parser;
    jsmn_init(&parser);
    jsmntok_t tokens[8192];
    int count = jsmn_parse(&parser, json, size, tokens, 8192);
    if (count < 0) { free(json); db->error = CDB_ERR_JSON_PARSE; return -1; }

    if (count < 1 || tokens[0].type != JSMN_OBJECT) { free(json); db->error = CDB_ERR_INVALID_CDB; return -1; }

    int sheets_idx = find_in_object(0, "sheets", tokens, count, json);
    if (sheets_idx < 0 || tokens[sheets_idx].type != JSMN_ARRAY) { free(json); db->error = CDB_ERR_INVALID_CDB; return -1; }

    int num_sheets = tokens[sheets_idx].size;
    db->sheet_count = num_sheets;
    db->sheets = calloc(num_sheets, sizeof(CDBSheet));

    int i = sheets_idx + 1;
    for (int s = 0; s < num_sheets && i < count; s++) {
        while (i < count && tokens[i].type != JSMN_OBJECT) i++;
        if (i >= count) break;
        parse_sheet(&db->sheets[s], i, tokens, count, json);
        i = skip_to_end(i, tokens, count); // skip sheet object
    }

    free(json);
    return 0;
}

static int parse_type_string(CDBColumn *col, const char *type_str) {
    if (!type_str || !col) return -1;

    char buffer[256];
    strncpy(buffer, type_str, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    col->type = (CDBColumnType)atoi(buffer);

    char *colon = strchr(buffer, ':');
    if (colon) {
        *colon = '\0';
        if (col->type == CDB_TENUM || col->type == CDB_TFLAGS) {
            parse_enum_values(colon + 1, &col->enum_values, &col->enum_count);
        } else if (col->type == CDB_TREF) {
            col->ref_sheet = strdup(colon + 1);
        }
    }
    return 0;
}

static int parse_enum_values(const char *enum_str, char ***values, int *count) {
    if (!enum_str || !values || !count) return -1;

    *values = malloc(sizeof(char*) * 4);
    int capacity = 4;
    int cnt = 0;

    char buffer[1024];
    strncpy(buffer, enum_str, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    char *token = strtok(buffer, ",");
    while (token) {
        while (*token == ' ') token++;
        if (cnt >= capacity) {
            capacity *= 2;
            *values = realloc(*values, sizeof(char*) * capacity);
        }
        (*values)[cnt++] = strdup(token);
        token = strtok(NULL, ",");
    }
    *count = cnt;
    return 0;
}
