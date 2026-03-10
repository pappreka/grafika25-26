#ifndef UI_H
#define UI_H

#include <stdbool.h>

typedef struct UI{
    bool show_help;
    char status_text[128];
    float status_timer;
} UI;

void ui_init(UI *ui);
void ui_toggle_help(UI *ui);
void ui_set_status(UI *ui, const char *text, float seconds);
void ui_update(UI *ui, float dt);
void ui_render_help_overlay(const UI *ui, int screen_w, int screen_h);

#endif