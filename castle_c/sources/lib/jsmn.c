/*
 * CastleDB C - Standalone JSON Parser
 * Uses JSMN (lightweight JSON parser)
 */

#include "jsmn.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Forward declare functions from jsmn.h
void jsmn_init(jsmn_parser *parser);
int jsmn_parse(jsmn_parser *parser, const char *js, size_t len, jsmntok_t *tokens, unsigned int num_tokens);

// Extended JSON parser for CDB
typedef struct {
    jsmn_parser parser;
    jsmntok_t *tokens;
    int token_count;
    int token_capacity;
    const char *json_str;
} CDBJsonParser;

CDBJsonParser *cdb_json_create(void) {
    CDBJsonParser *p = (CDBJsonParser *)calloc(1, sizeof(CDBJsonParser));
    if (!p) return NULL;
    p->token_capacity = 256;
    p->tokens = (jsmntok_t *)malloc(sizeof(jsmntok_t) * p->token_capacity);
    if (!p->tokens) {
        free(p);
        return NULL;
    }
    jsmn_init(&p->parser);
    return p;
}

void cdb_json_destroy(CDBJsonParser *p) {
    if (!p) return;
    free(p->tokens);
    free(p);
}

int cdb_json_parse(CDBJsonParser *p, const char *json, int json_len) {
    if (!p || !json) return -1;
    p->json_str = json;

    // Expand token buffer if needed
    int needed = json_len / 4 + 10; // Rough estimate
    if (needed > p->token_capacity) {
        needed = needed * 2;
        jsmntok_t *new_tokens = (jsmntok_t *)realloc(p->tokens, sizeof(jsmntok_t) * needed);
        if (!new_tokens) return -1;
        p->tokens = new_tokens;
        p->token_capacity = needed;
    }

    jsmn_init(&p->parser);
    int result = jsmn_parse(&p->parser, json, json_len, p->tokens, p->token_capacity);
    if (result >= 0) {
        p->token_count = result;
    }
    return result;
}

const char *cdb_json_get_string(CDBJsonParser *p, const char *key) {
    if (!p || !p->json_str || !key) return NULL;

    // Find the object containing this key
    for (int i = 0; i < p->token_count - 1; i++) {
        if (p->tokens[i].type == JSMN_STRING &&
            p->tokens[i].size == 1) {
            // Check if this key matches
            int key_len = p->tokens[i].end - p->tokens[i].start;
            const char *key_start = p->json_str + p->tokens[i].start;
            if (strncmp(key_start, key, key_len) == 0 && strlen(key) == (size_t)key_len) {
                // Found the key, next token should be the value
                if (p->tokens[i + 1].type == JSMN_STRING) {
                    int val_len = p->tokens[i + 1].end - p->tokens[i + 1].start;
                    char *result = (char *)malloc(val_len + 1);
                    if (result) {
                        strncpy(result, p->json_str + p->tokens[i + 1].start, val_len);
                        result[val_len] = '\0';
                    }
                    return result;
                }
            }
        }
    }
    return NULL;
}

int cdb_json_get_int(CDBJsonParser *p, const char *key, int default_val) {
    if (!p || !p->json_str || !key) return default_val;

    for (int i = 0; i < p->token_count - 1; i++) {
        if (p->tokens[i].type == JSMN_STRING &&
            p->tokens[i].size == 1) {
            int key_len = p->tokens[i].end - p->tokens[i].start;
            const char *key_start = p->json_str + p->tokens[i].start;
            if (strncmp(key_start, key, key_len) == 0 && strlen(key) == (size_t)key_len) {
                if (p->tokens[i + 1].type == JSMN_PRIMITIVE) {
                    int val_len = p->tokens[i + 1].end - p->tokens[i + 1].start;
                    char *buf = (char *)malloc(val_len + 1);
                    if (buf) {
                        strncpy(buf, p->json_str + p->tokens[i + 1].start, val_len);
                        buf[val_len] = '\0';
                        int result = atoi(buf);
                        free(buf);
                        return result;
                    }
                }
            }
        }
    }
    return default_val;
}

const char *cdb_json_get_type_str(CDBJsonParser *p, const char *type_key) {
    if (!p || !p->json_str || !type_key) return NULL;

    // Find the type string property
    for (int i = 0; i < p->token_count - 1; i++) {
        if (p->tokens[i].type == JSMN_STRING &&
            p->tokens[i].size == 1) {
            int key_len = p->tokens[i].end - p->tokens[i].start;
            const char *key_start = p->json_str + p->tokens[i].start;
            if (strncmp(key_start, type_key, key_len) == 0 && strlen(type_key) == (size_t)key_len) {
                if (p->tokens[i + 1].type == JSMN_STRING) {
                    int val_len = p->tokens[i + 1].end - p->tokens[i + 1].start;
                    char *result = (char *)malloc(val_len + 1);
                    if (result) {
                        strncpy(result, p->json_str + p->tokens[i + 1].start, val_len);
                        result[val_len] = '\0';
                    }
                    return result;
                }
            }
        }
    }
    return NULL;
}

int cdb_json_count_tokens(CDBJsonParser *p) {
    return p ? p->token_count : 0;
}

jsmntok_t *cdb_json_get_token(CDBJsonParser *p, int index) {
    if (!p || index < 0 || index >= p->token_count) return NULL;
    return &p->tokens[index];
}

const char *cdb_json_get_str_ptr(CDBJsonParser *p, jsmntok_t *tok) {
    if (!p || !tok || !p->json_str) return NULL;
    return p->json_str + tok->start;
}

int cdb_json_get_str_len(CDBJsonParser *p, jsmntok_t *tok) {
    if (!p || !tok) return 0;
    return tok->end - tok->start;
}
