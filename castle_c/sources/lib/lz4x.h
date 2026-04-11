#pragma once

#include <stddef.h>

// LZ4 wrapper header
// Links against system LZ4 library (-llz4)

// Note: LZ4 uses camelCase function names
// We provide lowercase aliases for compatibility
#define LZ4_compress_bound LZ4_compressBound

int LZ4_decompress_safe(const char *source, char *dest, int compressedSize, int maxDecompressedSize);
int LZ4_compressBound(int inputSize);
int LZ4_compress_default(const char *source, char *dest, int inputSize, int outputSize);
