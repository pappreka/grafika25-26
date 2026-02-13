#include "app.h"
#include <stdio.h>

#if defined(_WIN32)
    #include <windows.h>
#endif

#include <GL/gl.h>

static float seconds_from_ticks(unsigned int ticks){
    return (float)ticks / 1000.0f;
}

static void update_jump_physics(App *app, float dt){
    const float gravity = 18.0f;
    const float jump_speed = 7.0f;
    const float floor_eye_y = app -> ground_y + app -> eye_height;

    if(input_key_pressed(&app -> input, SDL_SCANCODE_SPACE) && app -> on_ground){
        app -> vertical_velocity = jump_speed;
        app -> on_ground = false;
    }

    app -> vertical_velocity -= gravity * dt;
    app -> camera.position.y += app -> vertical_velocity * dt;

    if(app -> camera.position.y <= floor_eye_y){
        app -> camera.position.y = floor_eye_y;
        app -> vertical_velocity = 0.0f;
        app -> on_ground = true;
    }
}

static void handle_input_and_camera(App *app, float dt){
    Input *in = &app -> input;

    if(input_key_pressed(in, SDL_SCANCODE_ESCAPE)){
        app -> running = false;
    }

    if(input_key_pressed(in, SDL_SCANCODE_F1)){
        ui_toggle_help(&app -> ui);
    }

    camera_add_mouse(&app -> camera, in -> mouse_dx, in -> mouse_dy);

    float speed = app -> camera.move_speed;
    if(input_key_down(in, SDL_SCANCODE_LSHIFT) || input_key_down(in, SDL_SCANCODE_RSHIFT)){
        speed *= 3.0f;
    }
    float step = speed * dt;

    Vec3 forward_xz = vec3(app -> camera.front.x, 0.0f, app -> camera.front.z);
    forward_xz = vec3_norm(forward_xz);

    if(input_key_down(in, SDL_SCANCODE_W)){
        camera_move(&app -> camera, app -> camera.front, step);
    }
    if(input_key_down(in, SDL_SCANCODE_S)){
        camera_move(&app -> camera, app -> camera.front, -step);
    }
    if(input_key_down(in, SDL_SCANCODE_A)){
        camera_move(&app -> camera, app -> camera.right, -step);
    }
    if(input_key_down(in, SDL_SCANCODE_D)){
        camera_move(&app -> camera, app -> camera.right, step);
    }
    
    update_jump_physics(app, dt);
}

bool app_init(App *app, const char *title, int w, int h){
    app -> window = NULL;
    app -> gl_ctx = NULL;
    app -> running = false;
    app -> win_w = w;
    app -> win_h = h;
    app -> last_ticks = 0;

    app -> ground_y = 0.0f;
    app -> eye_height = 1.7f;
    app -> vertical_velocity = 0.0f;
    app -> on_ground = true;

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0){
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    app -> window = SDL_CreateWindow(title,
                                    SDL_WINDOWPOS_CENTERED,
                                    SDL_WINDOWPOS_CENTERED,
                                    w,
                                    h,
                                    SDL_WINDOW_OPENGL |
                                    SDL_WINDOW_RESIZABLE
                                    );
    if(!app -> window){
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    app -> gl_ctx = SDL_GL_CreateContext(app -> window);
    if(!app -> gl_ctx){
        fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(app -> window);
        SDL_Quit();
        return false;
    }

    SDL_GL_SetSwapInterval(1);
    SDL_SetRelativeMouseMode(SDL_TRUE);

    camera_init(&app -> camera);
    app -> camera.position.y = app -> ground_y + app -> eye_height;
    ui_init(&app -> ui);

    for(int i = 0; i < SDL_NUM_SCANCODES; i++){
        app -> input.keys[i] = false;
        app -> input.keys_pressed[i] = false;
    }

    app -> input.mouse_dx = 0;
    app -> input.mouse_dy = 0;
    app -> input.mouse_wheel = 0;
    app -> input.quit_requested = false;

    if(!renderer_init(&app -> renderer, w, h)){
        fprintf(stderr, "renderer_init failed\n");
        SDL_GL_DeleteContext(app -> gl_ctx);
        SDL_DestroyWindow(app -> window);
        SDL_Quit();
        return false;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    app -> running = true;
    app -> last_ticks = SDL_GetTicks();
    return true;
}

void app_shutdown(App *app){
    if(app -> gl_ctx){
        SDL_GL_DeleteContext(app -> gl_ctx);
    }
    if(app -> window){
        SDL_DestroyWindow(app -> window);
    }
    SDL_Quit();
}

void app_run(App *app){
    while(app -> running){
        unsigned int now = SDL_GetTicks();
        unsigned int dt_ticks = now - app -> last_ticks;
        app -> last_ticks = now;

        float dt = seconds_from_ticks(dt_ticks);
        
        if(dt > 0.1f){
            dt = 0.1f;        
        }

        input_begin_frame(&app -> input);

        SDL_Event e;
        while(SDL_PollEvent(&e)){
            input_process_event(&app -> input, &e);

            if(e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED){
                app -> win_w = e.window.data1;
                app -> win_h = e.window.data2;
                renderer_resize(&app -> renderer, app -> win_w, app -> win_h);
            }
        }

        if(app -> input.quit_requested){
            app -> running = false;
        }

        handle_input_and_camera(app, dt);
        renderer_begin_frame(&app -> renderer);
        renderer_set_3d(&app -> renderer, &app -> camera);
        renderer_draw_world_axes_and_grid();
        ui_render_help_overlay(&app -> ui, app -> win_w, app -> win_h);
        renderer_end_frame(&app -> renderer);
        SDL_GL_SwapWindow(app -> window);
    }
}