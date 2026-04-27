#ifndef MESH_H
#define MESH_H

#include <stdbool.h>

// Egy csúcs (vertex) adatai: pozíció, normál, textúra koordináták
typedef struct Vertex{
    float px;
    float py;
    float pz;

    float nx;
    float ny;
    float nz;

    float u;
    float v;
} Vertex;

// 3D mesh (csúcsok tömbje)
typedef struct Mesh{
    Vertex *verts;
    int vert_count;
} Mesh;

// OBJ fájlból mesh betöltése
bool mesh_load_obj(Mesh *m, const char *path);

// Memória felszabadítása
void mesh_free(Mesh *m);

#endif