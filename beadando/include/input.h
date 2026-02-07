#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>
#include <SDL2/SDL.h>

typedef struct Input{
    bool keys[SDL_NUM_SCANCODES];
    bool keys_pressed[SDL_NUM_SCANCODES];

    int mouse_dx;
    int mouse_dy;
    int mouse_wheel;
    bool quit_requested;
}Input;

void input_begin_frame(Input *in);
void input_process_event(Input *in, const SDL_Event *e);
bool input_key_down(const Input *in, SDL_Scancode sc);
bool input_key_pressed(const Input *in, SDL_Scancode sc);

#endif