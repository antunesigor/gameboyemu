#include "bus.h"
#include "cartridge.h"
#include "ppu.h"
#include "ram.h"
#include "io.h"
#include "interrupt.h"
#include <stdio.h>
#include <stdlib.h>
#include "cpu.h"

void bus_dump(){    
    for(int i=0x00;i<0xFFFF;i+=2){
        if(i >= 0xA000 && i < 0xC000){
            continue;
        } 
        if(i >= 0xE000 && i < 0xFE00){
            continue;
        }   
        if(i >= 0xFEA0 && i < 0xFF00){
            continue;
        }   
        if(i >= 0xFF4C && i < 0xFF80){
            continue;
        } 
          if(i%64 == 0){
            printf("\n %4.4x: ",i);
        }
        else if(i%16 == 0){
            printf(" | ");
        }
        printf("%2.2X",bus_read(i));
        printf("%2.2X ",bus_read(i+1));     
    }
    printf("\n");
}

u8 bus_read(u16 address){

    if(address < 0x8000){
        //0x0000 - 0x3FFF: ROM Bank #0
        //0x4000 - 0x7FFF: ROM Bank #X (Switchable)
        return cartridge_read(address);
    }
    if(address < 0xA000){
        //Video RAM
        return vram_read(address - 0x8000);
    }
    if(address < 0xC000){
        //RAM bank (Switchable)
        printf("RAM switchable not implemented %4.4X\n",address);
        exit(1);
        return 0;
    }
    if(address < 0xE000){
        //Internal RAM
        return ram_read(address - 0xC000);
    }
    if(address < 0xFE00){
        //ECHO Internal RAM
        printf("ECHO Internal RAM not implemented: %4.4x\n", address);
        return 0;
    }
    if(address < 0xFEA0){
        //OAM: Sprite Attribute Memory
        return oam_read(address - 0xFE00);
    }
    if(address < 0xFF00){
        //Empty
        printf("R | Empty Address should never be accessed: %4.4x\n", address);
        return 0;
    }
    if(address < 0xFF4C){       
        //IO ports
        return io_read(address - 0xFF00);
    }
    if(address < 0xFF80){
        //Empty
        printf("R | Empty Address should never be accessed: %4.4x\n", address);    
        return 0;
    }
    if(address < 0xFFFF){
        return hram_read(address - 0xFF80);
    }
    if(address == 0xFFFF){
        return interrupt_read(address);
    }
    return 0;
}

void bus_write(u16 address, u8 value){

    if(address > 0xFFFF){
        cpu_printregs();
        bus_dump();
        exit(1);
    }
    if(address < 0x8000){
        //0x0000 - 0x3FFF: ROM Bank #0
        //0x4000 - 0x7FFF: ROM Bank #X (Switchable)
        cartridge_write(address, value);
        return;
    }
    if(address < 0xA000){
        //Video RAM
        vram_write(address - 0x8000,value);
        return;
    }
    if(address < 0xC000){
        //RAM bank (Switchable)
        printf("RAM switchable not implemented\n");
        return;
    }
    if(address < 0xE000){
        //Internal RAM
        ram_write(address - 0xC000,value);
        return;
    }
    if(address < 0xFE00){
        //ECHO Internal RAM
        printf("ECHO Internal RAM not implemented: %4.4x | ", address);
        exit(1);
        return;
    }
    if(address < 0xFEA0){
        //OAM: Sprite Attribute Memory
        oam_write(address - 0xFE00,value);
        return;
    }
    if(address < 0xFE00){
        //Empty
        printf("W |  Empty Address should never be accessed: %4.4x\n", address);
        return;
    }
    if(address < 0xFF4C){
        //IO ports
        io_write(address - 0xFF00,value);
        return;
    }
    if(address < 0xFF80){
        if(address == 0xFF7F)
            return;
        //Empty
        printf("W | Empty Address should never be accessed: %4.4x\n", address);  
        //bus_dump();
        //exit(1);  
        return;
    }
    if(address < 0xFFFF){
        hram_write(address - 0xFF80,value);
        return;
    }
    if(address == 0xFFFF || address == 0xFF0F){
        interrupt_write(address,value);
        return;
    }
    //bus_dump();
    //exit(0);
    return;
}

