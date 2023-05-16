#include <ui.h>
#include <bus.h>
#include "ppu.h"
#include <SDL2/SDL.h>

SDL_Window *sdlWindow;
SDL_Renderer *sdlRenderer;
SDL_Texture *sdlTexture;
SDL_Surface *screen;

SDL_Window *sdlDebugWindow;
SDL_Renderer *sdlDebugRenderer;
SDL_Texture *sdlDebugTexture;
SDL_Surface *debugScreen;

SDL_Window *sdlTilemapWindow;
SDL_Renderer *sdlTilemapRenderer;
SDL_Texture *sdlTilemapTexture;
SDL_Surface *tilemapScreen;

SDL_Window *sdlOAMWindow;
SDL_Renderer *sdlOAMRenderer;
SDL_Texture *sdlOAMTexture;
SDL_Surface *OAMScreen;


static int scale = 3;

void ui_init() {
    
    SDL_Init(SDL_INIT_VIDEO);
    
    SDL_CreateWindowAndRenderer(160 * scale, 144 * scale, 0, &sdlWindow, &sdlRenderer);
    SDL_CreateWindowAndRenderer(16 * 8 * scale, 32 * 8 * scale, 0, &sdlDebugWindow, &sdlDebugRenderer);
    SDL_CreateWindowAndRenderer(32 * 8 * scale, 32 * 8 * scale, 0, &sdlTilemapWindow, &sdlTilemapRenderer);
    SDL_CreateWindowAndRenderer(160 * scale, 144 * scale, 0, &sdlOAMWindow, &sdlOAMRenderer);
    
    screen = SDL_CreateRGBSurface(0, (160 * scale), (144 * scale), 32, 0x00FF0000,0x0000FF00,0x000000FF,0xFF000000);
    debugScreen = SDL_CreateRGBSurface(0, (16 * 8 * scale) + (16 * scale), (32 * 8 * scale) + (64 * scale), 32, 0x00FF0000,0x0000FF00,0x000000FF,0xFF000000);
    tilemapScreen = SDL_CreateRGBSurface(0, (32 * 8 * scale) + (16 * scale), (32 * 8 * scale) + (64 * scale), 32, 0x00FF0000,0x0000FF00,0x000000FF,0xFF000000);
    OAMScreen = SDL_CreateRGBSurface(0, (160 * scale), (144 * scale), 32, 0x00FF0000,0x0000FF00,0x000000FF,0xFF000000);
    
    sdlTexture = SDL_CreateTexture(sdlRenderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,(160 * scale),(144 * scale));
    sdlDebugTexture = SDL_CreateTexture(sdlDebugRenderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,(16 * 8 * scale) + (16 * scale), (32 * 8 * scale) + (64 * scale));
    sdlTilemapTexture = SDL_CreateTexture(sdlTilemapRenderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,(32 * 8 * scale) + (16 * scale), (32 * 8 * scale) + (64 * scale));
    sdlOAMTexture = SDL_CreateTexture(sdlOAMRenderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,(160 * scale),(144 * scale));
    
    int x, y;
    SDL_GetWindowPosition(sdlWindow, &x, &y);
    SDL_SetWindowPosition(sdlWindow, x - 200, y);
    SDL_SetWindowPosition(sdlDebugWindow, x + SCREEN_WIDTH + 10, y);
    SDL_GetWindowPosition(sdlDebugWindow, &x, &y);
    SDL_SetWindowPosition(sdlTilemapWindow, x + SCREEN_WIDTH + 10, y);
    SDL_SetWindowPosition(sdlOAMWindow, x + SCREEN_WIDTH + 10, y);
    
}

void delay(u32 ms) {
    SDL_Delay(ms);
}

static unsigned long tile_colors[4] = {0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000}; 

static unsigned long sprite_colors[4] = {0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0x00000000}; 


void display_tile(SDL_Surface *surface, u16 startLocation, u16 tileNum, int x, int y) {
    SDL_Rect rc;

    for (int tileY=0; tileY<16; tileY += 2) {
        u8 b1 = bus_read(startLocation + (tileNum * 16) + tileY);
        u8 b2 = bus_read(startLocation + (tileNum * 16) + tileY + 1);

        for (int bit=7; bit >= 0; bit--) {
            u8 hi = !!(b2 & (1 << bit)) << 1;
            u8 lo = !!(b1 & (1 << bit));

            u8 color = hi | lo;

            rc.x = x + ((7 - bit) * scale);
            rc.y = y + (tileY / 2 * scale);
            rc.w = scale;
            rc.h = scale;

            SDL_FillRect(surface, &rc, tile_colors[color]);
        }
    }
}

void display_sprite(SDL_Surface *surface, u16 startLocation, u16 tileNum, int x, int y, bool flipX, bool flipY) {
    SDL_Rect rc;

    for (int tileY=0; tileY<16; tileY += 2) {
        u8 b1 = bus_read(startLocation + (tileNum * 16) + tileY);
        u8 b2 = bus_read(startLocation + (tileNum * 16) + tileY + 1);

        for (int bit=7; bit >= 0; bit--) {
            u8 hi = !!(b2 & (1 << bit)) << 1;
            u8 lo = !!(b1 & (1 << bit));

            u8 color = hi | lo;

            if(flipX){
                rc.x = x + ((bit) * scale);
            }
            else{
                rc.x = x + ((7 - bit) * scale);
            }
            if(flipY){
                rc.y = y + ((7 - (tileY / 2)) * scale);
            }
            else{
                rc.y = y + (tileY / 2 * scale);
            }
            rc.w = scale;
            rc.h = scale;

            SDL_FillRect(surface, &rc, sprite_colors[color]);
        }
    }
}


void update_dbg_window() {
    int xDraw = 0;
    int yDraw = 0;
    int tileNum = 0;

    SDL_Rect rc;
    rc.x = 0;
    rc.y = 0;
    rc.w = debugScreen->w;
    rc.h = debugScreen->h;
    SDL_FillRect(debugScreen, &rc, 0xFF111111);

    u16 addr = 0x8000;

    //384 tiles, 24 x 16
    for (int y=0; y<24; y++) {
        for (int x=0; x<16; x++) {
            display_tile(debugScreen, addr, tileNum, xDraw + (x * scale), yDraw + (y * scale));
            xDraw += (8 * scale);
            tileNum++;
        }

        yDraw += (8 * scale);
        xDraw = 0;
    }

	SDL_UpdateTexture(sdlDebugTexture, NULL, debugScreen->pixels, debugScreen->pitch);
	SDL_RenderClear(sdlDebugRenderer);
	SDL_RenderCopy(sdlDebugRenderer, sdlDebugTexture, NULL, NULL);
	SDL_RenderPresent(sdlDebugRenderer);
}

void update_tilemap_window() {
    int xDraw = 0;
    int yDraw = 0;
    
    SDL_Rect rc;
    rc.x = 0;
    rc.y = 0;
    rc.w = tilemapScreen->w;
    rc.h = tilemapScreen->h;
    SDL_FillRect(tilemapScreen, &rc, 0xFF111111);

    u16 addr = 0x9800;

    //32 x 32 tiles, each tile contains
    for (int y=0; y<32; y++) {
        for (int x=0; x<32; x++) {
            u8 index = bus_read(addr++);
            display_tile(tilemapScreen, 0x8000, index , xDraw + (x * scale), yDraw + (y * scale));
            xDraw += (8 * scale) -2;
        }

        yDraw += (8 * scale) -2;
        xDraw = 0;
    }

	SDL_UpdateTexture(sdlTilemapTexture, NULL, tilemapScreen->pixels, tilemapScreen->pitch);
	SDL_RenderClear(sdlTilemapRenderer);
	SDL_RenderCopy(sdlTilemapRenderer, sdlTilemapTexture, NULL, NULL);
	SDL_RenderPresent(sdlTilemapRenderer);

}

void update_oam_window() {
    SDL_Rect rc;
    rc.x = 0;
    rc.y = 0;
    rc.w = OAMScreen->w;
    rc.h = OAMScreen->h;
    SDL_FillRect(OAMScreen, &rc, 0xFF111111);

    u16 addr = 0xFE00;

    while(addr <= 0xFE9F){
        u8 ypos = bus_read(addr++) - 17;
        u8 xpos = bus_read(addr++) - 8;     
        u8 index = bus_read(addr++);
        u8 flags = bus_read(addr++);
        display_sprite(OAMScreen, 0x8000, index , xpos * scale, ypos * scale,BIT(flags,5),BIT(flags,6));
    }

	SDL_UpdateTexture(sdlOAMTexture, NULL, OAMScreen->pixels, OAMScreen->pitch);
	SDL_RenderClear(sdlOAMRenderer);
	SDL_RenderCopy(sdlOAMRenderer, sdlOAMTexture, NULL, NULL);
	SDL_RenderPresent(sdlOAMRenderer);

}



void ui_update() {

    u32 *video_buffer = get_video_buffer();

    for (int line_num = 0; line_num < 144; line_num++) {
        for (int x = 0; x < 160; x++) {
            SDL_Rect rc;
            rc.x = x * scale;
            rc.y = line_num * scale;
            rc.w = scale;
            rc.h = scale;

            SDL_FillRect(screen, &rc, video_buffer[(x) + ((line_num) * 160)]);
            //SDL_FillRect(screen, &rc, 0xFF555555);
        }
    }
   
    SDL_UpdateTexture(sdlTexture, NULL, screen->pixels, screen->pitch);
    SDL_RenderClear(sdlRenderer);
    SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL);
    SDL_RenderPresent(sdlRenderer);
 
    update_dbg_window();
    update_tilemap_window();
    update_oam_window();
}

void ui_handle_events() {
    SDL_Event e;
    while (SDL_PollEvent(&e) > 0)
    {
        //TODO SDL_UpdateWindowSurface(sdlWindow);
        //TODO SDL_UpdateWindowSurface(sdlTraceWindow);
        //TODO SDL_UpdateWindowSurface(sdlDebugWindow);

        if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE) {
         
        }
    }
}