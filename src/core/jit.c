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
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/mman.h>

#include "../consts.h"
#include "jit.h"
#include "cpu.h"

/* (Externally-visible) counter of the number of cycles executed so far. */
uint64_t elapsed_cycles = 0;
int num_buffers = 0;
int alloc_bytes = 0;

/* Entry point to use when entering the next recompiled block. */
static uint8_t* p_entry = NULL;
/* Pointer to the next point in recompiled code buffer to emit to. */
static uint8_t* p_cur = NULL;
/* Counter for the number of instructions the block contains. */
static int num_insns = 0;
/* Buffer of instruction structs for the block. */
static jit_insn p_insns[16 * 1024];

/* 
 * We keep a copy of the value of rsp at the start of the recompiled block
 * since it is used as a GPR.
 */
static uintptr_t preserved_rsp;

typedef void (*jit_emit_fn)(cpu_state *s, jit_insn *i);
jit_emit_fn jit_e[0x100];

typedef struct jit_buffer
{
    uint8_t *p;
    uint8_t *__unaligned_p;
} jit_buffer;

/* 
 * A Chip16 instruction is 4 bytes.
 * So in the worst case an instruction block can only start in
 * 65536/4 = 16384 different addresses (aligned on a 4-byte boundary).
 */
jit_buffer bufmap[16*1024];

jit_var vars[16];

cpu_state s;

void jit_init()
{
    memset(&s, 0, sizeof(cpu_state));
    // TODO: Populate the jit_emit_fn table!!!
}

void jit_block_start()
{
    jit_buffer *b = &bufmap[s.pc];
    b->__unaligned_p = malloc(CACHE_BUFSZ + PAGESZ);
    if(!b->__unaligned_p) {
       fprintf(stderr, "error: could not allocate 64 KiB for cache\n");
       exit(1);
    }
    // Align the buffer to the nearest page boundary
    b->p = (uint8_t*)
        ((uint64_t)(b->__unaligned_p + PAGESZ - 1) & ~(PAGESZ - 1));
    alloc_bytes += CACHE_BUFSZ;
    num_buffers += 1;
    num_insns = 0;
    p_entry = p_cur = b->p;
}

void jit_block_end()
{
    // Calculate timing info now the block has been executed
}

int jit_decode(uint8_t *p)
{
    return 0;
}

int f_test(void) { return 321; }

uint64_t d64 = 0xffffffffffffffff;
uint64_t c64 = 0x6464646464646464;
uint32_t d32 = 0xffffffff;
uint32_t c32 = 0x32323232;
uint16_t d16 = 0xffff;
uint16_t c16 = 0x1616;

void jit_recompile()
{
    int i;

    jit_block_start();

    jit_decode(s.m + s.pc);
    jit_regs_alloc(p_insns, num_insns);
    // Save the stack pointer so we can return 
    //e_mov_m64_r((uint64_t)&preserved_rsp, rsp);
    //for(i = 0; i < num_insns; i++) {
    //    jit_e[p_insns[i].op](&s, &p_insns[i]);
    //}
    // Restore the stack pointer
    //e_mov_r_m64(rsp, (uint64_t)&preserved_rsp);

    e_mov_r_m64(r8, (uint64_t)&d64);
    e_mov_r_m32(r8, &d32);
    e_mov_r_m16(r8, &d16);
    e_ret();

    jit_block_end();
    
    for(i = 0; i < (int)(p_cur - p_entry); i++)
        printf("%02x ", p_entry[i]);
    printf("\n");

    // Set PROT_EXEC on the memory pages so we can execute them.
    if(mprotect(p_entry, CACHE_BUFSZ, PROT_READ | PROT_EXEC) != 0) {
        fprintf(stderr, "error: failed to make JIT cache executable"
                        "(errno: %d)\n", errno);
        exit(1);
    }
}

/* Execute next block in recompiled cache. */
void jit_execute()
{
    recblk_fn entry;
    int result = 0;

    if(!p_entry)
        jit_recompile();

    entry = (recblk_fn) p_entry;
    // Begin executing code in JIT buffer.
    result = entry();
}

int main(int argc, char *argv[])
{
    jit_execute();
    return 0;
}

void jit_inc_vages()
{
    int i;

    for(i = 0; i < 16; i++) {
        if(vars[i].in_reg)
            vars[i].age++;
        else
            // Assume we allocate registers in order from 0..15
            return;
    }
}

jit_var* jit_get_loc(uint16_t *p)
{
    int i;
    int age, ei;

    for(i = 0; i < 16; i++) {
        if(vars[i].p == p) {
            return &vars[i];
        }
        // Unused register: make p use it and reset age
        if(!vars[i].in_reg) {
            vars[i].in_reg = 1;
            vars[i].p = p;
            vars[i].age = 0;
            return &vars[i];
        }
    }
    // p is not in a register: evict something and take its place
    for(i = 0, age = -1, ei = 0; i < 16; i++) {
        // Use a simple LRU scheme
        if(vars[i].age > age) {
            age = vars[i].age;
            ei = i;
        }
    }
    // Emit code to store the evicted register
    e_mov_m64_r((uint64_t)vars[ei].p, vars[ei].r);
    // Set the new pointer and reset age
    vars[ei].p = p;
    vars[ei].age = 0;
    vars[ei].in_reg = 1;
    return NULL;
}

void e_nop()
{
    *p_cur++ = 0x90;    // NOP op
}

void e_lea_r_m64(uint8_t to, uintptr_t from)
{
    int32_t dest = (int32_t)((uint64_t)from - (uint64_t)(p_cur + 6 + !!(to & 0x8)));
    if(to & 0x8) *p_cur++ = REX(0, 0, 0, 1);
    *p_cur++ = 0x8d;
    *p_cur++ = MODRM(0, 0, disp32);
    *(int32_t *)p_cur = dest;
    p_cur += sizeof(int32_t);
}

void e_mov_r_r(uint8_t to, uint8_t from)
{
    *p_cur++ = REX(1, 0, 0, to & 0x8);        // REX
    *p_cur++ = 0x89;                           // MOV r64/r64
    *p_cur++ = MODRM(3, to, from);             // ModR/M
}

void e_mov_r_imm8(uint8_t to, uint8_t from)
{
    if(to & 0x8) *p_cur++ = REX(0, 0, 0, to & 0x8);
    *p_cur++ = 0xb0 + (to & 0x7);
    *p_cur++ = from;
}

void e_mov_r_imm16(uint8_t to, uint16_t from)
{
    *p_cur++ = 0x66;
    if(to & 0x8) *p_cur++ = REX(0, 0, 0, to & 0x8);
    *p_cur++ = 0xb8 + (to & 0x7);
    *(uint16_t *)p_cur = from;
    p_cur += sizeof(uint16_t);
}

void e_mov_r_imm32(uint8_t to, uint32_t from)
{
    if(to & 0x8) *p_cur++ = REX(0, 0, 0, to & 0x8);
    *p_cur++ = 0xb8 + (to & 0x7);            // MOV R64/imm32
    *(uint32_t *)p_cur = from;               // imm32
    p_cur += sizeof(uint32_t);
}

void e_mov_m64_imm32(uint32_t *to, uint32_t from)
{
    int32_t dest = (int32_t)((uint64_t)to - ((uint64_t)p_cur + 10));
    *p_cur++ = 0xc7;                           // MOV R64/imm16
    *p_cur++ = MODRM(0, 0, disp32);            // ModR/M
    *(int32_t *)p_cur = dest;                 // m64
    p_cur += sizeof(uint32_t);
    *(uint32_t *)p_cur = from;               // imm32
    p_cur += sizeof(uint32_t);
}

void e_mov_m64_imm16(uint16_t *to, uint16_t from)
{
    int32_t dest = (int32_t)((uint64_t)to - ((uint64_t)p_cur + 9));
    *p_cur++ = 0x66;
    *p_cur++ = 0xc7;
    *p_cur++ = MODRM(0, 0, disp32);
    *(int32_t *)p_cur = dest;
    p_cur += sizeof(uint32_t);
    *(uint16_t *)p_cur = from;
    p_cur += sizeof(uint16_t);
}

void e_mov_r_m8(uint8_t to, uint8_t *from)
{
    int32_t dest = (int32_t)((uint64_t)from -
                             ((uint64_t)p_cur + 6 + !!(to & 0x80)));
    if(to & 0x8) *p_cur++ = REX(0, 1, 0, 0);
    *p_cur++ = 0x8a;                           // MOV R64/m64
    *p_cur++ = MODRM(0, to, disp32);           // ModR/M
    *(int32_t *)p_cur = dest;               // imm32
    p_cur += sizeof(uint32_t);
}

void e_mov_r_m16(uint8_t to, uint16_t *from)
{
    int32_t dest = (int32_t)((uint64_t)from -
                             ((uint64_t)p_cur + 7 + !!(to & 0x80)));
    *p_cur++ = 0x66;
    if(to & 0x8) *p_cur++ = REX(0, 1, 0, 0);
    *p_cur++ = 0x8b;                           // MOV R64/m64
    *p_cur++ = MODRM(0, to, disp32);           // ModR/M
    *(int32_t *)p_cur = dest;               // imm32
    p_cur += sizeof(uint32_t);
}

void e_mov_r_m32(uint8_t to, uint32_t *from)
{
    int32_t dest = (int32_t)((uint64_t)from - 
                             ((uint64_t)p_cur + 6 + !!(to & 0x80)));
    if(to & 0x8) *p_cur++ = REX(0, 1, 0, 0);
    *p_cur++ = 0x8b;                           // MOV R64/m64
    *p_cur++ = MODRM(0, to, disp32);           // ModR/M
    *(int32_t *)p_cur = dest;               // imm32
    p_cur += sizeof(uint32_t);
}

void e_mov_r_m64(uint8_t to, uint64_t *from)
{
    int32_t dest = (int32_t)((uint64_t)from -
                             ((uint64_t)p_cur + 6 + !!(to + 0x80)));
    if(to & 0x8) *p_cur++ = REX(1, 1, 0, 0);
    *p_cur++ = 0x8b;                           // MOV R64/m64
    *p_cur++ = MODRM(0, to, disp32);           // ModR/M
    *(int32_t *)p_cur = dest;               // imm32
    p_cur += sizeof(uint32_t);
}

void e_mov_m8_r(uint8_t* to, uint8_t from)
{
    int32_t dest = (int32_t)((uint64_t)to - ((uint64_t)p_cur + 6 + !!(from & 0x80)));
    if(from & 0x8) *p_cur++ = REX(0, 1, 0, 0);      // REX
    *p_cur++ = 0x88;                           // MOV m64/R64
    *p_cur++ = MODRM(0, from, disp32);         // ModR/M
    *(int32_t *)p_cur = dest;               // imm32
    p_cur += sizeof(int32_t);
}

void e_mov_m16_r(uint16_t *to, uint8_t from)
{
    int32_t dest = (int32_t)((uint64_t)to - ((uint64_t)p_cur + 6 + !!(from & 0x80)));
    *p_cur++ = 0x66;
    if(from & 0x8) *p_cur++ = REX(0, 1, 0, 0);      // REX
    *p_cur++ = 0x8b;                           // MOV m64/R64
    *p_cur++ = MODRM(0, from, disp32);         // ModR/M
    *(int32_t *)p_cur = dest;               // imm32
    p_cur += sizeof(int32_t);
}

void e_mov_m64_r(uint64_t* to, uint8_t from)
{
    int32_t dest = (int32_t)((uint64_t)to - ((uint64_t)p_cur + 6 + !!(from & 0x80)));
    if(from & 0x8) *p_cur++ = REX(1, 1, 0, 0);      // REX
    *p_cur++ = 0x89;                           // MOV m64/R64
    *p_cur++ = MODRM(0, from, disp32);         // ModR/M
    *(int32_t *)p_cur = dest;               // imm32
    p_cur += sizeof(int32_t);
}

void e_call(uintptr_t addr)
{
    int32_t dest = (int32_t)((uint64_t)addr - ((uint64_t)p_cur + 5));
    *p_cur++ = 0xe8;                            // CALL
    *(int32_t *)p_cur = dest;                  // disp32
    p_cur += sizeof(int32_t);
}

void e_ret()
{
    *p_cur++ = 0xC3;                                        // RET
}

void e_and_r_imm32(uint8_t to, uint32_t from)
{
    if(to & 0x8) *p_cur++ = REX(1, 0, 0, 1);
    *p_cur++ = 0x81;
    *p_cur++ = MODRM(3, 4, to);
    *(uint32_t *)p_cur = from;
    p_cur += sizeof(uint32_t);
}

void e_push(uint8_t from)
{
    if(from & 0x8) *p_cur++ = REX(0, 0, 0, 1);
    *p_cur++ = 0x50 + (from & 0x7);
}

void e_pop(uint8_t to)
{
    if(to & 0x8) *p_cur++ = REX(0, 0, 0, 1);
    *p_cur++ = 0x58 + (to & 0x7);
}
