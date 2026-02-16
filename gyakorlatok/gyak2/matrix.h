#ifndef MATRIX_H
#define MATRIX_H


void init_zero_matrix(float matrix[3][3]);

void print_matrix(const float matrix[3][3]);

void add_matrices(const float a[3][3], const float b[3][3], float c[3][3]);

void init_identity_matrix(float matrix[3][3]);

void scalar_multiply(float matrix[3][3],  float scalar);

void multiply_matrices(const float a[3][3], const float b[3][3], float c[3][3]);

void transform_matrix(float matrix[3][3], float point[3], float result[3]);

void scale_matrix(float matrix[3][3], float sx, float sy);

void shift_matrix(float matrix[3][3], float tx, float ty);

void rotate_matrix(float matrix[3][3], float angle);

void initStack(MatrixStack *s);

int isEmpty(MatrixStack *s);

int isFull(MatrixStack *s);

TransformMatrix identityMatrix();

TransformMatrix copy_matrix(TransformMatrix src);

void push_matrix(MatrixStack *s, TransformMatrix matrix);


#endif

