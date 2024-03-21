#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>

#include <vector>

#include "cpu.h"
#include "cpu_rec_ops.h"

#define CPU_REC_TOTAL (CPU_REC_PAGES * page_size)

void cpu_rec_init(cpu_state *state, program_opts *opts)
{
    state->rec.jit_base =
        (void *)ROUNDUP((size_t)state + sizeof(*state), page_size);
    memset(state->rec.jit_base, 0x90, CPU_REC_TOTAL);
    memset(&state->rec.host[0], 0, sizeof(state->rec.host[0]) * 16);
    state->rec.bblk_1per_op = opts->cpu_rec_1bblk_per_op;
    state->rec.bblk_no_fallback = opts->cpu_rec_no_fallback;
    state->rec.dirty_map = reinterpret_cast<uint8_t *>(calloc(8192, 1));
}

void cpu_rec_free(cpu_state *state) { free(state->rec.dirty_map); }

void *cpu_rec_get_page(cpu_state *state, uint16_t a)
{
    return state->rec.jit_base + (a >> 2) * page_size;
}

void cpu_rec_hostreg_release(cpu_state *state, int i)
{
    if (state->rec.host[i].use == CPU_HOST_REG_VAR &&
        state->rec.host[i].ptr != NULL &&
        (state->rec.host[i].flags & CPU_VAR_WRITE)) {
        ptrdiff_t disp = (uint8_t *)state->rec.host[i].ptr - state->rec.jit_p;
        int size = state->rec.host[i].size;
        int ptr64 = 0;
        int regPtr = REG_NONE;

        if (disp > INT32_MAX || disp < INT32_MIN) {
            regPtr = cpu_rec_hostreg_var(state, NULL, QWORD, 0);
            ptr64 = 1;

            // printf("%s: reg %d: ptr %p\n", __FUNCTION__, i,
            // state->rec.host[i].ptr);
            EMIT_REX_RBI(REG_NONE, regPtr, REG_NONE, QWORD);
            EMIT(0xb8 + (regPtr & 7));
            EMIT8u(state->rec.host[i].ptr);
        }

        switch (size) {
        case 1:
            EMIT_REX_RBI(i, regPtr, REG_NONE, size);
            EMIT(0x88);
            break;
        case 2:
            EMIT(P_WORD);
        case 4:
        case 8:
            EMIT_REX_RBI(i, regPtr, REG_NONE, size);
            EMIT(0x89);
            break;
        default:
            panic("host reg %d: invalid size %d!\n", i, size);
        }

        if (ptr64) {
            EMIT(MODRM_REG_RM(i, regPtr));
            cpu_rec_hostreg_release(state, regPtr);
        } else {
            EMIT(MODRM_RIP_DISP32(i));
            EMIT4i(OFFSET(state->rec.host[i].ptr, 4));
        }
    }
    memset(&state->rec.host[i], 0, sizeof(state->rec.host[i]));
}

void cpu_rec_hostreg_release_mask(cpu_state *state, unsigned int mask)
{
    for (int i = 0; i < 16; ++i) {
        if (mask & (1 << i)) {
            cpu_rec_hostreg_release(state, i);
        }
    }
}

void cpu_rec_hostreg_release_all(cpu_state *state)
{
    for (int i = 0; i < 16; ++i) {
        cpu_rec_hostreg_release(state, i);
    }
}

void cpu_rec_hostreg_unfreeze(cpu_state *state, int hostreg)
{
    state->rec.host[hostreg].use &= ~CPU_HOST_REG_FROZEN;
}

void cpu_rec_hostreg_freeze(cpu_state *state, int hostreg)
{
    // if (state->rec.host[hostreg].use == 0) {
    state->rec.host[hostreg].use |= CPU_HOST_REG_FROZEN;
    //}
}

static void cpu_rec_hostreg_readvar(cpu_state *state, int reg, void *ptr,
                                    size_t size, int flags)
{
    if (size < 4) {
        // XOR reg, reg
        EMIT_REX_RBI(reg, reg, REG_NONE, DWORD);
        EMIT(0x33);
        EMIT(MODRM_REG_DIRECT(reg, reg));
    }

    // MOV reg, [byte|word|dword|qword] ptr
    ptrdiff_t disp = (uint8_t *)ptr - state->rec.jit_p;
    int ptr64 = 0;
    int regPtr = REG_NONE;

    if (disp > INT32_MAX || disp < INT32_MIN) {
        regPtr = cpu_rec_hostreg_var(state, NULL, QWORD, 0);
        ptr64 = 1;

        EMIT_REX_RBI(REG_NONE, regPtr, REG_NONE, QWORD);
        EMIT(0xb8 + (regPtr & 7));
        EMIT8u(ptr);
    }

    switch (size) {
    case 1:
        EMIT_REX_RBI(reg, regPtr, REG_NONE, size);
        EMIT(0x8a);
        break;
    case 2:
        EMIT(P_WORD);
    case 4:
    case 8:
        EMIT_REX_RBI(reg, regPtr, REG_NONE, size);
        EMIT(0x8b);
        break;
    }

    if (ptr64) {
        EMIT(MODRM_REG_RM(reg, regPtr));
        cpu_rec_hostreg_release(state, regPtr);
    } else {
        EMIT(MODRM_RIP_DISP32(reg));
        EMIT4i(OFFSET(ptr, 4));
    }
}

/*
 * XXX:
 *
 * Need to specify the registers that need to be preserved for a given
 * cpu_rec_XXX_op.
 *
 * Otherwise, the following might happen:
 *
 * int regSrc = HOSTREG_STATE_VAR_R(...); // -> RDX, was already cached
 * int regDst = HOSTREG_STATE_VAR_W(...); // -> eviction, and RDX chosen!
 *
 *
 */

int cpu_rec_hostreg_var(cpu_state *state, void *ptr, size_t size, int flags)
{
    int lru_time = INT_MAX;
    int lru = 0;
    int reg = 0, i = 0;

    ++state->rec.time;

    if (ptr != NULL) {
        for (; i < 16; ++i) {
            if (state->rec.host[i].use == CPU_HOST_REG_VAR &&
                ptr == state->rec.host[i].ptr) {
                // If this call adds a read flag, make sure we read it
                // if (((state->rec.host[i].flags & CPU_VAR_READ) == 0) &&
                //    (flags & CPU_VAR_READ)) {
                //   cpu_rec_hostreg_readvar(state, i, ptr, size, flags);
                //}
                state->rec.host[i].flags |= flags;
                state->rec.host[i].last_access_time = state->rec.time;
                return i;
            }
        }
    }
    if (size != 1 && size != 2 && size != 4 && size != 8) {
        panic("host reg %d: invalid size %d!\n", i, size);
    }
    for (i = 0; i < 16; ++i) {
        if (state->rec.host[i].use == 0) {
            break;
        }
        if (state->rec.host[i].use & CPU_HOST_REG_FROZEN) {
            continue;
        }
        if (state->rec.host[i].use == CPU_HOST_REG_VAR &&
            state->rec.host[i].last_access_time < lru_time) {
            lru_time = state->rec.host[i].last_access_time;
            lru = i;
        }
    }

    reg = i < 16 ? i : lru;

    if (i == 16) {
        cpu_rec_hostreg_release(state, reg);
    }

    if (flags & CPU_VAR_ADDRESS_OF) {
        if (size != QWORD) {
            panic("%s: address not 64-bits wide!", __FUNCTION__);
        }

        ptrdiff_t disp = (uint8_t *)ptr - state->rec.jit_p;
        if (disp <= INT32_MAX && disp >= INT32_MIN) {
            // LEA reg, qword [ptr]
            EMIT_REX_RBI(reg, REG_NONE, REG_NONE, QWORD);
            EMIT(0x8d);
            EMIT(MODRM_RIP_DISP32(reg));
            EMIT4i(OFFSET(ptr, 4));
        } else {
            // MOV reg, qword ptr
            EMIT_REX_RBI(REG_NONE, reg, REG_NONE, QWORD);
            EMIT(0xb8 + (reg & 7));
            EMIT8u(ptr);
        }

    } else {
        if (flags & CPU_VAR_READ) {
            cpu_rec_hostreg_readvar(state, reg, ptr, size, flags);
        } else {
            // We want to clear top bits as WORD/BYTE ops are not zero-extended
            if (!(flags & CPU_VAR_DIRTY) && size < 4) {
                // XOR reg, reg
                EMIT_REX_RBI(reg, reg, REG_NONE, DWORD);
                EMIT(0x33);
                EMIT(MODRM_REG_DIRECT(reg, reg));
            }
        }
    }

    // printf("%s: reg %d: ptr %p\n", __FUNCTION__, reg, ptr);
    state->rec.host[reg].use = CPU_HOST_REG_VAR;
    state->rec.host[reg].ptr = ptr;
    state->rec.host[reg].size = size;
    state->rec.host[reg].flags = flags;
    state->rec.host[reg].last_access_time = state->rec.time;
    return reg;
}

/*
 * Reassign hostreg to a different ptr.
 */
void cpu_rec_hostreg_convert_to_w_var(cpu_state *state, void *ptr, size_t size,
                                      int reg)
{
    cpu_rec_hostreg_release(state, reg);
    state->rec.host[reg].use = CPU_HOST_REG_VAR;
    state->rec.host[reg].ptr = ptr;
    state->rec.host[reg].size = size;
    state->rec.host[reg].flags = CPU_VAR_WRITE;
    state->rec.host[reg].last_access_time = state->rec.time;
}

/*
 * Helper function to release a register (with write-back if WRITE flag set),
 * but preserve the register contents as a temp variable for further use.
 */
void cpu_rec_hostreg_convert_to_tempvar(cpu_state *state, int hostreg)
{
    auto reg = state->rec.host[hostreg];
    reg.use = 0;
    cpu_rec_hostreg_release(state, hostreg);
    state->rec.host[hostreg] = reg;
}

/*
 * Ignore the registers which the SysV x86_64 calling preserves across
 * function calls.
 * Otherwise we'd have to push/pop.
 */
void cpu_rec_hostreg_preserve(cpu_state *state)
{
    cpu_rec_hostreg_freeze(state, RBX);
    cpu_rec_hostreg_freeze(state, RSP);
    cpu_rec_hostreg_freeze(state, RBP);
    cpu_rec_hostreg_freeze(state, R12);
    cpu_rec_hostreg_freeze(state, R13);
    cpu_rec_hostreg_freeze(state, R14);
    cpu_rec_hostreg_freeze(state, R15);
}

/*
 * Write the necessary boilerplate to save registers, setup stack, etc.
 * - `mov rdi, qword ptr [state]` for all opcode calls
 */
static void cpu_rec_compile_start(cpu_state *state, uint16_t a)
{
    cpu_rec_hostreg_preserve(state);

    int regEndPc = HOSTREG_STATE_VAR_W(rec.bblk_pcN, WORD);
    // MOV regEndPc, [state->rec.bblk_map[a].end_pc]
    EMIT_REX_RBI(regEndPc, REG_NONE, REG_NONE, WORD);
    EMIT(P_WORD);
    EMIT(0x8b);
    EMIT(MODRM_RIP_DISP32(regEndPc));
    EMIT4i(OFFSET(&state->rec.bblk_map[a].end_pc, 4));

    cpu_rec_hostreg_release(state, regEndPc);
    int regPc = HOSTREG_STATE_VAR_W(pc, WORD);
    // MOV eax, a
    EMIT_REX_RBI(REG_NONE, regPc, REG_NONE, DWORD);
    EMIT(0xb8 + (regPc & 7));
    EMIT4u(a);
}

/* Write the necessary boilerplate to restore registers, and return. */
static void cpu_rec_compile_end(cpu_state *state)
{
    cpu_rec_hostreg_release_all(state);
    EMIT(0xc3); // ret
}

/*
 * The CPU JIT currently does one-pass translation with dynamic register
 * allocation. */

static void cpu_rec_compile_instr(cpu_state *state, uint16_t a)
{
    // Copy instruction 32 bits to state
    state->i = *(instr *)(&state->m[a]);

    // MOV reg, instr
    int regInstr = HOSTREG_STATE_VAR_W(i, DWORD);
    EMIT_REX_RBI(REG_NONE, regInstr, REG_NONE, DWORD);
    EMIT(0xb8 + (regInstr & 7));
    EMIT4u(state->i.dword);

    // Add 4 to PC
    int regPc = HOSTREG_STATE_VAR_RW(pc, WORD);
    // ADD eax, 4
    EMIT_REX_RBI(REG_NONE, regPc, REG_NONE, DWORD);
    EMIT(0x83);
    EMIT(MODRM_REG_IMM8(regPc));
    EMIT(4);

    // Call `op_instr`
    cpu_rec_dispatch(state, state->m[a]);
}

void cpu_rec_validate(cpu_state *state, uint16_t a)
{
    cpu_rec_bblk *bblk = &state->rec.bblk_map[a];
    for (auto aa = a; aa < bblk->end_pc; aa += 4) {
        int i = aa >> 3;
        int bit = aa & 7;
        int mask = 1 << bit;
        if (state->rec.dirty_map[i] & mask) {
            // Don't break yet, as we want to clear all bits corresponding to
            // this basic block.
            state->rec.dirty_map[i] &= ~mask;
            bblk->invalid = true;
        }
    }
    if (bblk->code && bblk->invalid) {
        if (use_verbose)
            printf("> invalidate dirty bblk @ 0x%04x [%p]\n", a, bblk->code);
        // Reset page protections so we can rewrite that basic block.
        mprotect((void *)bblk->code, page_size, PROT_READ | PROT_WRITE);
    }
}

void cpu_rec_compile(cpu_state *state, uint16_t a)
{
    uint16_t start = a, end, nb_instrs;
    int ret, found_branch = 0;
    cpu_rec_bblk *bblk = &state->rec.bblk_map[start];

    std::vector<std::pair<uint16_t, uint16_t>> imm_stores;

    for (end = start; end < UINT16_MAX; end += 4) {
        uint8_t op = state->m[end];

        // RET - we know we hit the end of a subroutine, so return.
        // CALL - we transfer to another basic block, so return.
        // JMP - ditto.
        if ((op & 0xf0) == 0x10) {
            end += 4;
            break;
        }
        // STM - we might modify the rest of the basic block - return.
        // For opcode 0x30 we can examine the immediate address and see if it
        // will indeed affect the basic block or not.
        if ((op & 0xf0) == 0x30) {
            if (op == 0x30) {
                uint16_t hhll = *(uint16_t *)&state->m[end + 2];
                if (hhll >= end + 4) {
                    imm_stores.push_back({end, hhll});
                }
            } else {
                end += 4;
                break;
            }
        }
        if (state->rec.bblk_1per_op != 0) {
            end += 4;
            break;
        }
    }
    // We look back at our immediate stores, and if any wrote to an address
    // within the basic block, we know we must truncate the basic block to that
    // address.
    for (auto const &[store_pc, store_addr] : imm_stores) {
        if (store_addr < end) {
            end = store_pc + 4;
        }
    }
    nb_instrs = (end - a) / 4;

    state->rec.bblk_pc0 = start;
    state->rec.bblk_pcN = end;
    bblk->end_pc = end;

    uint8_t *jit_ptr;
    if (bblk->invalid) {
        jit_ptr = (uint8_t *)bblk->code;
        bblk->invalid = false;
    } else {
        jit_ptr = (uint8_t *)cpu_rec_get_page(state, a);
    }

    if (use_verbose)
        printf("> ... bblk->code @ %p (%d Chip16 instructions)\n", jit_ptr,
               nb_instrs);

    uint8_t *jit_end = jit_ptr + page_size;
    state->rec.jit_p = jit_ptr;
    cpu_rec_compile_start(state, a);
    for (; a < end; a += 4) {
        state->rec.bblk_pcI = a;
        cpu_rec_compile_instr(state, a);

        if ((jit_end - state->rec.jit_p) <= 32) {
            printf("> ... basic block will exceed JIT page, shortening\n");
            raise(SIGTRAP);
        }
        if (state->rec.bblk_stop) {
            nb_instrs = (a - start) / 4 + 1;
            printf("> ... bblk shortened to %d Chip16 instructions\n",
                   nb_instrs);
            break;
        }
    }
    cpu_rec_compile_end(state);
    if (mprotect(jit_ptr, page_size, PROT_EXEC) < 0) {
        fprintf(stderr, "mprotect(%p, %zu, x) failed with errno %d.\n", jit_ptr,
                page_size, errno);
        exit(1);
    }

    state->rec.bblk_map[start].code = (void (*)(void))(jit_ptr);
    state->rec.bblk_map[start].size = state->rec.jit_p - jit_ptr;
    state->rec.bblk_map[start].cycles = nb_instrs;
    if (use_verbose)
        printf("> ... page size %zu, final size: %zu bytes\n", page_size,
               state->rec.bblk_map[start].size);
}
