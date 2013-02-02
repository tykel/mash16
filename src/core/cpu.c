#include "../consts.h"
#include "cpu.h"
#include "gpu.h"
#include "audio.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/* Initialise the CPU to safe values. */
void cpu_init(cpu_state** state, uint8_t* mem)
{
    if(!(*state = (cpu_state*)calloc(1,sizeof(cpu_state))))
    {
        fprintf(stderr,"error: calloc failed (state)\n");
        exit(1);
    }
    (*state)->m = mem;
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
    (*state)->sp = STACK_ADDR;
    (*state)->f = (flags){0};
    
    srand(time(NULL));

    /* Ensure unused instructions return errors. */
    for(int i=0; i<0x100; ++i)
    {
        op_table[i] = &op_error;
    }
    /* Map instr. table entries to functions. */
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
    op_table[0x11] = &op_jmc;
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
    /* Call function pointer table entry */
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
    free(state);
}

/* CPU instructions. */

void op_error(cpu_state* state)
{
    fprintf(stderr,"error: unknown opcode encountered! (0x%x)\n",state->i.op);
    fprintf(stderr,"state: pc=%40x\n",state->pc);
    exit(1);
}

void op_nop(cpu_state* state)
{
}

void op_cls(cpu_state* state)
{
    memset(state->vm,0,320*240);
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
    int16_t x = state->r[state->i.yx & 0x0f];
    int16_t y = state->r[state->i.yx >> 4];
    state->f.c = op_drw(&state->m[state->i.hhll],
        state->vm, x, y, state->sw, state->sh,
        state->fx, state->fy);
}

void op_drw_r(cpu_state* state)
{
    int16_t x = state->r[state->i.yx & 0x0f];
    int16_t y = state->r[state->i.yx >> 4];
    state->f.c = op_drw(&state->m[(uint16_t)state->r[state->i.z]],
        state->vm, x, y, state->sw, state->sh,
        state->fx, state->fy);
}

int op_drw(uint8_t* m, uint8_t* vm, int x, int y, int w, int h, int fx, int fy)
{
    /* If nothing will be on-screen, may as well exit. */
    if(x > 319 || y > 239 || !w || !h || y+h < 0 || x+w*2 < 0)
        return 0;
    int hit = 0;
    /* Sort out what direction the sprite will be drawn in. */
    int ix_st = 0, ix_end = w*2, ix_inc = 2;
    if(fx)
    {
        ix_st = w*2 - 2;
        ix_end = -2;
        ix_inc = -2;
    }
    int iy_st = 0, iy_end = h, iy_inc = 1;
    if(fy)
    {
        iy_st = h - 1;
        iy_end = -1;
        iy_inc = -1;
    }
    /* Start drawing... */
    for(int iy=iy_st, j=0; iy!=iy_end; iy+=iy_inc, ++j)
    {
        for(int ix=ix_st, i=0; ix!=ix_end; ix+=ix_inc, i+=2)
        {
            /* Bounds checking for memory accesses. */
            if(i+x < 0 || i+x > 318 || j+y < 0 || j+y > 239)
                continue;
            uint8_t p  = m[w*iy + ix/2];
            uint8_t hp = p >> 4;
            uint8_t lp = p & 0x0f;
            /* Flip the pixel couple if necessary. */
            if(fx)
            {
                int t = lp;
                lp = hp;
                hp = t;
            }
            /* Draw the pixels if not transparent. */
            if(hp)
            {
                hit += vm[320*(y+j) + x+i];
                vm[320*(y+j) + x+i] = hp;
            }
            if(lp)
            {
                hit += vm[320*(y+j) + x+i+1]; 
                vm[320*(y+j) + x+i+1] = lp;
            }
        }
    }
    return (hit > 0);
}

void op_rnd(cpu_state* state)
{
    state->r[state->i.yx & 0x0f] = rand() % (state->i.hhll + 1);
}

void op_flip(cpu_state* state)
{
    state->fx = state->i.hhll >> 9;
    state->fy = (state->i.hhll >> 8) & 0x01;
}

void op_snd0(cpu_state* state)
{
    audio_stop();
}

void op_snd1(cpu_state* state)
{
    int16_t dt = state->i.hhll;
    audio_play(500,dt);
}

void op_snd2(cpu_state* state)
{
    int16_t dt = state->i.hhll;
    audio_play(1000,dt);
}

void op_snd3(cpu_state* state)
{
    int16_t dt = state->i.hhll;
    audio_play(1500,dt);
}

void op_snp(cpu_state* state)
{
    int16_t f = state->m[(uint16_t)(state->r[state->i.yx & 0x0f])];
    int16_t dt = state->i.hhll;
    audio_play(f,dt);
}

void op_sng(cpu_state* state)
{
    state->atk = state->i.yx >> 4;
    state->dec = state->i.yx & 0x0f;
    state->sus = (state->i.hhll >> 12) & 0x0f;
    state->rls = (state->i.hhll >> 8) & 0x0f;
    state->vol = (state->i.hhll >> 4) & 0x0f;
    state->type = state->i.hhll & 0x0f;
    audio_update(state);
}

void op_jmp_imm(cpu_state* state)
{
    state->pc = state->i.hhll;
}

/* DEPRECATED -- implemented purely for compatibility. */
void op_jmc(cpu_state* state)
{
    if(state->f.c)
    {
        state->pc = state->i.hhll;
    }
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
    {
        state->pc = state->i.hhll;
    }
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
    state->pc = (uint16_t)state->r[state->i.yx & 0x0f];
}

void op_cx(cpu_state* state)
{
    if(test_cond(state))
    {
        state->m[state->sp] = state->pc & 0x00ff;
        state->m[state->sp + 1] = state->pc >> 8;
        state->sp += 2;
        state->pc = state->i.hhll;
    }
}

void op_call_r(cpu_state* state)
{
    state->m[state->sp] = state->pc & 0x00ff;
    state->m[state->sp + 1] = state->pc >> 8;
    state->sp += 2;
    state->pc = (uint16_t)state->r[state->i.yx & 0x0f];
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
    state->r[state->i.yx & 0x0f] = state->m[state->i.hhll] |
                                  (state->m[state->i.hhll + 1] << 8);
}

void op_ldm_r(cpu_state* state)
{
    state->r[state->i.yx & 0x0f] = state->m[(uint16_t)state->r[state->i.yx >> 4]] |
                                  (state->m[(uint16_t)state->r[state->i.yx >> 4] + 1] << 8);
}

void op_mov(cpu_state* state)
{
    state->r[state->i.yx & 0x0f] = state->r[state->i.yx >> 4];
}

void op_stm_imm(cpu_state* state)
{
    state->m[state->i.hhll] = state->r[state->i.yx & 0x0f] & 0x00ff;
    state->m[state->i.hhll + 1] = state->r[state->i.yx & 0x0f] >> 8;
}

void op_stm_r(cpu_state* state)
{
    state->m[(uint16_t)state->r[state->i.yx >> 4]] = state->r[state->i.yx & 0x0f] & 0x00ff;
    state->m[(uint16_t)state->r[state->i.yx >> 4] + 1] = state->r[state->i.yx & 0x0f] >> 8;
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
    flags_and(rx,ry,state);
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
    flags_xor(*rx,ry,state);
    *rx ^= ry;
}

void op_xor_r3(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    flags_xor(rx,ry,state);
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
    int16_t ry = state->r[state->i.yx >> 4];
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
    if(!imm)
    {
        fprintf(stderr,"error: attempted to divide by 0\n");
        fprintf(stderr,"state: pc=0x%40x\n",state->pc);
        exit(1);
    }
    flags_div(*rx,imm,state);
    *rx /= imm;
}

void op_div_r2(cpu_state* state)
{
    int16_t* rx = &state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    if(!ry)
    {
        fprintf(stderr,"error: attempted to divide by 0\n");
        fprintf(stderr,"state: pc=0x%40x\n",state->pc);
        exit(1);
    }
    flags_div(*rx,ry,state);
    *rx /= ry;
}

void op_div_r3(cpu_state* state)
{
    int16_t rx = state->r[state->i.yx & 0x0f];
    int16_t ry = state->r[state->i.yx >> 4];
    if(!ry)
    {
        fprintf(stderr,"error: attempted to divide by 0\n");
        fprintf(stderr,"state: pc=0x%40x\n",state->pc);
        exit(1);
    }
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
    flags_sar(*rx,n,state);
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
    flags_sar(*rx,ry,state);
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
    state->r[state->i.yx & 0x0f] = (int16_t)(state->m[state->sp] | (state->m[state->sp + 1] << 8));
}

void op_pushall(cpu_state* state)
{
    for(int i=0; i<16; ++i)
    {
        state->m[state->sp] = (uint16_t)state->r[i] & 0x00ff;
        state->m[state->sp + 1] = (uint16_t)state->r[i] >> 8;
        state->sp += 2;
    }
}

void op_popall(cpu_state* state)
{
    for(int i=15; i>=0; --i)
    {
        state->sp -= 2;
        state->r[i] = (int16_t)(state->m[state->sp] | (state->m[state->sp + 1] << 8));
    }
}

void op_pushf(cpu_state* state)
{
    state->m[state->sp] = state->f.c << 1 | state->f.z << 2 |
                          state->f.o << 6 | state->f.n << 7;
    state->m[state->sp + 1] = 0;
    state->sp += 2;
}

void op_popf(cpu_state* state)
{
    state->sp -= 2;
    state->f.c = (state->m[state->sp] >> 1) & 1;
    state->f.z = (state->m[state->sp] >> 2) & 1;
    state->f.o = (state->m[state->sp] >> 6) & 1;
    state->f.n = (state->m[state->sp] >> 7) & 1;
}

void op_pal_imm(cpu_state* state)
{
    load_pal(&state->m[state->i.hhll],0,state);
}

void op_pal_r(cpu_state* state)
{
    load_pal(&state->m[(uint16_t)state->r[state->i.yx & 0x0f]],0,state);
}

/* 
    Flag computing functions.
*/

void flags_add(int16_t x, int16_t y, cpu_state* state)
{
    state->f = (flags){0};
    uint32_t res = (uint16_t)x + (uint16_t)y;
    if(!res)
        state->f.z = 1;
    if(res > UINT16_MAX)
        state->f.c = 1;
    if(((int16_t)res < 0 && (int16_t)x > 0 && (int16_t)y > 0) ||
        ((int16_t)res > 0 && (int16_t)x < 0 && (int16_t)y < 0))
        state->f.o = 1;
    if((int16_t)res < 0)
        state->f.n = 1;
}

void flags_sub(int16_t x, int16_t y, cpu_state* state)
{
    state->f = (flags){0};
    uint32_t res = (uint16_t)x - (uint16_t)y;
    if(!res)
        state->f.z = 1;
    if(res > UINT16_MAX)
        state->f.c = 1;
    if(((int16_t)res < 0 && (int16_t)x > 0 && (int16_t)y < 0) || 
        ((int16_t)res > 0 && (int16_t)x < 0 && y > 0))
        state->f.o = 1;
    if((int16_t)res < 0)
        state->f.n = 1;
}

void flags_and(int16_t x, int16_t y, cpu_state* state)
{
    state->f = (flags){0};
    uint16_t res = (uint16_t)x & (uint16_t)y;
    if(!res)
        state->f.z = 1;
    if((int16_t)res < 0)
        state->f.n = 1;
}

void flags_or(int16_t x, int16_t y, cpu_state* state)
{
    state->f = (flags){0};
    uint16_t res = (uint16_t)x | (uint16_t)y;
    if(!res)
        state->f.z = 1;
    if((int16_t)res < 0)
        state->f.n = 1;
}

void flags_xor(int16_t x, int16_t y, cpu_state* state)
{
    state->f = (flags){0};
    uint16_t res = (uint16_t)x ^ (uint16_t)y;
    if(!res)
        state->f.z = 1;
    if((int16_t)res < 0)
        state->f.n = 1;
}

void flags_mul(int16_t x, int16_t y, cpu_state* state)
{
    state->f = (flags){0};
    uint32_t res = (uint16_t)x * (uint16_t)y;
    if(!res)
        state->f.z = 1;
    if(res > UINT16_MAX)
        state->f.c = 1;
    if((int16_t)res < 0)
        state->f.n = 1;
}

void flags_div(int16_t x, int16_t y, cpu_state* state)
{
    state->f = (flags){0};
    uint16_t res = (uint16_t)x / (uint16_t)y;
    uint16_t rem = (uint16_t)x % (uint16_t)y;
    if(!res)
        state->f.z = 1;
    if(rem)
        state->f.c = 1;
    if((int16_t)res < 0)
        state->f.n = 1;
}

void flags_shl(int16_t x, int16_t y, cpu_state* state)
{
    state->f = (flags){0};
    uint16_t res = (uint16_t)x << y;
    if(!res)
        state->f.z = 1;
    if((int16_t)res < 0)
        state->f.n = 1;
}

void flags_shr(uint16_t x, int16_t y, cpu_state* state)
{
    state->f = (flags){0};
    uint16_t res = (uint16_t)x >> y;
    if(!res)
        state->f.z = 1;
    if((int16_t)res < 0)
        state->f.n = 1;
}

void flags_sar(int16_t x, int16_t y, cpu_state* state)
{
    state->f = (flags){0};
    int16_t res = x >> y;
    if(!res)
        state->f.z = 1;
    if(res < 0)
        state->f.n = 1;
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
            if(state->f.z)
                return 1;
            break;
        case C_NZ:
            if(!state->f.z)
                return 1;
            break;
        case C_N:
            if(state->f.n)
                return 1;
            break;
        case C_NN:
            if(!state->f.n)
                return 1;
            break;
        case C_P:
            if(!state->f.n && !state->f.z)
                return 1;
            break;
        case C_O:
            if(state->f.o)
                return 1;
            break;
        case C_NO:
            if(!state->f.o)
                return 1;
            break;
        case C_A:
            if(!state->f.c && !state->f.z)
                return 1;
            break;
        case C_AE:
            if(!state->f.c)
                return 1;
            break;
        case C_B:
            if(state->f.c)
                return 1;
            break;
        case C_BE:
            if(state->f.c || state->f.z)
                return 1;
            break;
        case C_G:
            if((state->f.o == state->f.n) && !state->f.z)
                return 1;
            break;
        case C_GE:
            if(state->f.o == state->f.n)
                return 1;
            break;
        case C_L:
            if(state->f.o != state->f.n)
                return 1;
            break;
        case C_LE:
            if((state->f.o != state->f.n) || state->f.z)
                return 1;
            break;
        case C_RES:
        default:
            break;
    }
    /* The test has failed. */
    return 0;
}
