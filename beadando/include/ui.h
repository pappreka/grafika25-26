#ifndef UI_H
#define UI_H

#include <stdbool.h>

// Felhasználói felület állapota
typedef struct UI{
    bool show_help;
    char status_text[128];
    float status_timer;
} UI;

// Inicializálás
void ui_init(UI *ui);

// Súgó ki/be kapcsolása
void ui_toggle_help(UI *ui);

// Állapotüzenet beállítása
void ui_set_status(UI *ui, const char *text, float seconds);

// Frissítés (időzítés)
void ui_update(UI *ui, float dt);

// Súgó overlay kirajzolása
void ui_render_help_overlay(const UI *ui, int screen_w, int screen_h);

#endif