	lod_bs	equ	0x500
	mbr_sz	equ 	512
	pg_bs	equ 	0x100000
	knl_buf equ 	0x70000
	knl_ety	equ 	0xffff800000000800

	;置光标位置
	mbr_start:

	mov 	ax,0x0202	;ah功能读磁盘，al扇区数
	mov 	cx,0x0002	;ch柱面号，cl扇区号
	mov 	dh,0x00		;dh磁头号，dl驱动器号
	mov 	bx,lod_bs	;es:bx缓冲区地址
	int 	0x13

	mov 	bh,0
	mov 	ah,2
	mov 	dx,0
	int 	0x10

	;清屏
	mov 	ax,0x0600
	mov 	bx,0x0700
	mov 	cx,0
	mov 	dx,0x184f
	int	0x10

	mov 	ax,0xb800
	mov 	ds,ax

	mov word [0],'M'+0x0700
	mov word [2],'B'+0x0700
	mov word [4],'R'+0x0700

	jmp 	0:lod_bs+lod_start-mbr_sz

	times 	510-($-$$) 	db 	0
	dw 	0xaa55

	gdt_bs:
	dq 	0
	code_des:
	dq 	0x0020980000000000
	data_des:
	dq 	0x0000920000000000
	tss_des:
	dq 	0
	dq 	0
	user_des:
	dq 	0x0000f20000000000
	dq 	0x0020f80000000000

	gdt_bs32:
	dq 	0
	code32_des:
	dq 	0x00cf98000000ffff
	data32_des:
	dq 	0x00cf92000000ffff


	gdt_ptr:
	dw 	$-gdt_bs-1
	dq 	gdt_bs+lod_bs-mbr_sz

	gdt_ptr32:
	dw 	$-gdt_bs32-1
	dd 	gdt_bs32+lod_bs-mbr_sz

	sct_code 	equ 	code_des-gdt_bs
	sct_data 	equ 	data_des-gdt_bs

	sct_code32	equ	code32_des-gdt_bs32
	sct_data32	equ	data32_des-gdt_bs32

	lod_start:
	xor 	ax,ax
	mov 	ds,ax

	;打印'LOD'
	mov word ds:[0],0x0700+'L'
	mov word ds:[2],0x0700+'O'
	mov word ds:[4],0x0700+'D'

	in 	al,0x92
	or 	al,0b10
	out 	0x92,al

	mov 	eax,cr0
	or 	eax,1
	mov 	cr0,eax

	cli

	lgdt 	[gdt_ptr32+lod_bs-mbr_sz]

	jmp 	sct_code32:lod_bs+code32_start-mbr_sz

	section code32
	bits 	32

	code32_start:
	mov 	ax,sct_data32
	mov 	ds,ax
	mov 	es,ax
	mov 	ss,ax
	mov 	gs,ax
	mov 	esp,0xffffffff

	mov word [0xb8000],0x0700+'P'
	mov word [0xb8002],0x0700+'M'
	mov word [0xb8004],0x0700+' '

	mov 	ecx,0x1000

	pml4_clr:
	mov byte [ecx+pg_bs-1],0
	loop 	pml4_clr

	mov dword [pg_bs],pg_bs+0x1007
	mov dword [pg_bs+0x800],pg_bs+0x1007
	mov dword [pg_bs+0x1000],pg_bs+0x2007
	mov dword [pg_bs+0x2000],pg_bs+0x3007

	mov dword [pg_bs+4088],pg_bs+7

	mov 	ecx,256
	xor 	ebx,ebx
	mov 	eax,7
	create_pte:
	mov 	[8*ebx+pg_bs+0x3000],eax
	inc 	ebx
	add 	eax,0x1000
	loop 	create_pte

	lgdt 	[gdt_ptr+lod_bs-mbr_sz]

	mov	eax,	cr4
	or	eax,	100000b
	mov	cr4,	eax

	mov	eax,	pg_bs
	mov	cr3,	eax

	mov	ecx,	0C0000080h
	rdmsr

	or	eax,	101h
	wrmsr

	mov	eax,	cr0
	or	eax,	80000001h
	mov	cr0,	eax

	mov dword [gdt_ptr+lod_bs-mbr_sz+6],0xffff8000
	jmp 	sct_code:lod_bs+code_start-mbr_sz

	section code
	bits 	64

	code_start:
	lgdt 	[gdt_ptr+lod_bs-mbr_sz]
	mov 	rax,0xffff8000000b8000
	mov word [rax+0],0x0700+'L'
	mov word [rax+2],0x0700+'O'
	mov word [rax+4],0x0700+'N'
	mov word [rax+6],0x0700+'G'

	mov	rsp,0xffff80000009f000

	mov 	rax,0xff
	mov 	rdi,knl_buf
	mov 	rcx,3

	call	read_disk
	call 	knl_init

	mov	rax,knl_ety
	jmp 	rax

	;al扇区数，rdi目的地址，rcx 8~39位lba28地址
	read_disk:
	push	rbp
	mov 	rbp,rsp
	mov 	[rbp-8],rdx
	mov 	[rbp-16],rax
	mov 	[rbp-24],rcx

	mov 	dx,0x1f2
	out 	dx,al

	mov 	rax,rcx

	mov 	rcx,3

	write_lba:
	inc 	dx
	out 	dx,al
	shr	rax,8
	loop 	write_lba

	inc 	dx
	or 	al,0xe0
	out 	dx,al

	shr	rax,8
	inc 	dx
	mov 	al,0x20
	out 	dx,al

	not_ready:
	in 	al,dx
	and 	al,0x88
	cmp	al,8
	jne	not_ready

	mov 	rax,[rbp-16]
	mov 	dx,256
	mul	dx
	mov 	rcx,rax
	
	mov 	dx,0x1f0
	go_on_read:
	in 	ax,dx
	mov 	[rdi],ax
	add 	rdi,2
	loop 	go_on_read

	mov 	rcx,[rbp-24]
	mov 	rax,[rbp-16]
	mov 	rdx,[rbp-8]
	leave
	ret

	knl_init:
	xor 	rcx,rcx
	xor 	rbx,rbx
	mov 	rax,[knl_buf+32];程序表偏移
	mov 	bx,[knl_buf+54]	;程序表长度
	mov 	cx,[knl_buf+56]	;程序表数量

	sgmt_ld:
	mov 	rdi,[knl_buf+rax+16]

	mov 	rsi,0xffff800000000000
	cmp 	rdi,rsi
	jb	next_sgmt

	cmp dword [knl_buf+rax],0
	je 	next_sgmt
	mov 	rsi,[knl_buf+rax+8]
	add 	rsi,knl_buf
	push 	rcx

	mov 	rcx,[knl_buf+rax+32]
	cld
	rep 	movsb

	pop 	rcx

	next_sgmt:
	add 	rax,rbx
	loop 	sgmt_ld

	ret


