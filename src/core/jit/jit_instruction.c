
/* 
 * Intermediate representation for decoded instructions, before corresponding
 * host code is emitted.
 */

struct jit_instruction;
typedef struct {
    int opcode;

    int reg_from;
    int reg_opd;
    int reg_to;
    

    struct jit_instrcution *next;
} jit_instruction;
