#include "ppu_pipeline.h"
#include "ppu.h"
#include "fifo.h"
#include "bus.h"
#include <chrono>
#include <unistd.h>

static pixel_fetcher fetcher;
//static u32 tile_colors[4] = {0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000}; 

pixel_fifo* bg_fifo;
pixel_fifo* sprite_fifo;

sprite* selected_sprites[10];
sprite* sprite_depth[5];
int count = 0;

void ppu_pipeline_init(){
    bg_fifo = (pixel_fifo*) malloc(sizeof(pixel_fifo));
    sprite_fifo = (pixel_fifo*) malloc(sizeof(pixel_fifo));
}

void bg_pixel_flush(){
    pixel_fifo_flush(bg_fifo);
}

u32 bg_pixel_pop(){
      return pixel_fifo_pop(bg_fifo);
}
u32 bg_fifo_size(){
    return bg_fifo->size;
}

void hblank(){
    fetcher.tile_x = 0;
    fetcher.current_step = GET_TILE;
    pixel_fifo_flush(bg_fifo);
}

void tilemap_index_dump(){

    u8 tile_index_address;
 for(int y = 0; y < 18; y++){
        
    for(int x = 0; x < 20; x++){


        
           if((get_lcdc()->bg_display_select)){ //9C00
                        tile_index_address = bus_read(0x9C00 + ((x + (get_lcds()->scx / 8)) % 20) + (((y + (get_lcds()->scy / 8)) % 18) * 32));
                    }
                    else{ //9800
                        tile_index_address = bus_read(0x9800 + ((x + (get_lcds()->scx / 8)) % 20) + (((y + (get_lcds()->scy / 8)) % 18) * 32));
                    }

        if((get_lcdc()->tile_data_select)){ //8000
             printf("%2.2X",tile_index_address);
        }
        else{ //8800
             printf("%d ",(s8)tile_index_address);
        }
        }
        printf("\n");
    }    
   
}

void fetch_bg_window_tile_address(){
    int ly = get_lcds()->ly;
    int scany = ly % 8;
    u8 tile_index_address;
        if(get_lcdc()->window_display && get_lcds()->wx <= fetcher.tile_x * 8 && get_lcds()->wy <= ly){
            //window mode
            fetcher.tile_address = 0x8000 + (4 * 16) + (scany * 2); 

            if((get_lcdc()->window_display_select)){ //9C00
                tile_index_address = bus_read(0x9C00 + ((fetcher.tile_x - (get_lcds()->wx / 8)) % 32) + (((get_lcds()->wly / 8) % 32) * 32));
            }
            else{ //9800
                tile_index_address = bus_read(0x9800 + ((fetcher.tile_x - (get_lcds()->wx / 8)) % 32) + (((get_lcds()->wly / 8) % 32) * 32));
            }

            if((get_lcdc()->tile_data_select)){ //8000
                fetcher.tile_address = 0x8000 + (tile_index_address * 16) + (scany * 2);
            }
            else{ //8800
                s8 signed_index = (s8)tile_index_address;
                fetcher.tile_address = 0x9000 + (signed_index * 16) + (scany * 2);
            }
        }
        else{ 
            if((get_lcdc()->bg_display_select)){ //9C00
                tile_index_address = bus_read(0x9C00 + ((fetcher.tile_x + (get_lcds()->scx / 8)) % 32) + (((ly / 8 + (get_lcds()->scy / 8)) % 32) * 32));
            }
            else{ //9800
                tile_index_address = bus_read(0x9800 + ((fetcher.tile_x + (get_lcds()->scx / 8)) % 32) + (((ly / 8 + (get_lcds()->scy / 8)) % 32) * 32));
            }

            if((get_lcdc()->tile_data_select)){ //8000
                fetcher.tile_address = 0x8000 + (tile_index_address * 16) + (scany * 2);
            }
            else{ //8800
                s8 signed_index = (s8)tile_index_address;
                fetcher.tile_address = 0x9000 + (signed_index * 16) + (scany * 2);
            }
        }

}

sprite* sprites_fetch(){
    u16 addr = 0xFE00;
        int ind = 0;
        while(addr <= 0xFE9F && ind < 10){
            u8 ypos = bus_read(addr++) - 16;
            u8 xpos = bus_read(addr++) - 8;     
            u8 index = bus_read(addr++);
            u8 flags = bus_read(addr++);
            //printf("ly %d",get_lcds()->ly);
            if((get_lcds()->ly) >= ypos && (get_lcds()->ly) < ypos + 8){
                if(selected_sprites[ind]){
                    free(selected_sprites[ind]);
                }
                selected_sprites[ind] = (sprite*) malloc(sizeof(sprite));
                selected_sprites[ind]->posy = ypos;
                selected_sprites[ind]->posx = xpos;
                selected_sprites[ind]->tile_index = index;
                selected_sprites[ind]->pallete = BIT(flags,4);
                selected_sprites[ind]->flipX = BIT(flags,5);
                selected_sprites[ind]->flipY = BIT(flags,6);
                selected_sprites[ind]->bg_priority = BIT(flags,7);
                ind++;
            }
        }
        while(ind < 10){
            free(selected_sprites[ind]);
            selected_sprites[ind++] = NULL;
        }
    return selected_sprites[0];
}

void pixel_fetch(){
    if(fetcher.current_step == GET_TILE){
        fetch_bg_window_tile_address();
        for(int i=0; i<10;i++){
            if(selected_sprites[i]){
                int tile_inicio = fetcher.tile_x * 8;
                int tile_fim = (fetcher.tile_x * 8) + 8;
                int sprite_inicio = selected_sprites[i]->posx;
                int sprite_fim = selected_sprites[i]->posx + 8;
                if((sprite_inicio >= tile_inicio && sprite_inicio < tile_fim) || (sprite_fim > tile_inicio && sprite_fim < tile_fim)){
                 //   printf("selected sprite tile_inicio %d tile_fim %d sprite_inicio %d sprite_fim %d\n",tile_inicio,tile_fim,sprite_inicio,sprite_fim);
                    sprite_depth[count++] = selected_sprites[i];
                    fetcher.selected = selected_sprites[i];
                }
            }
        }
        //fetcher.selected = NULL;
        fetcher.current_step = GET_TILE_LOW;
        return;
    }
    if(fetcher.current_step == GET_TILE_LOW){
        fetcher.tile_low = bus_read(fetcher.tile_address);
        for(int i=0;i<count;i++){
                int sprite_ly = (get_lcds()->ly - sprite_depth[i]->posy) % 8;
                if(sprite_ly < 0){
                    sprite_depth[i]->sprite_low = 0;
                    continue;
                }
                if(!sprite_depth[i]->flipY)
                    sprite_depth[i]->sprite_low = bus_read(0x8000 + (sprite_depth[i]->tile_index * 16) + ((sprite_ly) * 2));
                else 
                    sprite_depth[i]->sprite_low = bus_read(0x8000 + (sprite_depth[i]->tile_index * 16) + (14 - (sprite_ly)* 2));
            
            }
    
        fetcher.current_step = GET_TILE_HIGH;
        return;
    }
    if(fetcher.current_step == GET_TILE_HIGH){
        fetcher.tile_high = bus_read(fetcher.tile_address+1);

             for(int i=0;i<count;i++){
                int sprite_ly = (get_lcds()->ly - sprite_depth[i]->posy) % 8;
                             if(sprite_ly < 0){
                    sprite_depth[i]->sprite_high = 0;
                    continue;
                }
               if(!sprite_depth[i]->flipY)
                    sprite_depth[i]->sprite_high = bus_read(0x8000 + (sprite_depth[i]->tile_index * 16) + ((sprite_ly) * 2) + 1);
                else 
                    sprite_depth[i]->sprite_high = bus_read(0x8000 + (sprite_depth[i]->tile_index * 16) + (14 - (sprite_ly)* 2) + 1);
            
            }   
        fetcher.tile_x++;
        fetcher.current_step = PUSH;
        return;
    }
    if(fetcher.current_step == PUSH){
        if (bg_fifo->size < 16) { 
               // printf("lxly %d %d count sprites %d\n",(fetcher.tile_x - 1)*8, get_lcds()->ly,count);
                
            for (int bit=7; bit >= 0; bit--) {
                int palette = 2;
                u8 hi,lo,bg_color;
                if(get_lcdc()->bg_window_display){
                    hi = !!(fetcher.tile_high & (1 << bit)) << 1;
                    lo = !!(fetcher.tile_low & (1 << bit));
                    bg_color = hi | lo;                
                }
                else{
                    bg_color = 0;
                }
                u8 color = bg_color;
                
                if(count > 0 && get_lcdc()->sprite_display){
                    //we have sprites in the line block
                   // int current_lx = fetcher.tile_x * 8 + (7-bit);
                    int last_posx = 160;
                    for(int i=0;i< count;i++){
                        int bitshift = sprite_depth[i]->posx - (fetcher.tile_x-1) * 8;
                        if(bitshift < 0 && (bit+bitshift) < 0)
                            continue;
                        u8 hi1,lo1;
                        if(sprite_depth[i]->flipX ){
                            hi1 = !!(sprite_depth[i]->sprite_high & (1 << (7-(bit+bitshift)))) << 1;
                            lo1 = !!(sprite_depth[i]->sprite_low & (1 << (7-(bit+bitshift))));
                        }
                        else
                        {
                            hi1 = !!(sprite_depth[i]->sprite_high & (1 << (bit+bitshift))) << 1;
                            lo1 = !!(sprite_depth[i]->sprite_low & (1 << (bit+bitshift)));
                        }     
                        u8 temp = hi1 | lo1;
                        //for each sprite in the line block
                        if(temp > 0 && last_posx > sprite_depth[i]->posx && (!sprite_depth[i]->bg_priority || bg_color ==0)){
                            color = temp;
                            palette = sprite_depth[i]->pallete;
                            last_posx = sprite_depth[i]->posx;
                        //    printf("selected color for index %d color %d palette %d\n",i,temp,palette);
                        }
                        else{
                            //printf("NOT selected color for index %d color %d palette %d sprite_high %2.2X sprite_low %2.2X tile_index %2.2x\n",i,temp,palette,sprite_depth[i]->sprite_high, sprite_depth[i]->sprite_low,sprite_depth[i]->tile_index);
                        
                        }
                    }

                }  
                pixel_fifo_push(bg_fifo,get_color(palette,color));
            }
                             
            fetcher.selected = NULL;
            count = 0;
            fetcher.current_step = GET_TILE;
        }
    }

    
}