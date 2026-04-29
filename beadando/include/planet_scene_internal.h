#ifndef PLANET_SCENE_INTERNAL_H
#define PLANET_SCENE_INTERNAL_H

#include "planet_scene.h"
#include "mesh.h"
#include "texture.h"
#include <stdbool.h>

#if defined(_WIN32)
#include <windows.h>
#endif
#include <GL/gl.h>

// Egy betöltött felszíni asset adatai
typedef struct SurfaceAsset{
    Mesh mesh;
    Texture2D texture;
    bool loaded;
    bool texture_loaded;
    GLuint list_id;
} SurfaceAsset;

// A jelenetben használt összes modell és textúra
typedef struct SurfaceAssets{
    bool tried;
    SurfaceAsset ship;
    SurfaceAsset river_rock;
    SurfaceAsset wild_rock;
    SurfaceAsset throw_rock;
    SurfaceAsset tree;
    SurfaceAsset rover;
} SurfaceAssets;

extern float g_light_intensity;
extern float g_river_time;
extern Vec3 g_cam_right;
extern Vec3 g_cam_up;
extern SurfaceAssets g_assets;

void ensure_assets_loaded(void);
void draw_asset(const SurfaceAsset *asset, Vec3 position, float yaw_deg, float scale);
void draw_rover_asset(const SurfaceAsset *asset, Vec3 position, float yaw_deg, float scale, float time_sec);

void setup_earth_surface_lighting(void);
void teardown_earth_surface_lighting(void);
void setup_mars_fog(void);
void teardown_mars_fog(void);
void setup_venus_fog(void);
void teardown_venus_fog(void);

float mountain_ring_inner_radius(const PlanetScene *scene);
float mountain_collision_radius(const PlanetScene *scene);

float terrain_height(const PlanetScene *scene, float x, float z);
Vec3 terrain_normal(const PlanetScene *scene, float x, float z);
void draw_mountain_ring(const PlanetScene *scene);
void draw_terrain(const PlanetScene *scene);
float venus_lava_mask(float x, float z);

float river_center_x(const PlanetScene *scene, float z);
float river_surface_height(const PlanetScene *scene, float x, float z);
bool point_in_river(const PlanetScene *scene, float x, float z);
float lake_blend(const PlanetScene *scene, float z);
void draw_river(const PlanetScene *scene);
void draw_river_splashes(const PlanetScene *scene);
void draw_thrown_stones(const PlanetScene *scene);

void draw_box(Vec3 center, Vec3 half);
void add_object(PlanetScene *scene,
                SurfaceObjectType type,
                float x,
                float z,
                float hx,
                float hy,
                float hz,
                bool interactive,
                bool blocks_movement,
                const char *label);
SurfaceObject *last_object(PlanetScene *scene);
int find_interactive_index(const PlanetScene *scene, Vec3 pos, Vec3 forward, float max_dist);
bool collides_with_object(const SurfaceObject *o, Vec3 pos, float radius);
void draw_objects(const PlanetScene *scene);

ThrownStone *alloc_stone(PlanetScene *scene);
void spawn_ripple(PlanetScene *scene, Vec3 pos, float strength);

void spawn_dust_burst(PlanetScene *scene, Vec3 origin, Vec3 move_dir, int count, float strength);
void update_dust_particles(PlanetScene *scene, float dt);
void update_venus_particles(PlanetScene *scene, float dt);
void draw_dust_particles(const PlanetScene *scene, Vec3 camera_right, Vec3 camera_up);
void draw_venus_cloud_layer(const PlanetScene *scene);
void draw_venus_lava_cracks(const PlanetScene *scene);

#endif