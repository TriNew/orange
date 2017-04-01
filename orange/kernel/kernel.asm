;nasm -f elf kernal.asm -o kernal.o
;ld -s kernal.o -o kernal.bin
; gcc -c -o start.o start.c
; ld -s -Ttext 0x30400 -o kernel.bin kernel.o string.o start.o kliba.o  将程序入口地址设置为0x30400

extern 	cstart 
extern 	gdt_ptr
extern 	idt_ptr
extern 	exception_handler
extern 	spurious_irq     ;假的
extern  kernel_main
extern 	proc_table
extern  p_proc_ready
extern 	tss 
extern 	disp_str
extern 	disp_int
extern 	delay
extern 	k_reenter
extern 	clock_handler
extern 	irq_table
extern 	sys_call_table 


[section .bss]
; "RESB", "RESW", "RESD", "RESQ" and "REST"被设计用在模块的 BSS 段中：它们声明
;未初始化的存储空间。每一个带有单个操作数，用来表明字节数，字数，或双字数或其它的需要保留单位。
StackSpace 	resb 	2*1024
;StackTop是一个标记（LABEL），用来指示当前的段内偏移地址，
StackTop: 				

[SECTION .data]
clock_int_msg 	db 	"^",0

[section .text]
global _start    ;默认_start为程序入口地址，global导出start，告诉编译器程序的开始
global restart

global sys_call 

global divide_error 
global single_step_exception 
global nmi 
global breakpoint_exception 
global overflow 
global bounds_check 
global inval_opcode 
global copr_not_available
global double_fault
global copr_seg_overrun 
global inval_tss 
global segment_not_present
global stack_exception 
global general_protection 
global page_fault 
global copr_error 

global 	hwint00
global 	hwint01
global 	hwint02
global 	hwint03
global 	hwint04
global 	hwint05
global 	hwint06
global 	hwint07
global 	hwint08
global 	hwint09
global 	hwint10
global 	hwint11
global 	hwint12
global 	hwint13
global 	hwint14
global 	hwint15

_start: 

	mov 	esp , StackTop
;将旧GDT复制到新GDT中
	sgdt 	[gdt_ptr]   
	call 	cstart
	lgdt	[gdt_ptr]
	
	lidt 	[idt_ptr]
	

	jmp 	SELECTOR_KERNEL_CS:csinit

csinit:
	;ud2
	;jmp 	0x40:0
	push 	0  ;标志寄存器EFLAGS清零 
	popfd 
	;sti
	xor 	eax , eax 
	mov 	ax , SELECTOR_TSS 
	ltr 	ax 
	
	jmp		 kernel_main



%include "sconst.inc"



;-----------------------------------------------------
;如果有出错码，直接将向量号压栈，如果没有出错码，将FFFFFFFF压栈，再压栈向量号 
divide_error:
	push 	0xFFFFFFFF ;no error code 
	push 	0 			;vector_no= 0 
	jmp 	exception 
single_step_exception:
	push 	0xFFFFFFFF ;no error code 
	push 	1 
	jmp 	exception 
nmi:
	push 	0xFFFFFFFF ;no error code 
	push 	2 
	jmp 	exception 
breakpoint_exception:
	push 	0xFFFFFFFF ;no error code 
	push 	3
	jmp 	exception 
overflow:
	push 	0xFFFFFFFF ;no error code 
	push 	4 
	jmp 	exception 
bounds_check:
	push 	0xFFFFFFFF ;no error code 
	push 	5 
	jmp 	exception 
inval_opcode:
	push 	0xFFFFFFFF ;no error code 
	push 	6 
	jmp 	exception 
copr_not_available:
	push 	0xFFFFFFFF ;no error code 
	push 	7 
	jmp 	exception 
double_fault:
	push 	8 
	jmp 	exception 
copr_seg_overrun:
	push 	0xFFFFFFFF ;no error code 
	push 	9 
	jmp 	exception 
inval_tss:
	push 	10
	jmp 	exception 
segment_not_present:
	push 	11
	jmp 	exception 
stack_exception:
	push 	12
	jmp 	exception 
general_protection:
	push 	13
	jmp 	exception 
page_fault:
	push 	14
	jmp 	exception 
copr_error:
	push 	0xFFFFFFFF ;no error code 
	push 	16 
	jmp 	exception 

exception:
	call 	exception_handler
	add 	esp , 4*2        	;esp指向eip
	hlt 

;----------------------------------
%macro hwint_master 	1
	call 	save 
	in 		al , INT_M_CTLMASK
	or 		al ,(1<<%1)
	out 	INT_M_CTLMASK , al 
	mov 	al ,EOI
	out 	INT_M_CTL , al 
	sti 
	push 	%1
	call 	[irq_table  + 4 * %1]
	pop 	ecx 
	cli 
	in 		al , INT_M_CTLMASK
	and 	al ,~(1<< %1)
	out 	INT_M_CTLMASK , al 
	ret 
%endmacro
; --------------------------------------------------------

ALIGN   16
hwint00:                ; Interrupt routine for irq 0 (the clock).
        hwint_master    0
      
       ; call 	save
        ;in 		al , INT_M_CTLMASK
        ;or 		al ,1
       ;out 	INT_M_CTLMASK ,al 

       ;mov 	al ,EOI
       ; out 	INT_M_CTL , al 

       ; sti 
      ;  push 	0
      ;  call 	clock_handler
      ;  add 	esp , 4
      ;  cli 

      ;  in 		al , INT_M_CTLMASK
      ;  and 	al , 0xFE
      ;  out 	INT_M_CTLMASK ,al 
         
      ;  ret  ;重入时跳到restart_reenter,正常时跳到restart
        	 ;以上两个标签是第一次进入内核时使用的程序段

;.restart_v2:
;        mov 	esp , [p_proc_ready]
;        lldt  	[esp + P_LDT_SEL]
;        lea 	eax , [esp + P_STACKTOP]  
;        mov 	dword [tss+TSS3_S_SP0] , eax 
; ;.re_enter:
; .restart_reenter_v2:
; 		dec 	dword [k_reenter]
;        pop 	gs 
;        pop 	fs 
;        pop 	es 
;        pop 	ds 
;        popad 
;        add 	esp , 4
;
;
;        iretd
        ;hwint_master    0

ALIGN   16
hwint01:                ; Interrupt routine for irq 1 (keyboard)
        hwint_master    1

ALIGN   16
hwint02:                ; Interrupt routine for irq 2 (cascade!)
        hwint_master    2

ALIGN   16
hwint03:                ; Interrupt routine for irq 3 (second serial)
        hwint_master    3

ALIGN   16
hwint04:                ; Interrupt routine for irq 4 (first serial)
        hwint_master    4

ALIGN   16
hwint05:                ; Interrupt routine for irq 5 (XT winchester)
        hwint_master    5

ALIGN   16
hwint06:                ; Interrupt routine for irq 6 (floppy)
        hwint_master    6

ALIGN   16
hwint07:                ; Interrupt routine for irq 7 (printer)
        hwint_master    7

; -------------------------------------------------
%macro  hwint_slave     1
		call 	save 
		in  	al , INT_S_CTLMASK
		or 		al , (1 << (%1 -8))
		out 	INT_S_CTLMASK, al 
		mov 	al ,EOI
		out 	INT_M_CTL ,al 
		nop 
		out 	INT_S_CTL,al 
		sti 
        push    %1
        call    [irq_table + 4 * %1]
        pop 	ecx 
        cli 
        in 		al , INT_S_CTLMASK
        and 	al , ~(1 << (%1 - 8))
        out 	INT_S_CTLMASK, al 
        ret
%endmacro
; ------------------------------------------

ALIGN   16
hwint08:                ; Interrupt routine for irq 8 (realtime clock).
        hwint_slave     8

ALIGN   16
hwint09:                ; Interrupt routine for irq 9 (irq 2 redirected)
        hwint_slave     9

ALIGN   16
hwint10:                ; Interrupt routine for irq 10
        hwint_slave     10

ALIGN   16
hwint11:                ; Interrupt routine for irq 11
        hwint_slave     11

ALIGN   16
hwint12:                ; Interrupt routine for irq 12
        hwint_slave     12

ALIGN   16
hwint13:                ; Interrupt routine for irq 13 (FPU exception)
        hwint_slave     13

ALIGN   16
hwint14:                ; Interrupt routine for irq 14 (AT winchester)
        hwint_slave     14

ALIGN   16
hwint15:                ; Interrupt routine for irq 15
        hwint_slave     15
;------------------------------------------------------
save:

	    pushad 
        push 	ds 
        push 	es 
        push 	fs 
        push 	gs 

        mov 	esi , edx 

        mov 	dx , ss 
        mov 	ds , dx 
        mov 	es , dx 
        mov 	fs , dx
   
   		mov 	edx , esi 

   		mov 	esi , esp 

        inc 	dword [k_reenter]
        cmp 	dword [k_reenter], 0 
        jne  	.1

        mov 	esp , StackTop   ;切换到内核栈
        
        push 	restart
        jmp 	[esi + RETADR - P_STACKBASE] 
.1:
		push 	restart_reenter
        jmp 	[esi + RETADR - P_STACKBASE] 
        ;相当于ret，跳出本函数，返回到RETADR地指出
        ;即将eip指向压栈的RETADR     P216

sys_call:
	call 	save 
	
	;sti 

	push 	esi ;esi == esp 

	push 	dword [p_proc_ready]
	push 	edx 
	push 	ecx 
	push 	ebx 

	call 	[sys_call_table + eax * 4] 

	add 	esp , 4 * 4

	pop 	esi 
	mov 	[esi + EAXREG - P_STACKBASE] ,eax 
	;cli 

	ret 
;----------------------------------------------------------
;进入进程
;P_STACKTOP 指向segs结构的ss
restart:
	mov 	esp , [p_proc_ready ];Addr of regs
	lldt 	[esp + P_LDT_SEL]	;LDT的描述符在GDT中，否则无法切换,本地LDT表在PCB中
	;保存被切换进程的指向regs最高地址的esp到tss，以便返回
	lea 	esi , [esp + P_STACKTOP]
	mov 	dword [tss + TSS3_S_SP0] , esi ;esi -->tss.esp0 

restart_reenter:
	dec 	dword [k_reenter]
	pop 	gs 
	pop 	fs 
	pop 	es 
	pop 	ds 
	popad 

	add 	esp ,4 
	
	iretd   		; auto sti ; int auto cli