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

#ifndef CPU_REC_OPS_H
#define CPU_REC_OPS_H

#include <inttypes.h>

#define ROUNDUP(n,d) ((((n)+(d)-1) / (d)) * (d))

static const uint8_t P_WORD = 0x66;
static const uint8_t REX_W = 0x48;

#define STRUCT_OFFSET(p, m) ((char*)&((p)->m) - (char*)(p))

#define OFFSET(p, n) ((char*)(p) - (char*)((state->rec.jit_p) + (n)))
#define EMIT(b)   (*state->rec.jit_p++ = (b))
#define EMIT2i(w) do { *(int16_t*)state->rec.jit_p = (int16_t)(w); state->rec.jit_p += sizeof(int16_t); } while (0)
#define EMIT2u(w) do { *(uint16_t*)state->rec.jit_p = (uint16_t)(w); state->rec.jit_p += sizeof(uint16_t); } while (0)
#define EMIT4i(dw) do { *(int32_t*)state->rec.jit_p = (int32_t)(dw); state->rec.jit_p += sizeof(int32_t); } while (0)
#define EMIT4u(dw) do { *(uint32_t*)state->rec.jit_p = (uint32_t)(dw); state->rec.jit_p += sizeof(uint32_t); } while (0)
#define EMIT8i(qw) do { *(int64_t*)state->rec.jit_p = (int64_t)(qw); state->rec.jit_p += sizeof(int64_t); } while (0)
#define EMIT8u(qw) do { *(uint64_t*)state->rec.jit_p = (uint64_t)(qw); state->rec.jit_p += sizeof(uint64_t); } while (0)

#define EMIT_REX_RBI(reg,b,i,q) do {\
   uint8_t rex = 0;\
   if (q) rex += 8;\
   if (reg >= 8) rex += 4;\
   if (i >= 8) rex += 2;\
   if (b >= 8) rex += 1;\
   if (rex != 0) EMIT((rex+0x40));\
} while (0);

static uint8_t modrm(uint8_t mod, uint8_t reg, uint8_t rm)
{
   return (mod << 6) | ((reg&7) << 3) | (rm&7);
}

#define MODRM_REG_IMM8(rm) modrm(3, 0, rm)
#define MODRM_REG_OPX_IMM8(op, rm) modrm(3, op, rm)
#define MODRM_RIP_DISP32(reg) modrm(0, reg, 5)
#define MODRM_REG_RMDISP8(reg, rm) modrm(1, reg, rm)
#define MODRM_REG_RMDISP32(reg, rm) modrm(2, reg, rm)
#define MODRM_REG_DIRECT(reg, rm) modrm(3, reg, rm)
#define MODRM_REG_SIB(reg) modrm(0, reg, 4)
#define MODRM_REG_SIB_DISP8(reg) modrm(1, reg, 4)
#define MODRM_REG_SIB_DISP32(reg) modrm(2, reg, 4)

#define SIB(s,i,b) modrm(s,i,b)

static const int REG_NONE = -1;

enum {
   AL = 0, CL, DL, BL, AH, CH, DH, BH,
};

enum {
    AX = 0, CX, DX, BX, SP, BP, SI, DI,
};

enum {
    EAX = 0, ECX, EDX, EBX, ESP, EBP, ESI, EDI,
    E8, E9, E10, E11, E12, E13, E14, E15,
};

enum {
    RAX = 0, RCX, RDX, RBX, RSP, RBP, RSI, RDI,
    R8, R9, R10, R11, R12, R13, R14, R15,
};

enum {
   XMM0 = 0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7,
   XMM8, XMM9, XMM10, XMM11, XMM12, XMM13, XMM14, XMM15,
};

int cpu_rec_hostreg_ptr(cpu_state *state, void* ptr);
int cpu_rec_hostreg_local(cpu_state *state, const char *name);
int cpu_rec_hostreg_c16reg(cpu_state *state, int c16reg);
void cpu_rec_hostreg_freeze(cpu_state *state, int hostreg);
void cpu_rec_hostreg_release(cpu_state *state, int hostreg);
void cpu_rec_hostreg_release_all(cpu_state *state);

#endif

