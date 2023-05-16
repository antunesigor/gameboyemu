#include "common.h"

typedef struct {
} io_state;

u8 io_read(u16 address);

void io_write(u16 address, u8 value);

void io_init();