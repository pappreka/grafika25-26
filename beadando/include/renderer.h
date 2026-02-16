#ifndef RENDERER_H
#define RENDERER_H

#include "camera.h"
#include "math3d.h"
#include <stdbool.h>

typedef struct Renderer{
    int width;
    int height;
} Renderer;

bool renderer_init(Renderer *r, int width, int height);
void renderer_resize(Renderer *r, int width, int height);
void renderer_begin_frame(Renderer *r);
void renderer_set_3d(Renderer *r, const Camera *cam);
void renderer_draw_world_axes_and_grid(void);
void renderer_end_frame(Renderer *r);
void renderer_set_lighting_enabled(bool enabled);
void renderer_update_sun_light(Vec3 sun_pos, float intensity);

#endif