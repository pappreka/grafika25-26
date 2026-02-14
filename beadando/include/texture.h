#ifndef TEXTURE_H
#define TEXTURE_H

#include <stdbool.h>

typedef struct Texture2D{
    unsigned int id;
    int width;
    int height;
} Texture2D;

bool texture_load(Texture2D *t, const char *path);
void texture_free(Texture2D *t);
bool texture_create_solid(Texture2D *t, unsigned char r, unsigned char g, unsigned char b, unsigned char a);

#endif