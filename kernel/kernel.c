#include "../include/types.h"

void kernel_main(void){

volatile uint8_t *video_memory = (volatile uint8_t*)0xB8000;

volatile uint8_t msg [15] = "OS DEVELOPMENT";

for ( int8_t i =0;i<15;i++){
video_memory[2*i] = msg[i] ;
video_memory[(2*i)+1] = 0x0A;}

while (1) {

}

}
