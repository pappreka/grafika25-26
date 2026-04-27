#ifndef PLANET_SCENE_H
#define PLANET_SCENE_H

#include <stdbool.h>
#include "math3d.h"
#include "camera.h"
#include "input.h"
#include "solar.h"

// Felszíni objektum típusok
typedef enum SurfaceObjectType{
    SURFACE_OBJECT_ROCK = 0,
    SURFACE_OBJECT_CRATE = 1,
    SURFACE_OBJECT_ANTENNA = 2,
    SURFACE_OBJECT_MODULE = 3,
    SURFACE_OBJECT_CRYSTAL = 4,
    SURFACE_OBJECT_TREE = 5,
    SURFACE_OBJECT_CABIN = 6,
    SURFACE_OBJECT_SHIP = 7,
    SURFACE_OBJECT_RIVER_ROCK = 8,
    SURFACE_OBJECT_LOG = 9,
    SURFACE_OBJECT_ROVER = 10
} SurfaceObjectType;

// Egy objektum a felszínen
typedef struct SurfaceObject{
    SurfaceObjectType type;
    Vec3 position;
    Vec3 half_extents; // Ütközési méret

    bool interactive;
    bool blocks_movement; // Ütközés blokkolja a mozgást
    bool active;

    float state;
    float yaw_deg;
    float model_scale;

    const char *label;
} SurfaceObject;

// Eldobott kő adatai
typedef struct ThrownStone{
    bool active;
    Vec3 position;
    Vec3 velocity;
    float life;
    bool splash_done;
} ThrownStone;

// Víz hullám effekt
typedef struct WaterRipple{
    bool active;
    Vec3 position;
    float age;
    float life;
    float amplitude;
    float radius;
    float speed;
} WaterRipple;

// Por/részecske effekt
typedef struct DustParticle{
    bool active;
    Vec3 position;
    Vec3 velocity;
    float age;
    float life;
    float size;
    float alpha;
} DustParticle;

// Teljes bolygófelszíni jelenet
typedef struct PlanetScene{
    bool active;
    bool exit_requested;
    bool splash_flash;

    int planet_index;
    int highlighted_object;
    char planet_name[32];

    // Mozgás és fizika
    float terrain_extent;
    float eye_height;
    float move_speed;
    float run_multiplier;

    float jump_velocity;
    float gravity;
    float vertical_velocity;
    bool grounded;

    // Felszín színe
    float ground_r;
    float ground_g;
    float ground_b;

    // Spawn pozíció
    Vec3 spawn_position;
    float spawn_yaw_deg;
    float spawn_pitch_deg;

    // Objektumok
    int object_count;
    SurfaceObject objects[160];

    // Folyó adatok
    bool has_river;
    float river_x;
    float river_half_width;
    float river_z_min;
    float river_z_max;

    // Kövek és víz effektek
    int stone_count;
    int stones_available;
    ThrownStone stones[24];
    WaterRipple ripples[16];

    // Por részecskék
    DustParticle dust_particles[48];

    // Vénusz speciális effektek
    float venus_heat;
    float venus_heat_timer;
    float venus_cloud_time;

    char interaction_message[128];
} PlanetScene;

// Inicializálás
void planet_scene_init(PlanetScene *scene);

// Jelenet felépítése adott bolygóra
void planet_scene_build(PlanetScene *scene, const Planet *planet, int planet_index);

// Frissítés (input + fizika)
void planet_scene_update(PlanetScene *scene, Camera *camera, const Input *input, float dt);

// Kirajzolás
void planet_scene_render(const PlanetScene *scene);

// Interakciók
const char *planet_scene_interact(PlanetScene *scene, Vec3 camera_position);
const char *planet_scene_handle_click(PlanetScene *scene, Vec3 camera_position, Vec3 camera_forward);
const char *planet_scene_throw_stone(PlanetScene *scene, Vec3 camera_position, Vec3 camera_forward);

// Kilépés ellenőrzése
bool planet_scene_should_exit(const PlanetScene *scene);

// UI prompt lekérdezése
const char *planet_scene_get_prompt(const PlanetScene *scene, Vec3 camera_position, Vec3 camera_forward);

// Render kamera alapján
void planet_scene_render_with_camera(const PlanetScene *scene, const Camera *camera);

#endif