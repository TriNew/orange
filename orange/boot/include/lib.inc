;lib.inc

;--------------------------------------------------------
;display a char
DispAL:  
    push    ecx  
    push    edx  
    push	edi

    mov	edi, [dwDispPos]

    mov ah, 0Ch         ; 0000: 黑底    1100: 红字  
    mov dl, al  
    shr al, 4  
    mov ecx, 2  
.begin:  
    and al, 01111b  
    cmp al, 9  
    ja  .1  
    add al, '0'  
    jmp .2  
.1:  
    sub al, 0Ah  
    add al, 'A'  
.2:  
    mov [gs:edi], ax  ;一个字节打印时是两个16进制数表示，一个数是一个字符占一个字节，共两个字节
    add edi, 2  
  
    mov al, dl  
    loop    .begin  
    ;add edi, 2  
  	mov	[dwDispPos], edi

  	pop edi
    pop edx  
    pop ecx  
  
    ret  
 ;end dispAL






;-------------------------------------------------------
;display integers
DispInt:
	

	mov eax ,[esp+4]
	shr eax,24
	call DispAL

	mov eax ,[esp+4]
	shr eax,16
	call DispAL

	mov eax ,[esp+4]
	shr eax,8
	call DispAL

	mov eax ,[esp+4]
	call DispAL

	mov ah ,07h
	mov al ,'h'
	push edi

	mov edi ,[dwDispPos]
	mov [gs:edi],ax
	add edi ,4
	mov [dwDispPos],edi
	pop edi 

	ret
;end dispInt

;------------------------------------------------------------
;show a string
DispStr:
	push 	ebp
	mov 	ebp, esp
	push 	ebx
	push 	esi
	push 	edi

	mov 	esi ,[ebp+8] ;pstring
	mov 	edi ,[dwDispPos]
	mov 	ah  ,0Fh
.1:	
	lodsb
	test 	al,al
	jz 	.2
	cmp 	al ,0Ah ;if al = 0A  print return
	jnz 	.3
	push 	eax
	mov 	eax,edi
	mov 	bl ,160
	div 	bl
	and 	eax,0FFh
	inc 	eax
	mov 	bl,160
	mul 	bl
	mov 	edi,eax
	pop		eax
	jmp 	.1
.3:
	mov 	[gs:edi],ax
	add 	edi,2
	jmp 	.1
.2:	
	mov 	[dwDispPos],edi

	pop 	edi
	pop 	esi
	pop 	ebx
	pop 	ebp
	ret
;end DispStr

;----------------------------------------------------------	
;return 
DispReturn:
	push 	szReturn
	call 	DispStr
	add 	esp ,4

	ret
;end DispReturn

; ------------------------------------------------------------------------
; 内存拷贝，仿 memcpy
; ------------------------------------------------------------------------
; void* MemCpy(void* es:pDest, void* ds:pSrc, int iSize);
; ------------------------------------------------------------------------
MemCpy:
	push 	ebp
	mov 	ebp , esp   ;skill
	
	push 	esi
	push 	edi 
	push 	ecx

	mov 	edi , [ebp+8] 	;destination
	mov 	esi , [ebp+12]	;Source
	mov 	ecx , [ebp+16]	;Counter
.1:
	cmp 	ecx ,0
	jz 		.2

	mov 	al , [ds:esi]
	inc 	esi
	mov 	byte [es:edi] ,al
	inc 	edi

	dec		ecx
	jmp 	.1

.2:
	mov 	eax , [ebp+8] ;返回值

	pop 	ecx
	pop 	edi
	pop 	esi
	mov 	esp , ebp
	pop 	ebp

	ret



