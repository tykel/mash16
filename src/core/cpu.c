#include "cpu.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/* Initialise the CPU to safe values. */
void cpu_init(cpu_state* state)
{
    state = (cpu_state*)calloc(sizeof(state),1);
    state->m = calloc(sizeof(uint8_t),MEM_SIZE);
    state->vm = calloc(sizeof(uint8_t),160*240);

    srand(time(NULL));

    /* Map instr. table entries to functions. */
    memset(op_table,(size_t)(&op_error),0x100);
    op_table[0x00] = &op_nop;
    op_table[0x01] = &op_cls;
    op_table[0x02] = &op_vblnk;
    op_table[0x03] = &op_bgc;
    op_table[0x04] = &op_spr;
    op_table[0x05] = &op_drw_imm;
    op_table[0x06] = &op_drw_r;
    op_table[0x07] = &op_rnd;
    op_table[0x08] = &op_flip;
    op_table[0x09] = &op_snd0;
    op_table[0x0a] = &op_snd1;
    op_table[0x0b] = &op_snd2;
    op_table[0x0c] = &op_snd3;
    op_table[0x0d] = &op_snp;
    op_table[0x0e] = &op_sng;
    op_table[0x10] = &op_jmp_imm;
    op_table[0x12] = &op_jx;
    op_table[0x13] = &op_jme;
    op_table[0x14] = &op_call_imm;
    op_table[0x15] = &op_ret;
    op_table[0x16] = &op_jmp_r;
    op_table[0x17] = &op_cx;
    op_table[0x18] = &op_call_r;
    op_table[0x20] = &op_ldi_r;
    op_table[0x21] = &op_ldi_sp;
    op_table[0x22] = &op_ldm_imm;
    op_table[0x23] = &op_ldm_r;
    op_table[0x24] = &op_mov;
    op_table[0x30] = &op_stm_imm;
    op_table[0x31] = &op_stm_r;
    op_table[0x40] = &op_addi;
    op_table[0x41] = &op_add_r2;
    op_table[0x42] = &op_add_r3;
    op_table[0x50] = &op_subi;
    op_table[0x51] = &op_sub_r2;
    op_table[0x52] = &op_sub_r3;
    op_table[0x53] = &op_cmpi;
    op_table[0x54] = &op_cmp;
    op_table[0x60] = &op_andi;
    op_table[0x61] = &op_and_r2;
    op_table[0x62] = &op_and_r3;
    op_table[0x63] = &op_tsti;
    op_table[0x64] = &op_tst;
    op_table[0x70] = &op_ori;
    op_table[0x71] = &op_or_r2;
    op_table[0x72] = &op_or_r3;
    op_table[0x80] = &op_xori;
    op_table[0x81] = &op_xor_r2;
    op_table[0x82] = &op_xor_r3;
    op_table[0x90] = &op_muli;
    op_table[0x91] = &op_mul_r2;
    op_table[0x92] = &op_mul_r3;
    op_table[0xa0] = &op_divi;
    op_table[0xa1] = &op_div_r2;
    op_table[0xa2] = &op_div_r3;
    op_table[0xb0] = &op_shl_n;
    op_table[0xb1] = &op_shr_n;
    op_table[0xb2] = &op_sar_n;
    op_table[0xb3] = &op_shl_r;
    op_table[0xb4] = &op_shr_r;
    op_table[0xb5] = &op_sar_r;
    op_table[0xc0] = &op_push;
    op_table[0xc1] = &op_pop;
    op_table[0xc2] = &op_pushall;
    op_table[0xc3] = &op_popall;
    op_table[0xc4] = &op_pushf;
    op_table[0xc5] = &op_popf;
    op_table[0xd0] = &op_pal_imm;
    op_table[0xd1] = &op_pal_r;
}

/* Execute 1 CPU cycle. */
void cpu_step(cpu_state* state)
{
    state->pc += 4;
    state->i = *(instr*)(&state->m[state->pc]);
    /* Call function ptr table entry */
    ++state->meta.cycles;
}

void op_error(cpu_state* state)
{
    fprintf(stderr,"Unknown opcode encountered! (0x%x)\n",state->i.op);
}

void op_nop(cpu_state* state)
{
}

void op_cls(cpu_state* state)
{
    memset(state->vm,(state->bgc << 4) & state->bgc,160*240);
}

void op_vblnk(cpu_state* state)
{
    state->meta.wait_vblnk = 1;
}

void op_bgc(cpu_state* state)
{
    state->bgc = state->i.n;
}

void op_spr(cpu_state* state)
{
    state->sw = state->i.hhll & 0x0f;
    state->sh = state->i.hhll >> 4;
}

void op_drw_imm(cpu_state* state)
{

}

void op_drw_r(cpu_state* state)
{
}

void op_rnd(cpu_state* state)
{
    state->r[state->i.yx & 0x0f] = rand() & state->i.hhll;
}

void op_flip(cpu_state* state)
{
}

void op_snd0(cpu_state* state)
{
}

void op_snd1(cpu_state* state)
{
}

void op_snd2(cpu_state* state)
{
}

void op_snd3(cpu_state* state)
{
}

void op_snp(cpu_state* state)
{
}

void op_sng(cpu_state* state)
{
    state->atk = state->i.yx >> 4;
    state->dec = state->i.yx & 0x0f;
    state->sus = (state->i.hhll >> 12) & 0x0f;
    state->rls = (state->i.hhll >> 8) & 0x0f;
    state->vol = (state->i.hhll >> 4) & 0x0f;
    state->type = state->i.hhll & 0x0f;
}

void op_jmp_imm(cpu_state* state)
{
    state->pc = state->i.hhll;
}

void op_jx(cpu_state* state)
{
    
}

void op_jme(cpu_state* state)
{
}

void op_call_imm(cpu_state* state)
{
    state->m[state->sp] = state->pc;
    state->sp += 2;
    state->pc = state->i.hhll;
}

void op_ret(cpu_state* state)
{
    state->sp -= 2;
    state->pc = state->m[state->sp];
}

void op_jmp_r(cpu_state* state)
{
    state->pc = state->r[state->i.yx & 0x0f];
}

void op_cx(cpu_state* state)
{
}

void op_call_r(cpu_state* state)
{
    state->m[state->sp] = state->pc;
    state->sp += 2;
    state->pc = state->r[state->i.yx & 0x0f];
}

void op_ldi_r(cpu_state* state)
{
    state->r[state->i.yx & 0x0f] = (int16_t)state->i.hhll;
}

void op_ldi_sp(cpu_state* state)
{
    state->sp = state->i.hhll;
}

void op_ldm_imm(cpu_state* state)
{
    state->r[state->i.yx & 0x0f] = state->m[state->i.hhll];  
}

void op_ldm_r(cpu_state* state)
{
    state->r[state->i.yx & 0x0f] = state->m[state->r[state->i.yx >> 4]];
}

void op_mov(cpu_state* state)
{
    state->r[state->i.yx & 0x0f] = state->r[state->i.yx >>4];
}

void op_stm_imm(cpu_state* state)
{
    state->m[state->i.hhll] = state->r[state->i.yx & 0x0f];
}

void op_stm_r(cpu_state* state)
{
    state->m[state->r[state->i.yx >> 4]] = state->r[state->i.yx & 0x0f];
}

void op_addi(cpu_state* state)
{
    int16_t* r = &state->r[state->i.yx & 0x0f];
    int16_t imm = (int16_t)state->i.hhll;
    flags_add(*r,imm);
    *r = imm;
}

void op_add_r2(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_add(*rx,ry);
    *rx = ry;
}

void op_add_r3(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_add(rx,ry);
    state->r[state->i.z] = rx + ry;
}

void op_subi(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t imm = state->i.hhll;
    flags_sub(*rx,imm);
    *rx -= imm;
}

void op_sub_r2(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_sub(*rx,ry);
    *rx -= ry;
}

void op_sub_r3(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_sub(rx,ry);
    state->r[state->i.z] = rx - ry;
}

void op_cmpi(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t imm = state->i.hhll;
    flags_sub(rx,imm);
}

void op_cmp(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_sub(rx,ry);
}

void op_andi(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t imm = state->i.hhll;
    flags_and(*rx,imm);
    *rx &= imm;
}

void op_and_r2(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_and(*rx,ry);
    *rx &= ry;
}

void op_and_r3(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_sub(rx,ry);
    state->r[state->i.z] = rx & ry;
}

void op_tsti(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t imm = state->i.hhll;
    flags_and(rx,imm);
}

void op_tst(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_and(rx,ry);
}

void op_ori(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t imm = state->i.hhll;
    flags_or(*rx,imm);
    *rx |= imm;
}

void op_or_r2(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_or(*rx,ry);
    *rx |= ry;
}

void op_or_r3(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_or(rx,ry);
    state->r[state->i.z] = rx | ry;
}

void op_xori(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t imm = state->i.hhll;
    flags_xor(*rx,imm);
    *rx ^= imm;
}

void op_xor_r2(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_sub(*rx,ry);
    *rx ^= ry;
}

void op_xor_r3(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_sub(rx,ry);
    state->r[state->i.z] = rx ^ ry;
}

void op_muli(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t imm = state->i.hhll;
    flags_mul(*rx,imm);
    *rx *= imm;
}

void op_mul_r2(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t ry = state->i.hhll;
    flags_mul(*rx,ry);
    *rx *= ry;
}

void op_mul_r3(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_mul(rx,ry);
    state->r[state->i.z] = rx * ry;
}

void op_divi(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t imm = state->i.hhll;
    flags_div(*rx,imm);
    *rx /= imm;
}

void op_div_r2(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_div(*rx,ry);
    *rx /= ry;
}

void op_div_r3(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_div(rx,ry);
    state->r[state->i.z] = rx / ry;
}

void op_shl_n(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t n = state->i.n;
    flags_shl(*rx,n);
    *rx <<= n;
}

void op_shr_n(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t n = state->i.n;
    flags_shr(*rx,n);
    *rx >>= n;
}

void op_sar_n(cpu_state* state)
{
}

void op_shl_r(cpu_state* state)
{
}

void op_shr_r(cpu_state* state)
{
}

void op_sar_r(cpu_state* state)
{
}

void op_push(cpu_state* state)
{
    state->m[state->sp] = state->r[state->i.yx & 0x0f];
    state->sp += 2;
}

void op_pop(cpu_state* state)
{
    state->sp -= 2;
    state->r[state->i.yx & 0x0f] = state->m[state->sp];
}

void op_pushall(cpu_state* state)
{
    for(int i=0; i<0x10; ++i)
    {
        state->m[state->sp] = state->r[i];
        state->sp += 2;
    }
}

void op_popall(cpu_state* state)
{
    for(int i=0xf; i<=0; --i)
    {
        state->sp -= 2;
        state->r[i] = state->m[state->sp];
    }
}

void op_pushf(cpu_state* state)
{
    state->m[state->sp] = state->flags;
    state->sp += 2;
}

void op_popf(cpu_state* state)
{
    state->sp -= 2;
    state->flags = state->m[state->sp];
}

void op_pal_imm(cpu_state* state)
{
}

void op_pal_r(cpu_state* state)
{
}

