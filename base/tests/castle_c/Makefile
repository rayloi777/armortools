# CastleDB C Test - Standalone Makefile
# Builds test binary without Iron engine framework

CC = clang
CFLAGS = -std=c11 -O2 -Wall -I/Users/rayloi/Documents/GitHub/armortools/castle_c/sources/lib -I/Users/rayloi/Documents/GitHub/armortools/base/sources

SRCS = \
	castle_c/sources/lib/jsmn.c \
	castle_c/sources/lib/cdb.c \
	castle_c/sources/lib/cdb_reader.c \
	castle_c/sources/lib/cdb_writer.c \
	castle_c/sources/lib/cdb_types.c \
	castle_c/sources/lib/cdb_json.c \
	base/tests/castle_c/test_cdb.c \
	base/tests/castle_c/iron_stubs.c

OBJDIR = base/tests/castle_c/obj
BIN = base/tests/castle_c/CastleCDBTest

OBJS = $(SRCS:%.c=$(OBJDIR)/%.o)

$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN): $(OBJS)
	$(CC) -o $@ $(OBJS)

run: $(BIN)
	./$(BIN)

clean:
	rm -rf $(OBJDIR) $(BIN)

.PHONY: run clean