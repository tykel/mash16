#include "../consts.h"
#include "cpu.h"
#include "gpu.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/* Initialise the CPU to safe values. */
void cpu_init(cpu_state** state, uint8_t* mem)
{
    *state = (cpu_state*)calloc(1,sizeof(cpu_state));
    (*state)->m = mem;
    (*state)->vm = calloc(160*240,1);
    (*state)->pal = malloc(16*sizeof(uint32_t));
    (*state)->sp = STACK_ADDR;
    
    srand(time(NULL));

    /* Map instr. table entries to functions. */
    for(int i=0; i<0x100; ++i)
        op_table[i] = &op_error;
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

    /* Load default palette. */
    init_pal(*state);
}

/* Execute 1 CPU cycle. */
void cpu_step(cpu_state* state)
{
    /* Fetch instruction, increase PC. */
    state->i = *(instr*)(&state->m[state->pc]);
    state->pc += 4;
    /* Call function ptr table entry */
    (*op_table[state->i.op])(state);
    /* Update cycles. */
    ++state->meta.cycles;
}

/* Update I/O port contents with gamepad input. */
void cpu_io_update(SDL_KeyboardEvent* key, cpu_state* state)
{
    switch(key->keysym.sym)
    {
        case SDLK_UP:
            state->m[IO_PAD1_ADDR] |= PAD_UP;
            break;
        case SDLK_DOWN:
            state->m[IO_PAD1_ADDR] |= PAD_DOWN;
            break;
        case SDLK_LEFT:
            state->m[IO_PAD1_ADDR] |= PAD_LEFT;
            break;
        case SDLK_RIGHT:
            state->m[IO_PAD1_ADDR] |= PAD_RIGHT;
            break;
        case SDLK_RSHIFT:
            state->m[IO_PAD1_ADDR] |= PAD_SELECT;
            break;
        case SDLK_RETURN:
            state->m[IO_PAD1_ADDR] |= PAD_START;
            break;
        case SDLK_z:
            state->m[IO_PAD1_ADDR] |= PAD_A;
            break;
        case SDLK_x:
            state->m[IO_PAD1_ADDR] |= PAD_B;
            break;
        default:
            break;
    }
}

/* Free resources held by the cpu state. */
void cpu_free(cpu_state* state)
{
    free(state->vm);
    free(state);
}

/* CPU instructions. */

void op_error(cpu_state* state)
{
    fprintf(stderr,"error: unknown opcode encountered! (0x%x)\n",state->i.op);
}

void op_nop(cpu_state* state)
{
}

void op_cls(cpu_state* state)
{
    memset(state->vm,0,160*240);
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
    state->sw = state->i.hhll & 0x00ff;
    state->sh = state->i.hhll >> 8;
}

void op_drw_imm(cpu_state* state)
{
    /* If width=0 or height=0, nothing to draw. */
    if(!state->sw || !state->sh)
        return;
    int16_t x = state->r[state->i.yx & 0x0f];
    int16_t y = state->r[state->i.yx >> 4];
    int w = state->sw > 160 ? state->sw - (state->sw % 160) : state->sw;
    int h = state->sh > 240 ? state->sh - (state->sh % 240) : state->sh;
    /* Check we actually need to draw something. */
    if(!w || !h || x > 319 || y > 239 || x + w*2 < 0 || y + h < 0)
        return;
    uint8_t* dbpx = &state->m[state->i.hhll];
    /* Copy sprite data to (chip16) video memory. */
    for(int iy=0; iy<h; ++iy)
    {
        for(int ix=0; ix<w; ++ix, ++dbpx)
        {
            uint8_t* vmp = &(state->vm[(y+iy)*160 + (x+ix)]);
            uint8_t lp = ((*dbpx & 0x0f) == 0) ? (*vmp & 0x0f) : (*dbpx & 0x0f);
            uint8_t hp = ((*dbpx >>   4) == 0) ? (*vmp >>   4) : (*dbpx >>   4);
            *vmp = (hp << 4) | lp;
        }
    }
}

void op_drw_r(cpu_state* state)
{
    /* If width=0 or height=0, nothing to draw. */
    if(!state->sw || !state->sh)
        return;
    int16_t x = state->r[state->i.yx & 0x0f];
    int16_t y = state->r[state->i.yx >> 4];
    int w = state->sw > 160 ? state->sw - (state->sw % 160) : state->sw;
    int h = state->sh > 240 ? state->sh - (state->sh % 240) : state->sh;
    /* If off-screen, nothing to draw. */
    if(w || !h || x > 319 || y > 239 || x + w*2 < 0 || y + h < 0)
        return;
    uint8_t* dbpx = &state->m[state->r[state->i.z]];
    /* Copy sprite data to (chip16) video memory. */
    for(int iy=0; iy<h; ++iy)
    {
        for(int ix=0; ix<w; ++ix, ++dbpx)
        {
            uint8_t* vmp = &(state->vm[(y+iy)*160 + (x+ix)]);
            uint8_t lp = ((*dbpx & 0x0f) == 0) ? (*vmp & 0x0f) : (*dbpx & 0x0f);
            uint8_t hp = ((*dbpx >>   4) == 0) ? (*vmp >>   4) : (*dbpx >>   4);
            *vmp = (hp << 4) | lp;
        }
    }
}

void op_rnd(cpu_state* state)
{
    state->r[state->i.yx & 0x0f] = rand() & state->i.hhll;
}

void op_flip(cpu_state* state)
{
    state->fx = (state->i.hhll & 0x02) >> 1;
    state->fy = state->i.hhll & 0x01;
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
    if(test_cond(state))
    {
        state->pc = state->i.hhll;
    }
}

void op_jme(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    if(rx == ry)
        state->pc = state->i.hhll;
}

void op_call_imm(cpu_state* state)
{
    state->m[state->sp] = state->pc & 0x00ff;
    state->m[state->sp + 1] = state->pc >> 8;
    state->sp += 2;
    state->pc = state->i.hhll;
}

void op_ret(cpu_state* state)
{
    state->sp -= 2;
    state->pc = state->m[state->sp] | (state->m[state->sp + 1] << 8);
}

void op_jmp_r(cpu_state* state)
{
    state->pc = state->r[state->i.yx & 0x0f];
}

void op_cx(cpu_state* state)
{
    if(test_cond(state))
    {
        state->m[state->sp] = state->pc & 0x00ff;
        state->m[state->sp+1] = state->pc >> 8;
        state->sp += 2;
        state->pc = state->i.hhll;
    }
}

void op_call_r(cpu_state* state)
{
    state->m[state->sp] = state->pc & 0x00ff;
    state->m[state->sp+1] = state->pc >> 8;
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
    state->r[state->i.yx & 0x0f] = state->r[state->i.yx >> 4];
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
    flags_add(*r,imm,state);
    *r += imm;
}

void op_add_r2(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_add(*rx,ry,state);
    *rx += ry;
}

void op_add_r3(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_add(rx,ry,state);
    state->r[state->i.z] = rx + ry;
}

void op_subi(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t imm = state->i.hhll;
    flags_sub(*rx,imm,state);
    *rx -= imm;
}

void op_sub_r2(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_sub(*rx,ry,state);
    *rx -= ry;
}

void op_sub_r3(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_sub(rx,ry,state);
    state->r[state->i.z] = rx - ry;
}

void op_cmpi(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t imm = state->i.hhll;
    flags_sub(rx,imm,state);
}

void op_cmp(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_sub(rx,ry,state);
}

void op_andi(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t imm = state->i.hhll;
    flags_and(*rx,imm,state);
    *rx &= imm;
}

void op_and_r2(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_and(*rx,ry,state);
    *rx &= ry;
}

void op_and_r3(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_sub(rx,ry,state);
    state->r[state->i.z] = rx & ry;
}

void op_tsti(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t imm = state->i.hhll;
    flags_and(rx,imm,state);
}

void op_tst(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_and(rx,ry,state);
}

void op_ori(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t imm = state->i.hhll;
    flags_or(*rx,imm,state);
    *rx |= imm;
}

void op_or_r2(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_or(*rx,ry,state);
    *rx |= ry;
}

void op_or_r3(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_or(rx,ry,state);
    state->r[state->i.z] = rx | ry;
}

void op_xori(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t imm = state->i.hhll;
    flags_xor(*rx,imm,state);
    *rx ^= imm;
}

void op_xor_r2(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_sub(*rx,ry,state);
    *rx ^= ry;
}

void op_xor_r3(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_sub(rx,ry,state);
    state->r[state->i.z] = rx ^ ry;
}

void op_muli(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t imm = state->i.hhll;
    flags_mul(*rx,imm,state);
    *rx *= imm;
}

void op_mul_r2(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t ry = state->i.hhll;
    flags_mul(*rx,ry,state);
    *rx *= ry;
}

void op_mul_r3(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_mul(rx,ry,state);
    state->r[state->i.z] = rx * ry;
}

void op_divi(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t imm = state->i.hhll;
    flags_div(*rx,imm,state);
    *rx /= imm;
}

void op_div_r2(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_div(*rx,ry,state);
    *rx /= ry;
}

void op_div_r3(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_div(rx,ry,state);
    state->r[state->i.z] = rx / ry;
}

void op_shl_n(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t n = state->i.n;
    flags_shl(*rx,n,state);
    *rx <<= n;
}

void op_shr_n(cpu_state* state)
{
    uint16_t* rx = (uint16_t*)&state->r[state->i.yx & 0x0f];
    int16_t n = state->i.n;
    flags_shr(*rx,n,state);
    *rx >>= n;
}

void op_sar_n(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t n = state->i.n;
    flags_shr(*rx,n,state);
    *rx >>= n;
}

void op_shl_r(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_shl(*rx,ry,state);
    *rx <<= ry;
}

void op_shr_r(cpu_state* state)
{
    uint16_t* rx = (uint16_t*)&state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_shr(*rx,ry,state);
    *rx >>= ry;
}

void op_sar_r(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_shr(*rx,ry,state);
    *rx >>= ry;
}

void op_push(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    state->m[state->sp] = rx & 0x00ff;
    state->m[state->sp + 1] = rx >> 8;
    state->sp += 2;
}

void op_pop(cpu_state* state)
{
    state->sp -= 2;
    state->r[state->i.yx & 0x0f] = state->m[state->sp] | (state->m[state->sp + 1] << 8);
}

void op_pushall(cpu_state* state)
{
    for(int i=0; i<0x10; ++i)
    {
        state->m[state->sp] = state->r[i] & 0x0f;
        state->m[state->sp + 1] = state->r[i] >> 8;
        state->sp += 2;
    }
}

void op_popall(cpu_state* state)
{
    for(int i=0xf; i<=0; --i)
    {
        state->sp -= 2;
        state->r[i] = state->m[state->sp] | (state->m[state->sp + 1] << 8);
    }
}

void op_pushf(cpu_state* state)
{
    state->m[state->sp] = state->flags & 0x0f;
    state->m[state->sp + 1] = state->flags >> 8;
    state->sp += 2;
}

void op_popf(cpu_state* state)
{
    state->sp -= 2;
    state->flags = state->m[state->sp] | (state->m[state->sp + 1] << 8);
}

void op_pal_imm(cpu_state* state)
{
    load_pal(&state->m[state->i.hhll],0,state);
}

void op_pal_r(cpu_state* state)
{
    load_pal(&state->m[state->r[state->i.yx & 0x0f]],0,state);
}

/* Flag computing functions. */

void flags_add(int16_t x, int16_t y, cpu_state* state)
{
    state->flags = 0;
    int32_t res = (int32_t)x + (int32_t)y;
    if(!res)
        state->flags |= FLAG_Z;
    if(res < INT16_MIN || res > INT16_MAX)
        state->flags |= FLAG_C;
    if(((int16_t)res < 0 && (int16_t)x > 0 && (int16_t)y > 0) ||
        ((int16_t)res > 0 && (int16_t)x < 0 && (int16_t)y < 0))
        state->flags |= FLAG_O;
    if(res < 0)
        state->flags |= FLAG_N;
}

void flags_sub(int16_t x, int16_t y, cpu_state* state)
{
    state->flags = 0;
    int32_t res = (int32_t)x - (int32_t)y;
    if(!res)
        state->flags |= FLAG_Z;
    if(res < INT16_MIN || res > INT16_MAX)
        state->flags |= FLAG_C;
    if(((int16_t)res < 0 && (int16_t)x > 0 && (int16_t)y < 0) || 
        ((int16_t)res > 0 && x < 0 && y > 0))
        state->flags |= FLAG_O;
    if(res < 0)
        state->flags |= FLAG_N;
}

void flags_and(int16_t x, int16_t y, cpu_state* state)
{
    state->flags = 0;
    int16_t res = x & y;
    if(!res)
        state->flags |= FLAG_Z;
    if(res < 0)
        state->flags |= FLAG_N;
}

void flags_or(int16_t x, int16_t y, cpu_state* state)
{
    state->flags = 0;
    int16_t res = x | y;
    if(!res)
        state->flags |= FLAG_Z;
    if(res < 0)
        state->flags |= FLAG_N;
}

void flags_xor(int16_t x, int16_t y, cpu_state* state)
{
    state->flags = 0;
    int16_t res = x ^ y;
    if(!res)
        state->flags |= FLAG_Z;
    if(res < 0)
        state->flags |= FLAG_N;
}

void flags_mul(int16_t x, int16_t y, cpu_state* state)
{
    state->flags = 0;
    int32_t res = (int32_t)x * (int32_t)y;
    if(!res)
        state->flags |= FLAG_Z;
    if(res > INT16_MAX || res < INT16_MIN)
        state->flags |= FLAG_C;
    if(res < 0)
        state->flags |= FLAG_N;
}

void flags_div(int16_t x, int16_t y, cpu_state* state)
{
    state->flags = 0;
    int32_t res = (int32_t)x / (int32_t)y;
    int32_t rem = (int32_t)x % (int32_t)y;
    if(!res)
        state->flags |= FLAG_Z;
    if(rem)
        state->flags |= FLAG_C;
    if(res < 0)
        state->flags |= FLAG_N;
}

void flags_shl(int16_t x, int16_t y, cpu_state* state)
{
    state->flags = 0;
    int16_t res = x << y;
    if(!res)
        state->flags |= FLAG_Z;
    if(res < 0)
        state->flags |= FLAG_N;
}

void flags_shr(uint16_t x, int16_t y, cpu_state* state)
{
    state->flags = 0;
    uint16_t res = (uint16_t)x >> y;
    if(!res)
        state->flags |= FLAG_Z;
    if((int16_t)res < 0)
        state->flags |= FLAG_N;
}

void flags_sar(int16_t x, int16_t y, cpu_state* state)
{
    state->flags = 0;
    int16_t res = x >> y;
    if(!res)
        state->flags |= FLAG_Z;
    if(res < 0)
        state->flags |= FLAG_N;
}

int test_cond(cpu_state* state)
{
    /*
        Z   = 0x0 // [z==1]         Equal (Zero)
        NZ  = 0x1 // [z==0]         Not Equal (Non-Zero)
        N   = 0x2 // [n==1]         Negative
        NN  = 0x3 // [n==0]         Not-Negative (Positive or Zero)
        P   = 0x4 // [n==0 && z==0] Positive
        O   = 0x5 // [o==1]         Overflow
        NO  = 0x6 // [o==0]         No Overflow
        A   = 0x7 // [c==0 && z==0] Above       (Unsigned Greater Than)
        AE  = 0x8 // [c==0]         Above Equal (Unsigned Greater Than or Equal)
        B   = 0x9 // [c==1]         Below       (Unsigned Less Than)
        BE  = 0xA // [c==1 || z==1] Below Equal (Unsigned Less Than or Equal)
        G   = 0xB // [o==n && z==0] Signed Greater Than
        GE  = 0xC // [o==n]         Signed Greater Than or Equal
        L   = 0xD // [o!=n]         Signed Less Than
        LE  = 0xE // [o!=n || z==1] Signed Less Than or Equal
        RES = 0xF // Reserved for future use
    */
    switch(state->i.yx & 0x0f)
    {
        case C_Z:
            if(state->flags & FLAG_Z)
                break;
            return 0;
        case C_NZ:
            if(!(state->flags & FLAG_Z))
                break;
            return 0;
        case C_N:
            if(state->flags & FLAG_N)
                break;
            return 0;
        case C_NN:
            if(!(state->flags & FLAG_N))
                break;
            return 0;
        case C_P:
            if(!(state->flags & (FLAG_N | FLAG_Z)))
                break;
            return 0;
        case C_O:
            if(state->flags & FLAG_O)
                break;
            return 0;
        case C_NO:
            if(!(state->flags & FLAG_O))
                break;
            return 0;
        case C_A:
            if(!(state->flags & (FLAG_C | FLAG_Z)))
                break;
            return 0;
        case C_AE:
            if(!(state->flags & FLAG_C))
                break;
            return 0;
        case C_B:
            if(state->flags & FLAG_C)
                break;
            return 0;
        case C_BE:
            if((state->flags & FLAG_C) || (state->flags & FLAG_Z))
                break;
            return 0;
        case C_G:
            if(((state->flags & FLAG_O) >> 5 == (state->flags & FLAG_N) >> 6) && !(state->flags & FLAG_Z))
                break;
            return 0;
        case C_GE:
            if((state->flags & FLAG_O) >> 5 == (state->flags & FLAG_N) >> 6)
                break;
            return 0;
        case C_L:
            if((state->flags & FLAG_O) >> 5 != (state->flags & FLAG_N) >> 6)
                break;
            return 0;
        case C_LE:
            if(((state->flags & FLAG_O) >> 5 != (state->flags & FLAG_N) >> 6) || state->flags & FLAG_Z)
                break;
            return 0;
        case C_RES:
        default:
            return 0;
    }
    /* The test has succeeded as the switch has transferred here. */
    return 1;
}
