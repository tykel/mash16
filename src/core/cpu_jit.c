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

typedef void (*pfn_jitblock)(void);

static void* gp_blockmap[0x10000 >> 2];
static void* gp_unaligned_blockmap[0x10000 >> 2];

static pfn_jit_op gpfn_jit_ops[0x100] = { NULL };

void cpu_jit_translate(cpu_state *state)
{
    gpfn_jit_ops[i_op(state->i)](state);
}

void cpu_jit_init(cpu_state *state)
{
    int n;
    
    j_init(&state->j);
    
    for(n = 0; n < (0x10000>>2); n++) {
        gp_unaligned_blockmap[n] = NULL;
        gp_blockmap[n] = NULL;
    }

    gpfn_jit_ops[0x00] = translate_NOP;
    gpfn_jit_ops[0x01] = translate_CLS;
    gpfn_jit_ops[0x02] = translate_VBLNK;
    gpfn_jit_ops[0x03] = translate_BGC;
    gpfn_jit_ops[0x04] = translate_SPR;
    gpfn_jit_ops[0x05] = translate_DRW_I;
    gpfn_jit_ops[0x06] = translate_DRW_R;
    gpfn_jit_ops[0x07] = translate_RND;
    gpfn_jit_ops[0x08] = translate_FLIP;
    gpfn_jit_ops[0x09] = translate_SND0;
    gpfn_jit_ops[0x0a] = translate_SND1;
    gpfn_jit_ops[0x0b] = translate_SND2;
    gpfn_jit_ops[0x0c] = translate_SND3;
    gpfn_jit_ops[0x0d] = translate_SNP;
    gpfn_jit_ops[0x0e] = translate_SNG;
    gpfn_jit_ops[0x10] = translate_JMP_I;
    gpfn_jit_ops[0x11] = translate_JMC;
    gpfn_jit_ops[0x12] = translate_Jx;
    gpfn_jit_ops[0x13] = translate_JME;
    gpfn_jit_ops[0x14] = translate_CALL_I;
    gpfn_jit_ops[0x15] = translate_RET;
    gpfn_jit_ops[0x16] = translate_JMP_R;
    gpfn_jit_ops[0x17] = translate_Cx;
    gpfn_jit_ops[0x18] = translate_CALL_R;
    gpfn_jit_ops[0x20] = translate_LDI;
    gpfn_jit_ops[0x21] = translate_LDI_SP;
    gpfn_jit_ops[0x22] = translate_LDM_I;
    gpfn_jit_ops[0x23] = translate_LDM_R;
    gpfn_jit_ops[0x24] = translate_MOV;
    gpfn_jit_ops[0x30] = translate_STM_I;
    gpfn_jit_ops[0x31] = translate_STM_R;
    gpfn_jit_ops[0x40] = translate_ADDI;
    gpfn_jit_ops[0x41] = translate_ADD_R2;
    gpfn_jit_ops[0x42] = translate_ADD_R3;
    gpfn_jit_ops[0x50] = translate_SUBI;
    gpfn_jit_ops[0x51] = translate_SUB_R2;
    gpfn_jit_ops[0x52] = translate_SUB_R3;
    gpfn_jit_ops[0x53] = translate_CMPI;
    gpfn_jit_ops[0x54] = translate_CMP;
    gpfn_jit_ops[0x60] = translate_ANDI;
    gpfn_jit_ops[0x61] = translate_AND_R2;
    gpfn_jit_ops[0x62] = translate_AND_R3;
    gpfn_jit_ops[0x63] = translate_TSTI;
    gpfn_jit_ops[0x64] = translate_TST;
    gpfn_jit_ops[0x70] = translate_ORI;
    gpfn_jit_ops[0x71] = translate_OR_R2;
    gpfn_jit_ops[0x72] = translate_OR_R3;
    gpfn_jit_ops[0x80] = translate_XORI;
    gpfn_jit_ops[0x81] = translate_XOR_R2;
    gpfn_jit_ops[0x82] = translate_XOR_R3;
    gpfn_jit_ops[0x90] = translate_MULI;
    gpfn_jit_ops[0x91] = translate_MUL_R2;
    gpfn_jit_ops[0x92] = translate_MUL_R3;
    gpfn_jit_ops[0xa0] = translate_DIVI;
    gpfn_jit_ops[0xa1] = translate_DIV_R2;
    gpfn_jit_ops[0xa2] = translate_DIV_R3;
    gpfn_jit_ops[0xb0] = translate_SHL_N;
    gpfn_jit_ops[0xb1] = translate_SHR_N;
    gpfn_jit_ops[0xb2] = translate_SAR_N;
    gpfn_jit_ops[0xb3] = translate_SHL_R;
    gpfn_jit_ops[0xb4] = translate_SHR_R;
    gpfn_jit_ops[0xb5] = translate_SAR_R;
    gpfn_jit_ops[0xc0] = translate_PUSH;
    gpfn_jit_ops[0xc1] = translate_POP;
    gpfn_jit_ops[0xc2] = translate_PUSHALL;
    gpfn_jit_ops[0xc3] = translate_POPALL;
    gpfn_jit_ops[0xc4] = translate_PUSHF;
    gpfn_jit_ops[0xc5] = translate_POPF;
    gpfn_jit_ops[0xd0] = translate_PAL_I;
    gpfn_jit_ops[0xd1] = translate_PAL_R;
    gpfn_jit_ops[0xe0] = translate_NOTI;
    gpfn_jit_ops[0xe1] = translate_NOT_R1;
    gpfn_jit_ops[0xe2] = translate_NOT_R2;
    gpfn_jit_ops[0xe3] = translate_NEGI;
    gpfn_jit_ops[0xe4] = translate_NEG_R1;
    gpfn_jit_ops[0xe5] = translate_NEG_R2;
}

void cpu_jit_destroy(cpu_state *state)
{
    j_destroy(state->j);
    state->j = NULL;
}

void cpu_jit_run(cpu_state *state)
{
    uint16_t start = state->pc;
    void *b = cpu_jit_get_block(state, start);
    ((pfn_jitblock)b)();
    printf(".");
    fflush(stdout);
}

void* cpu_jit_get_block(cpu_state *state, uint16_t a)
{
    void *p = NULL;
    j_block_node *b = j_get_block(state->j, a);

    if(b->init) {
        p = b->ptr;
    } else {
        uint16_t i, ni;
        uint16_t end = cpu_jit_find_block_end(state, state->pc);
        for(i = state->pc; i < end; i++) {
            cpu_jit_translate(state);
        }
        ni = (end - state->pc)/4;
        em2(add_imm32_to_mem, P(state), ni, (int32_t *)&state->meta.cycles);
        em0(ret, P(state));
    }
}

void cpu_jit_emit_zeroregs(cpu_state *state)
{
}

void cpu_jit_emit_cyclecount(cpu_state *state, size_t ni)
{
}

uint16_t cpu_jit_find_block_end(cpu_state *state, uint16_t start)
{
    uint16_t end = start;
    while(!(state->m[end] & 0x10) && end < 0xfffc)
        end += 4;
    return end;
}

