#ifndef MATH3D_H
#define MATH3D_H

#include <stdbool.h>

// 3D vektor (x, y, z koordináták)
typedef struct Vec3{
    float x;
    float y;
    float z;
} Vec3;

// Vektor műveletek
Vec3 vec3(float x, float y, float z); // Vektor létrehozása
Vec3 vec3_add(Vec3 a, Vec3 b); // Összeadás
Vec3 vec3_sub(Vec3 a, Vec3 b); // Kivonás
Vec3 vec3_scale(Vec3 v, float s); // Skalárral szorzás
float vec3_dot(Vec3 a, Vec3 b); // Skalárszorzat
Vec3 vec3_cross(Vec3 a, Vec3 b); // Keresztszorzat
float vec3_len(Vec3 v); // Hossz (norma)
Vec3 vec3_norm(Vec3 v); // Normalizálás

// Segédfüggvények
float clampf(float v, float lo, float hi); // Érték korlátozása intervallumba
float radiansf(float deg); // Fok -> radián átváltás

#endif