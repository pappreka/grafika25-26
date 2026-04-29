#include "planet_scene.h"
#include "planet_scene_internal.h"
#include "mesh.h"
#include "texture.h"

#include <string.h>
#include <math.h>

#if defined(_WIN32)
#include <windows.h>
#endif

#include <GL/gl.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

SurfaceAssets g_assets;

// Egy asset betöltése két lehetséges OBJ útvonalból
void asset_try_load(SurfaceAsset *asset,
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
void ensure_assets_loaded(void)
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
void compile_asset_list(SurfaceAsset *asset)
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
void draw_asset(const SurfaceAsset *asset,
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
void draw_rover_asset(const SurfaceAsset *asset,
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