CC = cc
CFLAGS = -std=c99 -D_POSIX_C_SOURCE=200809L
WARN = -Wall -Wextra -Wpedantic -Werror=implicit-function-declaration

PREFIX = /usr/local

SRC = main.c

all: ascii-breakout

ascii-breakout: $(SRC) rogueutil.h
	$(CC) $(CFLAGS) $(WARN) -o $@ $(SRC)

clean:
	rm -f ascii-breakout

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp ascii-breakout $(DESTDIR)$(PREFIX)/bin

uninstall:
	rm $(DESTDIR)$(PREFIX)/bin/ascii-breakout

.PHONY: all clean install uninstall
