#include "types.h"
#include "vga.h"
#include "idt.h"
#include "keyboard.h"
#include "timer.h"

void kernel_main(void){

    idt_init();
    keyboard_init();
    timer_init(100);
    vga_clear(VGA_COLOR(VGA_COLOR_WHITE,VGA_COLOR_BLACK));
    vga_print("======= 0xNIAZI  OS v0.1 ==\n",VGA_COLOR(VGA_COLOR_LIGHT_CYAN,VGA_COLOR_BLACK));
    vga_print("[SYSTEM] Kernel loaded at 0x1000\n", VGA_COLOR(VGA_COLOR_LIGHT_GREEN,VGA_COLOR_BLACK));
    vga_print("Timer & Keyboard Active! Sleeping 2 seconds...\n",VGA_COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    timer_sleep(200); // Sleep for 200 ticks = 2 full seconds!
    vga_print("Woke up! Type freely below:\n> ", VGA_COLOR(VGA_COLOR_LIGHT_GREEN,VGA_COLOR_BLACK));
    while (1) {
        __asm__ __volatile__("hlt");
    }

}
