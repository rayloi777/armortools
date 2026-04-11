#pragma once

#include <stddef.h>
#include <stdlib.h>

// Minimal i32 array implementation
typedef struct {
    int *data;
    int length;
    int capacity;
} i32_array_t;

static inline i32_array_t *i32_array_create(int initial_capacity) {
    i32_array_t *arr = (i32_array_t*)calloc(1, sizeof(i32_array_t));
    if (!arr) return NULL;
    arr->capacity = (initial_capacity > 0) ? initial_capacity : 16;
    arr->data = (int*)malloc(arr->capacity * sizeof(int));
    if (!arr->data) {
        free(arr);
        return NULL;
    }
    arr->length = 0;
    return arr;
}

static inline void i32_array_destroy(i32_array_t *arr) {
    if (!arr) return;
    if (arr->data) free(arr->data);
    free(arr);
}

static inline int i32_array_push(i32_array_t *arr, int value) {
    if (!arr) return -1;
    if (arr->length >= arr->capacity) {
        arr->capacity *= 2;
        int *new_data = (int*)realloc(arr->data, arr->capacity * sizeof(int));
        if (!new_data) return -1;
        arr->data = new_data;
    }
    arr->data[arr->length++] = value;
    return 0;
}
