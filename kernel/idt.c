#include "idt.h"
#include "ports.h"

static idt_entry_t idt[256];
static idt_ptr_t idt_ptr;

void idt_set_gate(uint8_t num,uint32_t base,uint16_t sel,uint8_t flags){
    idt[num].offset_low=base&0xFFFF;
    idt[num].offset_high=(base>>16)&0xFFFF;
    idt[num].selector = sel;
    idt[num].type_attr = flags;
    idt[num].zero=0;
} 

static void pic_remap(void){
    //ICW1: Initializing the Master PIC && Slave PIC
    outb(0x20,0x11);
    outb(0xA0,0x11);
    
    //ICW2: Vector offsets ( where irqs will map)
    outb(0x21,0x20);
    outb(0xA1,0x28);
    
    //ICW3: Wiring/Cascading
    outb(0x21,0x04); // Tell Master that Slave is at IRQ 2 (00000100b = 4)
    outb(0xA1,0x02); // Tell Slave its cascade identity (2)
    
    //ICW4: Env info(8086/88 mode)
    outb(0x21,0x01);
    outb(0xA1,0x01);

    //Mask all IRQs (0xFF = all disabled)
    outb(0x21,0xFF);
    outb(0xA1,0xFF);
}

static inline void idt_load(void) {
        __asm__ __volatile__("lidt [eax]" : : "a"(&idt_ptr));
}

void idt_init(){
    for(uint8_t i=0;i<255;i++){
        idt_set_gate(i,0,0,0);   
    }
    // Not wasting 8 bits
    idt_set_gate(255,0,0,0);
    idt_ptr.limit = sizeof(idt) -1;
    idt_ptr.base = (uint32_t)&idt;
    pic_remap();
    idt_load();
}
