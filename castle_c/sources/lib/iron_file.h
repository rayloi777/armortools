#pragma once

#include <stddef.h>

typedef enum {
    IRON_FILE_TYPE_SAVE,
    IRON_FILE_TYPE_LOAD
} IronFileType;

typedef struct {
    char *filepath;
    char *data;
    size_t size;
    size_t pos;
} IronFileReader;

int iron_file_reader_open(IronFileReader *reader, const char *filepath, IronFileType type);
size_t iron_file_reader_size(IronFileReader *reader);
size_t iron_file_reader_read(IronFileReader *reader, char *buffer, size_t size);
void iron_file_reader_close(IronFileReader *reader);
