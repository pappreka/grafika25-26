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

float g_light_intensity = 1.0f;
float g_river_time = 0.0f;
Vec3 g_cam_right;
Vec3 g_cam_up;

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