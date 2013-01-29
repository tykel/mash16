CC = gcc
SDL_CFLAGS = $(shell sdl-config --cflags)
CFLAGS = -O0 -std=gnu99 -Wall -g $(SDL_CFLAGS)
SDL_LDFLAGS = $(shell sdl-config --libs)
LDFLAGS = -lm $(SDL_LDFLAGS)
OBJ = obj
OBJECTS = $(OBJ)/main.o $(OBJ)/header.o $(OBJ)/crc.o $(OBJ)/cpu.o $(OBJ)/gpu.o

#DIRECTORIES
SRC = src
HDR = $(SRC)/header
CORE = $(SRC)/core

.PHONY: all clean

all: mash16

mash16: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LDFLAGS) -o $@

$(OBJ)/main.o: $(SRC)/main.c $(SRC)/consts.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/header.o: $(HDR)/header.c $(HDR)/header.h $(HDR)/crc.h $(SRC)/consts.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/crc.o: $(HDR)/crc.c $(HDR)/crc.h $(SRC)/consts.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/cpu.o: $(CORE)/cpu.c $(CORE)/cpu.h $(SRC)/consts.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/gpu.o: $(CORE)/gpu.c $(CORE)/gpu.h $(CORE)/cpu.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm $(OBJECTS) 2> /dev/null || true
