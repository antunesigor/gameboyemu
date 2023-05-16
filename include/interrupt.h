#include "common.h"

typedef struct {
    bool vblank;
    bool lcd;
    bool timer;
    bool serial;
    bool joypad;
} interrupt_stat;

typedef struct {
    /*
    Bit 0: VBlank   Interrupt Enable  (INT $40)  (1=Enable)
Bit 1: LCD STAT Interrupt Enable  (INT $48)  (1=Enable)
Bit 2: Timer    Interrupt Enable  (INT $50)  (1=Enable)
Bit 3: Serial   Interrupt Enable  (INT $58)  (1=Enable)
Bit 4: Joypad   Interrupt Enable  (INT $60)  (1=Enable)
*/
    interrupt_stat enable_stat;
    interrupt_stat request_stat;
    bool ime;
} interrupt_state;

void interrupt_init();

void interrupt_check();

void enable_interrupt();

void disable_interrupt();

u8 interrupt_read(u16 address);

void interrupt_write(u16 address, u8 value);

void interrupt_request(u8 source);

void interrupt_enable(u8 source);
