#include "../include/types.h"
#include "../include/vga.h"

void kernel_main(void){

    vga_clear(VGA_COLOR(VGA_COLOR_WHITE,VGA_COLOR_BLACK));
    vga_print("======= 0xNIAZI  OS v0.1 ==\n",VGA_COLOR(VGA_COLOR_LIGHT_CYAN,VGA_COLOR_BLACK));
    vga_print("[SYSTEM] Kernel loaded at 0x1000\n", VGA_COLOR(VGA_COLOR_LIGHT_GREEN,VGA_COLOR_BLACK));
    vga_draw_block(10, 10, VGA_COLOR_MAGENTA);
    vga_draw_block(11, 10, VGA_COLOR_MAGENTA);
    vga_draw_block(12, 10, VGA_COLOR_MAGENTA);
    vga_draw_block(11, 11, VGA_COLOR_MAGENTA);
    while (1) {

    }

}
