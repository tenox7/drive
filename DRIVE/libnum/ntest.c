/* (c) Copyright Hewlett-Packard Company 2001
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */



/* libnum.a tests */
#include "libnum.h"
#define SIZE 6

#define PRINTMAT(msg,mat,size) \
    {   int i,j;    \
    \
	printf("\n--- %s ---\n",msg);   \
	for (i=1;i<=size;++i) { \
	    for (j=1;j<=size;++j) printf("%6.3f  ",mat[i][j]);  \
	    printf("\n");   \
	}   \
	printf("\n");   \
    }


main()
{
    static float mat[SIZE][SIZE] = {
	5.0,    -1.0,   1.0,    7.0,    8.0,    9.0,
	2.0,    4.0,    0.0,    11.4,   6.3,    1.1,
	1.0,    1.0,    5.0,    -3.1,   -4.2,   3.5,
	3.1,    -3.1,   4.1,    73.2,   -4.23,  9.3,
	-.3,    4.23,   7.9,    -3.0,   7.3,    -3.0,
	-3.1,   0.43,   -3.1,   4.4,    -8.8,   3.6
    };
    MATRIX mat1,mat2,mat3,mat4;
    static float b[] = {0.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0 };
    float c[7];
    static float v1[] = {0.5, 1.0, 0.5};
    static float v2[] = {1.0, -2.0, -0.5};
    float v3[3];
    IVECTOR indx;
    float parity;
    int i;

    printf("\n**** Matrix conversion test ****\n");
    mat1 = convert_matrix((float *) mat,1,SIZE,1,SIZE);
    mat2 = matrix(1,SIZE,1,SIZE);
    mat3 = matrix(1,SIZE,1,SIZE);
    mat4 = matrix(1,SIZE,1,SIZE);
    printf("\n**** COPY_MATRIX test ****\n");
    COPY_MATRIX(mat1,mat2,1,SIZE,1,SIZE,1,1);
    if (!compare_matrices(mat1,mat2,1,SIZE,1,SIZE,1,1))
	printf("compare failed.\n");
    PRINTMAT("converted matrix",mat2,SIZE);

    printf("\n**** TRANSPOSE test ****\n");
    COPY_MATRIX(mat2,mat3,1,SIZE,1,SIZE,1,1);
    transpose_matrix(mat3,1,SIZE,1,SIZE);
    PRINTMAT("transposed matrix",mat3,SIZE);

    printf("\n**** invert & multiply test ****\n");
    invert_matrix(mat2,mat3,SIZE);
    PRINTMAT("directly inverted matrix",mat3,SIZE);
    multiply_matrices(mat2,mat3,mat4,1,SIZE,1,SIZE,1,1,1,1);
    PRINTMAT("M*inv(M): ",mat4,SIZE);

    printf("\n**** LU decomposition test ****\n");
    indx = ivector(1,SIZE);
    LU_decompose(mat2,SIZE,indx,&parity);
    PRINTMAT("decomposed matrix",mat2,SIZE);

    printf("determinant: %f\n",LU_determinant(mat2,SIZE,indx,parity));

    printf("\n**** LU solving test ****\n");
    LU_solve(mat2,SIZE,indx,b);
    printf("\nsolution vector:\n");
    for (i=1;i<=SIZE;++i) printf("%f, ",b[i]);
    printf("\n");

    matrix_times_vector(mat1,b,c,1,SIZE,1,SIZE,1,1);
    printf("\nmultiplied vector (should be 10,11,12,13...):\n");
    for (i=1;i<=SIZE;++i) printf("%f, ",c[i]);
    printf("\n");

    printf("dot product of b*b: %f\n",dot_product(b,b,1,SIZE,1));
    printf("  should be %f\n",b[1]*b[1] + b[2]*b[2] +
	b[3]*b[3] + b[4]*b[4] + b[5]*b[5] + b[6]*b[6]);

    cross_product(v1,v2,v3,0,0,0);
    printf("\ncross product:\n");
    for (i=0;i<3;++i) printf("%f, ",v3[i]);
    printf("\n");

    printf("\n**** LU matrix inversion test ****\n");
    LU_invert(mat2,mat3,SIZE,indx,parity);
    PRINTMAT("inverted matrix",mat3,SIZE);
    multiply_matrices(mat1,mat3,mat4,1,SIZE,1,SIZE,1,1,1,1);
    PRINTMAT("M*inv(M): ",mat4,SIZE);

    free_convert_matrix(mat2,1,SIZE,1,SIZE);
    free_matrix(mat3,1,SIZE,1,SIZE);
    free_matrix(mat4,1,SIZE,1,SIZE);
    free_ivector(indx,1,SIZE);
}
