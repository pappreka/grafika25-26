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
    Vec3 position;
    Texture2D texture;
} Planet;

typedef struct SolarSystem{
    Mesh *shared_sphere;
    Planet planets[9];
    int planet_count;
} SolarSystem;

bool solar_init(SolarSystem *s, Mesh *shared_sphere);
void solar_shutdown(SolarSystem *s);
void solar_update(SolarSystem *s, float dt);
Planet* solar_get(SolarSystem *s, int index);
int solar_count(const SolarSystem *s);

#endif