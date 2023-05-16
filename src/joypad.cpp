#include "joypad.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include "interrupt.h"

joypad_state joypad;

void joypad_init(){

}

void joypad_press(char a){
    switch(a){
        case 82: //upkey
            joypad.up = true;
            break;
        case 81: //downkey
            joypad.down = true;
            break;
        case 80: //leftkey
            joypad.left = true;
            break;
        case 79: //rightkey
            joypad.right = true;
            break;
        case 97: //a
            joypad.a = true;
            break;
        case 115: //s
            joypad.b = true;
            break;
        case 122: //z
            joypad.start = true;
            break;
        case 120: //x
            joypad.select = true;
            break;
        default:
            printf("default");
            return;
    }

    interrupt_request(0b10000);
}

void joypad_release(char a){
    switch(a){
        case 82: //upkey
            joypad.up = false;
            break;
        case 81: //downkey
            joypad.down = false;
            break;
        case 80: //leftkey
            joypad.left = false;
            break;
        case 79: //rightkey
            joypad.right = false;
            break;
        case 97: //a
            joypad.a = false;
            break;
        case 115: //s
            joypad.b = false;
            break;
        case 122: //z
            joypad.start = false;
            break;
        case 120: //x
            joypad.select = false;
            break;
        default:
            return;
    }
}
/*
Bit 7 - Not used
Bit 6 - Not used
Bit 5 - P15 Select Action buttons    (0=Select)
Bit 4 - P14 Select Direction buttons (0=Select)
Bit 3 - P13 Input: Down  or Start    (0=Pressed) (Read Only)
Bit 2 - P12 Input: Up    or Select   (0=Pressed) (Read Only)
Bit 1 - P11 Input: Left  or B        (0=Pressed) (Read Only)
Bit 0 - P10 Input: Right or A        (0=Pressed) (Read Only)*/
u8 joypad_read(){
    u8 val = 0b1111;
    if(joypad.action_buttons){
        if(joypad.a)
            val &= 0b1110;
        if(joypad.b)
            val &= 0b1101;
        if(joypad.select)
            val &= 0b1011;
        if(joypad.start)
            val &= 0b0111;
    }
    else if(joypad.direction_buttons){
        if(joypad.right)
            val &= 0b1110;
        if(joypad.left)
            val &= 0b1101;
        if(joypad.up)
            val &= 0b1011;
        if(joypad.down)
            val &= 0b0111;
    
    }
        return val;
    
}

void joypad_write(u8 value){
    joypad.action_buttons = (BIT(value,5)) == 0 ? true : false;
    joypad.direction_buttons = (BIT(value,4)) == 0 ? true : false; 
 }