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

#include "../consts.h"
#include "cpu.h"
#include "gpu.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

extern int use_verbose;
cpu_op op_table[0x100];
size_t page_size;

static uint32_t xorshift(uint32_t prev)
{
   prev ^= prev << 13;
   prev ^= prev >> 17;
   prev ^= prev << 5;
   return prev;
}

uint32_t mash16_rand(cpu_state *state)
{
   state->prev_rand = xorshift(state->prev_rand);
   return state->prev_rand;
}

/* Initialise the CPU to safe values. */
void cpu_init(cpu_state** state, uint8_t* mem, program_opts* opts)
{
    int i;

    page_size = sysconf(_SC_PAGESIZE);

    if (posix_memalign((void **)state, page_size, 16 * 1024 * 1024) < 0)
    {
        fprintf(stderr,"error: posix_memalign failed (state)\n");
        exit(1);
    }
    cpu_rec_init(*state, opts);
    (*state)->rec.bblk_map = calloc(65536, sizeof(cpu_rec_bblk));
    (*state)->meta.old_pc = 0;
    (*state)->pc = 0;
    (*state)->m = mem;
#ifdef HAVE_BANK_SEL 
    for (i = 0; i < 256; ++i)
    {
        if (!((*state)->mp[i] = calloc(0x8000, 1)))
        {
            fprintf(stderr, "error: calloc failed (state->mp[%d])\n", i);
            exit(1);
        }
    }
#endif
    if(!((*state)->vm = calloc(320*240,1)))
    {
        fprintf(stderr,"error: calloc failed (state->vm)\n");
        exit(1);
    }
    if(!((*state)->pal = malloc(16*sizeof(uint32_t))))
    {
        fprintf(stderr,"error: malloc failed (state->pal)\n");
        exit(1);
    }
    if(!((*state)->pal_r = malloc(16)))
    {
        fprintf(stderr,"error: malloc failed (state->pal_r)\n");
        exit(1);
    }
    if(!((*state)->pal_g = malloc(16)))
    {
        fprintf(stderr,"error: malloc failed (state->pal_g)\n");
        exit(1);
    }
    if(!((*state)->pal_b = malloc(16)))
    {
        fprintf(stderr,"error: malloc failed (state->pal_b)\n");
        exit(1);
    }
    (*state)->sp = STACK_ADDR;
    memset(&(*state)->f,0,sizeof(flags));
    
    (*state)->prev_rand = opts->rng_seed;

    /* Ensure unused instructions return errors. */
    for(i=0; i<0x100; ++i)
    {
        op_table[i] = op_error;
    }
    /* Map instr. table entries to functions. */
    op_table[0x00] = op_nop;
    op_table[0x01] = op_cls;
    op_table[0x02] = op_vblnk;
    op_table[0x03] = op_bgc;
    op_table[0x04] = op_spr;
    op_table[0x05] = op_drw_imm;
    op_table[0x06] = op_drw_r;
    op_table[0x07] = op_rnd;
    op_table[0x08] = op_flip;
    op_table[0x09] = op_snd0;
    op_table[0x0a] = op_snd1;
    op_table[0x0b] = op_snd2;
    op_table[0x0c] = op_snd3;
    op_table[0x0d] = op_snp;
    op_table[0x0e] = op_sng;
    op_table[0x10] = op_jmp_imm;
    op_table[0x11] = op_jmc;
    op_table[0x12] = op_jx;
    op_table[0x13] = op_jme;
    op_table[0x14] = op_call_imm;
    op_table[0x15] = op_ret;
    op_table[0x16] = op_jmp_r;
    op_table[0x17] = op_cx;
    op_table[0x18] = op_call_r;
    op_table[0x20] = op_ldi_r;
    op_table[0x21] = op_ldi_sp;
    op_table[0x22] = op_ldm_imm;
    op_table[0x23] = op_ldm_r;
    op_table[0x24] = op_mov;
    op_table[0x30] = op_stm_imm;
    op_table[0x31] = op_stm_r;
    op_table[0x40] = op_addi;
    op_table[0x41] = op_add_r2;
    op_table[0x42] = op_add_r3;
    op_table[0x50] = op_subi;
    op_table[0x51] = op_sub_r2;
    op_table[0x52] = op_sub_r3;
    op_table[0x53] = op_cmpi;
    op_table[0x54] = op_cmp;
    op_table[0x60] = op_andi;
    op_table[0x61] = op_and_r2;
    op_table[0x62] = op_and_r3;
    op_table[0x63] = op_tsti;
    op_table[0x64] = op_tst;
    op_table[0x70] = op_ori;
    op_table[0x71] = op_or_r2;
    op_table[0x72] = op_or_r3;
    op_table[0x80] = op_xori;
    op_table[0x81] = op_xor_r2;
    op_table[0x82] = op_xor_r3;
    op_table[0x90] = op_muli;
    op_table[0x91] = op_mul_r2;
    op_table[0x92] = op_mul_r3;
    op_table[0xa0] = op_divi;
    op_table[0xa1] = op_div_r2;
    op_table[0xa2] = op_div_r3;
    op_table[0xa3] = op_modi;
    op_table[0xa4] = op_mod_r2;
    op_table[0xa5] = op_mod_r3;
    op_table[0xa6] = op_remi;
    op_table[0xa7] = op_rem_r2;
    op_table[0xa8] = op_rem_r3;
    op_table[0xb0] = op_shl_n;
    op_table[0xb1] = op_shr_n;
    op_table[0xb2] = op_sar_n;
    op_table[0xb3] = op_shl_r;
    op_table[0xb4] = op_shr_r;
    op_table[0xb5] = op_sar_r;
    op_table[0xc0] = op_push;
    op_table[0xc1] = op_pop;
    op_table[0xc2] = op_pushall;
    op_table[0xc3] = op_popall;
    op_table[0xc4] = op_pushf;
    op_table[0xc5] = op_popf;
    op_table[0xd0] = op_pal_imm;
    op_table[0xd1] = op_pal_r;
    op_table[0xe0] = op_noti;
    op_table[0xe1] = op_not_r;
    op_table[0xe2] = op_not_r2;
    op_table[0xe3] = op_negi;
    op_table[0xe4] = op_neg_r;
    op_table[0xe5] = op_neg_r2;

    /* Load default palette. */
    init_pal(*state);
}

/* Execute 1 CPU cycle. */
void cpu_step(cpu_state* state)
{
    state->meta.old_pc = state->pc;
    /* Fetch instruction, increase PC. */
    state->i = *(instr*)(&state->m[state->pc]);
    state->pc += 4;
    /* Call function pointer table entry */
    (*op_table[i_op(state->i)])(state);
    /* Update cycles. */
    ++state->meta.cycles;
    ++state->meta.target_cycles;
}

/* Execute a (variable-length) JIT basic block. */
void cpu_rec_1bblk(cpu_state* state)
{
    cpu_rec_bblk* bblk = &state->rec.bblk_map[state->pc];
    state->meta.old_pc = state->pc;
    cpu_rec_validate(state, state->pc);
    if (bblk->code == NULL) {
        printf("> recompiler: compile basic block @ 0x%04x\n", state->pc);
        cpu_rec_compile(state, state->pc);
    }

    //printf("> recompiler: run basic block @ 0x%04x, %d cycles [%p]. r3=%d, r5=%04hx\n",
    //       state->pc, bblk->cycles, bblk->code, state->r[3], state->r[5]);
    bblk->code();
    state->meta.cycles += bblk->cycles;
    state->meta.target_cycles += bblk->cycles;
}

/* Update I/O port contents with gamepad input. */
void cpu_io_update(SDL_KeyboardEvent* key, cpu_state* state)
{
    switch(key->keysym.sym)
    {
        case SDLK_UP:
        {
            if(key->type == SDL_KEYDOWN)
                state->m[IO_PAD1_ADDR] |= PAD_UP;
            else
                state->m[IO_PAD1_ADDR] &= ~PAD_UP;
            break;
        }
        case SDLK_DOWN:
        {
            if(key->type == SDL_KEYDOWN)
                state->m[IO_PAD1_ADDR] |= PAD_DOWN;
            else
                state->m[IO_PAD1_ADDR] &= ~PAD_DOWN;
            break;
        }
        case SDLK_LEFT:
        {
            if(key->type == SDL_KEYDOWN)
                state->m[IO_PAD1_ADDR] |= PAD_LEFT;
            else
                state->m[IO_PAD1_ADDR] &= ~PAD_LEFT;
            break;
        }
        case SDLK_RIGHT:
        {    
            if(key->type == SDL_KEYDOWN)
                state->m[IO_PAD1_ADDR] |= PAD_RIGHT;
            else
                state->m[IO_PAD1_ADDR] &= ~PAD_RIGHT;
            break;
        }
        case SDLK_RSHIFT:
        {
            if(key->type == SDL_KEYDOWN)
                state->m[IO_PAD1_ADDR] |= PAD_SELECT;
            else
                state->m[IO_PAD1_ADDR] &= ~PAD_SELECT;
            break;
        }
        case SDLK_RETURN:
        {
            if(key->type == SDL_KEYDOWN)
                state->m[IO_PAD1_ADDR] |= PAD_START;
            else
                state->m[IO_PAD1_ADDR] &= ~PAD_START;
            break;
        }
        case SDLK_z:
        {
            if(key->type == SDL_KEYDOWN)
                state->m[IO_PAD1_ADDR] |= PAD_A;
            else
                state->m[IO_PAD1_ADDR] &= ~PAD_A;
            break;
        }
        case SDLK_x:
        {
            if(key->type == SDL_KEYDOWN)
                state->m[IO_PAD1_ADDR] |= PAD_B;
            else
                state->m[IO_PAD1_ADDR] &= ~PAD_B;
            break;
        }
        default:
            break;
    }
}

/* Reset I/O ports. */
void cpu_io_reset(cpu_state* state)
{
    state->m[IO_PAD1_ADDR] = 0;
    state->m[IO_PAD2_ADDR] = 0;
}

/* Free resources held by the cpu state. */
void cpu_free(cpu_state* state)
{
    free(state->vm);
#ifdef HAVE_BANK_SEL
    int i;
    for (i = 0; i < 256; ++i)
    {
        free(state->mp[i]);
    }
#endif
    cpu_rec_free(state);
    free(state->rec.bblk_map);
    free(state);
}


