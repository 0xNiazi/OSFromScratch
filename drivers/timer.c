#include "types.h"
#include "ports.h"
#include "idt.h"


static volatile uint32_t timer_ticks =0;

void timer_handler_c(void){
    timer_ticks++;
    outb(0x20,0x20);
}

uint32_t timer_get_ticks(void){
    return timer_ticks;
}

void timer_sleep(uint32_t ticks){
    uint32_t target = timer_ticks + ticks;
    while(target>timer_ticks){
        __asm__ __volatile__("hlt");
    }
}

void timer_init(uint32_t frequency ){
    extern void timer_handler_asm(void);
    idt_set_gate(0x20,(uint32_t)timer_handler_asm,0x08,0x8E);

    uint16_t divisor = (uint16_t) (1193182 / frequency );
    outb(0x43,0x36);
    outb(0x40,(uint8_t)(divisor&0xFF));
    outb(0x40,(uint8_t)((divisor>>8)&0xFF));

    outb(0x21,0xFC);
}
