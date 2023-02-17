#include "cpu.h"
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>

#define ROUNDUP(n,d) ((((n)+(d)-1) / (d)) * (d))

const uint8_t P_WORD = 0x66;
const uint8_t REX_W = 0x48;

#define STRUCT_OFFSET(p, m) ((void*)((p)->(m)) - (void*)(p))

#define OFFSET(p, n) ((void*)(p) - (void*)((jit_block) + (n)))
#define EMIT(b)   (*jit_block++ = (b))
#define EMIT4i(dw) do { *(int32_t*)jit_block = (int32_t)(dw); jit_block += sizeof(int32_t); } while (0)
#define EMIT4u(dw) do { *(uint32_t*)jit_block = (uint32_t)(dw); jit_block += sizeof(uint32_t); } while (0)
#define EMIT8i(qw) do { *(int64_t*)jit_block = (int64_t)(qw); jit_block += sizeof(int64_t); } while (0)
#define EMIT8u(qw) do { *(uint64_t*)jit_block = (uint64_t)(qw); jit_block += sizeof(uint64_t); } while (0)

static uint8_t modrm(uint8_t mod, uint8_t reg, uint8_t rm)
{
   return (mod << 6) | (reg << 3) | (rm);
}

#define MODRM_RIP_DISP32(reg) modrm(0, reg, 5)
#define MODRM_REG_DIRECT(reg, rm) modrm(3, reg, rm)
#define MODRM_SIB modrm(0, 0, 4)

#define SIB(s,i,b) modrm(s,i,b)

enum {
    RAX = 0,
    RCX,
    RDX,
    RBX,
    RSP,
    RBP,
    RSI,
    RDI,
    R8,
    R9,
    R10,
    R11,
    R12,
    R13,
    R14,
    R15
};


static void* cpu_rec_get_memblk(cpu_state *state, size_t min_bytes)
{
   void* block = state->rec.jit_next;
   state->rec.jit_next += min_bytes;
   return block;
}

void cpu_rec_init(cpu_state* state)
{
   state->rec.jit_base = (void*)ROUNDUP((size_t)global_alloc + sizeof(*state), page_size);
   state->rec.jit_next = state->rec.jit_base;
}

void cpu_rec_free(cpu_state* state)
{
}

/*
 * Write the necessary boilerplate to save registers, setup stack, etc.
 * - `mov rdi, qword ptr [state]` for all opcode calls
 */
static uint8_t* cpu_rec_compile_start(cpu_state *state, uint8_t* jit_block)
{
    ptrdiff_t offs_state = (void *)state - ((void *)jit_block + 7);
    // Move `state` into RDI
    if (offs_state <= INT32_MAX &&
        offs_state >= INT32_MIN) {
       // LEA rdi, [state]
       EMIT(0x48);    // REX.w
       EMIT(0x8d);    // LEA r64, m
       EMIT(MODRM_RIP_DISP32(RDI));
       EMIT4i(offs_state);
    } else {
       // MOV rdi, [state]
       EMIT(0x48);   // REX.w
       EMIT(0xb8 + RDI);   // MOV r64, imm64
       EMIT8u(state);
    }
    
    return jit_block;
}

/* Write the necessary boilerplate to restore registers, and return. */
static uint8_t* cpu_rec_compile_end(cpu_state *state, uint8_t* jit_block)
{
    EMIT(0xc3);    // ret
    return jit_block;
}

/*
 * The CPU JIT currently does partial recompilation, in the sense it does not
 * generate code for the logic of each individual opcode; rather, it chains
 * them together in a sequence of straight-line calls. (A.K.A. a "threaded
 * interpreter").
 */

static uint8_t* cpu_rec_compile_instr(cpu_state *state, uint16_t a, uint8_t *jit_block)
{
    void *op_instr = op_table[state->m[a]];

    // Push rdi
    EMIT(0x50 + RDI);
    
    // Copy instruction 32 bits to state
    // state->i = *(instr*)(&state->m[state->pc]);
    {
       // MOVZX rax, word [state->pc]
       EMIT(0x48);   // REX.w
       EMIT(0x0f);   // MOVZX
       EMIT(0xb7);   // MOVZX
       EMIT(MODRM_RIP_DISP32(RAX));
       EMIT4i(OFFSET(&state->pc, 4));
       
       // MOV rdx, [state->m]
       EMIT(0x48);    // REX.w
       EMIT(0x8b);    // MOV
       EMIT(MODRM_RIP_DISP32(RDX));
       EMIT4i(OFFSET(&state->m, 4));

       // MOV eax, [rax + rdx]
       EMIT(0x8b);    // MOV
       EMIT(MODRM_SIB);
       EMIT(SIB(0, RAX, RDX));

       // MOV [state->i], eax
       EMIT(0x89);   // MOV
       EMIT(MODRM_RIP_DISP32(RAX));
       EMIT4i(OFFSET(&state->i, 4));
    }

    // Add 4 to PC
    {
       // ADD word ptr [state->pc], 4
       EMIT(P_WORD);
       EMIT(0x83);
       EMIT(MODRM_RIP_DISP32(RAX));
       EMIT4i(OFFSET(&state->pc, 5));
       EMIT(4);
    }

    // Call `op_instr`
    {
       // MOV rax, [op_instr]
       EMIT(REX_W);
       EMIT(0xb8 + RAX);
       EMIT8u(op_instr);
       // CALL rax
       EMIT(0xff);
       EMIT(MODRM_REG_DIRECT(2, RAX));
    }

    // Pop rdi
    EMIT(0x58 + RDI);

    return jit_block;
}

void cpu_rec_compile(cpu_state* state, uint16_t a)
{
    uint8_t *jit_block, *jit_ptr;
    size_t size;
    uint16_t start = a, end, nb_instrs;
    int ret, found_branch = 0;

    for (end = start; end < UINT16_MAX; end += 4)
    {
        uint8_t op = state->m[end];
        
        // RET - we know we hit the end of a subroutine, so return.
        // CALL - we transfer to another basic block, so return.
        // JMP
        if ((op & 0xf0) == 0x10) {
            found_branch = 1;
            break;
        }
    }
    end += 4*found_branch;
    nb_instrs = (end - a) / 4;

    size = ROUNDUP(nb_instrs * 64, 8);
    jit_block = cpu_rec_get_memblk(state, size);
    void* page = (void *)((uintptr_t)jit_block & ~(sysconf(_SC_PAGESIZE) - 1));
    size_t sizeup = ROUNDUP(((jit_block + size) - (uint8_t *)page), sysconf(_SC_PAGESIZE));
    if (mprotect(page, sizeup, PROT_READ | PROT_WRITE) < 0) {
        fprintf(stderr, "mprotect(%p, %zu, PROT_READ | PROT_WRITE) failed with errno %d.\n",
                jit_block, size, errno);
        exit(1);
    }

    jit_ptr = cpu_rec_compile_start(state, jit_block);
    for (; a < end; a += 4)
    {
        jit_ptr = cpu_rec_compile_instr(state, a, jit_ptr);
    }
    jit_ptr = cpu_rec_compile_end(state, jit_ptr);
    if (mprotect(page, sizeup, PROT_EXEC) < 0) {
        fprintf(stderr, "mprotect(%p, %zu, PROT_EXEC) failed with errno %d.\n",
                jit_block, size, errno);
        exit(1);
    }

    state->rec.bblk_map[start].code = (void (*)(void))(jit_block);
    state->rec.bblk_map[start].size = jit_ptr - jit_block;
    state->rec.bblk_map[start].cycles = nb_instrs;
}
