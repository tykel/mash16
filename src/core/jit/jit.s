	.file	"jit.c"
	.intel_syntax noprefix
	.text
	.globl	jit_new_vreg
	.type	jit_new_vreg, @function
jit_new_vreg:
.LFB2:
	.cfi_startproc
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	mov	rbp, rsp
	.cfi_def_cfa_register 6
	mov	QWORD PTR [rbp-24], rdi
	mov	DWORD PTR [rbp-4], 0
	jmp	.L2
.L5:
	mov	rax, QWORD PTR [rbp-24]
	mov	rdx, QWORD PTR [rax+320]
	mov	eax, DWORD PTR [rbp-4]
	mov	esi, 1
	mov	ecx, eax
	sal	esi, cl
	mov	eax, esi
	cdqe
	and	rax, rdx
	test	rax, rax
	jne	.L3
	mov	rax, QWORD PTR [rbp-24]
	mov	rdx, QWORD PTR [rax+320]
	mov	eax, DWORD PTR [rbp-4]
	mov	esi, 1
	mov	ecx, eax
	sal	esi, cl
	mov	eax, esi
	cdqe
	or	rdx, rax
	mov	rax, QWORD PTR [rbp-24]
	mov	QWORD PTR [rax+320], rdx
	mov	eax, DWORD PTR [rbp-4]
	jmp	.L4
.L3:
	add	DWORD PTR [rbp-4], 1
.L2:
	cmp	DWORD PTR [rbp-4], 63
	jle	.L5
	mov	eax, -1
.L4:
	pop	rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE2:
	.size	jit_new_vreg, .-jit_new_vreg
	.globl	jit_get_vreg
	.type	jit_get_vreg, @function
jit_get_vreg:
.LFB3:
	.cfi_startproc
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	mov	rbp, rsp
	.cfi_def_cfa_register 6
	sub	rsp, 16
	mov	QWORD PTR [rbp-8], rdi
	mov	DWORD PTR [rbp-12], esi
	mov	rax, QWORD PTR [rbp-8]
	mov	edx, DWORD PTR [rbp-12]
	mov	eax, DWORD PTR [rax+rdx*4]
	cmp	eax, -1
	jne	.L7
	mov	rax, QWORD PTR [rbp-8]
	mov	rdi, rax
	call	jit_new_vreg
	mov	ecx, eax
	mov	rax, QWORD PTR [rbp-8]
	mov	edx, DWORD PTR [rbp-12]
	mov	DWORD PTR [rax+rdx*4], ecx
.L7:
	mov	rax, QWORD PTR [rbp-8]
	mov	edx, DWORD PTR [rbp-12]
	mov	eax, DWORD PTR [rax+rdx*4]
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE3:
	.size	jit_get_vreg, .-jit_get_vreg
	.section	.rodata
.LC0:
	.string	"mprotect() failed, errno: %d\n"
	.align 8
.LC1:
	.string	"posix_memalign() failed to allocate"
	.text
	.globl	jit_alloc_blk
	.type	jit_alloc_blk, @function
jit_alloc_blk:
.LFB4:
	.cfi_startproc
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	mov	rbp, rsp
	.cfi_def_cfa_register 6
	sub	rsp, 32
	mov	edi, 30
	call	sysconf
	mov	QWORD PTR [rbp-8], rax
	mov	rax, QWORD PTR [rbp-8]
	add	rax, rax
	mov	rdx, rax
	mov	rcx, QWORD PTR [rbp-8]
	lea	rax, [rbp-24]
	mov	rsi, rcx
	mov	rdi, rax
	call	posix_memalign
	mov	DWORD PTR [rbp-12], eax
	cmp	DWORD PTR [rbp-12], 0
	jne	.L10
	mov	rax, QWORD PTR [rbp-24]
	test	rax, rax
	je	.L10
	mov	rcx, QWORD PTR [rbp-8]
	mov	rax, QWORD PTR [rbp-24]
	mov	edx, 7
	mov	rsi, rcx
	mov	rdi, rax
	call	mprotect
	test	eax, eax
	jns	.L13
	call	__errno_location
	mov	eax, DWORD PTR [rax]
	mov	esi, eax
	mov	edi, OFFSET FLAT:.LC0
	mov	eax, 0
	call	printf
	mov	rax, QWORD PTR [rbp-24]
	mov	rdi, rax
	call	free
	mov	eax, 0
	jmp	.L14
.L10:
	mov	edi, OFFSET FLAT:.LC1
	call	puts
.L13:
	mov	rax, QWORD PTR [rbp-24]
.L14:
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE4:
	.size	jit_alloc_blk, .-jit_alloc_blk
	.globl	jit_host_nop
	.type	jit_host_nop, @function
jit_host_nop:
.LFB5:
	.cfi_startproc
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	mov	rbp, rsp
	.cfi_def_cfa_register 6
	mov	QWORD PTR [rbp-24], rdi
	mov	rax, QWORD PTR [rbp-24]
	mov	QWORD PTR [rbp-8], rax
	mov	rax, QWORD PTR [rbp-8]
	lea	rdx, [rax+1]
	mov	QWORD PTR [rbp-8], rdx
	mov	BYTE PTR [rax], -112
	mov	rax, QWORD PTR [rbp-8]
	pop	rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE5:
	.size	jit_host_nop, .-jit_host_nop
	.globl	jit_host_mov64_r_to_r
	.type	jit_host_mov64_r_to_r, @function
jit_host_mov64_r_to_r:
.LFB6:
	.cfi_startproc
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	mov	rbp, rsp
	.cfi_def_cfa_register 6
	mov	QWORD PTR [rbp-24], rdi
	mov	DWORD PTR [rbp-28], esi
	mov	DWORD PTR [rbp-32], edx
	mov	rax, QWORD PTR [rbp-24]
	mov	QWORD PTR [rbp-8], rax
	cmp	DWORD PTR [rbp-28], 7
	seta	al
	movzx	eax, al
	mov	DWORD PTR [rbp-12], eax
	cmp	DWORD PTR [rbp-32], 7
	jbe	.L18
	mov	eax, 8
	jmp	.L19
.L18:
	mov	eax, 0
.L19:
	mov	DWORD PTR [rbp-16], eax
	mov	rax, QWORD PTR [rbp-8]
	lea	rdx, [rax+1]
	mov	QWORD PTR [rbp-8], rdx
	mov	edx, DWORD PTR [rbp-12]
	mov	ecx, edx
	mov	edx, DWORD PTR [rbp-16]
	add	edx, ecx
	add	edx, 72
	mov	BYTE PTR [rax], dl
	mov	rax, QWORD PTR [rbp-8]
	lea	rdx, [rax+1]
	mov	QWORD PTR [rbp-8], rdx
	mov	BYTE PTR [rax], -119
	mov	rax, QWORD PTR [rbp-8]
	lea	rdx, [rax+1]
	mov	QWORD PTR [rbp-8], rdx
	mov	edx, DWORD PTR [rbp-28]
	sal	edx, 3
	mov	ecx, edx
	and	ecx, 56
	mov	edx, DWORD PTR [rbp-32]
	and	edx, 7
	or	edx, ecx
	sub	edx, 64
	mov	BYTE PTR [rax], dl
	mov	rax, QWORD PTR [rbp-8]
	pop	rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE6:
	.size	jit_host_mov64_r_to_r, .-jit_host_mov64_r_to_r
	.globl	jit_host_mov32_i_to_r
	.type	jit_host_mov32_i_to_r, @function
jit_host_mov32_i_to_r:
.LFB7:
	.cfi_startproc
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	mov	rbp, rsp
	.cfi_def_cfa_register 6
	mov	QWORD PTR [rbp-24], rdi
	mov	DWORD PTR [rbp-28], esi
	mov	DWORD PTR [rbp-32], edx
	mov	rax, QWORD PTR [rbp-24]
	mov	QWORD PTR [rbp-8], rax
	cmp	DWORD PTR [rbp-32], 7
	seta	al
	movzx	eax, al
	mov	DWORD PTR [rbp-12], eax
	mov	rax, QWORD PTR [rbp-8]
	lea	rdx, [rax+1]
	mov	QWORD PTR [rbp-8], rdx
	mov	edx, DWORD PTR [rbp-12]
	add	edx, 72
	mov	BYTE PTR [rax], dl
	mov	rax, QWORD PTR [rbp-8]
	lea	rdx, [rax+1]
	mov	QWORD PTR [rbp-8], rdx
	mov	BYTE PTR [rax], -57
	mov	rax, QWORD PTR [rbp-8]
	lea	rdx, [rax+1]
	mov	QWORD PTR [rbp-8], rdx
	mov	edx, DWORD PTR [rbp-32]
	and	edx, 7
	or	edx, -64
	mov	BYTE PTR [rax], dl
	mov	rax, QWORD PTR [rbp-8]
	mov	edx, DWORD PTR [rbp-28]
	mov	DWORD PTR [rax], edx
	add	QWORD PTR [rbp-8], 4
	mov	rax, QWORD PTR [rbp-8]
	pop	rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE7:
	.size	jit_host_mov32_i_to_r, .-jit_host_mov32_i_to_r
	.globl	jit_host_mov64_a_to_rax
	.type	jit_host_mov64_a_to_rax, @function
jit_host_mov64_a_to_rax:
.LFB8:
	.cfi_startproc
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	mov	rbp, rsp
	.cfi_def_cfa_register 6
	mov	QWORD PTR [rbp-24], rdi
	mov	QWORD PTR [rbp-32], rsi
	mov	rax, QWORD PTR [rbp-24]
	mov	QWORD PTR [rbp-8], rax
	mov	rax, QWORD PTR [rbp-8]
	lea	rdx, [rax+1]
	mov	QWORD PTR [rbp-8], rdx
	mov	BYTE PTR [rax], 72
	mov	rax, QWORD PTR [rbp-8]
	lea	rdx, [rax+1]
	mov	QWORD PTR [rbp-8], rdx
	mov	BYTE PTR [rax], -95
	mov	rax, QWORD PTR [rbp-8]
	mov	rdx, QWORD PTR [rbp-32]
	mov	QWORD PTR [rax], rdx
	add	QWORD PTR [rbp-8], 8
	mov	rax, QWORD PTR [rbp-8]
	pop	rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE8:
	.size	jit_host_mov64_a_to_rax, .-jit_host_mov64_a_to_rax
	.globl	jit_host_lea64_a_to_r
	.type	jit_host_lea64_a_to_r, @function
jit_host_lea64_a_to_r:
.LFB9:
	.cfi_startproc
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	mov	rbp, rsp
	.cfi_def_cfa_register 6
	mov	QWORD PTR [rbp-40], rdi
	mov	QWORD PTR [rbp-48], rsi
	mov	DWORD PTR [rbp-52], edx
	mov	rax, QWORD PTR [rbp-40]
	mov	QWORD PTR [rbp-8], rax
	mov	DWORD PTR [rbp-12], 7
	mov	rax, QWORD PTR [rbp-48]
	mov	edx, DWORD PTR [rbp-12]
	movsx	rcx, edx
	mov	rdx, QWORD PTR [rbp-40]
	add	rdx, rcx
	sub	rax, rdx
	mov	DWORD PTR [rbp-16], eax
	cmp	DWORD PTR [rbp-52], 7
	seta	al
	movzx	eax, al
	mov	DWORD PTR [rbp-20], eax
	mov	rax, QWORD PTR [rbp-8]
	lea	rdx, [rax+1]
	mov	QWORD PTR [rbp-8], rdx
	mov	edx, DWORD PTR [rbp-20]
	add	edx, 72
	mov	BYTE PTR [rax], dl
	mov	rax, QWORD PTR [rbp-8]
	lea	rdx, [rax+1]
	mov	QWORD PTR [rbp-8], rdx
	mov	BYTE PTR [rax], -115
	mov	rax, QWORD PTR [rbp-8]
	lea	rdx, [rax+1]
	mov	QWORD PTR [rbp-8], rdx
	mov	edx, DWORD PTR [rbp-52]
	sal	edx, 3
	and	edx, 56
	add	edx, 5
	mov	BYTE PTR [rax], dl
	mov	rax, QWORD PTR [rbp-8]
	mov	edx, DWORD PTR [rbp-16]
	mov	DWORD PTR [rax], edx
	add	QWORD PTR [rbp-8], 4
	mov	rax, QWORD PTR [rbp-8]
	pop	rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE9:
	.size	jit_host_lea64_a_to_r, .-jit_host_lea64_a_to_r
	.globl	jit_host_call
	.type	jit_host_call, @function
jit_host_call:
.LFB10:
	.cfi_startproc
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	mov	rbp, rsp
	.cfi_def_cfa_register 6
	mov	QWORD PTR [rbp-24], rdi
	mov	QWORD PTR [rbp-32], rsi
	mov	rax, QWORD PTR [rbp-24]
	mov	QWORD PTR [rbp-8], rax
	mov	rdx, QWORD PTR [rbp-32]
	mov	eax, 4294967294
	cmp	rdx, rax
	ja	.L28
	mov	rax, QWORD PTR [rbp-8]
	lea	rdx, [rax+1]
	mov	QWORD PTR [rbp-8], rdx
	mov	BYTE PTR [rax], -24
	mov	rax, QWORD PTR [rbp-32]
	mov	edx, eax
	mov	rax, QWORD PTR [rbp-8]
	mov	DWORD PTR [rax], edx
	add	QWORD PTR [rbp-8], 4
	jmp	.L29
.L28:
	mov	DWORD PTR [rbp-12], 6
	mov	rax, QWORD PTR [rbp-32]
	mov	edx, DWORD PTR [rbp-12]
	movsx	rcx, edx
	mov	rdx, QWORD PTR [rbp-24]
	add	rdx, rcx
	sub	rax, rdx
	mov	DWORD PTR [rbp-16], eax
	mov	rax, QWORD PTR [rbp-8]
	lea	rdx, [rax+1]
	mov	QWORD PTR [rbp-8], rdx
	mov	BYTE PTR [rax], -1
	mov	rax, QWORD PTR [rbp-8]
	lea	rdx, [rax+1]
	mov	QWORD PTR [rbp-8], rdx
	mov	BYTE PTR [rax], 21
	mov	rax, QWORD PTR [rbp-8]
	mov	edx, DWORD PTR [rbp-16]
	mov	DWORD PTR [rax], edx
	add	QWORD PTR [rbp-8], 4
.L29:
	mov	rax, QWORD PTR [rbp-8]
	pop	rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE10:
	.size	jit_host_call, .-jit_host_call
	.globl	jit_host_ret
	.type	jit_host_ret, @function
jit_host_ret:
.LFB11:
	.cfi_startproc
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	mov	rbp, rsp
	.cfi_def_cfa_register 6
	mov	QWORD PTR [rbp-24], rdi
	mov	rax, QWORD PTR [rbp-24]
	mov	QWORD PTR [rbp-8], rax
	mov	rax, QWORD PTR [rbp-8]
	lea	rdx, [rax+1]
	mov	QWORD PTR [rbp-8], rdx
	mov	BYTE PTR [rax], -61
	mov	rax, QWORD PTR [rbp-8]
	pop	rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE11:
	.size	jit_host_ret, .-jit_host_ret
	.globl	jit_emit_nop
	.type	jit_emit_nop, @function
jit_emit_nop:
.LFB12:
	.cfi_startproc
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	mov	rbp, rsp
	.cfi_def_cfa_register 6
	sub	rsp, 16
	mov	QWORD PTR [rbp-8], rdi
	mov	QWORD PTR [rbp-16], rsi
	mov	rax, QWORD PTR [rbp-16]
	mov	rdi, rax
	call	jit_host_nop
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE12:
	.size	jit_emit_nop, .-jit_emit_nop
	.section	.rodata
.LC2:
	.string	"jit_state: %p\n"
	.text
	.globl	jit_emit_cls
	.type	jit_emit_cls, @function
jit_emit_cls:
.LFB13:
	.cfi_startproc
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	mov	rbp, rsp
	.cfi_def_cfa_register 6
	sub	rsp, 32
	mov	QWORD PTR [rbp-24], rdi
	mov	QWORD PTR [rbp-32], rsi
	mov	rax, QWORD PTR [rbp-32]
	mov	QWORD PTR [rbp-8], rax
	mov	rax, QWORD PTR [rbp-8]
	mov	edx, 7
	mov	esi, 307200
	mov	rdi, rax
	call	jit_host_mov32_i_to_r
	mov	QWORD PTR [rbp-8], rax
	mov	rax, QWORD PTR [rbp-8]
	mov	edx, 6
	mov	esi, 0
	mov	rdi, rax
	call	jit_host_mov32_i_to_r
	mov	QWORD PTR [rbp-8], rax
	mov	rax, QWORD PTR [rbp-24]
	mov	rsi, rax
	mov	edi, OFFSET FLAT:.LC2
	mov	eax, 0
	call	printf
	mov	rax, QWORD PTR [rbp-24]
	lea	rdx, [rax+196976]
	mov	rax, QWORD PTR [rbp-8]
	mov	rsi, rdx
	mov	rdi, rax
	call	jit_host_mov64_a_to_rax
	mov	QWORD PTR [rbp-8], rax
	mov	rax, QWORD PTR [rbp-8]
	mov	edx, 2
	mov	esi, 0
	mov	rdi, rax
	call	jit_host_mov64_r_to_r
	mov	QWORD PTR [rbp-8], rax
	mov	rax, QWORD PTR [rbp-8]
	mov	esi, OFFSET FLAT:memset
	mov	rdi, rax
	call	jit_host_call
	mov	QWORD PTR [rbp-8], rax
	mov	rax, QWORD PTR [rbp-8]
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE13:
	.size	jit_emit_cls, .-jit_emit_cls
	.section	.rodata
	.align 8
.LC3:
	.string	"jit: unimplemented emitter for bgc"
	.text
	.globl	jit_emit_bgc
	.type	jit_emit_bgc, @function
jit_emit_bgc:
.LFB14:
	.cfi_startproc
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	mov	rbp, rsp
	.cfi_def_cfa_register 6
	sub	rsp, 16
	mov	QWORD PTR [rbp-8], rdi
	mov	QWORD PTR [rbp-16], rsi
	mov	edi, OFFSET FLAT:.LC3
	call	puts
	mov	rax, QWORD PTR [rbp-16]
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE14:
	.size	jit_emit_bgc, .-jit_emit_bgc
	.section	.rodata
	.align 8
.LC4:
	.string	"jit: unimplemented emitter for jmp imm"
	.text
	.globl	jit_emit_jmpi
	.type	jit_emit_jmpi, @function
jit_emit_jmpi:
.LFB15:
	.cfi_startproc
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	mov	rbp, rsp
	.cfi_def_cfa_register 6
	sub	rsp, 32
	mov	QWORD PTR [rbp-8], rdi
	mov	eax, esi
	mov	QWORD PTR [rbp-24], rdx
	mov	WORD PTR [rbp-12], ax
	mov	edi, OFFSET FLAT:.LC4
	call	puts
	mov	rax, QWORD PTR [rbp-24]
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE15:
	.size	jit_emit_jmpi, .-jit_emit_jmpi
	.section	.rodata
	.align 8
.LC5:
	.string	"jit: unimplemented emitter for jx"
	.text
	.globl	jit_emit_jx
	.type	jit_emit_jx, @function
jit_emit_jx:
.LFB16:
	.cfi_startproc
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	mov	rbp, rsp
	.cfi_def_cfa_register 6
	sub	rsp, 32
	mov	QWORD PTR [rbp-8], rdi
	mov	eax, esi
	mov	QWORD PTR [rbp-24], rdx
	mov	WORD PTR [rbp-12], ax
	mov	edi, OFFSET FLAT:.LC5
	call	puts
	mov	rax, QWORD PTR [rbp-24]
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE16:
	.size	jit_emit_jx, .-jit_emit_jx
	.section	.rodata
	.align 8
.LC6:
	.string	"jit: unimplemented emitter for ldi imm"
	.text
	.globl	jit_emit_ldii
	.type	jit_emit_ldii, @function
jit_emit_ldii:
.LFB17:
	.cfi_startproc
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	mov	rbp, rsp
	.cfi_def_cfa_register 6
	sub	rsp, 16
	mov	QWORD PTR [rbp-8], rdi
	mov	QWORD PTR [rbp-16], rsi
	mov	edi, OFFSET FLAT:.LC6
	call	puts
	mov	rax, QWORD PTR [rbp-16]
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE17:
	.size	jit_emit_ldii, .-jit_emit_ldii
	.section	.rodata
	.align 8
.LC7:
	.string	"jit: chip16 opcode %02x unimplemented, treating as nop\n"
	.text
	.globl	jit_chip16_to_host
	.type	jit_chip16_to_host, @function
jit_chip16_to_host:
.LFB18:
	.cfi_startproc
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	mov	rbp, rsp
	.cfi_def_cfa_register 6
	sub	rsp, 48
	mov	QWORD PTR [rbp-24], rdi
	mov	eax, esi
	mov	QWORD PTR [rbp-40], rdx
	mov	WORD PTR [rbp-28], ax
	mov	QWORD PTR [rbp-8], 0
	movzx	eax, WORD PTR [rbp-28]
	mov	rdx, QWORD PTR [rbp-24]
	cdqe
	movzx	eax, BYTE PTR [rdx+131440+rax]
	mov	BYTE PTR [rbp-9], al
	movzx	eax, BYTE PTR [rbp-9]
	cmp	eax, 32
	ja	.L46
	mov	eax, eax
	mov	rax, QWORD PTR .L48[0+rax*8]
	jmp	rax
	.section	.rodata
	.align 8
	.align 4
.L48:
	.quad	.L46
	.quad	.L47
	.quad	.L46
	.quad	.L49
	.quad	.L46
	.quad	.L46
	.quad	.L46
	.quad	.L46
	.quad	.L46
	.quad	.L46
	.quad	.L46
	.quad	.L46
	.quad	.L46
	.quad	.L46
	.quad	.L46
	.quad	.L46
	.quad	.L50
	.quad	.L46
	.quad	.L51
	.quad	.L46
	.quad	.L46
	.quad	.L46
	.quad	.L46
	.quad	.L46
	.quad	.L46
	.quad	.L46
	.quad	.L46
	.quad	.L46
	.quad	.L46
	.quad	.L46
	.quad	.L46
	.quad	.L46
	.quad	.L52
	.text
.L47:
	mov	rdx, QWORD PTR [rbp-40]
	mov	rax, QWORD PTR [rbp-24]
	mov	rsi, rdx
	mov	rdi, rax
	call	jit_emit_cls
	mov	QWORD PTR [rbp-8], rax
	jmp	.L53
.L49:
	mov	rdx, QWORD PTR [rbp-40]
	mov	rax, QWORD PTR [rbp-24]
	mov	rsi, rdx
	mov	rdi, rax
	call	jit_emit_bgc
	mov	QWORD PTR [rbp-8], rax
	jmp	.L53
.L50:
	movzx	ecx, WORD PTR [rbp-28]
	mov	rdx, QWORD PTR [rbp-40]
	mov	rax, QWORD PTR [rbp-24]
	mov	esi, ecx
	mov	rdi, rax
	call	jit_emit_jmpi
	mov	QWORD PTR [rbp-8], rax
	jmp	.L53
.L51:
	movzx	ecx, WORD PTR [rbp-28]
	mov	rdx, QWORD PTR [rbp-40]
	mov	rax, QWORD PTR [rbp-24]
	mov	esi, ecx
	mov	rdi, rax
	call	jit_emit_jx
	mov	QWORD PTR [rbp-8], rax
	jmp	.L53
.L52:
	mov	rdx, QWORD PTR [rbp-40]
	mov	rax, QWORD PTR [rbp-24]
	mov	rsi, rdx
	mov	rdi, rax
	call	jit_emit_ldii
	mov	QWORD PTR [rbp-8], rax
	jmp	.L53
.L46:
	movzx	eax, BYTE PTR [rbp-9]
	mov	esi, eax
	mov	edi, OFFSET FLAT:.LC7
	mov	eax, 0
	call	printf
	mov	rdx, QWORD PTR [rbp-40]
	mov	rax, QWORD PTR [rbp-24]
	mov	rsi, rdx
	mov	rdi, rax
	call	jit_emit_nop
	mov	QWORD PTR [rbp-8], rax
	nop
.L53:
	mov	rax, QWORD PTR [rbp-8]
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE18:
	.size	jit_chip16_to_host, .-jit_chip16_to_host
	.section	.rodata
	.align 8
.LC8:
	.string	"jit: got block %p, host pc %p\n"
	.align 8
.LC9:
	.string	"basic block: %04x .. %04x, size %d bytes (%d instructions)\n"
	.align 8
.LC10:
	.string	"emitted x86-64 code, %p ... %p):\n"
.LC11:
	.string	"%02x "
	.text
	.globl	jit_recompile_blk
	.type	jit_recompile_blk, @function
jit_recompile_blk:
.LFB19:
	.cfi_startproc
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	mov	rbp, rsp
	.cfi_def_cfa_register 6
	sub	rsp, 64
	mov	QWORD PTR [rbp-56], rdi
	mov	eax, esi
	mov	WORD PTR [rbp-60], ax
	mov	DWORD PTR [rbp-28], 0
	mov	DWORD PTR [rbp-32], 0
	movzx	eax, WORD PTR [rbp-60]
	mov	WORD PTR [rbp-6], ax
	call	jit_alloc_blk
	mov	QWORD PTR [rbp-40], rax
	mov	rax, QWORD PTR [rbp-40]
	mov	QWORD PTR [rbp-16], rax
	mov	rdx, QWORD PTR [rbp-16]
	mov	rax, QWORD PTR [rbp-40]
	mov	rsi, rax
	mov	edi, OFFSET FLAT:.LC8
	mov	eax, 0
	call	printf
	movzx	eax, WORD PTR [rbp-60]
	mov	DWORD PTR [rbp-4], eax
	jmp	.L56
.L59:
	mov	rdx, QWORD PTR [rbp-56]
	mov	eax, DWORD PTR [rbp-4]
	cdqe
	movzx	eax, BYTE PTR [rdx+131440+rax]
	mov	BYTE PTR [rbp-41], al
	mov	eax, DWORD PTR [rbp-4]
	movzx	ecx, ax
	mov	rdx, QWORD PTR [rbp-16]
	mov	rax, QWORD PTR [rbp-56]
	mov	esi, ecx
	mov	rdi, rax
	call	jit_chip16_to_host
	mov	QWORD PTR [rbp-16], rax
	cmp	BYTE PTR [rbp-41], 15
	jbe	.L57
	cmp	BYTE PTR [rbp-41], 24
	ja	.L57
	mov	eax, DWORD PTR [rbp-4]
	mov	WORD PTR [rbp-6], ax
	jmp	.L58
.L57:
	add	DWORD PTR [rbp-4], 4
.L56:
	cmp	DWORD PTR [rbp-4], 65535
	jle	.L59
.L58:
	mov	rax, QWORD PTR [rbp-16]
	mov	rdi, rax
	call	jit_host_ret
	mov	QWORD PTR [rbp-16], rax
	movzx	edx, WORD PTR [rbp-6]
	movzx	eax, WORD PTR [rbp-60]
	sub	edx, eax
	mov	eax, edx
	add	eax, 4
	mov	DWORD PTR [rbp-28], eax
	mov	eax, DWORD PTR [rbp-28]
	lea	edx, [rax+3]
	test	eax, eax
	cmovs	eax, edx
	sar	eax, 2
	mov	DWORD PTR [rbp-32], eax
	movzx	edx, WORD PTR [rbp-6]
	movzx	eax, WORD PTR [rbp-60]
	mov	esi, DWORD PTR [rbp-32]
	mov	ecx, DWORD PTR [rbp-28]
	mov	r8d, esi
	mov	esi, eax
	mov	edi, OFFSET FLAT:.LC9
	mov	eax, 0
	call	printf
	mov	rdx, QWORD PTR [rbp-16]
	mov	rax, QWORD PTR [rbp-40]
	mov	rsi, rax
	mov	edi, OFFSET FLAT:.LC10
	mov	eax, 0
	call	printf
	mov	rax, QWORD PTR [rbp-40]
	mov	QWORD PTR [rbp-24], rax
	jmp	.L60
.L61:
	mov	rax, QWORD PTR [rbp-24]
	movzx	eax, BYTE PTR [rax]
	movzx	eax, al
	mov	esi, eax
	mov	edi, OFFSET FLAT:.LC11
	mov	eax, 0
	call	printf
	add	QWORD PTR [rbp-24], 1
.L60:
	mov	rax, QWORD PTR [rbp-24]
	cmp	rax, QWORD PTR [rbp-16]
	jb	.L61
	mov	edi, 10
	call	putchar
	mov	rax, QWORD PTR [rbp-40]
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE19:
	.size	jit_recompile_blk, .-jit_recompile_blk
	.globl	jit_get_blk
	.type	jit_get_blk, @function
jit_get_blk:
.LFB20:
	.cfi_startproc
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	mov	rbp, rsp
	.cfi_def_cfa_register 6
	push	rbx
	sub	rsp, 40
	.cfi_offset 3, -24
	mov	QWORD PTR [rbp-40], rdi
	mov	eax, esi
	mov	WORD PTR [rbp-44], ax
	movzx	eax, WORD PTR [rbp-44]
	shr	ax, 2
	mov	WORD PTR [rbp-18], ax
	movzx	edx, WORD PTR [rbp-18]
	mov	rax, QWORD PTR [rbp-40]
	movsx	rdx, edx
	add	rdx, 40
	mov	rax, QWORD PTR [rax+8+rdx*8]
	test	rax, rax
	jne	.L64
	movzx	ebx, WORD PTR [rbp-18]
	movzx	edx, WORD PTR [rbp-44]
	mov	rax, QWORD PTR [rbp-40]
	mov	esi, edx
	mov	rdi, rax
	call	jit_recompile_blk
	mov	rcx, rax
	mov	rax, QWORD PTR [rbp-40]
	movsx	rdx, ebx
	add	rdx, 40
	mov	QWORD PTR [rax+8+rdx*8], rcx
.L64:
	movzx	edx, WORD PTR [rbp-18]
	mov	rax, QWORD PTR [rbp-40]
	movsx	rdx, edx
	add	rdx, 40
	mov	rax, QWORD PTR [rax+8+rdx*8]
	add	rsp, 40
	pop	rbx
	pop	rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE20:
	.size	jit_get_blk, .-jit_get_blk
	.globl	test_function
	.type	test_function, @function
test_function:
.LFB21:
	.cfi_startproc
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	mov	rbp, rsp
	.cfi_def_cfa_register 6
	mov	edx, 76800
	mov	esi, 0
	mov	edi, 287454020
	call	memset
	nop
	pop	rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE21:
	.size	test_function, .-test_function
	.section	.rodata
	.align 8
.LC12:
	.string	"chip16 framebuffer was cleared -- cls execution SUCCESS"
	.align 8
.LC13:
	.string	"chip16 framebuffer was NOT cleared -- cls execution FAILED"
	.text
	.globl	main
	.type	main, @function
main:
.LFB22:
	.cfi_startproc
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	mov	rbp, rsp
	.cfi_def_cfa_register 6
	sub	rsp, 64
	mov	DWORD PTR [rbp-52], edi
	mov	QWORD PTR [rbp-64], rsi
	mov	edi, 504176
	call	malloc
	mov	QWORD PTR [rbp-8], rax
	mov	BYTE PTR [rbp-48], 1
	mov	BYTE PTR [rbp-47], 0
	mov	BYTE PTR [rbp-46], 0
	mov	BYTE PTR [rbp-45], 0
	mov	BYTE PTR [rbp-44], 3
	mov	BYTE PTR [rbp-43], 0
	mov	BYTE PTR [rbp-42], 0
	mov	BYTE PTR [rbp-41], 0
	mov	BYTE PTR [rbp-40], 32
	mov	BYTE PTR [rbp-39], 0
	mov	BYTE PTR [rbp-38], -51
	mov	BYTE PTR [rbp-37], -85
	mov	BYTE PTR [rbp-36], 18
	mov	BYTE PTR [rbp-35], 0
	mov	BYTE PTR [rbp-34], 0
	mov	BYTE PTR [rbp-33], 0
	mov	BYTE PTR [rbp-32], 16
	mov	BYTE PTR [rbp-31], 0
	mov	BYTE PTR [rbp-30], 16
	mov	BYTE PTR [rbp-29], 0
	mov	rax, QWORD PTR [rbp-8]
	add	rax, 131440
	mov	rdx, QWORD PTR [rbp-48]
	mov	QWORD PTR [rax], rdx
	mov	rdx, QWORD PTR [rbp-40]
	mov	QWORD PTR [rax+8], rdx
	mov	edx, DWORD PTR [rbp-32]
	mov	DWORD PTR [rax+16], edx
	mov	rax, QWORD PTR [rbp-8]
	add	rax, 196976
	mov	edx, 307200
	mov	esi, 255
	mov	rdi, rax
	call	memset
	mov	rax, QWORD PTR [rbp-8]
	mov	esi, 0
	mov	rdi, rax
	call	jit_get_blk
	mov	QWORD PTR [rbp-16], rax
	mov	rdx, QWORD PTR [rbp-16]
	mov	eax, 0
	call	rdx
	mov	rax, QWORD PTR [rbp-8]
	movzx	eax, BYTE PTR [rax+196976]
	test	al, al
	jne	.L68
	mov	rax, QWORD PTR [rbp-8]
	movzx	eax, BYTE PTR [rax+504175]
	test	al, al
	jne	.L68
	mov	edi, OFFSET FLAT:.LC12
	call	puts
	jmp	.L69
.L68:
	mov	edi, OFFSET FLAT:.LC13
	call	puts
.L69:
	mov	rax, QWORD PTR [rbp-8]
	mov	rdi, rax
	call	free
	mov	eax, 0
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE22:
	.size	main, .-main
	.ident	"GCC: (GNU) 6.3.1 20170109"
	.section	.note.GNU-stack,"",@progbits
