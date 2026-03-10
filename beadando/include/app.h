#ifndef APP_H
#define APP_H

#include "camera.h"
#include "input.h"
#include "renderer.h"
#include "ui.h"
#include "mesh.h"
#include "texture.h"
#include "solar.h"

#include <stdbool.h>
#include <SDL2/SDL.h>

typedef struct App{
    SDL_Window *window;
    SDL_GLContext gl_ctx;
    bool running;
    int win_w;
    int win_h;

    Camera camera;
    Vec3 fixed_camera_position;

    Input input;
    Renderer renderer;
    UI ui;

    Mesh sphere_mesh;
    Texture2D stars_texture;
    SolarSystem solar;

    unsigned int last_ticks;
} App;

bool app_init(App *app, const char *title, int w, int h);
void app_shutdown(App *app);
void app_run(App *app);

#endif