CC = g++
CFLAGS = -g
LDFLAGS = -lm
OBJECTS = Core/System.o Core/CPU/CPU.o Core/CPU/InterpCPU.o Core/CPU/DynarecCPU.o \
		  Core/GPU/GPU.o Core/GPU/SfmlGPU.o Core/Timer/Timer.o Core/Timer/SfmlTimer.o \
		  Gui/Gui.o Gui/SfmlGui.o main.o 

.PHONY: all clean

all: mash16

Core/System.o: Core/System.cpp
	$(CC) -c $(CFLAGS) Core/System.cpp -o Core/System.o

Core/CPU/CPU.o: Core/CPU/CPU.cpp
	$(CC) -c $(CFLAGS) Core/CPU/CPU.cpp -o Core/CPU/CPU.o

Core/CPU/InterpCPU.o: Core/CPU/InterpCPU.cpp Core/CPU/InterpCPUOps.cpp
	$(CC) -c $(CFLAGS) Core/CPU/InterpCPU.cpp -o Core/CPU/InterpCPU.o

Core/CPU/DynarecCPU.o: Core/CPU/DynarecCPU.cpp
	$(CC) -c $(CFLAGS) Core/CPU/DynarecCPU.cpp -o Core/CPU/DynarecCPU.o

Core/GPU/GPU.o: Core/GPU/GPU.cpp
	$(CC) -c $(CFLAGS) Core/GPU/GPU.cpp -o Core/GPU/GPU.o

Core/GPU/SfmlGPU.o: Core/GPU/SfmlGPU.cpp
	$(CC) -c $(CFLAGS) Core/GPU/SfmlGPU.cpp -o Core/GPU/SfmlGPU.o

Core/Timer/Timer.o: Core/Timer/Timer.cpp
	$(CC) -c $(CFLAGS) Core/Timer/Timer.cpp -o Core/Timer/Timer.o

Core/Timer/SfmlTimer.o: Core/Timer/SfmlTimer.cpp
	$(CC) -c $(CFLAGS) Core/Timer/SfmlTimer.cpp -o Core/Timer/SfmlTimer.o

Gui/Gui.o: Gui/Gui.cpp
	$(CC) -c $(CFLAGS) Gui/Gui.cpp -o Gui/Gui.o

Gui/SfmlGui.o: Gui/SfmlGui.cpp
	$(CC) -c $(CFLAGS) Gui/SfmlGui.cpp -o Gui/SfmlGui.o

main.o: main.cpp 
	$(CC) -c $(CFLAGS) main.cpp -o main.o

mash16: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@ -L /usr/lib -lsfml-graphics -lsfml-window -lsfml-system

clean:
	@rm $(OBJECTS) 2> /dev/null || true
