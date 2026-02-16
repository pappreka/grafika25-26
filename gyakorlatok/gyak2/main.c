#include "matrix.h"

#include <stdio.h>

int main(int argc, char* argv[])
{
	float a[3][3] = {
        { 1.0f, -2.0f,  3.0f},
        { 5.0f, -3.0f,  0.0f},
        {-2.0f,  1.0f, -4.0f}
    };
	float b[3][3];
	float c[3][3];
    float d[3][3] = {
        { 2.0f, -7.0f,  -3.0f},
        { 4.0f, -4.0f,  0.5f},
        { 2.0f, -1.0f, -2.0f}
    };
    float e[3][3];

    float point[3] = { 2.0f, 3.0f, 1.0f};
    float result[3];

    float scalar = 1.5f;

    float sx = 2.0f;
    float sy = 0.5f;

    float tx = 3.0f;
    float ty = 4.0f;

    float angle = 45.0f;

    init_zero_matrix(b);
    b[1][1] =  8.0f;
    b[2][0] = -3.0f;
    b[2][2] =  5.0f;

    print_matrix(a);
    print_matrix(b);

    printf("\n");
    
    add_matrices(a, b, c);

    print_matrix(c);

    printf("\n");

    init_identity_matrix(e);

    print_matrix(e);

    printf("\n");

    scalar_multiply(a, scalar);
    print_matrix(a);

    printf("\n");

    multiply_matrices(a, d, c);

    print_matrix(c);

    printf("\n");

    transform_matrix(a, point, result);
    printf("", result);

    printf("\n");

    scale_matrix(d, sx, sy);
    print_matrix(d);

    printf("\n");

    shift_matrix(d, tx, ty);
    print_matrix(d);

    printf("\n");

    rotate_matrix(d, angle);
    print_matrix(d);
    

	return 0;
}

