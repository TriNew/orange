	org 	0100h  ; 将当前偏移地址指针指向参数表达式的地址
	jmp 	LABEL_START    		;start 
%include "fat12hdr.inc"
%include "load.inc"
%include "pm.inc"


;GDT 
LABEL_GDT: 			Descriptor 	0, 	   0,0
LABEL_DESC_FLAT_C:	Descriptor 	0, 0fffffh,DA_CR|DA_32|DA_LIMIT_4K
LABEL_DESC_FLAT_RW:	Descriptor 	0, 0fffffh,DA_DRW|DA_32|DA_LIMIT_4K
LABEL_DESC_VIDEO:	Descriptor 0B8000h, 0ffffh ,DA_DRW|DA_DPL3
GdtLen 	equ 	$ - LABEL_GDT
GdtPtr 	dw 	GdtLen
	dd 	BaseOfLoaderPhyAddr + LABEL_GDT

;Selector
SelectorFloatC 		equ 	LABEL_DESC_FLAT_C 	- LABEL_GDT
SelectorFloatRW 	equ 	LABEL_DESC_FLAT_RW	- LABEL_GDT
SelectorVideo 		equ 	LABEL_DESC_VIDEO	- LABEL_GDT + SA_RPL3

PageDirBase 		equ 	100000h 
PageTblBase 		equ 	101000h 

	
;----------------------------------------------------------	
BaseOfStack 		equ 	0100h
wRootDirSizeLoop 	dw 	14
wSectorNo 		dw 	0
bOdd 			db 	0
dwKernelSize		dd	0		; KERNEL.BIN 文件大小
;string 
KernelFileName 		db 	"KERNEL  BIN",0
MessageLength 		equ 	9
BootMessage: 		db 	"Booting  " ;?????????????????????????????????
Message1 			db 	"Ready.   "
Message2 			db 	"No KERNEL"
;---------------------------------------------------------------------
	
;包含FAT12磁盘头信息，

LABEL_START:
	mov 	ax , cs 
	mov 	ds , ax 
	mov		es , ax 
	mov 	ss , ax 
	mov 	sp , BaseOfStack

	mov 	dh , 0
	call 	DispStrModel

;获取内存信息 int 15h 
	mov 	ebx , 0 	; ebx = 后续值, 开始时需为 0
	mov 	di , _MemChkBuf ; es:di 指向一个地址范围描述符结构(ARDS)
.MemChkLoop:
	mov 	eax , 0E820h 
	mov 	ecx , 20        ; ecx = 地址范围描述符结构的大小
	mov 	edx , 0534D4150h
	int 	15h 
	jc 	.MemChkFail 
	add 	di ,20 
	inc 	dword [_dwMCRNumber]
	cmp 	ebx , 0 
	jne 	.MemChkLoop 
	jmp 	.MemChkOK 
.MemChkFail: 
	mov 	dword [_dwMCRNumber] , 0
.MemChkOK: 



;在A盘的根目录寻找kernal.bin
	mov 	word [wSectorNo] , SectorNoOfRootDirectory
	xor 	ah , ah 
	xor 	dl ,dl 
	int 	13h      ;reset floppy
LABEL_SEARCH_IN_ROOT_DIR_BEGIN:
	cmp 	word [wRootDirSizeLoop] ,0 
	jz 		LABEL_NO_KERNELBIN
	dec 	word [wRootDirSizeLoop]
	mov 	ax , BaseOfKernelFile
	mov 	es , ax 
	mov 	bx , OffsetOfKernelFile
	mov 	ax ,[wSectorNo]
	mov 	cl , 1
	call 	ReadSector 

	mov 	si , KernelFileName 
	mov 	di , OffsetOfKernelFile
	cld 
	mov 	dx , 10h 				;16*32=512
LABEL_SEARCH_FOR_KERNELBIN:
	cmp 	dx , 0          
	jz 		LABEL_GOTO_NEXT_SECTOR_IN_ROOT_DIR
	dec 	dx 
	mov 	cx , 11
LABEL_CMP_FILENAME:
	cmp  	cx , 0
	jz 		LABEL_FILENAME_FOUND
	dec 	cx 
	lodsb       			;ds:si-->al 
	cmp 	al , byte [es:di]
	jz 		LABEL_GO_ON 
	jmp 	LABEL_DIFFERENT
LABEL_GO_ON:
	inc 	di 
	jmp 	LABEL_CMP_FILENAME

LABEL_DIFFERENT:
	and 	di , 0FFE0h
	add 	di , 20h 
	mov 	si , KernelFileName
	jmp 	LABEL_SEARCH_FOR_KERNELBIN

LABEL_GOTO_NEXT_SECTOR_IN_ROOT_DIR:
	add 	word [wSectorNo] , 1
	jmp 	LABEL_SEARCH_IN_ROOT_DIR_BEGIN

LABEL_NO_KERNELBIN:
	mov 	dh ,2
	call 	DispStrModel 
%ifdef _lOADER_DEBUG_
	mov 	ax , 4c00h
	int 	21h 
%else 
	jmp 	$
%endif 

LABEL_FILENAME_FOUND:
;在fat中加载kernel.bin文件
	mov 	ax , RootDirSectors
	and 	di , 0FFE0h       ;??????????????????????????????????

	push 	eax 
	mov 	eax , [es:di + 01Ch ]
	mov 	dword [dwKernelSize] ,eax 
	pop 	eax 
	add 	di , 01Ah           ;此条目对应的开始簇号的偏移
	mov 	cx , word [es:di]
	push 	cx 
	add 	cx , ax 
	add 	cx , DeltaSectorNo
	mov 	ax , BaseOfKernelFile
	mov 	es , ax 
	mov 	bx , OffsetOfKernelFile
	mov 	ax , cx ;kernel.bin文件内容的第一个扇区号
LABEL_GOON_LOADING_FILE:
	push 	ax 
	push 	bx 
	mov 	ah , 0Eh 
	mov 	al , '.'
	mov 	bl , 0Fh
	int 	10h       ;ah=0Eh print a char
	pop 	bx 
	pop 	ax 

	mov 	cl , 1
	call 	ReadSector
	pop 	ax           ;该条目录中下一个FAT的地址
	call 	GetFATEntry
	cmp 	ax ,0FFFh 
	jz 		LABEL_FILE_LOADED
	push 	ax 
	mov 	dx , RootDirSectors 
	add 	ax , dx 
	add 	ax , DeltaSectorNo
	add 	bx , [BPB_BytesPerSec]   ;将bx加512，将kernel.bin的下一扇区复制到es:bx 
	jmp 	LABEL_GOON_LOADING_FILE
 LABEL_FILE_LOADED:
 	call 	KillMotor     				;关闭软驱马达

 	mov 	dh , 1

 	lgdt 	[GdtPtr]

 	cli 

 	in 		al , 92h 
 	or 		al , 00000010h 
 	out 	92h , al 

 	mov 	eax , cr0 
 	or 		eax , 1
 	mov 	cr0 , eax 
;虽然将文件内容的第一字节复制到base:offset处，但内容的偏移是依base为基准
 	jmp 	dword SelectorFloatC:(BaseOfLoaderPhyAddr+LABEL_PM_START)
 KillMotor:
 	push 	dx 
 	mov 	dx , 03F2h 
 	mov 	al , 0
 	out 	dx , al 
 	pop 	dx 
 	ret 
;-------------------------------------------------
;输入是ax，代表当前已读簇号，输出ax，指定下一个要读取的簇号
GetFATEntry:
	push 	es 
	push 	bx   
	push 	ax  
	mov 	ax , BaseOfLoader
	sub 	ax ,0100h
	mov 	es , ax 
	pop 	ax 
	mov 	byte [bOdd] , 0 
	mov 	bx , 3
	mul 	bx 
	mov 	bx , 2
	div 	bx 
	cmp 	dx , 0
	jz 		LABEL_EVEN 
	mov 	byte [bOdd] ,1 
LABEL_EVEN:
	xor 	dx , dx 
	mov 	bx ,[BPB_BytesPerSec]
	div 	bx 
	push 	dx 
	mov 	bx , 0
	add 	ax , SectorNoOfFAT1
	mov 	cl , 2
	call 	ReadSector

	pop 	dx 
	add 	bx , dx 
	mov 	ax , [es:bx] 
	cmp 	byte [bOdd] , 1
	jnz 	LABEL_EVEN_2
	shr 	ax , 4
LABEL_EVEN_2:
	and 	ax ,0FFFh

LABEL_GET_FAT_ENTRY_OK:
	
	pop 	bx 
	pop 	es 
	ret 

;----------------------------------------------------------------------------
; 函数名: ReadSector
;----------------------------------------------------------------------------
; 作用:
;	从第 ax 个 Sector 开始, 将 cl 个 Sector 读入 es:bx 中
ReadSector:
	; -----------------------------------------------------------------------
	; 怎样由扇区号求扇区在磁盘中的位置 (扇区号 -> 柱面号, 起始扇区, 磁头号)
	; -----------------------------------------------------------------------
	; 设扇区号为 x
	;                           ┌ 柱面号 = y >> 1
	;       x           ┌ 商 y	┤
	; -------------- => ┤       └ 磁头号 = y & 1
	;  每磁道扇区数   	│
	;                   └ 余 z => 起始扇区号 = z + 1     扇区按1,2,3……编号
	push	bp
	mov	bp, sp
	sub	esp, 2			; 辟出两个字节的堆栈区域保存要读的扇区数: byte [bp-2]

	mov	byte [bp-2], cl
	push	bx			; 保存 bx
	mov	bl, [BPB_SecPerTrk]	; bl: 除数
	div	bl			; y 在 al 中, z 在 ah 中
	inc	ah			; z ++
	mov	cl, ah			; cl <- 起始扇区号
	mov	dh, al			; dh <- y
	shr	al, 1			; y >> 1 (其实是 y/BPB_NumHeads, 这里BPB_NumHeads=2)
	mov	ch, al			; ch <- 柱面号
	and	dh, 1			; dh & 1 = 磁头号
	pop	bx			; 恢复 bx
	; 至此, "柱面号, 起始扇区, 磁头号" 全部得到 ^^^^^^^^^^^^^^^^^^^^^^^^
	mov	dl, [BS_DrvNum]		; 驱动器号 (0 表示 A 盘)
.GoOnReading:
	mov	ah, 2				; 读
	mov	al, byte [bp-2]		; 读 al 个扇区
	int	13h
	jc	.GoOnReading		; 如果读取错误 CF 会被置为 1, 这时就不停地读, 直到正确为止

	add	esp, 2
	pop	bp

	ret
;----------------------------------------------------------------------------
; 作用:
;	显示一个字符串, 函数开始时 dh 中应该是字符串序号(0-based)
DispStrModel:
	mov 	ax , MessageLength 
	mul 	dh 
	add 	ax , BootMessage 
	mov 	bp , ax 		; ┓
	mov	ax, ds			; ┣ ES:BP = 串地址
	mov	es, ax			; ┛
	mov 	cx , MessageLength 
	mov 	ax , 01301h     ;ah=13h write a string 
	mov 	bx , 0007h 
	mov 	dl , 0
	int 	10h 
	ret 	


[section .s32]
ALIGN 32
[BITS 32]
LABEL_PM_START:
	mov 	ax , SelectorVideo 
	mov 	gs , ax 

	mov 	ax , SelectorFloatRW
	mov 	ds , ax 
	mov 	es , ax 
	mov 	fs , ax 
	mov 	ss , ax 
	mov	esp , TopOfStack 

	push 	szMemChkTitle
	call 	DispStr
	add 	esp , 4

	call 	DispMemInfo
	;启动分页
	call 	SetupPaging

	call 	DispReturn
	push 	szPageComplete
	call 	DispStr
	add 	esp , 4


;将内核移动到指定位置，
;内核是一个elf文件，将内核文件的内容（去掉elf的各种头）移动到线性地址的0x30400
	call 	InitKernel
	
	;-------------------------------------------------
	; 正式进入内核,kernel的KernelEntryPointPhyAddr可以在编译时指定
	jmp SelectorFloatC:KernelEntryPointPhyAddr

%include "lib.inc"
;-------------------------------------------------------------
;显示内存信息
DispMemInfo:
	push 	esi 
	push 	edi 
	push 	ecx 

	mov 	esi , MemChkBuf 	;ARDS首地址
	mov 	ecx , [dwMCRNumber] 	;ARDS个数
.loop:
	mov 	edx , 5
	mov 	edi , ARDStruct 

.1:
	push 	dword 	[esi]
	call 	DispInt
	pop 	eax 
	stosd    		;eax-->es:di,填充ARDS结构
	add 	esi , 4
	dec 	edx 
	cmp 	edx , 0 
	jnz 	.1
	call 	DispReturn 
	cmp 	dword [dwType],1
	jne 	.2 
	mov 	eax , [dwBaseAddrLow]
	add 	eax , [dwLengthLow]
	cmp 	eax , [dwMemSize]     ;获得的是全部物理内存的大小
	jb 	.2
	mov 	[dwMemSize] , eax 
.2:
	loop 	.loop 

	call 	DispReturn
	push 	szRAMSize 
	call 	DispStr;;;;; 
	add  	esp ,4
	
	push 	dword [dwMemSize]
	call 	DispInt 
	add 	esp , 4 

	pop 	ecx 
	pop 	edi 
	pop 	esi
	ret  
SetupPaging:
	xor 	edx , edx 
	mov 	eax , [dwMemSize]
	mov 	ebx , 400000h  ;一个页表对应4M内存空间,除以4M得页目录数量,一个页目录表对应4G空间
	div 	ebx 
	mov 	ecx , eax 
	test 	edx ,edx 
	jz 	.no_remainder 
	inc 	ecx 
.no_remainder:
	push 	ecx   	;根据int 15h获得的内存信息，计算全部物理内存需要的页目录数

;所有线性地址与相应相等的物理地址，并不考虑内存空洞（内存碎片）
	mov 	ax , SelectorFloatRW
	mov 	es , ax 
	mov 	edi , PageDirBase 
	xor 	eax , eax 
	mov 	eax , PageTblBase |PG_P|PG_USU |PG_RWW 
.1:	
	stosd 			;es:di 
	add 	eax , 4096 
	loop 	.1

	pop 	eax 
	mov 	ebx , 1024 
	mul 	ebx 
	mov 	ecx , eax    ;需要的页表总数目
	mov 	edi , PageTblBase
	xor 	eax , eax 
	mov 	eax , PG_P|PG_USU|PG_RWW 
.2:
	stosd 	
	add 	eax , 4096
	loop 	.2
	mov 	eax , PageDirBase
	mov 	cr3 , eax 
	mov 	eax , cr0 
	or 	eax , 80000000h 
	mov 	cr0 , eax 
	jmp 	short .3
.3:
	nop 

	ret 
;--------------------------------------------------------
;InitKernel获得kernael的内容的开始地址
;elf文件的格式在P124
InitKernel:
	xor 	esi , esi 
	mov 	cx , word [BaseOfKernelPhyAddr+2Ch] ;(e_phnum)program header table中有多少个条目
	movzx 	ecx ,cx 
	mov 	esi , [BaseOfKernelPhyAddr +1Ch] ;(e_phoff)program header table在文件中的偏移量
	add 	esi , BaseOfKernelPhyAddr ;rogram header table的物理地址
.Begin: 
	mov 	eax , [esi + 0]
	cmp 	eax , 0 
	jz 	.NoAction 
	push 	dword [esi+010h]    	;段大小
	mov 	eax , [esi+04h] 	;段的第一个字节在文件中的偏移量
	add 	eax , BaseOfKernelPhyAddr;段在内存中的实际偏移
	push 	eax 		 ;源地址
	push 	dword [esi + 08h];取出目的地址p_vaddr(即虚拟地址es：offset) 程序入口地址0x30400
	call 	MemCpy 		 ;目的地址以及以后的各段的虚拟地址在编译是自动完成
	add 	esp , 12 
.NoAction:
	add 	esi , 020h 
	dec 	ecx 
	jnz 	.Begin
	ret 




[SECTION .data]
ALIGN 32 
LABEL_DATA:
_szMemChkTitle: 	db "BaseAddrL BaseAddrH LengthLow LengthHigh   Type",0Ah,0 
_szRAMSize: 		db "RAM size:",0
_szPageComplete:	db "paging completed!",0Ah, 0 
_szReturn: 		db 0Ah,0
;variable
_dwMCRNumber: 	dd 0 	;memory check result 含有多少各ARDS结构
_dwDispPos:	dd  (80*6+0 ) * 2 
_dwMemSize: 	dd 0 
_ARDStruct: 		;address Range Descriptor Structure 
	_dwBaseAddrLow:	dd 0 
	_dwBaseAddrHigh:dd 0 
	_dwLengthLow: 	dd 0 
	_dwLengthHigh: 	dd 0 
	_dwType:	dd 0
_MemChkBuf: 	times 256 db 0 
;-----------------------------------
szMemChkTitle 		equ BaseOfLoaderPhyAddr + _szMemChkTitle
szPageComplete 		equ BaseOfLoaderPhyAddr + _szPageComplete
szRAMSize 		equ BaseOfLoaderPhyAddr + _szRAMSize
szReturn 		equ BaseOfLoaderPhyAddr + _szReturn
dwDispPos 		equ BaseOfLoaderPhyAddr + _dwDispPos
dwMemSize 		equ BaseOfLoaderPhyAddr + _dwMemSize
dwMCRNumber 		equ BaseOfLoaderPhyAddr + _dwMCRNumber
ARDStruct 		equ BaseOfLoaderPhyAddr + _ARDStruct
	dwBaseAddrLow 	equ BaseOfLoaderPhyAddr + _dwBaseAddrLow
	dwBaseAddrHigh 	equ BaseOfLoaderPhyAddr + _dwBaseAddrHigh
	dwLengthLow 	equ BaseOfLoaderPhyAddr + _dwLengthLow
	dwLengthHigh 	equ BaseOfLoaderPhyAddr + _dwLengthHigh
	dwType 		equ BaseOfLoaderPhyAddr + _dwType
MemChkBuf 		equ BaseOfLoaderPhyAddr + _MemChkBuf
KernelEntryPointPhyAddr	equ 30400h 
DataLen 	equ 	$ - LABEL_DATA
