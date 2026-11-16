#if defined(__x86_64__)
    .section .text
    .global _start
    .type _start, @function

_start:
    movq %rsp, %rbp
    movq (%rbp), %rdi
    leaq 8(%rbp), %rsi
    leaq 16(%rsi,%rdi,8), %rdx

    andq $-16, %rsp
    xorq %rbp, %rbp
    call main

    movl %eax, %edi
    movl $60, %eax
    syscall
    ud2

    .size _start, .-_start
#elif defined(__i386__)
    .section .text
    .global _start
    .type _start, @function

_start:
    movl %esp, %ebp
    movl (%ebp), %eax
    leal 4(%ebp), %ecx
    leal 8(%ecx,%eax,4), %edx

    andl $-16, %esp
    subl $4, %esp
    xorl %ebp, %ebp
    pushl %edx
    pushl %ecx
    pushl %eax
    call main

    addl $16, %esp
    movl %eax, %ebx
    movl $1, %eax
    int $0x80
    ud2

    .size _start, .-_start
#else
#error "mcrt0 supports only i386 and x86_64"
#endif

    .section .note.GNU-stack,"",@progbits
