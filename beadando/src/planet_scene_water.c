#include "planet_scene.h"
#include "planet_scene_internal.h"
#include "mesh.h"
#include "texture.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#if defined(_WIN32)
#include <windows.h>
#endif

#include <GL/gl.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// A vízhullámok magassági hozzájárulása
float ripple_height_offset(const PlanetScene *scene, float x, float z)
{
    int i;
    float sum = 0.0f; // Összes aktív hullám összege

    for(i = 0; i < (int)(sizeof(scene->ripples) / sizeof(scene->ripples[0])); ++i){
        const WaterRipple *r = &scene->ripples[i];
        if(!r->active){
            continue;
        }

        {
            // Távolság a ripple középpontjától
            float dx = x - r->position.x;
            float dz = z - r->position.z;
            float dist = sqrtf(dx * dx + dz * dz);
            float front = r->radius + r->age * r->speed; // Hullámgyűrű aktuális állapota
            float band = 0.9f; // Hullámgyűrű vastagsága
            float d = fabsf(dist - front); // Gyűrű középpontjától lévő távolság

            if(d < band){
                float fade = 1.0f - (r->age / r->life); // Idővel halványodik
                float ring = 1.0f - (d / band); // Sáv széle felé csökken
                float wave = sinf((1.0f - d / band) * (float)M_PI);
                sum += r->amplitude * fade * ring * wave; // Összes hullám hatása
            }
        }
    }

    return sum;
}

// A folyó közepe enyhén kanyarog
float river_center_x(const PlanetScene *scene, float z)
{
    (void)scene;
    return scene->river_x + sinf(z * 0.045f) * 1.6f;
}

// A folyó végén tóvá szélesedés átmenete
float lake_blend(const PlanetScene *scene, float z)
{
    float start_z = scene->river_z_max - 14.0f; // Kezdés
    float t = (z - start_z) / (scene->river_z_max - start_z);

    // 0 és 1 közé szorítás
    if(t < 0.0f) t = 0.0f;
    if(t > 1.0f) t = 1.0f;

    return t * t * (3.0f - 2.0f * t); // Simított átmenet
}

// A vízfelszín magassága hullámokkal és hullámgyűrűkkel
float river_surface_height(const PlanetScene *scene, float x, float z)
{
    float base_level = -0.42f; // Általános magasság
    float slope = -0.10f * (z / 45.0f); // Enyhe lejtés

    // Hosszú hullámok
    float wave_long_1 = 0.050f * sinf(z * 0.22f + x * 0.05f + g_river_time * 0.90f);
    float wave_long_2 = 0.035f * cosf(z * 0.18f - x * 0.07f - g_river_time * 0.70f);

    // Közepes hullámok
    float wave_mid_1  = 0.020f * sinf(z * 0.85f + x * 0.18f + g_river_time * 1.80f);
    float wave_mid_2  = 0.014f * cosf(z * 1.10f - x * 0.14f - g_river_time * 1.40f);

    // Kis hullámok
    float wave_small_1 = 0.008f * sinf(z * 2.40f + x * 0.35f + g_river_time * 3.20f);
    float wave_small_2 = 0.006f * cosf(z * 2.90f - x * 0.28f - g_river_time * 2.70f);

    // Hullámgyűrű magassága
    float ripple = ripple_height_offset(scene, x, z);

    // A tóban nyugodtabb víz
    {
        float lake = lake_blend(scene, z);
        float calm = 1.0f - 0.45f * lake;

        wave_long_1 *= calm;
        wave_long_2 *= calm;
        wave_mid_1  *= calm;
        wave_mid_2  *= calm;
        wave_small_1 *= calm;
        wave_small_2 *= calm;
    }

    return base_level
         + slope
         + wave_long_1 + wave_long_2
         + wave_mid_1  + wave_mid_2
         + wave_small_1 + wave_small_2
         + ripple;
}

// A vízfelszín normálvektora
Vec3 river_surface_normal(const PlanetScene *scene, float x, float z)
{
    const float e = 0.12f;

    float hl = river_surface_height(scene, x - e, z);
    float hr = river_surface_height(scene, x + e, z);
    float hd = river_surface_height(scene, x, z - e);
    float hu = river_surface_height(scene, x, z + e);

    Vec3 n = vec3(hl - hr, 2.0f * e, hd - hu);
    return vec3_norm(n);
}

// A folyó kirajzolása habbal és szélesedő tóvéggel
void draw_river(const PlanetScene *scene)
{
    int i;
    const int segments = 220;
    float z0, z1, step;

    const GLfloat river_specular[]  = { 0.32f, 0.32f, 0.36f, 1.0f };
    const GLfloat river_shininess[] = { 34.0f };

    if(!scene->has_river){
        return;
    }

    z0 = scene->river_z_min;
    z1 = scene->river_z_max;
    step = (z1 - z0) / (float)segments;

    glEnable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, river_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, river_shininess);

    // Szegmensenkénti feldolgozás
    for(i = 0; i < segments; ++i){
        float za = z0 + (float)i * step;
        float zb = za + step;

        // Folyó közepe
        float center_a = river_center_x(scene, za);
        float center_b = river_center_x(scene, zb);

        float lake_a = lake_blend(scene, za);
        float lake_b = lake_blend(scene, zb);

        float half_w_a = (scene->river_half_width + 5.0f) + lake_a * 3.0f;
        float half_w_b = (scene->river_half_width + 5.0f) + lake_b * 3.0f;

        // Bal és jobb szél
        float left_a  = center_a - half_w_a;
        float right_a = center_a + half_w_a;
        float left_b  = center_b - half_w_b;
        float right_b = center_b + half_w_b;

        // Habos sáv szélessége
        float foam_band_a = half_w_a * 0.10f;
        float foam_band_b = half_w_b * 0.10f;

        // Víz és a hab határai
        float inner_left_a  = left_a  + foam_band_a;
        float inner_right_a = right_a - foam_band_a;
        float inner_left_b  = left_b  + foam_band_b;
        float inner_right_b = right_b - foam_band_b;

        // Pontok vízmagassága
        float yla  = river_surface_height(scene, left_a, za);
        float yra  = river_surface_height(scene, right_a, za);
        float ylb  = river_surface_height(scene, left_b, zb);
        float yrb  = river_surface_height(scene, right_b, zb);

        float yila = river_surface_height(scene, inner_left_a, za)  + 0.010f;
        float yira = river_surface_height(scene, inner_right_a, za) + 0.010f;
        float yilb = river_surface_height(scene, inner_left_b, zb)  + 0.010f;
        float yirb = river_surface_height(scene, inner_right_b, zb) + 0.010f;

        // Normálok
        Vec3 nla  = river_surface_normal(scene, left_a, za);
        Vec3 nra  = river_surface_normal(scene, right_a, za);
        Vec3 nlb  = river_surface_normal(scene, left_b, zb);
        Vec3 nrb  = river_surface_normal(scene, right_b, zb);

        Vec3 nila = river_surface_normal(scene, inner_left_a, za);
        Vec3 nira = river_surface_normal(scene, inner_right_a, za);
        Vec3 nilb = river_surface_normal(scene, inner_left_b, zb);
        Vec3 nirb = river_surface_normal(scene, inner_right_b, zb);

        float foam_anim_a = 0.5f + 0.5f * sinf(za * 2.6f);
        float foam_anim_b = 0.5f + 0.5f * sinf(zb * 2.6f);

        // Hab átlátszóság
        float foam_alpha_a = 0.82f + 0.12f * foam_anim_a;
        float foam_alpha_b = 0.82f + 0.12f * foam_anim_b;

        foam_alpha_a *= (1.0f - 0.25f * lake_a);
        foam_alpha_b *= (1.0f - 0.25f * lake_b);

        // Fő vízfelület
        glBegin(GL_QUADS);

        glColor4f(0.08f, 0.30f, 0.70f, 0.92f);
        glNormal3f(nila.x, nila.y, nila.z);
        glVertex3f(inner_left_a, yila, za);

        glColor4f(0.07f, 0.27f, 0.62f, 0.94f);
        glNormal3f(nira.x, nira.y, nira.z);
        glVertex3f(inner_right_a, yira, za);

        glColor4f(0.07f, 0.27f, 0.62f, 0.94f);
        glNormal3f(nirb.x, nirb.y, nirb.z);
        glVertex3f(inner_right_b, yirb, zb);

        glColor4f(0.08f, 0.30f, 0.70f, 0.92f);
        glNormal3f(nilb.x, nilb.y, nilb.z);
        glVertex3f(inner_left_b, yilb, zb);

        glEnd();

        // Bal habos rész
        glBegin(GL_QUADS);

        glColor4f(0.88f, 0.93f, 0.96f, foam_alpha_a);
        glNormal3f(nla.x, nla.y, nla.z);
        glVertex3f(left_a, yla + 0.006f, za);

        glColor4f(0.70f, 0.82f, 0.90f, 0.70f);
        glNormal3f(nila.x, nila.y, nila.z);
        glVertex3f(inner_left_a, yila + 0.002f, za);

        glColor4f(0.70f, 0.82f, 0.90f, 0.70f);
        glNormal3f(nilb.x, nilb.y, nilb.z);
        glVertex3f(inner_left_b, yilb + 0.002f, zb);

        glColor4f(0.88f, 0.93f, 0.96f, foam_alpha_b);
        glNormal3f(nlb.x, nlb.y, nlb.z);
        glVertex3f(left_b, ylb + 0.006f, zb);

        glEnd();

        // Jobb habos rész
        glBegin(GL_QUADS);

        glColor4f(0.70f, 0.82f, 0.90f, 0.70f);
        glNormal3f(nira.x, nira.y, nira.z);
        glVertex3f(inner_right_a, yira + 0.002f, za);

        glColor4f(0.88f, 0.93f, 0.96f, foam_alpha_a);
        glNormal3f(nra.x, nra.y, nra.z);
        glVertex3f(right_a, yra + 0.006f, za);

        glColor4f(0.88f, 0.93f, 0.96f, foam_alpha_b);
        glNormal3f(nrb.x, nrb.y, nrb.z);
        glVertex3f(right_b, yrb + 0.006f, zb);

        glColor4f(0.70f, 0.82f, 0.90f, 0.70f);
        glNormal3f(nirb.x, nirb.y, nirb.z);
        glVertex3f(inner_right_b, yirb + 0.002f, zb);

        glEnd();
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
}

// Egy pont a folyóban van-e
bool point_in_river(const PlanetScene *scene, float x, float z)
{
    float cx;
    float half_w;

    if(!scene->has_river){
        return false;
    }
    if(z < scene->river_z_min || z > scene->river_z_max){
        return false;
    }

    cx = river_center_x(scene, z); // Folyó közepe
    half_w = (scene->river_half_width + 0.35f) + lake_blend(scene, z) * 3.0f; // Fél szélesség

    return fabsf(x - cx) < half_w;
}

// Szabad kőslot keresése
ThrownStone *alloc_stone(PlanetScene *scene)
{
    int i;
    for(i = 0; i < (int)(sizeof(scene->stones) / sizeof(scene->stones[0])); ++i){
        if(!scene->stones[i].active){
            scene->stone_count = (scene->stone_count < i + 1) ? (i + 1) : scene->stone_count;
            return &scene->stones[i];
        }
    }
    return NULL;
}

// Szabad hullámslot keresése
WaterRipple *alloc_ripple(PlanetScene *scene)
{
    int i;
    for(i = 0; i < (int)(sizeof(scene->ripples) / sizeof(scene->ripples[0])); ++i){
        if(!scene->ripples[i].active){
            return &scene->ripples[i];
        }
    }
    return NULL;
}

// Új vízhullám létrehozása
void spawn_ripple(PlanetScene *scene, Vec3 pos, float strength)
{
    // Szabad hullámhely kérése
    WaterRipple *r = alloc_ripple(scene);
    if(!r){
        return;
    }

    // Kezdőállapotok
    r->active = true;
    r->position = pos;
    r->age = 0.0f;
    r->life = 1.8f;
    r->amplitude = strength;
    r->radius = 0.15f;
    r->speed = 4.0f;
}

// Eldobott kövek kirajzolása
void draw_thrown_stones(const PlanetScene *scene)
{
    int i;

    ensure_assets_loaded();

    for(i = 0; i < scene->stone_count; ++i){
        const ThrownStone *s = &scene->stones[i];
        if(!s->active){
            continue;
        }

        if(g_assets.throw_rock.loaded){
            draw_asset(&g_assets.throw_rock, s->position, 0.0f, 0.10f);
        }else{
            draw_box(s->position, vec3(0.12f, 0.12f, 0.12f));
        }
    }
}

// Folyócsobbanások kirajzolása
void draw_river_splashes(const PlanetScene *scene)
{
    int i;

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for(i = 0; i < (int)(sizeof(scene->ripples) / sizeof(scene->ripples[0])); ++i){
        const WaterRipple *r = &scene->ripples[i];
        if(!r->active){
            continue;
        }

        {
            int j;
            const int segments = 24; // Szegmensek
            float life_t = r->age / r->life; // Hullám életének aránya
            float radius = r->radius + r->age * r->speed; // Hullám sugara
            float alpha = (1.0f - life_t) * 0.65f;
            float y = r->position.y + 0.02f + 0.08f * (1.0f - life_t);

            glBegin(GL_TRIANGLE_STRIP);
            for(j = 0; j <= segments; ++j){
                // Aktuális körpont iránya
                float a = ((float)j / (float)segments) * 2.0f * (float)M_PI;
                float ca = cosf(a);
                float sa = sinf(a);

                // Belső perem teljesen átlátszó
                glColor4f(0.85f, 0.92f, 0.98f, 0.0f);
                glVertex3f(r->position.x + ca * (radius - 0.10f), y, r->position.z + sa * (radius - 0.10f));

                // Külső perem láthatóbb
                glColor4f(0.90f, 0.96f, 1.0f, alpha);
                glVertex3f(r->position.x + ca * radius, y, r->position.z + sa * radius);
            }
            glEnd();
        }
    }

    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
}