CFLAGS := -std=c99 -Wall -Wextra -Werror -Wno-unused-variable -Wno-unused-parameter

hexdump: hexdump.c hexdump.h
	$(CC) $(CFLAGS) -o $@ $< $(CPPFLAGS)

