CC = gcc
CFLAGS = -std=gnu99 -Wall
LDFLAGS = -lm `sdl-config --libs`
OBJ = obj
OBJECTS = $(OBJ)/main.o $(OBJ)/header.o $(OBJ)/crc.o $(OBJ)/cpu.o

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

clean:
	@rm $(OBJECTS) 2> /dev/null || true
