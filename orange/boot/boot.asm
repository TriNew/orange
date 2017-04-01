;============================================================================
;(1) 刚开机时CPU 处于实模式(和保护模式对应，实模式的寻址CS:IP(CS 左移4 位+IP) ，和保护模式不一样!)
;(2) 开机时，CS=0xFFFF; IP=0x0000
;(3) 寻址0xFFFF0(ROM BIOS 映射区)
;(4) 检查RAM ，键盘，显示器，软硬磁盘
;(5) 将磁盘0 磁道0 扇区读入0x7c00 处
;(6) 设置cs=0x07c0 ，ip=0x0000

;引导扇区共512B。前62字节是格式化参数，中间448字节是引导代码，数据以及其它填充字符等
;============================================================================
;%define	_BOOT_DEBUG_
%ifdef	_BOOT_DEBUG_
	org  0100h			; 调试状态, 做成 .COM 文件, 可调试
%else
	org  07c00h			; Boot 状态, Bios 将把 Boot Sector 加载到 0:7C00 处并开始执行
%endif
%ifdef	_BOOT_DEBUG_
BaseOfStack		equ	0100h	; 调试状态下堆栈基地址(栈底, 从这个位置向低地址生长)
%else
BaseOfStack		equ	07c00h	; 堆栈基地址(栈底, 从这个位置向低地址生长)
%endif

;---------------------------------------------	
	jmp 	short LABEL_START
	nop 			;NOP is must 
;包含FAT12磁盘头信息，
%include "fat12hdr.inc"
	
LABEL_START:
	mov 	ax , cs 
	mov 	ds , ax
	mov 	es , ax
	mov 	ss , ax
	mov 	sp ,BaseOfStack

	;clear screen
	mov 	ax , 0600h
	mov	 	bx , 0700h 
	mov 	cx , 0
	mov 	dx , 0184fh
	int 	10h 

	xor 	ah ,ah 	;通过int 13h进行软驱复位
	xor 	dl , dl 
	int 	13h 

	;在A盘的根目录寻找loader.bin 
	;一次读一个扇区到BaseOfLoader:OffsetOfLoader,用逐条比较的方法寻找loader.bin
	mov 	word [wSectorNo] , SectorNoOfRootDirectory ;根目录开始于19扇区
LABEL_SEARCH_IN_ROOT_DIR_BEGIN:
	cmp 	word [wRootDirSizeForLoop],0		;根目录占用空间(扇区数)
	jz 		LABEL_NO_LOADERBIN					;RootDirSectors 和 0比较,判断根目录是否读完
	dec 	word [wRootDirSizeForLoop]
	mov 	ax , BaseOfLoader
	mov 	es , ax							;es<-BaseOfLoader
	mov 	bx , OffsetOfLoader 			;bx<-OffsetOfLoader,es:bx指定扇区读取到内存的起始位置
	mov 	ax , [wSectorNo] 				;ax<-根目录中的某个Sector号,ReadSector读取从此号开始的扇区
	mov 	cl , 1 							;指定读取一个扇区
	call 	ReadSector

	mov 	si , LoaderFileName 			;ds:si->"LOADER BIN"
	mov 	di , OffsetOfLoader				;es:di->BaseOfLoader:0100
	cld 
	mov 	dx , 10h
	
LABEL_SEARCH_FOR_LOADERBIN:
	cmp 	dx , 0  						;16h×32B=512B，一个扇区的根目录数量,遍历一个扇区的所有根目录
	jz 		LABEL_GOTO_NEXT_SECTOR_IN_ROOT_DIR
	dec 	dx 
	mov 	cx ,11 
LABEL_CMP_FILENAME:
	cmp 	cx , 0      		;共比较11次字符,判断是否为loader.bin
	jz 		LABEL_FILENAME_FOUND
	dec 	cx
	lodsb 						;ds:si->al,逐字节比较
	cmp 	al , byte [es:di] 	;if  equ go no 
	jz 		LABEL_GO_ON 
	jmp 	LABEL_DIFFERENT    ;和本扇区的下一个目录进行比较

LABEL_GO_ON:
	inc 	di 
	jmp 	LABEL_CMP_FILENAME

LABEL_DIFFERENT:
	and 	di , 0FFE0h  ;指向本条目开头,屏蔽5位，32B
	add 	di ,20h
	mov 	si , LoaderFileName
	jmp 	LABEL_SEARCH_FOR_LOADERBIN
LABEL_GOTO_NEXT_SECTOR_IN_ROOT_DIR:
	add 	word [wSectorNo] ,1
	jmp 	LABEL_SEARCH_IN_ROOT_DIR_BEGIN
LABEL_NO_LOADERBIN:
	mov 	dh , 2
	call 	DispStr
%ifdef 		_BOOT_DEBUG_
		mov 	ax , 4c00h
		int 	21h 	 ; /  没有找到 LOADER.BIN,	
%else 
		jmp 	$
%endif 

LABEL_FILENAME_FOUND:
	mov 	ax , RootDirSectors ;根目录扇区数,
	and 	di , 0FFE0h
	add 	di , 01Ah    		;文件目录的簇号偏移在1Ah处
	mov 	cx , word [es:di] 	;找到当前目录对应的文件的起始簇号
	push 	cx 
	add 	cx , ax 
	add 	cx , DeltaSectorNo  ;cx为文件的起始扇区号，(从引导扇区的0扇区开始的)
	mov 	ax , BaseOfLoader
	mov 	es , ax 
	mov 	bx , OffsetOfLoader
	mov 	ax , cx 			;
LABEL_GOON_LOADING_FILE:
	push 	ax 
	push 	bx 
	mov 	ah , 0Eh    ;int 10h:AH=0Eh,BH = Page Number, BL = Color (only in graphic mode)
	mov 	al , '.'	;AL = Character
	mov 	bl ,0Fh 	;每读一个扇区打印一个“.”
	int 	10h         ;Display a character on the screen, advancing the cursor and scrolling the screen as necessary 
	pop 	bx 
	pop 	ax 

	mov 	cl ,1
	call 	ReadSector
	pop 	ax 
	call 	GetFATEntry
	cmp 	ax , 0FFFh
	jz 		LABEL_FILE_LOADED 
	push 	ax 
	mov 	dx , RootDirSectors
	add 	ax , dx 
	add 	ax , DeltaSectorNo
	add 	bx , [BPB_BytesPerSec]
	jmp 	LABEL_GOON_LOADING_FILE
LABEL_FILE_LOADED:

	mov		dh, 1			; "Ready."
	call	DispStr			; 显示字符串

	jmp BaseOfLoader:OffsetOfLoader



	;JMP $

;--------------------------------------------------
;INT 10h, INT 10H or INT 16 is shorthand for BIOS interrupt call 10hex,
;the 17th interrupt vector in an x86-based computer system. The BIOS 
;typically sets up a real mode interrupt handler at this vector that 
;provides video services. Such services include setting the video mode,
; character and string output,and graphics primitives (reading and writing pixels in graphics mode).
DispStr:
	mov 	ax , MessageLength
	mul 	dh
	add 	ax , BootMessage
	mov 	bp , ax 
	mov 	ax , ds
	mov 	es , ax     		;es:bp is string addr
	mov 	cx , MessageLength
	mov 	ax , 01301h
	mov 	bx , 0007h
	mov 	dl , 0
	int 	10h
	ret 
;----------------------------------------------------

BaseOfLoader		equ	09000h	; LOADER.BIN 被加载到的位置 ----  段地址
OffsetOfLoader		equ	0100h	; LOADER.BIN 被加载到的位置 ---- 偏移地址
wRootDirSizeForLoop 	dw 	RootDirSectors

wSectorNo 		dw 	0
bOdd 			db 	0


;string 
LoaderFileName 		db 	"LOADER  BIN",0
MessageLength 		equ 	9
BootMessage: 		db 	"Booting  " ;?????????????????????????????????
Message1 			db 	"Ready.   "
Message2 			db 	"No LOADER"
;----------------------------------------------------
;2*80*18*512=1.44MB .2面，80个磁道，18个扇区
;扇区号/每磁道扇区数(18)的结果(此软盘对于同一柱面上的数据在两片上连续写)
;商Q，柱面号=Q>>1,磁头号=Q&1
;余数R -->起始扇区号=R+1

;输入:ax	扇区号,es:bx==>数据缓冲区,cl=要读的扇区数
ReadSector:
	push 	bp
	mov 	bp , sp
	sub 	esp ,2

	mov 	byte [bp-2] , cl 
	push 	bx
	mov 	bl ,[BPB_SecPerTrk]   	;每个磁道18个扇区
	div 	bl 						;ax为要读取的扇区号
	inc 	ah
	mov 	cl ,ah 					;余数R+1
	mov 	dh , al 
	shr 	al , 1
	mov 	ch , al 
	and 	dh ,1
	pop 	bx 
	
	mov 	dl , [BS_DrvNum]		;指定驱动器号，0是A

.GoOnReading:
;===================================================================
;ah=0,dl=驱动器号（0表示A）==>复位软驱
;一次中断读取al个扇区到es:bx处
;ah=02h,al=要读的扇区数，ch=柱面号，cl=起始扇区号，dh=磁头号，dl=驱动器号（0表示A）
;es:bx==>数据缓冲区
;===================================================================
	mov 	ah ,2 
	mov 	al , byte [bp-2]  		;读al个扇区,不需要重复，一次即读al各扇区
	int 	13h 					;将数据读到es:bx指向的地址处
	jc 		.GoOnReading 			;如果读取错误CF=1就不停的读，直到正确为止

	add 	esp , 2
	pop 	bp 

	ret  
;----------------------------------------------------------
;输入：ax当前目录对应的文件的起始簇号,也是FAT表中对应项的号,数据段簇号从2开始，
;输出：下一个文件的扇区号
GetFATEntry:     
	push 	es
	push 	bx 
	push 	ax 
	mov 	ax , BaseOfLoader
	sub  	ax , 0100h   	;baseOfLoader前面留4K存放FAT
	mov 	es , ax 
	pop 	ax 
	mov 	byte [bOdd] , 0
	mov 	bx , 3         
	mul 	bx 
	mov 	bx , 2 	  ;一个FAT项12bit
	div 	bx        ;ax×1.5得相对于FAT起始地址的字节偏移地址
	cmp 	dx , 0    ;ax是商，等于偏移地址或小于偏移地址4bit
	jz 		LABEL_EVEN
	mov 	byte [bOdd] ,1
LABEL_EVEN:
	xor 	dx , dx
	mov 	bx , [BPB_BytesPerSec]
	div 	bx  			;判断在那个扇区,相对于FAT的扇区号
	push 	dx 				; ax <- 商 (FATEntry 所在的扇区相对于 FAT 的扇区号)
	mov 	bx , 0 			; dx <- 余数 (FATEntry 在扇区内的偏移)。
	add 	ax , SectorNoOfFAT1
	mov 	cl , 2
	call 	ReadSector      ;连续读取两个扇区到es:bx处,(BaseOfLoader-0100h):0

	pop 	dx 
	add 	bx , dx 
	mov 	ax , [es:bx]   
	cmp 	byte [bOdd] , 1
	jnz 	LABEL_EVEN_2
	shr 	ax , 4
LABEL_EVEN_2:
	and	ax, 0FFFh 
LABEL_GET_FAT_ENRY_OK:
	pop 	bx 
	pop 	es 
	ret 



times 	510-($-$$)	db	0	; 填充剩下的空间，使生成的二进制代码恰好为512字节
dw 	0xaa55					; 结束标志