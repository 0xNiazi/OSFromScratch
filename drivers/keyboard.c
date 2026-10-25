#include "../include/types.h"
#include "../include/ports.h"
#include "../include/vga.h"
#include "../include/idt.h"


static const char scancode_ascii[128] = {
        0,   27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, /* Ctrl */
        'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
        0, /* Left Shift */
        '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
        0, /* Right Shift */
        '*',
        0, /* Alt */
        ' ' /* Space bar */
};

void keyboard_handler_c(void){
    uint8_t scancode = inb(0x60);
    if(scancode & 0x80){
        outb(0x20,0x20);
        return;
    }
    char key_char = scancode_ascii[scancode];
    if(key_char==0){outb(0x20,0x20);return;}
    if(key_char=='\b'){
        int cursor_x=vga_get_x();
        int cursor_y=vga_get_y();
        if(cursor_x==0 && cursor_y ==0){
            outb(0x20,0x20);
            return;
        }
        else if (cursor_x!=0){
            vga_set_cursor(cursor_x-1,cursor_y);
            vga_put_char(' ',VGA_COLOR(VGA_COLOR_BLACK,VGA_COLOR_BLACK));
            vga_set_cursor(cursor_x-1,cursor_y);
            outb(0x20,0x20);
            return;
        }
        else{
            vga_set_cursor(VGA_WIDTH,cursor_y-1);
            vga_put_char(' ',VGA_COLOR(VGA_COLOR_BLACK,VGA_COLOR_BLACK));
            vga_set_cursor(VGA_WIDTH,cursor_y-1);
            outb(0x20,0x20);
            return;
            
        }


    }
    vga_put_char(key_char,VGA_COLOR(VGA_COLOR_YELLOW,VGA_COLOR_BLACK));
    outb(0x20,0x20);
}

void keyboard_init(void){
    extern void keyboard_handler_asm(void);
    idt_set_gate(0x21,(uint32_t)keyboard_handler_asm,0x08,0x8E);
    outb(0x21,0b11111101);
    __asm__ __volatile__("sti ");
}
