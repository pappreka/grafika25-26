#ifndef UI_H
#define UI_H

#include <stdbool.h>

typedef struct UI{
    bool show_help;
} UI;

void ui_init(UI *ui);
void ui_toggle_help(UI *ui);
void ui_render_help_overlay(const UI *ui, int screen_w, int screen_h);

#endif