#include "dma.h"
#include "bus.h"

static dma_state state;

void dma_start(u8 address_high){
    state.address = ((u16)address_high) << 8;
    state.pos = 0;
}

void dma_tick(){
    if(state.address > 0x0){
        u8 val = bus_read(state.address + state.pos);
        bus_write(0xFE00 + state.pos,val);
        
        if(state.pos++ == 0x9F){
            //end of transfer
            state.address = 0x0;
        }
    }
}