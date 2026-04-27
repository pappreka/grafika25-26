#ifndef TEXTURE_H
#define TEXTURE_H

#include <stdbool.h>

// 2D textúra adatai
typedef struct Texture2D{
    unsigned int id; // OpenGL textúra azonosító
    int width;
    int height;
} Texture2D;

// Textúra betöltése fájlból
bool texture_load(Texture2D *t, const char *path);

// Textúra felszabadítása
void texture_free(Texture2D *t);

// Egyszínű textúra létrehozása
bool texture_create_solid(Texture2D *t,
    unsigned char r, unsigned char g,
    unsigned char b, unsigned char a);

#endif