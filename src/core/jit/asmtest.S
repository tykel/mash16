	.file	"asmtest.c"
	.text
	.globl	function
	.type	function, @function
function:
.LFB4:
	.cfi_startproc
	subq	$8, %rsp
	.cfi_def_cfa_offset 16
	movq	(%rdi), %rdi
	movl	$307200, %edx
	movl	$0, %esi
	call	memset
	addq	$8, %rsp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE4:
	.size	function, .-function
	.globl	main
	.type	main, @function
main:
.LFB5:
	.cfi_startproc
	subq	$24, %rsp
	.cfi_def_cfa_offset 32
	movq	%rsp, %rdi
	call	function
	movl	$0, %eax
	addq	$24, %rsp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE5:
	.size	main, .-main
	.ident	"GCC: (GNU) 6.3.1 20170109"
	.section	.note.GNU-stack,"",@progbits
