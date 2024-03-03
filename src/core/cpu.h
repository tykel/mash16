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

#ifndef CPU_H
#define CPU_H

#include "../consts.h"
#include "../options.h"
#include <SDL2/SDL.h>

#define FLAG_C 2
#define FLAG_Z 4
#define FLAG_O 64
#define FLAG_N 128

#define FLAGS_CZON   (FLAG_C | FLAG_Z | FLAG_O | FLAG_N)
#define FLAGS_CZN   (FLAG_C | FLAG_Z | FLAG_N)
#define FLAGS_ZN     (FLAG_Z | FLAG_N)

#define C_Z  0x0
#define C_NZ 0x1
#define C_N  0x2
#define C_NN 0x3
#define C_P  0x4
#define C_O  0x5
#define C_NO 0x6
#define C_A  0x7
#define C_AE 0x8
#define C_B  0x9
#define C_BE 0xa
#define C_G  0xb
#define C_GE 0xc
#define C_L  0xd
#define C_LE 0xe
#define C_RES 0xf

extern int use_verbose;
extern size_t page_size;

extern const size_t CPU_REC_PAGES;
extern const size_t CPU_TOTAL_PAGES;

/* Instruction representation. */
typedef union
{
    uint32_t dword;
    struct {
        uint8_t op;
        uint8_t yx;
        union {
            uint16_t hhll;
            struct {
                union {
                    uint8_t z;
                    uint8_t n;
                } ub;
                uint8_t res;
            } sw;
        } uw;
    } sdw;
} instr;

/* Macros for reduced tediousness of instruction field accessing. */
#define i_dword(i) ((i).dword)
#define i_op(i) ((i).sdw.op)
#define i_yx(i) ((i).sdw.yx)
#define i_hhll(i) ((i).sdw.uw.hhll)
#define i_z(i) ((i).sdw.uw.sw.ub.z)
#define i_n(i) ((i).sdw.uw.sw.ub.n)
#define i_res(i) ((i).sdw.uw.sw.res)

/* Instruction type (encoding) -- mainly used for disassembler. */
typedef enum
{
    OP_NONE, OP_N, OP_N_N, OP_HHLL, OP_HHLL_HHLL, OP_R, OP_R_N, OP_R_HHLL, OP_R_R_HHLL, OP_SP_HHLL, OP_R_R, OP_R_R_R
} instr_type;

/* Stores CPU flags. */
typedef struct flags
{
    uint8_t c;
    uint8_t z;
    uint8_t o;
    uint8_t n;
} flags;

/* Holds information about the CPU. */
typedef struct cpu_meta
{
    long cycles;
    long target_cycles;
    int wait_vblnk;
    instr_type type;
    uint16_t old_pc;

} cpu_meta;

typedef struct cpu_rec_bblk
{
    void (*code)(void);
    size_t size;
    int cycles;
    uint16_t end_pc;
    bool invalid;
} cpu_rec_bblk;

#define CPU_HOST_REG_VAR         1  // cache a variable from memory
#define CPU_HOST_REG_FROZEN      2  // register not accessible - e.g. RSP, RBP

typedef struct cpu_host_regs_state {
   int use;
   void *ptr;
   size_t size;
   int flags;
   int last_access_time;            // used for eviction
} cpu_host_regs_state;

/* Holds information about the recompiler context. */
typedef struct cpu_rec
{
    /* A 65,536 entry map of Chip16 addresses to JIT blocks. */
    cpu_rec_bblk *bblk_map;

    /* A 8,192 entry map of Chip16 addresses to dirty bits. */
    uint8_t *dirty_map;

    /* Pointer to the next available page for a JIT block to use. */
    void *jit_next_page;

    /* Memory pages mmap()-d for JIT. 4 MiB total. */
    void *jit_base;

    /* Pointer to next position in current JIT block. */
    uint8_t *jit_p;

    /* Start Chip16 PC of the basic block being compiled. */
    uint16_t bblk_pc0;
    /* Current Chip16 PC in the basic block being compiled. */
    uint16_t bblk_pcI;
    /* First Chip16 PC past end of the basic block being compiled. */
    uint16_t bblk_pcN;

    /* Map of Chip16 r0..15 to host x86_64 rax..r15. */
    cpu_host_regs_state host[16];

    /* An integer which increases monotonically with each output JIT
     * instruction -- to decide which reg. to evict, if needed, using LRU. */
    int time;

    /* Flag indicating whether to end JIT block early.
     * Usually set by write between current and end points of JIT block. */
    int bblk_stop;

    int bblk_1per_op;
} cpu_rec;

/* Holds CPU functionality. */
typedef struct cpu_state
{
    /* Pure CPU stuff. */
    int16_t r[16];
    uint16_t pc;
    uint16_t sp;
    instr    i;
    flags    f;
    uint8_t* m;
#ifdef HAVE_BANK_SEL
    uint8_t* mp[256];
#endif
    
    /* Gfx stuff. */
    uint8_t  bgc;
    uint8_t  sw;
    uint8_t  sh;
    uint8_t  fx;
    uint8_t  fy;
    uint32_t* pal;
    uint8_t* pal_r;
    uint8_t* pal_g;
    uint8_t* pal_b;
    uint8_t pal_r0;
    uint8_t pal_g0;
    uint8_t pal_b0;

    uint8_t* vm;

    /* Sfx stuff. */
    uint16_t tone;
    uint8_t  atk;
    uint8_t  dec;
    uint8_t  sus;
    uint8_t  rls;
    uint8_t  vol;
    uint8_t  type;
    
    /* Other */
    uint32_t prev_rand;
    cpu_meta meta;
    cpu_rec rec;
    size_t total_alloc;

} cpu_state;

extern void* global_alloc;
extern size_t page_size;

/* Instruction function pointer table. */
typedef void (*cpu_op)(cpu_state*);

typedef struct {
   const char *name;
   instr_type type;
   int flags;
   cpu_op impl;
} cpu_op_entry;

extern cpu_op_entry op_table[256];

uint32_t mash16_rand(cpu_state *state);

/* CPU functions. */
void cpu_init(cpu_state**,uint8_t*,program_opts*);
void cpu_step(cpu_state*);
void cpu_io_update(SDL_KeyboardEvent*,cpu_state*);
void cpu_io_reset(cpu_state*);
void cpu_free(cpu_state*);

instr_type cpu_op_type(uint8_t);

void cpu_rec_init(cpu_state*, program_opts*);
void cpu_rec_compile(cpu_state*, uint16_t);
void cpu_rec_1bblk(cpu_state*);
void cpu_rec_validate(cpu_state*, uint16_t);
void cpu_rec_free(cpu_state*);
void cpu_rec_dispatch(cpu_state *, uint8_t);

void op_error(cpu_state*);
void op_nop(cpu_state*);
void op_cls(cpu_state*);
void op_vblnk(cpu_state*);
void op_bgc(cpu_state*);
void op_spr(cpu_state*);
void op_drw_imm(cpu_state*);
void op_drw_r(cpu_state*);
int op_drw(uint8_t*,uint8_t*,int,int,int,int,int,int);
void op_rnd(cpu_state*);
void op_flip(cpu_state*);
void op_snd0(cpu_state*);
void op_snd1(cpu_state*);
void op_snd2(cpu_state*);
void op_snd3(cpu_state*);
void op_snp(cpu_state*);
void op_sng(cpu_state*);
void op_jmp_imm(cpu_state*);
void op_jmc(cpu_state*);
void op_jx(cpu_state*);
void op_jme(cpu_state*);
void op_call_imm(cpu_state*);
void op_ret(cpu_state*);
void op_jmp_r(cpu_state*);
void op_cx(cpu_state*);
void op_call_r(cpu_state*);
void op_ldi_r(cpu_state*);
void op_ldi_sp(cpu_state*);
void op_ldm_imm(cpu_state*);
void op_ldm_r(cpu_state*);
void op_mov(cpu_state*);
void op_stm_imm(cpu_state*);
void op_stm_r(cpu_state*);
void op_addi(cpu_state*);
void op_add_r2(cpu_state*);
void op_add_r3(cpu_state*);
void op_subi(cpu_state*);
void op_sub_r2(cpu_state*);
void op_sub_r3(cpu_state*);
void op_cmpi(cpu_state*);
void op_cmp(cpu_state*);
void op_andi(cpu_state*);
void op_and_r2(cpu_state*);
void op_and_r3(cpu_state*);
void op_tsti(cpu_state*);
void op_tst(cpu_state*);
void op_ori(cpu_state*);
void op_or_r2(cpu_state*);
void op_or_r3(cpu_state*);
void op_xori(cpu_state*);
void op_xor_r2(cpu_state*);
void op_xor_r3(cpu_state*);
void op_muli(cpu_state*);
void op_mul_r2(cpu_state*);
void op_mul_r3(cpu_state*);
void op_divi(cpu_state*);
void op_div_r2(cpu_state*);
void op_div_r3(cpu_state*);
void op_modi(cpu_state*);
void op_mod_r2(cpu_state*);
void op_mod_r3(cpu_state*);
void op_remi(cpu_state*);
void op_rem_r2(cpu_state*);
void op_rem_r3(cpu_state*);
void op_shl_n(cpu_state*);
void op_shr_n(cpu_state*);
void op_sar_n(cpu_state*);
void op_shl_r(cpu_state*);
void op_shr_r(cpu_state*);
void op_sar_r(cpu_state*);
void op_push(cpu_state*);
void op_pop(cpu_state*);
void op_pushall(cpu_state*);
void op_popall(cpu_state*);
void op_pushf(cpu_state*);
void op_popf(cpu_state*);
void op_pal_imm(cpu_state*);
void op_pal_r(cpu_state*);
void op_noti(cpu_state*);
void op_not_r(cpu_state*);
void op_not_r2(cpu_state*);
void op_negi(cpu_state*);
void op_neg_r(cpu_state*);
void op_neg_r2(cpu_state*);
/* Generate the flags for the arithmetic instructions. */
void flags_add(int16_t,int16_t,cpu_state*);
void flags_sub(int16_t,int16_t,cpu_state*);
void flags_and(int16_t,int16_t,cpu_state*);
void flags_or(int16_t,int16_t,cpu_state*);
void flags_xor(int16_t,int16_t,cpu_state*);
void flags_mul(int16_t,int16_t,cpu_state*);
void flags_div(int16_t,int16_t,cpu_state*);
void flags_mod(int16_t,int16_t,cpu_state*);
void flags_rem(int16_t,int16_t,cpu_state*);
void flags_shl(int16_t,int16_t,cpu_state*);
void flags_shr(uint16_t,int16_t,cpu_state*);
void flags_sar(int16_t,int16_t,cpu_state*);
void flags_not(int16_t,cpu_state*);
void flags_neg(int16_t,cpu_state*);
/* Test the current jump/call conditional. */
int test_cond(cpu_state*);

#endif

