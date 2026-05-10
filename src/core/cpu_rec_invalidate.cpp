#include "cpu.h"

#include <algorithm>

void cpu_rec_invalidate(cpu_state *state, uint16_t addr, size_t size)
{
    if (!state || !state->rec.bblk_map || size == 0) return;
    size_t count = std::min<size_t>(size, MEM_SIZE - addr);
    for (size_t i = 0; i < count; ++i) {
        int a = static_cast<int>(addr + i);
        for (int delta = -3; delta <= 3; ++delta) {
            int bblk_addr = a + delta;
            if (bblk_addr < 0 || bblk_addr > MAX_ADDR) continue;
            state->rec.bblk_map[bblk_addr].invalid = true;
        }
    }
}
