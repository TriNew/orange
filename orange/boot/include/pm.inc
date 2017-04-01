;段描述符
DA_32       equ  4000h
DA_DPL0		EQU 00h ;DPL=0
DA_DPL1		EQU 20h ;DPL=1
DA_DPL2		EQU 40h ;DPL=3
DA_DPL3		EQU 60h ;DPL=4
SA_RPL0		EQU	0	; ┓
SA_RPL1		EQU	1	; ┣ RPL
SA_RPL2		EQU	2	; ┃
SA_RPL3		EQU	3	; ┛

SA_TIG		EQU	0	; ┓TI
SA_TIL		EQU	4	; ┛
;存储段描述符类型
DA_DR 	    equ 90h 
DA_C	    equ 98h
DA_DRW	    equ 92h
DA_DRWA		equ 93h
DA_CR		equ 9Ah
DA_CCO		equ 9ch
DA_CCOR		equ 9Eh



;统段描述符类型
DA_LDT		equ 82h
DA_TaskGate	equ 85h
DA_386TSS	equ 89h
DA_386CGate	equ 8Ch
DA_386IGate	equ 8Eh
DA_36TGate	equ 8Fh

;----------------------------------------------------------------------------
; 分页机制使用的常量说明
;----------------------------------------------------------------------------
PG_P		EQU	1	; 页存在属性位
PG_RWR		EQU	0	; R/W 属性位值, 读/执行
PG_RWW		EQU	2	; R/W 属性位值, 读/写/执行
PG_USS		EQU	0	; U/S 属性位值, 系统级
PG_USU		EQU	4	; U/S 属性位值, 用户级
DA_LIMIT_4K	EQU	8000h	; 段界限粒度为 4K 字节
;描述符-----------------------------------------------------------------------
;usage:Descriptor Base,Limit,Attribute
;		  Base: dd
;		  Limit:dd(low 20 bits available)
;		  Attr:dw(low 4 bits of higher)
%macro Descriptor 3
       dw	  %2 & 0FFFFh	;段界限
       dw	  %1 & 0FFFFh	;段基址1
       db	  (%1 >> 16) & 0FFh	;段基址3
       dw	  ((%2 >> 8) & 0F00h | (%3 & 0F0FFh))	;属性1+段界限2+属性2
       db	  (%1 >> 24) & 0FFh  ;段基址3
%endmacro;共8字节
       
;door
;usage:Gate Selector,Offset,DCount,Attr
;			Selector: dw
;			Offset  : dd
; 			DCount  : db
;           Attr    : db
%macro Gate 4
		dw 		(%2 & 0FFFFh)
		dw      %1
		dw 		(%3 & 1Fh) | ((%4 << 8) & 0FF00h)
		dw 		((%2 >> 16) & 0FFFFh)
%endmacro  ;8 bytes