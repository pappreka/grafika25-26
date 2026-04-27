#include "planet_scene.h"
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

// Ütközési dobozok méretarányai
#define COLLISION_SCALE_XZ 0.4f
#define COLLISION_SCALE_Y  0.6f

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

// Globális asset- és render segédváltozók
static SurfaceAssets g_assets;
static float g_light_intensity = 1.0f;
static float g_river_time = 0.0f;
static Vec3 g_cam_right;
static Vec3 g_cam_up;

// A hegygyűrű belső sugara
static float mountain_ring_inner_radius(const PlanetScene *scene)
{
    const float seam_overlap = 3.5f; // Átfedés
    return scene->terrain_extent - seam_overlap; // Belső sugár kezdete
}

// A hegygyűrű ütközési sugara
static float mountain_collision_radius(const PlanetScene *scene)
{
    return mountain_ring_inner_radius(scene) - 1.2f;
}

// Egy asset betöltése két lehetséges OBJ útvonalból
static void asset_try_load(SurfaceAsset *asset,
                           const char *obj_path_a,
                           const char *obj_path_b,
                           const char *tex_path)
{
    memset(asset, 0, sizeof(*asset)); // Struktúra nullázás

    asset->loaded =
        mesh_load_obj(&asset->mesh, obj_path_a) ||
        mesh_load_obj(&asset->mesh, obj_path_b);

    if(asset->loaded && tex_path){
        asset->texture_loaded = texture_load(&asset->texture, tex_path);
    }else{
        asset->texture_loaded = false;
        asset->texture.id = 0;
        asset->texture.width = 0;
        asset->texture.height = 0;
    }
}

// A szükséges modellek és textúrák egyszeri betöltése
static void ensure_assets_loaded(void)
{
    if(g_assets.tried){
        return;
    }
    g_assets.tried = true;

    asset_try_load(&g_assets.ship,
                   "assets/models/ship.obj",
                   "ship.obj",
                   "assets/textures/ship.png");

    asset_try_load(&g_assets.river_rock,
                   "assets/models/river-rock.obj",
                   "river-rock.obj",
                   "assets/textures/river_rock.png");

    asset_try_load(&g_assets.wild_rock,
                   "assets/models/rock.obj",
                   "rock.obj",
                   "assets/textures/rock.png");

    asset_try_load(&g_assets.throw_rock,
                   "assets/models/river-rock-throw.obj",
                   "river-rock-throw.obj",
                   "assets/textures/river_rock.png");

    asset_try_load(&g_assets.tree,
                   "assets/models/tree.obj",
                   "tree.obj",
                   "assets/textures/tree.png");

    asset_try_load(&g_assets.rover,
                   "assets/models/mars_rover.obj",
                   "mars_rover.obj",
                   "assets/textures/mars_rover_ao.png");
}

// Display list készítése gyorsabb rajzoláshoz
static void compile_asset_list(SurfaceAsset *asset)
{
    int i;

    if(!asset->loaded || asset->list_id != 0){
        return;
    }

    asset->list_id = glGenLists(1);
    if(asset->list_id == 0){
        return;
    }

    // Rajzoló parancsok eltárolása listában
    glNewList(asset->list_id, GL_COMPILE);
    glBegin(GL_TRIANGLES);
    for(i = 0; i < asset->mesh.vert_count; ++i){
        const Vertex *v = &asset->mesh.verts[i];
        glTexCoord2f(v->u, v->v);
        glNormal3f(v->nx, v->ny, v->nz);
        glVertex3f(v->px, v->py, v->pz);
    }
    glEnd();
    glEndList();
}

// Egy általános asset kirajzolása
static void draw_asset(const SurfaceAsset *asset,
                       Vec3 position,
                       float yaw_deg,
                       float scale)
{
    if(!asset->loaded){
        return;
    }

    // Ha még nincs display list -> létrehozás
    if(((SurfaceAsset*)asset)->list_id == 0){
        compile_asset_list((SurfaceAsset*)asset);
    }

    // Textúra betöltése
    if(asset->texture_loaded && asset->texture.id != 0){
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, (GLuint)asset->texture.id);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glColor3f(1.0f, 1.0f, 1.0f);
    }else{
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.8f, 0.8f, 0.8f);
    }

    // Modell transzformáció
    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);
    glRotatef(yaw_deg, 0.0f, 1.0f, 0.0f);
    glScalef(scale, scale, scale);

    // Rajzolás
    if(asset->list_id != 0){
        glCallList(asset->list_id);
    }else{
        int i;
        glBegin(GL_TRIANGLES);
        for(i = 0; i < asset->mesh.vert_count; ++i){
            const Vertex *v = &asset->mesh.verts[i];
            glTexCoord2f(v->u, v->v);
            glNormal3f(v->nx, v->ny, v->nz);
            glVertex3f(v->px, v->py, v->pz);
        }
        glEnd();
    }

    glPopMatrix();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}

// Rover kirajzolása kis animációval
static void draw_rover_asset(const SurfaceAsset *asset,
                             Vec3 position,
                             float yaw_deg,
                             float scale,
                             float time_sec)
{
    // Animációs paraméterek
    float scan_yaw = sinf(time_sec * 0.8f) * 8.0f;
    float rock_z = sinf(time_sec * 1.4f) * 2.5f;
    float rock_x = cosf(time_sec * 1.1f) * 1.2f;
    float bob = sinf(time_sec * 1.6f) * 0.05f;

    if(!asset->loaded){
        return;
    }

    if(((SurfaceAsset*)asset)->list_id == 0){
        compile_asset_list((SurfaceAsset*)asset);
    }

    if(asset->texture_loaded && asset->texture.id != 0){
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, (GLuint)asset->texture.id);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glColor3f(0.18f, 0.05f, 0.05f);
    }else{
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.18f, 0.05f, 0.05f);
    } 

    glPushMatrix();
    glTranslatef(position.x, position.y + bob, position.z);
    glRotatef(yaw_deg + scan_yaw, 0.0f, 1.0f, 0.0f);
    glRotatef(rock_z, 0.0f, 0.0f, 1.0f);
    glRotatef(rock_x, 1.0f, 0.0f, 0.0f);
    glScalef(scale, scale, scale);

    if(asset->list_id != 0){
        glCallList(asset->list_id);
    }else{
        int i;
        glBegin(GL_TRIANGLES);
        for(i = 0; i < asset->mesh.vert_count; ++i){
            const Vertex *v = &asset->mesh.verts[i];
            glTexCoord2f(v->u, v->v);
            glNormal3f(v->nx, v->ny, v->nz);
            glVertex3f(v->px, v->py, v->pz);
        }
        glEnd();
    }

    glPopMatrix();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}

// A vízhullámok magassági hozzájárulása
static float ripple_height_offset(const PlanetScene *scene, float x, float z)
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
static float river_center_x(const PlanetScene *scene, float z)
{
    (void)scene;
    return scene->river_x + sinf(z * 0.045f) * 1.6f;
}

// A folyó végén tóvá szélesedés átmenete
static float lake_blend(const PlanetScene *scene, float z)
{
    float start_z = scene->river_z_max - 14.0f; // Kezdés
    float t = (z - start_z) / (scene->river_z_max - start_z);

    // 0 és 1 közé szorítás
    if(t < 0.0f) t = 0.0f;
    if(t > 1.0f) t = 1.0f;

    return t * t * (3.0f - 2.0f * t); // Simított átmenet
}

// A vízfelszín magassága hullámokkal és hullámgyűrűkkel
static float river_surface_height(const PlanetScene *scene, float x, float z)
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
static Vec3 river_surface_normal(const PlanetScene *scene, float x, float z)
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
static void draw_river(const PlanetScene *scene)
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

// Egyszerű kráterprofil
static float crater_shape(float dx, float dz, float radius, float depth)
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
static float venus_volcano_shape(float x, float z,
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
static float venus_lava_mask(float x, float z)
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
static float terrain_height(const PlanetScene *scene, float x, float z)
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
static Vec3 terrain_normal(const PlanetScene *scene, float x, float z)
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

// Egy pont a folyóban van-e
static bool point_in_river(const PlanetScene *scene, float x, float z)
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

// Földi felszíni világítás beállítása
static void setup_earth_surface_lighting(void)
{
    const GLfloat global_ambient[] = { 0.22f, 0.22f, 0.24f, 1.0f };

    const GLfloat light0_position[] = { -18.0f, 28.0f, 12.0f, 1.0f };
    const GLfloat light0_ambient[]  = { 0.10f, 0.10f, 0.10f, 1.0f };
    const GLfloat light0_diffuse[]  = { 0.92f, 0.90f, 0.82f, 1.0f };
    const GLfloat light0_specular[] = { 0.35f, 0.35f, 0.30f, 1.0f };

    const GLfloat mat_specular[]    = { 0.18f, 0.18f, 0.18f, 1.0f };
    const GLfloat mat_shininess[]   = { 16.0f };

    // Általános világítás
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // Globális környezeti fény
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);

    // Fényparaméterek
    glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  light0_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  light0_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);

    // Anyagszínek követése
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    // Csillanás színe és fényessége
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);

    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
}

// Földi világítás lekapcsolása
static void teardown_earth_surface_lighting(void)
{
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_LIGHT0);
    glDisable(GL_LIGHTING);
}

// Mars köd
static void setup_mars_fog(void)
{
    const GLfloat fog_color[] = { 0.62f, 0.34f, 0.22f, 1.0f };

    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_EXP2);
    glFogf(GL_FOG_DENSITY, 0.060f); // Sűrűség
    glFogfv(GL_FOG_COLOR, fog_color); // Vöröses szín
    glHint(GL_FOG_HINT, GL_NICEST);
}

static void teardown_mars_fog(void)
{
    glDisable(GL_FOG);
}

// Vénusz köd
static void setup_venus_fog(void)
{
    const GLfloat fog_color[] = { 0.78f, 0.42f, 0.12f, 1.0f };

    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_EXP2);
    glFogf(GL_FOG_DENSITY, 0.025f); // Sűrűség
    glFogfv(GL_FOG_COLOR, fog_color); // Sárgás köd
    glHint(GL_FOG_HINT, GL_NICEST);
}

static void teardown_venus_fog(void)
{
    glDisable(GL_FOG);
}

// Egyszerű doboz rajzolása placeholder objektumokhoz
static void draw_box(Vec3 center, Vec3 half)
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

// A környező hegygyűrű kirajzolása
static void draw_mountain_ring(const PlanetScene *scene)
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
static void draw_terrain(const PlanetScene *scene)
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

// Űrhajóhoz speciális bounding box
static void get_ship_bbox(const SurfaceObject *o, Vec3 *center, Vec3 *half)
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
static Vec3 object_bbox_half(const SurfaceObject *o)
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
static Vec3 object_bbox_center(const SurfaceObject *o)
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
static void draw_objects(const PlanetScene *scene)
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

// Eldobott kövek kirajzolása
static void draw_thrown_stones(const PlanetScene *scene)
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
static void draw_river_splashes(const PlanetScene *scene)
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

// Új objektum hozzáadása a jelenethez
static void add_object(PlanetScene *scene,
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
static SurfaceObject *last_object(PlanetScene *scene)
{
    if(scene->object_count <= 0){
        return NULL;
    }
    return &scene->objects[scene->object_count - 1];
}

// A játékoshoz legközelebbi és nézett interaktív objektum keresése
static int find_interactive_index(const PlanetScene *scene, Vec3 pos, Vec3 forward, float max_dist)
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
static bool collides_with_object(const SurfaceObject *o, Vec3 pos, float radius)
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

// Szabad kőslot keresése
static ThrownStone *alloc_stone(PlanetScene *scene)
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
static WaterRipple *alloc_ripple(PlanetScene *scene)
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
static void spawn_ripple(PlanetScene *scene, Vec3 pos, float strength)
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

// Szabad porrészecske keresése
static DustParticle *alloc_dust_particle(PlanetScene *scene)
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
static float frand_range(float a, float b)
{
    return a + (b - a) * ((float)rand() / (float)RAND_MAX);
}

// Porfelhő létrehozása
static void spawn_dust_burst(PlanetScene *scene, Vec3 origin, Vec3 move_dir, int count, float strength)
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
static void update_dust_particles(PlanetScene *scene, float dt)
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
static void draw_dust_particles(const PlanetScene *scene, Vec3 camera_right, Vec3 camera_up)
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
static void spawn_venus_particle(PlanetScene *scene)
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
static void update_venus_particles(PlanetScene *scene, float dt)
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
static void draw_venus_cloud_layer(const PlanetScene *scene)
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
static void draw_venus_lava_cracks(const PlanetScene *scene)
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

// A jelenet alapállapotának beállítása
void planet_scene_init(PlanetScene *scene)
{
    memset(scene, 0, sizeof(*scene)); // Jelenet nullázása
    // Alap beállítások
    scene->terrain_extent = 45.0f;
    scene->eye_height = 1.75f;
    scene->move_speed = 5.0f;
    scene->run_multiplier = 2.0f;
    scene->jump_velocity = 6.5f;
    scene->gravity = 18.0f;
    scene->vertical_velocity = 0.0f;
    scene->grounded = true;

    scene->ground_r = 0.12f;
    scene->ground_g = 0.32f;
    scene->ground_b = 0.12f;

    scene->spawn_position = vec3(0.0f, scene->eye_height, 14.0f);
    scene->spawn_yaw_deg = -90.0f;
    scene->spawn_pitch_deg = 0.0f;
    scene->highlighted_object = -1;
    scene->stones_available = 6;
}

// A kiválasztott bolygó felszíni jelenetének felépítése
void planet_scene_build(PlanetScene *scene, const Planet *planet, int planet_index)
{
    (void)planet; // jelenleg nem használt paraméter

    planet_scene_init(scene); // Alapállapot
    scene->active = true; // Aktív
    scene->planet_index = planet_index; // Melyik bolygó

    // Merkúr felszín
    if(planet_index == 1){
        strcpy(scene->planet_name, "Mercury Surface");

        scene->terrain_extent = 55.0f;
        scene->eye_height = 1.75f;
        scene->move_speed = 3.0f;
        scene->run_multiplier = 1.4f;
        scene->jump_velocity = 3.0f;
        scene->gravity = 6.0f;

        scene->ground_r = 0.46f;
        scene->ground_g = 0.43f;
        scene->ground_b = 0.39f;

        scene->has_river = false;

        add_object(scene, SURFACE_OBJECT_SHIP,
                   0.0f, 5.0f,
                   10.0f, 7.0f, 10.0f,
                   true, true,
                   "Spaceship: click or press E to return.");
        if(last_object(scene)){
            SurfaceObject *ship = last_object(scene);
            float exit_x;
            float exit_z;
            float ship_yaw_rad;

            ship->yaw_deg = 180.0f;
            ship->model_scale = 0.45f;

            ship_yaw_rad = ship->yaw_deg * (float)M_PI / 180.0f;

            // Játékos kezdő pozíciójának kiszámítása
            exit_x = ship->position.x + cosf(ship_yaw_rad) * 6.0f;
            exit_z = ship->position.z + sinf(ship_yaw_rad) * 6.0f;

            scene->spawn_position = vec3(
                exit_x,
                terrain_height(scene, exit_x, exit_z) + scene->eye_height,
                exit_z
            );

            scene->spawn_yaw_deg = ship->yaw_deg + 180.0f;
            scene->spawn_pitch_deg = -5.0f;
        }

        scene->grounded = true;
        scene->vertical_velocity = 0.0f;
        return;
    }

    // Vénusz felszín
    if(planet_index == 2){
        strcpy(scene->planet_name, "Venus Surface");

        scene->terrain_extent = 58.0f;
        scene->eye_height = 1.75f;
        scene->move_speed = 3.4f;
        scene->run_multiplier = 1.35f;
        scene->jump_velocity = 3.2f;
        scene->gravity = 10.5f;

        scene->ground_r = 0.48f;
        scene->ground_g = 0.24f;
        scene->ground_b = 0.10f;

        scene->has_river = false;
        scene->venus_heat = 0.0f;
        scene->venus_heat_timer = 0.0f;
        scene->venus_cloud_time = 0.0f;

        add_object(scene, SURFACE_OBJECT_SHIP,
                   0.0f, 8.0f,
                   10.0f, 7.0f, 10.0f,
                   true, true,
                   "Spaceship: click or press E to return.");
        if(last_object(scene)){
            SurfaceObject *ship = last_object(scene);
            float exit_x;
            float exit_z;
            float ship_yaw_rad;

            ship->yaw_deg = 180.0f;
            ship->model_scale = 0.45f;

            ship_yaw_rad = ship->yaw_deg * (float)M_PI / 180.0f;
            exit_x = ship->position.x + cosf(ship_yaw_rad) * 6.0f;
            exit_z = ship->position.z + sinf(ship_yaw_rad) * 6.0f;

            scene->spawn_position = vec3(
                exit_x,
                terrain_height(scene, exit_x, exit_z) + scene->eye_height,
                exit_z
            );
            scene->spawn_yaw_deg = ship->yaw_deg + 180.0f;
            scene->spawn_pitch_deg = -5.0f;
        }

        scene->grounded = true;
        scene->vertical_velocity = 0.0f;
        return;
    }

    // Mars felszín
    if(planet_index == 4){
        strcpy(scene->planet_name, "Mars Surface");

        scene->terrain_extent = 52.0f;
        scene->eye_height = 1.75f;
        scene->move_speed = 4.0f;
        scene->run_multiplier = 1.6f;
        scene->jump_velocity = 4.2f;
        scene->gravity = 8.5f;

        scene->ground_r = 0.56f;
        scene->ground_g = 0.28f;
        scene->ground_b = 0.18f;

        scene->has_river = false;
        scene->river_x = 0.0f;
        scene->river_half_width = 0.0f;
        scene->river_z_min = 0.0f;
        scene->river_z_max = 0.0f;

        add_object(scene, SURFACE_OBJECT_SHIP,
                   0.0f, 0.0f,
                   10.0f, 7.0f, 10.0f,
                   true, true,
                   "Spaceship: click or press E to return.");
        if(last_object(scene)){
            SurfaceObject *ship = last_object(scene);
            ship->yaw_deg = 180.0f;
            ship->model_scale = 0.45f;
        } 

        add_object(scene, SURFACE_OBJECT_ROVER,
                   7.0f, -3.0f,
                   0.6f, 0.6f, 0.6f,
                   false, true,
                   NULL);

        if(last_object(scene)){
            SurfaceObject *rover = last_object(scene);
            rover->yaw_deg = 35.0f;
            rover->model_scale = 5.0f;
            rover->state = 1.0f;
        }

        scene->spawn_position = vec3(
            0.0f,
            terrain_height(scene, 0.0f, 8.0f) + scene->eye_height,
            8.0f
        );
        scene->spawn_yaw_deg = 180.0f;
        scene->spawn_pitch_deg = -4.0f;

        scene->grounded = true;
        scene->vertical_velocity = 0.0f;
        return;
    }

    // Alapértelmezésben Föld felszín
    strcpy(scene->planet_name, "Earth Surface");

    scene->has_river = true;
    scene->river_x = 7.5f;
    scene->river_half_width = 2.2f;
    scene->river_z_min = -scene->terrain_extent;
    scene->river_z_max =  scene->terrain_extent;

    // Folyó forrásvidékének kövei
    {
        float source_z = scene->river_z_min + 1.0f;
        float source_x = river_center_x(scene, source_z);

        add_object(scene, SURFACE_OBJECT_ROCK,
                   source_x - 1.8f, source_z + 3.0f,
                   0.35f, 0.20f, 0.30f,
                   false, true,
                   NULL);
        if(last_object(scene)){
            last_object(scene)->yaw_deg = 64.0f;
            last_object(scene)->model_scale = 0.01f;
            last_object(scene)->position.y -= 2.0f;
        }

        add_object(scene, SURFACE_OBJECT_ROCK,
                   source_x + 1.6f, source_z + 3.5f,
                   0.32f, 0.18f, 0.28f,
                   false, true,
                   NULL);
        if(last_object(scene)){
            last_object(scene)->yaw_deg = 198.0f;
            last_object(scene)->model_scale = 0.01f;
            last_object(scene)->position.y -= 2.0f;
        }
    }

    // Űrhajó elhelyezése a Földön
    add_object(scene, SURFACE_OBJECT_SHIP,
               -13.5f, 12.0f,
               10.0f, 7.0f, 10.0f,
               true, true,
               "Spaceship: click or press E to return.");
    if(last_object(scene)){
        SurfaceObject *ship = last_object(scene);
        float exit_x;
        float exit_z;
        float ship_yaw_rad;

        ship->yaw_deg = 145.0f;
        ship->model_scale = 0.400f;

        ship_yaw_rad = ship->yaw_deg * (float)M_PI / 180.0f;

        exit_x = ship->position.x + cosf(ship_yaw_rad) * 6.0f;
        exit_z = ship->position.z + sinf(ship_yaw_rad) * 6.0f;

        scene->spawn_position = vec3(
            exit_x,
            terrain_height(scene, exit_x, exit_z) + scene->eye_height,
            exit_z
        );

        scene->spawn_yaw_deg = ship->yaw_deg + 180.0f;
    }

    scene->grounded = true;
    scene->vertical_velocity = 0.0f;

    // Az alábbi rész a Föld objektumait építi fel:
    // sziklás zónák, folyóparti kövek, fák, sűrűbb erdős részek.
    // A struktúra ugyanaz: add_object() + opcionális finomhangolás last_object()-tel.

    add_object(scene, SURFACE_OBJECT_ROCK,
               -24.0f, 18.5f,
               0.20f, 0.12f, 0.18f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 48.0f;
        last_object(scene)->model_scale = 0.0030f;
    }

    add_object(scene, SURFACE_OBJECT_ROCK,
               -21.0f, 21.0f,
               0.22f, 0.13f, 0.20f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 102.0f;
        last_object(scene)->model_scale = 0.0031f;
    }

    add_object(scene, SURFACE_OBJECT_ROCK,
               -17.5f, 18.0f,
               0.21f, 0.12f, 0.19f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 156.0f;
        last_object(scene)->model_scale = 0.0029f;
    }

    add_object(scene, SURFACE_OBJECT_ROCK,
               21.5f, 17.0f,
               0.21f, 0.12f, 0.19f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 188.0f;
        last_object(scene)->model_scale = 0.0030f;
    }

    add_object(scene, SURFACE_OBJECT_ROCK,
               24.0f, 14.5f,
               0.23f, 0.14f, 0.21f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 274.0f;
        last_object(scene)->model_scale = 0.0032f;
    }

    add_object(scene, SURFACE_OBJECT_ROCK,
               18.5f, 20.0f,
               0.22f, 0.13f, 0.20f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 329.0f;
        last_object(scene)->model_scale = 0.0031f;
    }

    add_object(scene, SURFACE_OBJECT_ROCK,
               -23.0f, -8.0f,
               0.22f, 0.13f, 0.20f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 75.0f;
        last_object(scene)->model_scale = 0.0031f;
    }

    add_object(scene, SURFACE_OBJECT_ROCK,
               -17.5f, -14.0f,
               0.21f, 0.12f, 0.19f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 154.0f;
        last_object(scene)->model_scale = 0.0029f;
    }

    add_object(scene, SURFACE_OBJECT_ROCK,
               -21.5f, -12.8f,
               0.20f, 0.12f, 0.18f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 201.0f;
        last_object(scene)->model_scale = 0.0030f;
    }

    add_object(scene, SURFACE_OBJECT_ROCK,
               17.5f, -15.0f,
               0.22f, 0.13f, 0.20f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 266.0f;
        last_object(scene)->model_scale = 0.0031f;
    }

    add_object(scene, SURFACE_OBJECT_ROCK,
               21.0f, -13.0f,
               0.23f, 0.14f, 0.21f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 309.0f;
        last_object(scene)->model_scale = 0.0032f;
    }

    add_object(scene, SURFACE_OBJECT_ROCK,
               14.5f, -18.5f,
               0.21f, 0.12f, 0.19f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 83.0f;
        last_object(scene)->model_scale = 0.0030f;
    }

    add_object(scene, SURFACE_OBJECT_ROCK,
               -8.0f, 2.0f,
               0.21f, 0.12f, 0.19f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 59.0f;
        last_object(scene)->model_scale = 0.0030f;
    }

    add_object(scene, SURFACE_OBJECT_ROCK,
               -5.0f, 0.0f,
               0.20f, 0.12f, 0.18f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 144.0f;
        last_object(scene)->model_scale = 0.0029f;
    }

    add_object(scene, SURFACE_OBJECT_ROCK,
               10.0f, -1.0f,
               0.22f, 0.13f, 0.20f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 221.0f;
        last_object(scene)->model_scale = 0.0031f;
    }

    add_object(scene, SURFACE_OBJECT_ROCK,
               12.5f, 3.5f,
               0.20f, 0.12f, 0.18f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 287.0f;
        last_object(scene)->model_scale = 0.0030f;
    }

    add_object(scene, SURFACE_OBJECT_RIVER_ROCK,
               5.1f, -4.0f,
               5.0f, 3.0f, 4.5f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 75.0f;
        last_object(scene)->model_scale = 0.170f;
    }

    add_object(scene, SURFACE_OBJECT_RIVER_ROCK,
               9.8f, 4.0f,
               5.5f, 3.3f, 5.0f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 260.0f;
        last_object(scene)->model_scale = 0.180f;
    }

    add_object(scene, SURFACE_OBJECT_RIVER_ROCK,
               6.8f, 11.0f,
               5.0f, 3.1f, 4.5f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 140.0f;
        last_object(scene)->model_scale = 0.175f;
    }

    add_object(scene, SURFACE_OBJECT_RIVER_ROCK,
               8.6f, -12.0f,
               6.0f, 3.6f, 5.0f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 310.0f;
        last_object(scene)->model_scale = 0.190f;
    }

    add_object(scene, SURFACE_OBJECT_ROCK,
               3.0f, 16.0f,
               0.20f, 0.12f, 0.18f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 18.0f;
        last_object(scene)->model_scale = 0.0030f;
    }

    add_object(scene, SURFACE_OBJECT_ROCK,
               1.0f, 12.0f,
               0.21f, 0.12f, 0.19f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 93.0f;
        last_object(scene)->model_scale = 0.0031f;
    }

    add_object(scene, SURFACE_OBJECT_ROCK,
               2.5f, -16.5f,
               0.22f, 0.13f, 0.20f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 176.0f;
        last_object(scene)->model_scale = 0.0030f;
    }

    add_object(scene, SURFACE_OBJECT_ROCK,
               5.5f, -19.5f,
               0.20f, 0.12f, 0.18f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 248.0f;
        last_object(scene)->model_scale = 0.0029f;
    }

    add_object(scene, SURFACE_OBJECT_TREE,
               3.5f, 14.5f,
               2.0f, 5.0f, 2.0f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 315.0f;
        last_object(scene)->model_scale = 0.5f;
    }

    add_object(scene, SURFACE_OBJECT_TREE,
               2.0f, 10.5f,
               2.0f, 5.0f, 2.0f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 27.0f;
        last_object(scene)->model_scale = 0.5f;
    }

    add_object(scene, SURFACE_OBJECT_TREE,
               -1.5f, 13.0f,
               2.0f, 5.0f, 2.0f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 74.0f;
        last_object(scene)->model_scale = 0.5f;
    }

    add_object(scene, SURFACE_OBJECT_TREE,
               -2.5f, -15.0f,
               2.0f, 5.0f, 2.0f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 136.0f;
        last_object(scene)->model_scale = 0.5f;
    }

    add_object(scene, SURFACE_OBJECT_TREE,
               14.0f, -11.0f,
               2.1f, 5.2f, 2.1f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 204.0f;
        last_object(scene)->model_scale = 0.48f;
    }

    add_object(scene, SURFACE_OBJECT_TREE,
               -18.0f, 4.0f,
               2.0f, 5.0f, 2.0f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 15.0f;
        last_object(scene)->model_scale = 0.5f;
    }

    add_object(scene, SURFACE_OBJECT_TREE,
               -20.5f, 6.0f,
               2.0f, 5.0f, 2.0f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 58.0f;
        last_object(scene)->model_scale = 0.5f;
    }

    add_object(scene, SURFACE_OBJECT_TREE,
               -16.0f, 6.8f,
               2.0f, 5.0f, 2.0f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 102.0f;
        last_object(scene)->model_scale = 0.5f;
    }

    add_object(scene, SURFACE_OBJECT_TREE,
               -17.0f, 1.5f,
               2.0f, 5.0f, 2.0f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 144.0f;
        last_object(scene)->model_scale = 0.5f;
    }

    add_object(scene, SURFACE_OBJECT_TREE,
               -21.5f, 2.0f,
               2.1f, 5.2f, 2.1f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 201.0f;
        last_object(scene)->model_scale = 0.48f;
    }

    add_object(scene, SURFACE_OBJECT_TREE,
               -22.0f, -6.0f,
               2.2f, 5.5f, 2.2f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 80.0f;
        last_object(scene)->model_scale = 0.5f;
    }

    add_object(scene, SURFACE_OBJECT_TREE,
               -24.0f, -8.5f,
               2.2f, 5.5f, 2.2f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 126.0f;
        last_object(scene)->model_scale = 0.5f;
    }

    add_object(scene, SURFACE_OBJECT_TREE,
               -19.0f, -8.0f,
               2.1f, 5.2f, 2.1f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 24.0f;
        last_object(scene)->model_scale = 0.48f;
    }

    add_object(scene, SURFACE_OBJECT_TREE,
               -21.0f, -16.5f,
               2.2f, 5.4f, 2.2f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 153.0f;
        last_object(scene)->model_scale = 0.5f;
    }

    add_object(scene, SURFACE_OBJECT_TREE,
               -16.0f, -19.5f,
               2.0f, 5.0f, 2.0f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 214.0f;
        last_object(scene)->model_scale = 0.5f;
    }

    add_object(scene, SURFACE_OBJECT_TREE,
               -10.5f, -17.5f,
               2.1f, 5.2f, 2.1f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 271.0f;
        last_object(scene)->model_scale = 0.48f;
    }

    add_object(scene, SURFACE_OBJECT_TREE,
               14.0f, 8.0f,
               2.0f, 5.0f, 2.0f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 140.0f;
        last_object(scene)->model_scale = 0.5f;
    }

    add_object(scene, SURFACE_OBJECT_TREE,
               16.5f, 9.0f,
               2.0f, 5.0f, 2.0f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 205.0f;
        last_object(scene)->model_scale = 0.5f;
    }

    add_object(scene, SURFACE_OBJECT_TREE,
               12.5f, 10.0f,
               2.0f, 5.0f, 2.0f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 276.0f;
        last_object(scene)->model_scale = 0.5f;
    }

    add_object(scene, SURFACE_OBJECT_TREE,
               18.0f, 17.0f,
               2.0f, 5.0f, 2.0f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 302.0f;
        last_object(scene)->model_scale = 0.5f;
    }

    add_object(scene, SURFACE_OBJECT_TREE,
               22.0f, 15.5f,
               2.1f, 5.2f, 2.1f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 17.0f;
        last_object(scene)->model_scale = 0.48f;
    }

    add_object(scene, SURFACE_OBJECT_TREE,
               15.0f, 20.0f,
               2.0f, 5.0f, 2.0f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 88.0f;
        last_object(scene)->model_scale = 0.5f;
    }

    add_object(scene, SURFACE_OBJECT_TREE,
               18.0f, -10.0f,
               2.4f, 5.8f, 2.4f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 210.0f;
        last_object(scene)->model_scale = 0.5f;
    }

    add_object(scene, SURFACE_OBJECT_TREE,
               20.0f, -12.5f,
               2.4f, 5.8f, 2.4f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 248.0f;
        last_object(scene)->model_scale = 0.5f;
    }

    add_object(scene, SURFACE_OBJECT_TREE,
               15.5f, -12.0f,
               2.3f, 5.6f, 2.3f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 176.0f;
        last_object(scene)->model_scale = 0.49f;
    }

    add_object(scene, SURFACE_OBJECT_TREE,
               10.0f, -19.0f,
               2.0f, 5.0f, 2.0f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 327.0f;
        last_object(scene)->model_scale = 0.5f;
    }

    add_object(scene, SURFACE_OBJECT_TREE,
               22.5f, -16.0f,
               2.2f, 5.4f, 2.2f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 36.0f;
        last_object(scene)->model_scale = 0.5f;
    }

    add_object(scene, SURFACE_OBJECT_TREE,
               17.0f, -20.5f,
               2.1f, 5.2f, 2.1f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 95.0f;
        last_object(scene)->model_scale = 0.48f;
    }

    add_object(scene, SURFACE_OBJECT_TREE,
               -6.0f, -2.0f,
               2.0f, 5.0f, 2.0f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 122.0f;
        last_object(scene)->model_scale = 0.5f;
    }

    add_object(scene, SURFACE_OBJECT_TREE,
               -2.0f, -5.0f,
               2.0f, 5.0f, 2.0f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 187.0f;
        last_object(scene)->model_scale = 0.5f;
    }

    add_object(scene, SURFACE_OBJECT_TREE,
               12.0f, 1.5f,
               2.0f, 5.0f, 2.0f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 238.0f;
        last_object(scene)->model_scale = 0.5f;
    }
}

// A felszíni jelenet frissítése: mozgás, fizika, kövek, részecskék, speciális effektek
void planet_scene_update(PlanetScene *scene, Camera *camera, const Input *input, float dt)
{
    Vec3 desired = camera->position;
    Vec3 forward_xz, right_xz;
    float speed = scene->move_speed;
    const float player_radius = 0.45f;
    int i;

    if(!scene->active){
        return;
    }

    // Folyó animációs ideje nő
    g_river_time += dt;

    // Interaktív objektum keresése a kamera közelében
    scene->highlighted_object = find_interactive_index(scene, camera->position, camera->front, 3.0f);

    // Előre/jobbra irány levetítése xz síkra
    forward_xz = vec3_norm(vec3(camera->front.x, 0.0f, camera->front.z));
    right_xz   = vec3_norm(vec3(camera->right.x, 0.0f, camera->right.z));

    if(vec3_len(forward_xz) < 0.0001f) forward_xz = vec3(0.0f, 0.0f, -1.0f);
    if(vec3_len(right_xz)   < 0.0001f) right_xz   = vec3(1.0f, 0.0f, 0.0f);

    // Shift lenyomásával futás
    if(input_key_down(input, SDL_SCANCODE_LSHIFT) || input_key_down(input, SDL_SCANCODE_RSHIFT)){
        speed *= scene->run_multiplier;
    }

    // WASD mozgás
    if(input_key_down(input, SDL_SCANCODE_W)) desired = vec3_add(desired, vec3_scale(forward_xz,  speed * dt));
    if(input_key_down(input, SDL_SCANCODE_S)) desired = vec3_add(desired, vec3_scale(forward_xz, -speed * dt));
    if(input_key_down(input, SDL_SCANCODE_A)) desired = vec3_add(desired, vec3_scale(right_xz,   -speed * dt));
    if(input_key_down(input, SDL_SCANCODE_D)) desired = vec3_add(desired, vec3_scale(right_xz,    speed * dt));

    // Fényerő változtatása
    if(input_key_down(input, SDL_SCANCODE_UP)){
        g_light_intensity += 0.8f * dt;
    }

    if(input_key_down(input, SDL_SCANCODE_DOWN)){
        g_light_intensity -= 0.8f * dt;
    }

    if(g_light_intensity < 0.2f) g_light_intensity = 0.2f;
    if(g_light_intensity > 2.5f) g_light_intensity = 2.5f;

    // Játékos a határokon belül mozoghat
    desired.x = clampf(desired.x, -scene->terrain_extent + 1.5f, scene->terrain_extent - 1.5f);
    desired.z = clampf(desired.z, -scene->terrain_extent + 1.5f, scene->terrain_extent - 1.5f);

    // Ha a kívánt pozíció a folyóba esne, visszavonja az xz mozgást
    if(point_in_river(scene, desired.x, desired.z)){
        desired.x = camera->position.x;
        desired.z = camera->position.z;
    }

    {
        // Hegygyűrűn túl nem engedi ki a játékos
        const float block_r = mountain_collision_radius(scene) - player_radius;
        float dist2 = desired.x * desired.x + desired.z * desired.z;

        if(dist2 > block_r * block_r){
            desired.x = camera->position.x;
            desired.z = camera->position.z;
        }
    }

    // Ütközés ellenőrzése
    for(i = 0; i < scene->object_count; ++i){
        if(collides_with_object(&scene->objects[i], desired, player_radius)){
            desired.x = camera->position.x;
            desired.z = camera->position.z;
            break;
        }
    }

    // Ugrás kezelése
    if(scene->grounded && input_key_pressed(input, SDL_SCANCODE_SPACE)){
        scene->vertical_velocity = scene->jump_velocity;
        scene->grounded = false;

        if(scene->planet_index == 1){
            Vec3 burst_origin = vec3(
                camera->position.x,
                terrain_height(scene, camera->position.x, camera->position.z) + 0.03f,
                camera->position.z
            );
            spawn_dust_burst(scene, burst_origin, forward_xz, 10, 1.0f);
        }
    }

    scene->vertical_velocity -= scene->gravity * dt;

    {
        float ground_y = terrain_height(scene, desired.x, desired.z) + scene->eye_height;
        bool was_grounded = scene->grounded;

        desired.y = camera->position.y + scene->vertical_velocity * dt;

        if(desired.y <= ground_y){
            desired.y = ground_y;
            scene->vertical_velocity = 0.0f;
            scene->grounded = true;

            if(scene->planet_index == 1 && !was_grounded){
                Vec3 burst_origin = vec3(
                    desired.x,
                    terrain_height(scene, desired.x, desired.z) + 0.03f,
                    desired.z
                );
                spawn_dust_burst(scene, burst_origin, forward_xz, 12, 1.15f);
            }
        }else{
            scene->grounded = false;
        }
    }

    // Por részecskék
    if(scene->planet_index == 1 && scene->grounded){
        float move_dx = desired.x - camera->position.x;
        float move_dz = desired.z - camera->position.z;
        float move_len2 = move_dx * move_dx + move_dz * move_dz;

        if(move_len2 > 0.00002f){
            static float dust_accum = 0.0f;
            Vec3 move_dir = vec3_norm(vec3(move_dx, 0.0f, move_dz));

            dust_accum += dt;
            if(dust_accum >= 0.055f){
                Vec3 burst_origin = vec3(
                    desired.x,
                    terrain_height(scene, desired.x, desired.z) + 0.02f,
                    desired.z
                );
                spawn_dust_burst(scene, burst_origin, move_dir, 3, 0.65f);
                dust_accum = 0.0f;
            }
        }
    }

    camera->position = desired;

    // Objektumok animációja
    for(i = 0; i < scene->object_count; ++i){
        SurfaceObject *o = &scene->objects[i];
        if(o->type == SURFACE_OBJECT_SHIP && o->state > 0.0f){
            o->state += dt * 1.6f;
            if(o->state > 1.0f){
                o->state = 1.0f;
            }
        }

        if(o->type == SURFACE_OBJECT_ROVER){
            float speed = 1.2f;
            float min_z = -10.0f;
            float max_z = 6.0f;

            o->position.z += o->state * speed * dt;

            if(o->position.z > max_z){
                o->position.z = max_z;
                o->state = -1.0f;
                o->yaw_deg = 215.0f;
            }
            else if(o->position.z < min_z){
                o->position.z = min_z;
                o->state = 1.0f;
                o->yaw_deg = 35.0f;
            }

            o->position.y = terrain_height(scene, o->position.x, o->position.z) + o->half_extents.y;
        }
    }

    // Hullámok frissítése
    for(i = 0; i < (int)(sizeof(scene->ripples) / sizeof(scene->ripples[0])); ++i){
        WaterRipple *r = &scene->ripples[i];
        if(!r->active){
            continue;
        }

        r->age += dt;
        if(r->age >= r->life){
            r->active = false;
        }
    }

    // Eldobott kövek frissítése
    for(i = 0; i < scene->stone_count; ++i){
        ThrownStone *s = &scene->stones[i];
        if(!s->active){
            continue;
        }

        s->life -= dt;
        if(s->life <= 0.0f){
            s->active = false;
            continue;
        }

        if(!s->splash_done){
            s->velocity.y -= 9.5f * dt;
            s->position = vec3_add(s->position, vec3_scale(s->velocity, dt));

            if(point_in_river(scene, s->position.x, s->position.z)){
                float water_y = river_surface_height(scene, s->position.x, s->position.z);

                if(s->position.y <= water_y){
                    s->position.y = water_y;
                    s->splash_done = true;
                    s->velocity = vec3(0.0f, 0.0f, 0.0f);
                    s->life = 0.45f;

                    spawn_ripple(scene, s->position, 0.07f);
                }
            }else if(s->position.y <= terrain_height(scene, s->position.x, s->position.z) + 0.12f){
                s->active = false;
            }
        }
    }

    // Vénusz speciális effektjei
    if(scene->planet_index == 2){
        scene->venus_cloud_time += dt;
        update_venus_particles(scene, dt);

        scene->venus_heat += dt * 0.22f;
        if(scene->venus_heat > 1.0f){
            scene->venus_heat = 1.0f;
        }

        scene->venus_heat_timer += dt;
        if(scene->venus_heat_timer >= 2.5f){
            scene->venus_heat_timer = 0.0f;
            snprintf(scene->interaction_message,
                     sizeof(scene->interaction_message),
                     "WARNING: Extreme heat and a toxic atmosphere on Venus!");
        }
    }

    update_dust_particles(scene, dt);
    g_cam_right = camera->right;
    g_cam_up    = camera->up;
}

// Renderelés külső kamerával
void planet_scene_render_with_camera(const PlanetScene *scene, const Camera *camera)
{
    if(!scene->active){
        return;
    }

    if(scene->planet_index == 1){

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const GLfloat global_ambient[] = { 0.05f, 0.05f, 0.05f, 1.0f };
        const GLfloat light0_position[] = { 80.0f, 55.0f, 25.0f, 1.0f };
        const GLfloat light0_ambient[]  = { 0.04f, 0.04f, 0.04f, 1.0f };
        const GLfloat light0_diffuse[]  = { 1.05f, 1.00f, 0.90f, 1.0f };
        const GLfloat light0_specular[] = { 0.35f, 0.33f, 0.30f, 1.0f };
        const GLfloat mat_specular[]    = { 0.08f, 0.08f, 0.08f, 1.0f };
        const GLfloat mat_shininess[]   = { 6.0f };

        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glEnable(GL_NORMALIZE);
        glShadeModel(GL_SMOOTH);

        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);
        glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
        glLightfv(GL_LIGHT0, GL_AMBIENT,  light0_ambient);
        glLightfv(GL_LIGHT0, GL_DIFFUSE,  light0_diffuse);
        glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);

        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);

        draw_terrain(scene);
        draw_objects(scene);
        draw_dust_particles(scene, camera->right, camera->up);

        glDisable(GL_COLOR_MATERIAL);
        glDisable(GL_LIGHT0);
        glDisable(GL_LIGHTING);
        return;
    }

    glClearColor(0.55f, 0.75f, 0.95f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setup_earth_surface_lighting();

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    {
        GLfloat light_pos[] = { 50.0f, 80.0f, 50.0f, 1.0f };
        float i_light = g_light_intensity;
        GLfloat ambient[]  = { 0.2f * i_light, 0.2f * i_light, 0.2f * i_light, 1.0f };
        GLfloat diffuse[]  = { 0.9f * i_light, 0.9f * i_light, 0.85f * i_light, 1.0f };
        GLfloat specular[] = { 1.0f * i_light, 1.0f * i_light, 1.0f * i_light, 1.0f };
        GLfloat mat_spec[] = { 0.3f, 0.3f, 0.3f, 1.0f };

        glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
        glLightfv(GL_LIGHT0, GL_AMBIENT,  ambient);
        glLightfv(GL_LIGHT0, GL_DIFFUSE,  diffuse);
        glLightfv(GL_LIGHT0, GL_SPECULAR, specular);

        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_spec);
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 16.0f);
    }

    draw_terrain(scene);
    draw_mountain_ring(scene);
    draw_objects(scene);
    draw_thrown_stones(scene);
    draw_river(scene);
    draw_river_splashes(scene);

    teardown_earth_surface_lighting();
}

// A normál felszíni render függvény
void planet_scene_render(const PlanetScene *scene)
{
    int i;

    if(!scene->active){
        return;
    }

    // Bolygónként eltérő háttér és köd
    if(scene->planet_index == 2){
        glClearColor(0.78f, 0.42f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        setup_venus_fog();
    }else if(scene->planet_index == 4){
        glClearColor(0.62f, 0.34f, 0.22f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        setup_mars_fog();
    }else{
        teardown_venus_fog();
        teardown_mars_fog();
    }

    // Merkúr speciális sötét render
    if(scene->planet_index == 1){
        const GLfloat global_ambient[] = { 0.05f, 0.05f, 0.05f, 1.0f };
        const GLfloat light0_position[] = { 80.0f, 55.0f, 25.0f, 1.0f };
        const GLfloat light0_ambient[]  = { 0.04f, 0.04f, 0.04f, 1.0f };
        const GLfloat light0_diffuse[]  = { 1.05f, 1.00f, 0.90f, 1.0f };
        const GLfloat light0_specular[] = { 0.35f, 0.33f, 0.30f, 1.0f };
        const GLfloat mat_specular[]    = { 0.08f, 0.08f, 0.08f, 1.0f };
        const GLfloat mat_shininess[]   = { 6.0f };

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glEnable(GL_NORMALIZE);
        glShadeModel(GL_SMOOTH);

        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);
        glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
        glLightfv(GL_LIGHT0, GL_AMBIENT,  light0_ambient);
        glLightfv(GL_LIGHT0, GL_DIFFUSE,  light0_diffuse);
        glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);

        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);

        draw_terrain(scene);

        // Űrhajó árnyékának rajzolása a talajra
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        for(i = 0; i < scene->object_count; ++i){
            const SurfaceObject *o = &scene->objects[i];

            if(o->type != SURFACE_OBJECT_SHIP){
                continue;
            }

            {
                int a;
                const int shadow_segments = 40;
                float sx = o->position.x - 0.75f;
                float sz = o->position.z + 0.15f;
                float sy = -0.20f;
                float rx = 3.0f;
                float rz = 1.9f;

                glColor4f(0.02f, 0.02f, 0.02f, 0.42f);

                glBegin(GL_TRIANGLE_FAN);
                glVertex3f(sx, sy, sz);

                for(a = 0; a <= shadow_segments; ++a){
                    float ang = ((float)a / (float)shadow_segments) * 2.0f * (float)M_PI;
                    float px = sx + cosf(ang) * rx;
                    float pz = sz + sinf(ang) * rz;

                    glVertex3f(px, sy, pz);
                }
                glEnd();
            }
        }

        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);

        glEnable(GL_LIGHTING);
        draw_objects(scene);
        draw_dust_particles(scene, g_cam_right, g_cam_up);

        glDisable(GL_COLOR_MATERIAL);
        glDisable(GL_LIGHT0);
        glDisable(GL_LIGHTING);
        return;
    }

    // Alap világítás a többi bolygóra
    setup_earth_surface_lighting();

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    {
        GLfloat light_pos[] = { 50.0f, 80.0f, 50.0f, 1.0f };
        float i_light = g_light_intensity;
        GLfloat ambient[]  = { 0.2f * i_light, 0.2f * i_light, 0.2f * i_light, 1.0f };
        GLfloat diffuse[]  = { 0.9f * i_light, 0.9f * i_light, 0.85f * i_light, 1.0f };
        GLfloat specular[] = { 1.0f * i_light, 1.0f * i_light, 1.0f * i_light, 1.0f };
        GLfloat mat_spec[] = { 0.3f, 0.3f, 0.3f, 1.0f };

        glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
        glLightfv(GL_LIGHT0, GL_AMBIENT,  ambient);
        glLightfv(GL_LIGHT0, GL_DIFFUSE,  diffuse);
        glLightfv(GL_LIGHT0, GL_SPECULAR, specular);

        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_spec);
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 16.0f);
    }

    draw_terrain(scene);

    if(scene->planet_index == 2){
        draw_venus_lava_cracks(scene);
    }

    draw_mountain_ring(scene);
    draw_objects(scene);
    draw_thrown_stones(scene);
    draw_river(scene);
    draw_river_splashes(scene);

    if(scene->planet_index == 2){
        draw_venus_cloud_layer(scene);
    }

    draw_dust_particles(scene, g_cam_right, g_cam_up);
    teardown_earth_surface_lighting();

    if(scene->planet_index == 2){
        teardown_venus_fog();
    }else if(scene->planet_index == 4){
        teardown_mars_fog();
    }
}

// Interakció E billentyűvel
const char *planet_scene_interact(PlanetScene *scene, Vec3 camera_position)
{
    int idx;

    if(!scene->active){
        return NULL;
    }

    idx = find_interactive_index(scene, camera_position, vec3(0.0f, 0.0f, -1.0f), 3.0f);
    if(idx < 0 && scene->highlighted_object >= 0){
        idx = scene->highlighted_object;
    }
    if(idx < 0){
        return NULL;
    }

    if(scene->objects[idx].type == SURFACE_OBJECT_SHIP){
        scene->objects[idx].state = 0.05f;
        scene->exit_requested = true;
        snprintf(scene->interaction_message,
                 sizeof(scene->interaction_message),
                 "The spaceship door opened. Returning to the overview.");
        return scene->interaction_message;
    }

    if(scene->objects[idx].type == SURFACE_OBJECT_CRATE){
        scene->stones_available += 3;
        if(scene->stones_available > 12){
            scene->stones_available = 12;
        }
        snprintf(scene->interaction_message,
                 sizeof(scene->interaction_message),
                 "You picked up some stones. Throwable stones: %d",
                 scene->stones_available);
        return scene->interaction_message;
    }

    snprintf(scene->interaction_message,
             sizeof(scene->interaction_message),
             "There is no special interaction for this object.");
    return scene->interaction_message;
}

// Kattintásos interakció
const char *planet_scene_handle_click(PlanetScene *scene, Vec3 camera_position, Vec3 camera_forward)
{
    int idx;

    if(!scene->active){
        return NULL;
    }

    idx = find_interactive_index(scene, camera_position, camera_forward, 5.0f);
    if(idx < 0){
        return NULL;
    }

    if(scene->objects[idx].type == SURFACE_OBJECT_SHIP){
        scene->objects[idx].state = 0.05f;
        scene->exit_requested = true;

        snprintf(scene->interaction_message,
                 sizeof(scene->interaction_message),
                 "You clicked on the spaceship. Returning to the overview.");

        return scene->interaction_message;
    }

    return NULL;
}

// Kődobás
const char *planet_scene_throw_stone(PlanetScene *scene, Vec3 camera_position, Vec3 camera_forward)
{
    ThrownStone *s;
    Vec3 dir;

    if(!scene->active || scene->planet_index != 3){
        return NULL;
    }

    if(scene->stones_available <= 0){
        snprintf(scene->interaction_message,
                 sizeof(scene->interaction_message),
                 "You do not have any stones.");
        return scene->interaction_message;
    }

    s = alloc_stone(scene);
    if(!s){
        snprintf(scene->interaction_message,
                 sizeof(scene->interaction_message),
                 "There are too many stones in the air right now.");
        return scene->interaction_message;
    }

    // Kamera nézési irányának normalizálása
    dir = vec3_norm(camera_forward);
    if(vec3_len(dir) < 0.0001f){
        dir = vec3(0.0f, 0.0f, -1.0f);
    }

    // Kő létrehozása
    s->active = true;
    s->splash_done = false;
    s->position = vec3_add(camera_position, vec3(0.0f, -0.15f, 0.0f));
    s->velocity = vec3_add(vec3_scale(dir, 8.0f), vec3(0.0f, 3.6f, 0.0f));
    s->life = 3.0f;

    scene->stones_available--;

    snprintf(scene->interaction_message,
             sizeof(scene->interaction_message),
             "Stone thrown. Remaining: %d",
             scene->stones_available);

    return scene->interaction_message;
}

// Jelzi, hogy ki kell-e lépni a felszíni nézetből
bool planet_scene_should_exit(const PlanetScene *scene)
{
    return scene->exit_requested;
}