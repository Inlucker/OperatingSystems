.586P	

descr	struc
	lim		dw	0
	base_l	dw	0
	base_m	db	0
	attr_1	db	0
	attr_2	db	0
	base_h	db	0
descr	ends

data	segment	use16

	gdt_null descr <0,0,0,0,0,0>
	gdt_data descr <data_size-1,0,0,92h,0,0>
	gdt_code descr <code_size-1,0,0,98h,0,0>
	gdt_stack descr <255,0,0,92h,0,0>
	gdt_screen descr <3999,8000h,0Bh,92h,0,0>
	gdt_size=$-gdt_null

	pdescr	df	0
	sym		db	1
	attr	db	1Eh
	msg db 27,'[31;42m Back to real mode! ',27,'[0m$'

	msg_real_mode db 'real mode', 13, 10, '$'
	msg_real_back db 'back real mode', 13, 10, '$'
	msg_prot_mode db 'protected mode'
	data_size=$-gdt_null
data ends

text	segment use16
		assume	CS:text,DS:data
main	proc
		xor		EAX,EAX
		mov		AX,data
		mov		DS,AX
		
		shl		EAX,4
		mov		EBP,EAX
		mov 	BX,offset gdt_data
		mov 	[BX].base_l,AX
		shr 	EAX,16
		mov 	[BX].base_m,AL
		
		xor		EAX,EAX
		mov 	AX,CS
		shl		EAX,4
		mov 	BX,offset gdt_code
		mov 	[BX].base_l,AX
		shr 	EAX,16
		mov 	[BX].base_m,AL
		
		xor		EAX,EAX
		mov		AX,SS
		shl		EAX,4
		mov		BX,offset gdt_stack
		mov		[BX].base_l,AX
		shr		EAX,16
		mov		[BX].base_m,AL
		
		mov		dword ptr pdescr+2,EBP;
		mov		word ptr pdescr,gdt_size-1	
		lgdt	pdescr
		
	;mov ah, 09h
	;mov dx, offset msg_real_mode
	;int 21h
	
		;mov		AX,40h
		;mov		ES,AX
		;mov		word ptr ES:[67h],offset return
		;mov		ES:[69h],CS
		;mov		AL,0Fh
		;out		70h,AL
		;mov		AL,0Ah
		;out		71h,AL	
		
		cli
		
		mov		EAX,CR0
		or		EAX,1
		mov		CR0,EAX	

		db 0EAh				
		dw offset continue	
		dw 16				
continue:					
		mov		AX,8
		mov		DS,AX
		
		mov		AX,24
		mov		SS,AX
		
		mov		AX,32
		mov		ES,AX
		
		mov		DI,1920
		mov		CX,80
		mov 	AX,word ptr sym
scrn:	stosw
		inc		AL
		loop 	scrn
 
	;mov gdt_data.lim, 0FFFFh
    ;mov gdt_code.lim, 0FFFFh
    ;mov gdt_stack.lim, 0FFFFh
    ;mov gdt_screen.lim, 0FFFFh

    push ds
    pop  ds 
    push es
    pop  es
    push ss
    pop  ss

	db 0EAh
	dw offset go
	dw 16

go: 
	mov eax, CR0
	and eax, 0FFFFFFFEh
	mov CR0, eax
	db 0EAh
	dw offset return
	dw text
	
return:
		mov	AX,data
		mov DS,AX
		mov AX,stk
		mov SS,AX
		mov	SP,256
		sti

	;mov ah, 09h
	;mov dx, offset msg_real_back
	;int 21h
	
		mov AH,09h
		mov DX,offset msg
		int 21h
		
		mov AX,4C00h
		int 21h	
main	endp
code_size=$-main
text	ends

stk		segment stack use16
		db 256 dup ('^')
stk		ends
		end main