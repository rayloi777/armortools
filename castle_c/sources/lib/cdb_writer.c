/*
 * CastleDB C JSON Writer
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
#include <stdio.h>
#include <string.h>

// JSON encoding buffer
typedef struct {
    char *buffer;
    int capacity;
    int length;
} JsonEncode;

static void json_enc_init(JsonEncode *enc) {
    enc->capacity = 65536;
    enc->buffer = (char*)malloc(enc->capacity);
    enc->buffer[0] = '\0';
    enc->length = 0;
}

static void json_enc_free(JsonEncode *enc) {
    if (enc->buffer) free(enc->buffer);
    enc->buffer = NULL;
}

static void json_enc_ensure(JsonEncode *enc, int needed) {
    while (enc->length + needed >= enc->capacity) {
        enc->capacity *= 2;
        enc->buffer = (char*)realloc(enc->buffer, enc->capacity);
    }
}

static void json_enc_append(JsonEncode *enc, const char *str) {
    int len = strlen(str);
    json_enc_ensure(enc, len + 1);
    strcpy(enc->buffer + enc->length, str);
    enc->length += len;
}

static void json_enc_comma(JsonEncode *enc) {
    if (enc->length > 0 && enc->buffer[enc->length - 1] != '{' && enc->buffer[enc->length - 1] != '[') {
        json_enc_append(enc, ",");
    }
}

static void json_enc_string(JsonEncode *enc, const char *str) {
    json_enc_append(enc, "\"");
    if (str) {
        // Escape special characters
        for (const char *p = str; *p; p++) {
            char esc[8];
            switch (*p) {
                case '"': strcpy(esc, "\\\""); break;
                case '\\': strcpy(esc, "\\\\"); break;
                case '\n': strcpy(esc, "\\n"); break;
                case '\r': strcpy(esc, "\\r"); break;
                case '\t': strcpy(esc, "\\t"); break;
                default:
                    esc[0] = *p;
                    esc[1] = '\0';
                    break;
            }
            json_enc_append(enc, esc);
        }
    }
    json_enc_append(enc, "\"");
}

static void json_enc_cstring(JsonEncode *enc, const char *key, const char *value) {
    json_enc_comma(enc);
    json_enc_string(enc, key);
    json_enc_append(enc, ":");
    json_enc_string(enc, value);
}

static void json_enc_begin_object(JsonEncode *enc) {
    json_enc_comma(enc);
    json_enc_append(enc, "{");
}

static void json_enc_end_object(JsonEncode *enc) {
    json_enc_append(enc, "}");
}

static void json_enc_begin_array(JsonEncode *enc, const char *key) {
    json_enc_comma(enc);
    if (key) {
        json_enc_string(enc, key);
        json_enc_append(enc, ":[");
    } else {
        json_enc_append(enc, "[");
    }
}

static void json_enc_end_array(JsonEncode *enc) {
    json_enc_append(enc, "]");
}

static void encode_sheet(JsonEncode *enc, CDBSheet *sheet);
static void encode_column(JsonEncode *enc, CDBColumn *col);
static void encode_line(JsonEncode *enc, char **line, CDBColumn *columns, int column_count);

int cdb_save(CDB *db, const char *filepath) {
    if (!db || !filepath) {
        return -1;
    }

    JsonEncode enc;
    json_enc_init(&enc);

    // Begin object
    json_enc_append(&enc, "{\"sheets\":[");

    for (int i = 0; i < db->sheet_count; i++) {
        if (i > 0) json_enc_append(&enc, ",");
        encode_sheet(&enc, &db->sheets[i]);
    }

    json_enc_append(&enc, "]}");

    // Write to file
    FILE *f = fopen(filepath, "wb");
    if (!f) {
        json_enc_free(&enc);
        db->error = CDB_ERR_FILE_WRITE;
        return -1;
    }

    size_t written = fwrite(enc.buffer, 1, enc.length, f);
    fclose(f);
    json_enc_free(&enc);

    if ((int)written != enc.length) {
        db->error = CDB_ERR_FILE_WRITE;
        return -1;
    }

    return 0;
}

static void encode_sheet(JsonEncode *enc, CDBSheet *sheet) {
    if (!sheet) return;

    json_enc_begin_object(enc);

    // Encode sheet name
    if (sheet->name) {
        json_enc_cstring(enc, "name", sheet->name);
    } else {
        json_enc_cstring(enc, "name", "");
    }

    // Encode columns array
    json_enc_begin_array(enc, "columns");
    for (int i = 0; i < sheet->column_count; i++) {
        if (i > 0) json_enc_append(enc, ",");
        encode_column(enc, &sheet->columns[i]);
    }
    json_enc_end_array(enc);

    // Encode lines array
    json_enc_begin_array(enc, "lines");
    for (int i = 0; i < sheet->line_count; i++) {
        if (i > 0) json_enc_append(enc, ",");
        encode_line(enc, sheet->lines[i], sheet->columns, sheet->column_count);
    }
    json_enc_end_array(enc);

    json_enc_end_object(enc);
}

static void encode_column(JsonEncode *enc, CDBColumn *col) {
    if (!col) return;

    json_enc_begin_object(enc);

    // Encode column name
    if (col->name) {
        json_enc_cstring(enc, "name", col->name);
    } else {
        json_enc_cstring(enc, "name", "");
    }

    // Encode type string (e.g., "5:value1,value2" for TEnum)
    if (col->type_str) {
        json_enc_cstring(enc, "typeStr", col->type_str);
    } else {
        char default_type[2];
        snprintf(default_type, sizeof(default_type), "%d", CDB_TSTRING);
        json_enc_cstring(enc, "typeStr", default_type);
    }

    // Encode optional flag
    json_enc_cstring(enc, "opt", col->is_optional ? "true" : "false");

    json_enc_end_object(enc);
}

static void encode_line(JsonEncode *enc, char **line, CDBColumn *columns, int column_count) {
    if (!line) return;

    json_enc_begin_object(enc);

    for (int i = 0; i < column_count; i++) {
        if (columns[i].name) {
            if (line[i]) {
                json_enc_comma(enc);
                json_enc_string(enc, columns[i].name);
                json_enc_append(enc, ":");
                json_enc_string(enc, line[i]);
            }
        }
    }

    json_enc_end_object(enc);
}
