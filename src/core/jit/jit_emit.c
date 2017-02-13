#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef int host_reg;
typedef uint8_t *ptr_t;

typedef union {
    int32_t i32;
    int16_t i16;
    int8_t i8;
} int_union;

typedef enum {
    OPD_REG,            /* eax */
    OPD_IMM,            /* 100 */
    OPD_MEM_ABS,        /* [0x7fff3ab2] */
    OPD_MEM_PCREL,      /* [rip + 100] */
    OPD_MEM_REGREL,     /* [rax + 100] */
} opd_type;

static uint8_t rex1(int data_size, host_reg reg, bool is_src_reg)
{
    uint8_t reg_bits = (reg >= 8) << (is_src_reg ? 2 : 0);
    if (reg >= 8) { 
        return 0x40 + reg_bits;
    } else if (data_size == 1 && reg >= 4) {
        return 0x40;
    }
    return 0;
}

static uint8_t rex2(int data_size, host_reg from, host_reg to)
{
    uint8_t from_bits = (from >= 8) << 2;
    uint8_t to_bits = to >= 8;
    if (from >= 8 || to >= 8) { 
        return 0x40 + from_bits + to_bits;
    } else if (data_size == 1 && (from >= 4 || to >= 4)) {
        return 0x40;
    }
    return 0;
}

static uint8_t modrm_r_to_r(host_reg from, host_reg to)
{
    uint8_t from_bits = (from & 7) << 3;
    uint8_t to_bits = to & 7;
    return 0xc0 + from_bits + to_bits;
}

static uint8_t modrm_i_to_r(host_reg reg)
{
    uint8_t reg_bits = (reg & 7);
    return 0xc0 + reg_bits;
}

static uint8_t modrm_r_to_m(host_reg reg)
{
    uint8_t reg_bits = (reg & 7) << 3;
    return reg_bits;
}

static uint8_t modrm_m_to_r(host_reg reg)
{
    return modrm_r_to_m(reg);
}

static uint8_t modrm_i_to_m()
{
    return 0x05;
}

ptr_t jit_E_add_r_to_r(ptr_t host_pc, int data_size, host_reg from, host_reg to)
{
    ptr_t p = host_pc;
    if (data_size == 2) *p++ = 0x66;
    if (to >= 8 || from >= 8) *p++ = 0x40 + (to >= 8) + ((from >= 8) << 2);
    else if(data_size == 1 && (from >= 4 || to >= 4)) *p++ = 0x40;
    if (data_size == 1)
        *p++ = 0x00;
    else
        *p++ = 0x01;
    *p++ = 0xc0 + ((from & 7) << 3) + (to & 7);
    return p;
}

ptr_t jit_E_add_i_to_r(ptr_t host_pc, int data_size, int_union imm, host_reg reg)
{
    ptr_t p = host_pc;
    if (data_size == 2) *p++ = 0x66;
    if (reg >= 8)       *p++ = 0x41;
    else if(data_size == 1 && reg >= 4) *p++ = 0x40;
    if (data_size == 1)
        *p++ = 0x80;
    else
        *p++ = 0x81;
    *p++ = 0xc0 + (reg & 7);
    if (data_size == 1)
        *(int8_t *)p = imm.i8;
    else if (data_size == 2)
        *(int16_t *)p = imm.i16;
    else if (data_size == 4)
        *(int32_t *)p = imm.i32;
    p += data_size;
    return p;
}

ptr_t jit_E_add_i_to_m(ptr_t host_pc, int data_size, int_union imm, ptr_t address)
{
    ptr_t p = host_pc;
    int32_t disp = address - (host_pc + (data_size == 2) + data_size + 6);
    if (data_size == 2) *p++ = 0x66;
    if (data_size == 1)
        *p++ = 0x80;
    else
        *p++ = 0x81;
    *p++ = 0x05;
    *(int32_t *)p = disp;
    p += 4;
    if (data_size == 1)
        *(int8_t *)p = imm.i8;
    else if (data_size == 2)
        *(int16_t *)p = imm.i16;
    else if (data_size == 4)
        *(int32_t *)p = imm.i32;
    p += data_size;
    return p;
}

ptr_t jit_E_mov_r_to_rdisp(ptr_t host_pc, int data_size, host_reg from, host_reg to, int_union disp)
{
    ptr_t p = host_pc;
    if (data_size == 2) *p++ = 0x66;
    if (from >= 8 || to >= 8) *p++ = 0x40 + ((from & 7) << 3) + (to & 7);
    else if(data_size == 1 && (from >= 4 || to >= 4)) *p++ = 0x40;
    if (data_size == 1)
        *p++ = 0x88;
    else
        *p++ = 0x89;
    *p++ = 0x40 + ((from & 7) << 3) + (to & 7);
    if (data_size == 1)
        *(int8_t *)p = disp.i8;
    else if (data_size == 4)
        *(int32_t *)p = disp.i32;
    p += data_size;
    return p;
}

ptr_t jit_E_mov_r_to_m(ptr_t host_pc, int data_size, host_reg reg, ptr_t address)
{
    ptr_t p = host_pc;
    int32_t disp = address - (host_pc + (data_size == 2) + (reg >= 8) + 6);
    if (data_size == 2) *p++ = 0x66;
    if (reg >= 8)       *p++ = 0x44;
    else if(data_size == 1 && reg >= 4) *p++ = 0x40;
    if (data_size == 1)
        *p++ = 0x88;
    else
        *p++ = 0x89;
    *p++ = ((reg & 7) << 3) + 5;
    *(int32_t *)p = disp;
    p += 4;
    return p;
}

ptr_t jit_E_mov_m_to_r(ptr_t host_pc, int data_size, ptr_t address, host_reg reg)
{
    ptr_t p = host_pc;
    int32_t disp = address - (host_pc + (data_size == 2) + (reg >= 8) + 6);
    if (data_size == 2) *p++ = 0x66;
    if (reg >= 8)       *p++ = 0x44;
    else if(data_size == 1 && reg >= 4) *p++ = 0x40;
    if (data_size == 1)
        *p++ = 0x8a;
    else
        *p++ = 0x8b;
    *p++ = ((reg & 7) << 3) + 5;
    *(int32_t *)p = disp;
    p += 4;
    return p;
}

ptr_t jit_E_xor_r_to_r(ptr_t host_pc, int data_size, host_reg from, host_reg to)
{
    ptr_t p = host_pc;
    if (data_size == 2) *p++ = 0x66;
    if (to >= 8 || from >= 8) *p++ = 0x40 + (to >= 8) + ((from >= 8) << 2);
    else if(data_size == 1 && (to >= 4 || from >= 4)) *p++ = 0x40;
    if (data_size == 1)
        *p++ = 0x30;
    else
        *p++ = 0x31;
    *p++ = 0xc0 + ((from & 7) << 3) + (to & 7);
    return p;
}

ptr_t jit_E_nop(ptr_t host_pc)
{
    ptr_t p = host_pc;
    *p++ = 0x90;
    return p;
}

ptr_t jit_E_call_m(ptr_t host_pc, ptr_t address)
{
    ptr_t p = host_pc;
    int32_t disp = address - (host_pc + 6);
    *p++ = 0xff;
    *p++ = 0x15;
    *(int32_t *)p = disp;
    p += 4;
    return p;
}

ptr_t jit_E_call_r(ptr_t host_pc, host_reg reg)
{
    ptr_t p = host_pc;
    if (reg >= 8) *p++ = 0x41;
    *p++ = 0xff;
    *p++ = 0x10 + (reg & 7);
    return p;
}

ptr_t jit_E_ret(ptr_t host_pc)
{
    ptr_t p = host_pc;
    *p++ = 0xc3;
    return p;
}

ptr_t jit_E_push_r(ptr_t host_pc, int data_size, host_reg reg)
{
    ptr_t p = host_pc;
    if (data_size == 2) *p++ = 0x66;
    if (reg >= 8)       *p++ = 0x41;
    *p++ = 0x50 + (reg & 7);
    return p;
}

ptr_t jit_E_pop_r(ptr_t host_pc, int data_size, host_reg reg)
{
    ptr_t p = host_pc;
    if (data_size == 2) *p++ = 0x66;
    if (reg >= 8)       *p++ = 0x41;
    *p++ = 0x58 + (reg & 7);
    return p;
}

int main(int argc, char *argv[])
{
    uint8_t codebuf[20];
    uint8_t *p, *p2;
    int_union tff = { 0x20 };
    
    p = jit_E_mov_m_to_r(codebuf, 2, codebuf + 20, 2);
    p2 = p;
    for (p = codebuf; p < p2; ++p) {
        printf("%02x ", *p);
    }
    printf("\n");
    
    p = jit_E_mov_r_to_m(codebuf, 2, 2, codebuf + 20);
    p2 = p;
    for (p = codebuf; p < p2; ++p) {
        printf("%02x ", *p);
    }
    printf("\n");
    
    p = jit_E_add_i_to_r(codebuf, 2, tff, 2);
    p2 = p;
    for (p = codebuf; p < p2; ++p) {
        printf("%02x ", *p);
    }
    printf("\n");
    
    p = jit_E_add_i_to_m(codebuf, 2, tff, codebuf + 20);
    p2 = p;
    for (p = codebuf; p < p2; ++p) {
        printf("%02x ", *p);
    }
    printf("\n");
 
    p = jit_E_add_r_to_r(codebuf, 2, 2, 10);
    p2 = p;
    for (p = codebuf; p < p2; ++p) {
        printf("%02x ", *p);
    }
    printf("\n");
 
    p = jit_E_mov_r_to_rdisp(codebuf, 1, 7, 0, tff);
    p2 = p;
    for (p = codebuf; p < p2; ++p) {
        printf("%02x ", *p);
    }
    printf("\n");
 
    p = jit_E_mov_r_to_rdisp(codebuf, 1, 0, 7, tff);
    p2 = p;
    for (p = codebuf; p < p2; ++p) {
        printf("%02x ", *p);
    }
    printf("\n");
 
    p = jit_E_mov_r_to_rdisp(codebuf, 2, 0, 7, tff);
    p2 = p;
    for (p = codebuf; p < p2; ++p) {
        printf("%02x ", *p);
    }
    printf("\n");

    return 0;
}
