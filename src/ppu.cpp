#include "ppu.h"
#include "interrupt.h"
#include "fifo.h"
#include "ppu_pipeline.h"
#include "bus.h"
#include <cstring>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <thread>
ppu_state ppu;
int fakely;

u32 *videobuffer;
int pushedx;

static u32 tile_colors[4] = {0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000}; 

void buffer_dump(){

for(int i=0;i<4;i++){
    printf("bg[%d] = %d %8.8X\n",i,ppu.bgp_pallete[i],get_color(2,i));
    printf("obp0[%d] = %d %8.8X\n",i,ppu.obp0_pallete[i],get_color(0,i));
    printf("obp1[%d] = %d %8.8X\n",i,ppu.obp1_pallete[i],get_color(1,i));
    
}
    
      for (int line_num = 0; line_num < 8; line_num++) {
        for (int x = 0; x < 8; x++) {
            printf("$%d: %4.4X ",x + (line_num * 160),videobuffer[x + (line_num * 160)]);
        }
        printf("\n");
    }
}

u32 get_color(int pallete, int index){
    if(pallete == 2) //bgp
        return tile_colors[ppu.bgp_pallete[index]];
    if(pallete == 0) //obp0
        return tile_colors[ppu.obp0_pallete[index]];
    if(pallete == 1) //obp1
        return tile_colors[ppu.obp1_pallete[index]];     

    return 0; 
}

u8 vram_read(u16 address){
    return ppu.vram[address];
}
void vram_write(u16 address, u8 value){
    ppu.vram[address] = value;
}

u8 oam_read(u16 address){
    return ppu.oam[address];
}
void oam_write(u16 address, u8 value){
    ppu.oam[address] = value;
}

void ppu_init(){
     videobuffer = (u32*) malloc(160 * 144 * sizeof(u32));
     for(int i=0;i<4;i++){
        ppu.bgp_pallete[i] = i;
        ppu.obp0_pallete[i] = i;
        ppu.obp1_pallete[i] = i;
        
     }
     
     //ppu.lcds.ly = 0;
     pushedx = 0;
     ppu_pipeline_init();
}

u32* get_video_buffer(){
    return videobuffer;
}

lcd_control *get_lcdc(){
    return &(ppu.lcdc);
}

lcd_state *get_lcds(){
    return &(ppu.lcds);
}

void ppu_tick(){
  //  printf("lx/ly %d %d\n", get_lcds()->lx - 80, get_lcds()->ly);
     
    if(ppu.lcds.ly >= 144 && ppu.lcds.ly <= 153){     
        if(ppu.lcds.ly == 144){
            ppu.lcds.mode = 1;
            interrupt_request(0b1);
            if(ppu.lcds.mode1_interrupt)
                interrupt_request(0b10);  
        }
        ppu.lcds.lx++;
    }
    else if(ppu.lcds.lx >= 0 && ppu.lcds.lx < 80){
            ppu.lcds.mode = 2;
            ppu.lcds.lx++;
        if(ppu.lcds.mode2_interrupt)
            interrupt_request(0b10);

        //oam search
        if(ppu.lcds.lx == 80)
            sprites_fetch();
        
        if(ppu.lcds.lx == 80){
            hblank();
         }
    }
    else if(ppu.lcds.lx >= 80 && ppu.lcds.lx < 240){
         
        
        ppu.lcds.mode = 3;
        if(ppu.lcds.lx % 2 == 0)
            pixel_fetch();
        if(bg_fifo_size() > 8){
            u32 val = bg_pixel_pop();
            videobuffer[(ppu.lcds.lx - 80) + ppu.lcds.ly * 160] = val;             
            ppu.lcds.lx++;
        }
    }
    else{
        
        ppu.lcds.lx++ ;
        ppu.lcds.mode = 0;
         if(ppu.lcds.mode1_interrupt)
            interrupt_request(0b10);
    }
        
    if(ppu.lcds.lx == 456){
         
        ppu.lcds.lx = 0;
       if(ppu.lcds.wy <= ppu.lcds.ly && ppu.lcdc.window_display && ppu.lcds.wx <= 160){
            ppu.lcds.wly++;
        }

        if(ppu.lcds.ly++ == 153){
            ppu.lcds.ly = 0;
            ppu.lcds.wly = 0;
        }
            if(ppu.lcds.lyc_interrupt && ppu.lcds.ly == ppu.lcds.lyc){
                //printf("MATCH LY=LYC\n");
                interrupt_request(0b10);
            }
                
    }
}

u8 ppu_read(u16 address){
    if(address == 0x40)
    return ppu.lcdc.operation << 7 | 
           ppu.lcdc.window_display_select << 6 | 
           ppu.lcdc.window_display << 5 | 
           ppu.lcdc.tile_data_select << 4 | 
           ppu.lcdc.bg_display_select << 3 | 
           ppu.lcdc.sprite_size << 2 | 
           ppu.lcdc.sprite_display << 1 | 
           ppu.lcdc.bg_window_display; 

    if(address == 0x41)
        return ppu.lcds.lyc_interrupt << 6 | 
               ppu.lcds.mode2_interrupt << 5 | 
               ppu.lcds.mode1_interrupt << 4 | 
               ppu.lcds.mode0_interrupt << 3 | 
               (ppu.lcds.ly == ppu.lcds.lyc) << 2 | 
               ppu.lcds.mode;      
    if(address == 0x42)
        return ppu.lcds.scy;      
    if(address == 0x43)
        return ppu.lcds.scx;
    if(address == 0x44)
        return ppu.lcds.ly;
    if(address == 0x45)
        return ppu.lcds.lyc;
    if(address == 0x4A)
        return ppu.lcds.wy;
    if(address == 0x4B)
        return ppu.lcds.wx;                 
    return 0;
}
void ppu_write(u16 address, u8 value){
    if(address == 0x41){
        ppu.lcds.lyc_interrupt = value & 0b01000000;
        ppu.lcds.mode2_interrupt = value & 0b00100000;
        ppu.lcds.mode1_interrupt = value & 0b00010000;
        ppu.lcds.mode0_interrupt  = value & 0b00001000;
        return;
    }
    if(address == 0x40){
        ppu.lcdc.operation = value & 0b10000000;
        ppu.lcdc.window_display_select = value & 0b01000000;
        ppu.lcdc.window_display = value & 0b00100000;
        ppu.lcdc.tile_data_select = value & 0b00010000;
        ppu.lcdc.bg_display_select = value & 0b00001000;
        ppu.lcdc.sprite_size = value & 0b00000100;
        ppu.lcdc.sprite_display = value & 0b00000010;
        ppu.lcdc.bg_window_display = value & 0b00000001;
        return;
    }
    if(address == 0x42){
        ppu.lcds.scy = value;
        return;
    }
    if(address == 0x43){
        ppu.lcds.scx = value;
        return;
    }
    if(address == 0x44){
        printf("can't write on LY\n");
        return;
    }
    if(address == 0x45){
        ppu.lcds.lyc = value;
        return;
    }
    if(address == 0x47){ //BGP
        printf("writing on BGP %2.2X\n",value);
        ppu.bgp_pallete[0] = (value & 0b11);
        ppu.bgp_pallete[1] = (value & 0b1100) >> 2;
        ppu.bgp_pallete[2] = (value & 0b110000) >> 4;
        ppu.bgp_pallete[3] = (value & 0b11000000) >> 6;
        return;
    }
    if(address == 0x48){ //OBP0
        printf("writing on OBP0 %2.2X\n",value);
        ppu.obp0_pallete[0] = (value & 0b11);
        ppu.obp0_pallete[1] = (value & 0b1100) >> 2;
        ppu.obp0_pallete[2] = (value & 0b110000) >> 4;
        ppu.obp0_pallete[3] = (value & 0b11000000) >> 6;
        return;
    }
    if(address == 0x49){ //OBP1
        printf("writing on OBP1 %2.2X\n",value);
        ppu.obp1_pallete[0] = (value & 0b11);
        ppu.obp1_pallete[1] = (value & 0b1100) >> 2;
        ppu.obp1_pallete[2] = (value & 0b110000) >> 4;
        ppu.obp1_pallete[3] = (value & 0b11000000) >> 6;
        return;
    }
    if(address == 0x4A){
        ppu.lcds.wy = value;
        return;
    }
    if(address == 0x4B){
        ppu.lcds.wx = value - 7;
        return;
    }

    return;
}