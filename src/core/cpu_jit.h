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

#ifndef CPU_JIT_H
#define CPU_JIT_H

static void** gp_blockmap[0x10000 >> 2];

#define PAGESZ 4096
#define BYTES_TO_PAGESZ(b) (((b)+4095) & ~4095)

typedef void (*pfn_jit_op)(cpu_state *);

void cpu_jit_init(cpu_state *);
void cpu_jit_destroy(cpu_state *);
void* cpu_jit_get_block(cpu_state *, uint16_t);
void* cpu_jit_compile_block(cpu_state *, uint16_t);
void cpu_jit_run(cpu_state *);
void cpu_jit_emit_cyclecount(cpu_state *);

void cpu_emit__error(cpu_state *state);
/* 0x */
void cpu_emit_nop(cpu_state *state);
void cpu_emit_cls(cpu_state *state);
void cpu_emit_vblnk(cpu_state *state);
void cpu_emit_bgc(cpu_state *state);
void cpu_emit_spr(cpu_state *state);
void cpu_emit_drw_i(cpu_state *state);
void cpu_emit_drw_r(cpu_state *state);
void cpu_emit_rnd(cpu_state *state);
void cpu_emit_flip(cpu_state *state);
void cpu_emit_snd0(cpu_state *state);
void cpu_emit_snd1(cpu_state *state);
void cpu_emit_snd2(cpu_state *state);
void cpu_emit_snd3(cpu_state *state);
void cpu_emit_snp(cpu_state *state);
void cpu_emit_sng(cpu_state *state);
/* 1x */
void cpu_emit_jmp_i(cpu_state *state);
void cpu_emit_jmc(cpu_state *state);
void cpu_emit_jx(cpu_state *state);
void cpu_emit_jme(cpu_state *state);
void cpu_emit_call_i(cpu_state *state);
void cpu_emit_ret(cpu_state *state);
void cpu_emit_jmp_r(cpu_state *state);
void cpu_emit_cx(cpu_state *state);
void cpu_emit_call_r(cpu_state *state);
/* 2x */
void cpu_emit_ldi(cpu_state *state);
void cpu_emit_ldi_sp(cpu_state *state);
void cpu_emit_ldm_i(cpu_state *state);
void cpu_emit_ldm_r(cpu_state *state);
void cpu_emit_mov(cpu_state *state);
/* 3x */
void cpu_emit_stm_i(cpu_state *state);
void cpu_emit_stm_r(cpu_state *state);
/* 4x */
void cpu_emit_addi(cpu_state *state);
void cpu_emit_add_r2(cpu_state *state);
void cpu_emit_add_r3(cpu_state *state);
/* 5x */
void cpu_emit_subi(cpu_state *state);
void cpu_emit_sub_r2(cpu_state *state);
void cpu_emit_sub_r3(cpu_state *state);
void cpu_emit_cmpi(cpu_state *state);
void cpu_emit_cmp(cpu_state *state);
/* 6x */
void cpu_emit_andi(cpu_state *state);
void cpu_emit_and_r2(cpu_state *state);
void cpu_emit_and_r3(cpu_state *state);
void cpu_emit_tsti(cpu_state *state);
void cpu_emit_tst(cpu_state *state);
/* 7x */
void cpu_emit_ori(cpu_state *state);
void cpu_emit_or_r2(cpu_state *state);
void cpu_emit_or_r3(cpu_state *state);
/* 8x */
void cpu_emit_xori(cpu_state *state);
void cpu_emit_xor_r2(cpu_state *state);
void cpu_emit_xor_r3(cpu_state *state);
/* 9x */
void cpu_emit_muli(cpu_state *state);
void cpu_emit_mul_r2(cpu_state *state);
void cpu_emit_mul_r3(cpu_state *state);
/* Ax */
void cpu_emit_divi(cpu_state *state);
void cpu_emit_div_r2(cpu_state *state);
void cpu_emit_div_r3(cpu_state *state);
void cpu_emit_modi(cpu_state *state);
void cpu_emit_mod_r2(cpu_state *state);
void cpu_emit_mod_r3(cpu_state *state);
void cpu_emit_remi(cpu_state *state);
void cpu_emit_rem_r2(cpu_state *state);
void cpu_emit_rem_r3(cpu_state *state);
/* Bx */
void cpu_emit_shl_n(cpu_state *state);
void cpu_emit_shr_n(cpu_state *state);
void cpu_emit_sar_n(cpu_state *state);
void cpu_emit_shl_r(cpu_state *state);
void cpu_emit_shr_r(cpu_state *state);
void cpu_emit_sar_r(cpu_state *state);
/* Cx */
void cpu_emit_push(cpu_state *state);
void cpu_emit_pop(cpu_state *state);
void cpu_emit_pushall(cpu_state *state);
void cpu_emit_popall(cpu_state *state);
void cpu_emit_pushf(cpu_state *state);
void cpu_emit_popf(cpu_state *state);
/* Dx */
void cpu_emit_pal_i(cpu_state *state);
void cpu_emit_pal_r(cpu_state *state);
/* Ex */
void cpu_emit_noti(cpu_state *state);
void cpu_emit_not_r1(cpu_state *state);
void cpu_emit_not_r2(cpu_state *state);
void cpu_emit_negi(cpu_state *state);
void cpu_emit_neg_r1(cpu_state *state);
void cpu_emit_neg_r2(cpu_state *state);

#endif

