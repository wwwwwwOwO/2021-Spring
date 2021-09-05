[bits 32]

global asm_hello_world
global asm_lidt
global asm_unhandled_interrupt
global asm_halt

ASM_IDTR dw 0
         dd 0
count db 0x03
; void asm_unhandled_interrupt()
asm_unhandled_interrupt:
pushad
xor bx,bx
xor ecx,ecx
xor dx,dx
mov dx, 0x0200
mov bx, 0x0101
; dh 行 dl 列 
; bh 行方向 bl 列方向 1=increase 0=decrease
bounce:
    mov ax,0
    
;cursor 1:position
    mov al, dh
	imul cx, ax, 80
    mov al, dl
	add cx, ax
    shl cx, 1
    mov esi, ecx
    call print_a_char

; ;cursor 2:position which is symatrical to the other
    mov al, dh
    sub al, 24
    neg al
	imul cx, ax, 80
    mov al, dl
    sub al, 79
    neg al
	add cx, ax
    shl cx, 1
    mov esi, ecx
    call print_a_char


;always display
    mov ah, 0x0d
    mov al, 'w'
    mov [gs:2 * 33], ax
    mov al, 'e'
    mov [gs:2 * 34], ax
    mov al, 'n'
    mov [gs:2 * 35], ax
    mov al, 'n'
    mov [gs:2 * 36], ax
    mov al, 'y'
    mov [gs:2 * 37], ax

    mov al, 0
    mov [gs:2 * 38], ax
    mov al, '1'
    mov [gs:2 * 39], ax
    mov al, '9'
    mov [gs:2 * 40], ax
    mov al, '3'
    mov [gs:2 * 41], ax
    mov al, '3'
    mov [gs:2 * 42], ax
    mov al, '5'
    mov [gs:2 * 43], ax
    mov al, '0'
    mov [gs:2 * 44], ax
    mov al, '7'
    mov [gs:2 * 45], ax
    mov al, '4'
    mov [gs:2 * 46], ax

;change position depending on the direction stored in bx
;if inc
	add dh, bh
	add dl, bl	
;if dec
	mov ch, bh
	cmp ch, 0	
	jne L1          ;if equal, decrease dh by 1, or else just skip it
    call dec_dh
L1:	
    mov ch, bl
	cmp ch, 0	
	jne L2
    call dec_dl	    ;if equal, decrease dl by 1
L2: 

	call getDir     ;update the direction

	call delay      
    
	jmp bounce      ;loop

dec_dh:
	dec dh
	ret
dec_dl:
	dec dl
	ret
getDir:
;if the cursor meets the up and down side, flip dl
;if the cursor meets the left and right side, flip dh
	mov ch, dh
	cmp ch, 0	
	jne L3
    call flip_bh
L3: 
    mov ch, dh
	cmp ch, 24
	jne L4
    call flip_bh
L4:
	mov ch, dl
	cmp ch, 0	
	jne L5
    call flip_bl
L5: 
    mov ch, dl
	cmp ch, 79
	jne getDir_end
    call flip_bl
getDir_end:
    ret


flip_bl:
	xor bl,1
	ret
flip_bh:
	xor bh,1
	ret
print_a_char:
    mov ch, [count]
    call inc_count  ;increase to change the color
	mov cl, 'w'
	mov word [ gs : esi ], cx
    ret
delay:
	pushad
    mov ecx, 0x00070000
delay_s: 
	
    loop delay_s
	popad
    
	ret
inc_count:
    mov al, [count]
    inc al
    mov [count], al
    ret


; void asm_lidt(uint32 start, uint16 limit)
asm_lidt:
    push ebp
    mov ebp, esp
    push eax

    mov eax, [ebp + 4 * 3]
    mov [ASM_IDTR], ax
    mov eax, [ebp + 4 * 2]
    mov [ASM_IDTR + 2], eax
    lidt [ASM_IDTR]

    pop eax
    pop ebp
    ret

asm_hello_world:
    push eax
    xor eax, eax

    mov ah, 0x03 ;青色
    mov al, 'H'
    mov [gs:2 * 0], ax

    mov al, 'e'
    mov [gs:2 * 1], ax

    mov al, 'l'
    mov [gs:2 * 2], ax

    mov al, 'l'
    mov [gs:2 * 3], ax

    mov al, 'o'
    mov [gs:2 * 4], ax

    mov al, ' '
    mov [gs:2 * 5], ax

    mov al, 'W'
    mov [gs:2 * 6], ax

    mov al, 'o'
    mov [gs:2 * 7], ax

    mov al, 'r'
    mov [gs:2 * 8], ax

    mov al, 'l'
    mov [gs:2 * 9], ax

    mov al, 'd'
    mov [gs:2 * 10], ax

    pop eax
    ret

asm_halt:
    jmp $