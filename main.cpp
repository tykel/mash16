#include "CPU\DynarecCPU.h"
#include "CPU\InterpCPU.h"

int main(int argc, char** argv) {
	bool useDynarec = false;
	Chip16::CPU* chip16;
	if(useDynarec)
		chip16 = new Chip16::DynarecCPU();
	else
		chip16 = new Chip16::InterpCPU();
	chip16->Init();
	chip16->LoadROM(argv[1]);
	// TODO: Timing
	while(chip16->IsReady()) {
		chip16->Execute();
	}
	chip16->Clear();
	return 0;
}