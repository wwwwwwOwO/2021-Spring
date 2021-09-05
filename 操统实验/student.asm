; If you meet compile error, try 'sudo apt install gcc-multilib g++-multilib' first

%include "head.include"
; you code here
	; call your_if
	; call your_while
	; ret
your_if:
	mov eax, [a1]
	cmp eax,12
	jl c1
	mov eax, [a1]
	cmp eax,24
	jl c2
	jmp c3
c1:
	mov eax, [a1]
	xor edx, edx
	mov ebx, 2
	idiv ebx
	add eax, 1
	mov [if_flag], eax
	jmp end_if
c2:
	mov eax, [a1]
	sub eax, 24
	neg eax
	mov ebx, [a1]
	imul eax, ebx
	mov [if_flag], eax
	jmp end_if
c3:
	mov eax, [a1]
	shl eax, 4
	mov [if_flag], eax
	jmp end_if
end_if:
	; ret
your_while:
	mov edx, [a2]
	cmp edx, 12
	jl end_while
	call my_random
	mov ebx, [while_flag]
	mov ecx, [a2]
	mov byte[ebx+(ecx-12)*1], al
	mov edx, [a2]
	dec edx
	mov [a2], edx
	jmp your_while
end_while:
	; ret

%include "end.include"

your_function:
	mov eax,0	;i
my_loop:
	mov ebx, [your_string]
	mov byte ecx, [eax*1+ebx]
	cmp ecx, 0
	je end_my_function

	pushad
	mov ebx, [your_string]
	mov byte ecx, [eax*1+ebx]
	push ecx
	call print_a_char
	pop ecx
	popad

	inc eax
	jmp my_loop
end_my_function:
	; jmp $
	ret
	
