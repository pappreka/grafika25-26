#ifndef CAMERA_H
#define CAMERA_H

#include "math3d.h"

// Kamera adatai (pozíció és nézeti irány)
typedef struct Camera{
    Vec3 position;
    float yaw_deg;     // Vízszintes forgás (bal-jobb)
    float pitch_deg;   // Függőleges forgás (fel-le)

    float move_speed;
    float mouse_sens;

    Vec3 front;
    Vec3 right;
    Vec3 up;
} Camera;

// Kamera inicializálása
void camera_init(Camera *cam);

// Irányvektorok újraszámolása (yaw/pitch alapján)
void camera_update_vectors(Camera *cam);

// Egérmozgás kezelése
void camera_add_mouse(Camera *cam, int dx, int dy);

// Kamera mozgatása adott irányba
void camera_move(Camera *cam, Vec3 direction, float amount);

#endif