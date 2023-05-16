#include "common.h"

typedef struct {
    bool operation; //0 = stop completely .. 1 = operation
    bool window_display_select; // 0 = $9800-$9BFF or 1 = $9C00-$9FFF
    bool bg_display_select; // 0 = $9800-$9BFF or 1 = $9C00-$9FFF
    bool window_display;
    bool tile_data_select; // 0 = $8800-$97FF or 1 = $8000-$8FFF
    bool sprite_size; //0 = 8*8 or 1 = 8*16
    bool sprite_display;
    bool bg_window_display;
} lcd_control;

typedef struct {
    int lx;
    u8 ly;
    u8 lyc;
    u8 mode;
    bool lyc_interrupt;
    bool mode2_interrupt;
    bool mode1_interrupt;
    bool mode0_interrupt;
    u8 scy;
    u8 scx;
    u8 wx;
    u8 wy;
    u8 wly;
} lcd_state;

typedef struct {
    u8 vram[0x2000];
    u8 oam[0xA0];
    lcd_control lcdc;
    lcd_state lcds;
    int bgp_pallete[4];
    int obp0_pallete[4];
    int obp1_pallete[4];
} ppu_state;

u32 get_color(int pallete, int index);

lcd_control *get_lcdc();

lcd_state *get_lcds();

u8 vram_read(u16 address);
void vram_write(u16 address, u8 value);

u8 oam_read(u16 address);
void oam_write(u16 address, u8 value);

void ppu_init();
void ppu_tick();

u32* get_video_buffer();

void buffer_dump();
u8 ppu_read(u16 address);
void ppu_write(u16 address, u8 value);



