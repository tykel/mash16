#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

#include "core/cpu.h"
#include "options.h"

int use_verbose = 0;

void panic(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    exit(1);
}

char *get_symbol(uint16_t a)
{
    (void)a;
    return NULL;
}

void pause_cpu(void) {}

void print_state(cpu_state *state, uint16_t pc)
{
    (void)state;
    (void)pc;
}

namespace {

struct RecCase {
    const char *name;
    uint16_t start;
    int expected_cycles;
    uint16_t expected_end_pc;
    std::function<void(uint8_t *)> program;
    std::function<bool(cpu_state *, const RecCase &)> check;
};

static void store_instr(uint8_t *mem, uint16_t pc, uint8_t op, uint8_t yx,
                        uint16_t hhll)
{
    mem[pc] = op;
    mem[pc + 1] = yx;
    mem[pc + 2] = hhll & 0xff;
    mem[pc + 3] = hhll >> 8;
}

static bool fail_case(const RecCase &tc, const char *message)
{
    fprintf(stderr, "jit_sanity_test: %s failed: %s\n", tc.name, message);
    return false;
}

static bool expect_reg(cpu_state *s, const RecCase &tc, int reg, uint16_t value)
{
    if ((uint16_t)s->r[reg] != value) {
        fprintf(stderr,
                "jit_sanity_test: %s failed: r%d=0x%04x expected 0x%04x\n",
                tc.name, reg, (uint16_t)s->r[reg], value);
        return false;
    }
    return true;
}

static bool expect_pc(cpu_state *s, const RecCase &tc, uint16_t pc)
{
    if (s->pc != pc) {
        fprintf(stderr,
                "jit_sanity_test: %s failed: pc=0x%04x expected 0x%04x\n",
                tc.name, s->pc, pc);
        return false;
    }
    return true;
}

static bool expect_flags(cpu_state *s, const RecCase &tc, int c, int z, int o,
                         int n)
{
    if (s->f.c != c || s->f.z != z || s->f.o != o || s->f.n != n) {
        fprintf(stderr,
                "jit_sanity_test: %s failed: flags CZON=%u%u%u%u expected "
                "%d%d%d%d\n",
                tc.name, s->f.c, s->f.z, s->f.o, s->f.n, c, z, o, n);
        return false;
    }
    return true;
}

static bool compare_core_state(cpu_state *rec, cpu_state *interp,
                               const RecCase &tc)
{
    if (memcmp(rec, interp, (uint8_t *)&rec->m - (uint8_t *)rec) != 0)
        return fail_case(tc, "core state differs from interpreter");
    if (memcmp(rec->m, interp->m, MEM_SIZE) != 0)
        return fail_case(tc, "memory differs from interpreter");
    return true;
}

static bool run_case(const RecCase &tc)
{
    program_opts opts = {};
    opts.cpu_rec_1bblk_per_op = 0;
    opts.rng_seed = 1;

    uint8_t *mem = (uint8_t *)calloc(MEM_SIZE, 1);
    if (!mem) {
        fprintf(stderr, "alloc fail\n");
        return false;
    }
    tc.program(mem);

    cpu_state *s = NULL;
    cpu_init(&s, mem, &opts);
    if (!s) {
        fprintf(stderr, "cpu_init failed\n");
        free(mem);
        return false;
    }
    s->pc = tc.start;

    cpu_rec_compile(s, tc.start);

    cpu_rec_bblk *bb = &s->rec.bblk_map[tc.start];
    bool ok = true;
    if (bb->code == NULL || bb->size == 0) {
        fprintf(stderr, "jit_sanity_test: %s failed: code=%p size=%zu\n",
                tc.name, bb->code, bb->size);
        ok = false;
    }
    if (bb->cycles != tc.expected_cycles ||
        bb->end_pc != tc.expected_end_pc) {
        fprintf(stderr,
                "jit_sanity_test: %s failed: cycles=%d end_pc=0x%04x "
                "expected cycles=%d end_pc=0x%04x\n",
                tc.name, bb->cycles, bb->end_pc, tc.expected_cycles,
                tc.expected_end_pc);
        ok = false;
    }

    cpu_state *interp = &s[1];
    interp->pc = tc.start;
    memcpy(interp->m, s->m, MEM_SIZE);

    if (ok) {
        cpu_rec_1bblk(s);
        while (interp->pc != s->pc)
            cpu_step(interp);

        if (s->meta.cycles != tc.expected_cycles ||
            s->meta.target_cycles != tc.expected_cycles) {
            fprintf(stderr,
                    "jit_sanity_test: %s failed: cycles=%ld "
                    "target_cycles=%ld expected %d\n",
                    tc.name, s->meta.cycles, s->meta.target_cycles,
                    tc.expected_cycles);
            ok = false;
        }
        ok = tc.check(s, tc) && ok;
        ok = compare_core_state(s, interp, tc) && ok;
    }

    cpu_free(s);
    free(mem);
    return ok;
}

static std::vector<RecCase> build_cases()
{
    std::vector<RecCase> cases;

    cases.push_back({
        "straight_line_dataflow",
        0x0000,
        5,
        0x0014,
        [](uint8_t *m) {
            store_instr(m, 0x0000, 0x20, 0x00, 0x1234); /* LDI r0, 0x1234 */
            store_instr(m, 0x0004, 0x40, 0x00, 0x0001); /* ADDI r0, 1 */
            store_instr(m, 0x0008, 0x24, 0x01, 0x0000); /* MOV r1, r0 */
            store_instr(m, 0x000c, 0x41, 0x01, 0x0000); /* ADD r1, r0 */
            store_instr(m, 0x0010, 0x10, 0x00, 0x0020); /* JMP 0x0020 */
        },
        [](cpu_state *s, const RecCase &tc) {
            return expect_pc(s, tc, 0x0020) && expect_reg(s, tc, 0, 0x1235) &&
                   expect_reg(s, tc, 1, 0x246a) &&
                   expect_flags(s, tc, 0, 0, 0, 0);
        },
    });

    cases.push_back({
        "memory_roundtrip",
        0x0100,
        5,
        0x0114,
        [](uint8_t *m) {
            store_instr(m, 0x0100, 0x20, 0x02, 0x3456); /* LDI r2, 0x3456 */
            store_instr(m, 0x0104, 0x30, 0x02, 0x3000); /* STM r2, [0x3000] */
            store_instr(m, 0x0108, 0x22, 0x03, 0x3000); /* LDM r3, [0x3000] */
            store_instr(m, 0x010c, 0x40, 0x03, 0x0001); /* ADDI r3, 1 */
            store_instr(m, 0x0110, 0x10, 0x00, 0x0120); /* JMP 0x0120 */
        },
        [](cpu_state *s, const RecCase &tc) {
            uint16_t stored = s->m[0x3000] | (s->m[0x3001] << 8);
            if (stored != 0x3456)
                return fail_case(tc, "memory store did not persist");
            return expect_pc(s, tc, 0x0120) && expect_reg(s, tc, 2, 0x3456) &&
                   expect_reg(s, tc, 3, 0x3457) &&
                   expect_flags(s, tc, 0, 0, 0, 0);
        },
    });

    cases.push_back({
        "terminal_branch_not_taken",
        0x0200,
        3,
        0x020c,
        [](uint8_t *m) {
            store_instr(m, 0x0200, 0x20, 0x04, 0x0001); /* LDI r4, 1 */
            store_instr(m, 0x0204, 0x53, 0x04, 0x0002); /* CMPI r4, 2 */
            store_instr(m, 0x0208, 0x12, C_Z, 0x0240);  /* JZ 0x0240 */
        },
        [](cpu_state *s, const RecCase &tc) {
            return expect_pc(s, tc, 0x020c) && expect_reg(s, tc, 4, 0x0001) &&
                   expect_flags(s, tc, 1, 0, 0, 1);
        },
    });

    cases.push_back({
        "terminal_branch_taken",
        0x0300,
        3,
        0x030c,
        [](uint8_t *m) {
            store_instr(m, 0x0300, 0x20, 0x05, 0x0002); /* LDI r5, 2 */
            store_instr(m, 0x0304, 0x53, 0x05, 0x0002); /* CMPI r5, 2 */
            store_instr(m, 0x0308, 0x12, C_Z, 0x0340);  /* JZ 0x0340 */
        },
        [](cpu_state *s, const RecCase &tc) {
            return expect_pc(s, tc, 0x0340) && expect_reg(s, tc, 5, 0x0002) &&
                   expect_flags(s, tc, 0, 1, 0, 0);
        },
    });

    cases.push_back({
        "self_modifying_store_truncates_block",
        0x0400,
        2,
        0x0408,
        [](uint8_t *m) {
            store_instr(m, 0x0400, 0x20, 0x06, 0x0000); /* LDI r6, 0 */
            store_instr(m, 0x0404, 0x30, 0x06, 0x040c); /* STM r6, [0x040c] */
            store_instr(m, 0x0408, 0x40, 0x06, 0x0001); /* ADDI r6, 1 */
            store_instr(m, 0x040c, 0x10, 0x00, 0x0440); /* JMP 0x0440 */
        },
        [](cpu_state *s, const RecCase &tc) {
            uint16_t rewritten = s->m[0x040c] | (s->m[0x040d] << 8);
            if (rewritten != 0x0000)
                return fail_case(tc, "future instruction was not rewritten");
            return expect_pc(s, tc, 0x0408) && expect_reg(s, tc, 6, 0x0000);
        },
    });

    return cases;
}

} // namespace

int main(void)
{
    bool ok = true;
    auto cases = build_cases();
    for (const auto &tc : cases)
        ok = run_case(tc) && ok;

    if (!ok)
        return 1;

    printf("jit_sanity_test: %zu multi-op basic block cases ok\n",
           cases.size());
    return 0;
}
