#include "sulfur/codegen.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void push(const char* src, char** dst) {
	if (dst  == NULL) return;
	if (*dst == NULL) return;
	if (src  == NULL) return;

	*dst = realloc(
		*dst,
		strlen(*dst) + (strlen(src) * sizeof(char)) + 1
	);

	strcat(*dst, src);
}

char* sfGenerateAssembly(const sfIRProgram* program) {
	char* as = strdup("");

	push("bits 64\n", &as);

	push("section .data\n", &as);

	// static data goes here

	push("section .text\n", &as);
	push("	global _start\n", &as);

	push("_start:\n", &as);
	push("	push rbp\n", &as);
	push("	mov rbp, rsp\n", &as);
	push("	sub rsp, 64\n", &as);

	// code goes here

	push("	mov rsp, rbp\n", &as);
	push("	pop rbp\n", &as);

	push("	mov rax, 60\n", &as);
	push("	xor rdi, rdi\n", &as);
	push("	syscall\n", &as);

	return as;
}
