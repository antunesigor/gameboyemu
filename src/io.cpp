#include "io.h"
#include "common.h"
#include "dma.h"
#include "timer.h"
#include "ppu.h"
#include "interrupt.h"
#include "joypad.h"

static char serial_data[2];

u8 io_read(u16 address){
    if(address == 0x00)
        return joypad_read();
    if(address == 0x01)
        return serial_data[0];
    if(address == 0x02)
        return serial_data[1];
    if(address >= 0x04 && address <= 0x07)
        return timer_read(address);
    if(address == 0x0F)
        return interrupt_read(address);    
    if(address >= 0x40 && address <= 0x45)
        return ppu_read(address);
    if(address >= 0x4A && address <= 0x4B)
        return ppu_read(address);
    else{
        printf("IO ports not implemented: %4.4x\n", address + 0xFF00);
        return 0;
    }
}

void io_write(u16 address, u8 value){
    if(address == 0x00){
        joypad_write(value);
        return;
    }
    if(address == 0x01){
        serial_data[0] = value;
        return;
    }
    if(address == 0x02){
        serial_data[1] = value;
        return;
    }
    if(address >= 0x04 && address <= 0x07){
        timer_write(address,value);
        return;
    }
    if(address == 0x0F){
        interrupt_write(address,value);   
        return; 
    }
    if(address >= 0x40 && address <= 0x45){
        ppu_write(address,value);
        return;
    }
    if(address == 0x46){
        dma_start(value);
        return;
    }
    if(address >= 0x47 && address <= 0x4B){
         ppu_write(address,value);
        return;
    }
    
    
    //printf("IO ports not implemented: %4.4x\n", address + 0xFF00);
}
