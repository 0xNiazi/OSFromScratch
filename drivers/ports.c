#include "ports.h"

uint8_t inb(uint16_t port){
	uint8_t result;
	__asm__ __volatile__("in al,dx" : "=a"(result): "d"(port));
	return result;
}

void outb(uint16_t port,uint8_t data){
	__asm__ __volatile__("out dx,al" : : "a"(data), "d"(port));
}
