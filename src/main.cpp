#include <iostream>
#include <SDL2/SDL.h>
#include "cpu.h"
#include "cartridge.h"
#include "ppu.h"
#include "ui.h"
#include "dbg.h"
#include "io.h"
#include "dma.h"
#include "interrupt.h"
#include "timer.h"
#include "joypad.h"
#include "fifo.h"
#include "ppu_pipeline.h"
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <thread>

void *cpu_run(void *p) {
    while(true) {
        interrupt_check();
        dma_tick();
        timer_tick();
        ppu_tick();
        cpu_setdelay(cpu_tick());
        dbg_update();
        //usleep(1);
    }
    return 0;
}

int main(int argc, char **argv) {

    if(!cartridge_load("/Users/igorantunes/repo/GBEmu/roms/tetris.gb")){
        return EXIT_FAILURE;
    }
    cpu_init();
    ppu_init();
    ui_init();
    interrupt_init();
    timer_init();
    
    pthread_t t1;
    if (pthread_create(&t1, NULL, cpu_run, NULL)) {
        fprintf(stderr, "Failed to start CPU thread\n");
        return EXIT_FAILURE;
    }

    SDL_Event e; 
    bool quit = false;
    while(quit == false){
        ui_update();
        while(SDL_PollEvent( &e ))
            { 
                if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE) {
                        quit = true;
                        cpu_printregs(); 
                        bus_dump();
                        tilemap_index_dump();
                        buffer_dump();
                }
                if( e.type == SDL_QUIT ) 
                    quit = true;               
                if( e.type == SDL_KEYDOWN){
                   // exit(1);
                    joypad_press(e.key.keysym.sym);
                }
                if( e.type == SDL_KEYUP){
                    joypad_release(e.key.keysym.sym);
                }
            } 
    }
    return EXIT_SUCCESS;
}
