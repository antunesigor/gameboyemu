#include "operations.h"
#include "cpu.h"

const int Z = 7;
const int N = 6;
const int H = 5;
const int C = 4;

void ld_r1_r2(u8* reg_dest, u8* reg_src){
    *reg_dest = *reg_src;
}

void ld_r_n(u8* reg_dest, u8 value){
    *reg_dest = value;
}

void ld_r_addr(u8* reg_dest, u16 address){
    *reg_dest = bus_read(address);
}

void ld_addr_r(u16 address, u8* reg_src){
    bus_write(address,*reg_src);
}

void ld_addr_n(u16 address, u8 value){
    bus_write(address,value);
}

void ld_2r_nn(u8* reg_dest_start, u16 value){
    reg_dest_start[0] = value >> 8;
    reg_dest_start[1] = value & 0xFF;
}

void add_2r_nn(u8* reg_dest_start, u16 value){
    u16 dest = reg_dest_start[0] << 8 | reg_dest_start[1];

    int partial = dest + value;

    reg_dest_start[0] = partial >> 8;
    reg_dest_start[1] = partial & 0xFF;

    cpu_setflag(false,N);

    cpu_setflag(partial > 0xFFFF,C);

    cpu_setflag(((dest & 0b0000011111111111) + (value & 0b0000011111111111)) > 0b0000011111111111,H);

    //printf("%4.4X + %4.4X = %4.4X| Z %d N %d H %d C %d\n",dest, value, (reg_dest_start[0] << 8 | reg_dest_start[1]), cpu_readflag(7), cpu_readflag(6), cpu_readflag(5), cpu_readflag(4));
}

void add_2r_sn(u8* reg_start, s8 value){
    u16 dest = reg_start[0] << 8 | reg_start[1];

    int partial = dest + value;

    reg_start[0] = partial >> 8;
    reg_start[1] = partial & 0xFF;
}

void add_r_n(u8* reg_dest, u8 value){
    int dest = *reg_dest;
    *reg_dest += value;
/*
Z - Set if result is zero.
N - Reset.
H - Set if carry from bit 3.
C - Set if carry from bit 7.
*/

    cpu_setflag(false,N);   
        cpu_setflag(((int)(dest & 0xF) + (int)(value & 0xF)) > 0xF,H);
        cpu_setflag(*reg_dest == 0,Z);
        cpu_setflag(value > *reg_dest,C);
}

void adc_r_n(u8* reg_dest, u8 value){
    int dest = *reg_dest;
    int carry = cpu_readflag(C);
    *reg_dest = dest + value + carry;
/*
Z - Set if result is zero.
N - Reset.
H - Set if carry from bit 3.
C - Set if carry from bit 7.
*/

    cpu_setflag(false,N);
    cpu_setflag((((int)(dest & 0xF) + (int)(value & 0xF)) + carry) > 0xF,H);
    cpu_setflag(*reg_dest == 0,Z);
    cpu_setflag((value + carry) > *reg_dest,C);
           
}

void sub_r_n(u8* reg_dest, u8 value){
/*
Z - Set if result is zero.
N - Set.
H - Set if no borrow from bit 4.
C - Set if no borrow.
*/

        cpu_setflag(*reg_dest < value,C);
        cpu_setflag((*reg_dest & 0xF) < (value & 0xF),H);

    *reg_dest = *reg_dest - value;

    cpu_setflag(true,N);

        cpu_setflag(*reg_dest == 0,Z);
    
}

void sbc_r_n(u8* reg_dest, u8 value){
/*
Z - Set if result is zero.
N - Set.
H - Set if no borrow from bit 4.
C - Set if no borrow.
*/
    int carry = cpu_readflag(C);

        cpu_setflag(*reg_dest < (value +carry),C);
        cpu_setflag((int)(*reg_dest & 0xF) < ((int)(value & 0xF)+carry),H);

    *reg_dest = *reg_dest - value - carry;

    cpu_setflag(true,N);

        cpu_setflag(*reg_dest == 0,Z);
}

void and_r_n(u8* reg_dest, u8 value){
    *reg_dest &= value;

        cpu_setflag(*reg_dest == 0,Z);
    cpu_setflag(false,N);
    cpu_setflag(true,H);
    cpu_setflag(false,C);
}

void or_r_n(u8* reg_dest, u8 value){
    *reg_dest |= value;

        cpu_setflag(*reg_dest == 0,Z);
    cpu_setflag(false,N);
    cpu_setflag(false,H);
    cpu_setflag(false,C);
}

void xor_r_n(u8* reg_dest, u8 value){
    //printf("XOR %2.2X AND %2.2X = %2.2X", *reg_dest, value, *reg_dest ^ value);
    *reg_dest ^= value;

        cpu_setflag(*reg_dest == 0,Z);
    cpu_setflag(false,N);
    cpu_setflag(false,H);
    cpu_setflag(false,C);
}

void cp_r_n(u8* reg_dest, u8 value){
/*
Z - Set if result is zero.
N - Set.
H - Set if no borrow from bit 4.
C - Set if no borrow.
*/

        cpu_setflag(*reg_dest < value,C);
        cpu_setflag((*reg_dest & 0xF) < (value & 0xF),H);

    cpu_setflag(true,N);

        cpu_setflag(*reg_dest == value,Z);

       // printf("COMPARE %2.2X AND %2.2X | RESULT FLAGS: Z %d | N %d | H %d | C %d\n",*reg_dest,value,cpu_readflag(7),cpu_readflag(6),cpu_readflag(5),cpu_readflag(4));
}

void inc_r(u8* reg){
       *reg += 1;
/*
Z - Set if result is zero.
N - Reset.
H - Set if carry from bit 3.
C - Set if carry from bit 7.
*/

    cpu_setflag(false,N);

    cpu_setflag((*reg & 0x0F) == 0x00,H);
        cpu_setflag(*reg == 0,Z);    
}

void dec_r(u8* reg){
       *reg -= 1;
    
    

    cpu_setflag(true,N);
    cpu_setflag((*reg & 0x0F) == 0xF,H);
    cpu_setflag(*reg == 0,Z);    

}

void inc_addr(u16 address){
       u8 val = bus_read(address);
       val += 1;
       bus_write(address,val);
/*
Z - Set if result is zero.
N - Reset.
H - Set if carry from bit 3.
C - Set if carry from bit 7.
*/

    cpu_setflag(false,N);
    cpu_setflag((val & 0x0F) == 0x00,H);

    cpu_setflag(val == 0,Z);
}

void dec_addr(u16 address){
       u8 val = bus_read(address);
       val -= 1;
       bus_write(address,val);
/*
Z - Set if result is zero.
N - Reset.
H - Set if carry from bit 3.
C - Set if carry from bit 7.
*/

    cpu_setflag(true,N);

        cpu_setflag((val & 0x0F) == 0x0F,H);
    
        cpu_setflag(val == 0,Z);

}

void rrc_r(u8* reg){
    int bit0 = *reg & 0x1;

    cpu_setflag(bit0 == 1, C);

    *reg= *reg >> 1;

    if(bit0 == 1){
        BIT_SET(*reg,7,1);
    }

    cpu_setflag(false,N);
    cpu_setflag(false,H);
    
        cpu_setflag(*reg == 0,Z);
    
}


void rr_r(u8* reg){
    int bit0 = *reg & 0x1;

    *reg= *reg >> 1;

    if(cpu_readflag(C) == 1){
        BIT_SET(*reg,7,1);
    }

    cpu_setflag(false,N);
    cpu_setflag(false,H);
    
    cpu_setflag(*reg == 0,Z);
    
    cpu_setflag(bit0,C);
    
}

void rlc_r(u8* reg){

    int bit = BIT(*reg,7);

    cpu_setflag(bit,C);
    
    *reg= *reg << 1;

    *reg |= bit;

    cpu_setflag(false,N);
    cpu_setflag(false,H);   
    cpu_setflag(*reg == 0,Z);
     
}

void rl_r(u8* reg){

    int bit = BIT(*reg,7);

    int cflag = cpu_readflag(C);

    cpu_setflag(bit,C);
    
    *reg= *reg << 1;

    *reg |= cflag;

    cpu_setflag(false,N);
    cpu_setflag(false,H);
    
        cpu_setflag(*reg == 0,Z);
     
}