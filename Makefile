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

SOURCES = $(shell find $(SRC) -type f -name '*.c')
OBJECTS = $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SOURCES))

# Targets

.PHONY: all clean archive

all: mash16 

mash16: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) -c $(CFLAGS) $< -o $@

archive: mash16


clean:
	@rm -f $(OBJECTS) 2> /dev/null || true
	@rm -f mash16 2> /dev/null || true
