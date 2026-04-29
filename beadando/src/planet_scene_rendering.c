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

// Földi felszíni világítás beállítása
void setup_earth_surface_lighting(void)
{
    const GLfloat global_ambient[] = { 0.22f, 0.22f, 0.24f, 1.0f };

    const GLfloat light0_position[] = { -18.0f, 28.0f, 12.0f, 1.0f };
    const GLfloat light0_ambient[]  = { 0.10f, 0.10f, 0.10f, 1.0f };
    const GLfloat light0_diffuse[]  = { 0.92f, 0.90f, 0.82f, 1.0f };
    const GLfloat light0_specular[] = { 0.35f, 0.35f, 0.30f, 1.0f };

    const GLfloat mat_specular[]    = { 0.18f, 0.18f, 0.18f, 1.0f };
    const GLfloat mat_shininess[]   = { 16.0f };

    // Általános világítás
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // Globális környezeti fény
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);

    // Fényparaméterek
    glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  light0_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  light0_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);

    // Anyagszínek követése
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    // Csillanás színe és fényessége
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);

    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
}

// Földi világítás lekapcsolása
void teardown_earth_surface_lighting(void)
{
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_LIGHT0);
    glDisable(GL_LIGHTING);
}

// Mars köd
void setup_mars_fog(void)
{
    const GLfloat fog_color[] = { 0.62f, 0.34f, 0.22f, 1.0f };

    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_EXP2);
    glFogf(GL_FOG_DENSITY, 0.060f); // Sűrűség
    glFogfv(GL_FOG_COLOR, fog_color); // Vöröses szín
    glHint(GL_FOG_HINT, GL_NICEST);
}

void teardown_mars_fog(void)
{
    glDisable(GL_FOG);
}

// Vénusz köd
void setup_venus_fog(void)
{
    const GLfloat fog_color[] = { 0.78f, 0.42f, 0.12f, 1.0f };

    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_EXP2);
    glFogf(GL_FOG_DENSITY, 0.025f); // Sűrűség
    glFogfv(GL_FOG_COLOR, fog_color); // Sárgás köd
    glHint(GL_FOG_HINT, GL_NICEST);
}

void teardown_venus_fog(void)
{
    glDisable(GL_FOG);
}