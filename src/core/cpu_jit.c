#include <libjit.h>

#include "cpu.h"
#include "audio.h"

static void** gp_blockmap[0x10000 >> 2];

void* cpu_jit_compile_block(cpu_state *state, uint16_t a)
{
    uint16_t start = a;
    uint16_t end = start;
    uint16_t furthest = end;

    for(; end <= 0xfffc; end = end + 4) {
        uint8_t op = state->m[end];
        uint16_t hhll = *(uint16_t *)&state->m[end];
        /* A RET with no branch dests. after it means we're done. */
        if(op == 0x15 && end >= furthest) { /* RET */
            break;
        /* A branch to after end means we still need to go there. */
        } else if(op == 0x12) { /* Jx */
            if(hhll > end) {
                furthest = hhll;
            }
        /* A JMP to somewhere we've been means we're done. */
        } else if(op == 0x10) { /* JMP */
            if(hhll >= start && hhll <= end) {
                break;
            }
        }
    }

    printf("[jit] basic block found: 0x%04x - 0x%04x\n", start, end);

    return NULL;
}

void* cpu_jit_get_block(cpu_state *state, uint16_t a)
{
    void *p_block = NULL;

    /* Compile the block if it hasn't been visited yet! */
    if(gp_blockmap[a >> 2] == NULL) {
        gp_blockmap[a >> 2] = cpu_jit_compile_block(state, a);
    }
    p_block = gp_blockmap[a >> 2];

    return p_block;
}

#if 0
int cpu_jit_op_drw(cpu_state *state, size_t moffs, int x, int y)
{
    return op_drw(&state->m[moffs], state->vm, x, y, state->sw, state->sh, state->fx, state->fy);
}

void cpu_emit_nop(cpu_state *s)
{
}

void cpu_emit_cls(cpu_state *s)
{
    struct jit_instr *i = NULL;
    i = jit_instr_new(s->j);
    CALL_M(i, op_cls, JIT_32BIT);
}

void cpu_emit_vblnk(cpu_state *s)
{
    struct jit_instr *i = NULL;
    i = jit_instr_new(s->j);
    CALL_M(i, op_vblnk, JIT_32BIT);
}

void cpu_emit_bgc(cpu_state *s)
{
    struct jit_instr *i0, *i1;
    i0 = jit_instr_new(s->j);
    i1 = jit_instr_new(s->j);
    MOVE_I_R(i0, i_n(s->i), jit_reg_new(s->j), JIT_32BIT);
    MOVE_R_M(i1, i0->out.reg, &s->bgc, JIT_8BIT);
}

void cpu_emit_spr(cpu_state *s)
{
    struct jit_instr *i0, *i1, *i2, *i3, *i4, *i5;
    i0 = jit_instr_new(s->j);
    i1 = jit_instr_new(s->j);
    i2 = jit_instr_new(s->j);
    i3 = jit_instr_new(s->j);
    i4 = jit_instr_new(s->j);
    i5 = jit_instr_new(s->j);
    MOVE_I_R(i0, i_hhll(s->i), jit_reg_new(s->j), JIT_32BIT);
    MOVE_R_R(i1, i0->out.reg, jit_reg_new(s->j), JIT_32BIT);
    AND_I_R_R(i2, 0x00ff, i0->out.reg, i0->out.reg, JIT_32BIT);
    SHR_I_R(i3, 8, i1->out.reg, JIT_32BIT);
    MOVE_R_M(i4, i0->out.reg, &s->sw, JIT_8BIT);
    MOVE_R_M(i5, i1->out.reg, &s->sh, JIT_8BIT);
}

void cpu_emit_drw_i(cpu_state *s)
{
    struct jit_instr *i0, *i1, *i2, *i3, *i4, *i5;
    jit_reg a = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG0);
    jit_reg x = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG1);
    jit_reg y = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG2);
    jit_reg c = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_RET);
    
    i0 = jit_instr_new(s->j);
    i1 = jit_instr_new(s->j);
    i2 = jit_instr_new(s->j);
    i3 = jit_instr_new(s->j);
    i4 = jit_instr_new(s->j);
    i5 = jit_instr_new(s->j);
    
    MOVE_I_R(i0, i_hhll(s->i), a, JIT_32BIT);
    MOVE_ID_R(i1, &s->r, y, JIT_32BIT);
    MOVE_RP_R(i2, y, JIT_REG_INVALID, 0, i_yx(s->i) & 0x0f, x, JIT_32BIT);
    MOVE_RP_R(i3, y, JIT_REG_INVALID, 0, i_yx(s->i) >> 4, y, JIT_32BIT);
    CALL_M(i4, op_drw, JIT_32BIT);
    MOVE_R_M(i5, c, &s->f.c, JIT_32BIT);
}

void cpu_emit_drw_r(cpu_state *s)
{
    /* TODO: We assume a simplified op_drw(x, y, m) */
    int n;
    struct jit_instr *i[7];
    jit_reg a = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG0);
    jit_reg x = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG1);
    jit_reg y = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG2);
    jit_reg c = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_RET);
    
    for(n = 0; n < 7; n++)
        i[n] = jit_instr_new(s->j);
    
    MOVE_ID_R(i[0], &s->r, y, JIT_32BIT);
    MOVE_RP_R(i[3], y, JIT_REG_INVALID, 0, i_z(s->i) & 0x0f, a, JIT_32BIT);
    MOVE_RP_R(i[1], y, JIT_REG_INVALID, 0, i_yx(s->i) & 0x0f, x, JIT_32BIT);
    MOVE_RP_R(i[2], y, JIT_REG_INVALID, 0, i_yx(s->i) >> 4, y, JIT_32BIT);
    CALL_M(i[4], op_drw, JIT_32BIT);
    MOVE_R_M(i[5], c, &s->f.c, JIT_32BIT);
}

void cpu_emit_rnd(cpu_state *s)
{
    int n;
    struct jit_instr *i[5];
    jit_reg x = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG0);
    jit_reg r = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_RET);
    
    for(n = 0; n < 5; n++)
        i[n] = jit_instr_new(s->j);

    XOR_R_R_R(i[0], x, x, x, JIT_32BIT);
    CALL_M(i[1], rand, JIT_32BIT);
    AND_I_R_R(i[2], i_hhll(s->i), r, r, JIT_32BIT);
    MOVE_ID_R(i[3], &s->r, x, JIT_32BIT);
    MOVE_R_RP(i[4], x, x, JIT_REG_INVALID, 0, i_yx(s->i) & 0x0f, JIT_32BIT);
}

void cpu_emit_flip(cpu_state *s)
{
    int n;
    struct jit_instr *i[5];
    jit_reg a = jit_reg_new(s->j);
    jit_reg b = jit_reg_new(s->j);

    for(n = 0; n < 5; n++)
        i[n] = jit_instr_new(s->j);

    MOVE_I_R(i[0], i_hhll(s->i), a, JIT_32BIT);
    SHL_I_R_R(i[1], 1, a, b, JIT_32BIT);
    AND_I_R_R(i[2], 1, a, a, JIT_32BIT);
    OR_R_R_R(i[3], b, a, a, JIT_32BIT);
    MOVE_R_M(i[4], a, (int16_t *)&s->fx, JIT_16BIT);
}

void cpu_emit_snd0(cpu_state *s)
{
    int n;
    struct jit_instr *i[1];

    for(n = 0; n < 1; n++)
        i[n] = jit_instr_new(s->j);
    
    CALL_M(i[0], (int32_t *)audio_stop, JIT_32BIT);
}

void cpu_emit_snd1(cpu_state *s)
{
    int n;
    struct jit_instr *i[4];
    jit_reg f = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG0);
    jit_reg d = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG1);
    jit_reg a = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG2);

    for(n = 0; n < 4; n++)
        i[n] = jit_instr_new(s->j);
    
    MOVE_I_R(i[0], 500, f, JIT_32BIT);
    MOVE_I_R(i[1], i_hhll(s->i), d, JIT_32BIT);
    MOVE_I_R(i[2], 0, a, JIT_32BIT);
    CALL_M(i[3], (int32_t *)audio_play, JIT_32BIT);
}

void cpu_emit_snd2(cpu_state *s)
{
    int n;
    struct jit_instr *i[4];
    jit_reg f = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG0);
    jit_reg d = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG1);
    jit_reg a = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG2);

    for(n = 0; n < 4; n++)
        i[n] = jit_instr_new(s->j);
    
    MOVE_I_R(i[0], 1000, f, JIT_32BIT);
    MOVE_I_R(i[1], i_hhll(s->i), d, JIT_32BIT);
    MOVE_I_R(i[2], 0, a, JIT_32BIT);
    CALL_M(i[3], (int32_t *)audio_play, JIT_32BIT);
}

void cpu_emit_snd3(cpu_state *s)
{
    int n;
    struct jit_instr *i[4];
    jit_reg f = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG0);
    jit_reg d = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG1);
    jit_reg a = jit_reg_new_fixed(s->j, JIT_REGMAP_CALL_ARG2);

    for(n = 0; n < 4; n++)
        i[n] = jit_instr_new(s->j);
    
    MOVE_I_R(i[0], 1500, f, JIT_32BIT);
    MOVE_I_R(i[1], i_hhll(s->i), d, JIT_32BIT);
    MOVE_I_R(i[2], 0, a, JIT_32BIT);
    CALL_M(i[3], (int32_t *)audio_play, JIT_32BIT);
}

#endif
