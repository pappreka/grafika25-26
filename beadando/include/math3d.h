#ifndef MATH3D_H
#define MATH3D_H

#include <stdbool.h>

typedef struct Vec3{
    float x;
    float y;
    float z;
}Vec3;

Vec3 vec3(float x, float y, float z);
Vec3 vec3_add(Vec3 a, Vec3 b);
Vec3 vec3_sub(Vec3 a, Vec3 b);
Vec3 vec3_scale(Vec3 v, float s);
float vec3_dot(Vec3 a, Vec3 b);
Vec3 vec3_cross(Vec3 a, Vec3 b);
float vec3_len(Vec3 v);
Vec3 vec3_norm(Vec3 v);

float clampf(float v, float lo, float hi);
float radiansf(float deg);

#endif