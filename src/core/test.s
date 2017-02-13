.section .text
    .global _start

    _start:
        movw (%rcx), %dx
        movw (%rcx, %rax, 2), %dx
        movw 0xff(%rcx, %rax, 2), %dx
        mov $500, %eax
        cmp $1, (var)
        cmp $1, var

        imul %rsi, %rdi
        shl %cl, %rsi

        cmpb $255, %cl
        cmp %rdx, %rcx

        movl (var2), %ecx
        movw var(%ecx), %dx

        movb $255, var(%rip)
        movl $255, var3(%rip)

.section .data
    var:
        .byte 100
    var2:
        .word 200
    var3:
        .long 0
