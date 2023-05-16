#include "ram.h"

ram_state ram;

u8 ram_read(u16 address){
    //printf("READING RAM %4.4X: %2.2X\n",address,ram.internal_ram[address]);
    return ram.internal_ram[address];
}

void ram_write(u16 address, u8 value){
    ram.internal_ram[address] = value;
}

u8 hram_read(u16 address){
    return ram.high_ram[address];
}
void hram_write(u16 address, u8 value){
    ram.high_ram[address] = value;
}