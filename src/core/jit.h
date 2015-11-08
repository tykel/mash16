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

#include <stdint.h>

#define CACHE_BUFSZ (64*1024)
#define PAGESZ  (4*1024)

typedef int (*recblk_fn)(void);

typedef struct jit_cpu_state
{
    int pc;
    int wait_vblnk;
    int sw, sh;
    int bgc;
} jit_cpu_state;

typedef enum x64_regs
{
    rax = 0,
    rcx, rdx, rbx, rsp, rbp, rsi, rdi, r8, r9, r10, r11, r12, r13, r14, r15
} x64_regs;

typedef enum x64_rm
{
    sib = 4,
    disp32
} x64_rm;

uint8_t REX(uint8_t w, uint8_t r, uint8_t x, uint8_t b)
{
    return (0xc0 | (!!w << 3) | (!!r << 2) | (!!x << 1) | !!b);
}

uint8_t REX_W() { return REX(1, 0, 0, 0); }
uint8_t REX_WB() { return REX(1, 0, 0, 1); }
uint8_t REX_WRB() { return REX(1, 1, 0, 1); }
uint8_t REX_RB() { return REX(1, 0, 0, 1); }
uint8_t REX_B() { return REX(0, 0, 0, 1); }

uint8_t MODRM(uint8_t mod, uint8_t reg, uint8_t rm)
{
    return ((mod & 3) << 6) | ((reg & 7) << 3) | rm;
}

void e_nop();
void e_ret();
void e_mov_r_r(uint8_t to, uint8_t from);
void e_mov_r_imm32(uint8_t to, uint32_t from);
void e_mov_m64_imm32(uint64_t to, uint32_t from);
void e_mov_r_m64(uint8_t to, uint64_t from);
void e_mov_m64_r(uint64_t to, uint8_t from);

