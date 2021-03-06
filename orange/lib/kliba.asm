[SECTION .data]
global 	disp_pos;
disp_pos	dd	0

[SECTION .text]

; 导出函数
global	disp_str
global 	out_byte
global  in_byte
global 	disp_color_str
global 	enable_irq
global 	disable_irq
global 	enable_int
global 	disable_int
global  port_read
global 	port_write
%include "sconst.inc"
; ========================================================================
;                  void disp_str(char * info);
; ========================================================================
disp_str:
	push	ebp
	mov	ebp, esp

	mov	esi, [ebp + 8]	; pszInfo  pointer,eip,ebp
	mov	edi, [disp_pos]
	mov	ah, 0Fh
.1:
	lodsb
	test	al, al
	jz	.2
	cmp	al, 0Ah	; 是回车吗?换行键是0Ah
	jnz	.3
	push	eax
	mov	eax, edi
	mov	bl, 160
	div	bl
	and	eax, 0FFh
	inc	eax
	mov	bl, 160
	mul	bl
	mov	edi, eax
	pop	eax
	jmp	.1
.3:
	mov	[gs:edi], ax
	add	edi, 2
	jmp	.1

.2:
	mov	[disp_pos], edi

	pop	ebp
	ret

;===========================================================
;将al的值写入dx所指的地址中
out_byte:  
	mov 		edx , [esp +4]
	mov 		al , [esp + 4+ 4]
	out 		dx , al 
	nop 
	nop 
	ret 
;----------------------------------------------------------
in_byte:
	mov 		edx , [esp + 4]
	xor 		eax , eax 
	in 			al , dx 
	nop 
	nop 
	ret 

;===========================================================
disp_color_str:
	push 	ebp 
	mov 	ebp , esp 
	mov 	esi , [ebp+8]
	mov 	edi , [disp_pos]
	mov 	ah  , [ebp + 12]

.1:
	lodsb 
	test 	al , al 
	jz 		.2 
	cmp 	al , 0Ah 
	jnz 	.3 
	push 	eax 
	mov 	eax , edi 
	mov 	bl 	, 160 
	div 	bl 
	and 	eax , 0FFh 
	inc 	eax 
	mov 	bl , 160 
	mul 	bl 
	mov 	edi , eax 
	pop 	eax 
	jmp 	.1 
.3: 
	mov 	[gs:edi] , ax 
	add 	edi , 2 
	jmp 	.1 
.2:
	mov 	[disp_pos] , edi 

	pop 	ebp 
	ret 
;=================================================
;			void disable_irq(int irq )
;=================================================
;disable an interrupt request line by setting an 8259 bit 
;Equivalent:
;	if(irq<8)
;		outbyte(INT_M_CTLMASK,in_byte(INT_M_CTLMASK)|1<<irq)
;	else 
;		outbyte(INT_S_CTLMASK,in_byte(INT_S_CTLMASK)|1<<irq)

disable_irq:
	mov 	ecx , [esp+4]
	pushf 
	cli 
	mov 	ah , 1 
	rol 	ah , cl  ;ah = (1<<(irq%8))将al右移cl位，刚好对应于OCW1S屏蔽位
	cmp 	cl ,8 
	jae 	disable_8
disable_0:
	in 		al , INT_M_CTLMASK
	test 	al , ah 
	jnz		dis_already
	or 		al ,ah 
	out 	INT_M_CTLMASK ,al 
	popf 
	mov 	eax , 1
	ret 
disable_8:
	in 		al ,INT_S_CTLMASK
	test 	al ,ah 
	jnz 	dis_already
	or 		al ,ah 
	out 	INT_S_CTLMASK ,al 
	popf 
	mov 	eax , 1
	ret 
dis_already:
	popf 
	xor 	eax , eax 
	ret 
;-----------------------------------------------
;			void enable_irq(int irq)
;Equivalent:
;	if(irq<8)
;		outbyte(INT_M_CTLMASK,in_byte(INT_M_CTLMASK)|1<<irq)
;	else 
;		outbyte(INT_S_CTLMASK,in_byte(INT_S_CTLMASK)|1<<irq)
enable_irq:
	mov 	ecx , [esp + 4]
	pushf
	cli 
	mov 	ah ,~1
	rol 	ah , cl        ;ah = (1<<(irq%8)) 
	cmp 	cl ,8 
	jae 	enable_8
enable_0:
	in 		al , INT_M_CTLMASK
	and 	al ,ah 
	out 	INT_M_CTLMASK , al 
	popf 
	ret 
enable_8:
	in 		al , INT_S_CTLMASK
	and 	al ,ah 
	out 	INT_S_CTLMASK ,al 
	popf 
	ret 

;---------------------------------------
; open /close interrupt
enable_int:
	sti 
	ret 
disable_int:
	cli 
	ret 
; ========================================================================
;                  void port_read(u16 port, void* buf, int n);
; ========================================================================
port_read:
	mov	edx, [esp + 4]		; port
	mov	edi, [esp + 4 + 4]	; buf
	mov	ecx, [esp + 4 + 4 + 4]	; n
	shr	ecx, 1
	cld
	rep	insw
	ret

; ========================================================================
;                  void port_write(u16 port, void* buf, int n);
; ========================================================================
port_write:
	mov	edx, [esp + 4]		; port
	mov	esi, [esp + 4 + 4]	; buf
	mov	ecx, [esp + 4 + 4 + 4]	; n
	shr	ecx, 1
	cld
	rep	outsw
	ret


