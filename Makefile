VENDOR.OS := $(shell mk/vendor.os)
VENDOR.CC := $(shell env CC="${CC}" mk/vendor.cc)

prefix = /usr/local
libdir = $(prefix)/lib
datadir = $(prefix)/share
includedir = $(prefix)/include
luainclude = $(includedir)/lua52
luapath = $(datadir)/lua/5.2
luacpath = $(libdir)/lua/5.2

LUAC = $(prefix)/bin/luac

ifeq ($(VENDOR.CC), sunpro)
CFLAGS = -g
SOFLAGS = -xcode=pic13
else
CFLAGS = -std=c99 -g -O2 -Wall -Wextra -Werror -Wno-unused-variable -Wno-unused-parameter
SOFLAGS = -fPIC
endif

ifeq ($(VENDOR.OS), Darwin)
SOFLAGS += -bundle -undefined dynamic_lookup
else
SOFLAGS += -shared
endif

hexdump: hexdump.c hexdump.h
	$(CC) $(CFLAGS) -o $@ $< $(CPPFLAGS) -DHEXDUMP_MAIN

hexdump.so: hexdump.c hexdump.h
	$(CC) $(CFLAGS) $(SOFLAGS) -o $@ $< $(CPPFLAGS) -I$(luainclude) -DHEXDUMP_LUALIB

libhexdump.so: hexdump.c hexdump.h
	$(CC) $(CFLAGS) $(SOFLAGS) -o $@ $< $(CPPFLAGS)

.PHONY: clean clean~

clean:
	rm -f hexdump *.o *.a *.so
	rm -fr *.dSYM

clean~: clean
	rm -f *~


