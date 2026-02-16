#ifndef APP_H
#define APP_H

#include "camera.h"
#include "input.h"
#include "renderer.h"
#include "ui.h"
#include "mesh.h"
#include "texture.h"
#include "solar.h"
#include "math3d.h"

#include <stdbool.h>
#include <SDL2/SDL.h>

typedef struct App{
    SDL_Window *window;
    SDL_GLContext gl_ctx;
    bool running;
    int win_w;
    int win_h;
    Camera camera;
    Input input;
    Renderer renderer;
    UI ui;
    float ground_y;
    float eye_height;
    float vertical_velocity;
    bool on_ground;
    Mesh sphere_mesh;
    SolarSystem solar;
    bool visiting;
    Vec3 visit_target_pos;
    bool lighting_enabled;
    float light_intensity;
    unsigned int last_ticks;
} App;

bool app_init(App *app, const char *title, int w, int h);
void app_shutdown(App *app);
void app_run(App *app);

#endif