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

#define COLLISION_SCALE_XZ 0.4f
#define COLLISION_SCALE_Y  0.6f

typedef struct SurfaceAsset{
    Mesh mesh;
    Texture2D texture;
    bool loaded;
    bool texture_loaded;
    GLuint list_id;
} SurfaceAsset;

typedef struct SurfaceAssets{
    bool tried;
    SurfaceAsset ship;
    SurfaceAsset river_rock;
    SurfaceAsset wild_rock;
    SurfaceAsset throw_rock;
} SurfaceAssets;

static SurfaceAssets g_assets;

static void asset_try_load(SurfaceAsset *asset,
                           const char *obj_path_a,
                           const char *obj_path_b,
                           const char *tex_path)
{
    memset(asset, 0, sizeof(*asset));

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
}

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

static void draw_asset(const SurfaceAsset *asset,
                       Vec3 position,
                       float yaw_deg,
                       float scale)
{
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
        glColor3f(1.0f, 1.0f, 1.0f);
    }else{
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.8f, 0.8f, 0.8f);
    }

    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);
    glRotatef(yaw_deg, 0.0f, 1.0f, 0.0f);
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

static float terrain_height(const PlanetScene *scene, float x, float z)
{
    float hills = 0.40f * sinf(x * 0.08f) + 0.24f * cosf(z * 0.07f);
    float detail = 0.10f * sinf((x + z) * 0.16f);
    float h = hills + detail;

    if(scene->has_river){
        float dx = (x - scene->river_x) / (scene->river_half_width + 0.25f);
        float river_cut = expf(-(dx * dx) * 1.8f) * 1.2f;
        h -= river_cut;
    }

    return h;
}

static bool point_in_river(const PlanetScene *scene, float x, float z)
{
    if(!scene->has_river){
        return false;
    }
    if(z < scene->river_z_min || z > scene->river_z_max){
        return false;
    }
    return fabsf(x - scene->river_x) < scene->river_half_width;
}

static void draw_box(Vec3 center, Vec3 half)
{
    float x0 = center.x - half.x;
    float x1 = center.x + half.x;
    float y0 = center.y - half.y;
    float y1 = center.y + half.y;
    float z0 = center.z - half.z;
    float z1 = center.z + half.z;

    glDisable(GL_TEXTURE_2D);
    glColor3f(0.65f, 0.65f, 0.65f);

    glBegin(GL_QUADS);

    glVertex3f(x0,y0,z1); glVertex3f(x1,y0,z1); glVertex3f(x1,y1,z1); glVertex3f(x0,y1,z1);
    glVertex3f(x1,y0,z0); glVertex3f(x0,y0,z0); glVertex3f(x0,y1,z0); glVertex3f(x1,y1,z0);

    glVertex3f(x0,y0,z0); glVertex3f(x0,y0,z1); glVertex3f(x0,y1,z1); glVertex3f(x0,y1,z0);
    glVertex3f(x1,y0,z1); glVertex3f(x1,y0,z0); glVertex3f(x1,y1,z0); glVertex3f(x1,y1,z1);

    glVertex3f(x0,y1,z1); glVertex3f(x1,y1,z1); glVertex3f(x1,y1,z0); glVertex3f(x0,y1,z0);
    glVertex3f(x0,y0,z0); glVertex3f(x1,y0,z0); glVertex3f(x1,y0,z1); glVertex3f(x0,y0,z1);

    glEnd();
}

static void draw_bbox_wire(Vec3 center, Vec3 half, float r, float g, float b)
{
    float x0 = center.x - half.x;
    float x1 = center.x + half.x;
    float y0 = center.y - half.y;
    float y1 = center.y + half.y;
    float z0 = center.z - half.z;
    float z1 = center.z + half.z;

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glLineWidth(3.0f);
    glColor3f(r, g, b);

    glBegin(GL_LINES);

    glVertex3f(x0,y0,z0); glVertex3f(x1,y0,z0);
    glVertex3f(x1,y0,z0); glVertex3f(x1,y0,z1);
    glVertex3f(x1,y0,z1); glVertex3f(x0,y0,z1);
    glVertex3f(x0,y0,z1); glVertex3f(x0,y0,z0);

    glVertex3f(x0,y1,z0); glVertex3f(x1,y1,z0);
    glVertex3f(x1,y1,z0); glVertex3f(x1,y1,z1);
    glVertex3f(x1,y1,z1); glVertex3f(x0,y1,z1);
    glVertex3f(x0,y1,z1); glVertex3f(x0,y1,z0);

    glVertex3f(x0,y0,z0); glVertex3f(x0,y1,z0);
    glVertex3f(x1,y0,z0); glVertex3f(x1,y1,z0);
    glVertex3f(x1,y0,z1); glVertex3f(x1,y1,z1);
    glVertex3f(x0,y0,z1); glVertex3f(x0,y1,z1);

    glEnd();

    glLineWidth(1.0f);
    glEnable(GL_DEPTH_TEST);
}

static void draw_river(const PlanetScene *scene)
{
    int i;
    const int segments = 80;
    float z0, z1, left, right, step;

    if(!scene->has_river){
        return;
    }

    z0 = scene->river_z_min;
    z1 = scene->river_z_max;
    left = scene->river_x - scene->river_half_width;
    right = scene->river_x + scene->river_half_width;
    step = (z1 - z0) / (float)segments;

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for(i = 0; i < segments; ++i){
        float za = z0 + (float)i * step;
        float zb = za + step;
        float ya0 = terrain_height(scene, left, za) + 0.05f;
        float ya1 = terrain_height(scene, right, za) + 0.05f;
        float yb0 = terrain_height(scene, left, zb) + 0.05f;
        float yb1 = terrain_height(scene, right, zb) + 0.05f;

        glBegin(GL_QUADS);
        glColor4f(0.10f, 0.28f, 0.62f, 0.82f);
        glVertex3f(left,  ya0, za);
        glVertex3f(right, ya1, za);
        glColor4f(0.18f, 0.46f, 0.78f, 0.82f);
        glVertex3f(right, yb1, zb);
        glVertex3f(left,  yb0, zb);
        glEnd();
    }

    glDisable(GL_BLEND);
}

static void draw_terrain(const PlanetScene *scene)
{
    const int steps = 72;
    const float extent = scene->terrain_extent;
    const float step = (extent * 2.0f) / (float)steps;
    int iz, ix;

    glDisable(GL_TEXTURE_2D);

    for(iz = 0; iz < steps; ++iz){
        float z0 = -extent + (float)iz * step;
        float z1 = z0 + step;

        glBegin(GL_TRIANGLE_STRIP);
        for(ix = 0; ix <= steps; ++ix){
            float x = -extent + (float)ix * step;
            float y0 = terrain_height(scene, x, z0);
            float y1 = terrain_height(scene, x, z1);

            glColor3f(scene->ground_r * 0.9f, scene->ground_g * 0.9f, scene->ground_b * 0.9f);
            glVertex3f(x, y0, z0);

            glColor3f(scene->ground_r, scene->ground_g, scene->ground_b);
            glVertex3f(x, y1, z1);
        }
        glEnd();
    }
}

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

        case SURFACE_OBJECT_SHIP:
            return vec3(
                o->half_extents.x * 0.22f,
                o->half_extents.y * 0.33f,
                o->half_extents.z * 0.22f
            );

        default:
            return vec3(
                o->half_extents.x * COLLISION_SCALE_XZ,
                o->half_extents.y * COLLISION_SCALE_Y,
                o->half_extents.z * COLLISION_SCALE_XZ
            );
    }
}

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

        case SURFACE_OBJECT_SHIP:
            c.y -= o->half_extents.y * 0.60f;
            break;

        default:
            break;
    }

    return c;
}

static void draw_objects(const PlanetScene *scene)
{
    int i;

    ensure_assets_loaded();

    for(i = 0; i < scene->object_count; ++i){
        const SurfaceObject *o = &scene->objects[i];
        Vec3 bbox_center;
        Vec3 bbox_half;

        if(o->type == SURFACE_OBJECT_SHIP){
            get_ship_bbox(o, &bbox_center, &bbox_half);
        }else{
            bbox_center = object_bbox_center(o);
            bbox_half = object_bbox_half(o);
        }

        switch(o->type){
            case SURFACE_OBJECT_SHIP:
                draw_asset(&g_assets.ship,
                           vec3(o->position.x, o->position.y - o->half_extents.y, o->position.z),
                           o->yaw_deg,
                           o->model_scale);
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

            case SURFACE_OBJECT_CRATE:
            case SURFACE_OBJECT_LOG:
            default:
                draw_box(o->position, o->half_extents);
                break;
        }

        if(o->interactive){
            draw_bbox_wire(bbox_center, bbox_half, 1.0f, 1.0f, 0.0f);
        }else if(o->blocks_movement){
            draw_bbox_wire(bbox_center, bbox_half, 1.0f, 0.0f, 0.0f);
        }else{
            draw_bbox_wire(bbox_center, bbox_half, 0.0f, 1.0f, 1.0f);
        }
    }
}

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

        draw_bbox_wire(s->position, vec3(0.08f, 0.06f, 0.08f), 0.0f, 1.0f, 0.0f);
    }
}

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

static SurfaceObject *last_object(PlanetScene *scene)
{
    if(scene->object_count <= 0){
        return NULL;
    }
    return &scene->objects[scene->object_count - 1];
}

static int find_interactive_index(const PlanetScene *scene, Vec3 pos, Vec3 forward, float max_dist)
{
    int i;
    int best = -1;
    float best_score = -1000.0f;
    Vec3 look = vec3_norm(vec3(forward.x, 0.0f, forward.z));

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

    dx = fabsf(pos.x - c.x);
    dz = fabsf(pos.z - c.z);

    return dx < (h.x + radius) &&
           dz < (h.z + radius);
}

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

void planet_scene_init(PlanetScene *scene)
{
    memset(scene, 0, sizeof(*scene));
    scene->terrain_extent = 45.0f;
    scene->eye_height = 1.75f;
    scene->move_speed = 5.0f;
    scene->run_multiplier = 2.0f;
    scene->jump_velocity = 6.5f;
    scene->gravity = 18.0f;
    scene->vertical_velocity = 0.0f;
    scene->grounded = true;

    scene->ground_r = 0.23f;
    scene->ground_g = 0.56f;
    scene->ground_b = 0.20f;

    scene->spawn_position = vec3(0.0f, scene->eye_height, 14.0f);
    scene->spawn_yaw_deg = -90.0f;
    scene->spawn_pitch_deg = 0.0f;
    scene->highlighted_object = -1;
    scene->stones_available = 6;
}

void planet_scene_build(PlanetScene *scene, const Planet *planet, int planet_index)
{
    (void)planet;

    planet_scene_init(scene);
    scene->active = true;
    scene->planet_index = planet_index;

    strcpy(scene->planet_name, "Earth Surface");

    scene->has_river = true;
    scene->river_x = 7.5f;
    scene->river_half_width = 2.2f;
    scene->river_z_min = -26.0f;
    scene->river_z_max = 26.0f;

    scene->spawn_position = vec3(-28.0f,
                                 terrain_height(scene, -28.0f, 22.0f) + scene->eye_height,
                                 22.0f);
    scene->spawn_yaw_deg = -20.0f;
    scene->grounded = true;
    scene->vertical_velocity = 0.0f;

    add_object(scene, SURFACE_OBJECT_SHIP,
               -13.5f, 12.0f,
               10.0f, 7.0f, 10.0f,
               true, true,
               "Urhajo: kattints ra vagy nyomj E-t a visszatereshez.");
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 145.0f;
        last_object(scene)->model_scale = 0.400f;
    }

    add_object(scene, SURFACE_OBJECT_ROCK,
               -2.0f, 14.0f,
               0.20f, 0.12f, 0.18f,
               false, true,
               NULL);
    if(last_object(scene)){
        last_object(scene)->yaw_deg = 22.0f;
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
}

void planet_scene_update(PlanetScene *scene, Camera *camera, const Input *input, float dt)
{
    Vec3 desired = camera->position;
    Vec3 forward_xz, right_xz;
    float speed = scene->move_speed;
    const float player_radius = 0.55f;
    int i;

    if(!scene->active){
        return;
    }

    scene->highlighted_object = find_interactive_index(scene, camera->position, camera->front, 3.0f);

    forward_xz = vec3_norm(vec3(camera->front.x, 0.0f, camera->front.z));
    right_xz   = vec3_norm(vec3(camera->right.x, 0.0f, camera->right.z));

    if(vec3_len(forward_xz) < 0.0001f) forward_xz = vec3(0.0f, 0.0f, -1.0f);
    if(vec3_len(right_xz)   < 0.0001f) right_xz   = vec3(1.0f, 0.0f, 0.0f);

    if(input_key_down(input, SDL_SCANCODE_LSHIFT) || input_key_down(input, SDL_SCANCODE_RSHIFT)){
        speed *= scene->run_multiplier;
    }

    if(input_key_down(input, SDL_SCANCODE_W)) desired = vec3_add(desired, vec3_scale(forward_xz,  speed * dt));
    if(input_key_down(input, SDL_SCANCODE_S)) desired = vec3_add(desired, vec3_scale(forward_xz, -speed * dt));
    if(input_key_down(input, SDL_SCANCODE_A)) desired = vec3_add(desired, vec3_scale(right_xz,   -speed * dt));
    if(input_key_down(input, SDL_SCANCODE_D)) desired = vec3_add(desired, vec3_scale(right_xz,    speed * dt));

    desired.x = clampf(desired.x, -scene->terrain_extent + 1.5f, scene->terrain_extent - 1.5f);
    desired.z = clampf(desired.z, -scene->terrain_extent + 1.5f, scene->terrain_extent - 1.5f);

    if(point_in_river(scene, desired.x, desired.z)){
        desired.x = camera->position.x;
        desired.z = camera->position.z;
    }

    for(i = 0; i < scene->object_count; ++i){
        if(collides_with_object(&scene->objects[i], desired, player_radius)){
            desired.x = camera->position.x;
            desired.z = camera->position.z;
            break;
        }
    }

    if(scene->grounded && input_key_pressed(input, SDL_SCANCODE_SPACE)){
        scene->vertical_velocity = scene->jump_velocity;
        scene->grounded = false;
    }

    scene->vertical_velocity -= scene->gravity * dt;

    {
        float ground_y = terrain_height(scene, desired.x, desired.z) + scene->eye_height;
        desired.y = camera->position.y + scene->vertical_velocity * dt;

        if(desired.y <= ground_y){
            desired.y = ground_y;
            scene->vertical_velocity = 0.0f;
            scene->grounded = true;
        }else{
            scene->grounded = false;
        }
    }

    camera->position = desired;

    for(i = 0; i < scene->object_count; ++i){
        SurfaceObject *o = &scene->objects[i];
        if(o->type == SURFACE_OBJECT_SHIP && o->state > 0.0f){
            o->state += dt * 1.6f;
            if(o->state > 1.0f){
                o->state = 1.0f;
            }
        }
    }

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
                float water_y = terrain_height(scene, s->position.x, s->position.z) + 0.08f;
                if(s->position.y <= water_y){
                    s->position.y = water_y;
                    s->splash_done = true;
                    s->velocity = vec3(0.0f, 0.0f, 0.0f);
                    s->life = 0.5f;
                }
            }else if(s->position.y <= terrain_height(scene, s->position.x, s->position.z) + 0.12f){
                s->active = false;
            }
        }
    }
}

void planet_scene_render(const PlanetScene *scene)
{
    if(!scene->active){
        return;
    }

    draw_terrain(scene);
    draw_river(scene);
    draw_objects(scene);
    draw_thrown_stones(scene);
}

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
        snprintf(scene->interaction_message, sizeof(scene->interaction_message),
                 "Az urhajo ajtaja kinyilt. Visszatersz az attekinto nezetbe.");
        return scene->interaction_message;
    }

    if(scene->objects[idx].type == SURFACE_OBJECT_CRATE){
        scene->stones_available += 3;
        if(scene->stones_available > 12){
            scene->stones_available = 12;
        }
        snprintf(scene->interaction_message, sizeof(scene->interaction_message),
                 "Felvettel nehany kavicsot. Dobhato kovek: %d", scene->stones_available);
        return scene->interaction_message;
    }

    return NULL;
}

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
        snprintf(scene->interaction_message, sizeof(scene->interaction_message),
                 "Az urhajora kattintottal, az ajto kinyilt.");
        return scene->interaction_message;
    }

    return NULL;
}

const char *planet_scene_throw_stone(PlanetScene *scene, Vec3 camera_position, Vec3 camera_forward)
{
    ThrownStone *s;
    Vec3 dir;

    if(!scene->active || scene->planet_index != 3){
        return NULL;
    }

    if(scene->stones_available <= 0){
        snprintf(scene->interaction_message, sizeof(scene->interaction_message),
                 "Nincs nalad kavics. Nezz korul a lada kozeleben.");
        return scene->interaction_message;
    }

    s = alloc_stone(scene);
    if(!s){
        snprintf(scene->interaction_message, sizeof(scene->interaction_message),
                 "Most tul sok ko van a levegoben.");
        return scene->interaction_message;
    }

    dir = vec3_norm(camera_forward);
    if(vec3_len(dir) < 0.0001f){
        dir = vec3(0.0f, 0.0f, -1.0f);
    }

    s->active = true;
    s->splash_done = false;
    s->position = vec3_add(camera_position, vec3(0.0f, -0.15f, 0.0f));
    s->velocity = vec3_add(vec3_scale(dir, 8.0f), vec3(0.0f, 3.6f, 0.0f));
    s->life = 3.0f;

    scene->stones_available--;

    snprintf(scene->interaction_message, sizeof(scene->interaction_message),
             "Kavics eldobva. Maradt: %d", scene->stones_available);
    return scene->interaction_message;
}

bool planet_scene_should_exit(const PlanetScene *scene)
{
    return scene->exit_requested;
}

const char *planet_scene_get_prompt(const PlanetScene *scene, Vec3 camera_position, Vec3 camera_forward)
{
    static char prompt[160];
    int idx;

    if(!scene->active){
        return NULL;
    }

    idx = find_interactive_index(scene, camera_position, camera_forward, 3.0f);
    if(idx < 0){
        if(scene->stones_available > 0){
            snprintf(prompt, sizeof(prompt), "Bal klikk: kavicsdobas a folyoba (%d)", scene->stones_available);
            return prompt;
        }
        return NULL;
    }

    switch(scene->objects[idx].type){
        case SURFACE_OBJECT_SHIP:
            snprintf(prompt, sizeof(prompt), "E vagy bal klikk: urhajo ajto nyitasa es visszateres");
            return prompt;
        case SURFACE_OBJECT_CRATE:
            snprintf(prompt, sizeof(prompt), "E: kovek felvetele a ladabol (%d)", scene->stones_available);
            return prompt;
        default:
            snprintf(prompt, sizeof(prompt), "E: interakcio");
            return prompt;
    }
}