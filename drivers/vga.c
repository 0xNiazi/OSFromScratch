#include "vga.h"
#include "ports.h"

//Here we are specifying the VGA memory location (0xB8000)
static volatile uint8_t *video_memory = (volatile uint8_t *) 0xB8000;
static int cursor_x = 0;
static int cursor_y = 0;


int vga_get_x(void){
    return cursor_x;
} 
int vga_get_y(void){
    return cursor_y;
} 


uint16_t vga_pos_calc(uint8_t x,uint8_t y){
    //calculating the cursor pos
    uint16_t pos = y * VGA_WIDTH +x;
    return pos;
}

void vga_set_cursor(int x, int y){
    
    uint16_t pos = vga_pos_calc(x,y);
    //Higher byte of the pos 
    //0xE means we want to send the upper bytes
    outb(0x3D4,0xE);
    outb(0x3D5,(uint8_t)((pos>>8)&0xFF));

    //Lower byte of the pos 
    //0xF means we want to send the lower bytes
    outb(0x3D4,0xF);
    outb(0x3D5,(uint8_t)pos&0xFF);
    
    cursor_x=x;
    cursor_y=y;
}

void vga_clear(uint8_t color){
    for(uint16_t i =0;i<2000;i++){
        video_memory[i*2]=' ';
        video_memory[(i*2)+1]=color;
    }
        vga_set_cursor(0,0);
}

void vga_put_char(char c,uint8_t color){
    if (c!='\n'){
        // we are doing this as vga memory list is like this:
        // 0[char,color],1[char,color]....
        // so even index = char & odd index = color
        uint16_t offset = 2 * vga_pos_calc(cursor_x,cursor_y) ;
        video_memory[offset]=c;
        video_memory[offset+1]=color;
        cursor_x++;
        }
    else{
        cursor_y++;
        cursor_x=0;
        }
    if(cursor_x>VGA_WIDTH){cursor_x=0;cursor_y++; if(cursor_y>VGA_HEIGHT){cursor_x=0;cursor_y=0;}}
    vga_set_cursor(cursor_x,cursor_y);
}

void vga_print(const char* str,uint8_t color){
    while(*str !='\0'){
        vga_put_char(*str,color);
        str++;
    }

}

void vga_draw_block(int x, int y,uint8_t color){
   uint16_t offset = 2 * vga_pos_calc(x,y); 
   video_memory[offset]=' ';
   video_memory[offset+1]=(color <<4);
}
