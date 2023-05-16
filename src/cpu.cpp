#include "cpu.h"
#include "operations.h"
#include "interrupt.h"

cpu_state cpu; 

void cpu_init(){
    /*
    Reg DMG MGB SGB SGB2 CGB AGB AGS
    AF 01B0h FFB0h 0100h FF??h 1180h 1100h 1100h
    BC 0013h 0013h 0014h ????h 0000h 0100h 0100h
    DE 00D8h 00D8h 0000h ????h 0008h 0008h 0008h
    HL 014Dh 014Dh C060h ????h 007Ch 007Ch 007Ch
    SP FFFEh FFFEh FFFEh ????h FFFEh FFFEh FFFEh
    PC 0100h 0100h 0100h 0100h 0100h 0100h 0100h
    */
    cpu.regs.a = 0x01;
    cpu.regs.f = 0xB0;
    cpu.regs.b = 0x00;
    cpu.regs.c = 0x13;
    cpu.regs.d = 0x00;
    cpu.regs.e = 0xd8;
    cpu.regs.h = 0x01;
    cpu.regs.l = 0x4d;
    cpu.sp = 0xfffe;
    cpu.pc = 0x0100;
    cpu.halt = false;
    cpu.stop = false;
};

void cpu_disable_halt(){
    cpu.halt = false;
}

void cpu_printregs(){
    printf("AF: %2.2X %2.2X | BC: %2.2X %2.2X | DE: %2.2X %2.2X | HL: %2.2X %2.2X | SP = %4.4X | PC = %4.4X\n",cpu.regs.a,cpu.regs.f,cpu.regs.b,cpu.regs.c,cpu.regs.d,cpu.regs.e,cpu.regs.h,cpu.regs.l,cpu.sp,cpu.pc);
}

void cpu_setflag(bool value, int pos){
    BIT_SET(cpu.regs.f,pos,value);
}
int cpu_readflag(int pos){
    return BIT(cpu.regs.f,pos);
}

void cpu_setdelay(int value){
    if(value > 0)
        cpu.delay = value;
}

void push_16(u16 value){
   // printf("PUSH %4.4X\n",value);
    bus_write(--cpu.sp,value >> 8);
    bus_write(--cpu.sp,value & 0xFF);
}
u16 pop_16(){
    u16 result = 0;
    u8 val1 = bus_read(cpu.sp++);
    u8 val2 = bus_read(cpu.sp++);
    result = val1;
    result |= (val2 << 8);
   //printf("POP %4.4X\n",result);
     
    return result;
}

void ldd_a_hl(bool inc){
    ld_r_addr(&cpu.regs.a, (cpu.regs.h << 8 | cpu.regs.l));
    if(inc){
        cpu.regs.l++;
        if(cpu.regs.l == 0){
            cpu.regs.h++;
        }
    }
    else{
        if(cpu.regs.l-- == 0){
            cpu.regs.h--;
        }
    }
}
void ldd_hl_a(bool inc){
    ld_addr_r((cpu.regs.h << 8 | cpu.regs.l),&cpu.regs.a);
    if(inc){
        cpu.regs.l++;
        if(cpu.regs.l == 0){
            cpu.regs.h++;
        }
    }
    else{
        if(cpu.regs.l-- == 0){
            cpu.regs.h--;
        }
    }
}

void push_2r(u8* reg_start){
    //printf("PUSH REG1 %2.2X REG2 %2.2X\n", reg_start[0],reg_start[1]);
    int value = reg_start[0] << 8 | reg_start[1];
    push_16(value);
}

void pop_2r(u8* reg_start){
    int value = pop_16(); 
    reg_start[0] = value >> 8;
    reg_start[1] = value & 0xFF;
    //printf("POP REG1 %2.2X REG2 %2.2X\n", reg_start[0],reg_start[1]);
}

void add_sp_sn(s8 value){
    int prev = cpu.sp;
    cpu.sp += value;

    cpu_setflag(false,6);
    cpu_setflag(false,7);

    if(value >= 0){
        cpu_setflag((int)(prev & 0xFF) > (int)(cpu.sp & 0xFF),4);    
        cpu_setflag((int)(prev & 0xF) + (int)(value & 0xF) > 0xF,5);
    }else{
        cpu_setflag((int)(prev & 0xFF) > (int)(cpu.sp & 0xFF),4);    
        cpu_setflag((int)(prev & 0xF) > (int)(cpu.sp & 0xF),5);
    }

   // printf("%4.4X + %d = %4.4X | Z %d N %d H %d C %d\n", prev, value,cpu.sp,cpu_readflag(7),cpu_readflag(6),cpu_readflag(5),cpu_readflag(4));
    
}

void ld_addr_sp(){
    u16 addr = ((bus_read(cpu.pc++)) | (bus_read(cpu.pc++) << 8));

    bus_write(addr, cpu.sp & 0xFF);
    bus_write(addr+1,cpu.sp >> 8);
}

void call(){
    int pcnew = bus_read(cpu.pc++) | bus_read(cpu.pc++) << 8; 
    push_16(cpu.pc);
    cpu.pc = pcnew;
}

void push(u8 value){
    bus_write(--cpu.sp,value);
}

void srl(u8* reg){
    int bit0 = *reg & 0x1;
    *reg = *reg >> 1;
    cpu_setflag(false,6);
    cpu_setflag(false,5);
    cpu_setflag(*reg == 0,7);
    cpu_setflag(bit0,4);
}

void sla(u8* reg){

    int bit7 = BIT(*reg,7);
    *reg = *reg << 1;

    cpu_setflag(*reg == 0,7);
    cpu_setflag(false,6);
    cpu_setflag(false,5);
    cpu_setflag(bit7 == 1,4);

}


void sra(u8* reg){

    int bit7 = *reg & 0x80;
    int bit0 = BIT(*reg,0);
    *reg = *reg >> 1;

    *reg |= bit7;
    cpu_setflag(*reg == 0,7);
    cpu_setflag(false,6);
    cpu_setflag(false,5);
    cpu_setflag(bit0 == 1,4);

}

void rst(u8 value){
    push_16(cpu.pc++);
    cpu.pc = value;
}

void daa(){
    u8 u = 0;
    int fc = 0;

    if (cpu_readflag(5) == 1 || (cpu_readflag(6) == 0 && (cpu.regs.a & 0xF) > 9)) {
        u = 6;
    }

    if (cpu_readflag(4) || (cpu_readflag(6) == 0 && cpu.regs.a > 0x99)) {
        u |= 0x60;
        fc = 1;
    }

    cpu.regs.a += cpu_readflag(6) ? -u : u;
    cpu_setflag(cpu.regs.a == 0, 7);
    cpu_setflag(false, 5);
    cpu_setflag(fc,4);
}

void ld_hl_sp(){
    s8 value = bus_read(cpu.pc++);
    int prev = cpu.sp;
    u16 result = cpu.sp + value;
    
    cpu_setflag(false,6);
    cpu_setflag(false,7);

    if(value >= 0){
        cpu_setflag((int)(prev & 0xFF) > (int)(result & 0xFF),4);    
        cpu_setflag((int)(prev & 0xF) + (int)(value & 0xF) > 0xF,5);
    }else{
        cpu_setflag((int)(prev & 0xFF) > (int)(result & 0xFF),4);    
        cpu_setflag((int)(prev & 0xF) > (int)(result & 0xF),5);
    }

    ld_2r_nn(&cpu.regs.h,result); 
}

int execute_cbop(){
    cpu.opcode = bus_read(cpu.pc++);
    //printf("%4.4X | CB %2.2X\n",(cpu.pc-1),cpu.opcode);

    int reg = cpu.opcode & 0xF;
    int operation = cpu.opcode & 0xF0;
    u8* regptr = NULL;
    u16 address = 0;

    switch (reg)
    {
        case 0x0:
        case 0x8:
            regptr = &cpu.regs.b;
            break;
        case 0x1:
        case 0x9:
            regptr = &cpu.regs.c;
            break;
        case 0x2:
        case 0xA:
            regptr = &cpu.regs.d;
            break;
        case 0x3:
        case 0xB:
            regptr = &cpu.regs.e;
            break;
        case 0x4:
        case 0xC:
            regptr = &cpu.regs.h;
            break;
        case 0x5:
        case 0xD:
            regptr = &cpu.regs.l;
            break;
        case 0x7:
        case 0xF:
            regptr = &cpu.regs.a;
            break;
        case 0x6:
        case 0xE:
        default:
            address = cpu.regs.h << 8 | cpu.regs.l;
            regptr = new u8[1];
            *regptr = bus_read(address);
            break;
    }

    operation |= (reg > 7 ? 1 : 0);

    switch(operation){
        case 0x00:
            rlc_r(regptr);
            break;
        case 0x01:
            rrc_r(regptr);   
            break;     
        case 0x10:
            rl_r(regptr);
            break;
        case 0x11:
            rr_r(regptr);
            break;
        case 0x20:
            sla(regptr);
            break;
        case 0x21:
            sra(regptr);
            break;
        case 0x30:
            *regptr = ((*regptr & 0xF) << 4) | ((*regptr & 0xF0) >> 4);
            cpu_setflag(*regptr == 0,7);
            cpu_setflag(false,6);
            cpu_setflag(false,5);
            cpu_setflag(false,4);
            break;
        case 0x31:
            srl(regptr);
            break;
        case 0x40:
            cpu_setflag((BIT(*regptr,0) == 0),7);
            cpu_setflag(false,6);
            cpu_setflag(true,5);   
            break;         
        case 0x41:
            cpu_setflag((BIT(*regptr,1) == 0),7);
            cpu_setflag(false,6);
            cpu_setflag(true,5);   
            break;         
        
        case 0x50:
            cpu_setflag((BIT(*regptr,2) == 0),7);
            cpu_setflag(false,6);
            cpu_setflag(true,5);   
            break;         
        case 0x51:
            cpu_setflag((BIT(*regptr,3) == 0),7);
            cpu_setflag(false,6);
            cpu_setflag(true,5);   
            break;         
        
        case 0x60:
            cpu_setflag((BIT(*regptr,4) == 0),7);
            cpu_setflag(false,6);
            cpu_setflag(true,5);   
            break;         
        
        case 0x61:
            cpu_setflag((BIT(*regptr,5) == 0),7);
            cpu_setflag(false,6);
            cpu_setflag(true,5);   
            break;         
        
        case 0x70:
            cpu_setflag((BIT(*regptr,6) == 0),7);
            cpu_setflag(false,6);
            cpu_setflag(true,5);   
            break;         
        
        case 0x71:
            cpu_setflag((BIT(*regptr,7) == 0),7);
            cpu_setflag(false,6);
            cpu_setflag(true,5);   
            break;         
        
        case 0x80:
            BIT_SET(*regptr,0,0);  
            break;      
        case 0x81:
            BIT_SET(*regptr,1,0);  
            break;      
        case 0x90:
            BIT_SET(*regptr,2,0);  
            break;      
        case 0x91:
            BIT_SET(*regptr,3,0);  
            break;      
        

        case 0xA0:
            BIT_SET(*regptr,4,0);  
            break;      
        case 0xA1:
            BIT_SET(*regptr,5,0);  
            break;      
        
        case 0xB0:
            BIT_SET(*regptr,6,0);  
            break;      
        
        case 0xB1:
            BIT_SET(*regptr,7,0);  
            break;      
        
        case 0xC0:
            BIT_SET(*regptr,0,1);  
            break;      
        
        case 0xC1:
            BIT_SET(*regptr,1,1);  
            break;      
        
        case 0xD0:
            BIT_SET(*regptr,2,1);  
            break;      
        case 0xD1:
            BIT_SET(*regptr,3,1);  
            break;      
        
        case 0xE0:
            BIT_SET(*regptr,4,1);  
            break;      
        case 0xE1:
            BIT_SET(*regptr,5,1);  
            break;      
        
        case 0xF0:
            BIT_SET(*regptr,6,1);  
            break;      
        case 0xF1:
            BIT_SET(*regptr,7,1);  
            break;      
        
        default:
        printf("problem\n");
         
         return -1;

    }

    if(address > 0){
        bus_write(address,*regptr);
        return 16;   
    }
    else
        return 8;
}


int execute_op(){
        
    cpu.opcode = bus_read(cpu.pc++);
  
   //printf("%4.4X | %2.2X \n", cpu.pc-1,cpu.opcode);
 
 switch(cpu.opcode){

        #pragma region 0x00
        case 0x00: return 4;
        case 0x01: ld_2r_nn(&cpu.regs.b, (bus_read(cpu.pc++) | bus_read(cpu.pc++) << 8)); return 12;
        case 0x02: ld_addr_r((cpu.regs.b << 8 | cpu.regs.c),&cpu.regs.a); return 8;
        case 0x03: add_2r_sn(&cpu.regs.b,1); return 8;
        case 0x04: inc_r(&cpu.regs.b); return 4;
        case 0x05: dec_r(&cpu.regs.b); return 4;
        case 0x06: ld_r_n(&cpu.regs.b, bus_read(cpu.pc++)); return 8;
        case 0x07: rlc_r(&cpu.regs.a); cpu_setflag(false,7); return 4;
        case 0x08: ld_addr_sp(); return 20;
        case 0x09: add_2r_nn(&cpu.regs.h,(cpu.regs.b << 8 | cpu.regs.c)); return 8;
        case 0x0A: ld_r_addr(&cpu.regs.a,(cpu.regs.b << 8 | cpu.regs.c)); return 8;
        case 0x0B: add_2r_sn(&cpu.regs.b,-1); return 8;
        case 0x0C: inc_r(&cpu.regs.c); return 4;
        case 0x0D: dec_r(&cpu.regs.c); return 4;
        case 0x0E: ld_r_n(&cpu.regs.c, bus_read(cpu.pc++)); return 8;
        case 0x0F: rrc_r(&cpu.regs.a); cpu_setflag(false,7); return 4;
    #pragma endregion
    #pragma region 0x10        
        case 0x10: cpu.stop = true; return 4; 
        case 0x11: ld_2r_nn(&cpu.regs.d, (bus_read(cpu.pc++) | bus_read(cpu.pc++) << 8)); return 12;
        case 0x12: ld_addr_r((cpu.regs.d << 8 | cpu.regs.e),&cpu.regs.a); return 8;
        case 0x13: add_2r_sn(&cpu.regs.d,1); return 8;
        case 0x14: inc_r(&cpu.regs.d); return 4;
        case 0x15: dec_r(&cpu.regs.d); return 4;
        case 0x16: ld_r_n(&cpu.regs.d, bus_read(cpu.pc++)); return 8;
        case 0x17: rl_r(&cpu.regs.a); cpu_setflag(false,7); return 4;
        case 0x18: cpu.pc += (s8)(bus_read(cpu.pc++));  return 12;
        case 0x19: add_2r_nn(&cpu.regs.h,(cpu.regs.d << 8 | cpu.regs.e)); return 8;
        case 0x1A: ld_r_addr(&cpu.regs.a,(cpu.regs.d << 8 | cpu.regs.e)); return 8;
        case 0x1B: add_2r_sn(&cpu.regs.d,-1); return 8;
        case 0x1C: inc_r(&cpu.regs.e); return 4;
        case 0x1D: dec_r(&cpu.regs.e); return 4;
        case 0x1E: ld_r_n(&cpu.regs.e, bus_read(cpu.pc++)); return 8;
        case 0x1F: rr_r(&cpu.regs.a); cpu_setflag(false,7); return 4;
#pragma endregion
#pragma region 0x20
        case 0x20: if(cpu_readflag(7) == 0) { cpu.pc += (s8)bus_read(cpu.pc++); return 12; } cpu.pc++; return 8; 
        case 0x21: ld_2r_nn(&cpu.regs.h, (bus_read(cpu.pc++) | bus_read(cpu.pc++) << 8)); return 12;
        case 0x22: ldd_hl_a(true); return 8;
        case 0x23: add_2r_sn(&cpu.regs.h,1); return 8;
        case 0x24: inc_r(&cpu.regs.h); return 4;
        case 0x25: dec_r(&cpu.regs.h); return 4;
        case 0x26: ld_r_n(&cpu.regs.h, bus_read(cpu.pc++)); return 8;
        case 0x27: daa(); return 4;
        case 0x28: if(cpu_readflag(7) == 1) { cpu.pc += (s8)bus_read(cpu.pc++); return 12; } cpu.pc++;  return 8;
        case 0x29: add_2r_nn(&cpu.regs.h,(cpu.regs.h << 8 | cpu.regs.l)); return 8;
        case 0x2A: ldd_a_hl(true); return 8;
        case 0x2B: add_2r_sn(&cpu.regs.h,-1); return 8;
        case 0x2C: inc_r(&cpu.regs.l); return 4;
        case 0x2D: dec_r(&cpu.regs.l); return 4;
        case 0x2E: ld_r_n(&cpu.regs.l, bus_read(cpu.pc++)); return 8;
        case 0x2F: cpu.regs.a = ~cpu.regs.a; cpu_setflag(true,6); cpu_setflag(true,5); return 4;
#pragma endregion
#pragma region 0x30
        case 0x30: if(cpu_readflag(4) == 0) { cpu.pc += (s8)bus_read(cpu.pc++); return 12; } cpu.pc++; return 8; 
        case 0x31: cpu.sp = (bus_read(cpu.pc++) | bus_read(cpu.pc++) << 8); return 12;
        case 0x32: ldd_hl_a(false); return 8;
        case 0x33: cpu.sp++; return 8;
        case 0x34: inc_addr((cpu.regs.h << 8 | cpu.regs.l)); return 12;
        case 0x35: dec_addr((cpu.regs.h << 8 | cpu.regs.l)); return 12;
        case 0x36: ld_addr_n((cpu.regs.h << 8 | cpu.regs.l),bus_read(cpu.pc++)); return 12;
        case 0x37: cpu_setflag(true,4); cpu_setflag(false,5); cpu_setflag(false,6); return 4;
        case 0x38: if(cpu_readflag(4) == 1) { cpu.pc += (s8)bus_read(cpu.pc++); return 12; } cpu.pc++;  return 8; 
        case 0x39: add_2r_nn(&cpu.regs.h,cpu.sp); return 8;
        case 0x3A: ldd_a_hl(false); return 8;
        case 0x3B: cpu.sp--; return 8;
        case 0x3C: inc_r(&cpu.regs.a); return 4;
        case 0x3D: dec_r(&cpu.regs.a); return 4;
        case 0x3E: ld_r_n(&cpu.regs.a, bus_read(cpu.pc++)); return 8;
        case 0x3F: cpu_setflag(cpu_readflag(4) == 0,4); cpu_setflag(false,5); cpu_setflag(false,6); return 4;
#pragma endregion
#pragma region 0x40
        case 0x40: ld_r1_r2(&cpu.regs.b,&cpu.regs.b); return 4;
        case 0x41: ld_r1_r2(&cpu.regs.b,&cpu.regs.c); return 4;
        case 0x42: ld_r1_r2(&cpu.regs.b,&cpu.regs.d); return 4;
        case 0x43: ld_r1_r2(&cpu.regs.b,&cpu.regs.e); return 4;
        case 0x44: ld_r1_r2(&cpu.regs.b,&cpu.regs.h); return 4;
        case 0x45: ld_r1_r2(&cpu.regs.b,&cpu.regs.l); return 4;
        case 0x46: ld_r_addr(&cpu.regs.b,(cpu.regs.h << 8 | cpu.regs.l)); return 8;
        case 0x47: ld_r1_r2(&cpu.regs.b,&cpu.regs.a); return 4;
        case 0x48: ld_r1_r2(&cpu.regs.c,&cpu.regs.b); return 4;
        case 0x49: ld_r1_r2(&cpu.regs.c,&cpu.regs.c); return 4;
        case 0x4A: ld_r1_r2(&cpu.regs.c,&cpu.regs.d); return 4;
        case 0x4B: ld_r1_r2(&cpu.regs.c,&cpu.regs.e); return 4;
        case 0x4C: ld_r1_r2(&cpu.regs.c,&cpu.regs.h); return 4;
        case 0x4D: ld_r1_r2(&cpu.regs.c,&cpu.regs.l); return 4;
        case 0x4E: ld_r_addr(&cpu.regs.c,(cpu.regs.h << 8 | cpu.regs.l)); return 8;
        case 0x4F: ld_r1_r2(&cpu.regs.c,&cpu.regs.a); return 4;
#pragma endregion
#pragma region 0x50
        case 0x50: ld_r1_r2(&cpu.regs.d,&cpu.regs.b); return 4;
        case 0x51: ld_r1_r2(&cpu.regs.d,&cpu.regs.c); return 4;
        case 0x52: ld_r1_r2(&cpu.regs.d,&cpu.regs.d); return 4;
        case 0x53: ld_r1_r2(&cpu.regs.d,&cpu.regs.e); return 4;
        case 0x54: ld_r1_r2(&cpu.regs.d,&cpu.regs.h); return 4;
        case 0x55: ld_r1_r2(&cpu.regs.d,&cpu.regs.l); return 4;
        case 0x56: ld_r_addr(&cpu.regs.d,(cpu.regs.h << 8 | cpu.regs.l)); return 8;
        case 0x57: ld_r1_r2(&cpu.regs.d,&cpu.regs.a); return 4;
        case 0x58: ld_r1_r2(&cpu.regs.e,&cpu.regs.b); return 4;
        case 0x59: ld_r1_r2(&cpu.regs.e,&cpu.regs.c); return 4;
        case 0x5A: ld_r1_r2(&cpu.regs.e,&cpu.regs.d); return 4;
        case 0x5B: ld_r1_r2(&cpu.regs.e,&cpu.regs.e); return 4;
        case 0x5C: ld_r1_r2(&cpu.regs.e,&cpu.regs.h); return 4;
        case 0x5D: ld_r1_r2(&cpu.regs.e,&cpu.regs.l); return 4;
        case 0x5E: ld_r_addr(&cpu.regs.e,(cpu.regs.h << 8 | cpu.regs.l)); return 8;
        case 0x5F: ld_r1_r2(&cpu.regs.e,&cpu.regs.a); return 4;
#pragma endregion
#pragma region 0x60        
        case 0x60: ld_r1_r2(&cpu.regs.h,&cpu.regs.b); return 4;
        case 0x61: ld_r1_r2(&cpu.regs.h,&cpu.regs.c); return 4;
        case 0x62: ld_r1_r2(&cpu.regs.h,&cpu.regs.d); return 4;
        case 0x63: ld_r1_r2(&cpu.regs.h,&cpu.regs.e); return 4;
        case 0x64: ld_r1_r2(&cpu.regs.h,&cpu.regs.h); return 4;
        case 0x65: ld_r1_r2(&cpu.regs.h,&cpu.regs.l); return 4;
        case 0x66: ld_r_addr(&cpu.regs.h,(cpu.regs.h << 8 | cpu.regs.l)); return 8;
        case 0x67: ld_r1_r2(&cpu.regs.h,&cpu.regs.a); return 4;
        case 0x68: ld_r1_r2(&cpu.regs.l,&cpu.regs.b); return 4;
        case 0x69: ld_r1_r2(&cpu.regs.l,&cpu.regs.c); return 4;
        case 0x6A: ld_r1_r2(&cpu.regs.l,&cpu.regs.d); return 4;
        case 0x6B: ld_r1_r2(&cpu.regs.l,&cpu.regs.e); return 4;
        case 0x6C: ld_r1_r2(&cpu.regs.l,&cpu.regs.h); return 4;
        case 0x6D: ld_r1_r2(&cpu.regs.l,&cpu.regs.l); return 4;
        case 0x6E: ld_r_addr(&cpu.regs.l,(cpu.regs.h << 8 | cpu.regs.l)); return 8;
        case 0x6F: ld_r1_r2(&cpu.regs.l,&cpu.regs.a); return 4;
#pragma endregion
#pragma region 0x70
        case 0x70: ld_addr_r((cpu.regs.h << 8 | cpu.regs.l),&cpu.regs.b); return 8;
        case 0x71: ld_addr_r((cpu.regs.h << 8 | cpu.regs.l),&cpu.regs.c); return 8;
        case 0x72: ld_addr_r((cpu.regs.h << 8 | cpu.regs.l),&cpu.regs.d); return 8;
        case 0x73: ld_addr_r((cpu.regs.h << 8 | cpu.regs.l),&cpu.regs.e); return 8;
        case 0x74: ld_addr_r((cpu.regs.h << 8 | cpu.regs.l),&cpu.regs.h); return 8;
        case 0x75: ld_addr_r((cpu.regs.h << 8 | cpu.regs.l),&cpu.regs.l); return 8;
        case 0x76: cpu.halt = true; return 4;
        case 0x77: ld_addr_r((cpu.regs.h << 8 | cpu.regs.l),&cpu.regs.a); return 8;
        case 0x78: ld_r1_r2(&cpu.regs.a,&cpu.regs.b); return 4;
        case 0x79: ld_r1_r2(&cpu.regs.a,&cpu.regs.c); return 4;
        case 0x7A: ld_r1_r2(&cpu.regs.a,&cpu.regs.d); return 4;
        case 0x7B: ld_r1_r2(&cpu.regs.a,&cpu.regs.e); return 4;
        case 0x7C: ld_r1_r2(&cpu.regs.a,&cpu.regs.h); return 4;
        case 0x7D: ld_r1_r2(&cpu.regs.a,&cpu.regs.l); return 4;
        case 0x7E: ld_r_addr(&cpu.regs.a,(cpu.regs.h << 8 | cpu.regs.l)); return 8;
        case 0x7F: ld_r1_r2(&cpu.regs.a,&cpu.regs.a); return 4;
#pragma endregion
#pragma region 0x80
        case 0x80: add_r_n(&cpu.regs.a, cpu.regs.b); return 4;
        case 0x81: add_r_n(&cpu.regs.a, cpu.regs.c); return 4;
        case 0x82: add_r_n(&cpu.regs.a, cpu.regs.d); return 4;
        case 0x83: add_r_n(&cpu.regs.a, cpu.regs.e); return 4;
        case 0x84: add_r_n(&cpu.regs.a, cpu.regs.h); return 4;
        case 0x85: add_r_n(&cpu.regs.a, cpu.regs.l); return 4;
        case 0x86: add_r_n(&cpu.regs.a, bus_read(cpu.regs.h << 8 | cpu.regs.l)); return 8;
        case 0x87: add_r_n(&cpu.regs.a, cpu.regs.a); return 4;
        case 0x88: adc_r_n(&cpu.regs.a, cpu.regs.b); return 4;
        case 0x89: adc_r_n(&cpu.regs.a, cpu.regs.c); return 4;
        case 0x8A: adc_r_n(&cpu.regs.a, cpu.regs.d); return 4;
        case 0x8B: adc_r_n(&cpu.regs.a, cpu.regs.e); return 4;
        case 0x8C: adc_r_n(&cpu.regs.a, cpu.regs.h); return 4;
        case 0x8D: adc_r_n(&cpu.regs.a, cpu.regs.l); return 4;
        case 0x8E: adc_r_n(&cpu.regs.a, bus_read(cpu.regs.h << 8 | cpu.regs.l)); return 8;
        case 0x8F: adc_r_n(&cpu.regs.a, cpu.regs.a); return 4;
#pragma endregion
#pragma region 0x90
        case 0x90: sub_r_n(&cpu.regs.a, cpu.regs.b); return 4;
        case 0x91: sub_r_n(&cpu.regs.a, cpu.regs.c); return 4;
        case 0x92: sub_r_n(&cpu.regs.a, cpu.regs.d); return 4;
        case 0x93: sub_r_n(&cpu.regs.a, cpu.regs.e); return 4;
        case 0x94: sub_r_n(&cpu.regs.a, cpu.regs.h); return 4;
        case 0x95: sub_r_n(&cpu.regs.a, cpu.regs.l); return 4;
        case 0x96: sub_r_n(&cpu.regs.a, bus_read((cpu.regs.h << 8 | cpu.regs.l))); return 8;
        case 0x97: sub_r_n(&cpu.regs.a, cpu.regs.a); return 4;
        case 0x98: sbc_r_n(&cpu.regs.a, cpu.regs.b); return 4;
        case 0x99: sbc_r_n(&cpu.regs.a, cpu.regs.c); return 4;
        case 0x9A: sbc_r_n(&cpu.regs.a, cpu.regs.d); return 4;
        case 0x9B: sbc_r_n(&cpu.regs.a, cpu.regs.e); return 4;
        case 0x9C: sbc_r_n(&cpu.regs.a, cpu.regs.h); return 4;
        case 0x9D: sbc_r_n(&cpu.regs.a, cpu.regs.l); return 4;
        case 0x9E: sbc_r_n(&cpu.regs.a, bus_read((cpu.regs.h << 8 | cpu.regs.l))); return 8;
        case 0x9F: sbc_r_n(&cpu.regs.a, cpu.regs.a); return 4;
#pragma endregion
#pragma region 0xA0
        case 0xA0: and_r_n(&cpu.regs.a, cpu.regs.b); return 4;
        case 0xA1: and_r_n(&cpu.regs.a, cpu.regs.c); return 4;
        case 0xA2: and_r_n(&cpu.regs.a, cpu.regs.d); return 4;
        case 0xA3: and_r_n(&cpu.regs.a, cpu.regs.e); return 4;
        case 0xA4: and_r_n(&cpu.regs.a, cpu.regs.h); return 4;
        case 0xA5: and_r_n(&cpu.regs.a, cpu.regs.l); return 4;
        case 0xA6: and_r_n(&cpu.regs.a, bus_read((cpu.regs.h << 8 | cpu.regs.l))); return 8;
        case 0xA7: and_r_n(&cpu.regs.a, cpu.regs.a); return 4;
        case 0xA8: xor_r_n(&cpu.regs.a, cpu.regs.b); return 4;
        case 0xA9: xor_r_n(&cpu.regs.a, cpu.regs.c); return 4;
        case 0xAA: xor_r_n(&cpu.regs.a, cpu.regs.d); return 4;
        case 0xAB: xor_r_n(&cpu.regs.a, cpu.regs.e); return 4;
        case 0xAC: xor_r_n(&cpu.regs.a, cpu.regs.h); return 4;
        case 0xAD: xor_r_n(&cpu.regs.a, cpu.regs.l); return 4;
        case 0xAE: xor_r_n(&cpu.regs.a, bus_read((cpu.regs.h << 8 | cpu.regs.l))); return 8;
        case 0xAF: xor_r_n(&cpu.regs.a, cpu.regs.a); return 4;
#pragma endregion
#pragma region 0xB0
        case 0xB0: or_r_n(&cpu.regs.a, cpu.regs.b); return 4;
        case 0xB1: or_r_n(&cpu.regs.a, cpu.regs.c); return 4;
        case 0xB2: or_r_n(&cpu.regs.a, cpu.regs.d); return 4;
        case 0xB3: or_r_n(&cpu.regs.a, cpu.regs.e); return 4;
        case 0xB4: or_r_n(&cpu.regs.a, cpu.regs.h); return 4;
        case 0xB5: or_r_n(&cpu.regs.a, cpu.regs.l); return 4;
        case 0xB6: or_r_n(&cpu.regs.a, bus_read((cpu.regs.h << 8 | cpu.regs.l))); return 8;
        case 0xB7: or_r_n(&cpu.regs.a, cpu.regs.a); return 4;
        case 0xB8: cp_r_n(&cpu.regs.a, cpu.regs.b); return 4;
        case 0xB9: cp_r_n(&cpu.regs.a, cpu.regs.c); return 4;
        case 0xBA: cp_r_n(&cpu.regs.a, cpu.regs.d); return 4;
        case 0xBB: cp_r_n(&cpu.regs.a, cpu.regs.e); return 4;
        case 0xBC: cp_r_n(&cpu.regs.a, cpu.regs.h); return 4;
        case 0xBD: cp_r_n(&cpu.regs.a, cpu.regs.l); return 4;
        case 0xBE: cp_r_n(&cpu.regs.a, bus_read((cpu.regs.h << 8 | cpu.regs.l))); return 8;
        case 0xBF: cp_r_n(&cpu.regs.a, cpu.regs.a); return 4;
#pragma endregion
#pragma region 0xC0
        case 0xC0: if(cpu_readflag(7) == 0) cpu.pc = pop_16(); return 8;
        case 0xC1: pop_2r(&cpu.regs.b); return 12;
        case 0xC2: if(cpu_readflag(7) == 0) cpu.pc = bus_read(cpu.pc++) | bus_read(cpu.pc++) << 8; else cpu.pc += 2; return 12;
        case 0xC3: cpu.pc = bus_read(cpu.pc++) | bus_read(cpu.pc++) << 8; return 12;
        case 0xC4: if(cpu_readflag(7) == 0) call(); else cpu.pc += 2; return 12;
        case 0xC5: push_2r(&cpu.regs.b); return 16;
        case 0xC6: add_r_n(&cpu.regs.a, bus_read(cpu.pc++)); return 8;
        case 0xC7: rst(0x00); return 32;
        case 0xC8: if(cpu_readflag(7) == 1) cpu.pc = pop_16(); return 8;
        case 0xC9: cpu.pc = pop_16(); return 8;
        case 0xCA: if(cpu_readflag(7) == 1) cpu.pc = bus_read(cpu.pc++) | bus_read(cpu.pc++) << 8; else cpu.pc += 2;  return 12;
        case 0xCB: return execute_cbop();
        case 0xCC: if(cpu_readflag(7) == 1) call(); else cpu.pc += 2; return 12;
        case 0xCD: call(); return 12;
        case 0xCE: adc_r_n(&cpu.regs.a, bus_read(cpu.pc++)); return 8;
        case 0xCF: rst(0x08); return 32;
#pragma endregion

        case 0xD0: if(cpu_readflag(4) == 0) cpu.pc = pop_16(); return 8;
        case 0xD1: pop_2r(&cpu.regs.d); return 12;
        case 0xD2: if(cpu_readflag(4) == 0) cpu.pc = bus_read(cpu.pc++) | bus_read(cpu.pc++) << 8; else cpu.pc += 2; return 12;
        case 0xD3: printf("UNUSED\n"); return -1;
        case 0xD4: if(cpu_readflag(4) == 0) call(); else cpu.pc += 2;  return 12;
        case 0xD5: push_2r(&cpu.regs.d); return 16;
        case 0xD6: sub_r_n(&cpu.regs.a, bus_read(cpu.pc++)); return 8;
        case 0xD7: rst(0x10); return 32;
        case 0xD8: if(cpu_readflag(4) == 1) cpu.pc = pop_16(); return 8;
        case 0xD9: cpu.pc = pop_16(); enable_interrupt(); return 8;
        case 0xDA: if(cpu_readflag(4) == 1) cpu.pc = bus_read(cpu.pc++) | bus_read(cpu.pc++) << 8; else cpu.pc += 2;  return 12;
        case 0xDB: printf("UNUSED\n"); return -1;
        case 0xDC: if(cpu_readflag(4) == 1) call(); else cpu.pc += 2;  return 12;
        case 0xDD: printf("UNUSED\n"); return -1;
        case 0xDE: sbc_r_n(&cpu.regs.a,bus_read(cpu.pc++)); return 8;
        case 0xDF: rst(0x18); return 32;

#pragma region 0xE0
        case 0xE0: ld_addr_r(0xFF00 + bus_read(cpu.pc++),&cpu.regs.a); return 12;
        case 0xE1: pop_2r(&cpu.regs.h); return 12;
        case 0xE2: ld_addr_r(0xFF00 + cpu.regs.c,&cpu.regs.a); return 8;
        case 0xE3: printf("UNUSED\n"); return -1;
        case 0xE4: printf("UNUSED\n"); return -1;
        case 0xE5: push_2r(&cpu.regs.h); return 16;
        case 0xE6: and_r_n(&cpu.regs.a, bus_read(cpu.pc++)); return 8;
        case 0xE7: rst(0x20); return 32;
        case 0xE8: add_sp_sn(bus_read(cpu.pc++)); return 16;
        case 0xE9: cpu.pc = cpu.regs.l | cpu.regs.h << 8; return 4;
        case 0xEA: ld_addr_r((bus_read(cpu.pc++) | bus_read(cpu.pc++) << 8),&cpu.regs.a); return 16;
        case 0xEB: printf("UNUSED\n"); return -1;
        case 0xEC: printf("UNUSED\n"); return -1;
        case 0xED: printf("UNUSED\n"); return -1;
        case 0xEE: xor_r_n(&cpu.regs.a, bus_read(cpu.pc++)); return 8;
        case 0xEF: rst(0x28); return 32;
#pragma endregion

        case 0xF0: ld_r_addr(&cpu.regs.a, 0xFF00 + bus_read(cpu.pc++)); return 12;
        case 0xF1: pop_2r(&cpu.regs.a); cpu.regs.f &= 0xF0; return 12;
        case 0xF2: ld_r_addr(&cpu.regs.a, 0xFF00 + cpu.regs.c); return 8;
        case 0xF3: disable_interrupt(); return 4;
        case 0xF4: printf("UNUSED\n"); return -1;
        case 0xF5: push_2r(&cpu.regs.a); return 16;
        case 0xF6: or_r_n(&cpu.regs.a, bus_read(cpu.pc++)); return 8;
        case 0xF7: rst(0x30); return 32;
        case 0xF8: ld_hl_sp(); return 12;
        case 0xF9: cpu.sp = cpu.regs.h << 8 | cpu.regs.l; return 8;
        case 0xFA: ld_r_addr(&cpu.regs.a,(bus_read(cpu.pc++)| bus_read(cpu.pc++) << 8)); return 16;
        case 0xFB: enable_interrupt(); return 4;
        case 0xFC: printf("UNUSED\n"); return -1;
        case 0xFD: printf("UNUSED\n"); return -1;
        case 0xFE: cp_r_n(&cpu.regs.a, bus_read(cpu.pc++)); return 8;
        case 0xFF: rst(0x38); return 32;
        

        default:
            return -1;

    }
    return -1;
}


int cpu_tick(){    
    if(--cpu.delay > 0){
        return 0;
    }
    if(cpu.halt){
        return 4;
    }
    if(cpu.pc == 0x8000)
        return -1;
    return execute_op();   
}

void cpu_interrupt(u16 address){
    disable_interrupt();
    cpu.halt = false;
    push_16(cpu.pc);
    cpu.pc = address;
}
