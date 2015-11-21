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

static pfn_jit_op gpfn_jit_ops[0x100];

void cpu_jit_translate(cpu_state *state)
{
    /*gpfn_jit_ops[i_op(state->i)](state);*/
}

void cpu_jit_init(cpu_state *state)
{
    jit_create(&state->j, JIT_FLAG_NONE);
}

void cpu_jit_destroy(cpu_state *state)
{
    jit_destroy(state->j);
    state->j = NULL;
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

    /* Iterate through the instructions, translating them one at a time. */
    for(n = 0, pc = start; n < n_instrs && pc < end; n++, pc += 4) {
        state->i = *(instr*)(&state->m[pc]);
        cpu_jit_translate(state);
    }

    /* Emit the generated instructions to the buffer. */
    jit_begin_block(state->j, p);
    jit_emit_all(state->j);
    jit_end_block(state->j);

    /* Ensure we can execute the code. */
    mprotect(p, PAGESZ, PROT_READ | PROT_WRITE | PROT_EXEC);
    free(p_unaligned);

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

#if 0
int cpu_jit_op_drw(cpu_state *state, size_t moffs, int x, int y)
{
    return op_drw(&state->m[moffs], state->vm, x, y, state->sw, state->sh, state->fx, state->fy);
}

void cpu_emit__error(cpu_state *s)
{
    fprintf(stderr, "warning: unknown op encountered\n");
}

void cpu_emit_nop(cpu_state *s)
{
}

void cpu_emit_cls(cpu_state *s)
{
    struct jit_instr *i = NULL;
    i = jit_instr_new(s->j);
    CALL_M(i, op_cls, JIT_32BIT);
}

void cpu_emit_vblnk(cpu_state *s)
{
    struct jit_instr *i = NULL;
    i = jit_instr_new(s->j);
    CALL_M(i, op_vblnk, JIT_32BIT);
}

void cpu_emit_bgc(cpu_state *s)
{
    struct jit_instr *i0, *i1;
    i0 = jit_instr_new(s->j);
    i1 = jit_instr_new(s->j);
    MOVE_I_R(i0, i_n(s->i), jit_reg_new(s->j), JIT_32BIT);
    MOVE_R_M(i1, i0->out.reg, &s->bgc, JIT_8BIT);
}

void cpu_emit_spr(cpu_state *s)
{
    struct jit_instr *i0, *i1, *i2, *i3, *i4, *i5;
    i0 = jit_instr_new(s->j);
    i1 = jit_instr_new(s->j);
    i2 = jit_instr_new(s->j);
    i3 = jit_instr_new(s->j);
    i4 = jit_instr_new(s->j);
    i5 = jit_instr_new(s->j);
    MOVE_I_R(i0, i_hhll(s->i), jit_reg_new(s->j), JIT_32BIT);
    MOVE_R_R(i1, i0->out.reg, jit_reg_new(s->j), JIT_32BIT);
    AND_I_R_R(i2, 0x00ff, i0->out.reg, i0->out.reg, JIT_32BIT);
    SHR_I_R(i3, 8, i1->out.reg, JIT_32BIT);
    MOVE_R_M(i4, i0->out.reg, &s->sw, JIT_8BIT);
    MOVE_R_M(i5, i1->out.reg, &s->sh, JIT_8BIT);
}

void cpu_emit_drw_i(cpu_state *s)
{
    struct jit_instr *i0, *i1, *i2, *i3, *i4, *i5;
    jit_reg a = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG0);
    jit_reg x = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG1);
    jit_reg y = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG2);
    jit_reg c = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_RET);
    
    i0 = jit_instr_new(s->j);
    i1 = jit_instr_new(s->j);
    i2 = jit_instr_new(s->j);
    i3 = jit_instr_new(s->j);
    i4 = jit_instr_new(s->j);
    i5 = jit_instr_new(s->j);
    
    MOVE_I_R(i0, i_hhll(s->i), a, JIT_32BIT);
    MOVE_ID_R(i1, &s->r, y, JIT_32BIT);
    MOVE_RP_R(i2, y, JIT_REG_INVALID, 0, i_yx(s->i) & 0x0f, x, JIT_32BIT);
    MOVE_RP_R(i3, y, JIT_REG_INVALID, 0, i_yx(s->i) >> 4, y, JIT_32BIT);
    CALL_M(i4, op_drw, JIT_32BIT);
    MOVE_R_M(i5, c, &s->f.c, JIT_32BIT);
}

void cpu_emit_drw_r(cpu_state *s)
{
    /* TODO: We assume a simplified op_drw(x, y, m) */
    int n;
    struct jit_instr *i[7];
    jit_reg a = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG0);
    jit_reg x = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG1);
    jit_reg y = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG2);
    jit_reg c = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_RET);
    
    for(n = 0; n < 7; n++)
        i[n] = jit_instr_new(s->j);
    
    MOVE_ID_R(i[0], &s->r, y, JIT_32BIT);
    MOVE_RP_R(i[3], y, JIT_REG_INVALID, 0, i_z(s->i) & 0x0f, a, JIT_32BIT);
    MOVE_RP_R(i[1], y, JIT_REG_INVALID, 0, i_yx(s->i) & 0x0f, x, JIT_32BIT);
    MOVE_RP_R(i[2], y, JIT_REG_INVALID, 0, i_yx(s->i) >> 4, y, JIT_32BIT);
    CALL_M(i[4], op_drw, JIT_32BIT);
    MOVE_R_M(i[5], c, &s->f.c, JIT_32BIT);
}

void cpu_emit_rnd(cpu_state *s)
{
    int n;
    struct jit_instr *i[5];
    jit_reg x = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG0);
    jit_reg r = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_RET);
    
    for(n = 0; n < 5; n++)
        i[n] = jit_instr_new(s->j);

    XOR_R_R_R(i[0], x, x, x, JIT_32BIT);
    CALL_M(i[1], rand, JIT_32BIT);
    AND_I_R_R(i[2], i_hhll(s->i), r, r, JIT_32BIT);
    MOVE_ID_R(i[3], &s->r, x, JIT_32BIT);
    MOVE_R_RP(i[4], x, x, JIT_REG_INVALID, 0, i_yx(s->i) & 0x0f, JIT_32BIT);
}

void cpu_emit_flip(cpu_state *s)
{
    int n;
    struct jit_instr *i[5];
    jit_reg a = jit_reg_new(s->j);
    jit_reg b = jit_reg_new(s->j);

    for(n = 0; n < 5; n++)
        i[n] = jit_instr_new(s->j);

    MOVE_I_R(i[0], i_hhll(s->i), a, JIT_32BIT);
    SHL_I_R_R(i[1], 1, a, b, JIT_32BIT);
    AND_I_R_R(i[2], 1, a, a, JIT_32BIT);
    OR_R_R_R(i[3], b, a, a, JIT_32BIT);
    MOVE_R_M(i[4], a, (int16_t *)&s->fx, JIT_16BIT);
}

void cpu_emit_snd0(cpu_state *s)
{
    int n;
    struct jit_instr *i[1];

    for(n = 0; n < 1; n++)
        i[n] = jit_instr_new(s->j);
    
    CALL_M(i[0], (int32_t *)audio_stop, JIT_32BIT);
}

void cpu_emit_snd1(cpu_state *s)
{
    int n;
    struct jit_instr *i[4];
    jit_reg f = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG0);
    jit_reg d = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG1);
    jit_reg a = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG2);

    for(n = 0; n < 4; n++)
        i[n] = jit_instr_new(s->j);
    
    MOVE_I_R(i[0], 500, f, JIT_32BIT);
    MOVE_I_R(i[1], i_hhll(s->i), d, JIT_32BIT);
    MOVE_I_R(i[2], 0, a, JIT_32BIT);
    CALL_M(i[3], (int32_t *)audio_play, JIT_32BIT);
}

void cpu_emit_snd2(cpu_state *s)
{
    int n;
    struct jit_instr *i[4];
    jit_reg f = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG0);
    jit_reg d = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG1);
    jit_reg a = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG2);

    for(n = 0; n < 4; n++)
        i[n] = jit_instr_new(s->j);
    
    MOVE_I_R(i[0], 1000, f, JIT_32BIT);
    MOVE_I_R(i[1], i_hhll(s->i), d, JIT_32BIT);
    MOVE_I_R(i[2], 0, a, JIT_32BIT);
    CALL_M(i[3], (int32_t *)audio_play, JIT_32BIT);
}

void cpu_emit_snd3(cpu_state *s)
{
    int n;
    struct jit_instr *i[4];
    jit_reg f = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG0);
    jit_reg d = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG1);
    jit_reg a = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG2);

    for(n = 0; n < 4; n++)
        i[n] = jit_instr_new(s->j);
    
    MOVE_I_R(i[0], 1500, f, JIT_32BIT);
    MOVE_I_R(i[1], i_hhll(s->i), d, JIT_32BIT);
    MOVE_I_R(i[2], 0, a, JIT_32BIT);
    CALL_M(i[3], (int32_t *)audio_play, JIT_32BIT);
}

#endif
