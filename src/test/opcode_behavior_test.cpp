#include <array>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

#include "consts.h"
#include "options.h"
#include "src/core/audio.h"
#include "src/core/cpu.h"

int use_verbose = 0;

void panic(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    exit(1);
}

char *get_symbol(uint16_t) { return NULL; }
void pause_cpu(void) {}
void print_state(cpu_state *, uint16_t) {}

namespace {

constexpr uint16_t kPc = 0x0200;
constexpr uint16_t kMemA = 0x3000;
constexpr uint16_t kMemB = 0x3010;

struct Snap {
    std::array<int16_t, 16> r{};
    uint16_t pc = 0;
    uint16_t sp = 0;
    flags f{};
    instr i{};
    uint8_t bgc = 0;
    uint8_t sw = 0;
    uint8_t sh = 0;
    uint8_t fx = 0;
    uint8_t fy = 0;
    std::array<uint32_t, 16> pal{};
    std::array<uint8_t, 16> pal_r{};
    std::array<uint8_t, 16> pal_g{};
    std::array<uint8_t, 16> pal_b{};
    uint8_t pal_r0 = 0;
    uint8_t pal_g0 = 0;
    uint8_t pal_b0 = 0;
    std::vector<uint8_t> m;
    std::vector<uint8_t> vm;
    uint16_t tone = 0;
    uint8_t atk = 0;
    uint8_t dec = 0;
    uint8_t sus = 0;
    uint8_t rls = 0;
    uint8_t vol = 0;
    uint8_t type = 0;
    uint32_t prev_rand = 0;
    long cycles = 0;
    long target_cycles = 0;
    int wait_vblnk = 0;
    uint16_t old_pc = 0;
};

struct Case {
    std::string name;
    instr code{};
    std::function<void(cpu_state *)> setup;
    std::function<void(Snap &)> expect;
};

instr make_instr(uint8_t op, uint8_t yx = 0, uint16_t hhll = 0)
{
    instr i{};
    i.sdw.op = op;
    i.sdw.yx = yx;
    i.sdw.uw.hhll = hhll;
    return i;
}

uint8_t yx(uint8_t x, uint8_t y) { return static_cast<uint8_t>((y << 4) | x); }

uint16_t u16(int16_t v) { return static_cast<uint16_t>(v); }

void set_flags(flags &f, int c, int z, int o, int n)
{
    f.c = static_cast<uint8_t>(c);
    f.z = static_cast<uint8_t>(z);
    f.o = static_cast<uint8_t>(o);
    f.n = static_cast<uint8_t>(n);
}

Snap snapshot(cpu_state *s)
{
    Snap out;
    for (int i = 0; i < 16; ++i) {
        out.r[i] = s->r[i];
        out.pal[i] = s->pal[i];
        out.pal_r[i] = s->pal_r[i];
        out.pal_g[i] = s->pal_g[i];
        out.pal_b[i] = s->pal_b[i];
    }
    out.pc = s->pc;
    out.sp = s->sp;
    out.f = s->f;
    out.i = s->i;
    out.bgc = s->bgc;
    out.sw = s->sw;
    out.sh = s->sh;
    out.fx = s->fx;
    out.fy = s->fy;
    out.pal_r0 = s->pal_r0;
    out.pal_g0 = s->pal_g0;
    out.pal_b0 = s->pal_b0;
    out.m.assign(s->m, s->m + MEM_SIZE);
    out.vm.assign(s->vm, s->vm + 320 * 240);
    out.tone = s->tone;
    out.atk = s->atk;
    out.dec = s->dec;
    out.sus = s->sus;
    out.rls = s->rls;
    out.vol = s->vol;
    out.type = s->type;
    out.prev_rand = s->prev_rand;
    out.cycles = s->meta.cycles;
    out.target_cycles = s->meta.target_cycles;
    out.wait_vblnk = s->meta.wait_vblnk;
    out.old_pc = s->meta.old_pc;
    return out;
}

void store_instr(uint8_t *m, instr i)
{
    m[kPc] = i.sdw.op;
    m[kPc + 1] = i.sdw.yx;
    m[kPc + 2] = i.sdw.uw.hhll & 0xff;
    m[kPc + 3] = i.sdw.uw.hhll >> 8;
}

void common_setup(cpu_state *s, instr code)
{
    s->pc = kPc;
    for (int i = 0; i < 16; ++i)
        s->r[i] = static_cast<int16_t>(0x100 + i * 3);
    s->r[0] = 0x1234;
    s->r[1] = -2;
    s->r[2] = 3;
    s->r[3] = 0;
    s->r[4] = static_cast<int16_t>(0x8000);
    s->r[5] = 0x7fff;
    s->r[6] = 0x0100;
    s->r[7] = 4;
    s->r[8] = kMemA;
    s->r[9] = kMemB;
    set_flags(s->f, 1, 0, 1, 0);
    s->m[kMemA] = 0x78;
    s->m[kMemA + 1] = 0x56;
    s->m[kMemB] = 0xef;
    s->m[kMemB + 1] = 0xbe;
    store_instr(s->m, code);
}

void fetched(Snap &e, instr code, instr_type type)
{
    (void)type;
    e.i = code;
    e.old_pc = kPc;
    e.pc = kPc + 4;
    e.cycles += 1;
    e.target_cycles += 1;
}

uint32_t xorshift(uint32_t prev)
{
    prev ^= prev << 13;
    prev ^= prev >> 17;
    prev ^= prev << 5;
    return prev;
}

int mod16(int16_t x, int16_t y) { return ((x % y) + y) % y; }

bool same_flags(flags a, flags b)
{
    return a.c == b.c && a.z == b.z && a.o == b.o && a.n == b.n;
}

bool compare_snap(const Snap &actual, const Snap &expected, const Case &tc,
                  const char *engine)
{
    auto fail = [&](const char *field) {
        fprintf(stderr, "opcode_behavior_test: %s failed: op=0x%02x %s %s "
                        "case=%s engine=%s field=%s\n",
                tc.name.c_str(), tc.code.sdw.op, op_table[tc.code.sdw.op].name,
                cpu_rec_has_native_op(tc.code.sdw.op) ? "native" : "fallback",
                tc.name.c_str(), engine, field);
        return false;
    };
    for (int i = 0; i < 16; ++i) {
        if (actual.r[i] != expected.r[i])
            return fail(("r" + std::to_string(i)).c_str());
    }
    if (actual.pc != expected.pc)
        return fail("pc");
    if (actual.sp != expected.sp)
        return fail("sp");
    if (!same_flags(actual.f, expected.f))
    {
        fprintf(stderr, "  actual CZON=%u%u%u%u expected CZON=%u%u%u%u\n",
                actual.f.c, actual.f.z, actual.f.o, actual.f.n,
                expected.f.c, expected.f.z, expected.f.o, expected.f.n);
        return fail("flags");
    }
    if (actual.i.dword != expected.i.dword)
        return fail("instr");
    if (actual.bgc != expected.bgc || actual.sw != expected.sw ||
        actual.sh != expected.sh || actual.fx != expected.fx ||
        actual.fy != expected.fy)
        return fail("video_regs");
    if (actual.pal != expected.pal || actual.pal_r != expected.pal_r ||
        actual.pal_g != expected.pal_g || actual.pal_b != expected.pal_b)
        return fail("palette");
    if (actual.pal_r0 != expected.pal_r0 || actual.pal_g0 != expected.pal_g0 ||
        actual.pal_b0 != expected.pal_b0)
        return fail("palette0");
    if (actual.m != expected.m)
        return fail("memory");
    if (actual.vm != expected.vm)
        return fail("video_memory");
    if (actual.atk != expected.atk || actual.dec != expected.dec ||
        actual.sus != expected.sus || actual.rls != expected.rls ||
        actual.vol != expected.vol || actual.type != expected.type ||
        actual.tone != expected.tone)
        return fail("audio_regs");
    if (actual.prev_rand != expected.prev_rand)
        return fail("rng");
    if (actual.cycles != expected.cycles ||
        actual.target_cycles != expected.target_cycles)
        return fail("cycles");
    if (actual.wait_vblnk != expected.wait_vblnk)
        return fail("wait_vblnk");
    if (actual.old_pc != expected.old_pc)
        return fail("old_pc");
    return true;
}

cpu_state *make_state(uint8_t **mem_out)
{
    program_opts opts{};
    opts.cpu_rec_1bblk_per_op = 1;
    opts.rng_seed = 0x12345678;
    opts.use_audio = 0;
    auto *mem = static_cast<uint8_t *>(calloc(MEM_SIZE, 1));
    cpu_state *s = NULL;
    cpu_init(&s, mem, &opts);
    audio_init(s, &opts);
    *mem_out = mem;
    return s;
}

bool run_case(const Case &tc)
{
    uint8_t *mem = NULL;
    cpu_state *interp = make_state(&mem);
    common_setup(interp, tc.code);
    if (tc.setup)
        tc.setup(interp);
    Snap expected = snapshot(interp);
    tc.expect(expected);
    cpu_step(interp);
    Snap interp_snap = snapshot(interp);
    bool ok = compare_snap(interp_snap, expected, tc, "interpreter");
    cpu_free(interp);
    free(mem);

    mem = NULL;
    cpu_state *rec = make_state(&mem);
    common_setup(rec, tc.code);
    if (tc.setup)
        tc.setup(rec);
    cpu_rec_1bblk(rec);
    Snap rec_snap = snapshot(rec);
    ok = compare_snap(rec_snap, expected, tc, "recompiler") && ok;
    ok = compare_snap(rec_snap, interp_snap, tc, "rec-vs-interp") && ok;
    cpu_free(rec);
    free(mem);
    return ok;
}

Case simple(uint8_t op, const char *name, instr code, instr_type type)
{
    return {name, code, nullptr, [=](Snap &e) { fetched(e, code, type); }};
}

void add_case(std::vector<Case> &cases, Case c) { cases.push_back(std::move(c)); }

std::vector<Case> build_cases()
{
    std::vector<Case> c;
    add_case(c, simple(0x00, "nop", make_instr(0x00), OP_NONE));
    add_case(c, {"cls", make_instr(0x01), nullptr, [](Snap &e) {
                 fetched(e, make_instr(0x01), OP_NONE);
                 std::fill(e.vm.begin(), e.vm.end(), 0);
                 e.bgc = 0;
                 e.pal[0] = (e.pal_b0 << 16) | (e.pal_g0 << 8) | e.pal_r0;
                 e.pal_r[0] = e.pal_r0;
                 e.pal_g[0] = e.pal_g0;
                 e.pal_b[0] = e.pal_b0;
             }});
    add_case(c, {"vblnk", make_instr(0x02), nullptr, [](Snap &e) {
                 fetched(e, make_instr(0x02), OP_NONE);
                 e.wait_vblnk = 1;
             }});
    add_case(c, {"bgc", make_instr(0x03, 0, 5), nullptr, [](Snap &e) {
                 auto code = make_instr(0x03, 0, 5);
                 fetched(e, code, OP_N);
                 e.bgc = 5;
                 e.pal[0] = e.pal[5];
                 e.pal_r[0] = e.pal_r[5];
                 e.pal_g[0] = e.pal_g[5];
                 e.pal_b[0] = e.pal_b[5];
             }});
    add_case(c, {"spr", make_instr(0x04, 0, 0x0304), nullptr, [](Snap &e) {
                 fetched(e, make_instr(0x04, 0, 0x0304), OP_HHLL);
                 e.sw = 4;
                 e.sh = 3;
             }});
    add_case(c, {"drw_imm", make_instr(0x05, yx(3, 3), kMemA), [](cpu_state *s) {
                 s->sw = 1;
                 s->sh = 1;
                 s->m[kMemA] = 0x12;
             }, [](Snap &e) {
                 fetched(e, make_instr(0x05, yx(3, 3), kMemA), OP_R_R_HHLL);
                 e.vm[0] = 1;
                 e.vm[1] = 2;
                 e.f.c = 0;
             }});
    add_case(c, {"drw_r", make_instr(0x06, yx(3, 3), 8), [](cpu_state *s) {
                 s->sw = 1;
                 s->sh = 1;
                 s->m[kMemA] = 0x34;
             }, [](Snap &e) {
                 fetched(e, make_instr(0x06, yx(3, 3), 8), OP_R_R_R);
                 e.vm[0] = 3;
                 e.vm[1] = 4;
                 e.f.c = 0;
             }});
    add_case(c, {"rnd", make_instr(0x07, 2, 9), nullptr, [](Snap &e) {
                 fetched(e, make_instr(0x07, 2, 9), OP_HHLL);
                 e.prev_rand = xorshift(e.prev_rand);
                 e.r[2] = e.prev_rand % 10;
             }});
    add_case(c, {"flip", make_instr(0x08, 0, 0x0300), nullptr, [](Snap &e) {
                 fetched(e, make_instr(0x08, 0, 0x0300), OP_N_N);
                 e.fx = 1;
                 e.fy = 1;
             }});
    for (uint8_t op : {uint8_t(0x09), uint8_t(0x0a), uint8_t(0x0b), uint8_t(0x0c), uint8_t(0x0d)})
        add_case(c, simple(op, op_table[op].name, make_instr(op, 2, 15), op == 0x09 ? OP_NONE : op == 0x0d ? OP_R_HHLL : OP_HHLL));
    add_case(c, {"sng", make_instr(0x0e, 0x21, 0x3456), nullptr, [](Snap &e) {
                 fetched(e, make_instr(0x0e, 0x21, 0x3456), OP_HHLL_HHLL);
                 e.atk = 2; e.dec = 1; e.vol = 3; e.type = 4; e.sus = 5; e.rls = 6;
             }});

    auto branch_taken = [](Snap &e, instr code, instr_type type) {
        fetched(e, code, type);
        e.pc = code.sdw.uw.hhll;
    };
    add_case(c, {"jmp", make_instr(0x10, 0, 0x0400), nullptr, [=](Snap &e) { branch_taken(e, make_instr(0x10, 0, 0x0400), OP_HHLL); }});
    add_case(c, {"jmc_taken", make_instr(0x11, 0, 0x0404), nullptr, [=](Snap &e) { branch_taken(e, make_instr(0x11, 0, 0x0404), OP_HHLL); }});
    add_case(c, {"jx_not_taken", make_instr(0x12, C_Z, 0x0408), nullptr, [](Snap &e) { fetched(e, make_instr(0x12, C_Z, 0x0408), OP_HHLL); }});
    add_case(c, {"jme_taken", make_instr(0x13, yx(2, 2), 0x040c), nullptr, [=](Snap &e) { branch_taken(e, make_instr(0x13, yx(2, 2), 0x040c), OP_R_R); }});
    add_case(c, {"call_imm", make_instr(0x14, 0, 0x0410), nullptr, [](Snap &e) {
                 fetched(e, make_instr(0x14, 0, 0x0410), OP_HHLL);
                 e.m[e.sp] = 0x04; e.m[e.sp + 1] = 0x02; e.sp += 2; e.pc = 0x0410;
             }});
    add_case(c, {"ret", make_instr(0x15), [](cpu_state *s) {
                 s->m[s->sp] = 0x34; s->m[s->sp + 1] = 0x12; s->sp += 2;
             }, [](Snap &e) { fetched(e, make_instr(0x15), OP_NONE); e.sp -= 2; e.pc = 0x1234; }});
    add_case(c, {"jmp_r", make_instr(0x16, 8), nullptr, [](Snap &e) { fetched(e, make_instr(0x16, 8), OP_R); e.pc = kMemA; }});
    add_case(c, {"cx_taken", make_instr(0x17, C_B, 0x0414), nullptr, [](Snap &e) {
                 fetched(e, make_instr(0x17, C_B, 0x0414), OP_HHLL);
                 e.m[e.sp] = 0x04; e.m[e.sp + 1] = 0x02; e.sp += 2; e.pc = 0x0414;
             }});
    add_case(c, {"call_r", make_instr(0x18, 8), nullptr, [](Snap &e) {
                 fetched(e, make_instr(0x18, 8), OP_R);
                 e.m[e.sp] = 0x04; e.m[e.sp + 1] = 0x02; e.sp += 2; e.pc = kMemA;
             }});

    add_case(c, {"ldi", make_instr(0x20, 0, 0x8001), nullptr, [](Snap &e) { fetched(e, make_instr(0x20, 0, 0x8001), OP_R_HHLL); e.r[0] = static_cast<int16_t>(0x8001); }});
    add_case(c, {"ldi_sp", make_instr(0x21, 0, 0xf000), nullptr, [](Snap &e) { fetched(e, make_instr(0x21, 0, 0xf000), OP_SP_HHLL); e.sp = 0xf000; }});
    add_case(c, {"ldm_imm", make_instr(0x22, 0, kMemA), nullptr, [](Snap &e) { fetched(e, make_instr(0x22, 0, kMemA), OP_R_HHLL); e.r[0] = 0x5678; }});
    add_case(c, {"ldm_r", make_instr(0x23, yx(0, 9)), nullptr, [](Snap &e) { fetched(e, make_instr(0x23, yx(0, 9)), OP_R_R); e.r[0] = static_cast<int16_t>(0xbeef); }});
    add_case(c, {"mov", make_instr(0x24, yx(0, 2)), nullptr, [](Snap &e) { fetched(e, make_instr(0x24, yx(0, 2)), OP_R_R); e.r[0] = 3; }});
    add_case(c, {"stm_imm", make_instr(0x30, 0, kMemB), nullptr, [](Snap &e) { fetched(e, make_instr(0x30, 0, kMemB), OP_R_HHLL); e.m[kMemB] = 0x34; e.m[kMemB + 1] = 0x12; }});
    add_case(c, {"stm_r", make_instr(0x31, yx(0, 9)), nullptr, [](Snap &e) { fetched(e, make_instr(0x31, yx(0, 9)), OP_R_R); e.m[kMemB] = 0x34; e.m[kMemB + 1] = 0x12; }});

    auto ar = [&](uint8_t op, const char *name, instr code, int16_t result, flags fl, instr_type type) {
        add_case(c, {name, code, nullptr, [=](Snap &e) { fetched(e, code, type); e.r[0] = result; e.f = fl; }});
    };
    flags f{};
    set_flags(f, 0, 0, 0, 0); ar(0x40, "addi", make_instr(0x40, 0, 1), 0x1235, f, OP_R_HHLL);
    set_flags(f, 0, 0, 0, 0); ar(0x41, "add_r2", make_instr(0x41, yx(0, 2)), 0x1237, f, OP_R_R);
    add_case(c, {"add_r3", make_instr(0x42, yx(4, 5), 0), nullptr, [](Snap &e) { fetched(e, make_instr(0x42, yx(4, 5), 0), OP_R_R_R); e.r[0] = -1; set_flags(e.f, 0, 0, 0, 1); }});
    set_flags(f, 0, 0, 0, 0); ar(0x50, "subi", make_instr(0x50, 0, 0x1234), 0, f, OP_R_HHLL); c.back().expect = [](Snap &e) { fetched(e, make_instr(0x50, 0, 0x1234), OP_R_HHLL); e.r[0] = 0; set_flags(e.f, 0, 1, 0, 0); };
    add_case(c, {"sub_r2", make_instr(0x51, yx(0, 2)), nullptr, [](Snap &e) { fetched(e, make_instr(0x51, yx(0, 2)), OP_R_R); e.r[0] = 0x1231; set_flags(e.f, 0, 0, 0, 0); }});
    add_case(c, {"sub_r3", make_instr(0x52, yx(2, 0), 1), nullptr, [](Snap &e) { fetched(e, make_instr(0x52, yx(2, 0), 1), OP_R_R_R); e.r[1] = static_cast<int16_t>(3 - 0x1234); set_flags(e.f, 1, 0, 0, 1); }});
    add_case(c, {"cmpi", make_instr(0x53, 0, 0x1234), nullptr, [](Snap &e) { fetched(e, make_instr(0x53, 0, 0x1234), OP_R_HHLL); set_flags(e.f, 0, 1, 0, 0); }});
    add_case(c, {"cmp", make_instr(0x54, yx(2, 2)), nullptr, [](Snap &e) { fetched(e, make_instr(0x54, yx(2, 2)), OP_R_R); set_flags(e.f, 0, 1, 0, 0); }});

    add_case(c, {"andi", make_instr(0x60, 0, 0), nullptr, [](Snap &e) { fetched(e, make_instr(0x60, 0, 0), OP_R_HHLL); e.r[0] = 0; set_flags(e.f, 1, 1, 1, 0); }});
    add_case(c, {"and", make_instr(0x61, yx(0, 2)), nullptr, [](Snap &e) { fetched(e, make_instr(0x61, yx(0, 2)), OP_R_R); e.r[0] = 0; set_flags(e.f, 1, 1, 1, 0); }});
    add_case(c, {"and_r3", make_instr(0x62, yx(4, 5), 0), nullptr, [](Snap &e) { fetched(e, make_instr(0x62, yx(4, 5), 0), OP_R_R_R); e.r[0] = 0; set_flags(e.f, 1, 1, 1, 0); }});
    add_case(c, {"tsti", make_instr(0x63, 0, 0xffff), nullptr, [](Snap &e) { fetched(e, make_instr(0x63, 0, 0xffff), OP_R_HHLL); set_flags(e.f, 1, 0, 1, 0); }});
    add_case(c, {"tst", make_instr(0x64, yx(4, 4)), nullptr, [](Snap &e) { fetched(e, make_instr(0x64, yx(4, 4)), OP_R_R); set_flags(e.f, 1, 0, 1, 1); }});
    add_case(c, {"ori", make_instr(0x70, 0, 0x8000), nullptr, [](Snap &e) { fetched(e, make_instr(0x70, 0, 0x8000), OP_R_HHLL); e.r[0] = static_cast<int16_t>(0x9234); set_flags(e.f, 1, 0, 1, 1); }});
    add_case(c, {"or", make_instr(0x71, yx(0, 2)), nullptr, [](Snap &e) { fetched(e, make_instr(0x71, yx(0, 2)), OP_R_R); e.r[0] = 0x1237; set_flags(e.f, 1, 0, 1, 0); }});
    add_case(c, {"or_r3", make_instr(0x72, yx(0, 2), 1), nullptr, [](Snap &e) { fetched(e, make_instr(0x72, yx(0, 2), 1), OP_R_R_R); e.r[1] = 0x1237; set_flags(e.f, 1, 0, 1, 0); }});
    add_case(c, {"xori", make_instr(0x80, 0, 0x1234), nullptr, [](Snap &e) { fetched(e, make_instr(0x80, 0, 0x1234), OP_R_HHLL); e.r[0] = 0; set_flags(e.f, 1, 1, 1, 0); }});
    add_case(c, {"xor", make_instr(0x81, yx(2, 2)), nullptr, [](Snap &e) { fetched(e, make_instr(0x81, yx(2, 2)), OP_R_R); e.r[2] = 0; set_flags(e.f, 1, 1, 1, 0); }});
    add_case(c, {"xor_r3", make_instr(0x82, yx(0, 2), 1), nullptr, [](Snap &e) { fetched(e, make_instr(0x82, yx(0, 2), 1), OP_R_R_R); e.r[1] = 0x1237; set_flags(e.f, 1, 0, 1, 0); }});

    auto clear_o = [](cpu_state *s) { s->f.o = 0; };
    add_case(c, {"muli", make_instr(0x90, 2, 0x4000), clear_o, [](Snap &e) { fetched(e, make_instr(0x90, 2, 0x4000), OP_R_HHLL); e.r[2] = static_cast<int16_t>(0xc000); set_flags(e.f, 1, 0, 0, 1); }});
    add_case(c, {"mul", make_instr(0x91, yx(2, 7)), clear_o, [](Snap &e) { fetched(e, make_instr(0x91, yx(2, 7)), OP_R_R); e.r[2] = 12; set_flags(e.f, 0, 0, 0, 0); }});
    add_case(c, {"mul_r3", make_instr(0x92, yx(2, 7), 1), clear_o, [](Snap &e) { fetched(e, make_instr(0x92, yx(2, 7), 1), OP_R_R_R); e.r[1] = 12; set_flags(e.f, 0, 0, 0, 0); }});
    add_case(c, {"divi", make_instr(0xa0, 0, 3), nullptr, [](Snap &e) { fetched(e, make_instr(0xa0, 0, 3), OP_R_HHLL); e.r[0] = 0x0611; set_flags(e.f, 1, 0, 0, 0); }});
    add_case(c, {"div", make_instr(0xa1, yx(0, 2)), nullptr, [](Snap &e) { fetched(e, make_instr(0xa1, yx(0, 2)), OP_R_R); e.r[0] = 0x0611; set_flags(e.f, 1, 0, 0, 0); }});
    add_case(c, {"div_r3", make_instr(0xa2, yx(0, 2), 1), nullptr, [](Snap &e) { fetched(e, make_instr(0xa2, yx(0, 2), 1), OP_R_R_R); e.r[1] = 0x0611; set_flags(e.f, 1, 0, 0, 0); }});
    add_case(c, {"modi", make_instr(0xa3, 1, 3), nullptr, [](Snap &e) { fetched(e, make_instr(0xa3, 1, 3), OP_R_HHLL); e.r[1] = mod16(-2, 3); set_flags(e.f, 0, 0, 0, 0); }});
    add_case(c, {"mod", make_instr(0xa4, yx(1, 2)), nullptr, [](Snap &e) { fetched(e, make_instr(0xa4, yx(1, 2)), OP_R_R); e.r[1] = mod16(-2, 3); set_flags(e.f, 0, 0, 0, 0); }});
    add_case(c, {"mod_r3", make_instr(0xa5, yx(1, 2), 0), nullptr, [](Snap &e) { fetched(e, make_instr(0xa5, yx(1, 2), 0), OP_R_R_R); e.r[0] = mod16(-2, 3); set_flags(e.f, 0, 0, 0, 0); }});
    add_case(c, {"remi", make_instr(0xa6, 1, 3), nullptr, [](Snap &e) { fetched(e, make_instr(0xa6, 1, 3), OP_R_HHLL); e.r[1] = -2; set_flags(e.f, 0, 0, 0, 1); }});
    add_case(c, {"rem", make_instr(0xa7, yx(1, 2)), nullptr, [](Snap &e) { fetched(e, make_instr(0xa7, yx(1, 2)), OP_R_R); e.r[1] = -2; set_flags(e.f, 0, 0, 0, 1); }});
    add_case(c, {"rem_r3", make_instr(0xa8, yx(1, 2), 0), nullptr, [](Snap &e) { fetched(e, make_instr(0xa8, yx(1, 2), 0), OP_R_R_R); e.r[0] = -2; set_flags(e.f, 0, 0, 0, 1); }});

    add_case(c, {"shl_n", make_instr(0xb0, 2, 1), nullptr, [](Snap &e) { fetched(e, make_instr(0xb0, 2, 1), OP_R_N); e.r[2] = 6; set_flags(e.f, 1, 0, 1, 0); }});
    add_case(c, {"shr_n", make_instr(0xb1, 4, 1), nullptr, [](Snap &e) { fetched(e, make_instr(0xb1, 4, 1), OP_R_N); e.r[4] = 0x4000; set_flags(e.f, 1, 0, 1, 0); }});
    add_case(c, {"sar_n", make_instr(0xb2, 4, 1), nullptr, [](Snap &e) { fetched(e, make_instr(0xb2, 4, 1), OP_R_N); e.r[4] = static_cast<int16_t>(0xc000); set_flags(e.f, 1, 0, 1, 1); }});
    add_case(c, {"shl_r", make_instr(0xb3, yx(2, 7)), nullptr, [](Snap &e) { fetched(e, make_instr(0xb3, yx(2, 7)), OP_R_R); e.r[2] = 48; set_flags(e.f, 1, 0, 1, 0); }});
    add_case(c, {"shr_r", make_instr(0xb4, yx(4, 7)), nullptr, [](Snap &e) { fetched(e, make_instr(0xb4, yx(4, 7)), OP_R_R); e.r[4] = 0x0800; set_flags(e.f, 1, 0, 1, 0); }});
    add_case(c, {"sar_r", make_instr(0xb5, yx(4, 7)), nullptr, [](Snap &e) { fetched(e, make_instr(0xb5, yx(4, 7)), OP_R_R); e.r[4] = static_cast<int16_t>(0xf800); set_flags(e.f, 1, 0, 1, 1); }});

    add_case(c, {"push", make_instr(0xc0, 0), nullptr, [](Snap &e) { fetched(e, make_instr(0xc0, 0), OP_R); e.m[e.sp] = 0x34; e.m[e.sp + 1] = 0x12; e.sp += 2; }});
    add_case(c, {"pop", make_instr(0xc1, 0), [](cpu_state *s) { s->m[s->sp] = 0xcd; s->m[s->sp + 1] = 0xab; s->sp += 2; }, [](Snap &e) { fetched(e, make_instr(0xc1, 0), OP_R); e.sp -= 2; e.r[0] = static_cast<int16_t>(0xabcd); }});
    add_case(c, {"pushall", make_instr(0xc2), nullptr, [](Snap &e) { fetched(e, make_instr(0xc2), OP_NONE); for (int i = 0; i < 16; ++i) { e.m[e.sp] = u16(e.r[i]) & 0xff; e.m[e.sp + 1] = u16(e.r[i]) >> 8; e.sp += 2; } }});
    add_case(c, {"popall", make_instr(0xc3), [](cpu_state *s) { for (int i = 0; i < 16; ++i) { s->m[s->sp] = i; s->m[s->sp + 1] = 0x10; s->sp += 2; } }, [](Snap &e) { fetched(e, make_instr(0xc3), OP_NONE); for (int i = 15; i >= 0; --i) { e.sp -= 2; e.r[i] = static_cast<int16_t>(e.m[e.sp] | (e.m[e.sp + 1] << 8)); } }});
    add_case(c, {"pushf", make_instr(0xc4), nullptr, [](Snap &e) { fetched(e, make_instr(0xc4), OP_NONE); e.m[e.sp] = 0x42; e.m[e.sp + 1] = 0; e.sp += 2; }});
    add_case(c, {"popf", make_instr(0xc5), [](cpu_state *s) { s->m[s->sp] = 0x84; s->m[s->sp + 1] = 0; s->sp += 2; }, [](Snap &e) { fetched(e, make_instr(0xc5), OP_NONE); e.sp -= 2; set_flags(e.f, 0, 1, 0, 1); }});

    add_case(c, {"pal_imm", make_instr(0xd0, 0, kMemA), [](cpu_state *s) { for (int i = 0; i < 48; ++i) s->m[kMemA + i] = i; }, [](Snap &e) { fetched(e, make_instr(0xd0, 0, kMemA), OP_HHLL); for (int i = 0; i < 16; ++i) { e.pal[i] = (e.m[kMemA + i * 3] << 16) | (e.m[kMemA + i * 3 + 1] << 8) | e.m[kMemA + i * 3 + 2]; e.pal_r[i] = e.m[kMemA + i * 3]; e.pal_g[i] = e.m[kMemA + i * 3 + 1]; e.pal_b[i] = e.m[kMemA + i * 3 + 2]; } e.pal_r0 = e.pal_r[0]; e.pal_g0 = e.pal_g[0]; e.pal_b0 = e.pal_b[0]; }});
    add_case(c, {"pal_r", make_instr(0xd1, 8), [](cpu_state *s) { for (int i = 0; i < 48; ++i) s->m[kMemA + i] = 0x80 + i; }, [](Snap &e) { fetched(e, make_instr(0xd1, 8), OP_R); for (int i = 0; i < 16; ++i) { e.pal[i] = (e.m[kMemA + i * 3] << 16) | (e.m[kMemA + i * 3 + 1] << 8) | e.m[kMemA + i * 3 + 2]; e.pal_r[i] = e.m[kMemA + i * 3]; e.pal_g[i] = e.m[kMemA + i * 3 + 1]; e.pal_b[i] = e.m[kMemA + i * 3 + 2]; } e.pal_r0 = e.pal_r[0]; e.pal_g0 = e.pal_g[0]; e.pal_b0 = e.pal_b[0]; }});
    add_case(c, {"noti", make_instr(0xe0, 0, 0), nullptr, [](Snap &e) { fetched(e, make_instr(0xe0, 0, 0), OP_R_HHLL); e.r[0] = -1; set_flags(e.f, 1, 0, 1, 1); }});
    add_case(c, {"not", make_instr(0xe1, 0), nullptr, [](Snap &e) { fetched(e, make_instr(0xe1, 0), OP_R); e.r[0] = static_cast<int16_t>(~0x1234); set_flags(e.f, 1, 0, 1, 1); }});
    add_case(c, {"not_r2", make_instr(0xe2, yx(0, 2)), nullptr, [](Snap &e) { fetched(e, make_instr(0xe2, yx(0, 2)), OP_R_R); e.r[0] = static_cast<int16_t>(~3); set_flags(e.f, 1, 0, 1, 1); }});
    add_case(c, {"negi", make_instr(0xe3, 0, 1), nullptr, [](Snap &e) { fetched(e, make_instr(0xe3, 0, 1), OP_R_HHLL); e.r[0] = -1; set_flags(e.f, 1, 0, 1, 1); }});
    add_case(c, {"neg", make_instr(0xe4, 0), nullptr, [](Snap &e) { fetched(e, make_instr(0xe4, 0), OP_R); e.r[0] = static_cast<int16_t>(-0x1234); set_flags(e.f, 1, 0, 1, 1); }});
    add_case(c, {"neg_r2", make_instr(0xe5, yx(0, 2)), nullptr, [](Snap &e) { fetched(e, make_instr(0xe5, yx(0, 2)), OP_R_R); e.r[0] = -3; set_flags(e.f, 1, 0, 1, 1); }});
    return c;
}

} // namespace

int main()
{
    auto cases = build_cases();
    std::array<int, 256> case_count{};
    for (const auto &tc : cases)
        case_count[tc.code.sdw.op]++;

    int reserved = 0;
    int implemented = 0;
    int fallback = 0;
    bool ok = true;
    for (int op = 0; op < 256; ++op) {
        if (op_table[op].impl == op_error) {
            reserved++;
            if (case_count[op] != 0) {
                fprintf(stderr, "reserved opcode 0x%02x unexpectedly has behavior cases\n", op);
                ok = false;
            }
        } else {
            implemented++;
            if (!cpu_rec_has_native_op(static_cast<uint8_t>(op)))
                fallback++;
            if (case_count[op] == 0) {
                fprintf(stderr, "implemented opcode 0x%02x %s has no behavior case\n", op, op_table[op].name);
                ok = false;
            }
        }
    }

    for (const auto &tc : cases)
        ok = run_case(tc) && ok;

    printf("opcode_behavior_test: %zu behavior cases, %d implemented opcodes, "
           "%d reserved opcodes, %d fallback opcodes\n",
           cases.size(), implemented, reserved, fallback);
    return ok ? 0 : 1;
}
