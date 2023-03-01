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
   memset(&state->rec.host[0], 0, sizeof(state->rec.host));
}

void cpu_rec_free(cpu_state* state)
{
}

void cpu_rec_hostreg_release(cpu_state *state, int hostreg)
{
   state->rec.host[hostreg].use = 0;
}

void cpu_rec_hostreg_release_all(cpu_state *state)
{
   for (int i = 0; i < 16; ++i) {
      switch (state->rec.host[i].use) {
         case 0:
            break;
         case CPU_HOST_REG_C16REG:
         {
            int regPtrState = cpu_rec_hostreg_ptr(state, state);
            EMIT(P_WORD);
            EMIT_REX_RBI(i, regPtrState, 0, 0);
            EMIT(0x89);
            EMIT(MODRM_REG_RMDISP8(i, regPtrState));
            EMIT(STRUCT_OFFSET(state, r[i]));

            break;
         }
         case CPU_HOST_REG_LOCAL:
         {
            if (state->rec.host[i].local) {
               if (strcmp(state->rec.host[i].local, "pc") == 0) {
                  int regPtrState = cpu_rec_hostreg_ptr(state, state);
                  EMIT(P_WORD);
                  EMIT_REX_RBI(i, regPtrState, 0, 0);
                  EMIT(0x89);
                  EMIT(MODRM_REG_RMDISP8(i, regPtrState));
                  EMIT(STRUCT_OFFSET(state, pc));
               } else if (strcmp(state->rec.host[i].local, "sp") == 0) {
                  int regPtrState = cpu_rec_hostreg_ptr(state, state);
                  EMIT(P_WORD);
                  EMIT_REX_RBI(i, regPtrState, 0, 0);
                  EMIT(0x89);
                  EMIT(MODRM_REG_RMDISP8(i, regPtrState));
                  EMIT(STRUCT_OFFSET(state, sp));
               } else if (strcmp(state->rec.host[i].local, "bgc") == 0) {
                  int regPtrState = cpu_rec_hostreg_ptr(state, state);
                  EMIT_REX_RBI(i, regPtrState, 0, 0);
                  EMIT(0x88);
                  EMIT(MODRM_REG_RMDISP8(i, regPtrState));
                  EMIT(STRUCT_OFFSET(state, bgc));
               } else if (strcmp(state->rec.host[i].local, "sw") == 0) {
                  int regPtrState = cpu_rec_hostreg_ptr(state, state);
                  EMIT_REX_RBI(i, regPtrState, 0, 0);
                  EMIT(0x88);
                  EMIT(MODRM_REG_RMDISP8(i, regPtrState));
                  EMIT(STRUCT_OFFSET(state, sw));
               } else if (strcmp(state->rec.host[i].local, "sh") == 0) {
                  int regPtrState = cpu_rec_hostreg_ptr(state, state);
                  EMIT_REX_RBI(i, regPtrState, 0, 0);
                  EMIT(0x88);
                  EMIT(MODRM_REG_RMDISP8(i, regPtrState));
                  EMIT(STRUCT_OFFSET(state, sh));
               } else if (strcmp(state->rec.host[i].local, "fx") == 0) {
                  int regPtrState = cpu_rec_hostreg_ptr(state, state);
                  EMIT_REX_RBI(i, regPtrState, 0, 0);
                  EMIT(0x88);
                  EMIT(MODRM_REG_RMDISP8(i, regPtrState));
                  EMIT(STRUCT_OFFSET(state, fx));
               } else if (strcmp(state->rec.host[i].local, "fy") == 0) {
                  int regPtrState = cpu_rec_hostreg_ptr(state, state);
                  EMIT_REX_RBI(i, regPtrState, 0, 0);
                  EMIT(0x88);
                  EMIT(MODRM_REG_RMDISP8(i, regPtrState));
                  EMIT(STRUCT_OFFSET(state, fy));
               }
               // TODO: Sound-gen registers
            }
            break;
         }
      }
   }
   memset(&state->rec.host[0], 0, sizeof(state->rec.host));
}

void cpu_rec_hostreg_freeze(cpu_state *state, int hostreg)
{
   state->rec.host[hostreg].use = CPU_HOST_REG_FROZEN;
}

int cpu_rec_hostreg_ptr(cpu_state *state, void* ptr)
{
   int lru_time = INT_MAX;
   int lru = 0;
   int reg = 0, i = 0;
   for (; i < 16; ++i) {
      if (state->rec.host[i].use == CPU_HOST_REG_PTR &&
          ptr == state->rec.host[i].ptr) {
         return i;
      } else if (state->rec.host[i].use == 0) {
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

   ptrdiff_t disp = (uint8_t *)ptr - state->rec.jit_p;
   EMIT_REX_RBI(REG_NONE, reg, REG_NONE, 1);
   if (disp <= INT32_MAX && disp >= INT32_MIN) {
      EMIT(0x8d);    // LEA r64, [m]
      EMIT(MODRM_RIP_DISP32(reg));
      EMIT4i(OFFSET(ptr, 4));
   } else {
      EMIT(0xb8 + (reg & 7));    // MOV r64, m
      EMIT8u(ptr);
   }

   state->rec.host[reg].use = CPU_HOST_REG_PTR;
   state->rec.host[reg].ptr = ptr;
   return reg;
}

int cpu_rec_hostreg_local(cpu_state *state, const char *name)
{
   int lru_time = INT_MAX;
   int lru = 0;
   for (int i = 0; i < 16; ++i) {
      if (name != NULL &&
          state->rec.host[i].use == CPU_HOST_REG_LOCAL &&
          state->rec.host[i].local &&
          strcmp(name, state->rec.host[i].local) == 0) {
         return i;
      } else if (state->rec.host[i].use == 0) {
         state->rec.host[i].use = CPU_HOST_REG_LOCAL;
         state->rec.host[i].local = name;
         return i;
      }
      if (state->rec.host[i].last_access_time < lru_time) {
         lru_time = state->rec.host[i].last_access_time;
         lru = i;
      }
   }

   panic("%s: LRU: eviction not implemented!", __FUNCTION__);
   state->rec.host[lru].use = CPU_HOST_REG_LOCAL;
   state->rec.host[lru].local = name;
   return lru;
}

int cpu_rec_hostreg_c16reg(cpu_state *state, int c16reg)
{
   int lru_time = INT_MAX;
   int lru = 0;
   for (int i = 0; i < 16; ++i) {
      if (state->rec.host[i].use == CPU_HOST_REG_C16REG &&
          c16reg == state->rec.host[i].c16reg) {
         return i;
      } else if (state->rec.host[i].use == 0) {
         state->rec.host[i].use = CPU_HOST_REG_C16REG;
         state->rec.host[i].c16reg = c16reg;
         return i;
      }
      if (state->rec.host[i].last_access_time < lru_time) {
         lru_time = state->rec.host[i].last_access_time;
         lru = i;
      }
   }

   panic("%s: LRU: eviction not implemented!", __FUNCTION__);
   state->rec.host[lru].use = CPU_HOST_REG_C16REG;
   state->rec.host[lru].c16reg = c16reg;
   return lru;
}

/*
 * Write the necessary boilerplate to save registers, setup stack, etc.
 * - `mov rdi, qword ptr [state]` for all opcode calls
 */
static void cpu_rec_compile_start(cpu_state *state, uint16_t a)
{
    cpu_rec_hostreg_freeze(state, RSP);
    cpu_rec_hostreg_freeze(state, RBP);
    int regPtrState = cpu_rec_hostreg_ptr(state, state);
    int regPc = cpu_rec_hostreg_local(state, "pc");

    // MOV eax, a
    EMIT_REX_RBI(REG_NONE, regPc, REG_NONE, 0);
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
 * The CPU JIT currently does partial recompilation, in the sense it does not
 * generate code for the logic of each individual opcode; rather, it chains
 * them together in a sequence of straight-line calls. (A.K.A. a "threaded
 * interpreter").
 */

static void cpu_rec_compile_instr(cpu_state *state, uint16_t a)
{
    void *op_instr = op_table[state->m[a]];
    int regPc = cpu_rec_hostreg_local(state, "pc");
    
    // Copy instruction 32 bits to state
    state->i = *(instr*)(&state->m[a]);

    // Add 4 to PC
    // ADD eax, 4
    EMIT_REX_RBI(regPc, REG_NONE, REG_NONE, 0);
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

    size = ROUNDUP(nb_instrs * 80, 8);
    jit_ptr = cpu_rec_get_memblk(state, size);
    void* page = (void *)((uintptr_t)jit_ptr & ~(sysconf(_SC_PAGESIZE) - 1));
    size_t sizeup = ROUNDUP(((jit_ptr + size) - (uint8_t *)page), sysconf(_SC_PAGESIZE));
    if (mprotect(page, sizeup, PROT_READ | PROT_WRITE) < 0) {
        fprintf(stderr, "mprotect(%p, %zu, PROT_READ | PROT_WRITE) failed with errno %d.\n",
                state->rec.jit_p, size, errno);
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
                state->rec.jit_p, size, errno);
        exit(1);
    }

    state->rec.bblk_map[start].code = (void (*)(void))(jit_ptr);
    state->rec.bblk_map[start].size = state->rec.jit_p - jit_ptr;
    state->rec.bblk_map[start].cycles = nb_instrs;
}
