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

#define PAGESZ 4096
#define BYTES_TO_PAGESZ(b) (((b)+4095) & ~4095)

#define P(s) ((s)->j->p)

#define em0(x,s) ((s) = j_emit_##x((s)))
#define em1(x,s,o1) ((s) = j_emit_##x((s),(o1)))
#define em2(x,s,o1,o2) ((s) = j_emit_##x((s),(o1),(o2)))
#define em3(x,s,o1,o2,o3) ((s) = j_emit_##x((s),(o1),(o2),(o3)))
#define em4(x,s,o1,o2,o3,o4) ((s) = j_emit_##x((s),(o1),(o2),(o3),(o4)))

typedef void (*pfn_jit_op)(cpu_state *);

void cpu_jit_init(cpu_state *);
void cpu_jit_destroy(cpu_state *);
void* cpu_jit_get_block(cpu_state *, uint16_t);
void cpu_jit_run(cpu_state *);
void cpu_jit_emit_zeroregs(cpu_state *state);
void cpu_jit_emit_cyclecount(cpu_state *state, size_t ni);
uint16_t cpu_jit_find_block_end(cpu_state *state, uint16_t start);

void translate_none(cpu_state *state);
void emit_update_flags(cpu_state *state, uint32_t flags);
/* 0x */
void translate_NOP(cpu_state *state);
void translate_CLS(cpu_state *state);
void translate_VBLNK(cpu_state *state);
void translate_BGC(cpu_state *state);
void translate_SPR(cpu_state *state);
void translate_DRW_I(cpu_state *state);
void translate_DRW_R(cpu_state *state);
void translate_RND(cpu_state *state);
void translate_FLIP(cpu_state *state);
void translate_SND0(cpu_state *state);
void translate_SND1(cpu_state *state);
void translate_SND2(cpu_state *state);
void translate_SND3(cpu_state *state);
void translate_SNP(cpu_state *state);
void translate_SNG(cpu_state *state);
/* 1x */
void translate_JMP_I(cpu_state *state);
void translate_JMC(cpu_state *state);
void translate_Jx(cpu_state *state);
void translate_JME(cpu_state *state);
void translate_CALL_I(cpu_state *state);
void translate_RET(cpu_state *state);
void translate_JMP_R(cpu_state *state);
void translate_Cx(cpu_state *state);
void translate_CALL_R(cpu_state *state);
/* 2x */
void translate_LDI(cpu_state *state);
void translate_LDI_SP(cpu_state *state);
void translate_LDM_I(cpu_state *state);
void translate_LDM_R(cpu_state *state);
void translate_MOV(cpu_state *state);
/* 3x */
void translate_STM_I(cpu_state *state);
void translate_STM_R(cpu_state *state);
/* 4x */
void translate_ADDI(cpu_state *state);
void translate_ADD_R2(cpu_state *state);
void translate_ADD_R3(cpu_state *state);
/* 5x */
void translate_SUBI(cpu_state *state);
void translate_SUB_R2(cpu_state *state);
void translate_SUB_R3(cpu_state *state);
void translate_CMPI(cpu_state *state);
void translate_CMP(cpu_state *state);
/* 6x */
void translate_ANDI(cpu_state *state);
void translate_AND_R2(cpu_state *state);
void translate_AND_R3(cpu_state *state);
void translate_TSTI(cpu_state *state);
void translate_TST(cpu_state *state);
/* 7x */
void translate_ORI(cpu_state *state);
void translate_OR_R2(cpu_state *state);
void translate_OR_R3(cpu_state *state);
/* 8x */
void translate_XORI(cpu_state *state);
void translate_XOR_R2(cpu_state *state);
void translate_XOR_R3(cpu_state *state);
/* 9x */
void translate_MULI(cpu_state *state);
void translate_MUL_R2(cpu_state *state);
void translate_MUL_R3(cpu_state *state);
/* Ax */
void translate_DIVI(cpu_state *state);
void translate_DIV_R2(cpu_state *state);
void translate_DIV_R3(cpu_state *state);
void translate_MODI(cpu_state *state);
void translate_MOD_R2(cpu_state *state);
void translate_MOD_R3(cpu_state *state);
void translate_REMI(cpu_state *state);
void translate_REM_R2(cpu_state *state);
void translate_REM_R3(cpu_state *state);
/* Bx */
void translate_SHL_N(cpu_state *state);
void translate_SHR_N(cpu_state *state);
void translate_SAR_N(cpu_state *state);
void translate_SHL_R(cpu_state *state);
void translate_SHR_R(cpu_state *state);
void translate_SAR_R(cpu_state *state);
/* Cx */
void translate_PUSH(cpu_state *state);
void translate_POP(cpu_state *state);
void translate_PUSHALL(cpu_state *state);
void translate_POPALL(cpu_state *state);
void translate_PUSHF(cpu_state *state);
void translate_POPF(cpu_state *state);
/* Dx */
void translate_PAL_I(cpu_state *state);
void translate_PAL_R(cpu_state *state);
/* Ex */
void translate_NOTI(cpu_state *state);
void translate_NOT_R1(cpu_state *state);
void translate_NOT_R2(cpu_state *state);
void translate_NEGI(cpu_state *state);
void translate_NEG_R1(cpu_state *state);
void translate_NEG_R2(cpu_state *state);

//-------------------------------------------------------------
// Wrappers around libjit emitters
//-------------------------------------------------------------

void MOV_IMM16_TO_MEM(cpu_state *s, int16_t imm, int16_t *m)
{
    s->j->p = j_emit_mov_imm16_to_m(s->j->p, imm, m);
}

void MOV_IMM8_TO_MEM(cpu_state *s, int8_t imm, int16_t *m)
{
    s->j->p = j_emit_mov_imm8_to_m(s->j->p, imm, m);
}

void MOV_REG16_TO_MEM(cpu_state *s, j_reg *r, int16_t *m)
{
    s->j->p = j_emit_mov_reg16_to_reg(s->j->p, r->i, m);
}

void MOV_REG16_TO_REG(cpu_state *s, j_reg *rfrom, j_reg *rto)
{
    s->j->p = j_emit_mov_reg16_to_reg(s->j->p, rfrom->i, rto->i);
}

void MOV_MEM16_TO_REG(cpu_state *s, void *m, j_reg *r)
{
    s->j->p = j_emit_mov_mem16_to_reg(s->j->p, m, r->i);
}

void LEA_MEM_TO_REG(cpu_state *s, void *m, j_reg *r)
{
    s->j->p = j_emit_lea_mem_to_reg(s->j->p, m, r->i);
}

void MOV_SIB16_TO_REG(cpu_state *s, int scale, j_reg *ri, j_reg *rb, j_reg *rto)
{
    s->j->p = j_emit_mov_sib16_to_reg(s->j->p, scale, ri->i, rb->i, rto->i);
}

void MOV_REGIND16_TO_REG(cpu_state *s, j_reg *rfrom, j_reg *rto)
{
    s->j->p = j_emit_mov_regind16_to_reg(s->j->p, rfrom->i, rto->i);
}

void MOV_REGIND16_DISP32_TO_REG(cpu_state *s, j_reg *rb, void *dto, j_reg *rto)
{
    s->j->p = j_emit_mov_regind16_disp32_to_reg(s->j->p, rb->i, dto, rto->i);
}

void MOV_IMM16_TO_REGIND16_DISP32(cpu_state *s, int16_t imm, j_reg *rb, void *dto)
{
    s->j->p = j_emit_mov_imm16_to_regind16_disp32(s->j->p, imm, rb->i, dto);
}

void MOV_IMM32_TO_REG(cpu_state *s, int32_t imm, j_reg *r)
{
    s->j->p = j_emit_mov_imm32_to_reg(s->j->p, imm, r->i);
}

void MOV_IMM16_TO_REG(cpu_state *s, int16_t imm, j_reg *r)
{
    s->j->p = j_emit_mov_imm16_to_reg(s->j->p, imm, r->i);
}

void MOV_IMM8_TO_REG(cpu_state *s, int8_t imm, j_reg *r)
{
    s->j->p = j_emit_mov_imm8_to_reg(s->j->p, imm, r->i);
}

void CALL_MEM(cpu_state *s, void *m)
{
    s->j->p = j_emit_call_mem(s->j->p, m);
}

void ADD_IMM32_TO_MEM(cpu_state *s, int32_t imm, void *m)
{
    s->j->p = j_emit_add_imm32_to_mem(s->j->p, imm, m);
}

void ADD_IMM16_TO_MEM(cpu_state *s, int16_t imm, void *m)
{
    s->j->p = j_emit_add_imm16_to_mem(s->j->p, imm, m);
}

void ADD_REG16_TO_REG(cpu_state *s, j_reg* rfrom, j_reg *rto)
{
    s->j->p = j_emit_add_reg16_to_mem(s->j->p, rfrom->i, rto->i);
}

void SUB_IMM16_TO_MEM(cpu_state *s, int16_t imm, void *m)
{
    s->j->p = j_emit_sub_imm16_to_mem(s->j->p, imm, m);
}

void SUB_IMM16_TO_REG(cpu_state *s, int16_t imm, j_reg *r)
{
    s->j->p = j_emit_sub_imm16_to_reg(s->j->p, imm, r->i);
}

void SUB_REG16_TO_REG(cpu_state *s, j_reg* rfrom, j_reg *rto)
{
    s->j->p = j_emit_sub_reg16_to_mem(s->j->p, rfrom->i, rto->i);
}

void CMP_IMM8_TO_MEM(cpu_state *s, int8_t imm, void *m)
{
    s->j->p = j_emit_cmp_imm8_to_mem(s->j->p, imm, m);
}

void ADD_IMM16_TO_REG(cpu_state *s, int16_t imm, j_reg *r)
{
    s->j->p = j_emit_add_imm16_to_reg(s->j->p, imm, r->i);
}

void AND_IMM32_TO_REG(cpu_state *s, int32_t imm, j_reg *r)
{
    s->j->p = j_emit_and_imm32_to_reg(s->j->p, imm, r->i);
}

void RET(cpu_state *s)
{
    s->j->p = j_emit_ret(s->j->p);
}

#endif

