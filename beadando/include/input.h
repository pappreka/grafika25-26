#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>
#include <SDL2/SDL.h>

// Felhasználói input állapotának tárolása
typedef struct Input{
    bool keys[SDL_NUM_SCANCODES]; // Billentyűk lenyomva tartva
    bool keys_pressed[SDL_NUM_SCANCODES]; // Billentyűk egy frame-ben lenyomva

    bool mouse_buttons[8]; // Egérgombok lenyomva tartva
    bool mouse_buttons_pressed[8]; // Egérgombok egy frame-ben lenyomva

    int mouse_dx; // Egér elmozdulás X irányban
    int mouse_dy; // Egér elmozdulás Y irányban
    int mouse_wheel; // Görgő elmozdulás

    bool quit_requested; // Kilépési kérés (ablak bezárás)
} Input;

// Frame elején nullázza az egyszeri inputokat
void input_begin_frame(Input *in);

// SDL események feldolgozása
void input_process_event(Input *in, const SDL_Event *e);

// Billentyű állapot lekérdezése
bool input_key_down(const Input *in, SDL_Scancode sc);

// Billentyű lenyomás lekérdezése
bool input_key_pressed(const Input *in, SDL_Scancode sc);

// Egérgomb állapot lekérdezése
bool input_mouse_down(const Input *in, int button);

// Egérgomb lenyomás lekérdezése
bool input_mouse_pressed(const Input *in, int button);

#endif