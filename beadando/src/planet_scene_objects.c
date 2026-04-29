#include "planet_scene.h"
#include "planet_scene_internal.h"
#include "mesh.h"
#include "texture.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#if defined(_WIN32)
#include <windows.h>
#endif

#include <GL/gl.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define COLLISION_SCALE_XZ 0.4f
#define COLLISION_SCALE_Y  0.6f

// Egyszerű doboz rajzolása placeholder objektumokhoz
void draw_box(Vec3 center, Vec3 half)
{
    // Doboz végpontjai
    float x0 = center.x - half.x;
    float x1 = center.x + half.x;
    float y0 = center.y - half.y;
    float y1 = center.y + half.y;
    float z0 = center.z - half.z;
    float z1 = center.z + half.z;

    glDisable(GL_TEXTURE_2D);
    glColor3f(0.65f, 0.65f, 0.65f);

    glBegin(GL_QUADS);

    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(x0,y0,z1); glVertex3f(x1,y0,z1); glVertex3f(x1,y1,z1); glVertex3f(x0,y1,z1);

    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(x1,y0,z0); glVertex3f(x0,y0,z0); glVertex3f(x0,y1,z0); glVertex3f(x1,y1,z0);

    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(x0,y0,z0); glVertex3f(x0,y0,z1); glVertex3f(x0,y1,z1); glVertex3f(x0,y1,z0);

    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(x1,y0,z1); glVertex3f(x1,y0,z0); glVertex3f(x1,y1,z0); glVertex3f(x1,y1,z1);

    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(x0,y1,z1); glVertex3f(x1,y1,z1); glVertex3f(x1,y1,z0); glVertex3f(x0,y1,z0);

    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(x0,y0,z0); glVertex3f(x1,y0,z0); glVertex3f(x1,y0,z1); glVertex3f(x0,y0,z1);

    glEnd();
}

// Űrhajóhoz speciális bounding box
void get_ship_bbox(const SurfaceObject *o, Vec3 *center, Vec3 *half)
{
    float bottom;
    float top;

    half->x = o->half_extents.x * 0.22f;
    half->z = o->half_extents.z * 0.22f;

    bottom = o->half_extents.y * 0.95f;
    top    = o->half_extents.y * 0.35f;

    half->y = (top + bottom) * 0.5f;

    center->x = o->position.x;
    center->z = o->position.z;
    center->y = o->position.y - (bottom - half->y);
}

// Objektumonként eltérő bbox méret
Vec3 object_bbox_half(const SurfaceObject *o)
{
    switch(o->type){
        case SURFACE_OBJECT_RIVER_ROCK:
            return vec3(
                o->half_extents.x * 0.16f,
                o->half_extents.y * 0.10f,
                o->half_extents.z * 0.16f
            );

        case SURFACE_OBJECT_ROCK:
            return vec3(
                o->half_extents.x * 2.5f,
                o->half_extents.y * 2.0f,
                o->half_extents.z * 2.5f
            );

        case SURFACE_OBJECT_TREE:
            return vec3(
                o->half_extents.x * 0.30f,
                o->half_extents.y * 0.55f,
                o->half_extents.z * 0.30f
            );

        case SURFACE_OBJECT_SHIP:
            return vec3(
                o->half_extents.x * 0.22f,
                o->half_extents.y * 0.33f,
                o->half_extents.z * 0.22f
            );
        
        case SURFACE_OBJECT_ROVER:
            return vec3(
                o->half_extents.x * 0.45f,
                o->half_extents.y * 0.22f,
                o->half_extents.z * 0.45f
            );

        default:
            return vec3(
                o->half_extents.x * COLLISION_SCALE_XZ,
                o->half_extents.y * COLLISION_SCALE_Y,
                o->half_extents.z * COLLISION_SCALE_XZ
            );
    }
}

// Objektumonként eltérő bbox középpont
Vec3 object_bbox_center(const SurfaceObject *o)
{
    Vec3 c = o->position;

    switch(o->type){
        case SURFACE_OBJECT_RIVER_ROCK:
            c.y -= o->half_extents.y * 0.95f;
            break;

        case SURFACE_OBJECT_ROCK:
            c.y += 0.90f;
            break;

        case SURFACE_OBJECT_TREE:
            c.y += o->half_extents.y * 0.10f;
            break;

        case SURFACE_OBJECT_SHIP:
            c.y -= o->half_extents.y * 0.60f;
            break;

        case SURFACE_OBJECT_ROVER:
            c.y -= o->half_extents.y * 0.72f;
            break;

        default:
            break;
    }

    return c;
}

// A felszíni objektumok kirajzolása
void draw_objects(const PlanetScene *scene)
{
    int i;

    ensure_assets_loaded();

    for(i = 0; i < scene->object_count; ++i){
        const SurfaceObject *o = &scene->objects[i];

        switch(o->type){
            case SURFACE_OBJECT_SHIP:
                draw_asset(&g_assets.ship,
                           vec3(o->position.x, o->position.y - o->half_extents.y, o->position.z),
                           o->yaw_deg,
                           o->model_scale);
                break;
            
            case SURFACE_OBJECT_ROVER:
                draw_rover_asset(&g_assets.rover,
                                 vec3(o->position.x, o->position.y - 0.2f, o->position.z),
                                 o->yaw_deg,
                                 15.0f,
                                 g_river_time);
                break;

            case SURFACE_OBJECT_RIVER_ROCK:
                draw_asset(&g_assets.river_rock,
                           vec3(o->position.x, o->position.y - o->half_extents.y, o->position.z),
                           o->yaw_deg,
                           o->model_scale);
                break;

            case SURFACE_OBJECT_ROCK:
                draw_asset(&g_assets.wild_rock,
                           vec3(o->position.x, o->position.y - o->half_extents.y, o->position.z),
                           o->yaw_deg,
                           o->model_scale);
                break;

            case SURFACE_OBJECT_TREE:
                draw_asset(&g_assets.tree,
                           vec3(o->position.x, o->position.y - o->half_extents.y * 0.60f, o->position.z),
                           o->yaw_deg,
                           o->model_scale);
                break;

            case SURFACE_OBJECT_CRATE:
            case SURFACE_OBJECT_LOG:
            default:
                draw_box(o->position, o->half_extents);
                break;
        }
    }
}

// Új objektum hozzáadása a jelenethez
void add_object(PlanetScene *scene,
                       SurfaceObjectType type,
                       float x,
                       float z,
                       float hx,
                       float hy,
                       float hz,
                       bool interactive,
                       bool blocks_movement,
                       const char *label)
{
    SurfaceObject *o;

    if(scene->object_count >= (int)(sizeof(scene->objects) / sizeof(scene->objects[0]))){
        return;
    }

    o = &scene->objects[scene->object_count++];
    memset(o, 0, sizeof(*o));

    o->type = type;
    o->position = vec3(x, terrain_height(scene, x, z) + hy, z);
    o->half_extents = vec3(hx, hy, hz);
    o->interactive = interactive;
    o->blocks_movement = blocks_movement;
    o->active = false;
    o->state = 0.0f;
    o->yaw_deg = 0.0f;
    o->model_scale = 1.0f;
    o->label = label;
}

// Legutóbb hozzáadott objektum lekérése
SurfaceObject *last_object(PlanetScene *scene)
{
    if(scene->object_count <= 0){
        return NULL;
    }
    return &scene->objects[scene->object_count - 1];
}

// A játékoshoz legközelebbi és nézett interaktív objektum keresése
int find_interactive_index(const PlanetScene *scene, Vec3 pos, Vec3 forward, float max_dist)
{
    int i;
    int best = -1;
    float best_score = -1000.0f;
    Vec3 look = vec3_norm(vec3(forward.x, 0.0f, forward.z)); // Nézési irány az xz síkon

    if(vec3_len(look) < 0.0001f){
        look = vec3(0.0f, 0.0f, -1.0f);
    }

    for(i = 0; i < scene->object_count; ++i){
        const SurfaceObject *o = &scene->objects[i];
        Vec3 to;
        float dist;
        float facing;
        float score;

        if(!o->interactive){
            continue;
        }

        // Objektumhoz vezető vízszintes vektor és távolság
        to = vec3(o->position.x - pos.x, 0.0f, o->position.z - pos.z);
        dist = vec3_len(to);
        if(dist > max_dist || dist < 0.0001f){
            continue;
        }

        to = vec3_scale(to, 1.0f / dist);
        facing = vec3_dot(look, to);
        score = facing * 3.0f - dist;

        if(facing > 0.15f && score > best_score){
            best_score = score;
            best = i;
        }
    }

    return best;
}

// Ütközésvizsgálat egy objektummal
bool collides_with_object(const SurfaceObject *o, Vec3 pos, float radius)
{
    float dx, dz;
    Vec3 c, h;

    if(!o->blocks_movement){
        return false;
    }

    if(o->type == SURFACE_OBJECT_SHIP){
        get_ship_bbox(o, &c, &h);
    }else{
        c = object_bbox_center(o);
        h = object_bbox_half(o);
    }

    // Játékos és az objektum távolsága
    dx = fabsf(pos.x - c.x);
    dz = fabsf(pos.z - c.z);

    return dx < (h.x + radius) &&
           dz < (h.z + radius);
}