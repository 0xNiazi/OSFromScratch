[bits 32]
global keyboard_handler_asm
extern keyboard_handler_c

global timer_handler_asm
extern timer_handler_c

keyboard_handler_asm:
pusha
call keyboard_handler_c
popa
iret

timer_handler_asm:
pusha
call timer_handler_c
popa
iret

