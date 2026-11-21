bits 64

global sfrt_put_char

section .text

sfrt_put_char:
    push rdi

    mov eax, 1
    mov edi, 1
    mov rsi, rsp
    mov edx, 1
    syscall

    pop rdi
    ret
