bits 16
org 0x7c00

jmp 0x0000:start

start:
xor ax, ax
mov ds, ax
mov es, ax
mov ss, ax
mov sp,0x7c00


; Moving to 32 bit mode from 16 bit mode
cli

lgdt [gdt_descriptor]

mov eax, cr0
or eax, 0x01
mov cr0,eax

jmp CODE_SEG:init_pm

[bits 32]
init_pm:

mov ax , DATA_SEG
mov ds,ax
mov ss,ax
mov es,ax
mov fs,ax
mov gs,ax

mov ebp, 0x90000
mov esp,ebp

; printing text
mov edx,0xb8000
mov esi,msg
mov byte al,[esi]
.loop:
mov byte [edx],al
mov byte [edx+1],0x0f
inc si
mov byte al,[esi]
add edx,2
cmp al,0
jnz .loop

cli
.hang:
    hlt
    jmp .hang

msg db "Learning OS Devlopment ",0
%include "boot/gdt.asm"
times 510-($-$$) db 0

dw 0xaa55
