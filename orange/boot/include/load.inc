BaseOfLoader 	equ  	09000h  ;base of loader.bin
OffsetOfLoader 	equ 	0100h 	;offset of loader.bin

BaseOfLoaderPhyAddr 	equ 	BaseOfLoader*10h    ;loader.bin 被加载到的物理位置
BaseOfKernelFile 		equ 	08000h
OffsetOfKernelFile		equ 	0h 
BaseOfKernelPhyAddr 	equ 	BaseOfKernelFile*10h 
;-------------------------------------------------------------
StackSpace 		times 1024 	db 	0 
TopOfStack 				equ 	BaseOfLoaderPhyAddr + $
