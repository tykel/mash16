#include "cpu.h"
#include "cpu_rec_ops.h"

static void cpu_rec_op_nop(cpu_state *state)
{
}

static void cpu_rec_op_cls(cpu_state *state)
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

static void cpu_rec_op_jmc(cpu_state *state)
{
   int regPC = HOSTREG_STATE_VAR_RW(pc, WORD);
   int regFC = HOSTREG_STATE_VAR_R(f.c, BYTE);
   int rexNeeded;
   int rexRegPC = rex(REG_NONE, regPC, REG_NONE, DWORD, &rexNeeded);

   // CMP regFC, 1
   EMIT_REX_RBI(REG_NONE, regFC, REG_NONE, BYTE);
   EMIT(0x80);
   EMIT(MODRM_REG_OPX_IMM8(7, regFC));
   EMIT(1);
   // JNZ end
   EMIT(0x75);
   EMIT(rexNeeded+5);
   // MOV regPC, HHLL
   EMIT_REX_RBI(REG_NONE, regPC, REG_NONE, DWORD);
   EMIT(0xb8 + (regPC & 7));
   EMIT4u(i_hhll(state->i));
}

static void cpu_rec_op_jme(cpu_state *state)
{
   int regXReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i) & 0x0f], WORD);
   int regYReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i) >> 4], WORD);
   int regPC = HOSTREG_STATE_VAR_RW(pc, WORD);
   int rexNeeded;
   int rexRegPC = rex(REG_NONE, regPC, REG_NONE, DWORD, &rexNeeded);

   // CMP regXReg, regYReg
   EMIT(P_WORD);
   EMIT_REX_RBI(regXReg, regYReg, REG_NONE, WORD);
   EMIT(0x3b);
   EMIT(MODRM_REG_DIRECT(regXReg, regYReg));
   // JNZ end
   EMIT(0x75);
   EMIT(rexNeeded+5);
   // MOV regPC, HHLL
   EMIT_REX_RBI(REG_NONE, regPC, REG_NONE, DWORD);
   EMIT(0xb8 + (regPC & 7));
   EMIT4u(i_hhll(state->i));
}

static void cpu_rec_op_ret(cpu_state *state)
{
   int regSP = HOSTREG_STATE_VAR_RW(sp, WORD);
   int regPC = HOSTREG_STATE_VAR_RW(pc, WORD);
   int regPtrM = HOSTREG_PTR(state->m);

   // SUB regSP, 2
   EMIT_REX_RBI(REG_NONE, regSP, REG_NONE, WORD);
   EMIT(0x83);
   EMIT(MODRM_REG_DIRECT(5, regSP));
   EMIT(2);
   // MOV regPC, [regPtrM + regSP]
   EMIT(P_WORD);
   EMIT_REX_RBI(regPC, regPtrM, regSP, WORD);
   EMIT(0x8b);
   EMIT(MODRM_REG_SIB(regPC));
   EMIT(SIB(0, regSP, regPtrM));
}

static void cpu_rec_op_call_imm(cpu_state *state)
{
   // Store PC to [SP], increase SP by 2, set PC to HHLL.
   int regPC = HOSTREG_STATE_VAR_RW(pc, WORD);
   int regSP = HOSTREG_STATE_VAR_RW(sp, WORD);
   int regPtrM = HOSTREG_PTR(state->m);

   // MOV [regPtrM + regSP], regPC
   EMIT(P_WORD);
   EMIT_REX_RBI(regPC, regPtrM, regSP, DWORD);
   EMIT(0x89);
   EMIT(MODRM_REG_SIB(regPC));
   EMIT(SIB(0, regSP, regPtrM));

   // ADD regSP, 2
   EMIT_REX_RBI(REG_NONE, regSP, REG_NONE, DWORD);
   EMIT(0x83);
   EMIT(MODRM_REG_DIRECT(0, regSP));
   EMIT(2);

   // MOV regPC, HHLL
   EMIT_REX_RBI(REG_NONE, regPC, REG_NONE, DWORD);
   EMIT(0xb8 + (regPC & 7));
   EMIT4u(i_hhll(state->i));
}

static void cpu_rec_op_call(cpu_state *state)
{
   // Store PC to [SP], increase SP by 2, set PC to RX.
   int regPC = HOSTREG_STATE_VAR_RW(pc, WORD);
   int regSP = HOSTREG_STATE_VAR_RW(sp, WORD);
   int regPtrM = HOSTREG_PTR(state->m);
   int regXReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i) & 0x0f], WORD);

   // MOV [regPtrM + regSP], regPC
   EMIT(P_WORD);
   EMIT_REX_RBI(regPC, regPtrM, regSP, DWORD);
   EMIT(0x89);
   EMIT(MODRM_REG_SIB(regPC));
   EMIT(SIB(0, regSP, regPtrM));

   // ADD regSP, 2
   EMIT_REX_RBI(REG_NONE, regSP, REG_NONE, DWORD);
   EMIT(0x83);
   EMIT(MODRM_REG_DIRECT(0, regSP));
   EMIT(2);

   // MOV regPC, regXReg
   EMIT_REX_RBI(regPC, regXReg, REG_NONE, DWORD);
   EMIT(0x8b);
   EMIT(MODRM_REG_DIRECT(regPC, regXReg));
}

static void cpu_rec_op_jmp(cpu_state *state)
{
   int regSrc = HOSTREG_STATE_VAR_R(r[i_yx(state->i)], WORD);
   int regPc = HOSTREG_STATE_VAR_W(pc, WORD);

   EMIT_REX_RBI(regPc, regSrc, REG_NONE, DWORD);
   EMIT(0x8b);
   EMIT(MODRM_REG_DIRECT(regPc, regSrc));
}

static void cpu_rec_op_jx_cx(cpu_state *state, int cx)
{
   int regPC = HOSTREG_STATE_VAR_RW(pc, WORD);
   int regSP = HOSTREG_STATE_VAR_RW(sp, WORD);
   int regPtrM = HOSTREG_PTR(state->m);
   int rexCallMov0 = 0, rexCallAdd = 0, rexCallMov1 = 0;
   int rexJmpMov = 0;
   int temp = rex(regPC, regPtrM, regSP, WORD, &rexCallMov0);
   temp = rex(REG_NONE, regSP, REG_NONE, DWORD, &rexCallAdd);
   temp = rex(REG_NONE, regPC, REG_NONE, DWORD, &rexCallMov1);
   temp = rex(REG_NONE, regPC, REG_NONE, DWORD, &rexJmpMov);
   int xbytes = cx
      ? (rexCallMov0+4 + rexCallAdd+3 + rexCallMov1+5)
      : (rexJmpMov+5);

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
         EMIT(xbytes);
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
         EMIT(xbytes);
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
         EMIT(rexNeededN+3 + 2 + xbytes);
         // CMP regFN, 0
         EMIT_REX_RBI(REG_NONE, regFN, REG_NONE, BYTE);
         EMIT(0x80);
         EMIT(MODRM_REG_OPX_IMM8(7, regFN));
         EMIT(0);
         // JNZ end
         EMIT(0x75);
         EMIT(xbytes);
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
         EMIT(xbytes);
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
         EMIT(rexNeededZ+3 + 2 + xbytes);
         // CMP regFZ, 0
         EMIT_REX_RBI(REG_NONE, regFZ, REG_NONE, BYTE);
         EMIT(0x80);
         EMIT(MODRM_REG_OPX_IMM8(7, regFZ));
         EMIT(0);
         // JNZ end
         EMIT(0x75);
         EMIT(xbytes);
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
         EMIT(xbytes);
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
         EMIT(xbytes);
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
         EMIT(rexNeededNO+2 + 2 + xbytes);
         // CMP regFO, regFN
         EMIT_REX_RBI(regFN, regFO, REG_NONE, BYTE);
         EMIT(0x38);
         EMIT(MODRM_REG_DIRECT(regFN, regFO));
         // JNZ end
         EMIT(0x75);
         EMIT(xbytes);
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
         EMIT(xbytes);
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
         EMIT(xbytes);
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
         // JZ change_pc 
         EMIT(0x74);
         EMIT(rexNeededNO+2 + 2);
         // CMP regFO, regFN
         EMIT_REX_RBI(regFN, regFO, REG_NONE, BYTE);
         EMIT(0x38);
         EMIT(MODRM_REG_DIRECT(regFN, regFO));
         // JZ end
         EMIT(0x74);
         EMIT(xbytes);
      }
      break;
   }

   // change_pc;
   if (cx) {
      cpu_rec_op_call_imm(state);
   } else {
      cpu_rec_op_jmp_imm(state);
   }

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

   // AND regSrcReg, 0xffff
   EMIT_REX_RBI(REG_NONE, regSrcReg, REG_NONE, DWORD);
   EMIT(0x81);
   EMIT(MODRM_REG_OPX_IMM8(4, regSrcReg));
   EMIT4u(0xffff);
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
   // RCL CL, 1
   EMIT(0xd0);
   EMIT(MODRM_REG_DIRECT(2, CL));
   // SHR regAddress, 1
   EMIT_REX_RBI(REG_NONE, regAddress, REG_NONE, DWORD);
   EMIT(0xd1);
   EMIT(MODRM_REG_DIRECT(5, regAddress));
   // RCL CL, 1
   EMIT(0xd0);
   EMIT(MODRM_REG_DIRECT(2, CL));
   // SHR regAddress, 1
   EMIT_REX_RBI(REG_NONE, regAddress, REG_NONE, DWORD);
   EMIT(0xd1);
   EMIT(MODRM_REG_DIRECT(5, regAddress));
   // RCL CL, 1
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
   int regTemp = HOSTREG_TEMP_VAR();
   int regFC = HOSTREG_STATE_VAR_W(f.c, BYTE);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFO = HOSTREG_STATE_VAR_W(f.o, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // MOV regTemp, regXReg
   EMIT_REX_RBI(regTemp, regXReg, REG_NONE, DWORD);
   EMIT(0x8b);
   EMIT(MODRM_REG_DIRECT(regTemp, regXReg));
   // SUB regTemp, regYReg
   EMIT(P_WORD);
   EMIT_REX_RBI(regTemp, regYReg, REG_NONE, WORD);
   EMIT(0x2b);
   EMIT(MODRM_REG_DIRECT(regTemp, regYReg));

   cpu_rec_flag_c(state, regFC);
   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_o(state, regFO);
   cpu_rec_flag_n(state, regFN);

   // MOV regZReg, regTemp
   EMIT_REX_RBI(regZReg, regTemp, REG_NONE, DWORD);
   EMIT(0x8b);
   EMIT(MODRM_REG_DIRECT(regZReg, regTemp));
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

static void cpu_rec_op_and(cpu_state *state)
{
   int regXReg = HOSTREG_STATE_VAR_RW(r[i_yx(state->i)  & 0x0f], WORD);
   int regYReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i) >> 4], WORD);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // AND regXReg, regYReg
   EMIT(P_WORD);
   EMIT_REX_RBI(regXReg, regYReg, REG_NONE, WORD);
   EMIT(0x23);
   EMIT(MODRM_REG_DIRECT(regXReg, regYReg));

   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_n(state, regFN);
}

static void cpu_rec_op_and_r3(cpu_state *state)
{
   int regXReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i)  & 0x0f], WORD);
   int regYReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i) >> 4], WORD);
   int regZReg = HOSTREG_STATE_VAR_W(r[i_z(state->i)], WORD);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // MOV regZReg, regXReg
   EMIT_REX_RBI(regZReg, regXReg, REG_NONE, DWORD);
   EMIT(0x8b);
   EMIT(MODRM_REG_DIRECT(regZReg, regXReg));
   // AND regZReg, regYReg
   EMIT(P_WORD);
   EMIT_REX_RBI(regZReg, regYReg, REG_NONE, WORD);
   EMIT(0x23);
   EMIT(MODRM_REG_DIRECT(regZReg, regYReg));

   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_n(state, regFN);
}

static void cpu_rec_op_tsti(cpu_state *state)
{
   int regSrcReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i)], WORD);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // TEST regSrcReg, imm16
   EMIT(P_WORD);
   EMIT_REX_RBI(REG_NONE, regSrcReg, REG_NONE, WORD);
   EMIT(0xf7);
   EMIT(MODRM_REG_OPX_IMM8(0, regSrcReg));
   EMIT2u(i_hhll(state->i));

   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_n(state, regFN);
}

static void cpu_rec_op_tst(cpu_state *state)
{
   int regXReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i)  & 0x0f], WORD);
   int regYReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i) >> 4], WORD);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // TEST regXReg, regYReg
   EMIT(P_WORD);
   EMIT_REX_RBI(regXReg, regYReg, REG_NONE, WORD);
   EMIT(0x85);
   EMIT(MODRM_REG_DIRECT(regXReg, regYReg));

   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_n(state, regFN);
}

static void cpu_rec_op_ori(cpu_state *state)
{
   int regSrcReg = HOSTREG_STATE_VAR_RW(r[i_yx(state->i)], WORD);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // OR regSrcReg, imm16
   EMIT(P_WORD);
   EMIT_REX_RBI(REG_NONE, regSrcReg, REG_NONE, WORD);
   EMIT(0x81);
   EMIT(MODRM_REG_OPX_IMM8(1, regSrcReg));
   EMIT2u(i_hhll(state->i));

   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_n(state, regFN);
}

static void cpu_rec_op_or(cpu_state *state)
{
   int regXReg = HOSTREG_STATE_VAR_RW(r[i_yx(state->i)  & 0x0f], WORD);
   int regYReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i) >> 4], WORD);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // OR regXReg, regYReg
   EMIT(P_WORD);
   EMIT_REX_RBI(regXReg, regYReg, REG_NONE, WORD);
   EMIT(0x0b);
   EMIT(MODRM_REG_DIRECT(regXReg, regYReg));

   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_n(state, regFN);
}

static void cpu_rec_op_or_r3(cpu_state *state)
{
   int regXReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i)  & 0x0f], WORD);
   int regYReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i) >> 4], WORD);
   int regZReg = HOSTREG_STATE_VAR_W(r[i_z(state->i)], WORD);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // MOV regZReg, regXReg
   EMIT_REX_RBI(regZReg, regXReg, REG_NONE, DWORD);
   EMIT(0x8b);
   EMIT(MODRM_REG_DIRECT(regZReg, regXReg));
   // OR regZReg, regYReg
   EMIT(P_WORD);
   EMIT_REX_RBI(regZReg, regYReg, REG_NONE, WORD);
   EMIT(0x0b);
   EMIT(MODRM_REG_DIRECT(regZReg, regYReg));

   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_n(state, regFN);
}

static void cpu_rec_op_xori(cpu_state *state)
{
   int regSrcReg = HOSTREG_STATE_VAR_RW(r[i_yx(state->i)], WORD);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // XOR regSrcReg, imm16
   EMIT(P_WORD);
   EMIT_REX_RBI(REG_NONE, regSrcReg, REG_NONE, WORD);
   EMIT(0x81);
   EMIT(MODRM_REG_OPX_IMM8(6, regSrcReg));
   EMIT2u(i_hhll(state->i));

   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_n(state, regFN);
}

static void cpu_rec_op_xor(cpu_state *state)
{
   int regXReg = HOSTREG_STATE_VAR_RW(r[i_yx(state->i)  & 0x0f], WORD);
   int regYReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i) >> 4], WORD);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // XOR regXReg, regYReg
   EMIT(P_WORD);
   EMIT_REX_RBI(regXReg, regYReg, REG_NONE, WORD);
   EMIT(0x33);
   EMIT(MODRM_REG_DIRECT(regXReg, regYReg));

   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_n(state, regFN);
}

static void cpu_rec_op_xor_r3(cpu_state *state)
{
   int regXReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i)  & 0x0f], WORD);
   int regYReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i) >> 4], WORD);
   int regZReg = HOSTREG_STATE_VAR_W(r[i_z(state->i)], WORD);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // MOV regZReg, regXReg
   EMIT_REX_RBI(regZReg, regXReg, REG_NONE, DWORD);
   EMIT(0x8b);
   EMIT(MODRM_REG_DIRECT(regZReg, regXReg));
   // XOR regZReg, regYReg
   EMIT(P_WORD);
   EMIT_REX_RBI(regZReg, regYReg, REG_NONE, WORD);
   EMIT(0x33);
   EMIT(MODRM_REG_DIRECT(regZReg, regYReg));

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


static void cpu_rec_op_mul_r2(cpu_state *state)
{
   int regXReg = HOSTREG_STATE_VAR_RW(r[i_yx(state->i) & 15], WORD);
   int regYReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i) >> 4], WORD);
   int regFC = HOSTREG_STATE_VAR_W(f.c, BYTE);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // IMUL regXReg, regYReg
   EMIT(P_WORD);
   EMIT_REX_RBI(regXReg, regYReg, REG_NONE, WORD);
   EMIT(0x0f);
   EMIT(0xaf);
   EMIT(MODRM_REG_DIRECT(regXReg, regYReg));

   cpu_rec_flag_c(state, regFC);

   // CMP regXReg, 0
   EMIT(P_WORD);
   EMIT_REX_RBI(REG_NONE, regXReg, REG_NONE, DWORD);
   EMIT(0x83);
   EMIT(MODRM_REG_DIRECT(7, regXReg));
   EMIT(0);

   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_n(state, regFN);
}

static void cpu_rec_op_div_r3(cpu_state *state)
{
   // If this reg was cached, flush it
   int regSrcXReg = cpu_rec_hostreg_var(state, &state->r[i_yx(state->i) & 0x0f], WORD, 0);
   cpu_rec_hostreg_release(state, regSrcXReg);

   cpu_rec_hostreg_release(state, RAX);
   cpu_rec_hostreg_freeze(state, RAX);
   
   cpu_rec_hostreg_release(state, RCX);
   cpu_rec_hostreg_freeze(state, RCX);
   
   cpu_rec_hostreg_release(state, RDX);
   cpu_rec_hostreg_freeze(state, RDX);
   
   regSrcXReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i) & 0x0f], WORD);
  
   // RY can be in any register.
   int regSrcYReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i) >> 4], WORD);
   int regFC = HOSTREG_STATE_VAR_W(f.c, BYTE);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // MOV EAX, regSrcXReg
   EMIT_REX_RBI(EAX, regSrcXReg, REG_NONE, DWORD);
   EMIT(0x8b);
   EMIT(MODRM_REG_DIRECT(EAX, regSrcXReg));

   // CWD
   EMIT(P_WORD);
   EMIT(0x99);
   
   // IDIV regSrcYReg
   EMIT(P_WORD);
   EMIT_REX_RBI(REG_NONE, regSrcYReg, REG_NONE, WORD);
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
      EMIT_REX_RBI(regDstReg, RAX, REG_NONE, DWORD);
      EMIT(0x8b);
      EMIT(MODRM_REG_DIRECT(regDstReg, RAX));
   }

   cpu_rec_hostreg_release(state, RAX);
   cpu_rec_hostreg_release(state, RCX);
   cpu_rec_hostreg_release(state, RDX);
}

static void cpu_rec_op_shl_n(cpu_state *state)
{
   int regXReg = HOSTREG_STATE_VAR_RW(r[i_yx(state->i)], WORD);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // SHL regXReg, imm16
   EMIT(P_WORD);
   EMIT_REX_RBI(REG_NONE, regXReg, REG_NONE, WORD);
   EMIT(0xc1);
   EMIT(MODRM_REG_OPX_IMM8(4, regXReg));
   EMIT(i_n(state->i));
   // CMP regXReg, 0
   EMIT(P_WORD);
   EMIT_REX_RBI(REG_NONE, regXReg, REG_NONE, WORD);
   EMIT(0x83);
   EMIT(MODRM_REG_OPX_IMM8(7, regXReg));
   EMIT(0);

   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_n(state, regFN);
}

static void cpu_rec_op_shr_n(cpu_state *state)
{
   int regXReg = HOSTREG_STATE_VAR_RW(r[i_yx(state->i)], WORD);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // SHR regXReg, imm16
   EMIT(P_WORD);
   EMIT_REX_RBI(REG_NONE, regXReg, REG_NONE, WORD);
   EMIT(0xc1);
   EMIT(MODRM_REG_OPX_IMM8(5, regXReg));
   EMIT(i_n(state->i));
   // CMP regXReg, 0
   EMIT(P_WORD);
   EMIT_REX_RBI(REG_NONE, regXReg, REG_NONE, DWORD);
   EMIT(0x83);
   EMIT(MODRM_REG_OPX_IMM8(7, regXReg));
   EMIT(0);

   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_n(state, regFN);
}

static void cpu_rec_op_sar_n(cpu_state *state)
{
   int regXReg = HOSTREG_STATE_VAR_RW(r[i_yx(state->i)], WORD);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // SAR regXReg, imm16
   EMIT(P_WORD);
   EMIT_REX_RBI(REG_NONE, regXReg, REG_NONE, WORD);
   EMIT(0xc1);
   EMIT(MODRM_REG_OPX_IMM8(7, regXReg));
   EMIT(i_n(state->i));
   // CMP regXReg, 0
   EMIT(P_WORD);
   EMIT_REX_RBI(REG_NONE, regXReg, REG_NONE, DWORD);
   EMIT(0x83);
   EMIT(MODRM_REG_OPX_IMM8(7, regXReg));
   EMIT(0);

   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_n(state, regFN);
}

static void cpu_rec_op_shl(cpu_state *state)
{
   cpu_rec_hostreg_release(state, RCX);
   cpu_rec_hostreg_freeze(state, RCX);

   int regXReg = HOSTREG_STATE_VAR_RW(r[i_yx(state->i) & 0x0f], WORD);
   int regYReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i) >> 4], WORD);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // MOV ECX, regYReg
   EMIT_REX_RBI(REG_NONE, regYReg, REG_NONE, DWORD);
   EMIT(0x8b);
   EMIT(MODRM_REG_DIRECT(RCX, regYReg));
   // SHL regXReg, CL
   EMIT(P_WORD);
   EMIT_REX_RBI(REG_NONE, regXReg, REG_NONE, WORD);
   EMIT(0xd3);
   EMIT(MODRM_REG_DIRECT(4, regXReg));
   // CMP regXReg, 0
   EMIT(P_WORD);
   EMIT_REX_RBI(REG_NONE, regXReg, REG_NONE, DWORD);
   EMIT(0x83);
   EMIT(MODRM_REG_OPX_IMM8(7, regXReg));
   EMIT(0);

   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_n(state, regFN);

   cpu_rec_hostreg_release(state, RCX);
}

static void cpu_rec_op_shr(cpu_state *state)
{
   cpu_rec_hostreg_release(state, RCX);
   cpu_rec_hostreg_freeze(state, RCX);

   int regXReg = HOSTREG_STATE_VAR_RW(r[i_yx(state->i) & 0x0f], WORD);
   int regYReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i) >> 4], WORD);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // MOV ECX, regYReg
   EMIT_REX_RBI(REG_NONE, regYReg, REG_NONE, DWORD);
   EMIT(0x8b);
   EMIT(MODRM_REG_DIRECT(RCX, regYReg));
   // SHR regXReg, CL
   EMIT(P_WORD);
   EMIT_REX_RBI(REG_NONE, regXReg, REG_NONE, WORD);
   EMIT(0xd3);
   EMIT(MODRM_REG_DIRECT(5, regXReg));
   // CMP regXReg, 0
   EMIT(P_WORD);
   EMIT_REX_RBI(REG_NONE, regXReg, REG_NONE, DWORD);
   EMIT(0x83);
   EMIT(MODRM_REG_OPX_IMM8(7, regXReg));
   EMIT(0);

   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_n(state, regFN);

   cpu_rec_hostreg_release(state, RCX);
}

static void cpu_rec_op_sar(cpu_state *state)
{
   cpu_rec_hostreg_release(state, RCX);
   cpu_rec_hostreg_freeze(state, RCX);

   int regXReg = HOSTREG_STATE_VAR_RW(r[i_yx(state->i) & 0x0f], WORD);
   int regYReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i) >> 4], WORD);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // MOV ECX, regYReg
   EMIT_REX_RBI(REG_NONE, regYReg, REG_NONE, DWORD);
   EMIT(0x8b);
   EMIT(MODRM_REG_DIRECT(RCX, regYReg));
   // SAR regXReg, CL
   EMIT(P_WORD);
   EMIT_REX_RBI(REG_NONE, regXReg, REG_NONE, WORD);
   EMIT(0xd3);
   EMIT(MODRM_REG_DIRECT(7, regXReg));
   // CMP regXReg, 0
   EMIT(P_WORD);
   EMIT_REX_RBI(REG_NONE, regXReg, REG_NONE, DWORD);
   EMIT(0x83);
   EMIT(MODRM_REG_OPX_IMM8(7, regXReg));
   EMIT(0);

   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_n(state, regFN);

   cpu_rec_hostreg_release(state, RCX);
}

static void cpu_rec_op_push(cpu_state *state)
{
   int regXReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i)], WORD);
   int regSP = HOSTREG_STATE_VAR_RW(sp, WORD);
   int regPtrM = HOSTREG_PTR(state->m);

   // MOV [regPtrM + regSP], regXReg
   EMIT(P_WORD);
   EMIT_REX_RBI(regXReg, regPtrM, regSP, WORD);
   EMIT(0x89);
   EMIT(MODRM_REG_SIB(regXReg));
   EMIT(SIB(0, regSP, regPtrM));
   // ADD regSP, 2
   EMIT_REX_RBI(REG_NONE, regSP, REG_NONE, DWORD);
   EMIT(0x83);
   EMIT(MODRM_REG_DIRECT(0, regSP));
   EMIT(2);
}

static void cpu_rec_op_pop(cpu_state *state)
{
   int regSP = HOSTREG_STATE_VAR_RW(sp, WORD);
   int regPtrM = HOSTREG_PTR(state->m);
   int regXReg = HOSTREG_STATE_VAR_W(r[i_yx(state->i)], WORD);
   
   // SUB regSP, 2
   EMIT_REX_RBI(REG_NONE, regSP, REG_NONE, DWORD);
   EMIT(0x83);
   EMIT(MODRM_REG_DIRECT(5, regSP));
   EMIT(2);

   // MOV regXReg, [regPtrM + regSP]
   EMIT(P_WORD);
   EMIT_REX_RBI(regXReg, regPtrM, regSP, WORD);
   EMIT(0x8b);
   EMIT(MODRM_REG_SIB(regXReg));
   EMIT(SIB(0, regSP, regPtrM));
}

static void cpu_rec_op_pushall(cpu_state *state)
{
   int regSP = HOSTREG_STATE_VAR_RW(sp, WORD);
   int regPtrM = HOSTREG_PTR(state->m);
   cpu_rec_hostreg_freeze(state, regSP);
   cpu_rec_hostreg_freeze(state, regPtrM);
   
   for (int i = 0; i < 16; ++i) {
      int regXReg = HOSTREG_STATE_VAR_R(r[i], WORD);

      // MOV [regPtrM + regSP + 2i], regXReg
      EMIT(P_WORD);
      EMIT_REX_RBI(regXReg, regPtrM, regSP, WORD);
      EMIT(0x89);
      EMIT(MODRM_REG_SIB_DISP8(regXReg));
      EMIT(SIB(0, regSP, regPtrM));
      EMIT(2 * i);
   }
   cpu_rec_hostreg_unfreeze(state, regPtrM);
   
   // ADD regSP, 32
   EMIT_REX_RBI(REG_NONE, regSP, REG_NONE, DWORD);
   EMIT(0x83);
   EMIT(MODRM_REG_DIRECT(0, regSP));
   EMIT(32);
   
   cpu_rec_hostreg_unfreeze(state, regSP);
}

static void cpu_rec_op_popall(cpu_state *state)
{
   int regSP = HOSTREG_STATE_VAR_RW(sp, WORD);
   int regPtrM = HOSTREG_PTR(state->m);
   cpu_rec_hostreg_freeze(state, regSP);
   cpu_rec_hostreg_freeze(state, regPtrM);

   for (int i = 0; i < 16; ++i) {
      int regXReg = HOSTREG_STATE_VAR_W(r[15 - i], WORD);

      // MOV regXReg, [regPtrM + regSP - 2i - 2]
      EMIT(P_WORD);
      EMIT_REX_RBI(regXReg, regPtrM, regSP, WORD);
      EMIT(0x8b);
      EMIT(MODRM_REG_SIB_DISP8(regXReg));
      EMIT(SIB(0, regSP, regPtrM));
      EMIT(-2 * i - 2);
   }
   cpu_rec_hostreg_unfreeze(state, regPtrM);
   
   // SUB regSP, 32
   EMIT_REX_RBI(REG_NONE, regSP, REG_NONE, DWORD);
   EMIT(0x83);
   EMIT(MODRM_REG_DIRECT(5, regSP));
   EMIT(32);
   
   cpu_rec_hostreg_unfreeze(state, regSP);
}

static void cpu_rec_op_pushf(cpu_state *state)
{
   int regFN = HOSTREG_STATE_VAR_R(f.n, BYTE);
   int regFO = HOSTREG_STATE_VAR_R(f.o, BYTE);
   int regFZ = HOSTREG_STATE_VAR_R(f.z, BYTE);
   int regFC = HOSTREG_STATE_VAR_R(f.c, BYTE);
   int regTemp = HOSTREG_TEMP_VAR();
   // MOV regTemp, regFN
   EMIT_REX_RBI(regTemp, regFN, REG_NONE, DWORD);
   EMIT(0x8b);
   EMIT(MODRM_REG_DIRECT(regTemp, regFN));
   // SHL regTemp, 1
   EMIT_REX_RBI(REG_NONE, regTemp, REG_NONE, DWORD);
   EMIT(0xd1);
   EMIT(MODRM_REG_DIRECT(4, regTemp));
   // ADD regTemp, regFO
   EMIT_REX_RBI(regTemp, regFO, REG_NONE, DWORD);
   EMIT(0x03);
   EMIT(MODRM_REG_DIRECT(regTemp, regFO));
   // SHL regTemp, 4
   EMIT_REX_RBI(REG_NONE, regTemp, REG_NONE, DWORD);
   EMIT(0xc1);
   EMIT(MODRM_REG_DIRECT(4, regTemp));
   EMIT(4);
   // ADD regTemp, regFZ
   EMIT_REX_RBI(regTemp, regFZ, REG_NONE, DWORD);
   EMIT(0x03);
   EMIT(MODRM_REG_DIRECT(regTemp, regFZ));
   // SHL regTemp, 1
   EMIT_REX_RBI(REG_NONE, regTemp, REG_NONE, DWORD);
   EMIT(0xd1);
   EMIT(MODRM_REG_DIRECT(4, regTemp));
   // ADD regTemp, regFC
   EMIT_REX_RBI(regTemp, regFC, REG_NONE, DWORD);
   EMIT(0x03);
   EMIT(MODRM_REG_DIRECT(regTemp, regFC));
   // SHL regTemp, 1
   EMIT_REX_RBI(REG_NONE, regTemp, REG_NONE, DWORD);
   EMIT(0xd1);
   EMIT(MODRM_REG_DIRECT(4, regTemp));

   int regSP = HOSTREG_STATE_VAR_RW(sp, WORD);
   int regPtrM = HOSTREG_PTR(state->m);

   // MOV [regPtrM + regSP], regTemp
   EMIT(P_WORD);
   EMIT_REX_RBI(regTemp, regPtrM, regSP, WORD);
   EMIT(0x89);
   EMIT(MODRM_REG_SIB(regTemp));
   EMIT(SIB(0, regSP, regPtrM));
   // ADD regSP, 2
   EMIT_REX_RBI(REG_NONE, regSP, REG_NONE, DWORD);
   EMIT(0x83);
   EMIT(MODRM_REG_DIRECT(0, regSP));
   EMIT(2);

   cpu_rec_hostreg_release(state, regTemp);
}

static void cpu_rec_op_popf(cpu_state *state)
{
   int regSP = HOSTREG_STATE_VAR_RW(sp, WORD);
   int regPtrM = HOSTREG_PTR(state->m);
   int regTemp = HOSTREG_TEMP_VAR();
   
   // SUB regSP, 2
   EMIT_REX_RBI(REG_NONE, regSP, REG_NONE, DWORD);
   EMIT(0x83);
   EMIT(MODRM_REG_DIRECT(5, regSP));
   EMIT(2);

   // MOV regTemp, [regPtrM + regSP]
   EMIT(P_WORD);
   EMIT_REX_RBI(regTemp, regPtrM, regSP, WORD);
   EMIT(0x8b);
   EMIT(MODRM_REG_SIB(regTemp));
   EMIT(SIB(0, regSP, regPtrM));
   
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);
   int regFO = HOSTREG_STATE_VAR_W(f.o, BYTE);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFC = HOSTREG_STATE_VAR_W(f.c, BYTE);
   // SHR regTemp, 2
   EMIT_REX_RBI(REG_NONE, regTemp, REG_NONE, DWORD);
   EMIT(0xc1);
   EMIT(MODRM_REG_DIRECT(5, regTemp));
   EMIT(2);
   // SETC regFC
   EMIT_REX_RBI(REG_NONE, regFC, REG_NONE, DWORD);
   EMIT(0x0f);
   EMIT(0x92);
   EMIT(MODRM_REG_DIRECT(0, regFC));
   // SHR regTemp, 1
   EMIT_REX_RBI(REG_NONE, regTemp, REG_NONE, DWORD);
   EMIT(0xd1);
   EMIT(MODRM_REG_DIRECT(5, regTemp));
   // SETC regFZ
   EMIT_REX_RBI(REG_NONE, regFZ, REG_NONE, DWORD);
   EMIT(0x0f);
   EMIT(0x92);
   EMIT(MODRM_REG_DIRECT(0, regFZ));
   // SHR regTemp, 4
   EMIT_REX_RBI(REG_NONE, regTemp, REG_NONE, DWORD);
   EMIT(0xc1);
   EMIT(MODRM_REG_DIRECT(5, regTemp));
   EMIT(4);
   // SETC regFO
   EMIT_REX_RBI(REG_NONE, regFO, REG_NONE, DWORD);
   EMIT(0x0f);
   EMIT(0x92);
   EMIT(MODRM_REG_DIRECT(0, regFO));
   // SHR regTemp, 1
   EMIT_REX_RBI(REG_NONE, regTemp, REG_NONE, DWORD);
   EMIT(0xd1);
   EMIT(MODRM_REG_DIRECT(5, regTemp));
   // SETC regFN
   EMIT_REX_RBI(REG_NONE, regFN, REG_NONE, DWORD);
   EMIT(0x0f);
   EMIT(0x92);
   EMIT(MODRM_REG_DIRECT(0, regFN));

   cpu_rec_hostreg_release(state, regTemp);
}

static void cpu_rec_op_noti(cpu_state *state)
{
   int regXReg = HOSTREG_STATE_VAR_W(r[i_yx(state->i)], WORD);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // MOV regXReg, ~HHLL
   EMIT_REX_RBI(REG_NONE, regXReg, REG_NONE, DWORD);
   EMIT(0xb8 + (regXReg & 7));
   EMIT4u(~i_hhll(state->i));
   // CMP regXReg, 0
   EMIT_REX_RBI(REG_NONE, regXReg, REG_NONE, DWORD);
   EMIT(0x83);
   EMIT(MODRM_REG_OPX_IMM8(7, regXReg));
   EMIT(0);

   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_n(state, regFN);
}

static void cpu_rec_op_not(cpu_state *state)
{
   int regXReg = HOSTREG_STATE_VAR_RW(r[i_yx(state->i)], WORD);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // NOT regXReg
   EMIT_REX_RBI(REG_NONE, regXReg, REG_NONE, DWORD);
   EMIT(0xf7);
   EMIT(MODRM_REG_DIRECT(2, regXReg));
   // CMP regXReg, 0
   EMIT_REX_RBI(REG_NONE, regXReg, REG_NONE, DWORD);
   EMIT(0x83);
   EMIT(MODRM_REG_OPX_IMM8(7, regXReg));
   EMIT(0);

   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_n(state, regFN);
}

static void cpu_rec_op_not_r2(cpu_state *state)
{
   int regXReg = HOSTREG_STATE_VAR_W(r[i_yx(state->i)], WORD);
   int regYReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i)], WORD);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // MOV regXReg, regYReg
   EMIT_REX_RBI(regXReg, regYReg, REG_NONE, DWORD);
   EMIT(0x8b);
   EMIT(MODRM_REG_DIRECT(regXReg, regYReg));
   // NOT regXReg
   EMIT_REX_RBI(REG_NONE, regXReg, REG_NONE, DWORD);
   EMIT(0xf7);
   EMIT(MODRM_REG_DIRECT(2, regXReg));
   // CMP regXReg, 0
   EMIT_REX_RBI(REG_NONE, regXReg, REG_NONE, DWORD);
   EMIT(0x83);
   EMIT(MODRM_REG_OPX_IMM8(7, regXReg));
   EMIT(0);

   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_n(state, regFN);
}

static void cpu_rec_op_negi(cpu_state *state)
{
   int regXReg = HOSTREG_STATE_VAR_W(r[i_yx(state->i)], WORD);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // MOV regXReg, -HHLL
   EMIT_REX_RBI(REG_NONE, regXReg, REG_NONE, DWORD);
   EMIT(0xb8 + (regXReg & 7));
   EMIT4i(-*(int16_t*)&i_hhll(state->i));
   // CMP regXReg, 0
   EMIT_REX_RBI(REG_NONE, regXReg, REG_NONE, DWORD);
   EMIT(0x83);
   EMIT(MODRM_REG_OPX_IMM8(7, regXReg));
   EMIT(0);

   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_n(state, regFN);
}

static void cpu_rec_op_neg(cpu_state *state)
{
   int regXReg = HOSTREG_STATE_VAR_RW(r[i_yx(state->i)], WORD);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // NEG regXReg
   EMIT_REX_RBI(REG_NONE, regXReg, REG_NONE, DWORD);
   EMIT(0xf7);
   EMIT(MODRM_REG_DIRECT(3, regXReg));
   // CMP regXReg, 0
   EMIT_REX_RBI(REG_NONE, regXReg, REG_NONE, DWORD);
   EMIT(0x83);
   EMIT(MODRM_REG_OPX_IMM8(7, regXReg));
   EMIT(0);

   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_n(state, regFN);
}

static void cpu_rec_op_neg_r2(cpu_state *state)
{
   int regXReg = HOSTREG_STATE_VAR_W(r[i_yx(state->i)], WORD);
   int regYReg = HOSTREG_STATE_VAR_R(r[i_yx(state->i)], WORD);
   int regFZ = HOSTREG_STATE_VAR_W(f.z, BYTE);
   int regFN = HOSTREG_STATE_VAR_W(f.n, BYTE);

   // MOV regXReg, regYReg
   EMIT_REX_RBI(regXReg, regYReg, REG_NONE, DWORD);
   EMIT(0x8b);
   EMIT(MODRM_REG_DIRECT(regXReg, regYReg));
   // NEG regXReg
   EMIT_REX_RBI(REG_NONE, regXReg, REG_NONE, DWORD);
   EMIT(0xf7);
   EMIT(MODRM_REG_DIRECT(3, regXReg));
   // CMP regXReg, 0
   EMIT_REX_RBI(REG_NONE, regXReg, REG_NONE, DWORD);
   EMIT(0x83);
   EMIT(MODRM_REG_OPX_IMM8(7, regXReg));
   EMIT(0);

   cpu_rec_flag_z(state, regFZ);
   cpu_rec_flag_n(state, regFN);
}

void cpu_rec_dispatch(cpu_state *state, uint8_t op)
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
      case 0x11:  // JMC
         cpu_rec_op_jmc(state);
         break;
      case 0x12:  // Jx
         cpu_rec_op_jx_cx(state, 0);
         break;
      case 0x13:  // JME
         cpu_rec_op_jme(state);
         break;
      case 0x14:  // CALL (imm)
         cpu_rec_op_call_imm(state);
         break;
      case 0x15:  // RET
         cpu_rec_op_ret(state);
         break;
      case 0x16:  // JMP
         cpu_rec_op_jmp(state);
         break;
      case 0x17:  // Cx
         cpu_rec_op_jx_cx(state, 1);
         break;
      case 0x18:  // CALL
         cpu_rec_op_call(state);
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
      case 0x61:  // AND
         cpu_rec_op_and(state);
         break;
      case 0x62:  // AND r3
         cpu_rec_op_and_r3(state);
         break;
      case 0x63:  // TSTI
         cpu_rec_op_tsti(state);
         break;
      case 0x64:  // TST
         cpu_rec_op_tst(state);
         break;
      case 0x70:  // ORI
         cpu_rec_op_ori(state);
         break;
      case 0x71:  // OR
         cpu_rec_op_or(state);
         break;
      case 0x72:  // OR r3
         cpu_rec_op_or_r3(state);
         break;
      case 0x80:  // XORI
         cpu_rec_op_xori(state);
         break;
      case 0x81:  // XOR
         cpu_rec_op_xor(state);
         break;
      case 0x82:  // XOR r3
         cpu_rec_op_xor_r3(state);
         break;
      case 0x90:  // MULI
         cpu_rec_op_muli(state);
         break;
      case 0x91:  // MUL r2
         cpu_rec_op_mul_r2(state);
         break;
      case 0xa2:  // DIV r3
         cpu_rec_op_div_r3(state);
         break;
      case 0xb0:  // SHL n
         cpu_rec_op_shl_n(state);
         break;
      case 0xb1:  // SHR n
         cpu_rec_op_shr_n(state);
         break;
      case 0xb2:  // SAR n
         cpu_rec_op_sar_n(state);
         break;
      case 0xb3:  // SHL
         cpu_rec_op_shl(state);
         break;
      case 0xb4:  // SHR
         cpu_rec_op_shr(state);
         break;
      case 0xb5:  // SAR
         cpu_rec_op_sar(state);
         break;
      case 0xc0:  // PUSH
         cpu_rec_op_push(state);
         break;
      case 0xc1:  // POP 
         cpu_rec_op_pop(state);
         break;
      case 0xc2:  // PUSHALL
         cpu_rec_op_pushall(state);
         break;
      case 0xc3:  // POPALL
         cpu_rec_op_popall(state);
         break;
      case 0xc4:  // PUSHF
         cpu_rec_op_pushf(state);
         break;
      case 0xc5:  // POPF
         cpu_rec_op_popf(state);
         break;
      case 0xe0:  // NOTI
         cpu_rec_op_noti(state);
         break;
      case 0xe1:  // NOT
         cpu_rec_op_not(state);
         break;
      case 0xe2:  // NOT r2
         cpu_rec_op_not_r2(state);
         break;
      case 0xe3:  // NEGI
         cpu_rec_op_negi(state);
         break;
      case 0xe4:  // NEG
         cpu_rec_op_neg(state);
         break;
      case 0xe5:  // NEG r2
         cpu_rec_op_neg_r2(state);
         break;
      default:
      {
         void *op_instr = (void *)op_table[op].impl;

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

