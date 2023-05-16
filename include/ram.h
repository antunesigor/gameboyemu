#include "common.h"

typedef struct {
    u8 internal_ram[0x2000];
    u8 high_ram[0x7E];
} ram_state;

u8 ram_read(u16 address);
void ram_write(u16 address, u8 value);

u8 hram_read(u16 address);
void hram_write(u16 address, u8 value);