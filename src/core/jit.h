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

#ifndef _JIT_H_
#define _JIT_H_

#include <stdint.h>

#define CACHE_BUFSZ (64*1024)
#define PAGESZ  (4*1024)

typedef int (*recblk_fn)(void);

typedef struct jit_insn
{
    uint8_t op;
    int has_regs;
    uint8_t r_to;
    int num_r_to;
    int color_to;
    uint8_t r_from[2];
    int num_r_from;
    int color_from[2];
} jit_insn;

typedef struct jit_var
{
    int in_reg;
    int r;
    uint16_t *p;
    int age;
} jit_var;

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

#define REX(w, r, x, b) (0x40 | (!!(w) << 3) | (!!(r) << 2) | (!!(x) << 1) | !!(b))

#define MODRM(mod, reg, rm) ((((mod) & 3) << 6) | (((reg) & 7) << 3) | (rm))

int jit_main(int argc, char *argv[]);

void jit_regs_init();
void jit_regs_alloc(jit_insn *is, int num_insns);

void e_nop();
void e_ret();
void e_lea_r_m64(uint8_t to, uintptr_t from);
void e_mov_r_r(uint8_t to, uint8_t from);
void e_mov_r_imm8(uint8_t to, uint8_t from);
void e_mov_r_imm16(uint8_t to, uint16_t from);
void e_mov_r_imm32(uint8_t to, uint32_t from);
void e_mov_m16_imm16(uint16_t *to, uint16_t from);
void e_mov_m32_imm32(uint32_t *to, uint32_t from);
void e_mov_m64_imm64(uint64_t *to, uint64_t from);
void e_mov_r_m8(uint8_t to, uint8_t *from);
void e_mov_r_m16(uint8_t to, uint16_t *from);
void e_mov_r_m32(uint8_t to, uint32_t *from);
void e_mov_r_m64(uint8_t to, uint64_t *from);
void e_mov_m8_r(uint8_t *to, uint8_t from);
void e_mov_m16_r(uint16_t *to, uint8_t from);
void e_mov_m64_r(uint64_t *to, uint8_t from);
void e_call(uintptr_t addr);
void e_and_r_imm32(uint8_t to, uint32_t from);
void e_push(uint8_t from);
void e_pop(uint8_t to);

#endif

