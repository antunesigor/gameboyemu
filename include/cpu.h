#include "common.h"
#include "bus.h"

typedef struct {
    u8 a;
    u8 f;
    u8 b;
    u8 c;
    u8 d;
    u8 e;
    u8 h;
    u8 l;
} cpu_registers;

typedef struct {
    bool flag_z;
    bool flag_n;
    bool flag_h;
    bool flag_c;
    u16 pc;
    u16 sp;
    cpu_registers regs;
    u8 opcode;
    int delay;
    bool halt;
    bool stop;
} cpu_state;

void cpu_init();
int cpu_tick();

void cpu_setflag(bool value, int pos);
int cpu_readflag(int pos);

void cpu_setdelay(int value);

void cpu_printregs();

void cpu_interrupt(u16 address);
void cpu_disable_halt();