#include <errno.h>
#include <limits.h>
#include <sys/mman.h>
#include <unistd.h>

#include "cpu.h"
#include "cpu_rec_ops.h"

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
   memset(state->rec.jit_base, 0x90, 16*1024*1024 - (state->rec.jit_base - global_alloc));
   memset(&state->rec.host[0], 0, sizeof(state->rec.host));
}

void cpu_rec_free(cpu_state* state)
{
}

void cpu_rec_hostreg_release(cpu_state *state, int i)
{
   if (state->rec.host[i].use == CPU_HOST_REG_VAR &&
       state->rec.host[i].ptr != NULL &&
       (state->rec.host[i].flags & CPU_VAR_WRITE)) {
      int size = state->rec.host[i].size;
      switch (size) {
      case 1:
         EMIT_REX_RBI(i, REG_NONE, REG_NONE, size);
         EMIT(0x88);
         EMIT(MODRM_RIP_DISP32(i));
         EMIT4i(OFFSET(state->rec.host[i].ptr, 4));
         break;
      case 2:
         EMIT(P_WORD);
      case 4:
      case 8:
         EMIT_REX_RBI(i, REG_NONE, REG_NONE, size);
         EMIT(0x89);
         EMIT(MODRM_RIP_DISP32(i));
         EMIT4i(OFFSET(state->rec.host[i].ptr, 4));
         break;
      }
   }
   state->rec.host[i].use = 0;
}

void cpu_rec_hostreg_release_all(cpu_state *state)
{
   for (int i = 0; i < 16; ++i) {
      cpu_rec_hostreg_release(state, i);
   }
   memset(&state->rec.host[0], 0, sizeof(state->rec.host));
}

void cpu_rec_hostreg_freeze(cpu_state *state, int hostreg)
{
   state->rec.host[hostreg].use = CPU_HOST_REG_FROZEN;
}

int cpu_rec_hostreg_var(cpu_state *state, void* ptr, size_t size, int flags)
{
   int lru_time = INT_MAX;
   int lru = 0;
   int reg = 0, i = 0;
   for (; i < 16; ++i) {
      if (state->rec.host[i].use == CPU_HOST_REG_VAR &&
          ptr == state->rec.host[i].ptr) {
         return i;
      }
   }
   for (i = 0; i < 16; ++i) {
      if (state->rec.host[i].use == 0) {
         break;
      }
      if (state->rec.host[i].last_access_time < lru_time) {
         lru_time = state->rec.host[i].last_access_time;
         lru = i;
      }
   }

   reg = i < 16 ? i : lru;

   if (i == 16) {
      panic("%s: LRU: eviction not implemented!", __FUNCTION__);
   }

   if (flags & CPU_VAR_ADDRESS_OF) {
      if (size != QWORD) {
         panic("%s: address not 64-bits wide!", __FUNCTION__);
      }

      ptrdiff_t disp = (uint8_t*)ptr - state->rec.jit_p;
      if (0 && disp <= INT32_MAX && disp >= INT32_MIN) {
         // LEA reg, qword [ptr]
         EMIT_REX_RBI(i, REG_NONE, REG_NONE, QWORD);
         EMIT(0x8d);
         EMIT(MODRM_RIP_DISP32(i));
         EMIT4i(OFFSET(ptr, 4));
      } else {
         // MOV reg, qword ptr
         EMIT_REX_RBI(REG_NONE, i, REG_NONE, QWORD);
         EMIT(0xb8 + (i & 7));
         EMIT8u(ptr);
      }
   
   } else {
      if (flags & CPU_VAR_READ) {
         if (size < 4) {
            // XOR reg, reg
            EMIT_REX_RBI(i, i, REG_NONE, DWORD);
            EMIT(0x33);
            EMIT(MODRM_REG_DIRECT(i, i));
         }

         // MOV reg, [byte|word|dword|qword] ptr
         switch (size) {
         case 1:
            EMIT_REX_RBI(i, REG_NONE, REG_NONE, size);
            EMIT(0x8a);
            EMIT(MODRM_RIP_DISP32(i));
            EMIT4i(OFFSET(ptr, 4));
            break;
         case 2:
            EMIT(P_WORD);
         case 4:
         case 8:
            EMIT_REX_RBI(i, REG_NONE, REG_NONE, size);
            EMIT(0x8b);
            EMIT(MODRM_RIP_DISP32(i));
            EMIT4i(OFFSET(ptr, 4));
            break;
         }
      }
   }

   state->rec.host[reg].use = CPU_HOST_REG_VAR;
   state->rec.host[reg].ptr = ptr;
   state->rec.host[reg].size = size;
   state->rec.host[reg].flags = flags;
   return reg;
}

/*
 * Ignore the registers which the SysV x86_64 calling preserves across
 * function calls.
 * Otherwise we'd have to push/pop.
 */
void cpu_rec_hostreg_preserve(cpu_state *state)
{
    cpu_rec_hostreg_freeze(state, RBX);
    cpu_rec_hostreg_freeze(state, RSP);
    cpu_rec_hostreg_freeze(state, RBP);
    cpu_rec_hostreg_freeze(state, R12);
    cpu_rec_hostreg_freeze(state, R13);
    cpu_rec_hostreg_freeze(state, R14);
    cpu_rec_hostreg_freeze(state, R15);
}

/*
 * Write the necessary boilerplate to save registers, setup stack, etc.
 * - `mov rdi, qword ptr [state]` for all opcode calls
 */
static void cpu_rec_compile_start(cpu_state *state, uint16_t a)
{
    cpu_rec_hostreg_preserve(state);

    int regPc = HOSTREG_STATE_VAR_W(pc, 2);

    // MOV eax, a
    EMIT_REX_RBI(REG_NONE, regPc, REG_NONE, DWORD);
    EMIT(0xb8 + (regPc & 7));
    EMIT4u(a);
}

/* Write the necessary boilerplate to restore registers, and return. */
static void cpu_rec_compile_end(cpu_state *state)
{
    cpu_rec_hostreg_release_all(state);
    EMIT(0xc3);    // ret
}

/*
 * The CPU JIT currently does one-pass translation with dynamic register
 * allocation. */

static void cpu_rec_compile_instr(cpu_state *state, uint16_t a)
{
    void *op_instr = op_table[state->m[a]];
    int regPc = HOSTREG_STATE_VAR_RW(pc, 2);
    
    // Copy instruction 32 bits to state
    state->i = *(instr*)(&state->m[a]);
    // MOV reg, instr
    int regInstr = HOSTREG_STATE_VAR_W(i, DWORD);
    EMIT_REX_RBI(regInstr, REG_NONE, REG_NONE, DWORD);
    EMIT(0xb8 + (regInstr & 7));
    EMIT4u(state->i.dword);

    // Add 4 to PC
    // ADD eax, 4
    EMIT_REX_RBI(regPc, REG_NONE, REG_NONE, DWORD);
    EMIT(0x83);
    EMIT(MODRM_REG_IMM8(regPc));
    EMIT(4);

    // Call `op_instr`
    cpu_rec_dispatch(state, state->m[a]);
}

void cpu_rec_compile(cpu_state* state, uint16_t a)
{
    uint8_t *jit_ptr;
    size_t size;
    uint16_t start = a, end, nb_instrs;
    int ret, found_branch = 0;

    for (end = start; end < UINT16_MAX; end += 4)
    {
        uint8_t op = state->m[end];
        
        // RET - we know we hit the end of a subroutine, so return.
        // CALL - we transfer to another basic block, so return.
        // JMP - ditto.
        if ((op & 0xf0) == 0x10) {
            found_branch = 1;
            break;
        }
    }
    end += 4*found_branch;
    nb_instrs = (end - a) / 4;

    size = ROUNDUP(nb_instrs * 64, 8);
    jit_ptr = cpu_rec_get_memblk(state, size);
    void* page = (void *)((uintptr_t)jit_ptr & ~(sysconf(_SC_PAGESIZE) - 1));
    size_t sizeup = ROUNDUP(((jit_ptr + size) - (uint8_t *)page), sysconf(_SC_PAGESIZE));
    if (mprotect(page, sizeup, PROT_READ | PROT_WRITE) < 0) {
        fprintf(stderr, "mprotect(%p, %zu, PROT_READ | PROT_WRITE) failed with errno %d.\n",
                page, size, errno);
        exit(1);
    }

    state->rec.jit_p = jit_ptr;
    cpu_rec_compile_start(state, a);
    for (; a < end; a += 4)
    {
        cpu_rec_compile_instr(state, a);
    }
    cpu_rec_compile_end(state);
    if (mprotect(page, sizeup, PROT_EXEC) < 0) {
        fprintf(stderr, "mprotect(%p, %zu, PROT_EXEC) failed with errno %d.\n",
                page, size, errno);
        exit(1);
    }

    state->rec.bblk_map[start].code = (void (*)(void))(jit_ptr);
    state->rec.bblk_map[start].size = state->rec.jit_p - jit_ptr;
    state->rec.bblk_map[start].cycles = nb_instrs;
    printf("> ... reserved %zu, final size: %zu bytes\n",
           size, state->rec.bblk_map[start].size);
}
