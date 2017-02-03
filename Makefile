# mash16 - the chip16 emulator
# Copyright (C) 2012-2013 tykel
# This program is licensed under the GPLv3; see LICENSE for details.

# Common definitions

CC = gcc
WIN_PREFIX = $(shell ls /usr/bin | grep mingw32 | grep '86-' | tail -1 | rev | cut -d- -f2- | rev)
WIN_CC = $(WIN_PREFIX)-gcc
VERSION = \"$(shell git describe --match "v*" | cut -d'-' -f1 | cut -c2-)\"
VERSION_NQ = $(shell echo $(VERSION) | cut -c2- | rev | cut -c2- | rev)
TAG = \"$(shell git rev-parse --short HEAD)\"
SDL_CFLAGS = $(shell pkg-config --cflags sdl)
CFLAGS = -O3 -finline-functions -g -Wall -std=c89 -pedantic -DVERSION=$(VERSION) -DBUILD=$(TAG) $(SDL_CFLAGS)
#WIN_CFLAGS = $(CFLAGS) -I/home/tim/Downloads/SDL-1.2.15/include -I/usr/include 
WIN_CFLAGS = -O0 -g -Wall -std=c89 -pedantic -DVERSION=$(VERSION) -DBUILD=$(TAG) -I/usr/local/cross-tools/$(WIN_PREFIX)/include $(shell /usr/local/cross-tools/$(WIN_PREFIX)/bin/sdl-config --cflags)
SDL_LDFLAGS = -lSDLmain $(shell pkg-config --libs sdl)
LDFLAGS = -lm $(SDL_LDFLAGS)
WIN_LDFLAGS = $(shell /usr/local/cross-tools/$(WIN_PREFIX)/bin/sdl-config --libs)

# Directories

SRC = src
OBJ = build
ARCHIVE = archive
DOC = doc

SOURCES = $(shell find $(SRC) -type f -name '*.c')
OBJECTS = $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SOURCES))
WIN_OBJECTS = $(patsubst $(SRC)/%.c, $(OBJ)/%.obj, $(SOURCES))
TAR_SOURCES = $(SRC) $(DOC) vs2010 INSTALL LICENSE Makefile README.md 

# Targets

.PHONY: all clean archive install uninstall windows win

all: mash16 
win: windows 
windows: mash16.exe

mash16: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

$(OBJ)/%.o: $(SRC)/%.c
	@mkdir -p $(@D)
	$(CC) -c $(CFLAGS) $< -o $@

mash16.exe: $(WIN_OBJECTS)
	$(WIN_CC) $(WIN_OBJECTS) $(WIN_LDFLAGS) -o $@

$(OBJ)/%.obj: $(SRC)/%.c
ifeq ($(WIN_CC),-gcc)
	@echo No MinGW installation found.
	@echo Please specify the prefix with the WIN_PREFIX variable, or install MinGW on your system.
	@false
endif
	@mkdir -p $(@D)
	$(WIN_CC) -c $(WIN_CFLAGS) $< -o $@

archive: mash16
	@mkdir -p $(ARCHIVE)
	@echo "creating mash16-$(VERSION_NQ)-src.tar.gz"
	@tar -czf $(ARCHIVE)/mash16-$(VERSION_NQ)-src.tar.gz $(TAR_SOURCES)
	@echo "creating mash16-$(VERSION_NQ).tar.gz"
	@tar -czf $(ARCHIVE)/mash16-$(VERSION_NQ).tar.gz mash16 README.md
	@if test -e mash16.exe; then \
		echo "creating mash16-$(VERSION_NQ).zip"; \
		zip $(ARCHIVE)/mash16-$(VERSION_NQ).zip mash16.exe README.md; \
	fi


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
	@echo "Removing object files..."
	@rm -f $(OBJECTS) $(WIN_OBJECTS)
	@echo "Removing executable..."
	@rm -f mash16 mash16.exe
