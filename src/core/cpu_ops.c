/*
 *   mash16 - the chip16 emulator
 *   Copyright (C) 2012-2013 tykel
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

extern int use_verbose;

#include "../consts.h"
#include "cpu.h"
#include "gpu.h"
#include "audio.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

 /* CPU instructions. */
void op_error(cpu_state* state)
{
    fprintf(stderr,"error: unknown opcode encountered! (0x%x)\n",state->i.op);
    fprintf(stderr,"state: pc=%04x\n",state->pc);
    exit(1);
}

void op_nop(cpu_state* state)
{
    state->meta.type = OP_NONE;
}

void op_cls(cpu_state* state)
{
    memset(state->vm,0,320*240);
    state->meta.type = OP_NONE;
}

void op_vblnk(cpu_state* state)
{
    state->meta.wait_vblnk = 1;
    state->meta.type = OP_NONE;
}

void op_bgc(cpu_state* state)
{
    state->bgc = state->i.n;
    state->meta.type = OP_N;
}

void op_spr(cpu_state* state)
{
    state->sw = state->i.hhll & 0x00ff;
    state->sh = state->i.hhll >> 8;
    state->meta.type = OP_HHLL;
}

void op_drw_imm(cpu_state* state)
{
    int16_t x = state->r[state->i.yx & 0x0f];
    int16_t y = state->r[state->i.yx >> 4];
    state->f.c = op_drw(&state->m[state->i.hhll],
        state->vm, x, y, state->sw, state->sh,
        state->fx, state->fy);
    state->meta.type = OP_R_HHLL;
}

void op_drw_r(cpu_state* state)
{
    int16_t x = state->r[state->i.yx & 0x0f];
    int16_t y = state->r[state->i.yx >> 4];
    state->f.c = op_drw(&state->m[(uint16_t)state->r[state->i.z]],
        state->vm, x, y, state->sw, state->sh,
        state->fx, state->fy);
    state->meta.type = OP_R;
}

int op_drw(uint8_t* m, uint8_t* vm, int x, int y, int w, int h, int fx, int fy)
{
    int iy, ix, i, j;
    /* If nothing will be on-screen, may as well exit. */
    if(x > 319 || y > 239 || !w || !h || y+h < 0 || x+w*2 < 0)
        return 0;
    int hit = 0;
    /* Sort out what direction the sprite will be drawn in. */
    int ix_st = 0, ix_end = w*2, ix_inc = 2;
    if(fx)
    {
        ix_st = w*2 - 2;
        ix_end = -2;
        ix_inc = -2;
    }
    int iy_st = 0, iy_end = h, iy_inc = 1;
    if(fy)
    {
        iy_st = h - 1;
        iy_end = -1;
        iy_inc = -1;
    }
    /* Start drawing... */
    for(iy=iy_st, j=0; iy!=iy_end; iy+=iy_inc, ++j)
    {
        for(ix=ix_st, i=0; ix!=ix_end; ix+=ix_inc, i+=2)
        {
            /* Bounds checking for memory accesses. */
            if(i+x < 0 || i+x > 318 || j+y < 0 || j+y > 239)
                continue;
            uint8_t p  = m[w*iy + ix/2];
            uint8_t hp = p >> 4;
            uint8_t lp = p & 0x0f;
            /* Flip the pixel couple if necessary. */
            if(fx)
            {
                int t = lp;
                lp = hp;
                hp = t;
            }
            /* Draw the pixels if not transparent. */
            if(hp)
            {
                hit += vm[320*(y+j) + x+i];
                vm[320*(y+j) + x+i] = hp;
            }
            if(lp)
            {
                hit += vm[320*(y+j) + x+i+1]; 
                vm[320*(y+j) + x+i+1] = lp;
            }
        }
    }
    return (hit > 0);
}

void op_rnd(cpu_state* state)
{
    state->r[state->i.yx & 0x0f] = rand() % (state->i.hhll + 1);
    state->meta.type = OP_HHLL;
}

void op_flip(cpu_state* state)
{
    state->fx = state->i.hhll >> 9;
    state->fy = (state->i.hhll >> 8) & 0x01;
    state->meta.type = OP_N_N;
}

void op_snd0(cpu_state* state)
{
    audio_stop();
    state->meta.type = OP_NONE;
}

void op_snd1(cpu_state* state)
{
    int16_t dt = state->i.hhll;
    audio_play(500,dt,0);
    state->meta.type = OP_HHLL;
}

void op_snd2(cpu_state* state)
{
    int16_t dt = state->i.hhll;
    audio_play(1000,dt,0);
    state->meta.type = OP_HHLL;
}

void op_snd3(cpu_state* state)
{
    int16_t dt = state->i.hhll;
    audio_play(1500,dt,0);
    state->meta.type = OP_HHLL;
}

void op_snp(cpu_state* state)
{
    uint16_t f = state->m[(uint16_t)(state->r[state->i.yx & 0x0f])] |
                ((uint16_t)state->m[(uint16_t)(state->r[state->i.yx & 0x0f]) + 1] << 8);
    uint16_t dt = state->i.hhll;
    audio_play(f,dt,1);
    state->meta.type = OP_R_HHLL;
}

void op_sng(cpu_state* state)
{
    state->atk = state->i.yx >> 4;
    state->dec = state->i.yx & 0x0f;
    state->vol = (state->i.hhll >> 12) & 0x0f;
    state->type = (state->i.hhll >> 8) & 0x0f;
    state->sus = (state->i.hhll >> 4) & 0x0f;
    state->rls = state->i.hhll & 0x0f;
    audio_update(state);
    state->meta.type = OP_HHLL_HHLL;
}

void op_jmp_imm(cpu_state* state)
{
    state->pc = state->i.hhll;
    state->meta.type = OP_HHLL;
}

/* DEPRECATED -- implemented purely for compatibility. */
void op_jmc(cpu_state* state)
{
    if(state->f.c)
    {
        state->pc = state->i.hhll;
    }
    state->meta.type = OP_HHLL;
}
void op_jx(cpu_state* state)
{
    if(test_cond(state))
    {
        state->pc = state->i.hhll;
    }
    state->meta.type = OP_HHLL;
}

void op_jme(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    if(rx == ry)
    {
        state->pc = state->i.hhll;
    }
    state->meta.type = OP_R_R;
}

void op_call_imm(cpu_state* state)
{
    state->m[state->sp] = state->pc & 0x00ff;
    state->m[state->sp + 1] = state->pc >> 8;
    state->sp += 2;
    state->pc = state->i.hhll;
    state->meta.type = OP_HHLL;
}

void op_ret(cpu_state* state)
{
    state->sp -= 2;
    state->pc = state->m[state->sp] | (state->m[state->sp + 1] << 8);
    state->meta.type = OP_NONE;
}

void op_jmp_r(cpu_state* state)
{
    state->pc = (uint16_t)state->r[state->i.yx & 0x0f];
    state->meta.type = OP_R;
}

void op_cx(cpu_state* state)
{
    if(test_cond(state))
    {
        state->m[state->sp] = state->pc & 0x00ff;
        state->m[state->sp + 1] = state->pc >> 8;
        state->sp += 2;
        state->pc = state->i.hhll;
    }
    state->meta.type = OP_HHLL;
}

void op_call_r(cpu_state* state)
{
    state->m[state->sp] = state->pc & 0x00ff;
    state->m[state->sp + 1] = state->pc >> 8;
    state->sp += 2;
    state->pc = (uint16_t)state->r[state->i.yx & 0x0f];
    state->meta.type = OP_R;
}

void op_ldi_r(cpu_state* state)
{
    state->r[state->i.yx & 0x0f] = (int16_t)state->i.hhll;
    state->meta.type = OP_R_HHLL;
}

void op_ldi_sp(cpu_state* state)
{
    state->sp = state->i.hhll;
    state->meta.type = OP_SP_HHLL;
}

void op_ldm_imm(cpu_state* state)
{
    state->r[state->i.yx & 0x0f] = state->m[state->i.hhll] |
                                  (state->m[state->i.hhll + 1] << 8);
    state->meta.type = OP_R_HHLL;
}

void op_ldm_r(cpu_state* state)
{
    state->r[state->i.yx & 0x0f] = state->m[(uint16_t)state->r[state->i.yx >> 4]] |
                                  (state->m[(uint16_t)state->r[state->i.yx >> 4] + 1] << 8);
    state->meta.type = OP_R_R;
}

void op_mov(cpu_state* state)
{
    state->r[state->i.yx & 0x0f] = state->r[state->i.yx >> 4];
    state->meta.type = OP_R_R;
}

void op_stm_imm(cpu_state* state)
{
    state->m[state->i.hhll] = state->r[state->i.yx & 0x0f] & 0x00ff;
    state->m[state->i.hhll + 1] = state->r[state->i.yx & 0x0f] >> 8;
    state->meta.type = OP_R_HHLL;
}

void op_stm_r(cpu_state* state)
{
    state->m[(uint16_t)state->r[state->i.yx >> 4]] = state->r[state->i.yx & 0x0f] & 0x00ff;
    state->m[(uint16_t)state->r[state->i.yx >> 4] + 1] = state->r[state->i.yx & 0x0f] >> 8;
    state->meta.type = OP_R_R;
}

void op_addi(cpu_state* state)
{
    int16_t* r = &state->r[state->i.yx & 0x0f];
    int16_t imm = (int16_t)state->i.hhll;
    flags_add(*r,imm,state);
    *r += imm;
    state->meta.type = OP_R_HHLL;
}

void op_add_r2(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_add(*rx,ry,state);
    *rx += ry;
    state->meta.type = OP_R_R;
}

void op_add_r3(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_add(rx,ry,state);
    state->r[state->i.z] = rx + ry;
    state->meta.type = OP_R_R_R;
}

void op_subi(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t imm = state->i.hhll;
    flags_sub(*rx,imm,state);
    *rx -= imm;
    state->meta.type = OP_R_HHLL;
}

void op_sub_r2(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_sub(*rx,ry,state);
    *rx -= ry;
    state->meta.type = OP_R_R;
}

void op_sub_r3(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_sub(rx,ry,state);
    state->r[state->i.z] = rx - ry;
    state->meta.type = OP_R_R_R;
}

void op_cmpi(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t imm = state->i.hhll;
    flags_sub(rx,imm,state);
    state->meta.type = OP_R_HHLL;
 }

void op_cmp(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_sub(rx,ry,state);
    state->meta.type = OP_R_R;
}

void op_andi(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t imm = state->i.hhll;
    flags_and(*rx,imm,state);
    *rx &= imm;
    state->meta.type = OP_R_HHLL;
}

void op_and_r2(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_and(*rx,ry,state);
    *rx &= ry;
    state->meta.type = OP_R_R;
}

void op_and_r3(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_and(rx,ry,state);
    state->r[state->i.z] = rx & ry;
    state->meta.type = OP_R_R_R;
}

void op_tsti(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t imm = state->i.hhll;
    flags_and(rx,imm,state);
    state->meta.type = OP_R_HHLL;
}

void op_tst(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_and(rx,ry,state);
    state->meta.type = OP_R_R;
}

void op_ori(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t imm = state->i.hhll;
    flags_or(*rx,imm,state);
    *rx |= imm;
    state->meta.type = OP_R_HHLL;
}

void op_or_r2(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_or(*rx,ry,state);
    *rx |= ry;
    state->meta.type = OP_R_R;
}

void op_or_r3(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_or(rx,ry,state);
    state->r[state->i.z] = rx | ry;
    state->meta.type = OP_R_R_R;
}

void op_xori(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t imm = state->i.hhll;
    flags_xor(*rx,imm,state);
    *rx ^= imm;
    state->meta.type = OP_R_HHLL;
}

void op_xor_r2(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_xor(*rx,ry,state);
    *rx ^= ry;
    state->meta.type = OP_R_R;
}

void op_xor_r3(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_xor(rx,ry,state);
    state->r[state->i.z] = rx ^ ry;
    state->meta.type = OP_R_R_R;
}

void op_muli(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t imm = state->i.hhll;
    flags_mul(*rx,imm,state);
    *rx *= imm;
    state->meta.type = OP_R_HHLL;
}

void op_mul_r2(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_mul(*rx,ry,state);
    *rx *= ry;
    state->meta.type = OP_R_R;
}

void op_mul_r3(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_mul(rx,ry,state);
    state->r[state->i.z] = rx * ry;
    state->meta.type = OP_R_R_R;
}

void op_divi(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t imm = state->i.hhll;
    if(!imm)
    {
        fprintf(stderr,"error: attempted to divide by 0\n");
        fprintf(stderr,"state: pc=0x%04x\n",state->pc);
        exit(1);
    }
    flags_div(*rx,imm,state);
    *rx /= imm;
    state->meta.type = OP_R_HHLL;
}

void op_div_r2(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    if(!ry)
    {
        fprintf(stderr,"error: attempted to divide by 0\n");
        fprintf(stderr,"state: pc=0x%04x\n",state->pc);
        exit(1);
    }
    flags_div(*rx,ry,state);
    *rx /= ry;
    state->meta.type = OP_R_R;
}

void op_div_r3(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    if(!ry)
    {
        fprintf(stderr,"error: attempted to divide by 0\n");
        fprintf(stderr,"state: pc=0x%04x\n",state->pc);
        exit(1);
    }
    flags_div(rx,ry,state);
    state->r[state->i.z] = rx / ry;
    state->meta.type = OP_R_R_R;
}

void op_shl_n(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t n = state->i.n;
    flags_shl(*rx,n,state);
    *rx <<= n;
    state->meta.type = OP_R_N;
}

void op_shr_n(cpu_state* state)
{
    uint16_t* rx = (uint16_t*)&state->r[state->i.yx & 0x0f];
    int16_t n = state->i.n;
    flags_shr(*rx,n,state);
    *rx >>= n;
    state->meta.type = OP_R_N;
}

void op_sar_n(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t n = state->i.n;
    flags_sar(*rx,n,state);
    *rx >>= n;
    state->meta.type = OP_R_N;
}

void op_shl_r(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_shl(*rx,ry,state);
    *rx <<= ry;
    state->meta.type = OP_R_R;
}

void op_shr_r(cpu_state* state)
{
    uint16_t* rx = (uint16_t*)&state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_shr(*rx,ry,state);
    *rx >>= ry;
    state->meta.type = OP_R_R;
}

void op_sar_r(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_sar(*rx,ry,state);
    *rx >>= ry;
    state->meta.type = OP_R_R;
}

void op_push(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    state->m[state->sp] = rx & 0x00ff;
    state->m[state->sp + 1] = rx >> 8;
    state->sp += 2;
    state->meta.type = OP_R;
}

void op_pop(cpu_state* state)
{
    state->sp -= 2;
    state->r[state->i.yx & 0x0f] = (int16_t)(state->m[state->sp] | (state->m[state->sp + 1] << 8));
    state->meta.type = OP_R;
}

void op_pushall(cpu_state* state)
{
    int i;
    for(i=0; i<16; ++i)
    {
        state->m[state->sp] = (uint16_t)state->r[i] & 0x00ff;
        state->m[state->sp + 1] = (uint16_t)state->r[i] >> 8;
        state->sp += 2;
    }
    state->meta.type = OP_NONE;
}

void op_popall(cpu_state* state)
{
    int i;
    for(i=15; i>=0; --i)
    {
        state->sp -= 2;
        state->r[i] = (int16_t)(state->m[state->sp] | (state->m[state->sp + 1] << 8));
    }
    state->meta.type = OP_NONE;
}

void op_pushf(cpu_state* state)
{
    state->m[state->sp] = state->f.c << 1 | state->f.z << 2 |
                          state->f.o << 6 | state->f.n << 7;
    state->m[state->sp + 1] = 0;
    state->sp += 2;
    state->meta.type = OP_NONE;
}

void op_popf(cpu_state* state)
{
    state->sp -= 2;
    state->f.c = (state->m[state->sp] >> 1) & 1;
    state->f.z = (state->m[state->sp] >> 2) & 1;
    state->f.o = (state->m[state->sp] >> 6) & 1;
    state->f.n = (state->m[state->sp] >> 7) & 1;
    state->meta.type = OP_NONE;
}

void op_pal_imm(cpu_state* state)
{
    load_pal(&state->m[state->i.hhll],0,state);
    state->meta.type = OP_HHLL;
}

void op_pal_r(cpu_state* state)
{
    load_pal(&state->m[(uint16_t)state->r[state->i.yx & 0x0f]],0,state);
    state->meta.type = OP_R;
}

/* Flag computing functions. */
void flags_add(int16_t x, int16_t y, cpu_state* state)
{
    state->f = (flags){0};
    uint32_t res = (uint16_t)x + (uint16_t)y;
    if(!res)
        state->f.z = 1;
    if(res > UINT16_MAX)
        state->f.c = 1;
    if(((int16_t)res < 0 && (int16_t)x > 0 && (int16_t)y > 0) ||
        ((int16_t)res > 0 && (int16_t)x < 0 && (int16_t)y < 0))
        state->f.o = 1;
    if((int16_t)res < 0)
        state->f.n = 1;
}

void flags_sub(int16_t x, int16_t y, cpu_state* state)
{
    state->f = (flags){0};
    uint32_t res = (uint16_t)x - (uint16_t)y;
    if(!res)
        state->f.z = 1;
    if(res > UINT16_MAX)
        state->f.c = 1;
    if(((int16_t)res < 0 && (int16_t)x > 0 && (int16_t)y < 0) || 
        ((int16_t)res > 0 && (int16_t)x < 0 && y > 0))
        state->f.o = 1;
    if((int16_t)res < 0)
        state->f.n = 1;
}

void flags_and(int16_t x, int16_t y, cpu_state* state)
{
    state->f = (flags){0};
    uint16_t res = (uint16_t)x & (uint16_t)y;
    if(!res)
        state->f.z = 1;
    if((int16_t)res < 0)
        state->f.n = 1;
}

void flags_or(int16_t x, int16_t y, cpu_state* state)
{
    state->f = (flags){0};
    uint16_t res = (uint16_t)x | (uint16_t)y;
    if(!res)
        state->f.z = 1;
    if((int16_t)res < 0)
        state->f.n = 1;
}

void flags_xor(int16_t x, int16_t y, cpu_state* state)
{
    state->f = (flags){0};
    uint16_t res = (uint16_t)x ^ (uint16_t)y;
    if(!res)
        state->f.z = 1;
    if((int16_t)res < 0)
        state->f.n = 1;
}

void flags_mul(int16_t x, int16_t y, cpu_state* state)
{
    state->f = (flags){0};
    uint32_t res = (uint16_t)x * (uint16_t)y;
    if(!res)
        state->f.z = 1;
    if(res > UINT16_MAX)
        state->f.c = 1;
    if((int16_t)res < 0)
        state->f.n = 1;
}

void flags_div(int16_t x, int16_t y, cpu_state* state)
{
    state->f = (flags){0};
    uint16_t res = (uint16_t)x / (uint16_t)y;
    uint16_t rem = (uint16_t)x % (uint16_t)y;
    if(!res)
        state->f.z = 1;
    if(rem)
        state->f.c = 1;
    if((int16_t)res < 0)
        state->f.n = 1;
}

void flags_shl(int16_t x, int16_t y, cpu_state* state)
{
    state->f = (flags){0};
    uint16_t res = (uint16_t)x << y;
    if(!res)
        state->f.z = 1;
    if((int16_t)res < 0)
        state->f.n = 1;
}

void flags_shr(uint16_t x, int16_t y, cpu_state* state)
{
    state->f = (flags){0};
    uint16_t res = (uint16_t)x >> y;
    if(!res)
        state->f.z = 1;
    if((int16_t)res < 0)
        state->f.n = 1;
}

void flags_sar(int16_t x, int16_t y, cpu_state* state)
{
    state->f = (flags){0};
    int16_t res = x >> y;
    if(!res)
        state->f.z = 1;
    if(res < 0)
        state->f.n = 1;
}

/* Test a combination of flags depending on test code. */
int test_cond(cpu_state* state)
{
    switch(state->i.yx & 0x0f)
    {
        /* Z   = 0x0 // [z==1]         Equal (Zero) */
        case C_Z:
            if(state->f.z)
                return 1;
            break;
        /* NZ  = 0x1 // [z==0]         Not Equal (Non-Zero) */
        case C_NZ:
            if(!state->f.z)
                return 1;
            break;
        /* N   = 0x2 // [n==1]         Negative */
        case C_N:
            if(state->f.n)
                return 1;
            break;
        /* NN  = 0x3 // [n==0]         Not-Negative (Positive or Zero) */
        case C_NN:
            if(!state->f.n)
                return 1;
            break;
        /* P   = 0x4 // [n==0 && z==0] Positive */
        case C_P:
            if(!state->f.n && !state->f.z)
                return 1;
            break;
        /* O   = 0x5 // [o==1]         Overflow */
        case C_O:
            if(state->f.o)
                return 1;
            break;
        /* NO  = 0x6 // [o==0]         No Overflow */
        case C_NO:
            if(!state->f.o)
                return 1;
            break;
        /* A   = 0x7 // [c==0 && z==0] Above       (Unsigned Greater Than) */
        case C_A:
            if(!state->f.c && !state->f.z)
                return 1;
            break;
        /* AE  = 0x8 // [c==0]         Above Equal (Unsigned Greater Than or Equal) */
        case C_AE:
            if(!state->f.c)
                return 1;
            break;
        /* B   = 0x9 // [c==1]         Below       (Unsigned Less Than) */
        case C_B:
            if(state->f.c)
                return 1;
            break;
        /* BE  = 0xA // [c==1 || z==1] Below Equal (Unsigned Less Than or Equal) */
        case C_BE:
            if(state->f.c || state->f.z)
                return 1;
            break;
        /* G   = 0xB // [o==n && z==0] Signed Greater Than */
        case C_G:
            if((state->f.o == state->f.n) && !state->f.z)
                return 1;
            break;
        /* GE  = 0xC // [o==n]         Signed Greater Than or Equal */
        case C_GE:
            if(state->f.o == state->f.n)
                return 1;
            break;
        /* L   = 0xD // [o!=n]         Signed Less Than */
        case C_L:
            if(state->f.o != state->f.n)
                return 1;
            break;
        /* LE  = 0xE // [o!=n || z==1] Signed Less Than or Equal */
        case C_LE:
            if((state->f.o != state->f.n) || state->f.z)
                return 1;
            break;
        /* RES = 0xF // Reserved for future use */
        case C_RES:
        default:
            break;
    }
    return 0;
}
