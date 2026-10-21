#include "../include/types.h"

void kernel_main(void){

volatile uint8_t *video_memory = (volatile uint8_t*)0xB8000;

video_memory[0] = 'X';
video_memory[1] = 0x0A;

while (1) {

}

}
