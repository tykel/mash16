#ifndef CPU_H
#define CPU_H

#include "../consts.h"
#include <SDL/SDL.h>

#define FLAG_C 2
#define FLAG_Z 4
#define FLAG_O 64
#define FLAG_N 128

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
                };
                uint8_t res;
            };
        };
    };
} instr;

/* Stores CPU flags. */
typedef struct 
{
    uint32_t c;
    uint32_t z;
    uint32_t o;
    uint32_t n;
} flags;

/* Holds information about the CPU. */
typedef struct
{
    uint64_t cycles;
    int wait_vblnk;

} cpu_meta;

/* Holds CPU functionality. */
typedef struct
{
    /* Pure CPU stuff. */
    int16_t r[16];
    uint16_t pc;
    uint16_t sp;
    instr    i;
    flags    f;
    uint8_t* m;
    
    /* Gfx stuff. */
    uint8_t  bgc;
    uint8_t  sw;
    uint8_t  sh;
    uint8_t  fx;
    uint8_t  fy;
    uint32_t* pal;
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
    cpu_meta meta;

} cpu_state;

/* Instruction function pointer table. */
typedef void (*cpu_op)(cpu_state*);
cpu_op op_table[0x100];

/* CPU functions. */
void cpu_init(cpu_state**,uint8_t*);
void cpu_step(cpu_state*);
void cpu_io_update(SDL_KeyboardEvent*,cpu_state*);
void cpu_free(cpu_state*);

void op_error(cpu_state*);
void op_nop(cpu_state*);
void op_cls(cpu_state*);
void op_vblnk(cpu_state*);
void op_bgc(cpu_state*);
void op_spr(cpu_state*);
void op_drw_imm(cpu_state*);
void op_drw_r(cpu_state*);
void op_rnd(cpu_state*);
void op_flip(cpu_state*);
void op_snd0(cpu_state*);
void op_snd1(cpu_state*);
void op_snd2(cpu_state*);
void op_snd3(cpu_state*);
void op_snp(cpu_state*);
void op_sng(cpu_state*);
void op_jmp_imm(cpu_state*);
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
/* Generate the flags for the arithmetic instructions. */
void flags_add(int16_t,int16_t,cpu_state*);
void flags_sub(int16_t,int16_t,cpu_state*);
void flags_and(int16_t,int16_t,cpu_state*);
void flags_or(int16_t,int16_t,cpu_state*);
void flags_xor(int16_t,int16_t,cpu_state*);
void flags_mul(int16_t,int16_t,cpu_state*);
void flags_div(int16_t,int16_t,cpu_state*);
void flags_shl(int16_t,int16_t,cpu_state*);
void flags_shr(uint16_t,int16_t,cpu_state*);
void flags_sar(int16_t,int16_t,cpu_state*);
/* Test the current jump/call conditional. */
int test_cond(cpu_state*);

#endif

