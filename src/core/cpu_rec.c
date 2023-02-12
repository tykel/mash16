#include "cpu.h"
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>

#define ROUNDUP(n,d) ((((n)+(d)-1) / (d)) * (d))

#define DISP32(rip,ptr) ((ptr)-(rip))
#define MODRM_eip_disp32_to_rm(dst) (0x00 | ((dst)<<3) | 0x5)
#define MODRM_rm_to_rm(src,dst) (0xc0 | ((src)<<3) | (dst))
#define MODRM_sib() (0x4)

#define SIB(s,i,b) (((s)<<6) | ((i)<<3) | (b))

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
       *jit_block++ = 0x48;    // REX.w
       *jit_block++ = 0x8d;    // LEA r64, m
       *jit_block++ = MODRM_eip_disp32_to_rm(RDI);
       *(int32_t *)jit_block = (int32_t)(offs_state);
       jit_block += sizeof(int32_t);
    } else {
       // MOV rdi, [state]
       *jit_block++ = 0x48;   // REX.w
       *jit_block++ = 0xb8 + RDI;   // MOV r64, imm64
       *(cpu_state **)jit_block = state;
       jit_block += sizeof(cpu_state*);
    }
    
    return jit_block;
}

/* Write the necessary boilerplate to restore registers, and return. */
static uint8_t* cpu_rec_compile_end(cpu_state *state, uint8_t* jit_block)
{
    *jit_block++ = 0xc3;    // ret
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
    *jit_block++ = 0x50 + RDI;
    
    // Copy instruction 32 bits to state
    // state->i = *(instr*)(&state->m[state->pc]);
    {
       ptrdiff_t offs_pc = (void *)&state->pc - (void *)(jit_block + 8);
       // MOVZX rax, word [state->pc]
       *jit_block++ = 0x48;   // REX.w
       *jit_block++ = 0x0f;   // MOVZX
       *jit_block++ = 0xb7;   // MOVZX
       *jit_block++ = MODRM_eip_disp32_to_rm(RAX);
       *(int32_t *)jit_block = (int32_t)(offs_pc);
       jit_block += sizeof(int32_t);
       
       ptrdiff_t offs_m = (void *)&state->m - (void *)(jit_block + 7);
       // MOV rdx, [state->m]
       *jit_block++ = 0x48;    // REX.w
       *jit_block++ = 0x8b;    // MOV
       *jit_block++ = MODRM_eip_disp32_to_rm(RDX);
       *(int32_t *)jit_block = (int32_t)(offs_m);
       jit_block += sizeof(int32_t);

       // MOV eax, [rax + rdx]
       *jit_block++ = 0x8b;    // MOV
       *jit_block++ = MODRM_sib();
       *jit_block++ = SIB(0, RAX, RDX);

       ptrdiff_t offs_i = (void *)&state->i - (void *)(jit_block + 6);
       // MOV [state->i], eax
       *jit_block++ = 0x89;   // MOV
       *jit_block++ = MODRM_eip_disp32_to_rm(RAX);
       *(int32_t *)jit_block = (int32_t)(offs_i);
       jit_block += sizeof(int32_t);
    }

    // Add 4 to PC
    {
       // ADD word ptr [state->pc], 4
       ptrdiff_t offs_pc = (void *)&state->pc - (void *)(jit_block + 8);
       *jit_block++ = 0x66;   // Operand prefix - word
       *jit_block++ = 0x83;   // ADD
       *jit_block++ = MODRM_eip_disp32_to_rm(RAX);
       *(int32_t *)jit_block = (int32_t)offs_pc;
       jit_block += sizeof(int32_t);
       *jit_block++ = 4;
    }

    // Call `op_instr`
    {
       // MOV rax, [op_instr]
       *jit_block++ = 0x48;         // REX.w
       *jit_block++ = 0xb8 + RAX;   // LEA
       *(void**)jit_block = op_instr;
       jit_block += sizeof(void*);
       // CALL rax
       *jit_block++ = 0xff;   // CALL
       *jit_block++ = MODRM_rm_to_rm(2,RAX);
    }

    // Pop rdi
    *jit_block++ = 0x58 + RDI;

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
    state->rec.bblk_map[start].size = size;
    state->rec.bblk_map[start].cycles = nb_instrs;
}
