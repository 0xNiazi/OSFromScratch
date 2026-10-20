
bits 16
org 0x7c00


mov si, msg
mov ah,0x0e
mov al,[si]
.loop:
int 0x10
inc si
mov al,[si]
cmp al,0
jnz .loop


jmp $

msg db "Learning OS Devlopment ",0
times 510-($-$$) db 0

dw 0xaa55
