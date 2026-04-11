CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c99 -I../sources/lib -I../../base/sources/libs
LDFLAGS =

SOURCES = test_cdb.c \
           ../sources/lib/cdb.c \
           ../sources/lib/cdb_reader.c \
           ../sources/lib/cdb_writer.c \
           ../sources/lib/jsmn.c

TARGET = test_cdb

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $(SOURCES) $(LDFLAGS)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
