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

#include <libjit.h>

#include "cpu.h"
#include "cpu_jit.h"
#include "audio.h"

int cpu_jit_op_drw(cpu_state *state, size_t moffs, int x, int y)
{
    return op_drw(&state->m[moffs], state->vm, x, y, state->sw, state->sh, state->fx, state->fy);
}

void cpu_emit__error(cpu_state *s)
{
    fprintf(stderr, "warning: unknown op encountered\n");
}

void cpu_emit__flags(cpu_state *s, uint32_t flags)
{
    // TODO!
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
    struct jit_instr *i0, *i1;
    
    i0 = jit_instr_new(s->j);
    i1 = jit_instr_new(s->j);
    
    MOVE_I_R(i0, 1, jit_reg_new(s->j), JIT_32BIT);
    MOVE_R_M(i1, i0->out.reg, &s->meta.wait_vblnk, JIT_32BIT);
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
    SHR_R_I_R(i3, i1->out.reg, 8, i1->out.reg, JIT_32BIT);
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
    int16_t *p_regx = &s->r[i_yx(s->i)&0x0f];
    int16_t *p_regy = &s->r[i_yx(s->i)>>4];
    
    i0 = jit_instr_new(s->j);
    i1 = jit_instr_new(s->j);
    i2 = jit_instr_new(s->j);
    i3 = jit_instr_new(s->j);
    i4 = jit_instr_new(s->j);
    
    MOVE_I_R(i0, i_hhll(s->i), a, JIT_32BIT);
    MOVE_M_R(i1, p_regx, x, JIT_16BIT);
    MOVE_M_R(i2, p_regy, y, JIT_16BIT);
    CALL_M(i3, op_drw, JIT_32BIT);
    MOVE_R_M(i4, c, &s->f.c, JIT_32BIT);
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
    MOVE_RP_R(i[3], y, JIT_REG_INVALID, 0, 2*(i_z(s->i)&0x0f), a, JIT_16BIT);
    MOVE_RP_R(i[1], y, JIT_REG_INVALID, 0, 2*(i_yx(s->i)&0x0f), x, JIT_16BIT);
    MOVE_RP_R(i[2], y, JIT_REG_INVALID, 0, 2*(i_yx(s->i)>>4), y, JIT_16BIT);
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
    MOVE_R_RP(i[4], x, x, JIT_REG_INVALID, 0, 2*(i_yx(s->i)&0x0f), JIT_32BIT);
}

void cpu_emit_flip(cpu_state *s)
{
    int n;
    struct jit_instr *i[5];
    jit_reg a = jit_reg_new(s->j);
    uint16_t imm = i_hhll(s->i);

    for(n = 0; n < 5; n++)
        i[n] = jit_instr_new(s->j);

    MOVE_I_R(i[0], ((imm&2) << 8) | (imm&1), a, JIT_32BIT);
    MOVE_R_M(i[1], a, (int16_t *)&s->fx, JIT_16BIT);
}

void cpu_emit_snd0(cpu_state *s)
{
    int n;
    struct jit_instr *i[1];

    for(n = 0; n < 1; n++)
        i[n] = jit_instr_new(s->j);
    
    CALL_M(i[0], audio_stop, JIT_32BIT);
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
    CALL_M(i[3], audio_play, JIT_32BIT);
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
    CALL_M(i[3], audio_play, JIT_32BIT);
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
    CALL_M(i[3], audio_play, JIT_32BIT);
}

void cpu_emit_snp(cpu_state *s)
{
    int n;
    struct jit_instr *i[5];
    
    jit_reg f = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG0);
    jit_reg dt = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG1);
    jit_reg one = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG2);

    for(n = 0; n < 5; n++)
        i[n] = jit_instr_new(s->j);

    MOVE_ID_R(i[0], &s->m, f, JIT_32BIT);
    MOVE_RP_R(i[1], f, JIT_REG_INVALID, 0, 2*(i_yx(s->i)&0xf), f, JIT_16BIT);
    MOVE_I_R(i[2], i_hhll(s->i), dt, JIT_32BIT);
    MOVE_I_R(i[3], 1, one, JIT_32BIT);
    CALL_M(i[4], audio_play, JIT_32BIT); 
}

void cpu_emit_sng(cpu_state *s)
{
    int n;
    struct jit_instr *i[14];
    
    jit_reg rs = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG0);
    jit_reg rt = jit_reg_new(s->j);

    for(n = 0; n < 14; n++)
        i[n] = jit_instr_new(s->j);

    MOVE_I_R(i[0], (i_yx(s->i))>>4, rt, JIT_8BIT);
    MOVE_R_M(i[1], rt, &s->atk, JIT_32BIT);
    MOVE_I_R(i[2], (i_yx(s->i))&0x0f, rt, JIT_8BIT);
    MOVE_R_M(i[3], rt, &s->dec, JIT_32BIT);
    MOVE_I_R(i[4], (i_hhll(s->i)>>12)&0x0f, rt, JIT_8BIT);
    MOVE_R_M(i[5], rt, &s->vol, JIT_32BIT);
    MOVE_I_R(i[6], (i_hhll(s->i)>>8)&0x0f, rt, JIT_8BIT);
    MOVE_R_M(i[7], rt, &s->type, JIT_32BIT);
    MOVE_I_R(i[8], (i_hhll(s->i)>>4)&0x0f, rt, JIT_8BIT);
    MOVE_R_M(i[9], rt, &s->sus, JIT_32BIT);
    MOVE_I_R(i[10], (i_hhll(s->i))&0x0f, rt, JIT_8BIT);
    MOVE_R_M(i[11], rt, &s->rls, JIT_32BIT);
    MOVE_ID_R(i[12], s, rs, JIT_32BIT);
    CALL_M(i[13], audio_update, JIT_32BIT);
}

void cpu_emit_jmp_i(cpu_state *s)
{
    int n;
    struct jit_instr *i[2];

    jit_reg x =  jit_reg_new(s->j);
    for(n = 0; n < 8; n++)
        i[n] = jit_instr_new(s->j);

    i[0] = jit_instr_new(s->j);
    i[1] = jit_instr_new(s->j);

    MOVE_I_R(i[0], i_hhll(s->i), x, JIT_32BIT);
    MOVE_R_M(i[1], x, &s->pc, JIT_16BIT);
}

void cpu_emit_jmc(cpu_state *s)
{
    int n;
    struct jit_instr *i[2];
    
    for(n = 0; n < 2; n++)
        i[n] = jit_instr_new(s->j);
    
    MOVE_ID_R(i[0], s, jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG0), JIT_32BIT);
    CALL_M(i[1], op_jmc, JIT_32BIT);
}

void cpu_emit_jx(cpu_state *s)
{
    int n;
    struct jit_instr *i[2];
    
    for(n = 0; n < 2; n++)
        i[n] = jit_instr_new(s->j);
    
    MOVE_ID_R(i[0], s, jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG0), JIT_32BIT);
    CALL_M(i[1], op_jx, JIT_32BIT);
}

void cpu_emit_jme(cpu_state *s)
{
    int n;
    struct jit_instr *i[2];
    
    for(n = 0; n < 2; n++)
        i[n] = jit_instr_new(s->j);
    
    MOVE_ID_R(i[0], s, jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG0), JIT_32BIT);
    CALL_M(i[1], op_jme, JIT_32BIT);
}

void cpu_emit_call_i(cpu_state *s)
{
    int n;
    struct jit_instr *i[8];

    for(n = 0; n < 8; n++)
        i[n] = jit_instr_new(s->j);

    MOVE_M_R(i[0], &s->pc, jit_reg_new(s->j), JIT_16BIT);
    MOVE_ID_R(i[1], &s->m, jit_reg_new(s->j), JIT_32BIT);
    MOVE_M_R(i[2], &s->sp, jit_reg_new(s->j), JIT_16BIT);
    // rzx := chip16.pc
    // rzy := &chip16.m
    // rzz := chip16.sp
    // movw %rzx, (%rzy, %rzz, 1) i.e.: chip16.m[chip16.sp] = chip16.pc
    MOVE_R_RP(i[3], i[0]->out.reg, i[1]->out.reg, i[2]->out.reg, 1, 0, JIT_16BIT);
    MOVE_M_R(i[4], &s->sp, jit_reg_new(s->j), JIT_16BIT);
    // chip16.sp += 2
    ADD_I_R_R(i[5], 2, i[4]->out.reg, i[4]->out.reg, JIT_16BIT);
    // chip16.pc = i.hhll
    MOVE_I_R(i[6], i_hhll(s->i), jit_reg_new(s->j), JIT_16BIT);
    MOVE_R_M(i[7], i[6]->out.reg, &s->pc, JIT_16BIT);
}

void cpu_emit_ret(cpu_state *s)
{
    int n;
    struct jit_instr *i[6];

    for(n = 0; n < 6; n++)
        i[n] = jit_instr_new(s->j);

    // chip16.sp -= 2
    MOVE_M_R(i[0], &s->sp, jit_reg_new(s->j), JIT_16BIT);
    SUB_I_R_R(i[1], 2, i[0]->out.reg, i[0]->out.reg, JIT_32BIT);
    MOVE_R_M(i[2], i[0]->out.reg, &s->sp, JIT_16BIT);
    // chip16.pc = chip16.m[chip16.sp]
    MOVE_ID_R(i[3], &s->m, jit_reg_new(s->j), JIT_32BIT);
    MOVE_RP_R(i[4], i[3]->out.reg, i[0]->out.reg, 1, 0, jit_reg_new(s->j), JIT_16BIT);
    MOVE_R_M(i[5], i[4]->out.reg, &s->pc, JIT_16BIT);
}

void cpu_emit_jmp_r(cpu_state *s)
{
    int n;
    struct jit_instr *i[3];

    for(n = 0; n < 3; n++)
        i[n] = jit_instr_new(s->j);
   
    // chip16.pc = chip16.r[i.x]
    MOVE_ID_R(i[0], &s->r, jit_reg_new(s->j), JIT_32BIT);
    MOVE_RP_R(i[1], i[0]->out.reg, JIT_REG_INVALID, 1, 2*(i_yx(s->i)&0x0f), jit_reg_new(s->j), JIT_16BIT);
    MOVE_R_M(i[2], i[1]->out.reg, &s->pc, JIT_16BIT);
}

void cpu_emit_cx(cpu_state *s)
{
    int n;
    struct jit_instr *i[2];

    for(n = 0; n < 2; n++)
        i[n] = jit_instr_new(s->j);

    MOVE_ID_R(i[0], s, jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG0), JIT_32BIT);
    CALL_M(i[1], op_cx, JIT_32BIT);
}

void cpu_emit_call_r(cpu_state *s)
{
    int n;
    struct jit_instr *i[9];

    for(n = 0; n < 9; n++)
        i[n] = jit_instr_new(s->j);

    MOVE_M_R(i[0], &s->pc, jit_reg_new(s->j), JIT_16BIT);
    MOVE_ID_R(i[1], &s->m, jit_reg_new(s->j), JIT_32BIT);
    MOVE_M_R(i[2], &s->sp, jit_reg_new(s->j), JIT_16BIT);
    // rzx := chip16.pc
    // rzy := &chip16.m
    // rzz := chip16.sp
    // movw %rzx, (%rzy, %rzz, 1) i.e.: chip16.m[chip16.sp] = chip16.pc
    MOVE_R_RP(i[3], i[0]->out.reg, i[1]->out.reg, i[2]->out.reg, 1, 0, JIT_16BIT);
    MOVE_M_R(i[4], &s->sp, jit_reg_new(s->j), JIT_16BIT);
    // chip16.sp += 2
    ADD_I_R_R(i[5], 2, i[4]->out.reg, i[4]->out.reg, JIT_16BIT);
    // chip16.pc = chip16.r[i.x]
    MOVE_ID_R(i[6], &s->r, jit_reg_new(s->j), JIT_16BIT);
    MOVE_RP_R(i[7], i[6]->out.reg, JIT_REG_INVALID, 0, 2*(i_yx(s->i)&0x0f), jit_reg_new(s->j), JIT_16BIT);
    MOVE_R_M(i[8], i[7]->out.reg, &s->pc, JIT_16BIT);
}

void cpu_emit_ldi(cpu_state *s)
{
    int n;
    struct jit_instr *i[3];

    for(n = 0; n < 3; n++)
        i[n] = jit_instr_new(s->j);
    
    MOVE_I_R(i[0], i_hhll(s->i), jit_reg_new(s->j), JIT_16BIT);
    MOVE_ID_R(i[1], &s->r, jit_reg_new(s->j), JIT_16BIT);
    MOVE_R_RP(i[2], i[0]->out.reg, i[1]->out.reg, JIT_REG_INVALID, 0, 2*(i_yx(s->i)&0x0f), JIT_16BIT);
}

void cpu_emit_ldi_sp(cpu_state *s)
{
    int n;
    struct jit_instr *i[2];

    for(n = 0; n < 2; n++)
        i[n] = jit_instr_new(s->j);
    
    MOVE_I_R(i[0], i_hhll(s->i), jit_reg_new(s->j), JIT_16BIT);
    MOVE_R_M(i[1], i[0]->out.reg, &s->sp, JIT_16BIT);
}

void cpu_emit_ldm_i(cpu_state *s)
{
    int n;
    struct jit_instr *i[4];

    for(n = 0; n < 4; n++)
        i[n] = jit_instr_new(s->j);
    
    // state.r[i.x] = state.m[i.hhll]
    MOVE_ID_R(i[0], &s->m, jit_reg_new(s->j), JIT_32BIT);
    MOVE_RP_R(i[1], i[0]->out.reg, JIT_REG_INVALID, 0, i_hhll(s->i), jit_reg_new(s->j), JIT_16BIT);
    MOVE_ID_R(i[2], &s->r, jit_reg_new(s->j), JIT_32BIT);
    MOVE_R_RP(i[3], i[1]->out.reg, i[2]->out.reg, JIT_REG_INVALID, 0, 2*(i_yx(s->i)&0x0f), JIT_16BIT);
}

void cpu_emit_ldm_r(cpu_state *s)
{
    int n;
    struct jit_instr *i[5];

    for(n = 0; n < 5; n++)
        i[n] = jit_instr_new(s->j);
    
    // state.r[i.x] = state.m[state.r[i.y]]
    MOVE_ID_R(i[0], &s->r, jit_reg_new(s->j), JIT_32BIT);
    MOVE_RP_R(i[1], i[0]->out.reg, JIT_REG_INVALID, 0, 2*(i_yx(s->i)&0x0f), jit_reg_new(s->j), JIT_16BIT);
    MOVE_ID_R(i[2], &s->m, jit_reg_new(s->j), JIT_32BIT);
    MOVE_RP_R(i[3], i[2]->out.reg, i[1]->out.reg, 2, 0, jit_reg_new(s->j), JIT_16BIT);
    MOVE_R_RP(i[4], i[3]->out.reg, i[0]->out.reg, JIT_REG_INVALID, 0, 2*(i_yx(s->i)&0x0f), JIT_16BIT);
}

void cpu_emit_mov(cpu_state *s)
{
    int n;
    struct jit_instr *i[3];

    for(n = 0; n < 3; n++)
        i[n] = jit_instr_new(s->j);
    
    // state.r[i.x] = state.r[i.y]
    MOVE_ID_R(i[0], &s->r, jit_reg_new(s->j), JIT_32BIT);
    MOVE_RP_R(i[1], i[0]->out.reg, JIT_REG_INVALID, 0, 2*(i_yx(s->i)>>4), jit_reg_new(s->j), JIT_16BIT);
    MOVE_R_RP(i[2], i[1]->out.reg, i[0]->out.reg, JIT_REG_INVALID, 0, 2*(i_yx(s->i)&0x0f), JIT_16BIT);
}

void cpu_emit_stm_i(cpu_state *s)
{
    int n;
    struct jit_instr *i[4];

    for(n = 0; n < 4; n++)
        i[n] = jit_instr_new(s->j);
    
    // state.m[i.hhll] = state.r[i.x]
    MOVE_ID_R(i[0], &s->r, jit_reg_new(s->j), JIT_32BIT);
    MOVE_RP_R(i[1], i[0]->out.reg, JIT_REG_INVALID, 0, 2*(i_yx(s->i)&0x0f), jit_reg_new(s->j), JIT_16BIT);
    MOVE_ID_R(i[2], &s->m, jit_reg_new(s->j), JIT_32BIT);
    MOVE_R_RP(i[3], i[1]->out.reg, i[2]->out.reg, JIT_REG_INVALID, 0, i_hhll(s->i), JIT_16BIT);
}

void cpu_emit_stm_r(cpu_state *s)
{
    int n;
    struct jit_instr *i[5];

    for(n = 0; n < 5; n++)
        i[n] = jit_instr_new(s->j);
    
    // state.m[state.r[i.y]] = state.r[i.x]
    MOVE_ID_R(i[0], &s->r, jit_reg_new(s->j), JIT_32BIT);
    MOVE_RP_R(i[1], i[0]->out.reg, JIT_REG_INVALID, 0, 2*(i_yx(s->i)&0x0f), jit_reg_new(s->j), JIT_16BIT);
    MOVE_RP_R(i[2], i[0]->out.reg, JIT_REG_INVALID, 0, 2*(i_yx(s->i)>>4), jit_reg_new(s->j), JIT_16BIT);
    MOVE_ID_R(i[3], &s->m, jit_reg_new(s->j), JIT_32BIT);
    MOVE_R_RP(i[4], i[1]->out.reg, i[3]->out.reg, i[2]->out.reg, 1, 0, JIT_16BIT);
}

void cpu_emit_addi(cpu_state *s)
{
    int n;
    struct jit_instr *i[3];

    for(n = 0; n < 3; n++)
        i[n] = jit_instr_new(s->j);
    
    // state.r[i.x] += i.hhll
    MOVE_M_R(i[0], &s->r[i_yx(s->i)&0x0f], jit_reg_new(s->j), JIT_16BIT);
    ADD_I_R_R(i[1], i_hhll(s->i), i[0]->out.reg, i[0]->out.reg, JIT_16BIT);
    MOVE_R_M(i[2], i[0]->out.reg, &s->r[i_yx(s->i)&0x0f], JIT_16BIT);
    // state.f <- update
    cpu_emit__flags(s, FLAG_C | FLAG_Z | FLAG_O | FLAG_N);
}

void cpu_emit_add_r2(cpu_state *s)
{
    int n;
    struct jit_instr *i[4];

    for(n = 0; n < 4; n++)
        i[n] = jit_instr_new(s->j);

    // state.r[i.x] += state.r[i.y]
    MOVE_M_R(i[0], &s->r[i_yx(s->i)&0x0f], jit_reg_new(s->j), JIT_16BIT);
    MOVE_M_R(i[1], &s->r[i_yx(s->i)>>4], jit_reg_new(s->j), JIT_16BIT);
    ADD_R_R_R(i[2], i[1]->out.reg, i[0]->out.reg, i[0]->out.reg, JIT_32BIT);
    MOVE_R_M(i[3], i[0]->out.reg, &s->r[i_yx(s->i)&0x0f], JIT_16BIT);
    // state.f <- update
    cpu_emit__flags(s, FLAG_C | FLAG_Z | FLAG_O | FLAG_N);
}

void cpu_emit_add_r3(cpu_state *s)
{
    int n;
    struct jit_instr *i[4];

    for(n = 0; n < 4; n++)
        i[n] = jit_instr_new(s->j);

    // state.r[i.x] += state.r[i.y]
    MOVE_M_R(i[0], &s->r[i_yx(s->i)&0x0f], jit_reg_new(s->j), JIT_16BIT);
    MOVE_M_R(i[1], &s->r[i_yx(s->i)>>4], jit_reg_new(s->j), JIT_16BIT);
    ADD_R_R_R(i[2], i[1]->out.reg, i[0]->out.reg, i[0]->out.reg, JIT_32BIT);
    MOVE_R_M(i[3], i[0]->out.reg, &s->r[i_z(s->i)&0x0f], JIT_16BIT);
    // state.f <- update
    cpu_emit__flags(s, FLAG_C | FLAG_Z | FLAG_O | FLAG_N);
}

void cpu_emit_subi(cpu_state *s)
{
    int n;
    struct jit_instr *i[3];

    for(n = 0; n < 3; n++)
        i[n] = jit_instr_new(s->j);
    
    // state.r[i.x] += i.hhll
    MOVE_M_R(i[0], &s->r[i_yx(s->i)&0x0f], jit_reg_new(s->j), JIT_16BIT);
    SUB_I_R_R(i[1], i_hhll(s->i), i[0]->out.reg, i[0]->out.reg, JIT_16BIT);
    MOVE_R_M(i[2], i[0]->out.reg, &s->r[i_yx(s->i)&0x0f], JIT_16BIT);
    // state.f <- update
    cpu_emit__flags(s, FLAG_C | FLAG_Z | FLAG_O | FLAG_N);
}

void cpu_emit_sub_r2(cpu_state *s)
{
    int n;
    struct jit_instr *i[4];

    for(n = 0; n < 4; n++)
        i[n] = jit_instr_new(s->j);

    // state.r[i.x] += state.r[i.y]
    MOVE_M_R(i[0], &s->r[i_yx(s->i)&0x0f], jit_reg_new(s->j), JIT_16BIT);
    MOVE_M_R(i[1], &s->r[i_yx(s->i)>>4], jit_reg_new(s->j), JIT_16BIT);
    SUB_R_R_R(i[2], i[1]->out.reg, i[0]->out.reg, i[0]->out.reg, JIT_32BIT);
    MOVE_R_M(i[3], i[0]->out.reg, &s->r[i_yx(s->i)&0x0f], JIT_16BIT);
    // state.f <- update
    cpu_emit__flags(s, FLAG_C | FLAG_Z | FLAG_O | FLAG_N);
}

void cpu_emit_sub_r3(cpu_state *s)
{
    int n;
    struct jit_instr *i[4];

    for(n = 0; n < 4; n++)
        i[n] = jit_instr_new(s->j);

    // state.r[i.x] += state.r[i.y]
    MOVE_M_R(i[0], &s->r[i_yx(s->i)&0x0f], jit_reg_new(s->j), JIT_16BIT);
    MOVE_M_R(i[1], &s->r[i_yx(s->i)>>4], jit_reg_new(s->j), JIT_16BIT);
    SUB_R_R_R(i[2], i[1]->out.reg, i[0]->out.reg, i[0]->out.reg, JIT_32BIT);
    MOVE_R_M(i[3], i[0]->out.reg, &s->r[i_z(s->i)&0x0f], JIT_16BIT);
    // state.f <- update
    cpu_emit__flags(s, FLAG_C | FLAG_Z | FLAG_O | FLAG_N);
}

void cpu_emit_cmpi(cpu_state *s)
{
    int n;
    struct jit_instr *i[2];

    for(n = 0; n < 2; n++)
        i[n] = jit_instr_new(s->j);
    
    // state.r[i.x] += i.hhll
    MOVE_M_R(i[0], &s->r[i_yx(s->i)&0x0f], jit_reg_new(s->j), JIT_16BIT);
    SUB_I_R_R(i[1], i_hhll(s->i), i[0]->out.reg, i[0]->out.reg, JIT_16BIT);
    // state.f <- update
    cpu_emit__flags(s, FLAG_C | FLAG_Z | FLAG_O | FLAG_N);
}

void cpu_emit_cmp(cpu_state *s)
{
    int n;
    struct jit_instr *i[3];

    for(n = 0; n < 3; n++)
        i[n] = jit_instr_new(s->j);

    // state.r[i.x] += state.r[i.y]
    MOVE_M_R(i[0], &s->r[i_yx(s->i)&0x0f], jit_reg_new(s->j), JIT_16BIT);
    MOVE_M_R(i[1], &s->r[i_yx(s->i)>>4], jit_reg_new(s->j), JIT_16BIT);
    SUB_R_R_R(i[2], i[1]->out.reg, i[0]->out.reg, i[0]->out.reg, JIT_32BIT);
    // state.f <- update
    cpu_emit__flags(s, FLAG_C | FLAG_Z | FLAG_O | FLAG_N);
}

void cpu_emit_andi(cpu_state *s)
{
    int n;
    struct jit_instr *i[3];

    for(n = 0; n < 3; n++)
        i[n] = jit_instr_new(s->j);
    
    // state.r[i.x] += i.hhll
    MOVE_M_R(i[0], &s->r[i_yx(s->i)&0x0f], jit_reg_new(s->j), JIT_16BIT);
    AND_I_R_R(i[1], i_hhll(s->i), i[0]->out.reg, i[0]->out.reg, JIT_16BIT);
    MOVE_R_M(i[2], i[0]->out.reg, &s->r[i_yx(s->i)&0x0f], JIT_16BIT);
    // state.f <- update
    cpu_emit__flags(s, FLAG_C | FLAG_Z | FLAG_O | FLAG_N);
}

void cpu_emit_and_r2(cpu_state *s)
{
    int n;
    struct jit_instr *i[4];

    for(n = 0; n < 4; n++)
        i[n] = jit_instr_new(s->j);

    // state.r[i.x] += state.r[i.y]
    MOVE_M_R(i[0], &s->r[i_yx(s->i)&0x0f], jit_reg_new(s->j), JIT_16BIT);
    MOVE_M_R(i[1], &s->r[i_yx(s->i)>>4], jit_reg_new(s->j), JIT_16BIT);
    AND_R_R_R(i[2], i[1]->out.reg, i[0]->out.reg, i[0]->out.reg, JIT_32BIT);
    MOVE_R_M(i[3], i[0]->out.reg, &s->r[i_yx(s->i)&0x0f], JIT_16BIT);
    // state.f <- update
    cpu_emit__flags(s, FLAG_C | FLAG_Z | FLAG_O | FLAG_N);
}

void cpu_emit_tsti(cpu_state *s)
{
    int n;
    struct jit_instr *i[2];

    for(n = 0; n < 2; n++)
        i[n] = jit_instr_new(s->j);
    
    // state.r[i.x] += i.hhll
    MOVE_M_R(i[0], &s->r[i_yx(s->i)&0x0f], jit_reg_new(s->j), JIT_16BIT);
    AND_I_R_R(i[1], i_hhll(s->i), i[0]->out.reg, i[0]->out.reg, JIT_16BIT);
    // state.f <- update
    cpu_emit__flags(s, FLAG_C | FLAG_Z | FLAG_O | FLAG_N);
}

void cpu_emit_tst(cpu_state *s)
{
    int n;
    struct jit_instr *i[3];

    for(n = 0; n < 3; n++)
        i[n] = jit_instr_new(s->j);

    // state.r[i.x] += state.r[i.y]
    MOVE_M_R(i[0], &s->r[i_yx(s->i)&0x0f], jit_reg_new(s->j), JIT_16BIT);
    MOVE_M_R(i[1], &s->r[i_yx(s->i)>>4], jit_reg_new(s->j), JIT_16BIT);
    AND_R_R_R(i[2], i[1]->out.reg, i[0]->out.reg, i[0]->out.reg, JIT_32BIT);
    // state.f <- update
    cpu_emit__flags(s, FLAG_C | FLAG_Z | FLAG_O | FLAG_N);
}

void cpu_emit_and_r3(cpu_state *s)
{
    int n;
    struct jit_instr *i[4];

    for(n = 0; n < 4; n++)
        i[n] = jit_instr_new(s->j);

    // state.r[i.x] += state.r[i.y]
    MOVE_M_R(i[0], &s->r[i_yx(s->i)&0x0f], jit_reg_new(s->j), JIT_16BIT);
    MOVE_M_R(i[1], &s->r[i_yx(s->i)>>4], jit_reg_new(s->j), JIT_16BIT);
    AND_R_R_R(i[2], i[1]->out.reg, i[0]->out.reg, i[0]->out.reg, JIT_32BIT);
    MOVE_R_M(i[3], i[0]->out.reg, &s->r[i_z(s->i)&0x0f], JIT_16BIT);
    // state.f <- update
    cpu_emit__flags(s, FLAG_C | FLAG_Z | FLAG_O | FLAG_N);
}

void cpu_emit_ori(cpu_state *s)
{
    int n;
    struct jit_instr *i[3];

    for(n = 0; n < 3; n++)
        i[n] = jit_instr_new(s->j);
    
    // state.r[i.x] += i.hhll
    MOVE_M_R(i[0], &s->r[i_yx(s->i)&0x0f], jit_reg_new(s->j), JIT_16BIT);
    OR_I_R_R(i[1], i_hhll(s->i), i[0]->out.reg, i[0]->out.reg, JIT_16BIT);
    MOVE_R_M(i[2], i[0]->out.reg, &s->r[i_yx(s->i)&0x0f], JIT_16BIT);
    // state.f <- update
    cpu_emit__flags(s, FLAG_C | FLAG_Z | FLAG_O | FLAG_N);
}

void cpu_emit_or_r2(cpu_state *s)
{
    int n;
    struct jit_instr *i[4];

    for(n = 0; n < 4; n++)
        i[n] = jit_instr_new(s->j);

    // state.r[i.x] += state.r[i.y]
    MOVE_M_R(i[0], &s->r[i_yx(s->i)&0x0f], jit_reg_new(s->j), JIT_16BIT);
    MOVE_M_R(i[1], &s->r[i_yx(s->i)>>4], jit_reg_new(s->j), JIT_16BIT);
    OR_R_R_R(i[2], i[1]->out.reg, i[0]->out.reg, i[0]->out.reg, JIT_32BIT);
    MOVE_R_M(i[3], i[0]->out.reg, &s->r[i_yx(s->i)&0x0f], JIT_16BIT);
    // state.f <- update
    cpu_emit__flags(s, FLAG_C | FLAG_Z | FLAG_O | FLAG_N);
}

void cpu_emit_or_r3(cpu_state *s)
{
    int n;
    struct jit_instr *i[4];

    for(n = 0; n < 4; n++)
        i[n] = jit_instr_new(s->j);

    // state.r[i.x] += state.r[i.y]
    MOVE_M_R(i[0], &s->r[i_yx(s->i)&0x0f], jit_reg_new(s->j), JIT_16BIT);
    MOVE_M_R(i[1], &s->r[i_yx(s->i)>>4], jit_reg_new(s->j), JIT_16BIT);
    OR_R_R_R(i[2], i[1]->out.reg, i[0]->out.reg, i[0]->out.reg, JIT_32BIT);
    MOVE_R_M(i[3], i[0]->out.reg, &s->r[i_z(s->i)&0x0f], JIT_16BIT);
    // state.f <- update
    cpu_emit__flags(s, FLAG_C | FLAG_Z | FLAG_O | FLAG_N);
}

void cpu_emit_xori(cpu_state *s)
{
    int n;
    struct jit_instr *i[3];

    for(n = 0; n < 3; n++)
        i[n] = jit_instr_new(s->j);
    
    // state.r[i.x] += i.hhll
    MOVE_M_R(i[0], &s->r[i_yx(s->i)&0x0f], jit_reg_new(s->j), JIT_16BIT);
    XOR_I_R_R(i[1], i_hhll(s->i), i[0]->out.reg, i[0]->out.reg, JIT_16BIT);
    MOVE_R_M(i[2], i[0]->out.reg, &s->r[i_yx(s->i)&0x0f], JIT_16BIT);
    // state.f <- update
    cpu_emit__flags(s, FLAG_C | FLAG_Z | FLAG_O | FLAG_N);
}

void cpu_emit_xor_r2(cpu_state *s)
{
    int n;
    struct jit_instr *i[4];

    for(n = 0; n < 4; n++)
        i[n] = jit_instr_new(s->j);

    // state.r[i.x] += state.r[i.y]
    MOVE_M_R(i[0], &s->r[i_yx(s->i)&0x0f], jit_reg_new(s->j), JIT_16BIT);
    MOVE_M_R(i[1], &s->r[i_yx(s->i)>>4], jit_reg_new(s->j), JIT_16BIT);
    XOR_R_R_R(i[2], i[1]->out.reg, i[0]->out.reg, i[0]->out.reg, JIT_32BIT);
    MOVE_R_M(i[3], i[0]->out.reg, &s->r[i_yx(s->i)&0x0f], JIT_16BIT);
    // state.f <- update
    cpu_emit__flags(s, FLAG_C | FLAG_Z | FLAG_O | FLAG_N);
}

void cpu_emit_xor_r3(cpu_state *s)
{
    int n;
    struct jit_instr *i[4];

    for(n = 0; n < 4; n++)
        i[n] = jit_instr_new(s->j);

    // state.r[i.x] += state.r[i.y]
    MOVE_M_R(i[0], &s->r[i_yx(s->i)&0x0f], jit_reg_new(s->j), JIT_16BIT);
    MOVE_M_R(i[1], &s->r[i_yx(s->i)>>4], jit_reg_new(s->j), JIT_16BIT);
    XOR_R_R_R(i[2], i[1]->out.reg, i[0]->out.reg, i[0]->out.reg, JIT_32BIT);
    MOVE_R_M(i[3], i[0]->out.reg, &s->r[i_z(s->i)&0x0f], JIT_16BIT);
    // state.f <- update
    cpu_emit__flags(s, FLAG_C | FLAG_Z | FLAG_O | FLAG_N);
}

void cpu_emit_muli(cpu_state *s)
{
    int n;
    struct jit_instr *i[4];
    int16_t *p_reg = &s->r[i_yx(s->i)&0x0f];

    for(n = 0; n < 4; n++)
        i[n] = jit_instr_new(s->j);
    
    MOVE_M_R(i[0], p_reg, jit_reg_new_fixed(s->j, JIT_REGMAP_MULTIPLICAND), JIT_16BIT);
    MOVE_I_R(i[1], i_hhll(s->i), jit_reg_new(s->j), JIT_16BIT);
    MUL_R_R_R(i[2], i[0]->out.reg, i[1]->out.reg, i[0]->out.reg, JIT_32BIT);
    MOVE_R_M(i[3], i[0]->out.reg, p_reg, JIT_16BIT);
    cpu_emit__flags(s, FLAG_C | FLAG_Z | FLAG_N);
}

void cpu_emit_mul_r2(cpu_state *s)
{
    int n;
    struct jit_instr *i[4];
    int16_t *p_reg = &s->r[i_yx(s->i)&0x0f];
    int16_t *p_regarg = &s->r[i_yx(s->i)>>4];

    for(n = 0; n < 4; n++)
        i[n] = jit_instr_new(s->j);
    
    MOVE_M_R(i[0], p_reg, jit_reg_new_fixed(s->j, JIT_REGMAP_MULTIPLICAND), JIT_16BIT);
    MOVE_M_R(i[1], p_regarg, jit_reg_new(s->j), JIT_16BIT);
    MUL_R_R_R(i[2], i[0]->out.reg, i[1]->out.reg, i[0]->out.reg, JIT_32BIT);
    MOVE_R_M(i[3], i[0]->out.reg, p_reg, JIT_16BIT);
    cpu_emit__flags(s, FLAG_C | FLAG_Z | FLAG_N);
}

void cpu_emit_mul_r3(cpu_state *s)
{
    int n;
    struct jit_instr *i[4];
    int16_t *p_reg = &s->r[i_yx(s->i)&0x0f];
    int16_t *p_regarg = &s->r[i_yx(s->i)>>4];
    int16_t *p_regout = &s->r[i_z(s->i)&0x0f];

    for(n = 0; n < 4; n++)
        i[n] = jit_instr_new(s->j);
    
    MOVE_M_R(i[0], p_reg, jit_reg_new_fixed(s->j, JIT_REGMAP_MULTIPLICAND), JIT_16BIT);
    MOVE_M_R(i[1], p_regarg, jit_reg_new(s->j), JIT_16BIT);
    MUL_R_R_R(i[2], i[0]->out.reg, i[1]->out.reg, i[0]->out.reg, JIT_32BIT);
    MOVE_R_M(i[3], i[0]->out.reg, p_regout, JIT_16BIT);
    cpu_emit__flags(s, FLAG_C | FLAG_Z | FLAG_N);
}

void cpu_emit_divi(cpu_state *s)
{
    int n;
    struct jit_instr *i[4];
    int16_t *p_reg = &s->r[i_yx(s->i)&0x0f];

    for(n = 0; n < 4; n++)
        i[n] = jit_instr_new(s->j);
    
    MOVE_M_R(i[0], p_reg, jit_reg_new_fixed(s->j, JIT_REGMAP_DIVIDEND), JIT_16BIT);
    MOVE_I_R(i[1], i_hhll(s->i), jit_reg_new(s->j), JIT_16BIT);
    DIV_R_R_R(i[2], i[0]->out.reg, i[1]->out.reg, i[0]->out.reg, JIT_32BIT);
    MOVE_R_M(i[3], i[0]->out.reg, p_reg, JIT_16BIT);
    cpu_emit__flags(s, FLAG_C | FLAG_Z | FLAG_N);
}

void cpu_emit_div_r2(cpu_state *s)
{
    int n;
    struct jit_instr *i[4];
    int16_t *p_reg = &s->r[i_yx(s->i)&0x0f];
    int16_t *p_regarg = &s->r[i_yx(s->i)>>4];

    for(n = 0; n < 4; n++)
        i[n] = jit_instr_new(s->j);
    
    MOVE_M_R(i[0], p_reg, jit_reg_new_fixed(s->j, JIT_REGMAP_DIVIDEND), JIT_16BIT);
    MOVE_M_R(i[1], p_regarg, jit_reg_new(s->j), JIT_16BIT);
    DIV_R_R_R(i[2], i[0]->out.reg, i[1]->out.reg, i[0]->out.reg, JIT_32BIT);
    MOVE_R_M(i[3], i[0]->out.reg, p_reg, JIT_16BIT);
    cpu_emit__flags(s, FLAG_C | FLAG_Z | FLAG_N);
}

void cpu_emit_div_r3(cpu_state *s)
{
    int n;
    struct jit_instr *i[4];
    int16_t *p_reg = &s->r[i_yx(s->i)&0x0f];
    int16_t *p_regarg = &s->r[i_yx(s->i)>>4];
    int16_t *p_regout = &s->r[i_z(s->i)&0x0f];

    for(n = 0; n < 4; n++)
        i[n] = jit_instr_new(s->j);
    
    MOVE_M_R(i[0], p_reg, jit_reg_new_fixed(s->j, JIT_REGMAP_DIVIDEND), JIT_16BIT);
    MOVE_M_R(i[1], p_regarg, jit_reg_new(s->j), JIT_16BIT);
    DIV_R_R_R(i[2], i[0]->out.reg, i[1]->out.reg, i[0]->out.reg, JIT_32BIT);
    MOVE_R_M(i[3], i[0]->out.reg, p_regout, JIT_16BIT);
    cpu_emit__flags(s, FLAG_C | FLAG_Z | FLAG_N);
}

void cpu_emit_shl_n(cpu_state *s)
{
    int n;
    struct jit_instr *i[4];
    int16_t *p_reg = &s->r[i_yx(s->i)&0x0f];

    for(n = 0; n < 4; n++)
        i[n] = jit_instr_new(s->j);
    
    MOVE_M_R(i[0], p_reg, jit_reg_new(s->j), JIT_16BIT);
    SHL_R_I_R(i[1], i[0]->out.reg, i_z(s->i)&0x0f, i[0]->out.reg, JIT_32BIT);
    MOVE_R_M(i[2], i[0]->out.reg, p_reg, JIT_16BIT);
    cpu_emit__flags(s, FLAG_Z | FLAG_N);
}

void cpu_emit_shr_n(cpu_state *s)
{
    int n;
    struct jit_instr *i[4];
    int16_t *p_reg = &s->r[i_yx(s->i)&0x0f];

    for(n = 0; n < 4; n++)
        i[n] = jit_instr_new(s->j);
    
    MOVE_M_R(i[0], p_reg, jit_reg_new(s->j), JIT_16BIT);
    SHR_R_I_R(i[1], i[0]->out.reg, i_z(s->i)&0x0f, i[0]->out.reg, JIT_32BIT);
    MOVE_R_M(i[2], i[0]->out.reg, p_reg, JIT_16BIT);
    cpu_emit__flags(s, FLAG_Z | FLAG_N);
}

void cpu_emit_sar_n(cpu_state *s)
{
    int n;
    struct jit_instr *i[4];
    int16_t *p_reg = &s->r[i_yx(s->i)&0x0f];

    for(n = 0; n < 4; n++)
        i[n] = jit_instr_new(s->j);
    
    MOVE_M_R(i[0], p_reg, jit_reg_new(s->j), JIT_16BIT);
    SAR_R_I_R(i[1], i[0]->out.reg, i_z(s->i)&0x0f, i[0]->out.reg, JIT_32BIT);
    MOVE_R_M(i[2], i[0]->out.reg, p_reg, JIT_16BIT);
    cpu_emit__flags(s, FLAG_Z | FLAG_N);
}

void cpu_emit_shl_r(cpu_state *s)
{
    int n;
    struct jit_instr *i[4];
    int16_t *p_reg = &s->r[i_yx(s->i)&0x0f];
    int16_t *p_regarg = &s->r[i_yx(s->i)>>4];

    for(n = 0; n < 4; n++)
        i[n] = jit_instr_new(s->j);
    
    MOVE_M_R(i[0], p_reg, jit_reg_new(s->j), JIT_16BIT);
    MOVE_M_R(i[1], p_regarg, jit_reg_new(s->j), JIT_16BIT);
    SHL_R_R_R(i[1], i[0]->out.reg, i[1]->out.reg, i[0]->out.reg, JIT_32BIT);
    MOVE_R_M(i[2], i[0]->out.reg, p_reg, JIT_16BIT);
    cpu_emit__flags(s, FLAG_Z | FLAG_N);
}

void cpu_emit_shr_r(cpu_state *s)
{
    int n;
    struct jit_instr *i[4];
    int16_t *p_reg = &s->r[i_yx(s->i)&0x0f];
    int16_t *p_regarg = &s->r[i_yx(s->i)>>4];

    for(n = 0; n < 4; n++)
        i[n] = jit_instr_new(s->j);
    
    MOVE_M_R(i[0], p_reg, jit_reg_new(s->j), JIT_16BIT);
    MOVE_M_R(i[1], p_regarg, jit_reg_new(s->j), JIT_16BIT);
    SHR_R_R_R(i[1], i[0]->out.reg, i[1]->out.reg, i[0]->out.reg, JIT_32BIT);
    MOVE_R_M(i[2], i[0]->out.reg, p_reg, JIT_16BIT);
    cpu_emit__flags(s, FLAG_Z | FLAG_N);
}

void cpu_emit_sar_r(cpu_state *s)
{
    int n;
    struct jit_instr *i[4];
    int16_t *p_reg = &s->r[i_yx(s->i)&0x0f];
    int16_t *p_regarg = &s->r[i_yx(s->i)>>4];

    for(n = 0; n < 4; n++)
        i[n] = jit_instr_new(s->j);
    
    MOVE_M_R(i[0], p_reg, jit_reg_new(s->j), JIT_16BIT);
    MOVE_M_R(i[1], p_regarg, jit_reg_new(s->j), JIT_16BIT);
    SAR_R_R_R(i[1], i[0]->out.reg, i[1]->out.reg, i[0]->out.reg, JIT_32BIT);
    MOVE_R_M(i[2], i[0]->out.reg, p_reg, JIT_16BIT);
    cpu_emit__flags(s, FLAG_Z | FLAG_N);
}

void cpu_emit_push(cpu_state *s)
{
    int n;
    struct jit_instr *i[6];
    int16_t *p_reg = &s->r[i_yx(s->i)&0x0f];

    for(n = 0; n < 6; n++)
        i[n] = jit_instr_new(s->j);

    // state.m[state.sp] = state.r[i.x]
    MOVE_ID_R(i[0], &s->m, jit_reg_new(s->j), JIT_32BIT);
    MOVE_M_R(i[1], &s->sp, jit_reg_new(s->j), JIT_16BIT);
    MOVE_M_R(i[2], p_reg, jit_reg_new(s->j), JIT_16BIT);
    MOVE_RP_R(i[3], i[2]->out.reg, i[0]->out.reg, i[1]->out.reg, 1, 0, JIT_16BIT);
    // state.sp += 2
    ADD_I_R_R(i[4], 2, i[1]->out.reg, i[1]->out.reg, JIT_32BIT);
    MOVE_R_M(i[5], i[1]->out.reg, &s->sp, JIT_16BIT);
}

void cpu_emit_pop(cpu_state *s)
{
    int n;
    struct jit_instr *i[6];
    int16_t *p_reg = &s->r[i_yx(s->i)&0x0f];

    for(n = 0; n < 6; n++)
        i[n] = jit_instr_new(s->j);

    // state.sp -= 2
    MOVE_M_R(i[0], &s->sp, jit_reg_new(s->j), JIT_16BIT);
    SUB_I_R_R(i[1], 2, i[0]->out.reg, i[0]->out.reg, JIT_32BIT);
    MOVE_R_M(i[2], i[0]->out.reg, &s->sp, JIT_16BIT);
    // state.r[i.x] = state.m[state.sp]
    MOVE_ID_R(i[3], &s->m, jit_reg_new(s->j), JIT_32BIT);
    MOVE_RP_R(i[4], i[0]->out.reg, i[3]->out.reg, jit_reg_new(s->j), 1, 0, JIT_16BIT);
    MOVE_R_M(i[5], i[4]->out.reg, p_reg, JIT_16BIT);
}

void cpu_emit_pushf(cpu_state *s)
{
    int n;
    struct jit_instr *i[6];
    uint16_t *f = malloc(sizeof(uint16_t));
    *f =(s->f.c << 1) | (s->f.z << 2) | (s->f.o << 6) | (s->f.n << 7);

    for(n = 0; n < 6; n++)
        i[n] = jit_instr_new(s->j);

    // state.m[state.sp] = state.f
    MOVE_ID_R(i[0], &s->m, jit_reg_new(s->j), JIT_32BIT);
    MOVE_M_R(i[1], &s->sp, jit_reg_new(s->j), JIT_16BIT);
    MOVE_M_R(i[2], f, jit_reg_new(s->j), JIT_16BIT);
    MOVE_RP_R(i[3], i[2]->out.reg, i[0]->out.reg, i[1]->out.reg, 1, 0, JIT_16BIT);
    // state.sp += 2
    ADD_I_R_R(i[4], 2, i[1]->out.reg, i[1]->out.reg, JIT_32BIT);
    MOVE_R_M(i[5], i[1]->out.reg, &s->sp, JIT_16BIT);

    free(f);
}

void cpu_emit_popf(cpu_state *s)
{
    // Fuck this
}

void cpu_emit_pushall(cpu_state *s)
{
    // Fuck this
}

void cpu_emit_popall(cpu_state *s)
{
    // Fuck this
}

void cpu_emit_pal_i(cpu_state *s)
{
    // Fuck this
}

void cpu_emit_pal_r(cpu_state *s)
{
    // Fuck this
}

void cpu_emit_noti(cpu_state *s)
{
    // Fuck this
}

void cpu_emit_not_r1(cpu_state *s)
{
    // Fuck this
}

void cpu_emit_not_r2(cpu_state *s)
{
    // Fuck this
}

void cpu_emit_negi(cpu_state *s)
{
    // Fuck this
}

void cpu_emit_neg_r1(cpu_state *s)
{
    // Fuck this
}

void cpu_emit_neg_r2(cpu_state *s)
{
    // Fuck this
}

