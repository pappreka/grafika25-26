#include "camera.h"
#include <math.h>

void camera_update_vectors(Camera *cam){
    float yaw = radiansf(cam -> yaw_deg);
    float pitch = radiansf(cam -> pitch_deg);

    Vec3 front = vec3(cosf(yaw) * cosf(pitch),
                        sinf(pitch),
                        sinf(yaw) * cosf(pitch));
    cam -> front = vec3_norm(front);

    Vec3 world_up = vec3(0.0f, 1.0f, 0.0f);
    cam -> right = vec3_norm(vec3_cross(cam -> front, world_up));
    cam -> up = vec3_norm(vec3_cross(cam -> right, cam -> front));
}

void camera_init(Camera * cam){
    cam -> position = vec3(0.0f, 1.5f, 6.0f);
    cam -> yaw_deg = -90.0f;
    cam -> pitch_deg = 0.0f;
    cam -> move_speed = 6.0f;
    cam -> mouse_sens = 0.12f;
    cam -> front = vec3(0.0f, 0.0f, -1.0f);
    cam -> right = vec3(1.0f, 0.0f, 0.0f);
    cam -> up = vec3(0.0f, 1.0f, 0.0f);

    camera_update_vectors(cam);
}

void camera_add_mouse(Camera *cam, int dx, int dy){
    cam -> yaw_deg += (float)dx * cam -> mouse_sens;
    cam -> pitch_deg -= (float)dy * cam -> mouse_sens;
    cam -> pitch_deg = clampf(cam -> pitch_deg, -89.0f, 89.0f);
    
    camera_update_vectors(cam);
}

void camera_move(Camera *cam, Vec3 direction, float amount){
    cam -> position = vec3_add(cam -> position, vec3_scale(direction, amount));
}