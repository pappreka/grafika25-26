#ifndef MESH_H
#define MESH_H

#include <stdbool.h>

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

typedef struct Mesh{
    Vertex *verts;
    int vert_count;
} Mesh;

bool mesh_load_obj(Mesh *m, const char *path);
void mesh_free(Mesh *m);

#endif