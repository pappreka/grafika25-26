#include "planet_scene.h"
#include "planet_scene_internal.h"
#include "mesh.h"
#include "texture.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

#if defined(_WIN32)
#include <windows.h>
#endif

#include <GL/gl.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Szabad porrészecske keresése
DustParticle *alloc_dust_particle(PlanetScene *scene)
{
    int i;

    for(i = 0; i < (int)(sizeof(scene->dust_particles) / sizeof(scene->dust_particles[0])); ++i){
        if(!scene->dust_particles[i].active){
            return &scene->dust_particles[i];
        }
    }

    return NULL;
}

// Véletlen szám adott intervallumban
float frand_range(float a, float b)
{
    return a + (b - a) * ((float)rand() / (float)RAND_MAX);
}

// Porfelhő létrehozása
void spawn_dust_burst(PlanetScene *scene, Vec3 origin, Vec3 move_dir, int count, float strength)
{
    int i;
    Vec3 dir = vec3(move_dir.x, 0.0f, move_dir.z); // Vízszintes mozgásirány

    if(vec3_len(dir) < 0.0001f){
        dir = vec3(0.0f, 0.0f, 1.0f);
    }else{
        dir = vec3_norm(dir);
    }

    for(i = 0; i < count; ++i){
        DustParticle *p = alloc_dust_particle(scene);
        float side;
        float forward_jitter;
        float upward;
        float spread_x;
        float spread_z;

        if(!p){
            return;
        }

        side = frand_range(-1.0f, 1.0f);
        forward_jitter = frand_range(-0.35f, 0.35f);
        upward = frand_range(0.55f, 1.15f) * strength;

        spread_x = -dir.z * side + dir.x * forward_jitter;
        spread_z =  dir.x * side + dir.z * forward_jitter;

        p->active = true;
        p->age = 0.0f;
        p->life = frand_range(0.45f, 0.95f);
        p->size = frand_range(0.18f, 0.42f) * (0.7f + strength * 0.4f);
        p->alpha = frand_range(0.22f, 0.42f);

        // Origin közepéből indul
        p->position = vec3(
            origin.x + spread_x * 0.18f,
            origin.y + frand_range(0.02f, 0.10f),
            origin.z + spread_z * 0.18f
        );

        // Véletlen oldalirányú és felfelé sebesség
        p->velocity = vec3(
            spread_x * frand_range(0.45f, 1.10f) * strength,
            upward,
            spread_z * frand_range(0.45f, 1.10f) * strength
        );
    }
}

// Porrészecskék frissítése
void update_dust_particles(PlanetScene *scene, float dt)
{
    int i;

    for(i = 0; i < (int)(sizeof(scene->dust_particles) / sizeof(scene->dust_particles[0])); ++i){
        DustParticle *p = &scene->dust_particles[i];

        if(!p->active){
            continue;
        }

        p->age += dt;
        // Ha lejárt az élettartam -> eltűnik
        if(p->age >= p->life){
            p->active = false;
            continue;
        }

        // Vízszintes lassulás
        p->velocity.x *= (1.0f - dt * 1.6f);
        p->velocity.z *= (1.0f - dt * 1.6f);
        p->velocity.y -= 2.2f * dt;

        // Frissítés
        p->position = vec3_add(p->position, vec3_scale(p->velocity, dt));

        {
            // Talaj alá kerülés kezelése
            float ground_y = terrain_height(scene, p->position.x, p->position.z) + 0.015f;
            if(p->position.y < ground_y){
                p->position.y = ground_y;
                p->velocity.x *= 0.55f;
                p->velocity.z *= 0.55f;
                p->velocity.y *= -0.10f;
            }
        }
    }
}

// Porrészecskék billboardként kirajzolása
void draw_dust_particles(const PlanetScene *scene, Vec3 camera_right, Vec3 camera_up)
{
    int i;

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glBegin(GL_QUADS);
    for(i = 0; i < (int)(sizeof(scene->dust_particles) / sizeof(scene->dust_particles[0])); ++i){
        const DustParticle *p = &scene->dust_particles[i];

        if(!p->active){
            continue;
        }

        {
            // Öregedés -> halványodik, nő
            float t = p->age / p->life;
            float alpha = p->alpha * (1.0f - t);
            float size = p->size * (0.65f + t * 1.1f);

            Vec3 right = vec3_scale(camera_right, size);
            Vec3 up    = vec3_scale(camera_up, size * 0.55f);

            // Négyszög sarkai
            Vec3 v0 = vec3_add(vec3_sub(p->position, right), up);
            Vec3 v1 = vec3_add(vec3_add(p->position, right), up);
            Vec3 v2 = vec3_sub(vec3_add(p->position, right), up);
            Vec3 v3 = vec3_sub(vec3_sub(p->position, right), up);

            // Vénuszon sárgás részecskék
            if(scene->planet_index == 2){
                glColor4f(0.92f, 0.56f, 0.18f, alpha); glVertex3f(v0.x, v0.y, v0.z);
                glColor4f(0.92f, 0.56f, 0.18f, alpha); glVertex3f(v1.x, v1.y, v1.z);
                glColor4f(0.92f, 0.56f, 0.18f, 0.0f);  glVertex3f(v2.x, v2.y, v2.z);
                glColor4f(0.92f, 0.56f, 0.18f, 0.0f);  glVertex3f(v3.x, v3.y, v3.z);
            }else{ // Egyébként szürkés
                glColor4f(0.72f, 0.68f, 0.60f, alpha); glVertex3f(v0.x, v0.y, v0.z);
                glColor4f(0.72f, 0.68f, 0.60f, alpha); glVertex3f(v1.x, v1.y, v1.z);
                glColor4f(0.72f, 0.68f, 0.60f, 0.0f);  glVertex3f(v2.x, v2.y, v2.z);
                glColor4f(0.72f, 0.68f, 0.60f, 0.0f);  glVertex3f(v3.x, v3.y, v3.z);
            }
        }
    }
    glEnd();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
}

// Vénuszi lebegő részecske létrehozása
void spawn_venus_particle(PlanetScene *scene)
{
    int i;

    for(i = 0; i < (int)(sizeof(scene->dust_particles) / sizeof(scene->dust_particles[0])); ++i){
        DustParticle *p = &scene->dust_particles[i];
        if(!p->active){
            // Véletlen x/z pozíció
            float x = ((float)rand() / (float)RAND_MAX) * scene->terrain_extent * 2.0f - scene->terrain_extent;
            float z = ((float)rand() / (float)RAND_MAX) * scene->terrain_extent * 2.0f - scene->terrain_extent;
            // Talaj fölött 2-7 egységgel indul
            float y = terrain_height(scene, x, z) + 2.0f + ((float)rand() / (float)RAND_MAX) * 5.0f;

            p->active = true;
            p->position = vec3(x, y, z);
            // Oldalirányban sodródik, enyhén emelkedik, Z irányban véletlenül mozog
            p->velocity = vec3(2.6f + ((float)rand() / (float)RAND_MAX) * 1.6f,
                               0.08f + ((float)rand() / (float)RAND_MAX) * 0.16f,
                               -0.8f + ((float)rand() / (float)RAND_MAX) * 1.6f);
            p->age = 0.0f;
            p->life = 2.0f + ((float)rand() / (float)RAND_MAX) * 1.8f;
            p->size = 0.75f + ((float)rand() / (float)RAND_MAX) * 1.15f;
            p->alpha = 0.55f + ((float)rand() / (float)RAND_MAX) * 0.25f;
            return;
        }
    }
}

// Vénusz részecskerendszerének frissítése
void update_venus_particles(PlanetScene *scene, float dt)
{
    int i;

    // Minden frame-ben 4 új részecske
    for(i = 0; i < 4; ++i){
        spawn_venus_particle(scene);
    }

    for(i = 0; i < (int)(sizeof(scene->dust_particles) / sizeof(scene->dust_particles[0])); ++i){
        DustParticle *p = &scene->dust_particles[i];

        if(!p->active){
            continue;
        }

        p->age += dt;
        if(p->age >= p->life){
            p->active = false;
            continue;
        }

        // Mozgatás
        p->position = vec3_add(p->position, vec3_scale(p->velocity, dt));
        // Lebegő hullámzás
        p->position.y += sinf(scene->venus_cloud_time * 2.2f + p->position.x * 0.25f) * 0.03f;

        // Elhagyja a pályát -> törlődik
        if(fabsf(p->position.x) > scene->terrain_extent ||
           fabsf(p->position.z) > scene->terrain_extent){
            p->active = false;
        }
    }
}

// Vénusz magas felhőrétegének kirajzolása
void draw_venus_cloud_layer(const PlanetScene *scene)
{
    int i;
    const int segments = 40;
    const float radius = scene->terrain_extent * 0.95f;
    const float y = 55.0f;

    if(scene->planet_index != 2){
        return;
    }

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glBegin(GL_TRIANGLE_FAN);
    glColor4f(0.95f, 0.70f, 0.40f, 0.40f);
    glVertex3f(0.0f, y, 0.0f);

    for(i = 0; i <= segments; ++i){
        float a = (float)i / (float)segments * 2.0f * (float)M_PI;
        float wave = 1.0f + 0.10f * sinf(a * 5.0f + scene->venus_cloud_time * 0.70f); // Enyhén hullámzik
        float x = cosf(a) * radius * wave;
        float z = sinf(a) * radius * wave;
        float yy = y + 0.35f * sinf(a * 4.0f + scene->venus_cloud_time * 0.55f); // Felhő pereme fel-le hullámzik

        glColor4f(0.92f, 0.70f, 0.35f, 0.30f);
        glVertex3f(x, yy, z);
    }
    glEnd();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
}

// Vénuszi lávarepedések kirajzolása
void draw_venus_lava_cracks(const PlanetScene *scene)
{
    const int steps = 90;
    const float extent = scene->terrain_extent;
    const float step = (extent * 2.0f) / (float)steps;
    int iz, ix;

    if(scene->planet_index != 2){
        return;
    }

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    glBegin(GL_QUADS);
    for(iz = 0; iz < steps; ++iz){
        float z0 = -extent + (float)iz * step;
        float z1 = z0 + step;

        for(ix = 0; ix < steps; ++ix){
            float x0 = -extent + (float)ix * step;
            float x1 = x0 + step;

            float cx = (x0 + x1) * 0.5f;
            float cz = (z0 + z1) * 0.5f;
            float mask = venus_lava_mask(cx, cz);

            if(mask < 0.22f){
                continue;
            }

            {
                float y = terrain_height(scene, cx, cz) + 0.03f;
                float pulse = 0.70f + 0.30f * sinf(scene->venus_cloud_time * 3.2f + cx * 0.25f + cz * 0.18f);
                float alpha = (mask - 0.22f) * 0.50f;

                if(alpha > 0.75f){
                    alpha = 0.75f;
                }

                glColor4f(1.00f, 0.30f + 0.20f * pulse, 0.02f, alpha);
                glVertex3f(x0, y, z0);
                glVertex3f(x1, y, z0);
                glVertex3f(x1, y, z1);
                glVertex3f(x0, y, z1);
            }
        }
    }
    glEnd();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
}