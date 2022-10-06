
#define SAVE_ALL                \
	pushq	%rbp            ;\
	movq	%rsp,%rbp       ;\
	movw	%ds ,-2(%rbp)   ;\
	movw	%es ,-4(%rbp)   ;\
	movw	%gs ,-6(%rbp)   ;\
	movw	%fs ,-8(%rbp)   ;\
	movq	%rax,-16(%rbp)  ;\
	movq	%rbx,-24(%rbp)  ;\
	movq	%rcx,-32(%rbp)  ;\
	movq	%rdx,-40(%rbp)  ;\
	movq	%rdi,-48(%rbp)  ;\
	movq	%rsi,-56(%rbp)  ;\
	movq	%r8 ,-64(%rbp)  ;\
	movq	%r9 ,-72(%rbp)  ;\
	movq	%r10,-80(%rbp)  ;\
	movq	%r11,-88(%rbp)  ;\
	movq	%r12,-96(%rbp)  ;\
	movq	%r13,-104(%rbp) ;\
	movq	%r14,-112(%rbp) ;\
	movq	%r15,-120(%rbp) ;\
	subq	$120,%rsp 	;



#define MK_INTR_ETY(NR, EC)			\
	.text					;\
	.global intr##NR##_entry		;\
	intr##NR##_entry:			;\
	EC					;\
	SAVE_ALL				;\
	movb	$0x20,%al			;\
	out	%al,$0xa0			;\
	out	%al,$0x20			;\
	movq	$##NR##,%rdi			;\
	movq	%rbp,%rsi			;\
	addq	$48,%rsi			;\
	leaq	intr_hdl_table(%rip),%rax	;\
	addq 	$##NR##*8,%rax;\
	callq	*(%rax)				;\
	leaq	intr_exit(%rip),%rax		;\
	jmp	*%rax				;\
	.data					;\
	.quad	intr##NR##_entry


.text
.global intr_exit
intr_exit:
    movw	-2(%rbp)  ,%ds 
    movw	-4(%rbp)  ,%es 
    movw	-6(%rbp)  ,%gs 
    movw	-8(%rbp)  ,%fs 
    movq	-16(%rbp) ,%rax
    movq	-24(%rbp) ,%rbx
    movq	-32(%rbp) ,%rcx
    movq	-40(%rbp) ,%rdx
    movq	-48(%rbp) ,%rdi
    movq	-56(%rbp) ,%rsi
    movq	-64(%rbp) ,%r8 
    movq	-72(%rbp) ,%r9 
    movq	-80(%rbp) ,%r10
    movq	-88(%rbp) ,%r11
    movq	-96(%rbp) ,%r12
    movq	-104(%rbp),%r13
    movq	-112(%rbp),%r14
    movq	-120(%rbp),%r15
    leave
    addq	$8,%rsp
    iretq


.data
.global intr_ety_table
intr_ety_table:


MK_INTR_ETY(0x00,pushq	$0)
MK_INTR_ETY(0x01,pushq	$0)
MK_INTR_ETY(0x02,pushq	$0)
MK_INTR_ETY(0x03,pushq	$0)
MK_INTR_ETY(0x04,pushq	$0)
MK_INTR_ETY(0x05,pushq	$0)
MK_INTR_ETY(0x06,pushq	$0)
MK_INTR_ETY(0x07,pushq	$0)
MK_INTR_ETY(0x08,nop	 )
MK_INTR_ETY(0x09,pushq	$0)
MK_INTR_ETY(0x0a,nop 	 )
MK_INTR_ETY(0x0b,nop 	 )
MK_INTR_ETY(0x0c,pushq	$0)
MK_INTR_ETY(0x0d,nop 	 )
MK_INTR_ETY(0x0e,nop 	 )
MK_INTR_ETY(0x0f,pushq	$0)
MK_INTR_ETY(0x10,pushq	$0)
MK_INTR_ETY(0x11,nop 	 )
MK_INTR_ETY(0x12,pushq	$0)
MK_INTR_ETY(0x13,pushq	$0)
MK_INTR_ETY(0x14,pushq	$0)
MK_INTR_ETY(0x15,pushq	$0)
MK_INTR_ETY(0x16,pushq	$0)
MK_INTR_ETY(0x17,pushq	$0)
MK_INTR_ETY(0x18,nop 	 )
MK_INTR_ETY(0x19,pushq	$0)
MK_INTR_ETY(0x1a,nop 	 )
MK_INTR_ETY(0x1b,nop 	 )
MK_INTR_ETY(0x1c,pushq	$0)
MK_INTR_ETY(0x1d,nop 	 )
MK_INTR_ETY(0x1e,nop 	 )
MK_INTR_ETY(0x1f,pushq	$0)
MK_INTR_ETY(0x20,pushq	$0)
MK_INTR_ETY(0x21,pushq	$0)
MK_INTR_ETY(0x22,pushq	$0)
MK_INTR_ETY(0x23,pushq	$0)
MK_INTR_ETY(0x24,pushq	$0)
MK_INTR_ETY(0x25,pushq	$0)
MK_INTR_ETY(0x26,pushq	$0)
MK_INTR_ETY(0x27,pushq	$0)
MK_INTR_ETY(0x28,pushq	$0)
MK_INTR_ETY(0x29,pushq	$0)
MK_INTR_ETY(0x2a,pushq	$0)
MK_INTR_ETY(0x2b,pushq	$0)
MK_INTR_ETY(0x2c,pushq	$0)
MK_INTR_ETY(0x2d,pushq	$0)
MK_INTR_ETY(0x2e,pushq	$0)
MK_INTR_ETY(0x2f,pushq	$0)


.text
.global switch_to

switch_to:
	pushq 	%rbp
	movq 	%rsp,%rbp
	movq 	%rbx,-8 (%rbp) 
	movq 	%r12,-16(%rbp)
	movq 	%r13,-24(%rbp)
	movq 	%r14,-32(%rbp)
	movq 	%r15,-40(%rbp)
	movq 	%rdi,-48(%rbp)
	movq 	%rsi,-56(%rbp)

	movq 	%rbp,(%rdi)

	movq 	%rsi,%rdi
	movq 	%rsi,%r12
	callq 	pml4_paddr
	movq 	%rax,%cr3

	movq 	(%r12),%rbp

	movq 	-8 (%rbp),%rbx
	movq 	-16(%rbp),%r12
	movq 	-24(%rbp),%r13
	movq 	-32(%rbp),%r14
	movq 	-40(%rbp),%r15
	movq 	-48(%rbp),%rdi
	movq 	-56(%rbp),%rsi

	leave

	ret



.text
.global	system_call
.global	system_ret
system_call:
	pushq 	%rbp
	movq	%rsp,%rbp

	movq	%rax,-8 (%rbp)
	movq	%rcx,-16(%rbp)
	movq	%r11,-24(%rbp)

	movq 	%rbx,-32(%rbp)
	movq 	%r12,-48(%rbp)
	movq 	%r13,-56(%rbp)
	movq 	%r14,-64(%rbp)
	movq 	%r15,-72(%rbp)

	subq	$72,%rsp

	leaq	syscall_table(%rip),%rax
	shlq	$3,-8(%rbp)
	addq 	-8(%rbp),%rax
	callq 	*(%rax)

	movq	%rax,-8 (%rbp)
system_ret:
	movq	-8 (%rbp),%rax
	movq	-16(%rbp),%rcx
	movq	-24(%rbp),%r11

	movq 	-32(%rbp),%rbx
	movq 	-48(%rbp),%r12
	movq 	-56(%rbp),%r13
	movq 	-64(%rbp),%r14
	movq 	-72(%rbp),%r15

	leave

	sysretq

