#include "renderer.h"

#if defined(_WIN32)
    #include <windows.h>
#endif

#include <GL/gl.h>
#include <GL/glu.h>

static bool g_lightning_enabled = true;

static void set_perspective(int w, int h){
    float aspect = (h > 0) ? ((float)w / (float)h) : 1.0f;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(70.0, (double)aspect, 0.1, 20000.0);
    glMatrixMode(GL_MODELVIEW);
}

bool renderer_init(Renderer * r, int width, int height){
    r -> width = width;
    r -> height = height;

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    
    {
        GLfloat ga[4] = {0.18f, 0.18f, 0.18f, 1.0f};
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ga);
    }

    {
        GLfloat spec[4] = {0.25f, 0.25f, 0.25f, 1.0f};
        GLfloat shin = 24.0f;
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shin);
    }

    renderer_resize(r, width, height);
    return true;
}

void renderer_resize(Renderer *r, int width, int height){
    r -> width = width;
    r -> height = height;
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);
    set_perspective(width, height);
}

void renderer_begin_frame(Renderer *r){
    (void)r;
    glClearColor(0.03f, 0.03f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void renderer_set_3d(Renderer *r, const Camera *cam){
    set_perspective(r -> width, r -> height);

    Vec3 eye = cam -> position;
    Vec3 center = vec3_add(cam -> position, cam -> front);
    Vec3 up = cam -> up;

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt((double)eye.x, (double)eye.y, (double)eye.z,
                (double)center.x, (double)center.y, (double)center.z,
                (double)up.x, (double)up.y, (double)up.z);
}

void renderer_draw_world_axes_and_grid(){
    glDisable(GL_LIGHTING);
    glBegin(GL_LINES);
    for(int i = -50; i <= 50; i++){
        float v = (float)i;

        glColor3f(0.12f, 0.12f, 0.14f);
        glVertex3f(-50.0f, 0.0f, v);
        glVertex3f(50.0f, 0.0f, v);
        glVertex3f(v, 0.0f, -50.0f);
        glVertex3f(v, 0.0f, 50.0f);
    }
    glEnd();

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

    if(g_lightning_enabled){
        glEnable(GL_LIGHTING);
    }
}

void renderer_end_frame(Renderer *r){
    (void)r;
}

void renderer_set_lighting_enabled(bool enabled){
    g_lightning_enabled = enabled;
    if(enabled){
        glEnable(GL_LIGHTING);
    }
    else{
        glDisable(GL_LIGHTING);
    }
}

void renderer_update_sun_light(Vec3 sun_pos, float intensity){
    float i = intensity;
    if(i < 0.0f){
        i = 0.0f;
    }

    GLfloat ambient[4] = {0.20f * i, 0.20f * i, 0.20f * i, 1.0f};
    GLfloat diffuse[4] = {0.90f * i, 0.90f * i, 0.90f * i, 1.0f};
    GLfloat specular[4] = {0.35f * i, 0.35f * i, 0.35f * i, 1.0f};
    GLfloat pos[4] = {sun_pos.x, sun_pos.y, sun_pos.z, 1.0f};

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);

    glPopMatrix();

    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.0f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.0f);
}