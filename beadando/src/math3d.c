#define _USE_MATH_DEFINES
#include "math3d.h"
#include <math.h>

Vec3 vec3(float x, float y, float z){
    Vec3 v = {x, y, z};
    return v;
}

Vec3 vec3_add(Vec3 a, Vec3 b){
    return vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

Vec3 vec3_sub(Vec3 a, Vec3 b){
    return vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

Vec3 vec3_scale(Vec3 v, float s){
    return vec3(v.x * s, v.y * s, v.z * s);
}

float vec3_dot(Vec3 a, Vec3 b){
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 vec3_cross(Vec3 a, Vec3 b){
    return vec3(a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x);
}

float vec3_len(Vec3 v){
    return sqrtf(vec3_dot(v, v));
}

Vec3 vec3_norm(Vec3 v){
    float l = vec3_len(v);
    if(l <= 0.000001f){
        return vec3(0.0f, 0.0f, 0.0f);
    }
    return vec3_scale(v, 1.f / l);
}

float clampf(float v, float lo, float hi){
    if(v < lo){
        return lo;
    }
    if(v > hi){
        return hi;
    }
    return v;
}

float radiansf(float deg){
    return deg * (float)M_PI / 180.0f;
}