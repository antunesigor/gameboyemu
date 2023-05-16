#include "interrupt.h"
#include "cpu.h"

interrupt_state interrupt;

void interrupt_init(){
    interrupt.ime = true;
}

void enable_interrupt(){
    interrupt.ime = true;
}

void disable_interrupt(){
    interrupt.ime = false;
}

u8 interrupt_read(u16 address){
    interrupt_stat selected;
    if(address == 0x0F || address == 0xFF0F){
        selected = interrupt.request_stat;        
    }
    else if(address == 0xFF || address == 0xFFFF)
        selected = interrupt.enable_stat;
    else {
        printf("error reading interrupt");
        return 0;
    }

    return selected.joypad << 4 | selected.serial << 3 | selected.timer << 2 | selected.lcd << 1 | selected.vblank;
}

void interrupt_write(u16 address, u8 value){
    if(address == 0x0F || address == 0xFF0F)
        interrupt_request(value);
    else if(address == 0xFF || address == 0xFFFF)
        interrupt_enable(value);
    else 
        printf("error writing interrupt");
}

void interrupt_check(){
    if(interrupt.ime){
        if(interrupt.enable_stat.vblank && interrupt.request_stat.vblank){
    //        printf("vblank to be executed");
            cpu_interrupt(0x40);
            interrupt.request_stat.vblank = false;
            return;
        }
        if(interrupt.enable_stat.lcd && interrupt.request_stat.lcd){
            //printf("lcd to be executed\n");
            cpu_interrupt(0x48);
            interrupt.request_stat.lcd = false;
            return;
        }
        if(interrupt.enable_stat.timer && interrupt.request_stat.timer){
            cpu_interrupt(0x50);
            interrupt.request_stat.timer = false;
            return;
        }
        if(interrupt.enable_stat.serial && interrupt.request_stat.serial){
            cpu_interrupt(0x58);
            interrupt.request_stat.serial = false;
            return;
        }
        if(interrupt.enable_stat.joypad && interrupt.request_stat.joypad){
    //        printf("joypad to be executed");
            cpu_interrupt(0x60);
            interrupt.request_stat.joypad = false;
            return;
        }
    }
}

/*
Bit 0: VBlank   Interrupt Enable  (INT $40)  (1=Enable)
Bit 1: LCD STAT Interrupt Enable  (INT $48)  (1=Enable)
Bit 2: Timer    Interrupt Enable  (INT $50)  (1=Enable)
Bit 3: Serial   Interrupt Enable  (INT $58)  (1=Enable)
Bit 4: Joypad   Interrupt Enable  (INT $60)  (1=Enable)
*/

void interrupt_request(u8 source){
    cpu_disable_halt();
    interrupt.request_stat.vblank = source & 0b1;
    interrupt.request_stat.lcd = source & 0b10;
    interrupt.request_stat.timer = source & 0b100;
    interrupt.request_stat.serial = source & 0b1000;
    interrupt.request_stat.joypad = source & 0b10000;  
    if(interrupt.request_stat.joypad){
        printf("joypad interrupt requested!\n");
    }
}
void interrupt_enable(u8 source){
    interrupt.enable_stat.vblank = source & 0b1;
    interrupt.enable_stat.lcd = source & 0b10;
    interrupt.enable_stat.timer = source & 0b100;
    interrupt.enable_stat.serial = source & 0b1000;
    interrupt.enable_stat.joypad = source & 0b10000;        
}