#include "input.h"
#include <string.h>

void input_begin_frame (Input *in){
    memset(in -> keys_pressed, 0, sizeof(in -> keys_pressed));
    in -> mouse_dx = 0;
    in -> mouse_dy = 0;
    in -> mouse_wheel = 0;
    in -> quit_requested = false;
}

void input_process_event(Input *in, const SDL_Event *e){
    switch (e -> type){
        case SDL_QUIT:
            in -> quit_requested = true;
            break;
        case SDL_KEYDOWN:
            if(!e -> key.repeat){
                in -> keys[e -> key.keysym.scancode] = true;
                in -> keys_pressed[e -> key.keysym.scancode] = true;
            }
            break;
        case SDL_KEYUP:
            in -> keys[e -> key.keysym.scancode] = false;
            break;
        case SDL_MOUSEMOTION:
            in -> mouse_dx += e -> motion.xrel;
            in -> mouse_dy += e -> motion.yrel;
            break;
        case SDL_MOUSEWHEEL:
            in -> mouse_wheel += e -> wheel.y;
            break;
        default:
            break;
    }
}

bool input_key_down(const Input *in, SDL_Scancode sc){
    return in -> keys[sc];
}

bool input_key_pressed(const Input *in, SDL_Scancode sc){
    return in -> keys_pressed[sc];
}