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
    cpu.rec.bblk_map = (cpu_rec_bblk*)calloc(MEM_SIZE, sizeof(cpu_rec_bblk));
    if (!cpu.rec.bblk_map) { free(cpu.m); return 2; }
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
    if (!cpu.rec.bblk_map[0x100].invalid) {
        std::cerr << "writeMemory did not invalidate JIT block\n";
        free(cpu.rec.bblk_map);
        free(cpu.m);
        return 5;
    }

    auto snap = insp.snapshot();
    cpu.rec.bblk_map[0x4321].invalid = false;
    if (!insp.restore(snap) || !cpu.rec.bblk_map[0x4321].invalid) {
        std::cerr << "restore did not invalidate JIT blocks\n";
        free(cpu.rec.bblk_map);
        free(cpu.m);
        return 6;
    }

    insp.setBreakpoint(0x200);
    auto bps = insp.listBreakpoints();
    std::cout << "breakpoints:"; for (auto bp : bps) std::cout << " " << std::hex << bp; std::cout << std::dec << "\n";

    Inspector::Event ev;
    insp.pushEvent({"dropped", "{\"type\":\"dropped\"}"});
    if (insp.popEventBlocking(ev, 1)) {
        std::cerr << "event queued without subscriber\n";
        free(cpu.rec.bblk_map);
        free(cpu.m);
        return 7;
    }

    insp.eventSubscriberAttached();
    for (size_t i = 0; i < Inspector::MAX_EVENT_QUEUE + 10; ++i) {
        insp.pushEvent({"test", "{\"type\":\"test\"}"});
    }
    size_t count = 0;
    while (insp.popEventBlocking(ev, 1)) ++count;
    insp.eventSubscriberDetached();
    if (count != Inspector::MAX_EVENT_QUEUE) {
        std::cerr << "unexpected queued event count: " << count << "\n";
        free(cpu.rec.bblk_map);
        free(cpu.m);
        return 8;
    }

    free(cpu.rec.bblk_map);
    free(cpu.m);
    return 0;
}
