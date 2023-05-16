#include "fifo.h"
#include "ppu.h"
#include "bus.h"


void pixel_fifo_push(pixel_fifo *fifo, u32 value){
    fifo_entry *entry;
    entry = (fifo_entry*) malloc(sizeof(fifo_entry));
    entry->value = value;
    if(fifo->last == NULL){
        fifo->first = entry;
        fifo->last = entry;
    }
    else{
        fifo->last->next = entry;
        fifo->last = entry;
    }
    fifo->size = fifo->size + 1;
}

u32 pixel_fifo_pop(pixel_fifo *fifo){
    fifo_entry *entry = fifo->first;
    fifo->first = entry->next;
    u32 value = entry->value;
    fifo->size -= 1;
    if(fifo->size == 0)
        fifo->last = entry->next;
    free(entry);
    return value;
}

void pixel_fifo_flush(pixel_fifo *fifo){
    while(fifo->size > 0)
        pixel_fifo_pop(fifo);
}


