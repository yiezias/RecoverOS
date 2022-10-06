
.globl _start
_start:
	call main
	
	movq 	%rax,%rdi
	call	exit
