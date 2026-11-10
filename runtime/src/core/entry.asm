bits 64

global _start
extern main

section .text

_start:
	call main

	mov edi, eax
	mov rax, 60
	syscall
