#ifndef SOLAR_H
#define SOLAR_H

#include <stdbool.h>
#include "mesh.h"
#include "texture.h"
#include "math3d.h"

// Egy bolygó adatai
typedef struct Planet{
    const char *name;

    // Méret és mozgás
    float radius;
    float orbit_radius;
    float orbit_speed;
    float rotation_speed;
    float orbit_angle;
    float rotation_angle;

    // Pozíció
    Vec3 base_position;
    Vec3 position;

    // Textúra
    Texture2D texture;

    // Gyűrű
    bool has_ring;
    float ring_inner_radius;
    float ring_outer_radius;
    float ring_tilt_deg;
    Texture2D ring_texture;

    bool landable; // Le lehet-e szállni rá
} Planet;

// Teljes Naprendszer
typedef struct SolarSystem{
    Mesh *shared_sphere;   // Közös gömb mesh a bolygókhoz

    Planet planets[9];
    int planet_count;

    // Kiválasztási sorrend (kamera fókusz)
    int selection_order[9];
    int selected_order_pos;
    float selected_forward_offset;
} SolarSystem;

// Inicializálás
bool solar_init(SolarSystem *s, Mesh *shared_sphere);

// Erőforrások felszabadítása
void solar_shutdown(SolarSystem *s);

// Bolygók frissítése
void solar_update(SolarSystem *s, float dt);

// Bolygó lekérése index alapján
Planet* solar_get(SolarSystem *s, int index);

// Bolygók száma
int solar_count(const SolarSystem *s);

// Kiválasztás váltása
void solar_select_next(SolarSystem *s);
void solar_select_prev(SolarSystem *s);
int solar_selected_index(const SolarSystem *s);

#endif