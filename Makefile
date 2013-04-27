# mash16 - the chip16 emulator
# Copyright (C) 2012-2013 tykel
# This program is licensed under the GPLv3; see LICENSE for details.

# Common definitions

CC = gcc
VERSION = \"$(shell git describe --abbrev=0)\"
TAG = \"$(shell git rev-parse HEAD | cut -c-7)\"
SDL_CFLAGS = $(shell pkg-config --cflags sdl)
CFLAGS = -O2 -std=c99 -Wall -Werror -s -DVERSION=$(VERSION) -DBUILD=$(TAG) $(SDL_CFLAGS)
SDL_LDFLAGS = $(shell pkg-config --libs sdl)
LDFLAGS = $(SDL_LDFLAGS)

# Directories

SRC = src
OBJ = build
ARCHIVE = archive
DOC = doc

SOURCES = $(shell find $(SRC) -type f -name '*.c')
OBJECTS = $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SOURCES))
TAR_SOURCES = $(SRC) $(DOC) vs2010 windows.sh INSTALL LICENSE Makefile README.md 

# Targets

.PHONY: all clean archive install uninstall

all: mash16 

mash16: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) -c $(CFLAGS) $< -o $@

archive: mash16
	tar -czf $(ARCHIVE)/mash16-$(shell echo $(VERSION) | cut -c3- | rev | cut -c2- | rev)-src.tar.gz \
		$(TAR_SOURCES)
	tar -czf $(ARCHIVE)/mash16-$(shell echo $(VERSION) | cut -c3- | rev | cut -c2- | rev).tar.gz \
		mash16 README.md

install: mash16
	@if test $(USER) != "root"; then \
		echo "error: please invoke 'make install' as root"; \
	else \
		echo "creating /usr/local/bin/mash16... "; \
		cp mash16 /usr/local/bin/mash16; \
		echo "creating /usr/share/man/man1/mash16.1.gz... "; \
		gzip -c $(DOC)/mash16.1 > /usr/share/man/man1/mash16.1.gz; \
	fi

uninstall:
	@if test $(USER) != "root"; then \
		echo "error: please invoke 'make uninstall' as root"; \
	else \
		echo "removing /usr/local/bin/mash16... "; \
		rm -f /usr/local/bin/mash16; \
		echo "removing /usr/share/man/man1/mash16.1.gz... "; \
		rm -f /usr/share/man/man1/mash16.1.gz; \
	fi

clean:
	rm -f $(OBJECTS)
	rm -f mash16
