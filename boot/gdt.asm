gdt_start:

gdt_null:
;null bytes
dd 0x00
dd 0x00

gdt_code:
; Limit low
dw 0xFFFF
; Base low
dw 0x00
; Base middle
db 0x00
; Access Byte
db 10011010b
; Flag + LImit high bits (4+4)
db 11001111b
; Base high
db 00

gdt_data:
dw 0xFFFF
dw 0x00
db 0x00
db 10010010b
db 11001111b
db 00

gdt_end:

gdt_descriptor:
dw gdt_end - gdt_start - 1
dd gdt_start 


CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start 


