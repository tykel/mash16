#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>

/*
 * Chip16 registers --
 *
 * The registers we are emulating.
 */
typedef enum {
    CREG_R0, CREG_R1, CREG_R2, CREG_R3, 
    CREG_R4, CREG_R5, CREG_R6, CREG_R7,
    CREG_R8, CREG_R9, CREG_RA, CREG_RB,
    CREG_RC, CREG_RD, CREG_RE, CREG_RF,
    CREG_INVALID,
} chip16_reg;
#define NUM_CREGS 16

/*
 * Virtual registers --
 *
 * Large number of placeholder registers used as intermediates before being
 * mapped into host registers or stack.
 */
#define VREG_INVALID    (-1)
#define VREG_0          0
#define VREG_MAX        63
#define NUM_VREGS       ((VREG_MAX) + 1)

typedef int virtual_reg;

/*
 * Host registers --
 *
 * The x86-64 registers we want to map onto.
 * Virtual registers which cannot be mapped here may spill into memory.
 */
typedef enum {
    HREG_RAX, HREG_RCX, HREG_RDX, HREG_RBX,
    HREG_RSP, HREG_RBP, HREG_RSI, HREG_RDI,
    HREG_R8,  HREG_R9,  HREG_R10, HREG_R11,
    HREG_R12, HREG_R13, HREG_R14, HREG_R15,
    HREG_INVALID,
} host_reg;
#define NUM_HREGS 16

/*
 * Recompiled blocks -- 
 *
 * We go the simple route and use a simple lookup table of 16384 entries, making
 * the assumption that we can safely discard the lower 2 bits of the Chip16
 * addresses.
 *
 * In future we may want to use an associative data structure instead.
 */
#define NUM_RECBLKS ((UINT16_MAX + 1) >> 2)

typedef uint8_t* ptr_t;
typedef void* recblk_t;

/*
 * Register allocators --
 *
 */


typedef struct {
    /* Mappings from chip16 -> virtual -> host registers. */
    virtual_reg creg_to_vreg[NUM_CREGS];
    host_reg    vreg_to_hreg[NUM_VREGS];

    uint64_t    vreg_used_mask;

    /* List of recompiled blocks. */
    recblk_t*   recompiled_blocks;

    /* Cross-block state. */
    size_t      total_elapsed_cycles;
    uint16_t    chip16_pc;
    ptr_t       host_pc;

    /* Status of the block currently being translated. */
    size_t      block_elapsed_cycles;
    recblk_t    block_ptr;

    /* Chip16 state. */
    uint8_t*    chip16_mem;
    uint8_t*    chip16_fb;
    int16_t     chip16_reg[NUM_CREGS];
    int16_t     chip16_flags;
    uint32_t    chip16_bgc;

} jit_state;

#define CHIP16_PC_OFFSET(x) ((size_t)(&(x)->chip16_pc) - (size_t)(x))
#define CHIP16_FB_OFFSET(x) ((size_t)(&(x)->chip16_fb) - (size_t)(x))
#define CHIP16_BGC_OFFSET(x) ((size_t)(&(x)->chip16_bgc) - (size_t)(x))
#define CHIP16_REG_OFFSET(x,i) ((size_t)(&(x)->chip16_reg[(i)]) - (size_t)(x))

typedef enum {
    HDISP8, HDISP32,
} host_disp_size;

typedef struct
{
    host_reg reg;
    host_disp_size disp_size;
    int32_t disp32;
} host_reg_disp;

virtual_reg jit_new_vreg(jit_state *state)
{
    int i;
    for (i = 0; i < NUM_VREGS; i++) {
        if (!(state->vreg_used_mask & (1 << i))) {
            state->vreg_used_mask |= (1 << i);
            return i;
        }
    }
    return VREG_INVALID;
}

virtual_reg jit_get_vreg(jit_state *state, chip16_reg r)
{
    if (state->creg_to_vreg[r] == VREG_INVALID) {
        state->creg_to_vreg[r] = jit_new_vreg(state);
    }
    return state->creg_to_vreg[r];
}

recblk_t jit_alloc_blk(void)
{
    recblk_t p;
    long page_size = sysconf(_SC_PAGESIZE);
    int res = posix_memalign(&p, page_size, 2 * page_size);
    if (res == 0 && p != NULL) {
        if (mprotect(p, page_size, PROT_READ | PROT_WRITE | PROT_EXEC) < 0) {
            printf("mprotect() failed, errno: %d\n", errno);
            free(p);
            return NULL;
        }
    } else {
        printf("posix_memalign() failed to allocate\n");
    }
    return p;
}

/*
 * Host code emitters --
 *
 * Emit a single host (x86-64) instruction to the code buffer.
 */
ptr_t jit_host_nop(ptr_t host_pc)
{
    ptr_t p = host_pc;
    *p++ = 0x90;
    return p;
}

ptr_t jit_host_mov64_r_to_r(ptr_t host_pc, host_reg reg_from, host_reg reg_to)
{
    ptr_t p = host_pc;
    int x_bit = reg_from >= HREG_R8 ? 1 : 0;
    int r_bit = reg_to >= HREG_R8 ? 8 : 0;
    *p++ = 0x48 + x_bit + r_bit;
    *p++ = 0x89;
    *p++ = 0xc0 + ((reg_from & 7) << 3) + (reg_to & 7);
    return p;
}

ptr_t jit_host_mov32_i_to_r(ptr_t host_pc, int32_t imm, host_reg reg)
{
    ptr_t p = host_pc;
    int x_bit = reg >= HREG_R8 ? 1 : 0;
    *p++ = 0x48 + x_bit;
    *p++ = 0xc7;
    *p++ = 0xc0 | (reg & 0x7);
    *(int32_t *)p = imm;
    p += 4;
    return p;
}

ptr_t jit_host_mov64_a_to_rax(ptr_t host_pc, ptr_t address)
{
    ptr_t p = host_pc;
    *p++ = 0x48;
    *p++ = 0xa1;
    *(ptr_t *)p = address;
    p += 8;
    return p;
}

ptr_t jit_host_lea64_a_to_r(ptr_t host_pc, ptr_t address, host_reg reg)
{
    ptr_t p = host_pc;
    int size_of_instruction = 7;
    int32_t offset = address - (host_pc + size_of_instruction);
    int x_bit = reg >= HREG_R8 ? 1 : 0;
    *p++ = 0x48 + x_bit;
    *p++ = 0x8d;
    *p++ = ((reg & 0x7) << 3) + 0x05;
    *(int32_t *)p = offset;
    p += 4;
    return p;
}

ptr_t jit_host_jnz32_i(ptr_t host_pc, int32_t imm)
{
    ptr_t p = host_pc;
    *p++ = 0x0f;
    *p++ = 0x85;
    *(int32_t *)p = imm;
    p += 4;
    return p;
}

ptr_t jit_host_call_r(ptr_t host_pc, host_reg reg)
{
    ptr_t p = host_pc;
    int x_bit = reg >= HREG_R8 ? 1 : 0;
    if (x_bit) *p++ = 0x40 + x_bit;
    *p++ = 0xff;
    *p++ = 0xc0 + (2 << 3) + (reg & 7);
    return p;
}

ptr_t jit_host_call(ptr_t host_pc, ptr_t function)
{
    ptr_t p = host_pc;
    if ((uint64_t)function < UINT32_MAX) {
        *p++ = 0xe8;
        *(uint32_t *)p = (uint32_t)(uint64_t)function;
        p += 4;
    } else {
        int size_of_instruction = 6;
        int32_t offset = function - (host_pc + size_of_instruction);
        *p++ = 0xff;
        *p++ = (2 << 3) + 5;
        *(int32_t *)p = offset;
        p += 4;
    }
    return p;
}

ptr_t jit_host_ret(ptr_t host_pc)
{
    ptr_t p = host_pc;
    *p++ = 0xc3;
    return p;
}

ptr_t jit_host_mov64_rdisp_to_r(ptr_t host_pc, host_reg_disp rd, host_reg reg)
{
    ptr_t p = host_pc;
    int x_bit = reg >= HREG_R8 ? 1 : 0;
    *p++ = 0x48 + x_bit;
    *p++ = 0x8b;
    *p++ = 0x80 + ((rd.reg & 7) << 3) + (reg & 7);
    *(int32_t *)p = rd.disp32;
    p += 4;
    return p;
}

ptr_t jit_host_mov16_i_to_rdisp(ptr_t host_pc, int16_t imm, host_reg_disp rd)
{
    ptr_t p = host_pc;
    int x_bit = rd.reg >= HREG_R8 ? 1 : 0;
    *p++ = 0x66;
    if (x_bit) *p++ = 0x40 + x_bit;
    *p++ = 0xc7;
    *p++ = 0x80 + (rd.reg & 7);
    *(int32_t *)p = rd.disp32;
    p += 4;
    *(int16_t *)p = imm;
    p += 2;
    return p;
}

ptr_t jit_host_mov32_i_to_rdisp(ptr_t host_pc, int32_t imm, host_reg_disp rd)
{
    ptr_t p = host_pc;
    int x_bit = rd.reg >= HREG_R8 ? 1 : 0;
    if (x_bit) *p++ = 0x40 + x_bit;
    *p++ = 0xc7;
    *p++ = 0x80 + (rd.reg & 7);
    *(int32_t *)p = rd.disp32;
    p += 4;
    *(int32_t *)p = imm;
    p += 4;
    return p;
}

ptr_t jit_emit_add8_i_to_rdisp(ptr_t host_pc, int8_t imm, host_reg_disp rd)
{
    ptr_t p = host_pc;
    int x_bit = rd.reg >= HREG_R8 ? 1 : 0;
    *p++ = 0x66;
    if (x_bit) *p++ = 0x40 + x_bit;
    *p++ = 0x83;
    *p++ = 0x80 + (rd.reg & 7);
    *(int32_t *)p = rd.disp32;
    p += 4;
    *(int8_t *)p = imm;
    p += 1;
    return p;
}

ptr_t jit_host_push64_r(ptr_t host_pc, host_reg reg)
{
    ptr_t p = host_pc;
    int x_bit = reg >= HREG_R8 ? 1 : 0;
    if (x_bit) *p++ = 0x40 + x_bit;
    *p++ = 0x50 + (reg & 7);
    return p;
}

ptr_t jit_host_pop64_r(ptr_t host_pc, host_reg reg)
{
    ptr_t p = host_pc;
    int x_bit = reg >= HREG_R8 ? 1 : 0;
    if (x_bit) *p++ = 0x40 + x_bit;
    *p++ = 0x58 + (reg & 7);
    return p;
}

/*
 * Chip16 code emitters --
 *
 * Handle the emission of host instructions for a single Chip16 instruction.
 * Typically there are many host instructions per Chip16 instruction.
 */
ptr_t jit_emit_nop(jit_state *state, ptr_t host_pc)
{
    return jit_host_nop(host_pc);
}

ptr_t jit_emit_cls(jit_state *state, ptr_t host_pc)
{
    ptr_t p = host_pc;
    host_reg_disp rd = { HREG_RDI, HDISP32, CHIP16_FB_OFFSET(state) };
    /* Pseudo-C: memset(state->chip16_fb, 0, 320*240*4); */
    p = jit_host_push64_r(p, HREG_RDI);
    p = jit_host_mov64_rdisp_to_r(p, rd, HREG_RDI);
    p = jit_host_mov32_i_to_r(p, 0, HREG_RSI);
    p = jit_host_mov32_i_to_r(p, 320*240*4, HREG_RDX);
    p = jit_host_mov32_i_to_r(p, (int32_t)(int64_t)memset, HREG_RAX);
    p = jit_host_call_r(p, HREG_RAX);
    p = jit_host_pop64_r(p, HREG_RDI);
    return p;
}

ptr_t jit_emit_bgc(jit_state *state, int color, ptr_t host_pc)
{
    ptr_t p = host_pc;
    host_reg_disp rd = { HREG_RDI, HDISP32, CHIP16_BGC_OFFSET(state) }; 
    p = jit_host_mov32_i_to_rdisp(p, color, rd);
    return p;
}

bool jit__jx_condition_met(jit_state *state, int condition)
{
    switch (condition) {
        case 0: return !!(state->chip16_flags & 0x20);
        case 1: return !(state->chip16_flags & 0x20);
        case 2: return !!(state->chip16_flags & 0x01);
        case 3: return !(state->chip16_flags & 0x01);
        case 4: return !(state->chip16_flags & 0x21);
        case 5: return !!(state->chip16_flags & 0x2);
        case 6: return !(state->chip16_flags & 0x2);
        case 7: return !(state->chip16_flags & 0x60);
        case 8: return !(state->chip16_flags & 0x40);
        case 9: return !!(state->chip16_flags & 0x40);
        case 10: return !!(state->chip16_flags & 0x60);
        case 11: return !(state->chip16_flags & 0x20) &&
                        !(state->chip16_flags & 0x02 ^ state->chip16_flags & 0x01);
        case 12: return !(state->chip16_flags & 0x02 ^ state->chip16_flags & 0x01);
        case 13: return !!(state->chip16_flags & 0x02 ^ state->chip16_flags & 0x01);
        case 14: return !!(state->chip16_flags & 0x20) ||
                        !!(state->chip16_flags & 0x02 ^ state->chip16_flags & 0x01);
        case 15: return false;
    }
}

ptr_t jit_emit_jmpi(jit_state *state, uint16_t chip16_pc, ptr_t host_pc)
{
    ptr_t p = host_pc;
    host_reg_disp rd = { HREG_RDI, HDISP32, CHIP16_PC_OFFSET(state) };
    p = jit_host_mov16_i_to_rdisp(p, *(int16_t *)&chip16_pc, rd);
    return p;
}

ptr_t jit_emit_jx(jit_state *state, int condition, uint16_t chip16_pc, ptr_t host_pc)
{
    ptr_t p = host_pc;
    host_reg_disp rd = { HREG_RDI, HDISP32, CHIP16_PC_OFFSET(state) };
    /* Pseudo-C:
     * if (!jit__jx_condition_met(state, condition))
     *    state->chip16_pc = chip16_pc;
     */
    p = jit_host_push64_r(p, HREG_RSI);
    p = jit_host_mov32_i_to_r(p, condition, HREG_RSI);
    p = jit_host_mov32_i_to_r(p, (int32_t)(int64_t)jit__jx_condition_met, HREG_RAX);
    p = jit_host_call_r(p, HREG_RAX);
    p = jit_host_jnz32_i(p, 9);
    p = jit_host_mov16_i_to_rdisp(p, *(int16_t *)&chip16_pc, rd);
    p = jit_host_pop64_r(p, HREG_RSI);
    return p;
}

ptr_t jit_emit_ldii(jit_state *state, int16_t imm, chip16_reg reg, ptr_t host_pc)
{
    ptr_t p = host_pc;
    host_reg_disp rd = { HREG_RDI, HDISP32, CHIP16_REG_OFFSET(state, reg) };
    p = jit_host_mov16_i_to_rdisp(p, imm, rd);
    return p;
}

ptr_t jit_chip16_to_host(jit_state *state, uint16_t chip16_pc, ptr_t host_pc)
{
    ptr_t next_host_pc = NULL;
    uint8_t opcode = state->chip16_mem[chip16_pc];
    uint8_t ib1 = state->chip16_mem[chip16_pc + 1];
    chip16_reg rx = ib1 & 0xf;
    int condition = rx;
    chip16_reg ry = ib1 >> 4;
    uint8_t ib2 = state->chip16_mem[chip16_pc + 2];
    uint8_t ib3 = state->chip16_mem[chip16_pc + 3];
    uint16_t iw1 = ((uint16_t)ib3 << 8) + ib2;

    /* First, emit an add(4) to the chip16 pc. */
    host_reg_disp rd = { HREG_RDI, HDISP32, CHIP16_PC_OFFSET(state) };
    next_host_pc = jit_emit_add8_i_to_rdisp(host_pc, 4, rd);

    /* Next, emit the code specific to each opcode. */
    switch(opcode) {
        case 0x01:
            next_host_pc = jit_emit_cls(state, next_host_pc);
            break;
        case 0x03:
            next_host_pc = jit_emit_bgc(state, ib2, next_host_pc);
            break;
        case 0x10:
            next_host_pc = jit_emit_jmpi(state, chip16_pc, next_host_pc);
            break;
        case 0x12:
            next_host_pc = jit_emit_jx(state, condition, chip16_pc, next_host_pc);
            break;
        case 0x20:
            next_host_pc = jit_emit_ldii(state, *(int16_t *)&iw1, rx, next_host_pc);
            break;
        default:
            printf("jit: chip16 opcode %02x unimplemented, treating as nop\n", opcode);
            next_host_pc = jit_emit_nop(state, next_host_pc);
            break;
    }
    return next_host_pc;
}

ptr_t jit_recompile_block_prelude(jit_state *state, ptr_t host_pc)
{
    ptr_t p = host_pc;
    p = jit_host_push64_r(p, HREG_RBP);
    p = jit_host_mov64_r_to_r(p, HREG_RSP, HREG_RBP);
    return p;
}

ptr_t jit_recompile_block_epilogue(jit_state *state, ptr_t host_pc)
{
    ptr_t p = host_pc;
    p = jit_host_pop64_r(p, HREG_RBP);
    p = jit_host_ret(p);
    return p;
}

recblk_t jit_recompile_blk(jit_state *state, uint16_t chip16_start_addr)
{
    int p, chip16_blk_bytes = 0, chip16_num_instructions = 0;
    uint16_t chip16_end_addr = chip16_start_addr;
    recblk_t block = jit_alloc_blk();
    ptr_t host_pc = block, hp;

    /*
     * Steps 1-2: Find the end of the block, and translate to host code.
     *
     * We end a block at the next branch instruction.
     * This means any of JMP, JMC, Jx, JME, CALL, Cx, RET.
     *
     * We use virtual registers at this stage.
     */
    host_pc = jit_recompile_block_prelude(state, host_pc);
    for (p = chip16_start_addr; p < UINT16_MAX + 1; p += 4) {
        uint8_t opcode = state->chip16_mem[p];
        /* Translate. */
        host_pc = jit_chip16_to_host(state, p, host_pc);
        /* End condition. */
        if (opcode >= 0x10 && opcode <= 0x18) {
            chip16_end_addr = p;
            break;
        }
    }
    /* End our code buffer with a return. */
    host_pc = jit_recompile_block_epilogue(state, host_pc);

    /* Write some debug output. */
    chip16_blk_bytes = chip16_end_addr - chip16_start_addr + 4;
    chip16_num_instructions = chip16_blk_bytes / 4;
    printf("basic block: %04x .. %04x, size %d bytes (%d instructions)\n",
            chip16_start_addr, chip16_end_addr,
            chip16_blk_bytes, chip16_num_instructions);
    printf("emitted x86-64 code, %p ... %p):\n", block, host_pc);
    for (hp = block; hp < host_pc; hp++) {
        printf("%02x ", *hp);
    }
    printf("\n");

    /*
     * Step 3: Convert the virtual registers to host registers, maybe emitting
     * more code to make it work.
     */

    return block;
}

recblk_t jit_get_blk(jit_state *state, uint16_t chip16_pc)
{
    uint16_t block_index = chip16_pc >> 2;
    if (state->recompiled_blocks[block_index] == NULL) {
        state->recompiled_blocks[block_index] = jit_recompile_blk(state, chip16_pc);
    }
    return state->recompiled_blocks[block_index];
}

typedef void (*void_fptr)(jit_state *state);

int main(int argc, char *argv)
{
    jit_state state = { 0 };
    uint8_t chip16_rom[] = {
        0x01, 0x00, 0x00, 0x00, // 0000: CLS
        0x03, 0x00, 0x07, 0x00, // 0004: BGC 7
        0x20, 0x00, 0xCD, 0xAB, // 0008: LDI r0, 0xABCD
        0x12, 0x01, 0x00, 0x00, // 000C: JNZ 000C
        0x10, 0x00, 0x10, 0x00, // 0010: JMP 0010
        0x10, 0x00, 0x14, 0x00, // 0014: JMP 0014
    };
    int test_passes = 0, num_tests = 4, test_matrix[4] = { 0 }, i;
    
    state.recompiled_blocks = malloc(NUM_RECBLKS);
    state.chip16_mem = malloc(0xffff + 1);
    state.chip16_fb = malloc(320 * 240 * 4);

    printf("chip16 fb offset from struct start: %lu bytes\n", CHIP16_FB_OFFSET(&state));
    memcpy(state.chip16_mem, chip16_rom, sizeof(chip16_rom));
    memset(state.chip16_fb, 0xff, 320 * 240 * 4);

    recblk_t block = jit_get_blk(&state, 0);
    ((void_fptr)block)(&state);

    /* Run tests on resulting state to verify execution. */
    if (state.chip16_fb[0] == 0x00 && state.chip16_fb[320 * 240 * 4 - 1] == 0x00) { test_passes++; test_matrix[0] = 1; }
    if (state.chip16_bgc == 7) { test_passes++; test_matrix[1] = 1; }
    if (state.chip16_reg[0] == *(int16_t *)&chip16_rom[10]) { test_passes++; test_matrix[2] = 1; }
    if (state.chip16_pc == 0x000c) { test_passes++; test_matrix[3] = 1; }
    
    printf("passed %d/%d tests [%s]: ",
            test_passes, num_tests, test_passes == num_tests ? "SUCCESS": "FAILURE");
    for (i = 0; i < num_tests; i++) {
        printf("%d:%s ", i, test_matrix[i] ? "OK  " : "FAIL");
    }
    printf("\n");

    free(state.chip16_fb);
    free(state.chip16_mem);
    free(state.recompiled_blocks);
    return 0;
}

