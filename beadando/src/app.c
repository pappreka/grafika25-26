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
    return vec3(
        lerpf(a.x, b.x, t),
        lerpf(a.y, b.y, t),
        lerpf(a.z, b.z, t)
    );
}

static void set_camera_look_at(Camera *cam, Vec3 target){
    Vec3 dir = vec3_norm(vec3_sub(target, cam->position));

    if(vec3_len(dir) < 0.0001f){
        return;
    }

    cam->yaw_deg = atan2f(dir.z, dir.x) * 180.0f / (float)M_PI;
    cam->pitch_deg = asinf(clampf(dir.y, -1.0f, 1.0f)) * 180.0f / (float)M_PI;
    camera_update_vectors(cam);
}

static void setup_fixed_camera(App *app){
    camera_init(&app->camera);

    app->fixed_camera_position = vec3(0.0f, 1.2f, 18.0f);
    app->camera.position = app->fixed_camera_position;
    app->camera.yaw_deg = -90.0f;
    app->camera.pitch_deg = 0.0f;
    app->camera.mouse_sens = 0.12f;

    camera_update_vectors(&app->camera);
}

static void reset_to_overview(App *app){
    app->landing_in_progress = false;
    app->surface_mode = false;
    app->landed_planet_index = -1;
    app->planet_scene.active = false;

    app->camera.position = app->fixed_camera_position;
    app->camera.yaw_deg = -90.0f;
    app->camera.pitch_deg = 0.0f;
    camera_update_vectors(&app->camera);
}

static void draw_mesh_textured(const Mesh *m, unsigned int tex_id){
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, (GLuint)tex_id);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
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
    if(app->surface_mode){
        return;
    }

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
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, (GLuint)app->stars_texture.id);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

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
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glPopMatrix();
}

static void draw_planet_ring(const Planet *p){
    if(!p->has_ring || p->ring_texture.id == 0){
        return;
    }

    const int segments = 128;
    const float inner_r = p->ring_inner_radius;
    const float outer_r = p->ring_outer_radius;

    glPushMatrix();
    glTranslatef(p->position.x, p->position.y, p->position.z);
    glRotatef(p->ring_tilt_deg, 0.0f, 0.0f, 1.0f);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, (GLuint)p->ring_texture.id);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.05f);

    glDisable(GL_CULL_FACE);
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

    glEnable(GL_CULL_FACE);
    glDisable(GL_ALPHA_TEST);

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

    glPopMatrix();
}

static void start_landing(App *app){
    int selected = solar_selected_index(&app->solar);
    Planet *p = solar_get(&app->solar, selected);

    if(!p){
        return;
    }

    if(!p->landable){
        ui_set_status(&app->ui, "You cannot land on this planet", 3.0f);
        return;
    }

    {
        Vec3 to_camera = vec3_sub(app->camera.position, p->position);
        Vec3 normal = vec3_norm(to_camera);
        float hover = p->radius * 0.15f;

        if(vec3_len(normal) < 0.0001f){
            normal = vec3(0.0f, 0.0f, 1.0f);
        }

        if(hover < 0.25f){
            hover = 0.25f;
        }

        app->landing_surface_normal = normal;
        app->landing_start_position = app->camera.position;
        app->landing_target_position = vec3_add(
            p->position,
            vec3_scale(normal, p->radius + hover)
        );
    }

    app->landing_t = 0.0f;
    app->landing_duration = 1.35f;
    app->landing_in_progress = true;
    app->surface_mode = false;
    app->landed_planet_index = selected;
}

static void update_landing(App *app, float dt){
    Planet *p;

    if(!app->landing_in_progress){
        return;
    }

    p = solar_get(&app->solar, app->landed_planet_index);
    if(!p){
        app->landing_in_progress = false;
        return;
    }

    app->landing_t += dt / app->landing_duration;
    if(app->landing_t > 1.0f){
        app->landing_t = 1.0f;
    }

    {
        float t = app->landing_t;
        float s = t * t * (3.0f - 2.0f * t);

        app->camera.position = vec3_lerp(
            app->landing_start_position,
            app->landing_target_position,
            s
        );
    }

    set_camera_look_at(&app->camera, p->position);

    if(app->landing_t >= 1.0f){
        planet_scene_build(&app->planet_scene, p, app->landed_planet_index);

        app->camera.position = app->planet_scene.spawn_position;
        app->camera.yaw_deg = app->planet_scene.spawn_yaw_deg;
        app->camera.pitch_deg = app->planet_scene.spawn_pitch_deg;
        camera_update_vectors(&app->camera);

        app->landing_in_progress = false;
        app->surface_mode = true;
    }
}

static void handle_surface_interaction(App *app){
    const char *msg = NULL;

    if(input_key_pressed(&app->input, SDL_SCANCODE_E)){
        msg = planet_scene_interact(&app->planet_scene, app->camera.position);
        if(msg){
            ui_set_status(&app->ui, msg, 3.0f);
        }else{
            ui_set_status(&app->ui, "Nincs kozelben interaktiv objektum.", 2.0f);
        }
    }

    if(input_mouse_pressed(&app->input, SDL_BUTTON_LEFT)){
        msg = planet_scene_handle_click(&app->planet_scene, app->camera.position, app->camera.front);
        if(!msg && app->landed_planet_index == 3){
            msg = planet_scene_throw_stone(&app->planet_scene, app->camera.position, app->camera.front);
        }
        if(msg){
            ui_set_status(&app->ui, msg, 2.5f);
        }
    }

    if(planet_scene_should_exit(&app->planet_scene)){
        reset_to_overview(app);
    }
}

static void handle_input_and_camera(App *app, float dt){
    Input *in = &app->input;

    if(input_key_pressed(in, SDL_SCANCODE_ESCAPE)){
        app->running = false;
    }

    if(input_key_pressed(in, SDL_SCANCODE_F1)){
        ui_toggle_help(&app->ui);
    }

    if(input_key_pressed(in, SDL_SCANCODE_BACKSPACE)){
        reset_to_overview(app);
    }

    if(app->landing_in_progress){
        update_landing(app, dt);
        return;
    }

    if(app->surface_mode){
        camera_add_mouse(&app->camera, in->mouse_dx, in->mouse_dy);

        if(app->camera.pitch_deg > 80.0f){
            app->camera.pitch_deg = 80.0f;
        }
        if(app->camera.pitch_deg < -80.0f){
            app->camera.pitch_deg = -80.0f;
        }

        camera_update_vectors(&app->camera);
        planet_scene_update(&app->planet_scene, &app->camera, &app->input, dt);

        if(app->planet_scene.planet_index == 2 &&
            app->planet_scene.venus_heat_timer <= 0.05f){
            ui_set_status(&app->ui,
                      "FIGYELEM: szelsoseges ho es mergezo legkor!",
                      1.5f);
        }

        handle_surface_interaction(app);
        return;
    }

    if(input_key_pressed(in, SDL_SCANCODE_LEFT)){
        solar_select_next(&app->solar);
    }

    if(input_key_pressed(in, SDL_SCANCODE_RIGHT)){
        solar_select_prev(&app->solar);
    }

    if(input_key_pressed(in, SDL_SCANCODE_RETURN) ||
       input_key_pressed(in, SDL_SCANCODE_KP_ENTER)){
        start_landing(app);
    }

    camera_add_mouse(&app->camera, in->mouse_dx, in->mouse_dy);

    if(app->camera.pitch_deg > 35.0f){
        app->camera.pitch_deg = 35.0f;
    }
    if(app->camera.pitch_deg < -35.0f){
        app->camera.pitch_deg = -35.0f;
    }

    camera_update_vectors(&app->camera);
    app->camera.position = app->fixed_camera_position;
}

bool app_init(App *app, const char *title, int w, int h){
    app->window = NULL;
    app->gl_ctx = NULL;
    app->running = false;
    app->win_w = w;
    app->win_h = h;
    app->last_ticks = 0;

    app->landing_in_progress = false;
    app->surface_mode = false;
    app->surface_speed = 4.0f;
    app->landed_planet_index = -1;
    app->landing_start_position = vec3(0.0f, 0.0f, 0.0f);
    app->landing_target_position = vec3(0.0f, 0.0f, 0.0f);
    app->landing_surface_normal = vec3(0.0f, 1.0f, 0.0f);

    app->sphere_mesh.verts = NULL;
    app->sphere_mesh.vert_count = 0;

    app->stars_texture.id = 0;
    app->stars_texture.width = 0;
    app->stars_texture.height = 0;

    planet_scene_init(&app->planet_scene);

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0){
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    app->window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        w,
        h,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
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

    setup_fixed_camera(app);
    ui_init(&app->ui);

    for(int i = 0; i < SDL_NUM_SCANCODES; i++){
        app->input.keys[i] = false;
        app->input.keys_pressed[i] = false;
    }
    for(int i = 0; i < (int)(sizeof(app->input.mouse_buttons) / sizeof(app->input.mouse_buttons[0])); ++i){
        app->input.mouse_buttons[i] = false;
        app->input.mouse_buttons_pressed[i] = false;
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

        {
            SDL_Event e;
            while(SDL_PollEvent(&e)){
                input_process_event(&app->input, &e);

                if(e.type == SDL_WINDOWEVENT &&
                   e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED){
                    app->win_w = e.window.data1;
                    app->win_h = e.window.data2;
                    renderer_resize(&app->renderer, app->win_w, app->win_h);
                }
            }
        }

        if(app->input.quit_requested){
            app->running = false;
        }

        ui_update(&app->ui, dt);
        handle_input_and_camera(app, dt);
        solar_update(&app->solar, dt);

        renderer_begin_frame(&app->renderer, app -> surface_mode);
        renderer_set_3d(&app->renderer, &app->camera);

        draw_sky_sphere(app);

        if(app->surface_mode){
            planet_scene_render(&app->planet_scene);
        }else{
            for(int i = 0; i < solar_count(&app->solar); i++){
                Planet *p = solar_get(&app->solar, i);

                if(p->has_ring){
                    draw_planet_ring(p);
                }

                glPushMatrix();
                glTranslatef(p->position.x, p->position.y, p->position.z);
                glScalef(p->radius, p->radius, p->radius);
                glRotatef(p->rotation_angle, 0.0f, 1.0f, 0.0f);

                if(i == solar_selected_index(&app->solar) &&
                   !app->landing_in_progress){
                    glColor4f(1.15f, 1.15f, 1.15f, 1.0f);
                }else{
                    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                }

                draw_mesh_textured(&app->sphere_mesh, p->texture.id);
                glPopMatrix();
            }
        }

        ui_render_help_overlay(&app->ui, app->win_w, app->win_h);
        renderer_end_frame(&app->renderer);
        SDL_GL_SwapWindow(app->window);
    }
}