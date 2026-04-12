// Stubs for Iron symbols that test doesn't need
// These satisfy linker references when building without full Iron app framework

#include <stddef.h>

char _temp_string[1024];

// engine.o and iron_sys.o reference these but don't use them in our test
int pipes_get_constant_location(char *s) { return 0; }
int pipes_offset = 0;

// iron_ui_nodes.o references these
char *tr(char *id) { return id; }
void *ui_children = NULL;
void *ui_nodes_custom_buttons = NULL;

// iron_file.o references this
char *strings_check_internet_connection(void) { return ""; }

// console functions
void console_error(char *s) {}
void console_info(char *s) {}

// Update callback - iron_sys.o calls this but we don't need it
void _iron_set_update_callback(void (*callback)(void)) { (void)callback; }
void _iron_internal_update_callback(void (*callback)(void)) { (void)callback; }

// LZ4 stubs - cdb_types.c references these but we don't use tilelayer in test
int LZ4_compressBound(int srcSize) { return srcSize + srcSize / 16 + 64; }
int LZ4_compress_default(const char *src, char *dst, int srcSize, int maxDstSize) {
    // Stub: just copy data (not actually compressing)
    int copySize = srcSize < maxDstSize ? srcSize : maxDstSize;
    for (int i = 0; i < copySize; i++) dst[i] = src[i];
    return copySize;
}
int LZ4_decompress_safe(const char *src, char *dst, int compressedSize, int maxDecompressedSize) {
    // Stub: just copy data (not actually decompressing)
    int copySize = compressedSize < maxDecompressedSize ? compressedSize : maxDecompressedSize;
    for (int i = 0; i < copySize; i++) dst[i] = src[i];
    return copySize;
}