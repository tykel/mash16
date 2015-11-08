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

#include "jit.h"
#include "cpu.h"

extern jit_cpu_state s;

void o_nop()
{
    e_nop();
}

void o_cls()
{
    // TODO: wipe 320*240 bytes to 0
}

void o_vblnk()
{
    e_mov_m64_imm32((uint64_t)&s.wait_vblnk, 0);
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
}


void o_drw_r(uint32_t x, uint32_t y)
{
//    state->f.c = op_drw(&state->m[(uint16_t)state->r[i_z(state->i)]],
//        state->vm, x, y, state->sw, state->sh,
//        state->fx, state->fy);
}

void o_rnd(uint32_t x, uint32_t imm)
{
    //o_mov_
    //o_mov_r_r( , );
}

