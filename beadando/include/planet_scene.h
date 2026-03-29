#ifndef PLANET_SCENE_H
#define PLANET_SCENE_H

#include <stdbool.h>
#include "math3d.h"
#include "camera.h"
#include "input.h"
#include "solar.h"

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

typedef struct SurfaceObject{
    SurfaceObjectType type;
    Vec3 position;
    Vec3 half_extents;
    bool interactive;
    bool blocks_movement;
    bool active;
    float state;
    float yaw_deg;
    float model_scale;
    const char *label;
} SurfaceObject;

typedef struct ThrownStone{
    bool active;
    Vec3 position;
    Vec3 velocity;
    float life;
    bool splash_done;
} ThrownStone;

typedef struct WaterRipple{
    bool active;
    Vec3 position;
    float age;
    float life;
    float amplitude;
    float radius;
    float speed;
} WaterRipple;

typedef struct DustParticle{
    bool active;
    Vec3 position;
    Vec3 velocity;
    float age;
    float life;
    float size;
    float alpha;
} DustParticle;

typedef struct PlanetScene{
    bool active;
    bool exit_requested;
    bool splash_flash;
    int planet_index;
    int highlighted_object;
    char planet_name[32];

    float terrain_extent;
    float eye_height;
    float move_speed;
    float run_multiplier;

    float jump_velocity;
    float gravity;
    float vertical_velocity;
    bool grounded;

    float ground_r;
    float ground_g;
    float ground_b;

    Vec3 spawn_position;
    float spawn_yaw_deg;
    float spawn_pitch_deg;

    int object_count;
    SurfaceObject objects[160];

    bool has_river;
    float river_x;
    float river_half_width;
    float river_z_min;
    float river_z_max;

    int stone_count;
    int stones_available;
    ThrownStone stones[24];
    WaterRipple ripples[16];

    DustParticle dust_particles[48];

    float venus_heat;
    float venus_heat_timer;
    float venus_cloud_time;
    //float venus_particle_timer;

    char interaction_message[128];
} PlanetScene;

void planet_scene_init(PlanetScene *scene);
void planet_scene_build(PlanetScene *scene, const Planet *planet, int planet_index);
void planet_scene_update(PlanetScene *scene, Camera *camera, const Input *input, float dt);
void planet_scene_render(const PlanetScene *scene);
const char *planet_scene_interact(PlanetScene *scene, Vec3 camera_position);
const char *planet_scene_handle_click(PlanetScene *scene, Vec3 camera_position, Vec3 camera_forward);
const char *planet_scene_throw_stone(PlanetScene *scene, Vec3 camera_position, Vec3 camera_forward);
bool planet_scene_should_exit(const PlanetScene *scene);
const char *planet_scene_get_prompt(const PlanetScene *scene, Vec3 camera_position, Vec3 camera_forward);
void planet_scene_render_with_camera(const PlanetScene *scene, const Camera *camera);

#endif