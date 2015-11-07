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

/*
 * Conventions for JIT register allocation.
 */

#define CACHE_BUFSZ (64*1024)

typedef void (*recblk_fn)(void);

/* (Externally-visible) counter of the number of cycles executed so far. */
uint64_t elapsed_cycles = 0;
int num_buffers = 0;
int alloc_bytes = 0;

/* Entry point to use when entering the next recompiled block. */
static uint8_t* p_entry = NULL;
/* Pointer to the next point in recompiled code buffer to emit to. */
static uint8_t* p_cur = NULL;


void jit_init()
{

}

void jit_block_start()
{
    p_entry = malloc(CACHE_BUFSZ);
    if(!p_entry) {
       fprintf(stderr, "error: could not allocate 64 KiB buffer for JIT cache\n");
       exit(1);
    }
    alloc_bytes += CACHE_BUFSZ;
    num_buffers += 1;
}

void jit_block_end()
{
    // Calculate timing info now the block has been executed
}

void jit_recompile()
{
    jit_block_start();
    p_entry = p_cur;

    jit_block_end();
}

/* Execute next block in recompiled cache. */
void jit_execute()
{
    recblk_fn entry;

    if(!p_entry)
        jit_recompile();

    entry = (recblk_fn) p_entry;
    entry();
}

void jit_alloc_reg()
{
}

void e_nop()
{
    *p_cur++ = 0x90;    // NOP op
}

void e_movrr(uint8_t to, uint8_t from)
{
    *p_cur++ = 0x48 | !!(to & 0x80);                        // REX
    *p_cur++ = 0x89;                                        // MOV r64/r64
    *p_cur++ = 0xc0 | ((to & 0x7f) << 3) | (from & 0x7f);   // ModR/M
}

void e_movrimm(uint8_t to, uint16_t from)
{
    *p_cur++ = 0x66;                                        // Mod: 16-bit opd
    *p_cur++ = 0x40 | !!(to & 0x80);                        // REX
    *p_cur++ = 0xc7;                                        // MOV R64/imm16
    *p_cur++ = 0xc0 | (0 << 3) | (0);                       // ModR/M
    *p_cur++ = (from & 0xff);                               // imm16.lo
    *p_cur++ = (from >> 8);                                 // imm16.hi
}

void e_ret()
{
    *p_cur++ = 0xC3;                                        // RET
}
