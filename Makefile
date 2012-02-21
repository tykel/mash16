CC = g++
CFLAGS = -g
LDFLAGS = -lm
OBJECTS = obj/System.o obj/CPU.o obj/InterpCPU.o obj/DynarecCPU.o \
		  obj/GPU.o obj/SfmlGPU.o obj/Timer.o obj/SfmlTimer.o \
		  obj/Gui.o obj/SfmlGui.o obj/main.o 

.PHONY: all clean

all: mash16

obj/System.o: Core/System.cpp
	$(CC) -c $(CFLAGS) Core/System.cpp -o obj/System.o

obj/CPU.o: Core/CPU/CPU.cpp
	$(CC) -c $(CFLAGS) Core/CPU/CPU.cpp -o obj/CPU.o

obj/InterpCPU.o: Core/CPU/InterpCPU.cpp Core/CPU/InterpCPUOps.cpp
	$(CC) -c $(CFLAGS) Core/CPU/InterpCPU.cpp -o obj/InterpCPU.o

obj/DynarecCPU.o: Core/CPU/DynarecCPU.cpp
	$(CC) -c $(CFLAGS) Core/CPU/DynarecCPU.cpp -o obj/DynarecCPU.o

obj/GPU.o: Core/GPU/GPU.cpp
	$(CC) -c $(CFLAGS) Core/GPU/GPU.cpp -o obj/GPU.o

obj/SfmlGPU.o: Core/GPU/SfmlGPU.cpp
	$(CC) -c $(CFLAGS) Core/GPU/SfmlGPU.cpp -o obj/SfmlGPU.o

obj/Timer.o: Core/Timer/Timer.cpp
	$(CC) -c $(CFLAGS) Core/Timer/Timer.cpp -o obj/Timer.o

obj/SfmlTimer.o: Core/Timer/SfmlTimer.cpp
	$(CC) -c $(CFLAGS) Core/Timer/SfmlTimer.cpp -o obj/SfmlTimer.o

obj/Gui.o: Gui/Gui.cpp
	$(CC) -c $(CFLAGS) Gui/Gui.cpp -o obj/Gui.o

obj/SfmlGui.o: Gui/SfmlGui.cpp
	$(CC) -c $(CFLAGS) Gui/SfmlGui.cpp -o obj/SfmlGui.o

obj/main.o: main.cpp 
	$(CC) -c $(CFLAGS) main.cpp -o obj/main.o

mash16: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@ -L /usr/lib -lsfml-graphics -lsfml-window -lsfml-system

clean:
	@rm $(OBJECTS) 2> /dev/null || true
