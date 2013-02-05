CC = gcc
SDL_CFLAGS = 
CFLAGS = -O2 -std=gnu99 -Wall -Werror -g $(SDL_CFLAGS)
SDL_LDFLAGS = -lSDLmain -lSDL 
LDFLAGS = -lm $(SDL_LDFLAGS)
OBJ = build
OBJECTS = $(OBJ)/main.o $(OBJ)/header.o $(OBJ)/crc.o $(OBJ)/cpu.o $(OBJ)/cpu_ops.o $(OBJ)/gpu.o $(OBJ)/audio.o

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

$(OBJ)/cpu_ops.o: $(CORE)/cpu_ops.c $(CORE)/cpu.h $(SRC)/consts.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/gpu.o: $(CORE)/gpu.c $(CORE)/gpu.h $(CORE)/cpu.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/audio.o: $(CORE)/audio.c $(CORE)/audio.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm $(OBJECTS) 2> /dev/null || true
