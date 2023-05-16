#include "common.h"

typedef struct {
    bool button_pressed;
    bool action_buttons;
    bool direction_buttons;
    bool up;
    bool down;
    bool left;
    bool right;
    bool a;
    bool b;
    bool start;
    bool select;
} joypad_state;

void joypad_init();

void joypad_press(char a);

void joypad_release(char a);

void joypad_write(u8 value);

u8 joypad_read();
