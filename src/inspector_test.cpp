#include <iostream>
#include <cstring>
#include <vector>
int use_verbose = 0;
#include "core/inspector.h"

using namespace mash16;

int main() {
    cpu_state cpu;
    memset(&cpu, 0, sizeof(cpu));
    cpu.m = (uint8_t*)malloc(MEM_SIZE);
    if (!cpu.m) return 2;
    for (size_t i = 0; i < MEM_SIZE; ++i) cpu.m[i] = (uint8_t)(i & 0xFF);

    Inspector insp(&cpu);
    auto regs = insp.getRegisters();
    std::cout << "PC=" << regs.pc << " SP=" << regs.sp << "\n";

    auto block = insp.readMemory(0x100, 16);
    std::cout << "mem[0x100..]=";
    for (auto b : block) std::cout << std::hex << (int)b << " ";
    std::cout << std::dec << "\n";

    std::vector<uint8_t> data = {0xAA, 0xBB, 0xCC};
    insp.writeMemory(0x100, data);
    auto after = insp.readMemory(0x100, 3);
    std::cout << "after: "; for (auto b : after) std::cout << std::hex << (int)b << " "; std::cout << std::dec << "\n";

    insp.setBreakpoint(0x200);
    auto bps = insp.listBreakpoints();
    std::cout << "breakpoints:"; for (auto bp : bps) std::cout << " " << std::hex << bp; std::cout << std::dec << "\n";

    free(cpu.m);
    return 0;
}
