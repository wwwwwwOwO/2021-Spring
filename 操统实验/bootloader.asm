%include "boot.inc"

[bits 16]

;空描述符
mov dword [GDT_START_ADDRESS+0x00],0x00b
mov dword [GDT_START_ADDRESS+0x04],0x00  

;创建描述符，这是一个数据段，对应0~4GB的线性地址空间
mov dword [GDT_START_ADDRESS+0x08],0x0000ffff    ; 基地址为0，段界限为0xFFFFF
mov dword [GDT_START_ADDRESS+0x0c],0x00cf9200    ; 粒度为4KB，存储器段描述符 

;建立保护模式下的堆栈段描述符      
mov dword [GDT_START_ADDRESS+0x10],0x00000000    ; 基地址为0x00000000，界限0x0 
mov dword [GDT_START_ADDRESS+0x14],0x00409600    ; 粒度为1个字节

;建立保护模式下的显存描述符   
mov dword [GDT_START_ADDRESS+0x18],0x80007fff    ; 基地址为0x000B8000，界限0x07FFF 
mov dword [GDT_START_ADDRESS+0x1c],0x0040920b    ; 粒度为字节

;创建保护模式下平坦模式代码段描述符
mov dword [GDT_START_ADDRESS+0x20],0x0000ffff    ; 基地址为0，段界限为0xFFFFF
mov dword [GDT_START_ADDRESS+0x24],0x00cf9800    ; 粒度为4kb，代码段描述符 

;初始化描述符表寄存器GDTR
mov word [pgdt], 39      ;描述符表的界限   
lgdt [pgdt]
      
in al,0x92                         ;南桥芯片内的端口 
or al,0000_0010B
out 0x92,al                        ;打开A20

cli                                ;中断机制尚未工作
mov eax,cr0
or eax,1
mov cr0,eax                        ;设置PE位
      
;以下进入保护模式
jmp dword CODE_SELECTOR:protect_mode_begin

;16位的描述符选择子：32位偏移
;清流水线并串行化处理器
[bits 32]           
protect_mode_begin:                              

mov eax, DATA_SELECTOR                     ;加载数据段(0..4GB)选择子
mov ds, eax
mov es, eax
mov eax, STACK_SELECTOR
mov ss, eax
mov eax, VIDEO_SELECTOR
mov gs, eax

assigment3:
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



count db 0x03

pgdt dw 0
     dd GDT_START_ADDRESS

bootloader_tag db 'run bootloader'
bootloader_tag_end:

protect_mode_tag db 'enter protect mode'
protect_mode_tag_end:
times 512*5 - ($ - $$) db 0