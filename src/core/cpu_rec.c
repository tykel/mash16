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
   memset(&state->rec.host[0], 0, sizeof(state->rec.host[0]) * 16);
}

void cpu_rec_free(cpu_state* state)
{
}

void cpu_rec_hostreg_release(cpu_state *state, int i)
{
   if (state->rec.host[i].use == CPU_HOST_REG_VAR &&
       state->rec.host[i].ptr != NULL &&
       (state->rec.host[i].flags & CPU_VAR_WRITE)) {
      ptrdiff_t disp = (uint8_t *)state->rec.host[i].ptr - state->rec.jit_p;
      int size = state->rec.host[i].size;
      int ptr64 = 0;
      int regPtr = REG_NONE;
      
      if (disp > INT32_MAX || disp < INT32_MIN) {
         regPtr = cpu_rec_hostreg_var(state, NULL, QWORD, 0);
         ptr64 = 1;

         //printf("%s: reg %d: ptr %p\n", __FUNCTION__, i, state->rec.host[i].ptr);
         EMIT_REX_RBI(REG_NONE, regPtr, REG_NONE, QWORD);
         EMIT(0xb8 + (regPtr & 7));
         EMIT8u(state->rec.host[i].ptr);
      }
      
      switch (size) {
      case 1:
         EMIT_REX_RBI(i, regPtr, REG_NONE, size);
         EMIT(0x88);
         break;
      case 2:
         EMIT(P_WORD);
      case 4:
      case 8:
         EMIT_REX_RBI(i, regPtr, REG_NONE, size);
         EMIT(0x89);
         break;
      default:
         panic("host reg %d: invalid size %d!\n", i, size);
      }

      if (ptr64) {
         EMIT(MODRM_REG_RM(i, regPtr));
         cpu_rec_hostreg_release(state, regPtr);
      } else {
         EMIT(MODRM_RIP_DISP32(i));
         EMIT4i(OFFSET(state->rec.host[i].ptr, 4));
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

   ++state->rec.time;

   for (; i < 16; ++i) {
      if (state->rec.host[i].use == CPU_HOST_REG_VAR &&
          ptr == state->rec.host[i].ptr) {
         state->rec.host[i].flags |= flags;
         return i;
      }
   }
   if (size != 1 && size != 2 && size != 4 && size != 8) {
      panic("host reg %d: invalid size %d!\n", i, size);
   }
   for (i = 0; i < 16; ++i) {
      if (state->rec.host[i].use == 0) {
         break;
      }
      if (state->rec.host[i].use == CPU_HOST_REG_VAR &&
          state->rec.host[i].last_access_time < lru_time) {
         lru_time = state->rec.host[i].last_access_time;
         lru = i;
      }
   }

   reg = i < 16 ? i : lru;

   if (i == 16) {
      cpu_rec_hostreg_release(state, reg);
   }

   if (flags & CPU_VAR_ADDRESS_OF) {
      if (size != QWORD) {
         panic("%s: address not 64-bits wide!", __FUNCTION__);
      }

      ptrdiff_t disp = (uint8_t*)ptr - state->rec.jit_p;
      if (disp <= INT32_MAX && disp >= INT32_MIN) {
         // LEA reg, qword [ptr]
         EMIT_REX_RBI(reg, REG_NONE, REG_NONE, QWORD);
         EMIT(0x8d);
         EMIT(MODRM_RIP_DISP32(reg));
         EMIT4i(OFFSET(ptr, 4));
      } else {
         // MOV reg, qword ptr
         EMIT_REX_RBI(REG_NONE, reg, REG_NONE, QWORD);
         EMIT(0xb8 + (reg & 7));
         EMIT8u(ptr);
      }
   
   } else {
      if (flags & CPU_VAR_READ) {
         if (size < 4) {
            // XOR reg, reg
            EMIT_REX_RBI(reg, reg, REG_NONE, DWORD);
            EMIT(0x33);
            EMIT(MODRM_REG_DIRECT(reg, reg));
         }

         // MOV reg, [byte|word|dword|qword] ptr
         ptrdiff_t disp = (uint8_t *)ptr - state->rec.jit_p;
         int ptr64 = 0;
         int regPtr = REG_NONE;
         
         if (disp > INT32_MAX || disp < INT32_MIN) {
            regPtr = cpu_rec_hostreg_var(state, NULL, QWORD, 0);
            ptr64 = 1;

            EMIT_REX_RBI(REG_NONE, regPtr, REG_NONE, QWORD);
            EMIT(0xb8 + (regPtr & 7));
            EMIT8u(ptr);
         }
         
         switch (size) {
         case 1:
            EMIT_REX_RBI(reg, regPtr, REG_NONE, size);
            EMIT(0x8a);
            break;
         case 2:
            EMIT(P_WORD);
         case 4:
         case 8:
            EMIT_REX_RBI(reg, regPtr, REG_NONE, size);
            EMIT(0x8b);
            break;
         }

         if (ptr64) {
            EMIT(MODRM_REG_RM(reg, regPtr));
            cpu_rec_hostreg_release(state, regPtr);
         } else {
            EMIT(MODRM_RIP_DISP32(reg));
            EMIT4i(OFFSET(ptr, 4));
         }
      }
   }

   //printf("%s: reg %d: ptr %p\n", __FUNCTION__, reg, ptr);
   state->rec.host[reg].use = CPU_HOST_REG_VAR;
   state->rec.host[reg].ptr = ptr;
   state->rec.host[reg].size = size;
   state->rec.host[reg].flags = flags;
   state->rec.host[reg].last_access_time = state->rec.time;
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
 * If the specified write address is in a basic block we've already JIT'd,
 * then invalidate it, as the source instructions (may) have been modified.
 */
void cpu_rec_invalidate_bblk(cpu_state *state, uint16_t a)
{
   for (int i = a; i >= 0; i -= 4) {
      cpu_rec_bblk *b = &state->rec.bblk_map[i];
      // If the write is in the bblk currently being compiled, just mark it for
      // deletion after it is run.
      if (i == state->rec.bblk_pc0) {
         printf("> mark bblk @ 0x%04x for deletion after next run\n", i);
         state->rec.bblk_invalidate = 1;
      }
      if (b->cycles == 0) {
         continue;
      } else if (i + (b->cycles * 4) < a) {
         break;
      } else if (b->code != NULL) {
         printf("> invalidate bblk @ 0x%04x [%p] (dirty @ 0x%04x)\n",
                i, b->code, a);
         memset(b, 0, sizeof(*b));
      }
   }
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
    
    // Copy instruction 32 bits to state
    state->i = *(instr*)(&state->m[a]);
    // MOV reg, instr
    int regInstr = HOSTREG_STATE_VAR_W(i, DWORD);
    EMIT_REX_RBI(REG_NONE, regInstr, REG_NONE, DWORD);
    EMIT(0xb8 + (regInstr & 7));
    EMIT4u(state->i.dword);

    // Add 4 to PC
    int regPc = HOSTREG_STATE_VAR_RW(pc, WORD);
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

    state->rec.bblk_pc0 = start;
    state->rec.bblk_pcN = end;

    size = ROUNDUP(nb_instrs * 64, 8);
    jit_ptr = cpu_rec_get_memblk(state, size);
    printf("> ... bblk->code @ %p\n", jit_ptr);
    void* page = (void *)((uintptr_t)jit_ptr & ~(sysconf(_SC_PAGESIZE) - 1));
    size_t sizeup = ROUNDUP(((jit_ptr + size) - (uint8_t *)page), sysconf(_SC_PAGESIZE));
    if (mprotect(page, sizeup, PROT_READ | PROT_WRITE) < 0) {
        fprintf(stderr, "mprotect(%p, %zu, rw) failed with errno %d.\n",
                page, sizeup, errno);
        exit(1);
    }

    uint8_t *jit_end = jit_ptr + size;
    state->rec.jit_p = jit_ptr;
    cpu_rec_compile_start(state, a);
    for (; a < end; a += 4)
    {
        if ((jit_end - state->rec.jit_p) <= 16) {
           size_t size2 = size * 2;
           state->rec.jit_next += size2 - size;
           size_t sizeup2 = ROUNDUP(((jit_ptr + size2) - (uint8_t*)page),
                                    sysconf(_SC_PAGESIZE));
           printf("> grow block from %zu to %zu bytes\n", size, size2);
           if (sizeup2 > sizeup) {
              if (mprotect(page, sizeup2, PROT_READ | PROT_WRITE) < 0) {
                 fprintf(stderr, "mprotect(%p, %zu, rw) failed with errno %d.\n",
                         page, sizeup2, errno);
                 exit(1);
              }
              sizeup = sizeup2;
           }
           size = size2;
        }
        cpu_rec_compile_instr(state, a);
    }
    cpu_rec_compile_end(state);
    if (mprotect(page, sizeup, PROT_EXEC) < 0) {
        fprintf(stderr, "mprotect(%p, %zu, x) failed with errno %d.\n",
                page, sizeup, errno);
        exit(1);
    }

    state->rec.bblk_map[start].code = (void (*)(void))(jit_ptr);
    state->rec.bblk_map[start].size = state->rec.jit_p - jit_ptr;
    state->rec.bblk_map[start].cycles = nb_instrs;
    state->rec.bblk_map[start].dirty = state->rec.bblk_invalidate;
    printf("> ... reserved %zu, final size: %zu bytes\n",
           size, state->rec.bblk_map[start].size);
    state->rec.bblk_invalidate = 0;
}
