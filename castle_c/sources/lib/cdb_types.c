#include "cdb_internal.h"
#include "iron_array.h"
#include "lz4x.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

// Gradient format: {"colors":[0xFF0000,0x00FF00], "positions":[0.0,1.0]}

typedef struct {
    int *colors;
    float *positions;
    int count;
} CDBGradient;

CDBGradient *cdb_gradient_parse(const char *str) {
    if (!str) return NULL;

    CDBGradient *g = (CDBGradient *)calloc(1, sizeof(CDBGradient));
    if (!g) return NULL;

    // Find colors array
    const char *colors_start = strstr(str, "\"colors\"");
    if (!colors_start) {
        free(g);
        return NULL;
    }
    colors_start = strchr(colors_start, '[');
    if (!colors_start) {
        free(g);
        return NULL;
    }
    colors_start++;

    // Find positions array
    const char *positions_start = strstr(str, "\"positions\"");
    if (!positions_start) {
        free(g);
        return NULL;
    }
    positions_start = strchr(positions_start, '[');
    if (!positions_start) {
        free(g);
        return NULL;
    }
    positions_start++;

    // Parse colors
    int capacity = 4;
    g->colors = (int *)malloc(capacity * sizeof(int));
    g->positions = (float *)malloc(capacity * sizeof(float));
    g->count = 0;

    const char *p = colors_start;
    while (*p && *p != ']') {
        while (*p == ' ' || *p == '\t' || *p == '\n') p++;
        if (*p == ']') break;
        if (*p == ',') {
            p++;
            continue;
        }

        // Parse hex color like 0xFF0000
        if (p[0] == '0' && p[1] == 'x') {
            int color = (int)strtol(p + 2, NULL, 16);
            if (g->count >= (int)capacity) {
                capacity *= 2;
                g->colors = (int *)realloc(g->colors, capacity * sizeof(int));
                g->positions = (float *)realloc(g->positions, capacity * sizeof(float));
            }
            g->colors[g->count] = color;
            g->count++;
        }

        // Skip to next token
        while (*p && *p != ',' && *p != ']') p++;
        if (*p == ',') p++;
    }

    // Parse positions
    const char *pp = positions_start;
    int pos_count = 0;
    while (*pp && *pp != ']' && pos_count < g->count) {
        while (*pp == ' ' || *pp == '\t' || *pp == '\n') pp++;
        if (*pp == ']') break;
        if (*pp == ',') {
            pp++;
            continue;
        }

        // Parse float
        float val = (float)atof(pp);
        g->positions[pos_count] = val;
        pos_count++;

        // Skip to next token
        while (*pp && *pp != ',' && *pp != ']') pp++;
        if (*pp == ',') pp++;
    }

    return g;
}

void cdb_gradient_free(CDBGradient *g) {
    if (!g) return;
    if (g->colors) free(g->colors);
    if (g->positions) free(g->positions);
    free(g);
}

int *cdb_gradient_generate(CDBGradient *g, int count) {
    if (!g || count <= 0 || g->count < 2) return NULL;

    int *result = (int *)malloc(count * sizeof(int));
    if (!result) return NULL;

    // Generate interpolated colors
    for (int i = 0; i < count; i++) {
        float t = (float)i / (count - 1);

        // Find the two positions we're between
        int idx = 0;
        for (int j = 0; j < g->count - 1; j++) {
            if (t >= g->positions[j] && t <= g->positions[j + 1]) {
                idx = j;
                break;
            }
        }

        // Interpolate between colors
        float range = g->positions[idx + 1] - g->positions[idx];
        float local_t = 0;
        if (range > 0.0001f) {
            local_t = (t - g->positions[idx]) / range;
        }

        int c1 = g->colors[idx];
        int c2 = g->colors[idx + 1];

        int r1 = (c1 >> 16) & 0xFF;
        int g1 = (c1 >> 8) & 0xFF;
        int b1 = c1 & 0xFF;

        int r2 = (c2 >> 16) & 0xFF;
        int g2 = (c2 >> 8) & 0xFF;
        int b2 = c2 & 0xFF;

        int r = (int)(r1 + (r2 - r1) * local_t);
        int gg = (int)(g1 + (g2 - g1) * local_t);
        int b = (int)(b1 + (b2 - b1) * local_t);

        result[i] = (r << 16) | (gg << 8) | b;
    }

    return result;
}

// TileLayer format: LZ4 compressed 16-bit tile indices
// Structure: {file: "tiles.png", stride: 16, size: 256, data: "LZ4_BASE64..."}

typedef struct {
    char *file;
    int stride;
    int size;
    char *data;  // LZ4 compressed
} CDBTileLayer;

static char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int base64_encode(const char *src, int src_len, char *dst) {
    int i = 0, j = 0;
    int output_len = 0;

    while (i < src_len) {
        unsigned int octet_a = i < src_len ? (unsigned char)src[i++] : 0;
        unsigned int octet_b = i < src_len ? (unsigned char)src[i++] : 0;
        unsigned int octet_c = i < src_len ? (unsigned char)src[i++] : 0;

        unsigned int triple = (octet_a << 16) + (octet_b << 8) + octet_c;

        dst[j++] = base64_chars[(triple >> 18) & 0x3F];
        dst[j++] = base64_chars[(triple >> 12) & 0x3F];
        dst[j++] = base64_chars[(triple >> 6) & 0x3F];
        dst[j++] = base64_chars[triple & 0x3F];
    }

    // Padding
    int pads = (3 - (src_len % 3)) % 3;
    for (int p = 0; p < pads; p++) {
        dst[j - 1 - p] = '=';
    }

    dst[j] = '\0';
    return j;
}

static int base64_decode(const char *src, int src_len, char *dst) {
    static int8_t lookup[256];
    static int lookup_init = 0;

    if (!lookup_init) {
        memset(lookup, -1, sizeof(lookup));
        for (int i = 0; base64_chars[i]; i++) {
            lookup[(unsigned char)base64_chars[i]] = i;
        }
        for (int i = 0; i < 10; i++) lookup['0' + i] = 52 + i;
        for (int i = 0; i < 26; i++) lookup['A' + i] = i;
        for (int i = 0; i < 26; i++) lookup['a' + i] = 26 + i;
        lookup_init = 1;
    }

    int i = 0, j = 0;
    int output_len = 0;

    // Remove padding
    while (src_len > 0 && src[src_len - 1] == '=') src_len--;

    while (i < src_len) {
        int a = lookup[(unsigned char)src[i++]];
        int b = i < src_len ? lookup[(unsigned char)src[i++]] : 0;
        int c = i < src_len ? lookup[(unsigned char)src[i++]] : 0;
        int d = i < src_len ? lookup[(unsigned char)src[i++]] : 0;

        unsigned int triple = (a << 18) + (b << 12) + (c << 6) + d;

        if (j < src_len * 3 / 4) dst[j++] = (triple >> 16) & 0xFF;
        if (j < src_len * 3 / 4) dst[j++] = (triple >> 8) & 0xFF;
        if (j < src_len * 3 / 4) dst[j++] = triple & 0xFF;
    }

    return j;
}

i32_array_t *cdb_tilelayer_decode(const char *data, int *out_count) {
    if (!data || !out_count) return NULL;

    *out_count = 0;

    // Parse JSON to extract metadata and compressed data
    const char *file_start = strstr(data, "\"file\"");
    const char *stride_start = strstr(data, "\"stride\"");
    const char *size_start = strstr(data, "\"size\"");
    const char *data_start = strstr(data, "\"data\"");

    if (!data_start) return NULL;

    // Skip to value
    data_start = strchr(data_start, ':');
    if (!data_start) return NULL;
    data_start++;

    // Skip whitespace and quotes
    while (*data_start == ' ' || *data_start == '\"') data_start++;

    // Find end of string (next quote)
    const char *data_end = data_start;
    while (*data_end && *data_end != '\"') data_end++;

    int data_len = (int)(data_end - data_start);

    // Allocate buffer for decoded base64
    int max_decoded = data_len * 3 / 4 + 1;
    char *decoded = (char *)malloc(max_decoded);
    if (!decoded) return NULL;

    int decoded_len = base64_decode(data_start, data_len, decoded);

    // Decompress with LZ4
    int max_decompressed = 256 * 256 * 2;  // Max tile layer size
    char *decompressed = (char *)malloc(max_decompressed);
    if (!decompressed) {
        free(decoded);
        return NULL;
    }

    int result = LZ4_decompress_safe(decoded, decompressed, decoded_len, max_decompressed);
    free(decoded);

    if (result < 0) {
        free(decompressed);
        return NULL;
    }

    int tile_count = result / sizeof(int16_t);
    *out_count = tile_count;

    // Convert to i32_array_t
    i32_array_t *arr = i32_array_create(tile_count);
    if (!arr) {
        free(decompressed);
        return NULL;
    }

    int16_t *tiles = (int16_t *)decompressed;
    for (int i = 0; i < tile_count; i++) {
        i32_array_push(arr, tiles[i]);
    }

    free(decompressed);
    return arr;
}

char *cdb_tilelayer_encode(int *tiles, int count) {
    if (!tiles || count <= 0) return NULL;

    // Compress with LZ4
    int max_compressed = LZ4_compress_bound(count * sizeof(int16_t));
    char *compressed = (char *)malloc(max_compressed);
    if (!compressed) return NULL;

    int16_t *tiles16 = (int16_t *)malloc(count * sizeof(int16_t));
    if (!tiles16) {
        free(compressed);
        return NULL;
    }

    for (int i = 0; i < count; i++) {
        tiles16[i] = (int16_t)tiles[i];
    }

    int compressed_len = (int)LZ4_compress_default((char *)tiles16, compressed, count * sizeof(int16_t), max_compressed);
    free(tiles16);

    if (compressed_len <= 0) {
        free(compressed);
        return NULL;
    }

    // Base64 encode
    int base64_len = (compressed_len * 4 / 3) + 4;
    char *base64 = (char *)malloc(base64_len);
    if (!base64) {
        free(compressed);
        return NULL;
    }

    base64_encode(compressed, compressed_len, base64);
    free(compressed);

    // Build JSON string
    char *result = (char *)malloc(256 + base64_len);
    if (!result) {
        free(base64);
        return NULL;
    }

    snprintf(result, 256 + base64_len,
        "{\"file\":\"tiles.png\",\"stride\":16,\"size\":%d,\"data\":\"%s\"}",
        (int)sqrt(count), base64);

    free(base64);
    return result;
}