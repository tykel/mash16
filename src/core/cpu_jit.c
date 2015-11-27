/*
 *   mash16 - the chip16 emulator
 *   Copyright (C) 2012-2015 tykel
 *
 *   mash16 is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   mash16 is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with mash16.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/mman.h>

#include <libjit.h>

#include "cpu.h"
#include "cpu_jit.h"
#include "audio.h"

typedef void (*pfn_jitblock)(void);

static void* gp_blockmap[0x10000 >> 2];
static void* gp_unaligned_blockmap[0x10000 >> 2];

static pfn_jit_op gpfn_jit_ops[0x100] = { NULL };

void cpu_jit_translate(cpu_state *state)
{
    gpfn_jit_ops[i_op(state->i)](state);
}

void cpu_jit_init(cpu_state *state)
{
    int n;
    
    jit_create(&state->j, JIT_FLAG_NONE);
    
    for(n = 0; n < (0x10000>>2); n++) {
        gp_unaligned_blockmap[n] = NULL;
        gp_blockmap[n] = NULL;
    }

    gpfn_jit_ops[0x00] = cpu_emit_nop;
    gpfn_jit_ops[0x01] = cpu_emit_cls;
    gpfn_jit_ops[0x02] = cpu_emit_vblnk;
    gpfn_jit_ops[0x03] = cpu_emit_bgc;
    gpfn_jit_ops[0x04] = cpu_emit_spr;
    gpfn_jit_ops[0x05] = cpu_emit_drw_i;
    gpfn_jit_ops[0x06] = cpu_emit_drw_r;
    gpfn_jit_ops[0x07] = cpu_emit_rnd;
    gpfn_jit_ops[0x08] = cpu_emit_flip;
    gpfn_jit_ops[0x09] = cpu_emit_snd0;
    gpfn_jit_ops[0x0a] = cpu_emit_snd1;
    gpfn_jit_ops[0x0b] = cpu_emit_snd2;
    gpfn_jit_ops[0x0c] = cpu_emit_snd3;
    gpfn_jit_ops[0x0d] = cpu_emit_snp;
    gpfn_jit_ops[0x0e] = cpu_emit_sng;
    gpfn_jit_ops[0x10] = cpu_emit_jmp_i;
    gpfn_jit_ops[0x11] = cpu_emit_jmc;
    gpfn_jit_ops[0x12] = cpu_emit_jx;
    gpfn_jit_ops[0x13] = cpu_emit_jme;
    gpfn_jit_ops[0x14] = cpu_emit_call_i;
    gpfn_jit_ops[0x15] = cpu_emit_ret;
    gpfn_jit_ops[0x16] = cpu_emit_jmp_r;
    gpfn_jit_ops[0x17] = cpu_emit_cx;
    gpfn_jit_ops[0x18] = cpu_emit_call_r;
    gpfn_jit_ops[0x20] = cpu_emit_ldi;
    gpfn_jit_ops[0x21] = cpu_emit_ldi_sp;
    gpfn_jit_ops[0x22] = cpu_emit_ldm_i;
    gpfn_jit_ops[0x23] = cpu_emit_ldm_r;
    gpfn_jit_ops[0x24] = cpu_emit_mov;
    gpfn_jit_ops[0x30] = cpu_emit_stm_i;
    gpfn_jit_ops[0x31] = cpu_emit_stm_r;
    gpfn_jit_ops[0x40] = cpu_emit_addi;
    gpfn_jit_ops[0x41] = cpu_emit_add_r2;
    gpfn_jit_ops[0x42] = cpu_emit_add_r3;
    gpfn_jit_ops[0x50] = cpu_emit_subi;
    gpfn_jit_ops[0x51] = cpu_emit_sub_r2;
    gpfn_jit_ops[0x52] = cpu_emit_sub_r3;
    gpfn_jit_ops[0x53] = cpu_emit_cmpi;
    gpfn_jit_ops[0x54] = cpu_emit_cmp;
    gpfn_jit_ops[0x60] = cpu_emit_andi;
    gpfn_jit_ops[0x61] = cpu_emit_and_r2;
    gpfn_jit_ops[0x62] = cpu_emit_and_r3;
    gpfn_jit_ops[0x63] = cpu_emit_tsti;
    gpfn_jit_ops[0x64] = cpu_emit_tst;
    gpfn_jit_ops[0x70] = cpu_emit_ori;
    gpfn_jit_ops[0x71] = cpu_emit_or_r2;
    gpfn_jit_ops[0x72] = cpu_emit_or_r3;
    gpfn_jit_ops[0x80] = cpu_emit_xori;
    gpfn_jit_ops[0x81] = cpu_emit_xor_r2;
    gpfn_jit_ops[0x82] = cpu_emit_xor_r3;
    gpfn_jit_ops[0x90] = cpu_emit_muli;
    gpfn_jit_ops[0x91] = cpu_emit_mul_r2;
    gpfn_jit_ops[0x92] = cpu_emit_mul_r3;
    gpfn_jit_ops[0xa0] = cpu_emit_divi;
    gpfn_jit_ops[0xa1] = cpu_emit_div_r2;
    gpfn_jit_ops[0xa2] = cpu_emit_div_r3;
    gpfn_jit_ops[0xb0] = cpu_emit_shl_n;
    gpfn_jit_ops[0xb1] = cpu_emit_shr_n;
    gpfn_jit_ops[0xb2] = cpu_emit_sar_n;
    gpfn_jit_ops[0xb3] = cpu_emit_shl_r;
    gpfn_jit_ops[0xb4] = cpu_emit_shr_r;
    gpfn_jit_ops[0xb5] = cpu_emit_sar_r;
    gpfn_jit_ops[0xc0] = cpu_emit_push;
    gpfn_jit_ops[0xc1] = cpu_emit_pop;
    gpfn_jit_ops[0xc2] = cpu_emit_pushall;
    gpfn_jit_ops[0xc3] = cpu_emit_popall;
    gpfn_jit_ops[0xc4] = cpu_emit_pushf;
    gpfn_jit_ops[0xc5] = cpu_emit_popf;
    gpfn_jit_ops[0xd0] = cpu_emit_pal_i;
    gpfn_jit_ops[0xd1] = cpu_emit_pal_r;
    gpfn_jit_ops[0xe0] = cpu_emit_noti;
    gpfn_jit_ops[0xe1] = cpu_emit_not_r1;
    gpfn_jit_ops[0xe2] = cpu_emit_not_r2;
    gpfn_jit_ops[0xe3] = cpu_emit_negi;
    gpfn_jit_ops[0xe4] = cpu_emit_neg_r1;
    gpfn_jit_ops[0xe5] = cpu_emit_neg_r2;
}

void cpu_jit_destroy(cpu_state *state)
{
    int n;
    
    jit_destroy(state->j);
    state->j = NULL;
    
    for(n = 0; n < (0x10000>>2); n++) {
        if(gp_unaligned_blockmap[n] != NULL) {
            free(gp_unaligned_blockmap[n]);
            gp_unaligned_blockmap[n] = NULL;
        }
        gp_blockmap[n] = NULL;
    }
}

void cpu_jit_run(cpu_state *state)
{
    uint16_t start = state->pc;
    void *b = cpu_jit_get_block(state, start);
    ((pfn_jitblock)b)();
    printf(".");
    fflush(stdout);
}

void* cpu_jit_compile_block(cpu_state *state, uint16_t a)
{
    uint16_t start = a;
    uint16_t end = start;
    uint16_t furthest = end;
    size_t n_instrs = 0, n, pc;
    void *p_unaligned = NULL, *p;

    for(; end <= 0xfffc; n_instrs++, end = end + 4) {
        uint8_t op = state->m[end];
        uint16_t hhll = *(uint16_t *)&state->m[end + 2];
        /* A RET with no branch dests. after it means we're done. */
        if(op == 0x15 && end >= furthest) { /* RET */
            break;
        /* A branch to after end means we still need to go there. */
        } else if(op == 0x12) { /* Jx */
            if(hhll > end) {
                furthest = hhll;
            }
        /* A JMP to somewhere we've been means we're done. */
        } else if(op == 0x10) { /* JMP */
            if(hhll >= start && hhll <= end) {
                break;
            }
        }
    }
    /* Make end the first address that is out of bounds. */
    end = end + 4;
    printf("basic block found: 0x%04x - 0x%04x\n", start, end);

    /* Make room for at least 16 bytes per Chip16 instruction, be safe. */
    p_unaligned = malloc(BYTES_TO_PAGESZ(16 * n_instrs) + PAGESZ);
    if(p_unaligned == NULL) {
        fprintf(stderr, "error: failed to allocate %zu byte cache block\n",
                BYTES_TO_PAGESZ(16 * n_instrs));
        return NULL;
    }
    p = (void *)(((uint64_t)p_unaligned + PAGESZ-1) & ~(PAGESZ-1));
    printf("allocated %zu bytes for jit block\n",
        BYTES_TO_PAGESZ(16 * n_instrs));

    /* Block the stack pointer so we don't need to restore it. */
    jit_reg_new_fixed(state->j, JIT_REGMAP_SP);
    cpu_jit_emit_zeroregs(state);
    /* Iterate through the instructions, translating them one at a time. */
    for(n = 0, pc = start; n < n_instrs && pc < end; n++, pc += 4) {
        state->i = *(instr*)(&state->m[pc]);
        cpu_jit_translate(state);
    }
    cpu_jit_emit_cyclecount(state, n_instrs);
    cpu_jit_emit_ret(state);

    /* Emit the generated instructions to the buffer. */
    jit_begin_block(state->j, p);
    jit_emit_all(state->j);
    jit_end_block(state->j);

    /* Ensure we can execute the code. */
    mprotect(p, PAGESZ, PROT_READ | PROT_WRITE | PROT_EXEC);

    /* Add this block to the cache. */
    gp_blockmap[a>>2] = p;
    gp_unaligned_blockmap[a>>2] = p_unaligned;

    {
        FILE *f = fopen("./block.o", "wb");
        fwrite(p, 1, state->j->blk_nb, f);
        fclose(f);
    }

    return p; 
}

void* cpu_jit_get_block(cpu_state *state, uint16_t a)
{
    void *p_block = NULL;

    /* Compile the block if it hasn't been visited yet! */
    if(gp_blockmap[a >> 2] == NULL) {
        gp_blockmap[a >> 2] = cpu_jit_compile_block(state, a);
    }
    p_block = gp_blockmap[a >> 2];

    return p_block;
}

void cpu_jit_emit_zeroregs(cpu_state *state)
{
    int n;
    struct jit_instr *i[1];
    jit_reg rrax = jit_reg_new_fixed(state->j, JIT_REGMAP_CALL_RET);

    for(n = 0; n < 1; n++) {
        i[n] = jit_instr_new(state->j);
    }

    XOR_R_R_R(i[0], rrax, rrax, rrax, JIT_32BIT);
}

void cpu_jit_emit_ret(cpu_state *state)
{
    struct jit_instr *i = jit_instr_new(state->j);
    RET(i);
}

void cpu_jit_emit_cyclecount(cpu_state *state, size_t ni)
{
    struct jit_instr *i0, *i1, *i2;
    i0 = jit_instr_new(state->j);
    i1 = jit_instr_new(state->j);
    i2 = jit_instr_new(state->j);
    MOVE_M_R(i0, &state->meta.cycles, jit_reg_new(state->j), JIT_64BIT);
    ADD_I_R_R(i1, ni, i0->out.reg, i0->out.reg, JIT_32BIT);
    MOVE_R_M(i2, i0->out.reg, &state->meta.cycles, JIT_64BIT);
}

