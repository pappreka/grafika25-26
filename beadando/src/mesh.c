#include "mesh.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Vec2f{
    float x;
    float y;
} Vec2f;

typedef struct Vec3f{
    float x;
    float y;
    float z;
}Vec3f;

static void *xrealloc(void *p, size_t n){
    void *q = realloc(p, n);
    return q;
}

static int parse_face_token(const char *tok, int *vi, int *ti, int *ni){
    *vi = *ti = *ni = 0;
    int a = 0;
    int b = 0;
    int c = 0;

    if(sscanf(tok, "%d/%d/%d", &a, &b, &c) == 3){
        *vi = a;
        *ti = b;
        *ni = c;
        return 3;
    }

    if(sscanf(tok, "%d//%d", &a, &c) == 2){
        *vi = a;
        *ni = c;
        return 2;
    }

    if(sscanf(tok, "%d/%d", &a, &b) == 2){
        *vi = a;
        *ti = b;
        return 2;
    }

    if(sscanf(tok, "%d", &a) == 1){
        *vi = a;
        return 1;
    }

    return 0;
}

static Vec3f v3_default_normal(void){
    Vec3f n = {0.f, 1.f, 0.f};
    return n;
}

static Vec2f v2_default_uv(void) {
    Vec2f t = {0.f, 0.f};
    return t;
}

bool mesh_load_obj(Mesh *m, const char *path){
    m -> verts = NULL;
    m -> vert_count = 0;

    FILE *f = fopen(path, "rb");
    if(!f){
        fprintf(stderr, "mesh_load_obj: cannot open %s\n", path);
        return false;
    }

    Vec3f *pos = NULL;
    Vec3f *nor = NULL;
    Vec2f *uv = NULL;

    int pos_n = 0;
    int nor_n = 0;
    int uv_n = 0;
    int pos_cap = 0;
    int nor_cap = 0;
    int uv_cap = 0;

    Vertex *out = NULL;
    int out_n = 0;
    int out_cap = 0;

    char line[1024];

    while(fgets(line, (int)sizeof(line), f)){
        if(line[0] == '#'){
            continue;
        }

        if(strncmp(line, "v ", 2) == 0){
            float x;
            float y;
            float z;
            if(sscanf(line + 2, "%f %f %f", &x, &y, &z) == 3){
                if(pos_n >= pos_cap){
                    pos_cap = (pos_cap == 0) ? 256 : pos_cap * 2;
                    pos = (Vec3f*)xrealloc(pos, (size_t)pos_cap * sizeof(Vec3f));
                    if(!pos){
                        fclose(f);
                        return false;
                    }
                }
                pos[pos_n++] = (Vec3f){x, y, z};
            }
            continue;
        }

        if(strncmp(line, "vt ", 3) == 0){
            float u0;
            float v0;

            if(sscanf(line + 3, "%f %f", &u0, &v0) >= 2){
                if(uv_n >= uv_cap){
                    uv_cap = (uv_cap == 0) ? 256 : uv_cap * 2;
                    uv = (Vec2f*)xrealloc(uv, (size_t)uv_cap * sizeof(Vec2f));
                    if(!uv){
                        fclose(f);
                        return false;
                    }
                }
                uv[uv_n++] = (Vec2f){u0, v0};
            }
            continue;
        }

        if(strncmp(line, "vn ", 3) == 0){
            float x;
            float y;
            float z;

            if(sscanf(line + 3, "%f %f %f", &x, &y, &z) == 3){
                if(nor_n >= nor_cap){
                    nor_cap = (nor_cap == 0) ? 256 : nor_cap * 2;
                    nor = (Vec3f*)xrealloc(nor, (size_t)nor_cap * sizeof(Vec3f));
                    if(!nor){
                        fclose(f);
                        return false;
                    }
                }
                nor[nor_n++] = (Vec3f){x, y, z};
            }
            continue;
        }

        if(strncmp(line, "f ", 2) == 0){
            char *p = line + 2;
            char *tok[64];
            int tc= 0;

            while(*p && tc < 64){
                while(*p == ' ' || *p == '\t'){
                    p++;
                }
                if(!*p || *p == '\n' || *p == '\r'){
                    break;
                }
                tok[tc++] = p;
                while(*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r'){
                    p++;
                }
                if(*p){
                    *p = '\0';
                    p++;
                }
            }

            if(tc < 3){
                continue;
            }

            int vi[64];
            int ti[64];
            int ni[64];
            for(int i = 0; i < tc; i++){
                if(!parse_face_token(tok[i], &vi[i], &ti[i], &ni[i])){
                    vi[i] = ti[i] = ni[i] = 0;
                }
            }

            for(int i = 1; i < tc - 1 ; i++){
                int ids[3] = {0, i, i + 1};
                for(int k = 0; k < 3; k++){
                    int idx = ids[k];
                    int v_idx = vi[idx];
                    int t_idx = ti[idx];
                    int n_idx = ni[idx];

                    Vec3f P = (v_idx > 0 && v_idx <= pos_n) ? pos[v_idx - 1] : (Vec3f){0, 0, 0};
                    Vec2f T = (t_idx > 0 && t_idx <= uv_n) ? uv[t_idx - 1] : v2_default_uv();
                    Vec3f N = (n_idx > 0 && n_idx <= nor_n) ? nor[n_idx - 1] : v3_default_normal();

                    Vertex V;
                    V.px = P.x;
                    V.py = P.y;
                    V.pz = P.z;
                    V.nx = N.x;
                    V.ny = N.y;
                    V.nz = N.z;
                    V.u = T.x;
                    V.v = T.y;

                    if(out_n >= out_cap){
                        out_cap = (out_cap == 0) ? 124 : out_cap * 2;
                        out = (Vertex*)xrealloc(out, (size_t)out_cap * sizeof(Vertex));
                        if(!out){
                            fclose(f);
                            free(pos);     
                            free(uv);
                            free(nor);
                            return false;                   
                        }
                    }
                    out[out_n++] = V;
                }
            }
        }
    }

    fclose(f);
    free(pos);
    free(uv);
    free(nor);

    if(!out || out_n == 0){
        fprintf(stderr, "mesh_load_obj : no triangles read from %s\n", path);
        free(out);
        return false;
    }

    m -> verts = out;
    m -> vert_count = out_n;
    return true;
}

void mesh_free(Mesh *m){
    free(m -> verts);
    m -> verts = NULL;
    m -> vert_count = 0;
}