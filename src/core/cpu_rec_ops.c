#include "cpu.h"
#include "cpu_rec_ops.h"

static void cpu_rec_op_nop(cpu_state *state)
{
}

static uint8_t* cpu_rec_op_cls(cpu_state *state)
{
   int regTemp = HOSTREG_TEMP_VAR();

   // XORPS xmm0, xmm0
   EMIT(0x0f);
   EMIT(0x57);
   EMIT(MODRM_REG_DIRECT(XMM0, XMM0));

   // MOV rax, 76800
   EMIT_REX_RBI(REG_NONE, regTemp, REG_NONE, DWORD);
   EMIT(0xb8 + (regTemp & 7));
   EMIT4i(76800);

   int regPtrVM = HOSTREG_PTR(state->vm);
   uint8_t *branchTarget = state->rec.jit_p;
   // @MOVDQA [rsi + rax - 16], xmm0
   EMIT(P_WORD);
   EMIT_REX_RBI(REG_NONE, regPtrVM, regTemp, DWORD);
   EMIT(0x0f);
   EMIT(0x7f);
   EMIT(MODRM_REG_SIB_DISP8(XMM0));
   EMIT(SIB(0, regTemp, regPtrVM));
   EMIT((int8_t)-16);

   // SUB eax, 16
   EMIT_REX_RBI(REG_NONE, regTemp, REG_NONE, DWORD);
   EMIT(0x83);
   EMIT(MODRM_REG_OPX_IMM8(5, regTemp));
   EMIT(16);

   // JNZ @
   EMIT(0x75);
   EMIT(OFFSET(branchTarget, 1));
   
   int regBgc = HOSTREG_STATE_VAR_W(bgc, BYTE);
   //state->bgc = 0;
   // XOR reg, reg
   EMIT_REX_RBI(regBgc, regBgc, REG_NONE, DWORD);
   EMIT(0x33);
   EMIT(MODRM_REG_DIRECT(regBgc, regBgc));
   

   cpu_rec_hostreg_release(state, regBgc);
   cpu_rec_hostreg_release(state, regPtrVM);
   cpu_rec_hostreg_release(state, regTemp);

   int regPalR0 = HOSTREG_STATE_VAR_R(pal_r0, BYTE);
   int regPalG0 = HOSTREG_STATE_VAR_R(pal_g0, BYTE);
   int regPalB0 = HOSTREG_STATE_VAR_R(pal_b0, BYTE);
   int regPal0 = HOSTREG_STATE_VAR_W(pal[0], DWORD);

   // MOV regPal0, regPalR0
   EMIT_REX_RBI(regPal0, regPalR0, REG_NONE, DWORD);
   EMIT(0x8b);
   EMIT(MODRM_REG_DIRECT(regPal0, regPalR0));
   

   // SHL regPal0, 8
   EMIT_REX_RBI(REG_NONE, regPal0, REG_NONE, DWORD);
   EMIT(0xc1);
   EMIT(MODRM_REG_DIRECT(4, regPal0));
   EMIT(8);
   // ADD regPal0, regPalG0
   EMIT_REX_RBI(regPal0, regPalG0, REG_NONE, DWORD);
   EMIT(0x02);
   EMIT(MODRM_REG_DIRECT(regPal0, regPalG0));
   // SHL regPal0, 8
   EMIT_REX_RBI(REG_NONE, regPal0, REG_NONE, DWORD);
   EMIT(0xc1);
   EMIT(MODRM_REG_DIRECT(4, regPal0));
   EMIT(8);
   // ADD regPal0, regPalB0
   EMIT_REX_RBI(regPal0, regPalB0, REG_NONE, DWORD);
   EMIT(0x02);
   EMIT(MODRM_REG_DIRECT(regPal0, regPalB0));

   cpu_rec_hostreg_release(state, regPal0);

   int regPalRI0 = HOSTREG_STATE_VAR_W(pal_r[0], BYTE);
   // MOV regPalRI0, regPalR0
   EMIT_REX_RBI(regPalRI0, regPalR0, REG_NONE, DWORD);
   EMIT(0x8b);
   EMIT(MODRM_REG_DIRECT(regPalRI0, regPalR0));

   cpu_rec_hostreg_release(state, regPalRI0);
   cpu_rec_hostreg_release(state, regPalR0);
   
   int regPalGI0 = HOSTREG_STATE_VAR_W(pal_g[0], BYTE);
   // MOV regPalGI0, regPalG0
   EMIT_REX_RBI(regPalGI0, regPalG0, REG_NONE, DWORD);
   EMIT(0x8b);
   EMIT(MODRM_REG_DIRECT(regPalGI0, regPalG0));

   cpu_rec_hostreg_release(state, regPalGI0);
   cpu_rec_hostreg_release(state, regPalG0);
   
   int regPalBI0 = HOSTREG_STATE_VAR_W(pal_b[0], BYTE);
   // MOV regPalBI0, regPalB0
   EMIT_REX_RBI(regPalBI0, regPalB0, REG_NONE, DWORD);
   EMIT(0x8b);
   EMIT(MODRM_REG_DIRECT(regPalBI0, regPalB0));
   

   cpu_rec_hostreg_release(state, regPalBI0);
   cpu_rec_hostreg_release(state, regPalB0);
}

static void cpu_rec_op_vblnk(cpu_state *state)
{
   int regVblnk = HOSTREG_STATE_VAR_W(meta.wait_vblnk, DWORD);
   //state->meta.wait_vblnk = 1;
   // XOR reg, reg
   EMIT_REX_RBI(regVblnk, regVblnk, REG_NONE, DWORD);
   EMIT(0x33);
   EMIT(MODRM_REG_DIRECT(regVblnk, regVblnk));
   // INC reg
   EMIT_REX_RBI(REG_NONE, regVblnk, REG_NONE, DWORD);
   EMIT(0xfe);
   EMIT(MODRM_REG_DIRECT(0, regVblnk));
   

   cpu_rec_hostreg_release(state, regVblnk);
}

static void cpu_rec_op_bgc(cpu_state *state)
{
   int regBgc = HOSTREG_STATE_VAR_W(bgc, BYTE);
   int bgc = i_n(state->i);
   //state->bgc = i_n(state->i);
   EMIT_REX_RBI(REG_NONE, regBgc, REG_NONE, DWORD);
   EMIT(0xc7);
   EMIT(MODRM_REG_IMM8(regBgc));
   EMIT4u(bgc);
   

 
   if (bgc != 0) {
      // MOV regPal0, regPalBgc
      int regPalBgc = HOSTREG_STATE_VAR_R(pal[bgc], DWORD);
      int regPal0 = HOSTREG_STATE_VAR_W(pal[0], DWORD);
      EMIT_REX_RBI(regPal0, regPalBgc, REG_NONE, DWORD);
      EMIT(0x8b);
      EMIT(MODRM_REG_DIRECT(regPal0, regPalBgc));
      cpu_rec_hostreg_release(state, regPalBgc);
      cpu_rec_hostreg_release(state, regPal0);

      // MOV regPalRI0, regPalRBgc
      int regPalRBgc = HOSTREG_STATE_VAR_R(pal_r[bgc], BYTE);
      int regPalRI0 = HOSTREG_STATE_VAR_W(pal_r[0], BYTE);
      EMIT_REX_RBI(regPalRI0, regPalRBgc, REG_NONE, BYTE);
      EMIT(0x8a);
      EMIT(MODRM_REG_DIRECT(regPalRI0, regPalRBgc));
      cpu_rec_hostreg_release(state, regPalRBgc);
      cpu_rec_hostreg_release(state, regPalRI0);

      // MOV regPalGI0, regPalGBgc
      int regPalGBgc = HOSTREG_STATE_VAR_R(pal_g[bgc], BYTE);
      int regPalGI0 = HOSTREG_STATE_VAR_W(pal_g[0], BYTE);
      EMIT_REX_RBI(regPalGI0, regPalGBgc, REG_NONE, BYTE);
      EMIT(0x8a);
      EMIT(MODRM_REG_DIRECT(regPalGI0, regPalGBgc));
      cpu_rec_hostreg_release(state, regPalGBgc);
      cpu_rec_hostreg_release(state, regPalGI0);
      
      // MOV regPalBI0, regPalBBgc
      int regPalBBgc = HOSTREG_STATE_VAR_R(pal_b[bgc], BYTE);
      int regPalBI0 = HOSTREG_STATE_VAR_W(pal_b[0], BYTE);
      EMIT_REX_RBI(regPalBI0, regPalBBgc, REG_NONE, BYTE);
      EMIT(0x8a);
      EMIT(MODRM_REG_DIRECT(regPalBI0, regPalBBgc));
      cpu_rec_hostreg_release(state, regPalBBgc);
      cpu_rec_hostreg_release(state, regPalBI0);
   }

   cpu_rec_hostreg_release(state, regBgc);
}

static void cpu_rec_op_spr(cpu_state *state)
{
    //state->sw = i_hhll(state->i) & 0x00ff;
    //state->sh = i_hhll(state->i) >> 8;

    int regSW = HOSTREG_STATE_VAR_W(sw, BYTE);
   
    EMIT_REX_RBI(REG_NONE, regSW, REG_NONE, BYTE);
    EMIT(0xb8 + (regSW & 7));
    EMIT4u(i_hhll(state->i) & 0xff);
   
    cpu_rec_hostreg_release(state, regSW);

    int regSH = HOSTREG_STATE_VAR_W(sh, BYTE);
   
    EMIT_REX_RBI(REG_NONE, regSH, REG_NONE, BYTE);
    EMIT(0xb8 + (regSH & 7));
    EMIT4u(i_hhll(state->i) >> 8);
   
    cpu_rec_hostreg_release(state, regSH);
}

static void cpu_rec_op_rnd(cpu_state *state)
{
   //state->r[i_yx(state->i) & 0x0f] = rand() % (i_hhll(state->i) + 1);
   cpu_rec_hostreg_release_all(state);
   cpu_rec_hostreg_preserve(state);
   
   int regPtrRand = HOSTREG_PTR(mash16_rand);

   // MOVABS rdi, state
   EMIT_REX_RBI(RDI, REG_NONE, REG_NONE, QWORD);
   EMIT(0xb8 + RDI);
   EMIT8u(state);

   // CALL rax
   EMIT_REX_RBI(REG_NONE, regPtrRand, REG_NONE, DWORD);
   EMIT(0xff);
   EMIT(MODRM_REG_DIRECT(2, regPtrRand));

   cpu_rec_hostreg_release(state, regPtrRand);

   cpu_rec_hostreg_freeze(state, RAX); // rand() result, and DIV
   cpu_rec_hostreg_freeze(state, RDX); // for DIV

   int regDivider = HOSTREG_TEMP_VAR();
   // XOR EDX, EDX
   EMIT(0x33);
   EMIT(MODRM_REG_DIRECT(EDX, EDX));
   // MOV regDivider, i_hhll(state->i) + 1
   EMIT_REX_RBI(REG_NONE, regDivider, REG_NONE, DWORD);
   EMIT(0xb8 + (regDivider & 7));
   EMIT4u(i_hhll(state->i) + 1);
   // DIV regDivider
   EMIT_REX_RBI(REG_NONE, regDivider, REG_NONE, DWORD);
   EMIT(0xf7);
   EMIT(MODRM_REG_DIRECT(6, regDivider));

   int regDst = HOSTREG_STATE_VAR_W(r[i_yx(state->i)], WORD);
   EMIT_REX_RBI(regDst, REG_NONE, REG_NONE, DWORD);
   EMIT(0x8b);
   EMIT(MODRM_REG_DIRECT(regDst, EDX));

   cpu_rec_hostreg_release(state, regDivider);
   cpu_rec_hostreg_release(state, RAX);
   cpu_rec_hostreg_release(state, RDX);
}

static void cpu_rec_op_flip(cpu_state *state)
{
   int regFX = HOSTREG_STATE_VAR_W(fx, BYTE);

   EMIT_REX_RBI(REG_NONE, regFX, REG_NONE, DWORD);
   EMIT(0xb8 + (regFX & 7));
   EMIT4u(i_res(state->i) >> 1);

   cpu_rec_hostreg_release(state, regFX);

   int regFY = HOSTREG_STATE_VAR_W(fy, BYTE);

   EMIT_REX_RBI(REG_NONE, regFY, REG_NONE, DWORD);
   EMIT(0xb8 + (regFY & 7));
   EMIT4u(i_res(state->i) & 1);
   
   cpu_rec_hostreg_release(state, regFX);
}

static void cpu_rec_op_jmp_imm(cpu_state *state)
{
   int regPc = HOSTREG_STATE_VAR_W(pc, WORD);

   EMIT_REX_RBI(REG_NONE, regPc, REG_NONE, DWORD);
   EMIT(0xb8 + (regPc & 7));
   EMIT4u(i_hhll(state->i));
}

static void cpu_rec_op_jmp(cpu_state *state)
{
   int regSrc = HOSTREG_STATE_VAR_R(r[i_yx(state->i)], WORD);
   int regPc = HOSTREG_STATE_VAR_W(pc, WORD);

   EMIT_REX_RBI(regPc, regSrc, REG_NONE, DWORD);
   EMIT(0x8b);
   EMIT(MODRM_REG_DIRECT(regPc, regSrc));
}

static void cpu_rec_op_jx(cpu_state *state)
{
   int regPc = HOSTREG_STATE_VAR_W(pc, WORD);
   int rexNeeded = 0;
   int rexByte = rex(REG_NONE, regPc, REG_NONE, DWORD, &rexNeeded);

   int cond = i_yx(state->i);
   switch (cond) {
   // Z, NZ
   case 0: 
   case 1:
      {
         int regFZ = HOSTREG_STATE_VAR_R(f.z, BYTE);
         // CMP regFZ, cond
         EMIT_REX_RBI(REG_NONE, regFZ, REG_NONE, BYTE);
         EMIT(0x80);
         EMIT(MODRM_REG_OPX_IMM8(7, regFZ));
         EMIT(cond);
         // JZ end
         EMIT(0x74);
         EMIT(rexNeeded+5);
      }
      break;
   // N, NN
   case 2: 
   case 3:
      {
         int regFN = HOSTREG_STATE_VAR_R(f.n, BYTE);
         // CMP regFN, (cond - 2)
         EMIT_REX_RBI(REG_NONE, regFN, REG_NONE, BYTE);
         EMIT(0x80);
         EMIT(MODRM_REG_OPX_IMM8(7, regFN));
         EMIT(cond - 2);
         // JZ end
         EMIT(0x74);
         EMIT(rexNeeded+5);
      }
      break;
   // P
   case 4: 
      {
         int regFN = HOSTREG_STATE_VAR_R(f.n, BYTE);
         int regFZ = HOSTREG_STATE_VAR_R(f.z, BYTE);
         int rexNeededN = 0;
         int rexN = rex(REG_NONE, regFN, REG_NONE, BYTE, &rexNeededN);
         // CMP regFZ, 0
         EMIT_REX_RBI(REG_NONE, regFZ, REG_NONE, BYTE);
         EMIT(0x80);
         EMIT(MODRM_REG_OPX_IMM8(7, regFZ));
         EMIT(0);
         // JNZ end
         EMIT(0x75);
         EMIT(rexNeededN+3 + 2 + rexNeeded+5);
         // CMP regFN, 0
         EMIT_REX_RBI(REG_NONE, regFN, REG_NONE, BYTE);
         EMIT(0x80);
         EMIT(MODRM_REG_OPX_IMM8(7, regFN));
         EMIT(0);
         // JNZ end
         EMIT(0x75);
         EMIT(rexNeeded+5);
      }
      break;
   // O, NO
   case 5:
   case 6:
      {
         int regFO = HOSTREG_STATE_VAR_R(f.o, BYTE);
         // CMP regFO, (cond - 5)
         EMIT_REX_RBI(REG_NONE, regFO, REG_NONE, BYTE);
         EMIT(0x80);
         EMIT(MODRM_REG_OPX_IMM8(7, regFO));
         EMIT(cond - 5);
         // JZ end
         EMIT(0x75);
         EMIT(rexNeeded+5);
      }
      break;
   // A
   case 7:
      {
         int regFC = HOSTREG_STATE_VAR_R(f.c, BYTE);
         int regFZ = HOSTREG_STATE_VAR_R(f.z, BYTE);
         int rexNeededZ = 0;
         int rexZ = rex(REG_NONE, regFZ, REG_NONE, BYTE, &rexNeededZ);

         // CMP regFC, 0
         EMIT_REX_RBI(REG_NONE, regFC, REG_NONE, BYTE);
         EMIT(0x80);
         EMIT(MODRM_REG_OPX_IMM8(7, regFC));
         EMIT(0);
         // JNZ end
         EMIT(0x75);
         EMIT(rexNeededZ+3 + 2 + rexNeeded+5);
         // CMP regFZ, 0
         EMIT_REX_RBI(REG_NONE, regFZ, REG_NONE, BYTE);
         EMIT(0x80);
         EMIT(MODRM_REG_OPX_IMM8(7, regFZ));
         EMIT(0);
         // JNZ end
         EMIT(0x75);
         EMIT(rexNeeded+5);
      }
      break;
   // AE, B
   case 8:
   case 9:
      {
         int regFC = HOSTREG_STATE_VAR_R(f.c, BYTE);
         // CMP regFC, (cond - 8)
         EMIT_REX_RBI(REG_NONE, regFC, REG_NONE, BYTE);
         EMIT(0x80);
         EMIT(MODRM_REG_OPX_IMM8(7, regFC));
         EMIT(cond - 8);
         // JNZ end
         EMIT(0x75);
         EMIT(rexNeeded+5);
      }
      break;
   // BE
   case 10:
      {
         int regFC = HOSTREG_STATE_VAR_R(f.c, BYTE);
         int regFZ = HOSTREG_STATE_VAR_R(f.z, BYTE);
         int rexNeededZ = 0;
         int rexZ = rex(REG_NONE, regFZ, REG_NONE, BYTE, &rexNeededZ);

         // CMP regFC, 1
         EMIT_REX_RBI(REG_NONE, regFC, REG_NONE, BYTE);
         EMIT(0x80);
         EMIT(MODRM_REG_OPX_IMM8(7, regFC));
         EMIT(0);
         // JZ change_pc
         EMIT(0x74);
         EMIT(rexNeededZ+3 + 2);
         // CMP regFZ, 1
         EMIT_REX_RBI(REG_NONE, regFZ, REG_NONE, BYTE);
         EMIT(0x80);
         EMIT(MODRM_REG_OPX_IMM8(7, regFZ));
         EMIT(0);
         // JNZ end
         EMIT(0x75);
         EMIT(rexNeeded+5);
      }
      break;
   // G
   case 11:
      {
         int regFO = HOSTREG_STATE_VAR_R(f.o, BYTE);
         int regFN = HOSTREG_STATE_VAR_R(f.n, BYTE);
         int regFZ = HOSTREG_STATE_VAR_R(f.z, BYTE);
         int rexNeededNO = 0;
         int rexNO = rex(regFN, regFO, REG_NONE, BYTE, &rexNeededNO);
         
         // CMP regFZ, 0
         EMIT_REX_RBI(REG_NONE, regFZ, REG_NONE, BYTE);
         EMIT(0x80);
         EMIT(MODRM_REG_OPX_IMM8(7, regFZ));
         EMIT(0);
         // JNZ end
         EMIT(0x75);
         EMIT(rexNeededNO+2 + 2 + rexNeeded+5);
         // CMP regFO, regFN
         EMIT_REX_RBI(regFN, regFO, REG_NONE, BYTE);
         EMIT(0x38);
         EMIT(MODRM_REG_DIRECT(regFN, regFO));
         // JNZ end
         EMIT(0x75);
         EMIT(rexNeeded+5);
      }
      break;
   // GE
   case 12:
      {
         int regFO = HOSTREG_STATE_VAR_R(f.o, BYTE);
         int regFN = HOSTREG_STATE_VAR_R(f.n, BYTE);
         // CMP regFO, regFN
         EMIT_REX_RBI(regFN, regFO, REG_NONE, BYTE);
         EMIT(0x38);
         EMIT(MODRM_REG_DIRECT(regFN, regFO));
         // JNZ end
         EMIT(0x75);
         EMIT(rexNeeded+5);
      }
      break;
   // L
   case 13:
      {
         int regFO = HOSTREG_STATE_VAR_R(f.o, BYTE);
         int regFN = HOSTREG_STATE_VAR_R(f.n, BYTE);
         // CMP regFO, regFN
         EMIT_REX_RBI(regFN, regFO, REG_NONE, BYTE);
         EMIT(0x38);
         EMIT(MODRM_REG_DIRECT(regFN, regFO));
         // JZ end
         EMIT(0x74);
         EMIT(rexNeeded+5);
      }
      break;
   // LE
   case 14:
      {
         int regFO = HOSTREG_STATE_VAR_R(f.o, BYTE);
         int regFN = HOSTREG_STATE_VAR_R(f.n, BYTE);
         int regFZ = HOSTREG_STATE_VAR_R(f.z, BYTE);
         int rexNeededNO = 0;
         int rexNO = rex(regFN, regFO, REG_NONE, BYTE, &rexNeededNO);

         // CMP regFZ, 1
         EMIT_REX_RBI(REG_NONE, regFZ, REG_NONE, BYTE);
         EMIT(0x80);
         EMIT(MODRM_REG_OPX_IMM8(7, regFZ));
         EMIT(1);
         // JNZ end
         EMIT(0x75);
         EMIT(rexNeededNO+2 + 2 + rexNeeded+5);
         // CMP regFO, regFN
         EMIT_REX_RBI(regFN, regFO, REG_NONE, BYTE);
         EMIT(0x38);
         EMIT(MODRM_REG_DIRECT(regFN, regFO));
         // JZ end
         EMIT(0x74);
         EMIT(rexNeeded+5);
      }
      break;
   }

   // change_pc;
   EMIT_REX_RBI(REG_NONE, regPc, REG_NONE, DWORD);
   EMIT(0xb8 + (regPc & 7));
   EMIT4u(i_hhll(state->i));

   // end:
}

static void cpu_rec_op_ldi_imm(cpu_state *state)
{
   int regDstReg = HOSTREG_STATE_VAR_W(r[i_yx(state->i)], WORD);

   EMIT_REX_RBI(REG_NONE, regDstReg, REG_NONE, DWORD);
   EMIT(0xb8 + (regDstReg & 7));
   EMIT4u(i_hhll(state->i));
}

static void cpu_rec_op_ldi_sp_imm(cpu_state *state)
{
   int regSP = HOSTREG_STATE_VAR_W(sp, WORD); 

   EMIT_REX_RBI(REG_NONE, regSP, REG_NONE, DWORD);
   EMIT(0xb8 + (regSP & 7));
   EMIT4u(i_hhll(state->i));
}

static void cpu_rec_op_ldm_imm(cpu_state *state)
{
   int regDstReg = HOSTREG_STATE_VAR_W(r[i_yx(state->i)], WORD);
   int regPtrM = HOSTREG_PTR(state->m);

   // MOV regDstReg, [regPtrM + HHLL]
   EMIT(P_WORD);
   EMIT_REX_RBI(regDstReg, regPtrM, REG_NONE, WORD);
   EMIT(0x8b);
   EMIT(MODRM_REG_RMDISP32(regDstReg, regPtrM));
   EMIT4i(i_hhll(state->i));

}

static void cpu_rec_op_ldm(cpu_state *state)
{
   int regSrcReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i) >> 4], WORD);
   int regDstReg = HOSTREG_STATE_VAR_W(r[i_yx(state->i) & 0xf], WORD);
   int regPtrM = HOSTREG_PTR(state->m);

   // MOV regDstReg, [regPtrM + regSrcReg]
   EMIT(P_WORD);
   EMIT_REX_RBI(regDstReg, regPtrM, regSrcReg, WORD);
   EMIT(0x8b);
   EMIT(MODRM_REG_SIB(regDstReg));
   EMIT(SIB(0, regSrcReg, regPtrM));
}

static void cpu_rec_op_mov(cpu_state *state)
{
   int regSrcReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i) >> 4], WORD);
   int regDstReg = HOSTREG_STATE_VAR_W(r[i_yx(state->i) & 0xf], WORD);

   // MOV regDstReg, regSrcReg
   EMIT_REX_RBI(regDstReg, regSrcReg, REG_NONE, DWORD);
   EMIT(0x8b);
   EMIT(MODRM_REG_DIRECT(regDstReg, regSrcReg));
}

static void cpu_rec_emit_invalidate(cpu_state *state, int regAddress)
{
   if (regAddress == RCX) {
      int regTemp = HOSTREG_TEMP_VAR();

      // MOV regTemp, regAddress
      EMIT_REX_RBI(regTemp, regAddress, REG_NONE, DWORD);
      EMIT(0x8b);
      EMIT(MODRM_REG_DIRECT(regTemp, regAddress));

      regAddress = regTemp;
   }
   cpu_rec_hostreg_release(state, RCX);
   cpu_rec_hostreg_freeze(state, RCX);
  
   // XOR ecx, ecx
   EMIT(0x33);
   EMIT(MODRM_REG_DIRECT(RCX, RCX));

   int regBits = HOSTREG_TEMP_VAR();
   int regIndex = HOSTREG_TEMP_VAR();

   cpu_rec_hostreg_release(state, regAddress);
   cpu_rec_hostreg_freeze(state, regAddress);

   // MOV regBits, 1
   EMIT_REX_RBI(REG_NONE, regBits, REG_NONE, DWORD);
   EMIT(0xb8 + (regBits & 7));
   EMIT4i(1);
   // SHR regAddress, 1
   EMIT_REX_RBI(REG_NONE, regAddress, REG_NONE, DWORD);
   EMIT(0xd1);
   EMIT(MODRM_REG_DIRECT(5, regAddress));
   // RCL CL, 0
   EMIT(0xd0);
   EMIT(MODRM_REG_DIRECT(2, CL));
   // SHR regAddress, 1
   EMIT_REX_RBI(REG_NONE, regAddress, REG_NONE, DWORD);
   EMIT(0xd1);
   EMIT(MODRM_REG_DIRECT(5, regAddress));
   // RCL CL, 0
   EMIT(0xd0);
   EMIT(MODRM_REG_DIRECT(2, CL));
   // SHR regAddress, 1
   EMIT_REX_RBI(REG_NONE, regAddress, REG_NONE, DWORD);
   EMIT(0xd1);
   EMIT(MODRM_REG_DIRECT(5, regAddress));
   // RCL CL, 0
   EMIT(0xd0);
   EMIT(MODRM_REG_DIRECT(2, CL));
   // SHL regBits, CL
   EMIT_REX_RBI(REG_NONE, regBits, REG_NONE, DWORD);
   EMIT(0xd3);
   EMIT(MODRM_REG_DIRECT(4, regBits));
   // MOVABS regPtrDM, state->rec.dirty_map
   int regPtrDM = HOSTREG_STATE_VAR_R(rec.dirty_map, QWORD);
   // OR [regPtrDM + regAddress], regBits
   EMIT_REX_RBI(regBits, regPtrDM, regAddress, BYTE);
   EMIT(0x08);
   EMIT(MODRM_REG_SIB(regBits));
   EMIT(SIB(0, regAddress, regPtrDM));

   cpu_rec_hostreg_release(state, RCX);
   cpu_rec_hostreg_release(state, regBits);
   cpu_rec_hostreg_release(state, regAddress);
}

static void cpu_rec_op_stm_imm(cpu_state *state)
{
   int regSrcReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i)], WORD);
   int regPtrM = HOSTREG_PTR(state->m);
   uint16_t addr = i_hhll(state->i);

   // MOV [regPtrM + HHLL], regSrcReg
   EMIT(P_WORD);
   EMIT_REX_RBI(regSrcReg, regPtrM, REG_NONE, WORD);
   EMIT(0x89);
   EMIT(MODRM_REG_RMDISP32(regSrcReg, regPtrM));
   EMIT4i(addr);

   int regAddressReg = HOSTREG_TEMP_VAR();
   EMIT_REX_RBI(REG_NONE, regAddressReg, REG_NONE, DWORD);
   EMIT(0xb8 + (regAddressReg & 7));
   EMIT4i(addr);

   cpu_rec_emit_invalidate(state, regAddressReg);
}

static void cpu_rec_op_stm(cpu_state *state)
{
   int regSrcReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i) & 0xf], WORD);
   int regAddressReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i) >> 4], WORD);
   int regPtrM = HOSTREG_PTR(state->m);

   // MOV [regPtrM + regAddressReg], regSrcReg
   EMIT(P_WORD);
   EMIT_REX_RBI(regSrcReg, regPtrM, regAddressReg, WORD);
   EMIT(0x89);
   EMIT(MODRM_REG_SIB(regSrcReg));
   EMIT(SIB(0, regAddressReg, regPtrM));

   cpu_rec_emit_invalidate(state, regAddressReg);
}

static void cpu_rec_flag_c(cpu_state *state, int regFC)
{
   // SETC regFC
   EMIT_REX_RBI(REG_NONE, regFC, REG_NONE, BYTE);
   EMIT(0x0f);
   EMIT(0x92);
   EMIT(MODRM_REG_DIRECT(0, regFC));
}

static void cpu_rec_flag_z(cpu_state *state, int regFZ)
{
   // SETZ regFZ
   EMIT_REX_RBI(REG_NONE, regFZ, REG_NONE, BYTE);
   EMIT(0x0f);
   EMIT(0x94);
   EMIT(MODRM_REG_DIRECT(0, regFZ));
}

static void cpu_rec_flag_o(cpu_state *state, int regFO)
{
   // SETO regFO
   EMIT_REX_RBI(REG_NONE, regFO, REG_NONE, BYTE);
   EMIT(0x0f);
   EMIT(0x90);
   EMIT(MODRM_REG_DIRECT(0, regFO));
}

static void cpu_rec_flag_n(cpu_state *state, int regFN)
{
   // SETS regFN
   EMIT_REX_RBI(REG_NONE, regFN, REG_NONE, BYTE);
   EMIT(0x0f);
   EMIT(0x98);
   EMIT(MODRM_REG_DIRECT(0, regFN));
}

static void cpu_rec_op_addi(cpu_state *state)
{
   int regSrcReg = HOSTREG_STATE_VAR_RW(r[i_yx(state->i)], WORD);
   int regFC = HOSTREG_STATE_VAR_W(f.c, BYTE);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFO = HOSTREG_STATE_VAR_W(f.o, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // ADD regSrcReg, imm16
   EMIT(P_WORD);
   EMIT_REX_RBI(REG_NONE, regSrcReg, REG_NONE, WORD);
   EMIT(0x81);
   EMIT(MODRM_REG_OPX_IMM8(0, regSrcReg));
   EMIT2u(i_hhll(state->i));

   cpu_rec_flag_c(state, regFC);
   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_o(state, regFO);
   cpu_rec_flag_n(state, regFN);
}

static void cpu_rec_op_add(cpu_state *state)
{
   int regSrcReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i) >> 4], WORD);
   int regDstReg = HOSTREG_STATE_VAR_RW(r[i_yx(state->i) & 0x0f], WORD);
   int regFC = HOSTREG_STATE_VAR_W(f.c, BYTE);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFO = HOSTREG_STATE_VAR_W(f.o, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // ADD regDstReg, regSrcReg
   EMIT(P_WORD);
   EMIT_REX_RBI(regDstReg, regSrcReg, REG_NONE, WORD);
   EMIT(0x03);
   EMIT(MODRM_REG_DIRECT(regDstReg, regSrcReg));

   cpu_rec_flag_c(state, regFC);
   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_o(state, regFO);
   cpu_rec_flag_n(state, regFN);
}

static void cpu_rec_op_add_r3(cpu_state *state)
{
   int regXReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i) & 0x0f], WORD);
   int regYReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i) >> 4], WORD);
   int regZReg = HOSTREG_STATE_VAR_W(r[i_z(state->i)], WORD);
   int regFC = HOSTREG_STATE_VAR_W(f.c, BYTE);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFO = HOSTREG_STATE_VAR_W(f.o, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // MOV regZReg, regXReg
   EMIT_REX_RBI(regZReg, regXReg, REG_NONE, DWORD);
   EMIT(0x8b);
   EMIT(MODRM_REG_DIRECT(regZReg, regXReg));
   // ADD regZReg, regYReg
   EMIT(P_WORD);
   EMIT_REX_RBI(regZReg, regYReg, REG_NONE, WORD);
   EMIT(0x03);
   EMIT(MODRM_REG_DIRECT(regZReg, regYReg));

   cpu_rec_flag_c(state, regFC);
   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_o(state, regFO);
   cpu_rec_flag_n(state, regFN);
}

static void cpu_rec_op_subi(cpu_state *state)
{
   int regSrcReg = HOSTREG_STATE_VAR_RW(r[i_yx(state->i)], WORD);
   int regFC = HOSTREG_STATE_VAR_W(f.c, BYTE);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFO = HOSTREG_STATE_VAR_W(f.o, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // SUB regSrcReg, imm16
   EMIT(P_WORD);
   EMIT_REX_RBI(REG_NONE, regSrcReg, REG_NONE, WORD);
   EMIT(0x81);
   EMIT(MODRM_REG_OPX_IMM8(5, regSrcReg));
   EMIT2u(i_hhll(state->i));

   cpu_rec_flag_c(state, regFC);
   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_o(state, regFO);
   cpu_rec_flag_n(state, regFN);
}

static void cpu_rec_op_sub(cpu_state *state)
{
   int regDstReg = HOSTREG_STATE_VAR_RW(r[i_yx(state->i) & 0x0f], WORD);
   int regSrcReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i) >> 4], WORD);
   int regFC = HOSTREG_STATE_VAR_W(f.c, BYTE);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFO = HOSTREG_STATE_VAR_W(f.o, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // SUB regDstReg, regSrcReg
   EMIT(P_WORD);
   EMIT_REX_RBI(regDstReg, regSrcReg, REG_NONE, WORD);
   EMIT(0x2b);
   EMIT(MODRM_REG_DIRECT(regDstReg, regSrcReg));

   cpu_rec_flag_c(state, regFC);
   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_o(state, regFO);
   cpu_rec_flag_n(state, regFN);
}

static void cpu_rec_op_sub_r3(cpu_state *state)
{
   int regXReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i) & 0x0f], WORD);
   int regYReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i) >> 4], WORD);
   int regZReg = HOSTREG_STATE_VAR_W(r[i_z(state->i)], WORD);
   int regFC = HOSTREG_STATE_VAR_W(f.c, BYTE);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFO = HOSTREG_STATE_VAR_W(f.o, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // MOV regZReg, regXReg
   EMIT_REX_RBI(regZReg, regXReg, REG_NONE, DWORD);
   EMIT(0x8b);
   EMIT(MODRM_REG_DIRECT(regZReg, regXReg));
   // SUB regZReg, regYReg
   EMIT(P_WORD);
   EMIT_REX_RBI(regZReg, regYReg, REG_NONE, WORD);
   EMIT(0x2b);
   EMIT(MODRM_REG_DIRECT(regZReg, regYReg));

   cpu_rec_flag_c(state, regFC);
   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_o(state, regFO);
   cpu_rec_flag_n(state, regFN);
}

static void cpu_rec_op_cmpi(cpu_state *state)
{
   int regSrcReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i)], WORD);
   int regFC = HOSTREG_STATE_VAR_W(f.c, BYTE);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFO = HOSTREG_STATE_VAR_W(f.o, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // CMP regSrcReg, imm16
   EMIT(P_WORD);
   EMIT_REX_RBI(REG_NONE, regSrcReg, REG_NONE, WORD);
   EMIT(0x81);
   EMIT(MODRM_REG_OPX_IMM8(7, regSrcReg));
   EMIT2u(i_hhll(state->i));

   cpu_rec_flag_c(state, regFC);
   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_o(state, regFO);
   cpu_rec_flag_n(state, regFN);
}

static void cpu_rec_op_cmp(cpu_state *state)
{
   int regXReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i) & 0x0f], WORD);
   int regYReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i) >> 4], WORD);
   int regFC = HOSTREG_STATE_VAR_W(f.c, BYTE);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFO = HOSTREG_STATE_VAR_W(f.o, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // CMP regXReg, regYReg
   EMIT(P_WORD);
   EMIT_REX_RBI(regXReg, regYReg, REG_NONE, WORD);
   EMIT(0x3b);
   EMIT(MODRM_REG_DIRECT(regXReg, regYReg));

   cpu_rec_flag_c(state, regFC);
   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_o(state, regFO);
   cpu_rec_flag_n(state, regFN);
}

static void cpu_rec_op_andi(cpu_state *state)
{
   int regSrcReg = HOSTREG_STATE_VAR_RW(r[i_yx(state->i)], WORD);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // AND regSrcReg, imm16
   EMIT(P_WORD);
   EMIT_REX_RBI(REG_NONE, regSrcReg, REG_NONE, WORD);
   EMIT(0x81);
   EMIT(MODRM_REG_OPX_IMM8(4, regSrcReg));
   EMIT2u(i_hhll(state->i));

   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_n(state, regFN);
}

static void cpu_rec_op_muli(cpu_state *state)
{
   int regSrcReg = HOSTREG_STATE_VAR_RW(r[i_yx(state->i)], WORD);
   int regFC = HOSTREG_STATE_VAR_W(f.c, BYTE);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // IMUL regSrcReg, regSrcReg, imm16
   EMIT(P_WORD);
   EMIT_REX_RBI(regSrcReg, regSrcReg, REG_NONE, WORD);
   EMIT(0x69);
   EMIT(MODRM_REG_DIRECT(regSrcReg, regSrcReg));
   EMIT2u(i_hhll(state->i));

   cpu_rec_flag_c(state, regFC);

   // CMP regSrcReg, 0
   EMIT(P_WORD);
   EMIT_REX_RBI(REG_NONE, regSrcReg, REG_NONE, DWORD);
   EMIT(0x83);
   EMIT(MODRM_REG_DIRECT(7, regSrcReg));
   EMIT(0);

   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_n(state, regFN);
}

static void cpu_rec_op_div_r3(cpu_state *state)
{
   // If this reg was cached, flush it
   int regSrcXReg = cpu_rec_hostreg_var(state, &state->r[i_yx(state->i) & 0x0f], WORD, 0);
   cpu_rec_hostreg_release(state, regSrcXReg);

   cpu_rec_hostreg_release(state, RCX);
   cpu_rec_hostreg_freeze(state, RCX);
   
   cpu_rec_hostreg_release(state, RDX);
   cpu_rec_hostreg_freeze(state, RDX);
   
   // Now RX is in EAX.
   cpu_rec_hostreg_release(state, RAX);
   regSrcXReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i) & 0x0f], WORD);
  
   // RY can be in any register.
   int regSrcYReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i) >> 4], WORD);
   int regFC = HOSTREG_STATE_VAR_W(f.c, BYTE);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // CWD
   EMIT(P_WORD);
   EMIT(0x99);
   
   // IDIV regSrcYReg
   EMIT(P_WORD);
   EMIT(0xf7);
   EMIT(MODRM_REG_DIRECT(7, regSrcYReg));

   // CMP ax, 0
   EMIT(P_WORD);
   EMIT(0x83);
   EMIT(MODRM_REG_DIRECT(7, AX));
   EMIT(0);
   
   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_n(state, regFN);

   // CMP dx, 0
   EMIT(P_WORD);
   EMIT(0x83);
   EMIT(MODRM_REG_DIRECT(7, DX));
   EMIT(0);
   
   // SETNE regFC
   EMIT_REX_RBI(REG_NONE, regFC, REG_NONE, BYTE);
   EMIT(0x0f);
   EMIT(0x95);
   EMIT(MODRM_REG_DIRECT(0, regFC));
   
   // The result is in EAX, so fiddle to get RZ mapped to EAX to avoid MOV-ing 
   // if we can.
   cpu_rec_hostreg_release(state, RAX);
   int regDstReg = HOSTREG_STATE_VAR_W(r[i_z(state->i)], WORD);

   if (regDstReg != RAX) {
      EMIT_REX_RBI(RAX, regDstReg, REG_NONE, DWORD);
      EMIT(0x8b);
      EMIT(MODRM_REG_DIRECT(regDstReg, RAX));
   }

   cpu_rec_hostreg_release(state, RCX);
   cpu_rec_hostreg_release(state, RDX);
}

static void cpu_rec_op_shl_n(cpu_state *state)
{
   int regSrcReg = HOSTREG_STATE_VAR_RW(r[i_yx(state->i)], WORD);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // SHL regSrcReg, imm16
   EMIT(P_WORD);
   EMIT_REX_RBI(REG_NONE, regSrcReg, REG_NONE, WORD);
   EMIT(0xc1);
   EMIT(MODRM_REG_OPX_IMM8(4, regSrcReg));
   EMIT(i_n(state->i));

   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_n(state, regFN);
}

static void cpu_rec_op_sar_n(cpu_state *state)
{
   int regSrcReg = HOSTREG_STATE_VAR_RW(r[i_yx(state->i)], WORD);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // SAR regSrcReg, imm16
   EMIT(P_WORD);
   EMIT_REX_RBI(REG_NONE, regSrcReg, REG_NONE, WORD);
   EMIT(0xc1);
   EMIT(MODRM_REG_OPX_IMM8(7, regSrcReg));
   EMIT(i_n(state->i));

   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_n(state, regFN);
}

void* cpu_rec_dispatch(cpu_state *state, uint8_t op)
{
   switch (op) {
      case 0x00:  // NOP
         cpu_rec_op_nop(state);
         break;
      case 0x01:  // CLS
         cpu_rec_op_cls(state);
         break;
      case 0x02:  // VBLNK
         cpu_rec_op_vblnk(state);
         break;
      case 0x03:  // BGC
         cpu_rec_op_bgc(state);
         break;
      case 0x04:  // SPR
         cpu_rec_op_spr(state);
         break;
      case 0x07:  // RND
         cpu_rec_op_rnd(state);
         break;
      case 0x08:  // FLIP 
         cpu_rec_op_flip(state);
         break;
      case 0x10:  // JMP (imm)
         cpu_rec_op_jmp_imm(state);
         break;
      case 0x12:  // Jx
         cpu_rec_op_jx(state);
         break;
      case 0x16:  // JMP
         cpu_rec_op_jmp(state);
         break;
      case 0x20:  // LDI (imm)
         cpu_rec_op_ldi_imm(state);
         break;
      case 0x21:  // LDI SP (imm)
         cpu_rec_op_ldi_sp_imm(state);
         break;
      case 0x22:  // LDM (imm)
         cpu_rec_op_ldm_imm(state);
         break;
      case 0x23:  // LDM 
         cpu_rec_op_ldm(state);
         break;
      case 0x24:  // MOV 
         cpu_rec_op_mov(state);
         break;
      case 0x30:  // STM (imm)
         cpu_rec_op_stm_imm(state);
         break;
      case 0x31:  // STM
         cpu_rec_op_stm(state);
         break;
      case 0x40:  // ADDI
         cpu_rec_op_addi(state);
         break;
      case 0x41:  // ADD
         cpu_rec_op_add(state);
         break;
      case 0x42:  // ADD r3
         cpu_rec_op_add_r3(state);
         break;
      case 0x50:  // SUBI
         cpu_rec_op_subi(state);
         break;
      case 0x51:  // SUB
         cpu_rec_op_sub(state);
         break;
      case 0x52:  // SUB r3
         cpu_rec_op_sub_r3(state);
         break;
      case 0x53:  // CMPI
         cpu_rec_op_cmpi(state);
         break;
      case 0x54:  // CMP
         cpu_rec_op_cmp(state);
         break;
      case 0x60:  // ANDI
         cpu_rec_op_andi(state);
         break;
      case 0x90:  // MULI
         cpu_rec_op_muli(state);
         break;
      case 0xa2:  // DIV r3
         cpu_rec_op_div_r3(state);
         break;
      case 0xb0:  // SHL n
         cpu_rec_op_shl_n(state);
         break;
      case 0xb2:  // SAR n
         cpu_rec_op_sar_n(state);
         break;
      default:
      {
         void *op_instr = (void *)op_table[op];

         cpu_rec_hostreg_release_all(state);
         cpu_rec_hostreg_preserve(state);
         
         ptrdiff_t disp = (uint8_t *)state - state->rec.jit_p;
         EMIT_REX_RBI(RDX, REG_NONE, REG_NONE, QWORD);
         if (disp <= INT32_MAX && disp >= INT32_MIN) {
            EMIT(0x8d);    // LEA rdi, [m]
            EMIT(MODRM_RIP_DISP32(RDI));
            EMIT4i(OFFSET(state, 4));
         } else {
            EMIT(0xb8 + RDI);    // MOV rdi, m
            EMIT8u(state);
         }

         // MOV rax, [op_instr]
         EMIT(REX_W);
         EMIT(0xb8 + RAX);
         EMIT8u(op_instr);
         // CALL rax
         EMIT(0xff);
         EMIT(MODRM_REG_DIRECT(2, RAX));

         // XCHG eax, eax == NOP
         EMIT(0x90);
         
         break;
      }
   }
}

