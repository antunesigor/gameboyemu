#include "common.h"

typedef enum fetcher_step{
    GET_TILE,
    GET_TILE_LOW,
    GET_TILE_HIGH,
    PUSH
} fetcher_step;

typedef struct {
    u8 posx;
    u8 posy;
    u8 tile_index;
    bool flipX;
    bool flipY;
    bool pallete;
    bool bg_priority;
    u8 sprite_low;
    u8 sprite_high;
} sprite;

typedef struct {
    fetcher_step current_step;
    u16 tile_address;
    u16 some_address;
    u8 tile_index;
    u8 tile_x;
    u8 tile_low;
    u8 tile_high;
    sprite* selected;

} pixel_fetcher;

void ppu_pipeline_init();

sprite* sprites_fetch();

void pixel_fetch();

u32 bg_pixel_pop();

u32 bg_fifo_size();

void tilemap_index_dump();

void bg_pixel_flush();

void hblank();