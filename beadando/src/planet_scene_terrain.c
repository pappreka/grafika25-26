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

// A hegygyűrű belső sugara
float mountain_ring_inner_radius(const PlanetScene *scene)
{
    const float seam_overlap = 3.5f; // Átfedés
    return scene->terrain_extent - seam_overlap; // Belső sugár kezdete
}

// A hegygyűrű ütközési sugara
float mountain_collision_radius(const PlanetScene *scene)
{
    return mountain_ring_inner_radius(scene) - 1.2f;
}

// Egyszerű kráterprofil
float crater_shape(float dx, float dz, float radius, float depth)
{
    float d = sqrtf(dx * dx + dz * dz) / radius; // Normalizált távolság

    if(d >= 1.0f){
        return 0.0f;
    }

    {
        float bowl = 1.0f - d * d; // Fő mélyedés
        float rim = 1.0f - fabsf(d - 0.82f) / 0.18f; // Peremkiemelkedés

        if(rim < 0.0f){
            rim = 0.0f;
        }

        return -depth * bowl + depth * 0.22f * rim; // Középen negatív mélyedés, szélen enyhe pozitív perem
    }
}

// Vénuszi vulkán forma
float venus_volcano_shape(float x, float z,
                                 float cx, float cz,
                                 float radius,
                                 float height,
                                 float crater_radius,
                                 float crater_depth)
{
    // Távolság a vulkán középpontjától
    float dx = x - cx;
    float dz = z - cz;
    float d = sqrtf(dx * dx + dz * dz);

    if(d >= radius){
        return 0.0f;
    }

    {
        // Középpont fele nő a magasság
        float t = 1.0f - d / radius;
        float cone = t * t * height;
        float crater = 0.0f;

        // Középpontnál kráteres csúcs
        if(d < crater_radius){
            float k = 1.0f - d / crater_radius;
            crater = k * k * crater_depth;
        }

        return cone - crater; // Vulkán tetején kráteres csúcs
    }
}

// Vénuszi lávarepedések intenzitásmaszkja
float venus_lava_mask(float x, float z)
{
    // Repedésvonalak (vonal közelében nagy érték, távolabb csökken)
    float crack1 = expf(-fabsf(x + 8.0f * sinf(z * 0.09f)) * 0.85f);
    float crack2 = expf(-fabsf(z - 10.0f * sinf(x * 0.06f)) * 0.82f);
    float crack3 = expf(-fabsf((x + z * 0.55f) - 6.0f * cosf(z * 0.08f)) * 0.88f);
    float crack4 = expf(-fabsf((x - z * 0.70f) + 7.5f * sinf(x * 0.05f)) * 0.92f);
    float crack5 = expf(-fabsf((z + x * 0.35f) - 5.5f * cosf(x * 0.07f)) * 0.95f);

    float mask = 0.0f;
    mask += crack1 * 1.10f;
    mask += crack2 * 1.00f;
    mask += crack3 * 0.95f;
    mask += crack4 * 0.85f;
    mask += crack5 * 0.75f;

    if(mask > 1.0f){
        mask = 1.0f;
    }

    return mask;
}

// A terep magassága bolygónként eltérő szabályokkal
float terrain_height(const PlanetScene *scene, float x, float z)
{
    // Merkúr
    if(scene->planet_index == 1){
        float h = 0.0f;

        // Enyhe alapszaálytalanság
        h += 0.02f * sinf(x * 0.04f);
        h += 0.02f * cosf(z * 0.04f);

        // Több helyen kráterek
        h += crater_shape(x - 10.0f, z - 10.0f, 8.0f, 1.0f);
        h += crater_shape(x + 15.0f, z - 5.0f, 6.0f, 0.8f);
        h += crater_shape(x - 5.0f, z + 18.0f, 5.0f, 0.7f);
        h += crater_shape(x + 12.0f, z + 14.0f, 9.0f, 1.2f);
        h += crater_shape(x - 20.0f, z, 7.0f, 0.9f);

        return h;
    }

    // Vénusz
    if(scene->planet_index == 2){
        float h = 0.0f;

        // Erősebb alap
        h += 0.35f * sinf(x * 0.050f);
        h += 0.28f * cosf(z * 0.055f);
        h += 0.14f * sinf((x + z) * 0.110f);
        h += 0.08f * cosf((x - z) * 0.180f);
        h += 0.04f * sinf(x * 0.75f) * cosf(z * 0.60f);

        // Vulkánuk
        h += venus_volcano_shape(x, z, -14.0f, -10.0f, 14.0f, 4.8f, 4.0f, 1.8f);
        h += venus_volcano_shape(x, z,  16.0f,  12.0f, 12.0f, 4.1f, 3.6f, 1.5f);
        h += venus_volcano_shape(x, z,   4.0f, -18.0f, 10.0f, 3.3f, 2.8f, 1.2f);

        // Láva
        h -= venus_lava_mask(x, z) * 0.65f;

        return h;
    }

    // Mars
    if(scene->planet_index == 4){
        float h = 0.0f;

        // Alap
        h += 0.18f * sinf(x * 0.035f);
        h += 0.15f * cosf(z * 0.040f);
        h += 0.08f * sinf((x + z) * 0.060f);

        // Kráterek
        h += crater_shape(x - 12.0f, z - 8.0f, 9.0f, 0.9f);
        h += crater_shape(x + 16.0f, z + 10.0f, 7.0f, 0.7f);
        h += crater_shape(x - 4.0f, z + 18.0f, 5.5f, 0.55f);
        h += crater_shape(x + 20.0f, z - 14.0f, 11.0f, 1.0f);

        // Enyhe lejtés
        h += 0.012f * x;

        return h;
    }

    // Föld
    {
        float hills = 0.40f * sinf(x * 0.08f) + 0.24f * cosf(z * 0.07f);
        float detail = 0.10f * sinf((x + z) * 0.16f);
        float h = hills + detail;

        if(scene->has_river){
            float cx = river_center_x(scene, z);
            float lake = lake_blend(scene, z);
            float cut_half_width = (scene->river_half_width + 0.35f) + lake * 3.0f;
            float dx = (x - cx) / cut_half_width;
            float river_cut = expf(-(dx * dx) * 1.6f) * 1.35f;
            float bank_shape = expf(-(dx * dx) * 0.45f) * 0.18f;

            h -= river_cut;
            h -= bank_shape;
        }

        return h;
    }
}

// A terep normálvektora numerikus közelítéssel
Vec3 terrain_normal(const PlanetScene *scene, float x, float z)
{
    if(scene->planet_index == 1){
        const float e = 0.45f;

        float hl = terrain_height(scene, x - e, z);
        float hr = terrain_height(scene, x + e, z);
        float hd = terrain_height(scene, x, z - e);
        float hu = terrain_height(scene, x, z + e);

        Vec3 n = vec3(hl - hr, 2.0f * e, hd - hu);
        return vec3_norm(n);
    }

    if(scene->planet_index == 2){
        const float e = 0.18f;
        float hl = terrain_height(scene, x - e, z);
        float hr = terrain_height(scene, x + e, z);
        float hd = terrain_height(scene, x, z - e);
        float hu = terrain_height(scene, x, z + e);

        Vec3 n = vec3(hl - hr, 2.0f * e, hd - hu);
        return vec3_norm(n);
    }

    {
        const float e = 0.25f;
        float hl = terrain_height(scene, x - e, z);
        float hr = terrain_height(scene, x + e, z);
        float hd = terrain_height(scene, x, z - e);
        float hu = terrain_height(scene, x, z + e);

        Vec3 n = vec3(hl - hr, 2.0f * e, hd - hu);
        return vec3_norm(n);
    }
}

// A környező hegygyűrű kirajzolása
void draw_mountain_ring(const PlanetScene *scene)
{
    const int segments = 96;
    const float inner_r = mountain_ring_inner_radius(scene);
    const float outer_r = scene->terrain_extent + 28.0f;
    int i;

    // Spekuláris fény, fényesség 0 -> nincs csillogás
    const GLfloat mountain_specular[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    const GLfloat mountain_shininess[] = { 0.0f };

    if(scene->planet_index == 1 || scene->planet_index == 2 || scene->planet_index == 4){
        return;
    }

    glEnable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);

    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mountain_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mountain_shininess);

    glBegin(GL_TRIANGLE_STRIP);
    for(i = 0; i <= segments; ++i){
        float t = (float)i / (float)segments;
        float a = t * 2.0f * (float)M_PI;

        float ca = cosf(a);
        float sa = sinf(a);

        // Változatos magasságok
        float n1 = 0.5f + 0.5f * sinf(a * 3.0f + 0.4f);
        float n2 = 0.5f + 0.5f * cosf(a * 5.0f - 0.7f);
        float n3 = 0.5f + 0.5f * sinf(a * 9.0f + 1.1f);

        // Hegycsúcs magasság
        float peak = 14.0f + n1 * 10.0f + n2 * 7.0f + n3 * 5.0f;

        float ix = ca * inner_r;
        float iz = sa * inner_r;
        float ox = ca * outer_r;
        float oz = sa * outer_r;

        float iy = terrain_height(scene, ix, iz) - 0.15f;
        float oy = terrain_height(scene, ox * 0.35f, oz * 0.35f) + peak;

        {
            Vec3 edge1 = vec3(ox - ix, oy - iy, oz - iz);
            Vec3 edge2 = vec3(-sa, 0.0f, ca);
            Vec3 n = vec3_norm(vec3_cross(edge2, edge1));

            glNormal3f(n.x, n.y, n.z);
        }

        glColor3f(0.12f, 0.32f, 0.12f);
        glVertex3f(ix, iy, iz);

        glColor3f(0.06f, 0.22f, 0.08f);
        glVertex3f(ox, oy, oz);
    }
    glEnd();

    glBegin(GL_TRIANGLE_STRIP);
    for(i = 0; i <= segments; ++i){
        float t = (float)i / (float)segments;
        float a = t * 2.0f * (float)M_PI;

        float ca = cosf(a);
        float sa = sinf(a);

        float n1 = 0.5f + 0.5f * sinf(a * 3.0f + 0.4f);
        float n2 = 0.5f + 0.5f * cosf(a * 5.0f - 0.7f);
        float n3 = 0.5f + 0.5f * sinf(a * 9.0f + 1.1f);

        float peak = 14.0f + n1 * 10.0f + n2 * 7.0f + n3 * 5.0f;

        float ox = ca * outer_r;
        float oz = sa * outer_r;
        float tx = ca * (outer_r + 20.0f);
        float tz = sa * (outer_r + 20.0f);

        float oy = terrain_height(scene, ox * 0.35f, oz * 0.35f) + peak;
        float ty = oy - 12.0f;

        {
            Vec3 edge1 = vec3(tx - ox, ty - oy, tz - oz);
            Vec3 edge2 = vec3(-sa, 0.0f, ca);
            Vec3 n = vec3_norm(vec3_cross(edge2, edge1));

            glNormal3f(n.x, n.y, n.z);
        }

        glColor3f(scene->ground_r * 0.22f,
                  scene->ground_g * 0.30f,
                  scene->ground_b * 0.20f);
        glVertex3f(ox, oy, oz);

        glColor3f(scene->ground_r * 0.15f,
                  scene->ground_g * 0.20f,
                  scene->ground_b * 0.14f);
        glVertex3f(tx, ty, tz);
    }
    glEnd();

    glEnable(GL_CULL_FACE);
}

// A fő terep kirajzolása
void draw_terrain(const PlanetScene *scene)
{
    int steps;

    if(scene->planet_index == 1){
        steps = 72;
    }else if(scene->planet_index == 2){
        steps = 88;
    }else{
        steps = 160;
    }

    const float extent = scene->terrain_extent;
    const float step = (extent * 2.0f) / (float)steps;
    int iz, ix;

    const GLfloat terrain_specular[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    const GLfloat terrain_shininess[] = { 0.0f };

    glDisable(GL_TEXTURE_2D);
    glColor3f(scene->ground_r, scene->ground_g, scene->ground_b);

    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, terrain_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, terrain_shininess);

    for(iz = 0; iz < steps; ++iz){
        float z0 = -extent + (float)iz * step;
        float z1 = z0 + step;

        glBegin(GL_TRIANGLE_STRIP);
        for(ix = 0; ix <= steps; ++ix){
            float x = -extent + (float)ix * step;
            float y0 = terrain_height(scene, x, z0);
            float y1 = terrain_height(scene, x, z1);

            Vec3 n0 = terrain_normal(scene, x, z0);
            Vec3 n1 = terrain_normal(scene, x, z1);

            if(scene->planet_index == 1){
                glColor3f(0.55f, 0.52f, 0.48f);
            }else{
                glColor3f(scene->ground_r, scene->ground_g, scene->ground_b);
            }

            glNormal3f(n0.x, n0.y, n0.z);
            glVertex3f(x, y0, z0);

            glNormal3f(n1.x, n1.y, n1.z);
            glVertex3f(x, y1, z1);
        }
        glEnd();
    }
}