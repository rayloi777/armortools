/*
 * Minimal standalone file I/O - no external dependencies
 */

#include "iron_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int iron_file_reader_open(IronFileReader *reader, const char *filepath, IronFileType type) {
    if (!reader || !filepath) return 0;

    reader->filepath = NULL;
    reader->data = NULL;
    reader->size = 0;
    reader->pos = 0;

    const char *mode = (type == IRON_FILE_TYPE_SAVE) ? "wb" : "rb";
    FILE *f = fopen(filepath, mode);
    if (!f) return 0;

    if (type == IRON_FILE_TYPE_LOAD) {
        fseek(f, 0, SEEK_END);
        reader->size = (size_t)ftell(f);
        fseek(f, 0, SEEK_SET);

        if (reader->size > 0) {
            reader->data = (char*)malloc(reader->size + 1);
            if (!reader->data) {
                fclose(f);
                return 0;
            }
            size_t read = fread(reader->data, 1, reader->size, f);
            if (read != reader->size) {
                free(reader->data);
                fclose(f);
                return 0;
            }
            reader->data[reader->size] = '\0';
        }
    } else {
        // For save, just open and remember filepath
        reader->filepath = (char*)malloc(strlen(filepath) + 1);
        if (!reader->filepath) {
            fclose(f);
            return 0;
        }
        strcpy(reader->filepath, filepath);
    }

    fclose(f);
    return 1;
}

size_t iron_file_reader_size(IronFileReader *reader) {
    if (!reader) return 0;
    return reader->size;
}

size_t iron_file_reader_read(IronFileReader *reader, char *buffer, size_t size) {
    if (!reader || !buffer) return 0;
    size_t remaining = reader->size - reader->pos;
    size_t to_read = (size < remaining) ? size : remaining;
    memcpy(buffer, reader->data + reader->pos, to_read);
    reader->pos += to_read;
    return to_read;
}

void iron_file_reader_close(IronFileReader *reader) {
    if (!reader) return;
    if (reader->data) free(reader->data);
    if (reader->filepath) free(reader->filepath);
    reader->filepath = NULL;
    reader->data = NULL;
    reader->size = 0;
    reader->pos = 0;
}
