#ifndef VGA_H
#define VGA_H

#include "types.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

enum COLORS {

VGA_COLOR_BLACK,VGA_COLOR_BLUE,VGA_COLOR_GREEN,VGA_COLOR_CYAN,VGA_COLOR_RED,VGA_COLOR_MAGENTA,VGA_COLOR_BROWN,VGA_COLOR_LIGHT_GREY,VGA_COLOR_DARK_GREY,VGA_COLOR_LIGHT_BLUE,VGA_COLOR_LIGHT_GREEN,VGA_COLOR_LIGHT_CYAN,VGA_COLOR_LIGHT_RED,VGA_COLOR_LIGHT_MAGENTA,VGA_COLOR_YELLOW,VGA_COLOR_WHITE

};

#define VGA_COLOR(fg,bg) ((fg) | (bg)<< 4)

void vga_clear(uint8_t color);
void vga_put_char(char c, uint8_t color);
void vga_print(const char *str, uint8_t color);
void vga_set_cursor(int x, int y);
void vga_draw_block(int x,int y,uint8_t color);

#endif

