# Makefile for PS5 payload

CC = clang
CFLAGS = -target x86_64-pc-freebsd12 -fPIC -shared
LDFLAGS = -lkernel -lSceLibcInternal

ps5_pkg_server.elf: ps5_pkg_server.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f ps5_pkg_server.elf

install:
	mkdir -p /data/pkg_tool/www
	cp ps5_pkg_server.elf /data/pkg_tool/
	cp -r ../www/* /data/pkg_tool/www/
