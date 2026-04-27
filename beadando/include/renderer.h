#ifndef RENDERER_H
#define RENDERER_H

#include "camera.h"
#include <stdbool.h>

// Renderer állapot (ablak méret)
typedef struct Renderer {
    int width;
    int height;
} Renderer;

// Inicializálás
bool renderer_init(Renderer *r, int width, int height);

// Ablak átméretezés kezelése
void renderer_resize(Renderer *r, int width, int height);

// Frame kezdete (háttér törlés, szín beállítás)
void renderer_begin_frame(Renderer *r, bool surface_mode);

// 3D nézet beállítása a kamera alapján
void renderer_set_3d(Renderer *r, const Camera *cam);

// Segédrács és tengelyek kirajzolása
void renderer_draw_world_axes_and_grid(void);

// Frame vége
void renderer_end_frame(Renderer *r);

#endif