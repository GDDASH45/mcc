.global _start
.text

_start:
    mov rax, 60
    mov rdi, 42
    syscall