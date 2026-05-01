# Makefile for iron.hdll — HashLink native extension (HDLL)
# Compiles iron_bridge.c as a shared library.
# Iron engine symbols are resolved at load time from the host executable
# via -undefined dynamic_lookup (macOS).

CC ?= clang
IRON_BASE = ../../base
IRON_SOURCES = $(IRON_BASE)/sources
HL_INCLUDE = /usr/local/include

SOURCES = sources/iron_bridge.c

# macOS
iron.hdll: $(SOURCES)
	$(CC) -shared -fPIC -o iron.dylib $(SOURCES) \
		-I$(IRON_SOURCES) \
		-I$(IRON_SOURCES)/libs \
		-I$(IRON_SOURCES)/kong \
		-I$(HL_INCLUDE) \
		-undefined dynamic_lookup \
		-Wno-strict-prototypes
	mv iron.dylib iron.hdll

clean:
	rm -f iron.hdll iron.dylib

.PHONY: clean
