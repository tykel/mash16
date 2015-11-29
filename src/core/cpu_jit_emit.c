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

#include <stdbool.h>
#include <libjit.h>

#include "cpu.h"
#include "cpu_jit.h"
#include "audio.h"

int cpu_jit_op_drw(cpu_state *state, size_t moffs, int x, int y)
{
    return op_drw(&state->m[moffs], state->vm, x, y, state->sw, state->sh, state->fx, state->fy);
}

void translate_none(cpu_state *s)
{
    fprintf(stderr, "warning: unknown op encountered\n");
}

void emit_flags_update(cpu_state *s, uint32_t flags)
{
    // TODO!
}


void translate_NOP(cpu_state *s)
{
}

void translate_CLS(cpu_state *s)
{
    CALL_MEM(s, op_cls);
}

void translate_VBLNK(cpu_state *s)
{
    MOV_IMM8_TO_MEM(s, 1, &s->meta.wait_vblnk);
}

void translate_BGC(cpu_state *s)
{
    MOV_IMM8_TO_MEM(s, I_N(s->i), &s->bgc);
}

void translate_SPR(cpu_state *s)
{
    MOV_IMM8_TO_MEM(s, I_HHLL(s->i)&0xff, &s->sw);
    MOV_IMM8_TO_MEM(s, I_HHLL(s->i)>>8, &s->sh);
}

void translate_DRW_I(cpu_state *s)
{
    j_reg *rrdi = j_get_reg(s->j, RDI);
    j_reg *rrsi = j_get_reg(s->j, RDI);
    j_reg *rrdx = j_reg_containing_creg_choose(s->j, I_X(s->i), RSI);
    j_reg *rrcx = j_reg_containing_creg_choose(s->j, I_Y(s->i), RDX);
    j_reg *rrax = j_get_reg_readonly(s->j, RAX);

    LEA_MEM_TO_REG(s, s, rrdi);
    MOV_IMM16_TO_REG(s, I_HHLL(s->i), rrsi);
    CALL_MEM(s, cpu_jit_op_drw);
    MOV_REG32_TO_MEM(s, rrax, &s->f.c);
}

void translate_DRW_R(cpu_state *s)
{
    j_reg *rrdi = j_get_reg(s->j, RDI);
    j_reg *rrsi = j_reg_containing_creg_choose(s->j, I_Z(s->i), RDI);
    j_reg *rrdx = j_reg_containing_creg_choose(s->j, I_X(s->i), RSI);
    j_reg *rrcx = j_reg_containing_creg_choose(s->j, I_Y(s->i), RDX);
    j_reg *rrax = j_get_reg_readonly(s->j, RAX);

    j_reg *rbase = j_get_free_reg(s->j);

    LEA_MEM_TO_REG(s, s, rrdi);
    LEA_MEM_TO_REG(s, &s->m, rbase);
    MOV_SIB16_TO_REG(s, 0, rrsi, rbase, rrsi);
    CALL_MEM(s, cpu_jit_op_drw);
    MOV_REG32_TO_MEM(s, rrax, &s->f.c);
}

void translate_RND(cpu_state *s)
{
    j_reg *rx = j_reg_containing_creg(s->j, I_X(s->i));
    j_reg *rrax = j_get_reg_readonly(s->j, RAX);
    CALL_MEM(s, rand);
    MOV_REG16_TO_REG(s, rrax, rx);
    AND_IMM32_TO_REG(s, I_HHLL(s->i), rx);
}

void translate_FLIP(cpu_state *s)
{
    MOV_IMM8_TO_MEM(s, (I_HHLL(s->i)&2)>>1, &s->fx);
    MOV_IMM8_TO_MEM(s, (I_HHLL(s->i)&1), &s->fy);
}

void translate_SND0(cpu_state *s)
{
    CALL_MEM(s, audio_stop);
}

void translate_SND1(cpu_state *s)
{
    j_reg *rrdi = j_get_reg(s->j, RDI);
    j_reg *rrsi = j_get_reg(s->j, RSI);
    j_reg *rrdx = j_get_reg(s->j, RDX);

    MOV_IMM32_TO_REG(s, 500, rrdi);
    MOV_IMM32_TO_REG(s, I_HHLL(s->i), rrsi);
    MOV_IMM32_TO_REG(s, 0, rrdx);
    CALL_MEM(s, audio_play);
}

void translate_SND2(cpu_state *s)
{
    j_reg *rrdi = j_get_reg(s->j, RDI);
    j_reg *rrsi = j_get_reg(s->j, RSI);
    j_reg *rrdx = j_get_reg(s->j, RDX);

    MOV_IMM32_TO_REG(s, 1000, rrdi);
    MOV_IMM32_TO_REG(s, I_HHLL(s->i), rrsi);
    MOV_IMM32_TO_REG(s, 0, rrdx);
    CALL_MEM(s, audio_play);
}

void translate_SND3(cpu_state *s)
{
    j_reg *rrdi = j_get_reg(s->j, RDI);
    j_reg *rrsi = j_get_reg(s->j, RSI);
    j_reg *rrdx = j_get_reg(s->j, RDX);

    MOV_IMM32_TO_REG(s, 1500, rrdi);
    MOV_IMM32_TO_REG(s, I_HHLL(s->i), rrsi);
    MOV_IMM32_TO_REG(s, 0, rrdx);
    CALL_MEM(s, audio_play);
}

void translate_SNP(cpu_state *s)
{
    j_reg *rrdi = j_reg_containing_creg_choose(s->j, I_X(s->i), RDI);
    j_reg *rrsi = j_get_reg(s->j, RSI);
    j_reg *rrdx = j_get_reg(s->j, RDX);

    MOV_IMM32_TO_REG(s, I_HHLL(s->i), rrsi);
    MOV_IMM32_TO_REG(s, 1, rrdx);
    CALL_MEM(s, audio_play);
}

void translate_SNG(cpu_state *s)
{
    j_reg *rrdi = j_get_reg(s->j, RDI);

    em2(mov_imm8_to_mem, P(s), I_Y(s->i), &s->atk);
    em2(mov_imm8_to_mem, P(s), I_X(s->i), &s->dec);
    em2(mov_imm8_to_mem, P(s), (I_HHLL(s->i)>>12)&0x0f, &s->vol);
    em2(mov_imm8_to_mem, P(s), (I_HHLL(s->i)>>8)&0x0f, &s->type);
    em2(mov_imm8_to_mem, P(s), (I_HHLL(s->i)>>4)&0x0f, &s->sus);
    em2(mov_imm8_to_mem, P(s), I_HHLL(s->i)&0x0f, &s->rls);
    LEA_MEM_TO_REG(s, s, rrdi);
    CALL_MEM(s, audio_update);
}

void translate_JMP_I(cpu_state *s)
{
    MOV_IMM16_TO_MEM(s, I_HHLL(s->i), &s->pc);
}

void translate_JMC(cpu_state *s)
{
    j_label *l0 = NULL, *l1 = NULL;

    CMP_IMM8_TO_MEM(s, 1, &s->f.c);
    JNZ_MEML(s, l0);
    MOV_IMM16_TO_MEM(s, I_HHLL(s->i), &s->pc);
LABEL(s, l0);
}

void translate_Jx(cpu_state *s)
{
    j_label *l0 = NULL, *l1 = NULL;

    CMP_IMM8_TO_MEM(s, 1, &s->f.c);
    JNZ_MEML(s, l0);
    MOV_IMM16_TO_MEM(s, I_HHLL(s->i), &s->pc);
LABEL(s, l0);
}

void translate_JME(cpu_state *s)
{
    j_label *l0 = NULL, *l1 = NULL;

    CMP_IMM8_TO_MEM(s, 1, &s->f.z);
    JNZ_MEML(s, l0);
    MOV_IMM16_TO_MEM(s, I_HHLL(s->i), &s->pc);
LABEL(s, l0);
}

void translate_CALL_I(cpu_state *s)
{
    j_reg *rtemp = j_get_free_reg(s->j);
    j_reg *rbase = j_get_free_reg(s->j);
    MOV_MEM16_TO_REG(s, &s->pc, rtemp);
    MOV_REG_TO_REGIND16_DISP32(s, rtemp, rbase, &s->sp);
    ADD_IMM16_TO_MEM(s, 2, &s->sp);
    MOV_IMM16_TO_MEM(s, I_HHLL(s->i), &s->pc);
}

void translate_RET(cpu_state *s)
{
    j_reg *rtemp = j_get_free_reg(s->j);
    j_reg *rbase = j_get_free_reg(s->j);
    MOV_MEM16_TO_REG(s, &s->pc, rtemp);
    SUB_IMM16_TO_MEM(s, 2, &s->sp);
    MOV_REG_TO_REGIND16_DISP32(s, rtemp, rbase, &s->sp);
}

void translate_JMP_R(cpu_state *s)
{
    j_reg *rx = j_reg_containing_creg(s->j, I_X(s->i));
    if(rx->isconst) {
        MOV_IMM16_TO_MEM(s, rx->value.w, &s->pc);
    } else {
        MOV_REG16_TO_MEM(s, rx, &s->pc);
    }
}

void translate_Cx(cpu_state *s)
{
    // TODO: change this
    j_reg *rtemp = j_get_free_reg(s->j);
    j_reg *rbase = j_get_free_reg(s->j);
    MOV_MEM16_TO_REG(s, &s->pc, rtemp);
    MOV_REG_TO_REGIND16_DISP32(s, rtemp, rbase, &s->sp);
    ADD_IMM16_TO_MEM(s, 2, &s->sp);
    MOV_IMM16_TO_MEM(s, I_HHLL(s->i), &s->pc);
}

void translate_CALL_R(cpu_state *s)
{
    // TODO: change this
    j_reg *rtemp = j_get_free_reg(s->j);
    j_reg *rbase = j_get_free_reg(s->j);
    MOV_MEM16_TO_REG(s, &s->pc, rtemp);
    MOV_REG_TO_REGIND16_DISP32(s, rtemp, rbase, &s->sp);
    ADD_IMM16_TO_MEM(s, 2, &s->sp);
    MOV_IMM16_TO_MEM(s, I_HHLL(s->i), &s->pc);
}

void translate_LDI(cpu_state *s)
{
    j_reg *rx = j_reg_containing_creg(s->j, I_X(s->i));
    MOV_IMM16_TO_REG(s, I_HHLL(s->i), rx);
}

void translate_LDI_SP(cpu_state *s)
{
    MOV_IMM16_TO_MEM(s, I_HHLL(s->i), &s->sp);
}

void translate_LDM_I(cpu_state *s)
{
    j_reg *rx = j_reg_containing_creg(s->j, I_X(s->i));
    MOV_MEM16_TO_REG(s, MEMPTR16(I_HHLL(s->i)), rx);
}

void translate_LDM_R(cpu_state *s)
{
    j_reg *rto = j_reg_containing_creg(s->j, I_X(s->i));
    j_reg *rfrom = j_reg_containing_creg(s->j, I_Y(s->i));
    // We need the register to actually contain the value
    if(rfrom->isconst) {
        MOV_IMM16_TO_REG(s, rfrom->value.w, rfrom);
    }
    j_reg *rbase = j_get_free_reg(s->j);
    LEA_MEM_TO_REG(s, &s->m, rbase);
    MOV_SIB16_TO_REG(s, 0, rfrom, rbase, rto);
    rto->dirty = true;
    rto->isconst = false;
}

void translate_MOV(cpu_state *s)
{
    j_reg *rfrom = j_reg_containing_creg(s->j, I_Y(s->i));
    j_reg *rto = j_reg_containing_creg(s->j, I_X(s->i));
    if(rfrom->isconst) {
        rto->value.w = rfrom->value.w;
        rto->isconst = true;
    } else {
        MOV_REG16_TO_REG(s, rfrom, rto);
    }
    rto->dirty = true;
}

void translate_STM_I(cpu_state *s)
{
    j_reg *rfrom = j_reg_containing_creg(s->j, I_X(s->i));
    int16_t imm = I_HHLL(s->i);
    if(rfrom->isconst) {
        MOV_IMM16_TO_MEM(s, rfrom->value.w, MEMPTR16(imm));
    } else {
        MOV_REG16_TO_MEM(s, rfrom, MEMPTR16(imm));
    }
}

void translate_STM_R(cpu_state *s)
{
    j_reg *rx = j_reg_containing_creg(s->j, I_X(s->i));
    j_reg *ry = j_reg_containing_creg(s->j, I_Y(s->i));
    j_reg *rbase = j_get_free_reg(s->j);
    LEA_MEM_TO_REG(s, &s->m, rbase);
    MOV_SIB16_TO_MEM(s, 1, ry, rbase, rx);
}

void translate_ADDI(cpu_state *s)
{
    j_reg *rto = j_reg_containing_creg(s->j, I_X(s->i));
    int16_t imm = I_HHLL(s->i);
    
    if(rto->isconst && rto->value.w == 0) {
        rto->value.w = imm;
    } else {
        if(rto->isconst) {
            rto->value.w += imm;
        } else {
            ADD_IMM16_TO_REG(s, imm, rto);
        }
    }
    rto->dirty = true;
    emit_flags_update(s, FLAG_C | FLAG_Z | FLAG_O | FLAG_N);
}

void translate_ADD_R2(cpu_state *s)
{
    j_reg *rx = j_reg_containing_creg(s->j, I_X(s->i));
    j_reg *ry = j_reg_containing_creg(s->j, I_Y(s->i));
    
    if(rx->isconst && ry->isconst) {
        rx->value.w += ry->value.w;
    } else if(ry->isconst) {
        ADD_IMM16_TO_REG(s, ry->value.w, rx);
    } else {
        ADD_REG16_TO_REG(s, ry, rx);
    }
    rx->dirty = true;
    emit_flags_update(s, FLAG_C | FLAG_Z | FLAG_O | FLAG_N);
}

void translate_ADD_R3(cpu_state *s)
{
    j_reg *rx = j_reg_containing_creg(s->j, I_X(s->i));
    j_reg *ry = j_reg_containing_creg(s->j, I_Y(s->i));
    j_reg *rz = j_reg_containing_creg(s->j, I_Z(s->i));
    
    if(rx->isconst && ry->isconst) {
        rz->value.w = rx->value.w + ry->value.w;
        rz->isconst = true;
    } else {
        if(rx->isconst)
            j_reg_unconst(s, rx);
        if(ry->isconst)
            j_reg_unconst(s, ry);
        // LEA_SIB_TO_REG(s, 0, rz, ry, rx);
        MOV_REG16_TO_REG(s, ry, rx);
        ADD_REG16_TO_REG(s, rz, rx);
    }
    rz->dirty = true;
    emit_flags_update(s, FLAG_C | FLAG_Z | FLAG_O | FLAG_N);
}

void translate_SUBI(cpu_state *s)
{
    j_reg *rto = j_reg_containing_creg(s->j, I_X(s->i));
    int16_t imm = I_HHLL(s->i);
    
    if(rto->isconst && rto->value.w == 0) {
        rto->value.w = -imm;
    } else {
        if(rto->isconst) {
            rto->value.w -= imm;
        } else {
            SUB_IMM16_TO_REG(s, imm, rto);
        }
    }
    rto->dirty = true;
    emit_flags_update(s, FLAG_C | FLAG_Z | FLAG_O | FLAG_N);
}

void translate_SUB_R2(cpu_state *s)
{
    j_reg *rx = j_reg_containing_creg(s->j, I_X(s->i));
    j_reg *ry = j_reg_containing_creg(s->j, I_Y(s->i));
    
    if(rx->isconst && ry->isconst) {
        rx->value.w -= ry->value.w;
    } else if(ry->isconst) {
        SUB_IMM16_TO_REG(s, ry->value.w, rx);
    } else {
        SUB_REG16_TO_REG(s, ry, rx);
    }
    rx->dirty = true;
    emit_flags_update(s, FLAG_C | FLAG_Z | FLAG_O | FLAG_N);
}

void translate_SUB_R3(cpu_state *s)
{
    j_reg *rx = j_reg_containing_creg(s->j, I_X(s->i));
    j_reg *ry = j_reg_containing_creg(s->j, I_Y(s->i));
    j_reg *rz = j_reg_containing_creg(s->j, I_Z(s->i));
    
    if(rx->isconst && ry->isconst) {
        rz->value.w = rx->value.w - ry->value.w;
        rz->isconst = true;
    } else {
        if(rx->isconst)
            j_reg_unconst(s, rx);
        if(ry->isconst)
            j_reg_unconst(s, ry);
        // LEA_SIB_TO_REG(s, 0, rz, ry, rx);
        MOV_REG16_TO_REG(s, ry, rx);
        SUB_REG16_TO_REG(s, rz, rx);
    }
    rz->dirty = true;
    emit_flags_update(s, FLAG_C | FLAG_Z | FLAG_O | FLAG_N);
}

void translate_CMPI(cpu_state *s)
{
    j_reg *rto = j_reg_containing_creg(s->j, I_X(s->i));
    int16_t imm = I_HHLL(s->i);
    
    if(rto->isconst && rto->value.w == 0) {
    } else {
        if(rto->isconst) {
            rto->value.w -= imm;
        } else {
            CMP_IMM16_TO_REG(s, imm, rto);
        }
    }
    rto->dirty = true;
    emit_flags_update(s, FLAG_C | FLAG_Z | FLAG_O | FLAG_N);
}

void translate_CMP(cpu_state *s)
{
    j_reg *rx = j_reg_containing_creg(s->j, I_X(s->i));
    j_reg *ry = j_reg_containing_creg(s->j, I_Y(s->i));
    
    if(rx->isconst && ry->isconst) {
    } else if(ry->isconst) {
        CMP_IMM16_TO_REG(s, ry->value.w, rx);
    } else {
        CMP_REG16_TO_REG(s, ry, rx);
    }
    rx->dirty = true;
    emit_flags_update(s, FLAG_C | FLAG_Z | FLAG_O | FLAG_N);
}

void translate_ANDI(cpu_state *s)
{
    j_reg *rto = j_reg_containing_creg(s->j, I_X(s->i));
    int16_t imm = I_HHLL(s->i);
    
    if(rto->isconst && rto->value.w == 0) {
        rto->value.w = 0;
    } else {
        if(rto->isconst) {
            rto->value.w &= imm;
        } else {
            AND_IMM16_TO_REG(s, imm, rto);
        }
    }
    rto->dirty = true;
    emit_flags_update(s, FLAG_Z | FLAG_N);
}

void translate_AND_R2(cpu_state *s)
{
    j_reg *rx = j_reg_containing_creg(s->j, I_X(s->i));
    j_reg *ry = j_reg_containing_creg(s->j, I_Y(s->i));
    
    if(rx->isconst && ry->isconst) {
        rx->value.w &= ry->value.w;
    } else if(ry->isconst) {
        AND_IMM16_TO_REG(s, ry->value.w, rx);
    } else {
        AND_REG16_TO_REG(s, ry, rx);
    }
    rx->dirty = true;
    emit_flags_update(s, FLAG_Z | FLAG_N);
}

void translate_AND_R3(cpu_state *s)
{
    j_reg *rx = j_reg_containing_creg(s->j, I_X(s->i));
    j_reg *ry = j_reg_containing_creg(s->j, I_Y(s->i));
    j_reg *rz = j_reg_containing_creg(s->j, I_Z(s->i));
    
    if(rx->isconst && ry->isconst) {
        rz->value.w = rx->value.w & ry->value.w;
        rz->isconst = true;
    } else {
        if(rx->isconst)
            j_reg_unconst(s, rx);
        if(ry->isconst)
            j_reg_unconst(s, ry);
        if(rz->isconst)
            rz->isconst = false;
        // LEA_SIB_TO_REG(s, 0, rz, ry, rx);
        MOV_REG16_TO_REG(s, ry, rx);
        AND_REG16_TO_REG(s, rz, rx);
    }
    rz->dirty = true;
    emit_flags_update(s, FLAG_Z | FLAG_N);
}

void translate_TSTI(cpu_state *s)
{
    j_reg *rto = j_reg_containing_creg(s->j, I_X(s->i));
    j_reg *rtemp = j_get_free_reg(s->j);
    int16_t imm = I_HHLL(s->i);
    
    if(rto->isconst && rto->value.w == 0) {
    } else {
        if(rto->isconst) {
        } else {
            MOV_REG16_TO_REG(s, rto, rtemp);
            AND_IMM16_TO_REG(s, imm, rtemp);
        }
    }
    rto->dirty = true;
    emit_flags_update(s, FLAG_Z | FLAG_N);
}

void translate_TST(cpu_state *s)
{
    j_reg *rx = j_reg_containing_creg(s->j, I_X(s->i));
    j_reg *ry = j_reg_containing_creg(s->j, I_Y(s->i));
    j_reg *rtemp = j_get_free_reg(s->j);
    
    if(rx->isconst && ry->isconst) {
    } else if(ry->isconst) {
    } else {
        MOV_REG16_TO_REG(s, rx, rtemp);
        AND_REG16_TO_REG(s, ry, rtemp);
    }
    rx->dirty = true;
    emit_flags_update(s, FLAG_C | FLAG_Z | FLAG_O | FLAG_N);
}

void translate_ORI(cpu_state *s)
{
    j_reg *rto = j_reg_containing_creg(s->j, I_X(s->i));
    int16_t imm = I_HHLL(s->i);
    
    if(rto->isconst && rto->value.w == 0) {
        rto->value.w = imm;
    } else {
        if(rto->isconst) {
            rto->value.w |= imm;
        } else {
            OR_IMM16_TO_REG(s, imm, rto);
        }
    }
    rto->dirty = true;
    emit_flags_update(s, FLAG_Z | FLAG_N);
}

void translate_OR_R2(cpu_state *s)
{
    j_reg *rx = j_reg_containing_creg(s->j, I_X(s->i));
    j_reg *ry = j_reg_containing_creg(s->j, I_Y(s->i));
    
    if(rx->isconst && ry->isconst) {
        rx->value.w |= ry->value.w;
    } else if(ry->isconst) {
        OR_IMM16_TO_REG(s, ry->value.w, rx);
    } else {
        OR_REG16_TO_REG(s, ry, rx);
    }
    rx->dirty = true;
    emit_flags_update(s, FLAG_Z | FLAG_N);
}

void translate_OR_R3(cpu_state *s)
{
    j_reg *rx = j_reg_containing_creg(s->j, I_X(s->i));
    j_reg *ry = j_reg_containing_creg(s->j, I_Y(s->i));
    j_reg *rz = j_reg_containing_creg(s->j, I_Z(s->i));
    
    if(rx->isconst && ry->isconst) {
        rz->value.w = rx->value.w | ry->value.w;
        rz->isconst = true;
    } else {
        if(rx->isconst)
            j_reg_unconst(s, rx);
        if(ry->isconst)
            j_reg_unconst(s, ry);
        if(rz->isconst)
            rz->isconst = false;
        // LEA_SIB_TO_REG(s, 0, rz, ry, rx);
        MOV_REG16_TO_REG(s, ry, rx);
        OR_REG16_TO_REG(s, rz, rx);
    }
    rz->dirty = true;
    emit_flags_update(s, FLAG_Z | FLAG_N);
}

void translate_XORI(cpu_state *s)
{
    j_reg *rto = j_reg_containing_creg(s->j, I_X(s->i));
    int16_t imm = I_HHLL(s->i);
    
    if(rto->isconst && rto->value.w == 0) {
        rto->value.w = imm ^ 0;
    } else {
        if(rto->isconst) {
            rto->value.w ^= imm;
        } else {
            XOR_IMM16_TO_REG(s, imm, rto);
        }
    }
    rto->dirty = true;
    emit_flags_update(s, FLAG_Z | FLAG_N);
}

void translate_XOR_R2(cpu_state *s)
{
    j_reg *rx = j_reg_containing_creg(s->j, I_X(s->i));
    j_reg *ry = j_reg_containing_creg(s->j, I_Y(s->i));
    
    if(rx->isconst && ry->isconst) {
        rx->value.w ^= ry->value.w;
    } else if(ry->isconst) {
        XOR_IMM16_TO_REG(s, ry->value.w, rx);
    } else {
        XOR_REG16_TO_REG(s, ry, rx);
    }
    rx->dirty = true;
    emit_flags_update(s, FLAG_Z | FLAG_N);
}

void translate_XOR_R3(cpu_state *s)
{
    j_reg *rx = j_reg_containing_creg(s->j, I_X(s->i));
    j_reg *ry = j_reg_containing_creg(s->j, I_Y(s->i));
    j_reg *rz = j_reg_containing_creg(s->j, I_Z(s->i));
    
    if(rx->isconst && ry->isconst) {
        rz->value.w = rx->value.w ^ ry->value.w;
        rz->isconst = true;
    } else {
        if(rx->isconst)
            j_reg_unconst(s, rx);
        if(ry->isconst)
            j_reg_unconst(s, ry);
        if(rz->isconst)
            rz->isconst = false;
        // LEA_SIB_TO_REG(s, 0, rz, ry, rx);
        MOV_REG16_TO_REG(s, ry, rx);
        XOR_REG16_TO_REG(s, rz, rx);
    }
    rz->dirty = true;
    emit_flags_update(s, FLAG_Z | FLAG_N);
}

void translate_MULI(cpu_state *s)
{
    j_reg *rx = j_reg_containing_creg(s->j, I_X(s->i));

    if(rx->isconst) {
        rx->value.w *= I_HHLL(s->i);
    } else {
        IMUL_IMM32_REG32_TO_REG32(s, I_HHLL(s->i), rx, rx);
    }
    rx->dirty = true;
    emit_flags_update(s, FLAG_C | FLAG_Z | FLAG_N);
}

void translate_MUL_R2(cpu_state *s)
{
    j_reg *rx = j_reg_containing_creg(s->j, I_X(s->i));
    j_reg *ry = j_reg_containing_creg(s->j, I_Y(s->i));

    if(rx->isconst && ry->isconst) {
        rx->value.w *= ry->value.w;
    } else if(ry->isconst) {
        IMUL_IMM32_REG32_TO_REG32(s, ry->value.w, rx, rx);
    } else {
        if(rx->isconst)
            rx->isconst = false;
        IMUL_REG32_TO_REG32(s, ry, rx);
    }
    rx->dirty = true;
    emit_flags_update(s, FLAG_C | FLAG_Z | FLAG_N);
}

void translate_MUL_R3(cpu_state *s)
{
    j_reg *rx = j_reg_containing_creg(s->j, I_X(s->i));
    j_reg *ry = j_reg_containing_creg(s->j, I_Y(s->i));
    j_reg *rz = j_reg_containing_creg(s->j, I_Z(s->i));

    if(rx->isconst && ry->isconst && rz->isconst) {
        rz->value.w = rx->value.w * ry->value.w;
    } else if(rx->isconst && ry->isconst) {
        MOV_IMM16_TO_REG(s, rx->value.w * ry->value.w, rz);
    } else if(ry->isconst) {
        IMUL_IMM32_REG32_TO_REG32(s, ry->value.w, rx, rz);
    } else if(rx->isconst) {
        IMUL_IMM32_REG32_TO_REG32(s, rx->value.w, ry, rz);
    } else {
        if(rz->isconst)
            rz->isconst = false;
        IMUL_REG32_REG32_TO_REG32(s, rx, ry, rz);
    }
    rz->dirty = true;
    emit_flags_update(s, FLAG_C | FLAG_Z | FLAG_N);
}

void translate_DIVI(cpu_state *s)
{
    j_reg *rx = j_reg_containing_creg_choose(s->j, I_X(s->i), RAX);
    j_reg *rdiv = j_get_free_reg(s->j);
    if(rx->isconst) {
        rx->value.w *= I_HHLL(s->i);
    } else {
        MOV_IMM16_TO_REG(s, I_HHLL(s->i), rdiv);
        IDIV_EAX_TO_REG32(s, rdiv);
    }
    rx->dirty = true;
    emit_flags_update(s, FLAG_C | FLAG_Z | FLAG_N);
}

void translate_DIV_R2(cpu_state *s)
{
    j_reg *rx = j_reg_containing_creg_choose(s->j, I_X(s->i), RAX);
    j_reg *ry = j_reg_containing_creg(s->j, I_Y(s->i));
    if(rx->isconst && ry->isconst) {
        rx->value.w /= ry->value.w;
    } else {
        if(rx->isconst)
            j_reg_unconst(s->j, rx);
        else if(ry->isconst)
            j_reg_unconst(s->j, ry);
        IDIV_EAX_TO_REG32(s, ry);
    }
    rx->dirty = true;
    emit_flags_update(s, FLAG_C | FLAG_Z | FLAG_N);
}

void translate_DIV_R3(cpu_state *s)
{
    j_reg *rx = j_reg_containing_creg_choose(s->j, I_X(s->i), RAX);
    j_reg *ry = j_reg_containing_creg(s->j, I_Y(s->i));
    j_reg *rz = j_reg_containing_creg(s->j, I_Z(s->i));
    if(rx->isconst && ry->isconst) {
        rz->value.w = rx->value.w / ry->value.w;
        rz->isconst = true;
    } else {
        if(rx->isconst)
            j_reg_unconst(s->j, rx);
        else if(ry->isconst)
            j_reg_unconst(s->j, ry);
        if(rz->isconst)
            rz->isconst = false;
        IDIV_EAX_TO_REG32(s, ry);
        MOV_REG32_TO_REG(s, rx, rz);
    }
    rx->dirty = true;
    emit_flags_update(s, FLAG_C | FLAG_Z | FLAG_N);
}

void translate_SHL_N(cpu_state *s)
{
    j_reg *rx = j_reg_containing_creg(s->j, I_X(s->i));
    if(rx->isconst) {
        rx->value.w <<= I_N(s->i);
    } else {
        SHL_IMM8_TO_REG(s, I_N(s->i), rx);
    }
    rx->dirty = true;
    emit_flags_update(s, FLAG_Z | FLAG_N);
}

void translate_SHR_N(cpu_state *s)
{
    j_reg *rx = j_reg_containing_creg(s->j, I_X(s->i));
    if(rx->isconst) {
        rx->value.uw >>= I_N(s->i);
    } else {
        SHR_IMM8_TO_REG(s, I_N(s->i), rx);
    }
    rx->dirty = true;
    emit_flags_update(s, FLAG_Z | FLAG_N);
}

void translate_SAR_N(cpu_state *s)
{
    j_reg *rx = j_reg_containing_creg(s->j, I_X(s->i));
    if(rx->isconst) {
        rx->value.uw >>= I_N(s->i);
    } else {
        SAR_IMM8_TO_REG(s, I_N(s->i), rx);
    }
    rx->dirty = true;
    emit_flags_update(s, FLAG_Z | FLAG_N);
}

void translate_SHL_R(cpu_state *s)
{
    j_reg *rx = j_reg_containing_creg(s->j, I_X(s->i));
    j_reg *ry = j_reg_containing_creg_choose(s->j, I_Y(s->i), RCX);
    if(rx->isconst && ry->isconst) {
        rx->value.w <<= ry->value.w;
    } else {
        if(rx->isconst)
            j_reg_unconst(s->j, rx);
        else if(ry->isconst)
            j_reg_unconst(s->j, ry);
        SHL_CL_TO_REG(s, rx);
    }
    rx->dirty = true;
    emit_flags_update(s, FLAG_Z | FLAG_N);
}

void translate_SHR_R(cpu_state *s)
{
    j_reg *rx = j_reg_containing_creg(s->j, I_X(s->i));
    j_reg *ry = j_reg_containing_creg_choose(s->j, I_Y(s->i), RCX);
    if(rx->isconst && ry->isconst) {
        rx->value.uw >>= ry->value.w;
    } else {
        if(rx->isconst)
            j_reg_unconst(s->j, rx);
        else if(ry->isconst)
            j_reg_unconst(s->j, ry);
        SHR_CL_TO_REG(s, rx);
    }
    rx->dirty = true;
    emit_flags_update(s, FLAG_Z | FLAG_N);
}

void translate_SAR_R(cpu_state *s)
{
    j_reg *rx = j_reg_containing_creg(s->j, I_X(s->i));
    j_reg *ry = j_reg_containing_creg_choose(s->j, I_Y(s->i), RCX);
    if(rx->isconst && ry->isconst) {
        rx->value.w >>= ry->value.w;
    } else {
        if(rx->isconst)
            j_reg_unconst(s->j, rx);
        else if(ry->isconst)
            j_reg_unconst(s->j, ry);
        SAR_CL_TO_REG(s, rx);
    }
    rx->dirty = true;
    emit_flags_update(s, FLAG_Z | FLAG_N);
}

void translate_PUSH(cpu_state *s)
{
    j_reg *rx = j_reg_containing_creg(s->j, I_X(s->i));
    j_reg *rmem = j_get_free_reg(s->j);
    MOV_MEM16_TO_REG(s, &s->sp, rmem);
    if(rx->isconst) {
        MOV_IMM16_TO_REGIND16_DISP32(s, rx->value.w, rmem, &s->m);
    } else {
        MOV_REG16_TO_REGIND16_DISP32(s, rx, rmem, &s->m);
    }
    ADD_IMM16_TO_MEM(s, 2, &s->sp);
}

void translate_POP(cpu_state *s)
{
    j_reg *rx = j_reg_containing_creg(s->j, I_X(s->i));
    j_reg *rmem = j_get_free_reg(s->j);

    SUB_IMM16_TO_MEM(s, 2, &s->sp);
    MOV_MEM16_TO_REG(s, &s->sp, rmem);
    if(rx->isconst)
        rx->isconst = false;
    MOV_REGIND16_DISP32_TO_REG(s, rmem, &s->m, rx);
}

void translate_PUSHF(cpu_state *s)
{
    j_reg *rmem = j_get_free_reg(s->j);
    MOV_MEM16_TO_REG(s, &s->sp, rmem);
    MOV_IMM16_TO_REGIND16_DISP32(s, MAKEFLAGS(s), rmem, &s->m);
    ADD_IMM16_TO_MEM(s, 2, &s->sp);
}

void translate_POPF(cpu_state *s)
{
    j_reg *rrdi = j_get_reg(s->j, RDI);
    j_reg *rrsi = j_get_reg(s->j, RSI);
    j_reg *rmem = j_get_free_reg(s->j);
    SUB_IMM16_TO_MEM(s, 2, &s->sp);
    MOV_MEM16_TO_REG(s, &s->sp, rmem);
    MOV_REGIND16_DISP32_TO_REG(s, rmem, &s->m, rrsi);
    LEA_MEM_TO_REG(s, s, rrdi);
    CALL_MEM(s, UNPACK_WRITE_FLAGS);
}

void translate_PUSHALL(cpu_state *s)
{
}

void translate_POPALL(cpu_state *s)
{
}

void translate_PAL_I(cpu_state *s)
{
}

void translate_PAL_R(cpu_state *s)
{
}

void translate_NOTI(cpu_state *s)
{
}

void translate_NOT_R1(cpu_state *s)
{
}

void translate_NOT_R2(cpu_state *s)
{
}

void translate_NEGI(cpu_state *s)
{
}

void translate_NEG_R1(cpu_state *s)
{
}

void translate_NEG_R2(cpu_state *s)
{
}

