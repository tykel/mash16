#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/cpu.h"
#include "options.h"

int use_verbose = 0;

void panic(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    exit(1);
}

char *get_symbol(uint16_t a)
{
    (void)a;
    return NULL;
}

void pause_cpu(void) { }

void print_state(cpu_state* state, uint16_t pc) { (void)state; (void)pc; }

int main(void)
{
    program_opts opts = {0};
    opts.cpu_rec_1bblk_per_op = 1;
    opts.rng_seed = 1;

    /* Create a small ROM: a few NOPs (opcode 0x00) */
    uint8_t *mem = (uint8_t *)calloc(MEM_SIZE, 1);
    if (!mem) {
        fprintf(stderr, "alloc fail\n");
        return 2;
    }
    for (int i = 0; i < 16; ++i) {
        mem[i] = 0x00; /* NOP */
    }

    cpu_state *s = NULL;
    cpu_init(&s, mem, &opts);
    if (!s) {
        fprintf(stderr, "cpu_init failed\n");
        free(mem);
        return 2;
    }

    /* Compile a basic block at 0 */
    cpu_rec_compile(s, 0);

    cpu_rec_bblk *bb = &s->rec.bblk_map[0];
    if (bb->code == NULL || bb->size == 0) {
        fprintf(stderr, "jit compile failed: code=%p size=%zu\n", bb->code,
                bb->size);
        cpu_free(s);
        free(mem);
        return 2;
    }

    /* Execute compiled block and ensure it doesn't crash; compare interpreter */
    /* Run one block in JIT and emulate same with interpreter */
    cpu_state *interp = &s[1];

    /* copy small region for interpreter */
    memcpy(interp->m, s->m, MEM_SIZE);

    /* run JIT block */
    cpu_rec_1bblk(s);
    /* run interpreter until PCs match */
    while (interp->pc != s->pc) {
        cpu_step(interp);
    }

    /* Quick compare of some fields */
    if (memcmp(s, interp, (uint8_t *)&s->m - (uint8_t*)s) != 0) {
        fprintf(stderr, "state mismatch after jit run\n");
        cpu_free(s);
        free(mem);
        return 3;
    }

    cpu_free(s);
    free(mem);
    printf("jit_sanity_test: ok\n");
    return 0;
}
