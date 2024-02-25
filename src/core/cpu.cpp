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
size_t page_size;

cpu_op_entry op_table[] = {
    // 0x
   { "nop",    OP_NONE,       0,       op_nop },
   { "cls",    OP_NONE,       0,       op_cls },
   { "vblnk",  OP_NONE,       0,       op_vblnk },
   { "bgc",    OP_N,          0,       op_bgc },
   { "spr",    OP_HHLL,       0,       op_spr },
   { "drw",    OP_R_R_HHLL,   FLAG_C,  op_drw_imm },
   { "drw",    OP_R_R_R,      FLAG_C,  op_drw_r },
   { "rnd",    OP_R_HHLL,     0,       op_rnd },
   { "flip",   OP_N_N,        0,       op_flip },
   { "snd0",   OP_NONE,       0,       op_snd0 },
   { "snd1",   OP_HHLL,       0,       op_snd1 },
   { "snd2",   OP_HHLL,       0,       op_snd2 },
   { "snd3",   OP_HHLL,       0,       op_snd3 },
   { "snp",    OP_R_HHLL,     0,       op_snp },
   { "sng",    OP_HHLL_HHLL,  0,       op_sng },
   { "0x0f",   OP_NONE, 0, op_error },
   // 1x
   { "jmp",    OP_HHLL,       0,       op_jmp_imm },
   { "jmc",    OP_HHLL,       0,       op_jmc },
   { "jx",     OP_HHLL,       0,       op_jx },
   { "jme",    OP_R_R_HHLL,   0,       op_jme },
   { "call",   OP_HHLL,       0,       op_call_imm },
   { "ret",    OP_NONE,       0,       op_ret, },
   { "jmp",    OP_R,          0,       op_jmp_r },
   { "cx",     OP_HHLL,       0,       op_cx },
   { "call",   OP_R,          0,       op_call_r },
   { "0x19",   OP_NONE, 0, op_error },
   { "0x1a",   OP_NONE, 0, op_error },
   { "0x1b",   OP_NONE, 0, op_error },
   { "0x1c",   OP_NONE, 0, op_error },
   { "0x1d",   OP_NONE, 0, op_error },
   { "0x1e",   OP_NONE, 0, op_error },
   { "0x1f",   OP_NONE, 0, op_error },
   // 2x
   { "ldi",    OP_R_HHLL,     0,       op_ldi_r },
   { "ldi",    OP_SP_HHLL,    0,       op_ldi_sp },
   { "ldm",    OP_R_HHLL,     0,       op_ldm_imm },
   { "ldm",    OP_R_R,        0,       op_ldm_r },
   { "mov",    OP_HHLL, 0, op_error },
   { "0x25",   OP_NONE, 0, op_error },
   { "0x26",   OP_NONE, 0, op_error },
   { "0x27",   OP_NONE, 0, op_error },
   { "0x28",   OP_NONE, 0, op_error },
   { "0x29",   OP_NONE, 0, op_error },
   { "0x2a",   OP_NONE, 0, op_error },
   { "0x2b",   OP_NONE, 0, op_error },
   { "0x2c",   OP_NONE, 0, op_error },
   { "0x2d",   OP_NONE, 0, op_error },
   { "0x2e",   OP_NONE, 0, op_error },
   { "0x2f",   OP_NONE, 0, op_error },
   // 3x
   { "stm",    OP_R_HHLL,     0,       op_stm_imm },
   { "stm",    OP_R_R,        0,       op_stm_r },
   { "0x32",   OP_NONE, 0, op_error },
   { "0x33",   OP_NONE, 0, op_error },
   { "0x34",   OP_NONE, 0, op_error },
   { "0x35",   OP_NONE, 0, op_error },
   { "0x36",   OP_NONE, 0, op_error },
   { "0x37",   OP_NONE, 0, op_error },
   { "0x38",   OP_NONE, 0, op_error },
   { "0x39",   OP_NONE, 0, op_error },
   { "0x3a",   OP_NONE, 0, op_error },
   { "0x3b",   OP_NONE, 0, op_error },
   { "0x3c",   OP_NONE, 0, op_error },
   { "0x3d",   OP_NONE, 0, op_error },
   { "0x3e",   OP_NONE, 0, op_error },
   { "0x3f",   OP_NONE, 0, op_error },
   // 4x
   { "addi",   OP_R_HHLL,   FLAGS_CZON, op_addi },
   { "add",    OP_R_R,      FLAGS_CZON, op_add_r2 },
   { "add",    OP_R_R_R,    FLAGS_CZON, op_add_r3 },
   { "0x43",   OP_NONE, 0, op_error },
   { "0x44",   OP_NONE, 0, op_error },
   { "0x45",   OP_NONE, 0, op_error },
   { "0x46",   OP_NONE, 0, op_error },
   { "0x47",   OP_NONE, 0, op_error },
   { "0x48",   OP_NONE, 0, op_error },
   { "0x49",   OP_NONE, 0, op_error },
   { "0x4a",   OP_NONE, 0, op_error },
   { "0x4b",   OP_NONE, 0, op_error },
   { "0x4c",   OP_NONE, 0, op_error },
   { "0x4d",   OP_NONE, 0, op_error },
   { "0x4e",   OP_NONE, 0, op_error },
   { "0x4f",   OP_NONE, 0, op_error },
   // 5x
   { "subi",   OP_R_HHLL,   FLAGS_CZON, op_subi },
   { "sub",    OP_R_R,      FLAGS_CZON, op_sub_r2 },
   { "sub",    OP_R_R_R,    FLAGS_CZON, op_sub_r3 },
   { "cmpi",   OP_R_HHLL,   FLAGS_CZON, op_cmpi },
   { "cmp",    OP_R_R,      FLAGS_CZON, op_cmp },
   { "0x55",   OP_NONE, 0, op_error },
   { "0x56",   OP_NONE, 0, op_error },
   { "0x57",   OP_NONE, 0, op_error },
   { "0x58",   OP_NONE, 0, op_error },
   { "0x59",   OP_NONE, 0, op_error },
   { "0x5a",   OP_NONE, 0, op_error },
   { "0x5b",   OP_NONE, 0, op_error },
   { "0x5c",   OP_NONE, 0, op_error },
   { "0x5d",   OP_NONE, 0, op_error },
   { "0x5e",   OP_NONE, 0, op_error },
   { "0x5f",   OP_NONE, 0, op_error },
   // 6x
   { "andi",   OP_R_HHLL,   FLAGS_ZN,   op_andi },
   { "and",    OP_R_R,      FLAGS_ZN,   op_and_r2 },
   { "and",    OP_R_R_R,    FLAGS_ZN,   op_and_r3 },
   { "tsti",   OP_R_HHLL,   FLAGS_ZN,   op_tsti },
   { "tst",    OP_R_R,      FLAGS_ZN,   op_tst },
   { "0x65",   OP_NONE, 0, op_error },
   { "0x66",   OP_NONE, 0, op_error },
   { "0x67",   OP_NONE, 0, op_error },
   { "0x68",   OP_NONE, 0, op_error },
   { "0x69",   OP_NONE, 0, op_error },
   { "0x6a",   OP_NONE, 0, op_error },
   { "0x6b",   OP_NONE, 0, op_error },
   { "0x6c",   OP_NONE, 0, op_error },
   { "0x6d",   OP_NONE, 0, op_error },
   { "0x6e",   OP_NONE, 0, op_error },
   { "0x6f",   OP_NONE, 0, op_error },
   // 7x
   { "ori",    OP_R_HHLL,   FLAGS_ZN,   op_ori },
   { "or",     OP_R_R,      FLAGS_ZN,   op_or_r2 },
   { "or",     OP_R_R_R,    FLAGS_ZN,   op_or_r3 },
   { "0x73",   OP_NONE, 0, op_error },
   { "0x74",   OP_NONE, 0, op_error },
   { "0x75",   OP_NONE, 0, op_error },
   { "0x76",   OP_NONE, 0, op_error },
   { "0x77",   OP_NONE, 0, op_error },
   { "0x78",   OP_NONE, 0, op_error },
   { "0x79",   OP_NONE, 0, op_error },
   { "0x7a",   OP_NONE, 0, op_error },
   { "0x7b",   OP_NONE, 0, op_error },
   { "0x7c",   OP_NONE, 0, op_error },
   { "0x7d",   OP_NONE, 0, op_error },
   { "0x7e",   OP_NONE, 0, op_error },
   { "0x7f",   OP_NONE, 0, op_error },
   // 8x
   { "xori",   OP_R_HHLL,   FLAGS_ZN,   op_xori },
   { "xor",    OP_R_R,      FLAGS_ZN,   op_xor_r2 },
   { "xor",    OP_R_R_R,    FLAGS_ZN,   op_xor_r3 },
   { "0x83",   OP_NONE, 0, op_error },
   { "0x84",   OP_NONE, 0, op_error },
   { "0x85",   OP_NONE, 0, op_error },
   { "0x86",   OP_NONE, 0, op_error },
   { "0x87",   OP_NONE, 0, op_error },
   { "0x88",   OP_NONE, 0, op_error },
   { "0x89",   OP_NONE, 0, op_error },
   { "0x8a",   OP_NONE, 0, op_error },
   { "0x8b",   OP_NONE, 0, op_error },
   { "0x8c",   OP_NONE, 0, op_error },
   { "0x8d",   OP_NONE, 0, op_error },
   { "0x8e",   OP_NONE, 0, op_error },
   { "0x8f",   OP_NONE, 0, op_error },
   // 9x
   { "muli",   OP_R_HHLL,   FLAGS_CZN,  op_muli },
   { "mul",    OP_R_R,      FLAGS_CZN,  op_mul_r2 },
   { "mul",    OP_R_R_R,    FLAGS_CZN,  op_mul_r3 },
   { "0x93",   OP_NONE, 0, op_error },
   { "0x94",   OP_NONE, 0, op_error },
   { "0x95",   OP_NONE, 0, op_error },
   { "0x96",   OP_NONE, 0, op_error },
   { "0x97",   OP_NONE, 0, op_error },
   { "0x98",   OP_NONE, 0, op_error },
   { "0x99",   OP_NONE, 0, op_error },
   { "0x9a",   OP_NONE, 0, op_error },
   { "0x9b",   OP_NONE, 0, op_error },
   { "0x9c",   OP_NONE, 0, op_error },
   { "0x9d",   OP_NONE, 0, op_error },
   { "0x9e",   OP_NONE, 0, op_error },
   { "0x9f",   OP_NONE, 0, op_error },
   // Ax
   { "divi",   OP_R_HHLL,   FLAGS_CZN,  op_divi },
   { "div",    OP_R_R,      FLAGS_CZN,  op_div_r2 },
   { "div",    OP_R_R_R,    FLAGS_CZN,  op_div_r3 },
   { "modi",   OP_R_HHLL,   FLAGS_ZN,   op_modi },
   { "mod",    OP_R_R,      FLAGS_ZN,   op_mod_r2 },
   { "mod",    OP_R_R_R,    FLAGS_ZN,   op_mod_r3 },
   { "remi",   OP_R_HHLL,   FLAGS_ZN,   op_remi },
   { "rem",    OP_R_R,      FLAGS_ZN,   op_rem_r2 },
   { "rem",    OP_R_R_R,    FLAGS_ZN,   op_rem_r3 },
   { "0xa9",   OP_NONE, 0, op_error },
   { "0xaa",   OP_NONE, 0, op_error },
   { "0xab",   OP_NONE, 0, op_error },
   { "0xac",   OP_NONE, 0, op_error },
   { "0xad",   OP_NONE, 0, op_error },
   { "0xae",   OP_NONE, 0, op_error },
   { "0xaf",   OP_NONE, 0, op_error },
   // Bx
   { "shl",    OP_R_N,      FLAGS_ZN,   op_shl_n },
   { "shr",    OP_R_N,      FLAGS_ZN,   op_shr_n },
   { "sar",    OP_R_N,      FLAGS_ZN,   op_sar_n },
   { "shl",    OP_R_R,      FLAGS_ZN,   op_shl_r },
   { "shr",    OP_R_R,      FLAGS_ZN,   op_shr_r },
   { "sar",    OP_R_R,      FLAGS_ZN,   op_sar_r },
   { "0xb6",   OP_NONE, 0, op_error },
   { "0xb7",   OP_NONE, 0, op_error },
   { "0xb8",   OP_NONE, 0, op_error },
   { "0xb9",   OP_NONE, 0, op_error },
   { "0xba",   OP_NONE, 0, op_error },
   { "0xbb",   OP_NONE, 0, op_error },
   { "0xbc",   OP_NONE, 0, op_error },
   { "0xbd",   OP_NONE, 0, op_error },
   { "0xbe",   OP_NONE, 0, op_error },
   { "0xbf",   OP_NONE, 0, op_error },
   // Cx
   { "push",   OP_R,        0,          op_push },
   { "pop",    OP_R,        0,          op_pop },
   { "pushall",OP_NONE,     0,          op_pushall },
   { "popall", OP_NONE,     0,          op_popall },
   { "pushf",  OP_NONE,     0,          op_pushf },
   { "popf",   OP_NONE,     0,          op_popf },
   { "0xc6",   OP_NONE, 0, op_error },
   { "0xc7",   OP_NONE, 0, op_error },
   { "0xc8",   OP_NONE, 0, op_error },
   { "0xc9",   OP_NONE, 0, op_error },
   { "0xca",   OP_NONE, 0, op_error },
   { "0xcb",   OP_NONE, 0, op_error },
   { "0xcc",   OP_NONE, 0, op_error },
   { "0xcd",   OP_NONE, 0, op_error },
   { "0xce",   OP_NONE, 0, op_error },
   { "0xcf",   OP_NONE, 0, op_error },
   // Dx
   { "pal",    OP_HHLL,     0,          op_pal_imm },
   { "pal",    OP_R,        0,          op_pal_r },
   { "0xd2",   OP_NONE, 0, op_error },
   { "0xd3",   OP_NONE, 0, op_error },
   { "0xd4",   OP_NONE, 0, op_error },
   { "0xd5",   OP_NONE, 0, op_error },
   { "0xd6",   OP_NONE, 0, op_error },
   { "0xd7",   OP_NONE, 0, op_error },
   { "0xd8",   OP_NONE, 0, op_error },
   { "0xd9",   OP_NONE, 0, op_error },
   { "0xda",   OP_NONE, 0, op_error },
   { "0xdb",   OP_NONE, 0, op_error },
   { "0xdc",   OP_NONE, 0, op_error },
   { "0xdd",   OP_NONE, 0, op_error },
   { "0xde",   OP_NONE, 0, op_error },
   { "0xdf",   OP_NONE, 0, op_error },
   // Ex
   { "noti",   OP_R_HHLL,   FLAGS_ZN,   op_noti },
   { "not",    OP_R,        FLAGS_ZN,   op_not_r },
   { "not",    OP_R_R,      FLAGS_ZN,   op_not_r2 },
   { "negi",   OP_R_HHLL,   FLAGS_ZN,   op_negi },
   { "neg",    OP_R,        FLAGS_ZN,   op_neg_r },
   { "neg",    OP_R_R,      FLAGS_ZN,   op_neg_r2 },
   { "0xe6",   OP_NONE, 0, op_error },
   { "0xe7",   OP_NONE, 0, op_error },
   { "0xe8",   OP_NONE, 0, op_error },
   { "0xe9",   OP_NONE, 0, op_error },
   { "0xea",   OP_NONE, 0, op_error },
   { "0xeb",   OP_NONE, 0, op_error },
   { "0xec",   OP_NONE, 0, op_error },
   { "0xed",   OP_NONE, 0, op_error },
   { "0xee",   OP_NONE, 0, op_error },
   { "0xef",   OP_NONE, 0, op_error },
   // Fx
   { "0xf0",   OP_NONE, 0, op_error },
   { "0xf1",   OP_NONE, 0, op_error },
   { "0xf2",   OP_NONE, 0, op_error },
   { "0xf3",   OP_NONE, 0, op_error },
   { "0xf4",   OP_NONE, 0, op_error },
   { "0xf5",   OP_NONE, 0, op_error },
   { "0xf6",   OP_NONE, 0, op_error },
   { "0xf7",   OP_NONE, 0, op_error },
   { "0xf8",   OP_NONE, 0, op_error },
   { "0xf9",   OP_NONE, 0, op_error },
   { "0xfa",   OP_NONE, 0, op_error },
   { "0xfb",   OP_NONE, 0, op_error },
   { "0xfc",   OP_NONE, 0, op_error },
   { "0xfd",   OP_NONE, 0, op_error },
   { "0xfe",   OP_NONE, 0, op_error },
   { "0xff",   OP_NONE, 0, op_error },
};

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
    (*state)->rec.bblk_map = reinterpret_cast<cpu_rec_bblk *>(
          calloc(65536, sizeof(cpu_rec_bblk)));
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
    if(!((*state)->vm = reinterpret_cast<uint8_t *>(calloc(320*240,1))))
    {
        fprintf(stderr,"error: calloc failed (state->vm)\n");
        exit(1);
    }
    if(!((*state)->pal = reinterpret_cast<uint32_t *>(malloc(16*sizeof(uint32_t)))))
    {
        fprintf(stderr,"error: malloc failed (state->pal)\n");
        exit(1);
    }
    if(!((*state)->pal_r = reinterpret_cast<uint8_t *>(malloc(16))))
    {
        fprintf(stderr,"error: malloc failed (state->pal_r)\n");
        exit(1);
    }
    if(!((*state)->pal_g = reinterpret_cast<uint8_t *>(malloc(16))))
    {
        fprintf(stderr,"error: malloc failed (state->pal_g)\n");
        exit(1);
    }
    if(!((*state)->pal_b = reinterpret_cast<uint8_t *>(malloc(16))))
    {
        fprintf(stderr,"error: malloc failed (state->pal_b)\n");
        exit(1);
    }
    (*state)->sp = STACK_ADDR;
    memset(&(*state)->f,0,sizeof(flags));
    
    (*state)->prev_rand = opts->rng_seed;

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
    /* Call function pointele entry */
    (*op_table[i_op(state->i)].impl)(state);
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


