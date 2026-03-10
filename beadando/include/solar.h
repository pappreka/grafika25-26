#ifndef SOLAR_H
#define SOLAR_H

#include <stdbool.h>
#include "mesh.h"
#include "texture.h"
#include "math3d.h"

typedef struct Planet{
    const char *name;

    float radius;
    float orbit_radius;
    float orbit_speed;
    float rotation_speed;
    float orbit_angle;
    float rotation_angle;

    Vec3 base_position;
    Vec3 position;

    Texture2D texture;

    bool has_ring;
    float ring_inner_radius;
    float ring_outer_radius;
    float ring_tilt_deg;
    Texture2D ring_texture;
} Planet;

typedef struct SolarSystem{
    Mesh *shared_sphere;
    Planet planets[9];
    int planet_count;

    int selection_order[9];
    int selected_order_pos;
    float selected_forward_offset;
} SolarSystem;

bool solar_init(SolarSystem *s, Mesh *shared_sphere);
void solar_shutdown(SolarSystem *s);
void solar_update(SolarSystem *s, float dt);
Planet* solar_get(SolarSystem *s, int index);
int solar_count(const SolarSystem *s);

void solar_select_next(SolarSystem *s);
void solar_select_prev(SolarSystem *s);
int solar_selected_index(const SolarSystem *s);

#endif