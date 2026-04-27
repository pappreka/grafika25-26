#include "texture.h"
#include <stdio.h>

#if defined(_WIN32)
    #include <windows.h>
#endif

#include <GL/gl.h>
#include <GL/glu.h>

#include "stb_image.h"

// Egyszínű (fallback) textúra létrehozása
bool texture_create_solid(Texture2D *t, unsigned char r, unsigned char g, unsigned char b, unsigned char a){
    t->id = 0;
    t->width = 1;
    t->height = 1;

    // 1 pixel RGBA
    unsigned char px[4] = {r, g, b, a};

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    // Szűrés
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Ismétlődés (tiling)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Textúra feltöltése (1x1 pixel)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);

    glBindTexture(GL_TEXTURE_2D, 0);

    t->id = (unsigned int)tex;

    return true;
}

// Textúra betöltése fájlból
bool texture_load(Texture2D *t, const char *path){
    t->id = 0;
    t->width = 0;
    t->height = 0;

    int w = 0;
    int h = 0;
    int comp = 0;

    // Függőleges tükrözés (OpenGL UV miatt kell)
    stbi_set_flip_vertically_on_load(1);

    // Kép betöltése (mindig RGBA-ra konvertálva)
    unsigned char *pixels = stbi_load(path, &w, &h, &comp, 4);
    if(!pixels){
        fprintf(stderr, "texture_load: failed to load %s (%s)\n", path, stbi_failure_reason());
        return false;
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    // Mipmap + lineáris szűrés
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Ismétlés
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Mipmap generálás (régi GLU módszer)
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    glBindTexture(GL_TEXTURE_2D, 0);

    // CPU memória felszabadítása
    stbi_image_free(pixels);

    t->id = (unsigned int)tex;
    t->width = w;
    t->height = h;

    return true;
}

// Textúra felszabadítása GPU-ból
void texture_free(Texture2D *t){
    if(t->id != 0){
        GLuint tex = (GLuint)t->id;
        glDeleteTextures(1, &tex);
        t->id = 0;
    }

    t->width = 0;
    t->height = 0;
}