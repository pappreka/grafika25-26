#include "input.h"
#include <string.h>

// Frame elején lenullázzuk az egyszeri inputokat
void input_begin_frame(Input *in){
    memset(in->keys_pressed, 0, sizeof(in->keys_pressed));
    memset(in->mouse_buttons_pressed, 0, sizeof(in->mouse_buttons_pressed));

    in->mouse_dx = 0;
    in->mouse_dy = 0;
    in->mouse_wheel = 0;
    in->quit_requested = false;
}

// SDL események feldolgozása
void input_process_event(Input *in, const SDL_Event *e){
    switch(e->type){

        // Ablak bezárás
        case SDL_QUIT:
            in->quit_requested = true;
            break;

        // Billentyű lenyomás
        case SDL_KEYDOWN:
            if(!e->key.repeat){ // ne számoljuk a folyamatos ismétlést
                in->keys[e->key.keysym.scancode] = true;
                in->keys_pressed[e->key.keysym.scancode] = true;
            }
            break;

        // Billentyű felengedés
        case SDL_KEYUP:
            in->keys[e->key.keysym.scancode] = false;
            break;

        // Egér mozgás
        case SDL_MOUSEMOTION:
            in->mouse_dx += e->motion.xrel;
            in->mouse_dy += e->motion.yrel;
            break;

        // Egérgomb lenyomás
        case SDL_MOUSEBUTTONDOWN:
            if(e->button.button < (int)(sizeof(in->mouse_buttons) / sizeof(in->mouse_buttons[0]))){
                in->mouse_buttons[e->button.button] = true;
                in->mouse_buttons_pressed[e->button.button] = true;
            }
            break;

        // Egérgomb felengedés
        case SDL_MOUSEBUTTONUP:
            if(e->button.button < (int)(sizeof(in->mouse_buttons) / sizeof(in->mouse_buttons[0]))){
                in->mouse_buttons[e->button.button] = false;
            }
            break;

        // Egérgörgő
        case SDL_MOUSEWHEEL:
            in->mouse_wheel += e->wheel.y;
            break;

        default:
            break;
    }
}

// Billentyű lenyomva van-e (folyamatos)
bool input_key_down(const Input *in, SDL_Scancode sc){
    return in->keys[sc];
}

// Billentyű most lett lenyomva (egy frame)
bool input_key_pressed(const Input *in, SDL_Scancode sc){
    return in->keys_pressed[sc];
}

// Egérgomb lenyomva van-e
bool input_mouse_down(const Input *in, int button){
    if(button < 0 || button >= (int)(sizeof(in->mouse_buttons) / sizeof(in->mouse_buttons[0]))){
        return false;
    }
    return in->mouse_buttons[button];
}

// Egérgomb most lett lenyomva (egy frame)
bool input_mouse_pressed(const Input *in, int button){
    if(button < 0 || button >= (int)(sizeof(in->mouse_buttons_pressed) / sizeof(in->mouse_buttons_pressed[0]))){
        return false;
    }
    return in->mouse_buttons_pressed[button];
}