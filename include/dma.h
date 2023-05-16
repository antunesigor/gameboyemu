#include "common.h"

typedef struct {
    u16 address;
    int delay;
    int pos;
} dma_state;


void dma_start(u8 address_high);

void dma_tick();
