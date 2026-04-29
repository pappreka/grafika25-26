#include "renderer.h"
#include "math3d.h"

#if defined(_WIN32)
  #include <windows.h>
#endif
#include <GL/gl.h>

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Perspektivikus vetítés beállítása
static void set_perspective(int w, int h)
{
    float aspect = (h > 0) ? ((float)w / (float)h) : 1.0f;
    float fovy_rad = 70.0f * (float)M_PI / 180.0f;
    float f = 1.0f / tanf(fovy_rad * 0.5f);
    float z_near = 0.1f;
    float z_far = 20000.0f;

    float m[16] = {
        f / aspect, 0.0f, 0.0f, 0.0f,
        0.0f, f, 0.0f, 0.0f,
        0.0f, 0.0f, (z_far + z_near) / (z_near - z_far), -1.0f,
        0.0f, 0.0f, (2.0f * z_far * z_near) / (z_near - z_far), 0.0f
    };

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glLoadMatrixf(m);
    glMatrixMode(GL_MODELVIEW);
}

// Renderer inicializálása
bool renderer_init(Renderer *r, int width, int height) {
    r->width = width;
    r->height = height;

    // Mélységi teszt bekapcsolása
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Hátsó lapok eldobása
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Sima árnyalás
    glShadeModel(GL_SMOOTH);

    // Alapból nincs világítás
    glDisable(GL_LIGHTING);

    renderer_resize(r, width, height);
    return true;
}

// Ablakméret és viewport frissítése
void renderer_resize(Renderer *r, int width, int height) {
    r->width = width;
    r->height = height;

    glViewport(0, 0, (GLsizei)width, (GLsizei)height);
    set_perspective(width, height);
}

// Egy új frame eleje
void renderer_begin_frame(Renderer *r, bool surface_mode) {
    (void)r;

    // Más háttérszín felszíni és űr nézethez
    if (surface_mode) {
        glClearColor(0.53f, 0.81f, 0.98f, 1.0f);
    } else {
        glClearColor(0.03f, 0.03f, 0.05f, 1.0f);
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

// 3D kamera nézet beállítása
void renderer_set_3d(Renderer *r, const Camera *cam) {
    set_perspective(r->width, r->height);

    Vec3 eye = cam->position;
    Vec3 center = vec3_add(cam->position, cam->front);
    Vec3 up = cam->up;

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    {
        Vec3 f = vec3_norm(vec3_sub(center, eye));
        Vec3 s = vec3_norm(vec3_cross(f, up));
        Vec3 u = vec3_cross(s, f);

        float m[16] = {
            s.x,  u.x, -f.x, 0.0f,
            s.y,  u.y, -f.y, 0.0f,
            s.z,  u.z, -f.z, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };

        glMultMatrixf(m);
        glTranslatef(-eye.x, -eye.y, -eye.z);
    }  
}

// Világtengelyek és segédrács kirajzolása
void renderer_draw_world_axes_and_grid(void) {
    glDisable(GL_LIGHTING);

    // Rács
    glBegin(GL_LINES);
    for (int i = -80; i <= 80; i++) {
        float v = (float)i;

        glColor3f(0.12f, 0.12f, 0.14f);
        glVertex3f(-80.0f, 0.0f, v);
        glVertex3f( 80.0f, 0.0f, v);

        glVertex3f(v, 0.0f, -80.0f);
        glVertex3f(v, 0.0f,  80.0f);
    }
    glEnd();

    // X, Y, Z tengelyek
    glBegin(GL_LINES);

    glColor3f(0.9f, 0.2f, 0.2f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(3.0f, 0.0f, 0.0f);

    glColor3f(0.2f, 0.9f, 0.2f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 3.0f, 0.0f);

    glColor3f(0.2f, 0.2f, 0.9f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 3.0f);

    glEnd();
}

// Frame vége
void renderer_end_frame(Renderer *r) {
    (void)r;
}