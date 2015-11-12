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

#include <stdlib.h>

#include "jit.h"
#include "cpu.h"
//#include "audio.h"

extern cpu_state s;

void o_nop()
{
    e_nop();
}

void o_cls()
{
    e_mov_r_m64(rdi, (uint64_t)s.vm);
    e_mov_r_imm32(rsi, 0);
    e_mov_r_imm32(rdx, 320*240);
    e_call((uint64_t)memset); // memset(s.vm, 0, 320*240);
    e_mov_m64_imm32((uint64_t)&s.bgc, 0);
}

void o_vblnk()
{
    e_mov_m64_imm32((uint64_t)&s.meta.wait_vblnk, 0);
}

void o_bgc(uint32_t i)
{
    e_mov_m64_imm32((uint64_t)&s.bgc, i);
}

void o_spr(uint32_t sw, uint32_t sh)
{
    e_mov_m64_imm32((uint64_t)&s.sw, sw);
    e_mov_m64_imm32((uint64_t)&s.sh, sh);
}

void o_drw_imm(uint32_t x, uint32_t y)
{
//    state->f.c = op_drw(&state->m[i_hhll(state->i)],
//        state->vm, x, y, state->sw, state->sh,
//        state->fx, state->fy);
    e_mov_r_m64(rax, (uint64_t)(&s.i + 2*sizeof(uint8_t)));
}


void o_drw_r(uint32_t x, uint32_t y)
{
//    state->f.c = op_drw(&state->m[(uint16_t)state->r[i_z(state->i)]],
//        state->vm, x, y, state->sw, state->sh,
//        state->fx, state->fy);
}

void o_rnd(uint32_t x, uint32_t imm)
{
    e_call((uint64_t)rand);
    e_and_r_imm32(rax, imm);
    e_mov_m64_r((uint64_t)(&s.r[x]), rax);
}

void o_flip(uint32_t x, uint32_t y)
{
//    state->fx = i_hhll(state->i) >> 9;
//    state->fy = (i_hhll(state->i) >> 8) & 0x01;
    e_mov_m64_imm32((uint64_t)&s.fx, x);
    e_mov_m64_imm32((uint64_t)&s.fy, y);
}

void *audio_stop;
void *audio_play;

void o_snd0()
{
    e_call((uint64_t)audio_stop);
}

void o_snd1(uint32_t dt)
{
//    int16_t dt = i_hhll(state->i);
//    audio_play(500,dt,0);
    e_mov_r_imm32(rdi, 500);
    e_mov_r_imm32(rsi, dt);
    e_mov_r_imm32(rdx, 0);
    e_call((uint64_t)audio_play);
}

void o_snd2(uint32_t dt)
{
//    int16_t dt = i_hhll(state->i);
//    audio_play(1000,dt,0);
    e_mov_r_imm32(rdi, 1000);
    e_mov_r_imm32(rsi, dt);
    e_mov_r_imm32(rdx, 0);
    e_call((uint64_t)audio_play);
}

void o_snd3(uint32_t dt)
{
//    int16_t dt = i_hhll(state->i);
//    audio_play(1500,dt,0);
    e_mov_r_imm32(rdi, 1500);
    e_mov_r_imm32(rsi, dt);
    e_mov_r_imm32(rdx, 0);
    e_call((uint64_t)audio_play);
}
    
void o_snp(uint32_t f, uint32_t dt)
{
//   f = state->m[(uint16_t)(state->r[i_yx(state->i) & 0x0f])] |
//                ((uint16_t)state->m[(uint16_t)(state->r[i_yx(state->i) & 0x0f]) + 1] << 8);
//    dt = i_hhll(state->i);
//    audio_play(f,dt,1);
    e_mov_r_imm32(rdi, f);
    e_mov_r_imm32(rsi, dt);
    e_mov_r_imm32(rdx, 1);
    e_call((uint64_t)audio_play);
}

void o_sng(uint32_t atk, uint32_t dec, uint32_t vol, uint32_t type,
           uint32_t sus, uint32_t rls)
{
//    state->atk = i_yx(state->i) >> 4;
//    state->dec = i_yx(state->i) & 0x0f;
//    state->vol = (i_hhll(state->i) >> 12) & 0x0f;
//    state->type = (i_hhll(state->i) >> 8) & 0x0f;
//    state->sus = (i_hhll(state->i) >> 4) & 0x0f;
//    state->rls = i_hhll(state->i) & 0x0f;
//    audio_update(state);
    e_mov_m64_imm32((uint64_t)&s.atk, atk);
    e_mov_m64_imm32((uint64_t)&s.dec, dec);
    e_mov_m64_imm32((uint64_t)&s.vol, vol);
    e_mov_m64_imm32((uint64_t)&s.type, type);
    e_mov_m64_imm32((uint64_t)&s.sus, sus);
    e_mov_m64_imm32((uint64_t)&s.rls, rls);
    e_call((uint64_t)&s);
}

void o_jmp_imm(uint32_t pc)
{
//    state->pc = i_hhll(state->i);
    e_mov_m64_imm32((uint64_t)&s.pc, pc);
}
