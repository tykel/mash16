#ifndef CPU_H
#define CPU_H

#include "../consts.h"

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
    uint16_t flags;
    uint8_t* m;
    
    /* Gfx stuff. */
    uint8_t  bgc;
    uint8_t  sw;
    uint8_t  sh;
    uint8_t  fx;
    uint8_t  fy;
    uint8_t* pal;
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
void cpu_init(cpu_state*);
void cpu_step(cpu_state*);

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

void flags_add(int16_t,int16_t);
void flags_sub(int16_t,int16_t);
void flags_and(int16_t,int16_t);
void flags_or(int16_t,int16_t);
void flags_xor(int16_t,int16_t);
void flags_mul(int16_t,int16_t);
void flags_div(int16_t,int16_t);
void flags_shl(int16_t,int16_t);
void flags_shr(int16_t,int16_t);
void flags_sar(int16_t,int16_t);

#endif

