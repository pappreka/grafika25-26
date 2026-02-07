#ifndef CAMERA_H
#define CAMERA_H

#include "math3d.h"

typedef struct Camera{
    Vec3 position;
    float yaw_deg;
    float pitch_deg;
    float move_speed;
    float mouse_sens;
    Vec3 front;
    Vec3 right;
    Vec3 up;
} Camera;

void camera_init(Camera *cam);
void camera_update_vectors(Camera *cam);
void camera_add_mouse(Camera *cam, int dx, int dy);
void camera_move(Camera * cam, Vec3 direction, float amount);

#endif