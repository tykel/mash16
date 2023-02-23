static void* cpu_rec_op_nop(cpu_state *state, uint8_t *jit_block)
{
   return jit_block;
}

static uint8_t* cpu_rec_op_cls(cpu_state *state, uint8_t *jit_block)
{
   // XORPS xmm0, xmm0
   EMIT(0x0f);
   EMIT(0x57);
   EMIT(MODRM_REG_DIRECT(XMM0, XMM0));

   // MOV rax, 76800
   EMIT(0xb8 + RAX);
   EMIT4i(76800 - 16);
   
   // MOV rsi, [rdi + _offset(state, vm)]
   EMIT(REX_W);
   EMIT(0x8b);
   EMIT(MODRM_REG_RMDISP32(RSI, RDI));
   EMIT4i(STRUCT_OFFSET(state, vm));

   // @MOVDQA [rsi + rax - 16], xmm0
   EMIT(P_WORD);
   EMIT(0x0f);
   EMIT(0x7f);
   EMIT(MODRM_REG_SIB_DISP8(XMM0));
   EMIT(SIB(0, RAX, RSI));
   EMIT((int8_t)-16);

   // SUB eax, 16
   EMIT(0x83);
   EMIT(MODRM_REG_OPX_IMM8(5, RAX));
   EMIT(16);

   // JNZ @
   EMIT(0x75);
   EMIT((int8_t)-11);

   // MOV [rdi + _offset(state, bgc)], 0
   EMIT(0xc6);
   EMIT(MODRM_REG_RMDISP32(RAX, RDI));
   EMIT4i(STRUCT_OFFSET(state, bgc));
   EMIT(0);

   return jit_block;
}

static void* cpu_rec_op_vblnk(cpu_state *state, uint8_t *jit_block)
{
   //state->meta.wait_vblnk = 1;
   // MOV BYTE [rdi + _offset(state, meta.wait_vblnk)], 1
   EMIT(0xc6);
   EMIT(MODRM_REG_RMDISP32(0, RDI));
   EMIT4i(STRUCT_OFFSET(state, meta.wait_vblnk));
   EMIT(1);
   
   return jit_block;
}

static void* cpu_rec_op_bgc(cpu_state *state, uint8_t *jit_block)
{
   //state->bgc = i_n(state->i);
   // ROR edx, 16
   EMIT(0xc1);
   EMIT(MODRM_REG_DIRECT(1, EDX));
   EMIT(16);

   // MOV [RDI, _offset(state, bgc)], DL
   EMIT(0x88);
   EMIT(MODRM_REG_RMDISP8(DL, RDI));
   EMIT(STRUCT_OFFSET(state, bgc));

   return jit_block;
}

static void* cpu_rec_op_spr(cpu_state *state, uint8_t *jit_block)
{
    //state->sw = i_hhll(state->i) & 0x00ff;
    //state->sh = i_hhll(state->i) >> 8;
    // ROR EDX, 16
    EMIT(0xc1);
    EMIT(MODRM_REG_DIRECT(1, EDX));
    EMIT(16);

    if (STRUCT_OFFSET(state, sh) - STRUCT_OFFSET(state, sw) == 1) {
       // MOV [RDI + _offset(state, sw)], DX
       EMIT(P_WORD);
       EMIT(0x89);
       EMIT(MODRM_REG_RMDISP8(DX, RDI));
       EMIT(STRUCT_OFFSET(state, sw));
    } else {
       // MOV [RDI + _offset(state, sw)], DL    // 0x41 offset within struct
       EMIT(0x88);
       EMIT(MODRM_REG_RMDISP8(DL, RDI));
       EMIT(STRUCT_OFFSET(state, sw));

       // MOV [RDI + _offset(state, sh)], DH
       EMIT(0x88);
       EMIT(MODRM_REG_RMDISP8(DH, RDI));
       EMIT(STRUCT_OFFSET(state, sh));
    }

    return jit_block;
}

static void* cpu_rec_op_rnd(cpu_state *state, uint8_t *jit_block)
{
   //state->r[i_yx(state->i) & 0x0f] = rand() % (i_hhll(state->i) + 1);
   // MOV rax, rand
   EMIT(REX_W);
   EMIT(0xb8 + RAX);
   EMIT8u(rand);

   // PUSH rdi
   EMIT(0x50 + RDI);
   
   // PUSH rdx
   EMIT(0x50 + RDX);

   // CALL rax
   EMIT(0xff);
   EMIT(MODRM_REG_DIRECT(2, RAX));

   // POP rcx
   EMIT(0x58 + RCX);

   // POP rdi
   EMIT(0x58 + RDI);

   // ROR ecx, 16
   EMIT(0xc1);
   EMIT(MODRM_REG_DIRECT(1, ECX));
   EMIT(16);

   // AND ecx, 0xffff
   EMIT(0x81);
   EMIT(MODRM_REG_DIRECT(4, ECX));
   EMIT4u(0xffff);

   // INC ecx
   EMIT(0xff);
   EMIT(MODRM_REG_DIRECT(0, ECX));

   // DIV ecx
   EMIT(0xf7);
   EMIT(MODRM_REG_DIRECT(6, ECX));

   // MOVZX rax, BYTE [rdi + _offset(state, i) + 1]
   EMIT(REX_W);
   EMIT(0x0f);
   EMIT(0xb6);
   EMIT(MODRM_REG_RMDISP8(RAX, RDI));
   EMIT(STRUCT_OFFSET(state, i) + 1);

   // MOV [rdi + rax * 2 + _offset(state, r)], dx
   EMIT(P_WORD);
   EMIT(0x89);
   EMIT(MODRM_REG_SIB_DISP8(DX));
   EMIT(SIB(1, RAX, RDI));
   EMIT(STRUCT_OFFSET(state, r));

   return jit_block;
}

static void* cpu_rec_op_flip(cpu_state *state, uint8_t *jit_block)
{
   // ROR edx, 16
   EMIT(0xc1);
   EMIT(MODRM_REG_DIRECT(1, EDX));
   EMIT(16);

   // SHR dh, 1
   EMIT(0xd0);
   EMIT(MODRM_REG_DIRECT(5, DX));

   // ADC dl, 0
   EMIT(0x14);
   EMIT(0);

   // MOV [rdi + _offset(state, fx)], dh
   EMIT(0x88);
   EMIT(MODRM_REG_RMDISP8(DH, RDI));
   EMIT(STRUCT_OFFSET(state, fx));

   // MOV [rdi + _offset(state, fy)], dl
   EMIT(0x88);
   EMIT(MODRM_REG_RMDISP8(DL, RDI));
   EMIT(STRUCT_OFFSET(state, fy));

   return jit_block;
}

static void* cpu_rec_op_jmp_imm(cpu_state *state, uint8_t *jit_block)
{
   // ROR edx, 16
   EMIT(0xc1);
   EMIT(MODRM_REG_DIRECT(1, EDX));
   EMIT(16);

   // MOV [rdi + _offset(state,pc)], dx
   EMIT(P_WORD);
   EMIT(0x89);
   EMIT(MODRM_REG_RMDISP8(DX, RDI));
   EMIT(STRUCT_OFFSET(state, pc));

   return jit_block;
}

static void* cpu_rec_op_ldi_imm(cpu_state *state, uint8_t *jit_block)
{
   // MOVZX eax, dh
   EMIT(0x0f);
   EMIT(0xb6);
   EMIT(MODRM_REG_DIRECT(RAX, DH));

   // ROR edx, 16
   EMIT(0xc1);
   EMIT(MODRM_REG_DIRECT(1, EDX));
   EMIT(16);

   // MOV [rdi + rax*2 + _offset(state,r)], dx
   EMIT(P_WORD);
   EMIT(0x89);
   EMIT(MODRM_REG_SIB_DISP8(DX));
   EMIT(SIB(1, RAX, RDI));
   EMIT(STRUCT_OFFSET(state, r));

   return jit_block;
}

static void* cpu_rec_op_ldi_sp_imm(cpu_state *state, uint8_t *jit_block)
{
   // ROR edx, 16
   EMIT(0xc1);
   EMIT(MODRM_REG_DIRECT(1, EDX));
   EMIT(16);

   // MOV [rdi + _offset(state,sp)], dx
   EMIT(P_WORD);
   EMIT(0x89);
   EMIT(MODRM_REG_RMDISP8(DX, RDI));
   EMIT(STRUCT_OFFSET(state, sp));

   return jit_block;
}

static void* cpu_rec_op_ldm_imm(cpu_state *state, uint8_t *jit_block)
{
   // MOVZX eax, dh
   EMIT(0x0f);
   EMIT(0xb6);
   EMIT(MODRM_REG_DIRECT(RAX, DH));

   // SHR edx, 16
   EMIT(0xc1);
   EMIT(MODRM_REG_DIRECT(5, EDX));
   EMIT(16);

   // MOV rcx, [rdi + _offset(state,m)]
   EMIT(REX_W);
   EMIT(0x8b);
   EMIT(MODRM_REG_RMDISP8(RCX, RDI));
   EMIT(STRUCT_OFFSET(state, m));

   // MOV cx, [rcx + rdx]
   EMIT(P_WORD);
   EMIT(0x8b);
   EMIT(MODRM_REG_SIB(CX));
   EMIT(SIB(0, RDX, RCX));

   // MOV [rdi + rax*2 + _offset(state,r)], cx
   EMIT(P_WORD);
   EMIT(0x89);
   EMIT(MODRM_REG_SIB_DISP8(CX));
   EMIT(SIB(1, RAX, RDI));
   EMIT(STRUCT_OFFSET(state, r));

   return jit_block;
}

static void* cpu_rec_op_ldm(cpu_state *state, uint8_t *jit_block)
{
   // ROR dx, 8
   EMIT(P_WORD);
   EMIT(0xc1);
   EMIT(MODRM_REG_DIRECT(1, DX));
   EMIT(8);

   // MOVZX rax, dx
   EMIT(0x0f);
   EMIT(0xb6);
   EMIT(MODRM_REG_DIRECT(RAX, DX));
   
   // AND rdx, 0xf
   EMIT(0x83);
   EMIT(MODRM_REG_DIRECT(4, RDX));
   EMIT(0xf);

   // SHR al, 4
   EMIT(0xc0);
   EMIT(MODRM_REG_DIRECT(5, AL));
   EMIT(4);

   // AND eax, 0xf
   EMIT(0x83);
   EMIT(MODRM_REG_DIRECT(4, EAX));
   EMIT(0xf);

   // MOVZX cx, word [rdi + rax*2 + _offset(state,r)]
   EMIT(0x0f);
   EMIT(0xb7);
   EMIT(MODRM_REG_SIB_DISP8(CX));
   EMIT(SIB(1, RAX, RDI));
   EMIT(STRUCT_OFFSET(state, r));

   // MOV rax, [rdi + _offset(state,m)]
   EMIT(REX_W);
   EMIT(0x8b);
   EMIT(MODRM_REG_RMDISP8(RAX, RDI));
   EMIT(STRUCT_OFFSET(state, m));

   // MOV cx, word [rax + rcx*1]
   EMIT(P_WORD);
   EMIT(0x8b);
   EMIT(MODRM_REG_SIB(CX));
   EMIT(SIB(0, RCX, RAX));

   // MOV word [rdi + rdx*2 + _offset(state,r)], cx
   EMIT(P_WORD);
   EMIT(0x89);
   EMIT(MODRM_REG_SIB_DISP8(CX));
   EMIT(SIB(1, RDX, RDI));
   EMIT(STRUCT_OFFSET(state, r));

   return jit_block;
}

static void* cpu_rec_op_mov(cpu_state *state, uint8_t *jit_block)
{
   // ROR dx, 8
   EMIT(P_WORD);
   EMIT(0xc1);
   EMIT(MODRM_REG_DIRECT(1, DX));
   EMIT(8);

   // MOVZX rax, dx
   //EMIT(REX_W);
   EMIT(0x0f);
   EMIT(0xb6);
   EMIT(MODRM_REG_DIRECT(RAX, DX));
   
   // AND rdx, 0xf
   //EMIT(REX_W);
   EMIT(0x83);
   EMIT(MODRM_REG_DIRECT(4, RDX));
   EMIT(0xf);

   // SHR al, 4
   EMIT(0xc0);
   EMIT(MODRM_REG_DIRECT(5, AL));
   EMIT(4);

   // AND eax, 0xf
   EMIT(0x83);
   EMIT(MODRM_REG_DIRECT(4, EAX));
   EMIT(0xf);

   // MOV cx, [rdi + rax*2 + _offset(state,r)]
   EMIT(P_WORD);
   EMIT(0x8b);
   EMIT(MODRM_REG_SIB_DISP8(CX));
   EMIT(SIB(1, RAX, RDI));
   EMIT(STRUCT_OFFSET(state, r));
   
   // MOV [rdi + rdx*2 + _offset(state,r)], cx
   EMIT(P_WORD);
   EMIT(0x89);
   EMIT(MODRM_REG_SIB_DISP8(CX));
   EMIT(SIB(1, RDX, RDI));
   EMIT(STRUCT_OFFSET(state, r));

   return jit_block;
}

static void* cpu_rec_op_stm_imm(cpu_state *state, uint8_t *jit_block)
{
   // ROR dx, 8
   EMIT(P_WORD);
   EMIT(0xc1);
   EMIT(MODRM_REG_DIRECT(1, DX));
   EMIT(8);

   // MOVZX eax, dl
   EMIT(0x0f);
   EMIT(0xb6);
   EMIT(MODRM_REG_DIRECT(EAX, DL));

   // AND eax, 0xf
   EMIT(0x83);
   EMIT(MODRM_REG_DIRECT(4, EAX));
   EMIT(0xf);

   // SHR edx, 16
   EMIT(0xc1);
   EMIT(MODRM_REG_DIRECT(5, EDX));
   EMIT(16);

   // MOVZX cx, word [rdi + rax*2 + _offset(state,r)]
   EMIT(0x0f);
   EMIT(0xb7);
   EMIT(MODRM_REG_SIB_DISP8(CX));
   EMIT(SIB(1, RAX, RDI));
   EMIT(STRUCT_OFFSET(state, r));

   // MOV rax, [rdi + _offset(state,m)]
   EMIT(REX_W);
   EMIT(0x8b);
   EMIT(MODRM_REG_RMDISP8(RAX, RDI));
   EMIT(STRUCT_OFFSET(state, m));

   // MOV word [rax + rdx*1], cx
   EMIT(P_WORD);
   EMIT(0x89);
   EMIT(MODRM_REG_SIB(CX));
   EMIT(SIB(0, RDX, RAX));

   return jit_block;
}

static void* cpu_rec_op_stm(cpu_state *state, uint8_t *jit_block)
{
   // ROR dx, 8
   EMIT(P_WORD);
   EMIT(0xc1);
   EMIT(MODRM_REG_DIRECT(1, DX));
   EMIT(8);

   // MOVZX eax, dl
   EMIT(0x0f);
   EMIT(0xb6);
   EMIT(MODRM_REG_DIRECT(EAX, DL));

   // MOV ecx, eax
   EMIT(0x8b);
   EMIT(MODRM_REG_DIRECT(ECX, EAX));

   // SHR al, 4
   EMIT(0xc0);
   EMIT(MODRM_REG_DIRECT(5, AL));
   EMIT(4);

   // AND ecx, 0xf
   EMIT(0x83);
   EMIT(MODRM_REG_DIRECT(4, ECX));
   EMIT(0xf);

   // MOVZX ebx, word [rdi + rcx*2 + _offset(state,r)]
   EMIT(0x0f);
   EMIT(0xb7);
   EMIT(MODRM_REG_SIB_DISP8(EBX));
   EMIT(SIB(1, RCX, RDI));
   EMIT(STRUCT_OFFSET(state, r));

   // MOV rdx, [rdi + _offset(state,m)]
   EMIT(REX_W);
   EMIT(0x8b);
   EMIT(MODRM_REG_RMDISP8(RDX, RDI));
   EMIT(STRUCT_OFFSET(state, m));

   // MOVZX eax, [rdi + rax*2 + _offset(state,r)]
   EMIT(0x0f);
   EMIT(0xb7);
   EMIT(MODRM_REG_SIB_DISP8(EAX));
   EMIT(SIB(1, RAX, RDI));
   EMIT(STRUCT_OFFSET(state, r));

   // MOV word [rdx + rax*1], bx
   EMIT(P_WORD);
   EMIT(0x89);
   EMIT(MODRM_REG_SIB(BX));
   EMIT(SIB(0, RAX, RDX));

   return jit_block;
}

static void* cpu_rec_dispatch(cpu_state *state, uint8_t *jit_block, uint8_t op)
{
   switch (op) {
      case 0x00:  // NOP
         jit_block = cpu_rec_op_nop(state, jit_block);
         break;
      case 0x01:  // CLS
         jit_block = cpu_rec_op_cls(state, jit_block);
         break;
      case 0x02:  // VBLNK
         jit_block = cpu_rec_op_vblnk(state, jit_block);
         break;
      case 0x03:  // BGC
         jit_block = cpu_rec_op_bgc(state, jit_block);
         break;
      case 0x04:  // SPR
         jit_block = cpu_rec_op_spr(state, jit_block);
         break;
      case 0x07:  // RND
         jit_block = cpu_rec_op_rnd(state, jit_block);
         break;
      case 0x08:  // FLIP 
         jit_block = cpu_rec_op_flip(state, jit_block);
         break;
      case 0x10:  // JMP (imm)
         jit_block = cpu_rec_op_jmp_imm(state, jit_block);
         break;
      case 0x20:  // LDI (imm)
         jit_block = cpu_rec_op_ldi_imm(state, jit_block);
         break;
      case 0x21:  // LDI SP (imm)
         jit_block = cpu_rec_op_ldi_sp_imm(state, jit_block);
         break;
      case 0x22:  // LDM (imm)
         jit_block = cpu_rec_op_ldm_imm(state, jit_block);
         break;
      case 0x23:  // LDM 
         jit_block = cpu_rec_op_ldm(state, jit_block);
         break;
      case 0x24:  // MOV 
         jit_block = cpu_rec_op_mov(state, jit_block);
         break;
      case 0x30:  // STM (imm)
         jit_block = cpu_rec_op_stm_imm(state, jit_block);
         break;
      case 0x31:  // STM
         jit_block = cpu_rec_op_stm(state, jit_block);
         break;
      default:
      {
         void *op_instr = (void *)op_table[op];

         // PUSH rdi
         EMIT(0x50 + RDI);
         // MOV rax, [op_instr]
         EMIT(REX_W);
         EMIT(0xb8 + RAX);
         EMIT8u(op_instr);
         // CALL rax
         EMIT(0xff);
         EMIT(MODRM_REG_DIRECT(2, RAX));
         // POP rdi
         EMIT(0x58 + RDI);
         break;
      }
   }
   return jit_block;
}

