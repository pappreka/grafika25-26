#ifndef APP_H
#define APP_H

#include "camera.h"
#include "input.h"
#include "renderer.h"
#include "ui.h"
#include "mesh.h"
#include "texture.h"
#include "solar.h"
#include "planet_scene.h"

#include <stdbool.h>
#include <SDL2/SDL.h>

// Az alkalmazás teljes állapotát tároló struktúra
typedef struct App{

    // Ablak és OpenGL
    SDL_Window *window;
    SDL_GLContext gl_ctx;
    bool running;
    int win_w;
    int win_h;

    // Kamera
    Camera camera;
    Vec3 fixed_camera_position;

    // Leszállás és felszíni mód
    bool landing_in_progress;
    bool surface_mode;
    float surface_speed;
    int landed_planet_index;

    Vec3 landing_start_position;
    Vec3 landing_target_position;
    Vec3 landing_surface_normal;

    // Leszállási animáció aktuális állapota
    float landing_t;
    // Leszállási animáció teljes időtartama
    float landing_duration;

    // Input, render, UI
    Input input;
    Renderer renderer;
    UI ui;

    // 3D objektumok és világ
    Mesh sphere_mesh;
    Texture2D stars_texture;
    SolarSystem solar;
    PlanetScene planet_scene;

    // Időkezelés
    unsigned int last_ticks;

} App;

// Inicializálás
bool app_init(App *app, const char *title, int w, int h);

// Felszabadítás
void app_shutdown(App *app);

// Fő ciklus
void app_run(App *app);

#endif