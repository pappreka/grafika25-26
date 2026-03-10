#include "solar.h"
#include <stdio.h>

static Texture2D load_or_fallback(const char *path){
    Texture2D t;
    if(!texture_load(&t, path)){
        fprintf(stderr, "WARN: texture missing, using fallback: %s\n", path);
        texture_create_solid(&t, 255, 0, 255, 255);
    }
    return t;
}

static Texture2D load_ring_or_fallback(const char *path){
    Texture2D t;
    if(!texture_load(&t, path)){
        fprintf(stderr, "WARN: ring texture missing, using fallback: %s\n", path);
        texture_create_solid(&t, 210, 190, 140, 180);
    }
    return t;
}

static int current_selected_planet_index(const SolarSystem *s){
    if(s->selected_order_pos < 0 || s->selected_order_pos >= s->planet_count){
        return 0;
    }
    return s->selection_order[s->selected_order_pos];
}

static void apply_selection_positions(SolarSystem *s){
    int selected_index = current_selected_planet_index(s);

    for(int i = 0; i < s->planet_count; i++){
        s->planets[i].position = s->planets[i].base_position;

        if(i == selected_index){
            s->planets[i].position.z += s->selected_forward_offset;
        }
    }
}

bool solar_init(SolarSystem *s, Mesh *shared_sphere){
    s->shared_sphere = shared_sphere;
    s->planet_count = 9;
    s->selected_forward_offset = 2.0f;

    /* Mars -> Earth -> Venus -> Mercury -> Sun -> Jupiter -> Saturn -> Uranus -> Neptune */
    s->selection_order[0] = 4;
    s->selection_order[1] = 3;
    s->selection_order[2] = 2;
    s->selection_order[3] = 1;
    s->selection_order[4] = 0;
    s->selection_order[5] = 5;
    s->selection_order[6] = 6;
    s->selection_order[7] = 7;
    s->selection_order[8] = 8;

    s->selected_order_pos = 4;

    s->planets[0] = (Planet){
        .name = "Sun",
        .radius = 2.2f,
        .orbit_radius = 0.0f,
        .orbit_speed = 0.0f,
        .rotation_speed = 8.0f,
        .orbit_angle = 0.0f,
        .rotation_angle = 0.0f,
        .base_position = {0.0f, 1.2f, 0.0f},
        .position = {0.0f, 1.2f, 0.0f},
        .texture = load_or_fallback("assets/textures/sun.jpg"),
        .has_ring = false,
        .landable = false
    };

    s->planets[1] = (Planet){
        .name = "Mercury",
        .radius = 0.30f,
        .orbit_radius = 0.0f,
        .orbit_speed = 0.0f,
        .rotation_speed = 35.0f,
        .orbit_angle = 0.0f,
        .rotation_angle = 0.0f,
        .base_position = {4.2f, 1.2f, 0.0f},
        .position = {4.2f, 1.2f, 0.0f},
        .texture = load_or_fallback("assets/textures/mercury.jpg"),
        .has_ring = false,
        .landable = true
    };

    s->planets[2] = (Planet){
        .name = "Venus",
        .radius = 0.55f,
        .orbit_radius = 0.0f,
        .orbit_speed = 0.0f,
        .rotation_speed = 25.0f,
        .orbit_angle = 0.0f,
        .rotation_angle = 0.0f,
        .base_position = {6.3f, 1.2f, 0.0f},
        .position = {6.3f, 1.2f, 0.0f},
        .texture = load_or_fallback("assets/textures/venus.jpg"),
        .has_ring = false,
        .landable = true
    };

    s->planets[3] = (Planet){
        .name = "Earth",
        .radius = 0.60f,
        .orbit_radius = 0.0f,
        .orbit_speed = 0.0f,
        .rotation_speed = 60.0f,
        .orbit_angle = 0.0f,
        .rotation_angle = 0.0f,
        .base_position = {8.8f, 1.2f, 0.0f},
        .position = {8.8f, 1.2f, 0.0f},
        .texture = load_or_fallback("assets/textures/earth.jpg"),
        .has_ring = false,
        .landable = true
    };

    s->planets[4] = (Planet){
        .name = "Mars",
        .radius = 0.45f,
        .orbit_radius = 0.0f,
        .orbit_speed = 0.0f,
        .rotation_speed = 55.0f,
        .orbit_angle = 0.0f,
        .rotation_angle = 0.0f,
        .base_position = {11.1f, 1.2f, 0.0f},
        .position = {11.1f, 1.2f, 0.0f},
        .texture = load_or_fallback("assets/textures/mars.jpg"),
        .has_ring = false,
        .landable = true
    };

    s->planets[5] = (Planet){
        .name = "Jupiter",
        .radius = 1.25f,
        .orbit_radius = 0.0f,
        .orbit_speed = 0.0f,
        .rotation_speed = 90.0f,
        .orbit_angle = 0.0f,
        .rotation_angle = 0.0f,
        .base_position = {-6.0f, 1.2f, 0.0f},
        .position = {-6.0f, 1.2f, 0.0f},
        .texture = load_or_fallback("assets/textures/jupiter.jpg"),
        .has_ring = false,
        .landable = false
    };

    s->planets[6] = (Planet){
        .name = "Saturn",
        .radius = 1.10f,
        .orbit_radius = 0.0f,
        .orbit_speed = 0.0f,
        .rotation_speed = 80.0f,
        .orbit_angle = 0.0f,
        .rotation_angle = 0.0f,
        .base_position = {-9.6f, 1.2f, 0.0f},
        .position = {-9.6f, 1.2f, 0.0f},
        .texture = load_or_fallback("assets/textures/saturn.jpg"),
        .has_ring = true,
        .ring_inner_radius = 1.45f,
        .ring_outer_radius = 2.30f,
        .ring_tilt_deg = 26.0f,
        .ring_texture = load_ring_or_fallback("assets/textures/saturn_ring.png"),
        .landable = false
    };

    s->planets[7] = (Planet){
        .name = "Uranus",
        .radius = 0.85f,
        .orbit_radius = 0.0f,
        .orbit_speed = 0.0f,
        .rotation_speed = 70.0f,
        .orbit_angle = 0.0f,
        .rotation_angle = 0.0f,
        .base_position = {-12.8f, 1.2f, 0.0f},
        .position = {-12.8f, 1.2f, 0.0f},
        .texture = load_or_fallback("assets/textures/uranus.jpg"),
        .has_ring = false,
        .landable = false
    };

    s->planets[8] = (Planet){
        .name = "Neptune",
        .radius = 0.83f,
        .orbit_radius = 0.0f,
        .orbit_speed = 0.0f,
        .rotation_speed = 68.0f,
        .orbit_angle = 0.0f,
        .rotation_angle = 0.0f,
        .base_position = {-15.7f, 1.2f, 0.0f},
        .position = {-15.7f, 1.2f, 0.0f},
        .texture = load_or_fallback("assets/textures/neptune.jpg"),
        .has_ring = false,
        .landable = false
    };

    apply_selection_positions(s);
    return true;
}

void solar_shutdown(SolarSystem *s){
    for(int i = 0; i < s->planet_count; i++){
        texture_free(&s->planets[i].texture);
        if(s->planets[i].has_ring){
            texture_free(&s->planets[i].ring_texture);
        }
    }
}

void solar_update(SolarSystem *s, float dt){
    for(int i = 0; i < s->planet_count; i++){
        Planet *p = &s->planets[i];
        p->rotation_angle += p->rotation_speed * dt;

        if(p->rotation_angle > 360.0f){
            p->rotation_angle -= 360.0f;
        }
    }

    apply_selection_positions(s);
}

Planet* solar_get(SolarSystem *s, int index){
    if(index < 0 || index >= s->planet_count){
        return NULL;
    }
    return &s->planets[index];
}

int solar_count(const SolarSystem *s){
    return s->planet_count;
}

void solar_select_next(SolarSystem *s){
    s->selected_order_pos++;
    if(s->selected_order_pos >= s->planet_count){
        s->selected_order_pos = 0;
    }
    apply_selection_positions(s);
}

void solar_select_prev(SolarSystem *s){
    s->selected_order_pos--;
    if(s->selected_order_pos < 0){
        s->selected_order_pos = s->planet_count - 1;
    }
    apply_selection_positions(s);
}

int solar_selected_index(const SolarSystem *s){
    return current_selected_planet_index(s);
}