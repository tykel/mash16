CC = gcc
CFLAGS = -g
LDFLAGS = -lm
OBJECTS =

.PHONY: all clean

all: mash16

mash16: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

clean:
	@rm $(OBJECTS) 2> /dev/null || true
