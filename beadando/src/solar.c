#include "solar.h"
#include <stdio.h>
#include <math.h>

static Texture2D load_or_fallback(const char *path){
    Texture2D t;
    if(!texture_load(&t, path)){
        fprintf(stderr, "WARN: texture missing, using fallback: %s\n", path);
        texture_create_solid(&t, 255, 0, 255, 255);
    }
    return t;
}

bool solar_init(SolarSystem *s, Mesh *shared_sphere){
    s -> shared_sphere = shared_sphere;
    s -> planet_count = 9;

    s -> planets[0] = (Planet){
        .name = "Sun",
        .radius = 2.2f,
        .orbit_radius = 0.0f,
        .orbit_speed = 0.0f,
        .rotation_speed = 8.0f,
        .orbit_angle = 0.0f,
        .rotation_angle = 0.0f,
        .position = {0, 1.4f, 0},
        .texture = load_or_fallback("assets/textures/sun.jpg")
    };

    s -> planets[1] = (Planet){
        .name = "Mercury",
        .radius = 0.30f,
        .orbit_radius = 3.5f,
        .orbit_speed = 1.25f,
        .rotation_speed = 35.0f,
        .position = {0, 1.2f, 0},
        .texture = load_or_fallback("assets/textures/mercury.jpg")
    };

    s -> planets[2] = (Planet){
        .name = "Venus",
        .radius = 0.55f,
        .orbit_radius = 5.0f,
        .orbit_speed = 0.95f,
        .rotation_speed = 25.0f,
        .position = {0, 1.2f, 0},
        .texture = load_or_fallback("assets/textures/venus.jpg")
    };

    s -> planets[3] = (Planet){
        .name = "Earth",
        .radius = 0.60f,
        .orbit_radius = 6.7f,
        .orbit_speed = 0.80f,
        .rotation_speed = 60.0f,
        .position = {0, 1.2f, 0},
        .texture = load_or_fallback("assets/textures/earth.jpg")
    };

    s -> planets[4] = (Planet){
        .name = "Mars",
        .radius = 0.45f,
        .orbit_radius = 8.3f,
        .orbit_speed = 0.70f,
        .rotation_speed = 55.0f,
        .position = {0, 1.2f, 0},
        .texture = load_or_fallback("assets/textures/mars.jpg")
    };

    s -> planets[5] = (Planet){
        .name = "Jupiter",
        .radius = 1.25f,
        .orbit_radius = 11.5f,
        .orbit_speed = 0.45f,
        .rotation_speed = 90.0f,
        .position = {0, 1.2f, 0},
        .texture = load_or_fallback("assets/textures/jupiter.jpg")
    };

    s -> planets[6] = (Planet){
        .name = "Saturn",
        .radius = 1.10f,
        .orbit_radius = 15.0f,
        .orbit_speed = 0.35f,
        .rotation_speed = 80.0f,
        .position = {0, 1.2f, 0},
        .texture = load_or_fallback("assets/textures/saturn.jpg")
    };

    s -> planets[7] = (Planet){
        .name = "Uranus",
        .radius = 0.85f,
        .orbit_radius = 18.2f,
        .orbit_speed = 0.28f,
        .rotation_speed = 70.0f,
        .position = {0, 1.2f, 0},
        .texture = load_or_fallback("assets/textures/uranus.jpg")
    };

    s -> planets[8] = (Planet){
        .name = "Neptune",
        .radius = 0.83f,
        .orbit_radius = 21.0f,
        .orbit_speed = 0.24f,
        .rotation_speed = 68.0f,
        .position = {0, 1.2f, 0},
        .texture = load_or_fallback("assets/textures/neptune.jpg")
    };

    for(int i = 1; i < s -> planet_count; i++){
        s -> planets[i].orbit_angle = (float)i * 0.7f;
        s -> planets[i].rotation_angle = 0.0f;
    }

    return true;
}

void solar_shutdown(SolarSystem *s){
    for(int i = 0; i < s -> planet_count; i++){
        texture_free(&s -> planets[i].texture);
    }
}

static void update_planet(Planet *p, const Planet *sun, float dt){
    p -> orbit_angle += p -> orbit_speed * dt;

    float ox = cosf(p -> orbit_angle) * p -> orbit_radius;
    float oz = sinf(p -> orbit_angle) * p -> orbit_radius;

    p -> position.x = sun -> position.x + ox;
    p -> position.z = sun -> position.z + oz;
    p -> position.y = 1.2f;
    p -> rotation_angle += p -> rotation_speed * dt;

    if(p -> rotation_angle > 360.0f){
        p -> rotation_angle -= 360.0f;
    }
}

void solar_update(SolarSystem *s, float dt){
    Planet *sun = &s -> planets[0];

    sun -> rotation_angle += sun -> rotation_speed *dt;
    if(sun -> rotation_angle > 360.0f){
        sun -> rotation_angle -= 360.0f;
    }

    for(int i = 1; i < s -> planet_count; i++){
        update_planet(&s -> planets[i], sun, dt);
    }
}

Planet* solar_get(SolarSystem *s, int index){
    if(index < 0 || index >= s -> planet_count){
        return NULL;
    }
    return & s -> planets[index];
}

int solar_count(const SolarSystem *s){
    return s -> planet_count;
}