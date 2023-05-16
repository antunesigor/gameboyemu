#include "common.h"

typedef struct fifo_entry {
    fifo_entry *next;
    u32 value;
} fifo_entry;

typedef struct {
    fifo_entry *first;
    fifo_entry *last;
    u32 size;
} pixel_fifo;

void fifo_init();

pixel_fifo* init();

void pixel_fifo_push(pixel_fifo *fifo, u32 value);

u32 pixel_fifo_pop(pixel_fifo *fifo);

void pixel_fifo_flush(pixel_fifo *fifo);

