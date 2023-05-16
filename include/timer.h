#include "common.h"

typedef struct {
    u8 div; //div counter
    u8 tima; //timer counter
    u8 tma; //timer modulo: default value when tima overflows
    bool enabled;
    int frequency;
    int div_delay;
    int tima_delay;
} timer_state;

void timer_init(); 

void timer_tick();

u8 timer_read(u16 address);

void timer_write(u16 address, u8 value);


