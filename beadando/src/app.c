#include "app.h"
#include <stdio.h>
#include <math.h>

#if defined(_WIN32)
    #include <windows.h>
#endif

#include <GL/gl.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static float seconds_from_ticks(unsigned int ticks){
    return (float)ticks / 1000.0f;
}

static float lerpf(float a, float b, float t){
    return a + (b - a) * t;
}

static Vec3 vec3_lerp(Vec3 a, Vec3 b, float t){
    return vec3(lerpf(a.x, b.x, t), lerpf(a.y, b.y, t), lerpf(a.z, b.z, t));
}

static void update_jump_physics(App *app, float dt){
    const float gravity = 18.0f;
    const float jump_speed = 7.0f;
    const float floor_eye_y = app->ground_y + app->eye_height;

    if(input_key_pressed(&app->input, SDL_SCANCODE_SPACE) && app->on_ground){
        app->vertical_velocity = jump_speed;
        app->on_ground = false;
    }

    app->vertical_velocity -= gravity * dt;
    app->camera.position.y += app->vertical_velocity * dt;

    if(app->camera.position.y <= floor_eye_y){
        app->camera.position.y = floor_eye_y;
        app->vertical_velocity = 0.0f;
        app->on_ground = true;
    }
}

static void draw_mesh_textured(const Mesh *m, unsigned int tex_id){
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, (GLuint)tex_id);

    glColor3f(1.0f, 1.0f, 1.0f);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glBegin(GL_TRIANGLES);
    for(int i = 0; i < m->vert_count; i++){
        const Vertex *v = &m->verts[i];
        glTexCoord2f(v->u, v->v);
        glNormal3f(v->nx, v->ny, v->nz);
        glVertex3f(v->px, v->py, v->pz);
    }
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}

static void draw_sky_sphere(const App *app){
    if(app->stars_texture.id == 0){
        return;
    }

    glPushMatrix();

    glTranslatef(app->camera.position.x,
                 app->camera.position.y,
                 app->camera.position.z);

    glScalef(5000.0f, 5000.0f, 5000.0f);

    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glDepthMask(GL_FALSE);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, (GLuint)app->stars_texture.id);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor3f(1.0f, 1.0f, 1.0f);

    glBegin(GL_TRIANGLES);
    for(int i = 0; i < app->sphere_mesh.vert_count; i++){
        const Vertex *v = &app->sphere_mesh.verts[i];
        glTexCoord2f(1.0f - v->u, v->v);
        glVertex3f(v->px, v->py, v->pz);
    }
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);

    glPopMatrix();
}

static void draw_planet_ring(const Planet *p){
    if(!p->has_ring || p->ring_texture.id == 0){
        return;
    }

    const int segments = 96;
    const float inner_r = p->ring_inner_radius;
    const float outer_r = p->ring_outer_radius;

    glPushMatrix();
    glTranslatef(p->position.x, p->position.y, p->position.z);
    glRotatef(p->ring_tilt_deg, 0.0f, 0.0f, 1.0f);
    glRotatef(p->rotation_angle * 0.15f, 0.0f, 1.0f, 0.0f);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, (GLuint)p->ring_texture.id);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    glBegin(GL_QUAD_STRIP);
    for(int i = 0; i <= segments; i++){
        float a = ((float)i / (float)segments) * 2.0f * (float)M_PI;
        float ca = cosf(a);
        float sa = sinf(a);
        float u = (float)i / (float)segments;

        glNormal3f(0.0f, 1.0f, 0.0f);

        glTexCoord2f(0.0f, u);
        glVertex3f(ca * inner_r, 0.0f, sa * inner_r);

        glTexCoord2f(1.0f, u);
        glVertex3f(ca * outer_r, 0.0f, sa * outer_r);
    }
    glEnd();

    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

    glPopMatrix();
}

static void resolve_camera_planet_collisions(App *app, Vec3 previous_position){
    const float epsilon = 0.001f;

    for(int i = 0; i < solar_count(&app->solar); i++){
        Planet *p = solar_get(&app->solar, i);
        if(!p){
            continue;
        }

        float collider_radius = p->radius + app->camera_collision_radius;
        if(p->has_ring){
            if(p->ring_outer_radius > collider_radius){
                collider_radius = p->ring_outer_radius + app->camera_collision_radius * 0.25f;
            }
        }

        Vec3 delta = vec3_sub(app->camera.position, p->position);
        float dist = vec3_len(delta);

        if(dist < collider_radius){
            Vec3 normal;

            if(dist > 0.0001f){
                normal = vec3_scale(delta, 1.0f / dist);
            }else{
                normal = vec3_norm(vec3_sub(previous_position, p->position));
                if(vec3_len(normal) < 0.0001f){
                    normal = vec3(0.0f, 0.0f, 1.0f);
                }
            }

            app->camera.position = vec3_add(p->position,
                                            vec3_scale(normal, collider_radius + epsilon));

            if(app->visiting){
                app->visiting = false;
            }
        }
    }

    if(app->camera.position.y < app->ground_y + app->eye_height){
        app->camera.position.y = app->ground_y + app->eye_height;
    }
}

static void start_visit(App *app, int planet_index){
    Planet *p = solar_get(&app->solar, planet_index);
    if(!p){
        return;
    }

    float blocking_radius = p->radius;
    if(p->has_ring && p->ring_outer_radius > blocking_radius){
        blocking_radius = p->ring_outer_radius;
    }

    float dist = blocking_radius + app->camera_collision_radius + 2.5f;
    Vec3 offset = vec3(0.0f, 0.0f, dist);

    app->visit_target_pos = vec3_add(p->position, offset);
    app->visit_target_pos.y = app->ground_y + app->eye_height;

    app->visiting = true;
    app->vertical_velocity = 0.0f;
    app->on_ground = true;
}

static void update_camera_visit(App *app, float dt){
    if(!app->visiting){
        return;
    }

    Vec3 previous_position = app->camera.position;

    float k = 8.0f;
    float t = 1.0f - expf(-k * dt);

    app->camera.position = vec3_lerp(app->camera.position, app->visit_target_pos, t);
    resolve_camera_planet_collisions(app, previous_position);

    Vec3 d = vec3_sub(app->visit_target_pos, app->camera.position);
    if(vec3_len(d) < 0.03f){
        app->camera.position = app->visit_target_pos;
        app->visiting = false;
        app->camera.position.y = app->ground_y + app->eye_height;
        app->vertical_velocity = 0.0f;
        app->on_ground = true;
    }
}

static void handle_visit_keys(App *app){
    if(input_key_pressed(&app->input, SDL_SCANCODE_1) || input_key_pressed(&app->input, SDL_SCANCODE_KP_1)){
        start_visit(app, 0);
    }
    if(input_key_pressed(&app->input, SDL_SCANCODE_2) || input_key_pressed(&app->input, SDL_SCANCODE_KP_2)){
        start_visit(app, 1);
    }
    if(input_key_pressed(&app->input, SDL_SCANCODE_3) || input_key_pressed(&app->input, SDL_SCANCODE_KP_3)){
        start_visit(app, 2);
    }
    if(input_key_pressed(&app->input, SDL_SCANCODE_4) || input_key_pressed(&app->input, SDL_SCANCODE_KP_4)){
        start_visit(app, 3);
    }
    if(input_key_pressed(&app->input, SDL_SCANCODE_5) || input_key_pressed(&app->input, SDL_SCANCODE_KP_5)){
        start_visit(app, 4);
    }
    if(input_key_pressed(&app->input, SDL_SCANCODE_6) || input_key_pressed(&app->input, SDL_SCANCODE_KP_6)){
        start_visit(app, 5);
    }
    if(input_key_pressed(&app->input, SDL_SCANCODE_7) || input_key_pressed(&app->input, SDL_SCANCODE_KP_7)){
        start_visit(app, 6);
    }
    if(input_key_pressed(&app->input, SDL_SCANCODE_8) || input_key_pressed(&app->input, SDL_SCANCODE_KP_8)){
        start_visit(app, 7);
    }
    if(input_key_pressed(&app->input, SDL_SCANCODE_9) || input_key_pressed(&app->input, SDL_SCANCODE_KP_9)){
        start_visit(app, 8);
    }
}

static void handle_input_and_camera(App *app, float dt){
    Input *in = &app->input;
    Vec3 previous_position = app->camera.position;

    if(input_key_pressed(in, SDL_SCANCODE_ESCAPE)){
        app->running = false;
    }

    if(input_key_pressed(in, SDL_SCANCODE_F1)){
        ui_toggle_help(&app->ui);
    }

    handle_visit_keys(app);

    camera_add_mouse(&app->camera, in->mouse_dx, in->mouse_dy);

    if(app->visiting){
        update_camera_visit(app, dt);
        return;
    }

    float speed = app->camera.move_speed;
    if(input_key_down(in, SDL_SCANCODE_LSHIFT) || input_key_down(in, SDL_SCANCODE_RSHIFT)){
        speed *= 3.0f;
    }
    float step = speed * dt;

    Vec3 forward_xz = vec3(app->camera.front.x, 0.0f, app->camera.front.z);
    forward_xz = vec3_norm(forward_xz);

    Vec3 right_xz = vec3(app->camera.right.x, 0.0f, app->camera.right.z);
    right_xz = vec3_norm(right_xz);

    if(input_key_down(in, SDL_SCANCODE_W)){
        camera_move(&app->camera, forward_xz, step);
    }
    if(input_key_down(in, SDL_SCANCODE_S)){
        camera_move(&app->camera, forward_xz, -step);
    }
    if(input_key_down(in, SDL_SCANCODE_A)){
        camera_move(&app->camera, right_xz, -step);
    }
    if(input_key_down(in, SDL_SCANCODE_D)){
        camera_move(&app->camera, right_xz, step);
    }

    update_jump_physics(app, dt);
    resolve_camera_planet_collisions(app, previous_position);
}

bool app_init(App *app, const char *title, int w, int h){
    app->window = NULL;
    app->gl_ctx = NULL;
    app->running = false;
    app->win_w = w;
    app->win_h = h;
    app->last_ticks = 0;

    app->ground_y = 0.0f;
    app->eye_height = 1.7f;
    app->vertical_velocity = 0.0f;
    app->on_ground = true;
    app->camera_collision_radius = 0.45f;

    app->sphere_mesh.verts = NULL;
    app->sphere_mesh.vert_count = 0;

    app->stars_texture.id = 0;
    app->stars_texture.width = 0;
    app->stars_texture.height = 0;

    app->visiting = false;
    app->visit_target_pos = vec3(0.0f, 0.0f, 0.0f);

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0){
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    app->window = SDL_CreateWindow(title,
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   w,
                                   h,
                                   SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if(!app->window){
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    app->gl_ctx = SDL_GL_CreateContext(app->window);
    if(!app->gl_ctx){
        fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(app->window);
        SDL_Quit();
        return false;
    }

    SDL_GL_SetSwapInterval(1);
    SDL_SetRelativeMouseMode(SDL_TRUE);

    camera_init(&app->camera);
    app->camera.position.y = app->ground_y + app->eye_height;
    ui_init(&app->ui);

    for(int i = 0; i < SDL_NUM_SCANCODES; i++){
        app->input.keys[i] = false;
        app->input.keys_pressed[i] = false;
    }

    if(!renderer_init(&app->renderer, w, h)){
        fprintf(stderr, "renderer_init failed\n");
        SDL_GL_DeleteContext(app->gl_ctx);
        SDL_DestroyWindow(app->window);
        SDL_Quit();
        return false;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if(!mesh_load_obj(&app->sphere_mesh, "assets/models/sphere.obj")){
        fprintf(stderr, "Failed to load sphere.obj\n");
        SDL_GL_DeleteContext(app->gl_ctx);
        SDL_DestroyWindow(app->window);
        SDL_Quit();
        return false;
    }

    if(!texture_load(&app->stars_texture, "assets/textures/stars.jpg")){
        fprintf(stderr, "WARN: Failed to load stars background: assets/textures/stars.jpg\n");
        texture_create_solid(&app->stars_texture, 8, 8, 16, 255);
    }

    if(!solar_init(&app->solar, &app->sphere_mesh)){
        fprintf(stderr, "solar_init failed\n");
        texture_free(&app->stars_texture);
        mesh_free(&app->sphere_mesh);
        SDL_GL_DeleteContext(app->gl_ctx);
        SDL_DestroyWindow(app->window);
        SDL_Quit();
        return false;
    }

    start_visit(app, 0);

    app->running = true;
    app->last_ticks = SDL_GetTicks();
    return true;
}

void app_shutdown(App *app){
    solar_shutdown(&app->solar);
    texture_free(&app->stars_texture);
    mesh_free(&app->sphere_mesh);

    if(app->gl_ctx){
        SDL_GL_DeleteContext(app->gl_ctx);
    }
    if(app->window){
        SDL_DestroyWindow(app->window);
    }
    SDL_Quit();
}

void app_run(App *app){
    while(app->running){
        unsigned int now = SDL_GetTicks();
        unsigned int dt_ticks = now - app->last_ticks;
        app->last_ticks = now;

        float dt = seconds_from_ticks(dt_ticks);
        if(dt > 0.1f){
            dt = 0.1f;
        }

        input_begin_frame(&app->input);

        SDL_Event e;
        while(SDL_PollEvent(&e)){
            input_process_event(&app->input, &e);

            if(e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED){
                app->win_w = e.window.data1;
                app->win_h = e.window.data2;
                renderer_resize(&app->renderer, app->win_w, app->win_h);
            }
        }

        if(app->input.quit_requested){
            app->running = false;
        }

        handle_input_and_camera(app, dt);
        solar_update(&app->solar, dt);

        renderer_begin_frame(&app->renderer);
        renderer_set_3d(&app->renderer, &app->camera);

        draw_sky_sphere(app);
        renderer_draw_world_axes_and_grid();

        for(int i = 0; i < solar_count(&app->solar); i++){
            Planet *p = solar_get(&app->solar, i);

            glPushMatrix();
            glTranslatef(p->position.x, p->position.y, p->position.z);
            glScalef(p->radius, p->radius, p->radius);
            glRotatef(p->rotation_angle, 0.0f, 1.0f, 0.0f);
            draw_mesh_textured(&app->sphere_mesh, p->texture.id);
            glPopMatrix();

            if(p->has_ring){
                draw_planet_ring(p);
            }
        }

        ui_render_help_overlay(&app->ui, app->win_w, app->win_h);
        renderer_end_frame(&app->renderer);
        SDL_GL_SwapWindow(app->window);
    }
}