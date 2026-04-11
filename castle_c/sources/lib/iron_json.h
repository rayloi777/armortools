#pragma once

#include <stddef.h>
#include "jsmn.h"

// Forward declarations
struct AnyValue;
struct AnyArray;
struct AnyMap;

typedef struct AnyValue AnyValue;
typedef struct AnyArray AnyArray;
typedef struct AnyMap AnyMap;

struct AnyArray {
    AnyValue *buffer;
    int length;
    int capacity;
};

struct AnyMap {
    AnyValue *keys;
    AnyValue *values;
    int count;
    const char *json_str;  // Store for string extraction
};

struct AnyValue {
    jsmntype_t type;
    union {
        char *string;
        double number;
        int boolean;
    } val;
    int start;
    int end;
};

AnyMap *json_parse_to_map(const char *json);
void json_free_map(AnyMap *map);
void *any_map_get(AnyMap *map, const char *key);
AnyArray *any_array_get(AnyMap *map, int index);
