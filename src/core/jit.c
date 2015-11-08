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
static int num_instrs = 0;

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

jit_cpu_state s;

void jit_init()
{
    memset(&s, 0, sizeof(jit_cpu_state));
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
    num_instrs = 0;
    p_entry = p_cur = b->p;
}

void jit_block_end()
{
    // Calculate timing info now the block has been executed
}

void jit_recompile()
{
    int i;
    jit_block_start();

    e_mov_r_imm32(rax, 123);
    e_ret();

    jit_block_end();
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
    printf("result before: %d\n", result);
    result = entry();
    printf("result after: %d\n", result);
}

int main(int argc, char *argv[])
{
    jit_execute();
    return 0;
}

void jit_alloc_reg()
{
}

void e_nop()
{
    *p_cur++ = 0x90;    // NOP op
}

void e_mov_r_r(uint8_t to, uint8_t from)
{
    *p_cur++ = REX(1, 0, 0, to & 0x8);        // REX
    *p_cur++ = 0x89;                           // MOV r64/r64
    *p_cur++ = MODRM(3, to, from);             // ModR/M
}

void e_mov_r_imm32(uint8_t to, uint32_t from)
{
    *p_cur++ = 0xb8 + to;                      // MOV R64/imm32
    *(uint32_t *)p_cur = from;               // imm32
    p_cur += sizeof(uint32_t);
}

void e_mov_m64_imm32(uint64_t to, uint32_t from)
{
    *p_cur++ = REX(1, 0, 0, to & 0x8);        // REX
    *p_cur++ = 0xc7;                           // MOV R64/imm16
    *p_cur++ = MODRM(0, 0, disp32);            // ModR/M
    *(uint64_t *)p_cur = to;                 // m64
    p_cur += sizeof(uint64_t);
    *(uint32_t *)p_cur = from;               // imm32
    p_cur += sizeof(uint32_t);
}

void e_mov_r_m64(uint8_t to, uint64_t from)
{
    *p_cur++ = REX(1, 0, 0, to & 0x8);        // REX
    *p_cur++ = 0x8b;                           // MOV R64/m64
    *p_cur++ = MODRM(0, to, disp32);           // ModR/M
    *(uint64_t *)p_cur = from;               // imm64
    p_cur += sizeof(uint64_t);
}

void e_mov_m64_r(uint64_t to, uint8_t from)
{
    *p_cur++ = REX(1, 0, 0, from & 0x8);      // REX
    *p_cur++ = 0x89;                           // MOV m64/R64
    *p_cur++ = MODRM(0, from, disp32);         // ModR/M
    *(uint64_t *)p_cur = to;                 // imm64
    p_cur += sizeof(uint64_t);
}

void e_ret()
{
    *p_cur++ = 0xC3;                                        // RET
}
