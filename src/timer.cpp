#include "timer.h"
#include "interrupt.h"
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static timer_state state;

void timer_init(){
    state.enabled = false;
}

void timer_tick(){

    /*
           00: CPU Clock / 1024
           01: CPU Clock / 16
           10: CPU Clock / 64
           11: CPU Clock / 256  << DIV IS ALWAYS HERE
    */

    if(state.div_delay++ >= 256){
        state.div++;    
    //    printf("div %d\n",state.div-1);
        state.div_delay = 0;
    }
    if(state.enabled && state.tima_delay++ >= state.frequency){
        state.tima++;
        if(state.tima == 0x00){
            interrupt_request(0b100);
        }
        state.tima_delay = 0;   
    }
        
   
}
u8 timer_read(u16 address){
    if(address == 0x04 || address == 0xFF04)
        return state.div;
    if(address == 0x05 || address == 0xFF05)
        return state.tima;
    if(address == 0x06 || address == 0xFF06)
        return state.tma;
    if(address == 0x07 || address == 0xFF07){
        int frequency_bit;
        switch(state.frequency){
            case 1024:
                frequency_bit = 0;
                break;
            case 16:
                frequency_bit = 1;
                break;
            case 64:
                frequency_bit = 2;
                break;
            default:
                frequency_bit = 3;
                break;
        }
        return state.enabled << 2 | frequency_bit;
  
    }
    return 0;
}

void timer_write(u16 address, u8 value){
    if(address == 0x04 || address == 0xFF04){
        state.div = 0x0;
        return;
    }
    if(address == 0x05 || address == 0xFF05){
        state.tima = value; //???
        return;    
    }
    if(address == 0x06 || address == 0xFF06){
        state.tma = value;
        return;
    }
    if(address == 0x07 || address == 0xFF07){
        state.enabled = BIT(value,2);
        switch(value & 0b11){
            case 0:
                state.frequency = 1024;
                break;
            case 1:
                state.frequency = 16;
                break;
            case 2:
                state.frequency = 64;
                break;
            default:
                state.frequency = 256;
                break;
        }

    }
}
