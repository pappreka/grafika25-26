#include "matrix.h"

#include <stdio.h>
#include <math.h>

#define PI 3.141592653589793
#define MAX_STACK_SIZE 100

void init_zero_matrix(float matrix[3][3])
{
    int i;
    int j;

    for (i = 0; i < 3; ++i) {
        for (j = 0; j < 3; ++j) {
            matrix[i][j] = 0.0;
        }
    }
}

void print_matrix(const float matrix[3][3])
{
    int i;
    int j;

    for (i = 0; i < 3; ++i) {
        for (j = 0; j < 3; ++j) {
            printf("%4.4f ", matrix[i][j]);
        }
        printf("\n");
    }
}

void add_matrices(const float a[3][3], const float b[3][3], float c[3][3])
{
    int i;
    int j;

    for (i = 0; i < 3; ++i) {
        for (j = 0; j < 3; ++j) {
            c[i][j] = a[i][j] + b[i][j];
        }
    }
}

void init_identity_matrix(float matrix[3][3]) {
    int i;
    int j;
        for(i = 0; i < 3; i++) {
            for (j = 0; j < 3; j++) {
                if(i == j) {
                    matrix[i][j] = 1.0;
                } else {
                    matrix[i][j] = 0.0;
                }
            }
        }
}

void scalar_multiply(float matrix[3][3],  float scalar) {
    int i;
    int j;
        for (i = 0; i < 3; i++) {
            for (j = 0; j < 3; j++) {
                matrix[i][j] *= scalar;
            }
        }
}

void multiply_matrices(const float a[3][3], const float b[3][3], float c[3][3]) {
    int i;
    int j;

    for (i = 0; i < 3; ++i) {
        for (j = 0; j < 3; ++j) {
            c[i][j] = 0.0;
            for (int k = 0; k < 3; ++k) {
                c[i][j] += a[i][k] * b[k][j];
            }
        }
    }
}

void transform_matrix(float matrix[3][3], float point[3], float result[3]) {
    int i;
    int j;

    for (i = 0; i < 3; ++i) {
        result[i] = 0.0;
        for (j = 0; j < 3; ++j) {
            result[i] += matrix[i][j] * point[j];
        }
    }
}

void scale_matrix(float matrix[3][3], float sx, float sy) {
    matrix[0][0] *= sx;
    matrix[1][1] *= sy;
}

void shift_matrix(float matrix[3][3], float tx, float ty) {
    matrix[0][2] += tx;
    matrix[1][2] += ty;
}

void rotate_matrix(float matrix[3][3], float angle) {
    float rad = angle * (PI / 180);
    float cosA = cos(rad);
    float sinA = sin(rad);

    float rot[3][3] = {
        {cosA, -sinA, 0.0},
        {sinA, cosA, 0.0},
        {0.0, 0.0, 1.0}
    };

    float temp[3][3] = {0};

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            for (int k = 0; k < 3; k++) {
                temp[i][j] += rot[i][k] * matrix[k][j];
            }
        }
    }

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            matrix[i][j] = temp[i][j];
        }
    }
}

typedef struct {
    float matrix[3][3];
} TransformMatrix;

typedef struct {
    TransformMatrix stack[3];
    int top;
} MatrixStack;

void initStack(MatrixStack *s) {
    s->top = -1;
}

int isEmpty(MatrixStack *s) {
    return s->top == -1;
}

int isFull(MatrixStack *s) {
    return s->top >= MAX_STACK_SIZE - 1;
}

TransformMatrix identityMatrix() {
    TransformMatrix matrix = {{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
    return matrix;
}

TransformMatrix copy_matrix(TransformMatrix src) {
    TransformMatrix dest;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            dest.matrix[i][j] = src.matrix[i][j];
        }
    }
    return dest;
}

void push_matrix(MatrixStack *s, TransformMatrix matrix) {
    if (isFull(s)) {
        printf("Error: Stack is full\n");
        return;
    }
    s->stack[(s->top)++] = copy_matrix(matrix);
}

TransformMatrix pop_matrix(MatrixStack *s) {
    if (isEmpty(s)) {
        printf("Error: Stack is empty\n");
        return;
    }
    return s->stack[(s->top)--];
}