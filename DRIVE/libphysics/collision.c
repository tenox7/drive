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


#define FRICTIONLESS
#define TRANSLATE_I

#include <stdio.h>
#include "libnum.h"
#include "physics.h"

/* Collision-solving routine */

/* 
 * Input:
 *   obj1,obj2 - pointers to the objects (see object.h)
 *   c1,c2 -- vectors from c.o.g. of obj to collision point.
 *      in world coordinates.
 *   icoll,jcoll -- two WC unit vectors defining the plane of collision.
 *   elast -- elasticity factor (0.0-1.0)
 * Output:
 *   The objects' p,w, and v are altered.
 *
 * This routine assumes the surfaces have infinite friction.
 * Reference: SIGGRAPH '88, p. 293-294.
 *
 * Relevant equations:
 *  Compute an impulse vector R from obj2 to obj1 such that:
 *     p1' = p1 + R
 *     p2' = p2 - R
 *     I1*w1' = I1*w1 + p1XR
 *     I2*w2' = I2*w2 - p2XR
 *     R*i = 0
 *     R*j = 0
 *     (v2' + w2'Xp2 - v1' - w1'Xp1) * k = 0
 *
 * This is done by solving the set of equations:
 *  [v1' v2' w1' w2' R][M] = transpose[p1 p2 I1*w1 I2*w2 0]
 *
 */

int solve_collision(
    PHYSICAL_OBJECT *obj1, PHYSICAL_OBJECT *obj2,
    const VECTOR c1, const VECTOR c2,
    const VECTOR icoll, const VECTOR jcoll,
    const float elast)
{
    MATRIX M,Itmp,Itmp2,I1,I2;
    VECTOR c,kcoll,tvec;
    IVECTOR LUindx;
    int i,j;
    float parity;
    float Rx,Ry,Rz;

    if ((M      = matrix(1,15,1,15)) == NULL) {
	return -1;
    }
    if ((Itmp   = matrix(1,3,1,3)) == NULL) {
	free_matrix(M,1,15,1,15);
	return -1;
    }
    if ((Itmp2  = matrix(1,3,1,3)) == NULL) {
	free_matrix(M,1,15,1,15);
	free_matrix(Itmp,1,3,1,3);
	return -1;
    }
    if ((I1     = matrix(1,3,1,3)) == NULL) {
	free_matrix(M,1,15,1,15);
	free_matrix(Itmp,1,3,1,3);
	free_matrix(Itmp2,1,3,1,3);
	return -1;
    }
    if ((I2     = matrix(1,3,1,3)) == NULL) {
	free_matrix(M,1,15,1,15);
	free_matrix(I1,1,3,1,3);
	free_matrix(Itmp,1,3,1,3);
	free_matrix(Itmp2,1,3,1,3);
	return -1;
    }
    if ((kcoll  = vector(1,3)) == NULL) {
	free_matrix(M,1,15,1,15);
	free_matrix(I1,1,3,1,3);
	free_matrix(I2,1,3,1,3);
	free_matrix(Itmp,1,3,1,3);
	free_matrix(Itmp2,1,3,1,3);
	return -1;
    }
    if ((tvec   = vector(1,3)) == NULL) {
	free_matrix(M,1,15,1,15);
	free_matrix(I1,1,3,1,3);
	free_matrix(I2,1,3,1,3);
	free_matrix(Itmp,1,3,1,3);
	free_matrix(Itmp2,1,3,1,3);
	free_vector(kcoll,1,3);
	return -1;
    }
    if ((c      = vector(1,15)) == NULL) {
	free_matrix(M,1,15,1,15);
	free_matrix(I1,1,3,1,3);
	free_matrix(I2,1,3,1,3);
	free_matrix(Itmp,1,3,1,3);
	free_matrix(Itmp2,1,3,1,3);
	free_vector(tvec,1,3);
	free_vector(kcoll,1,3);
	return -1;
    }
    if ((LUindx = ivector(1,15)) == NULL) {
	free_matrix(M,1,15,1,15);
	free_matrix(I1,1,3,1,3);
	free_matrix(I2,1,3,1,3);
	free_matrix(Itmp,1,3,1,3);
	free_matrix(Itmp2,1,3,1,3);
	free_vector(c,1,15);
	free_vector(tvec,1,3);
	free_vector(kcoll,1,3);
	return -1;
    }

    for (i=1; i<=15; ++i)
	    for (j=1; j<=15; ++j)
		    M[i][j] = 0.0;

    M[1][1]  = M[2][2]  = M[3][3]  = obj1->mass;
    M[1][13] = M[2][14] = M[3][15] = -1.0;

    M[4][4]  = M[5][5]  = M[6][6]  = obj2->mass;
    M[4][13] = M[5][14] = M[6][15] = 1.0;

    matrix_times_diag(obj1->Rinv,obj1->I,Itmp,1,3,1,3,1,1,1,1);
    multiply_matrices(Itmp,obj1->R,I1,1,3,1,3,1,1,1,1);
    for (i=1; i<=3; ++i)
	    for (j=1; j<=3; ++j)
		    M[i+6][j+6] = I1[i][j];

    M[7][14] =  c1[3];
    M[7][15] = -c1[2];
    M[8][13] = -c1[3];
    M[8][15] =  c1[1];
    M[9][13] =  c1[2];
    M[9][14] = -c1[1];

    matrix_times_diag(obj2->Rinv,obj2->I,Itmp,1,3,1,3,1,1,1,1);
    multiply_matrices(Itmp,obj2->R,I2,1,3,1,3,1,1,1,1);

    for (i=1; i<=3; ++i)
	    for (j=1; j<=3; ++j)
		    M[i+9][j+9] = I2[i][j];

    M[10][14] = -c2[3];
    M[10][15] =  c2[2];
    M[11][13] =  c2[3];
    M[11][15] = -c2[1];
    M[12][13] = -c2[2];
    M[12][14] =  c2[1];

#ifdef FRICTIONLESS
    /* R dot icoll = 0
     * R dot jcoll = 0
     * (v2' + w2'Xc2 - v1' - w1'Xc1) dot k = 0
     */
    M[13][13] = icoll[1];
    M[13][14] = icoll[2];
    M[13][15] = icoll[3];

    M[14][13] = jcoll[1];
    M[14][14] = jcoll[2];
    M[14][15] = jcoll[3];

    cross_product(icoll,jcoll,kcoll,1,1,1);

    M[15][ 1] = -kcoll[1];
    M[15][ 2] = -kcoll[2];
    M[15][ 3] = -kcoll[3];
    M[15][ 4] =  kcoll[1];
    M[15][ 5] =  kcoll[2];
    M[15][ 6] =  kcoll[3];

    M[15][ 7] = c1[3]*kcoll[2] - c1[2]*kcoll[3];
    M[15][ 8] = c1[1]*kcoll[3] - c1[3]*kcoll[1];
    M[15][ 9] = c1[2]*kcoll[1] - c1[1]*kcoll[2];

    M[15][10] = c2[2]*kcoll[3] - c2[3]*kcoll[2];
    M[15][11] = c2[3]*kcoll[1] - c2[1]*kcoll[3];
    M[15][12] = c2[1]*kcoll[2] - c2[2]*kcoll[1];
#else
    /* v2' + w2'Xc2 - v1' - w1'Xc1 = 0 */
    M[13][1]  = -1.0;
    M[13][4]  =  1.0;
    M[13][8]  = -c1[3];
    M[13][9]  =  c1[2];
    M[13][11] =  c2[3];
    M[13][12] = -c2[2];

    M[14][2]  = -1.0;
    M[14][5]  =  1.0;
    M[14][7]  =  c1[3];
    M[14][9]  = -c1[1];
    M[14][10] = -c2[3];
    M[14][12] =  c2[1];

    M[15][3]  = -1.0;
    M[15][6]  =  1.0;
    M[15][7]  = -c1[2];
    M[15][8]  =  c1[1];
    M[15][10] =  c2[2];
    M[15][11] = -c2[1];
#endif /* FRICTIONLESS */

    c[ 1] = obj1->p[1];
    c[ 2] = obj1->p[2];
    c[ 3] = obj1->p[3];
    c[ 4] = obj2->p[1];
    c[ 5] = obj2->p[2];
    c[ 6] = obj2->p[3];
    c[ 7] = I1[1][1]*obj1->w[1] + I1[2][1]*obj1->w[2] +
	    I1[3][1]*obj1->w[3];
    c[ 8] = I1[1][2]*obj1->w[1] + I1[2][2]*obj1->w[2] +
	    I1[3][2]*obj1->w[3];
    c[ 9] = I1[1][3]*obj1->w[1] + I1[2][3]*obj1->w[2] +
	    I1[3][3]*obj1->w[3];
    c[10] = I2[1][1]*obj2->w[1] + I2[2][1]*obj2->w[2] +
	    I2[3][1]*obj2->w[3];
    c[11] = I2[1][2]*obj2->w[1] + I2[2][2]*obj2->w[2] +
	    I2[3][2]*obj2->w[3];
    c[12] = I2[1][3]*obj2->w[1] + I2[2][3]*obj2->w[2] +
	    I2[3][3]*obj2->w[3];
    c[13] = 0.0;
    c[14] = 0.0;
    c[15] = 0.0;

    if (LU_decompose(M,15,LUindx,&parity) == FAILURE) {
	free_matrix(M,1,15,1,15);
	free_matrix(I1,1,3,1,3);
	free_matrix(I2,1,3,1,3);
	free_matrix(Itmp,1,3,1,3);
	free_matrix(Itmp2,1,3,1,3);
	free_ivector(LUindx,1,15);
	free_vector(c,1,15);
	free_vector(tvec,1,3);
	free_vector(kcoll,1,3);
	return -1;
    }
    LU_solve(M,15,LUindx,c);

    /* R[xyz] is the impulse vector necessary to get a good solution.
     * Its units are the same as momentum, and it's applied from obj2
     * to obj1.
     */

    Rx = c[13] * (1.0 + elast);
    Ry = c[14] * (1.0 + elast);
    Rz = c[15] * (1.0 + elast);

    obj1->p[1] += Rx;  obj1->v[1] = obj1->p[1]/obj1->mass;
    obj1->p[2] += Ry;  obj1->v[2] = obj1->p[2]/obj1->mass;
    obj1->p[3] += Rz;  obj1->v[3] = obj1->p[3]/obj1->mass;

    obj2->p[1] -= Rx;  obj2->v[1] = obj2->p[1]/obj2->mass;
    obj2->p[2] -= Ry;  obj2->v[2] = obj2->p[2]/obj2->mass;
    obj2->p[3] -= Rz;  obj2->v[3] = obj2->p[3]/obj2->mass;

    /* kcoll = c1 X R */
    kcoll[1] = c1[2]*Rz - c1[3]*Ry;
    kcoll[2] = c1[3]*Rx - c1[1]*Rz;
    kcoll[3] = c1[1]*Ry - c1[2]*Rx;

#ifdef TRANSLATE_I
    {
    float cx,cy,cz;
    /* c1 to MC's */
    cx = c1[1]*obj1->R[1][1] + c1[2]*obj1->R[1][2] + c1[3]*obj1->R[1][3];
    cy = c1[1]*obj1->R[2][1] + c1[2]*obj1->R[2][2] + c1[3]*obj1->R[2][3];
    cz = c1[1]*obj1->R[3][1] + c1[2]*obj1->R[3][2] + c1[3]*obj1->R[3][3];
    /* copy I to Itmp2 */
    for (i=1; i<=3; ++i) 
	for (j=1; j<=3; ++j) 
		Itmp2[i][j] = obj1->I[i][j];
    /* Apply translation terms */
    Itmp2[1][1] += obj1->mass * (cy*cy + cz*cz);
    Itmp2[2][2] += obj1->mass * (cx*cx + cz*cz);
    Itmp2[3][3] += obj1->mass * (cx*cx + cy*cy);
    Itmp2[1][2] -= obj1->mass * cx * cy;
    Itmp2[2][1]  = Itmp2[1][2];
    Itmp2[1][3] -= obj1->mass * cx * cz;
    Itmp2[3][1]  = Itmp2[1][3];
    Itmp2[2][3] -= obj1->mass * cy * cz;
    Itmp2[3][2]  = Itmp2[2][3];

    matrix_times_diag(obj1->Rinv,Itmp2,Itmp,1,3,1,3,1,1,1,1);
    multiply_matrices(Itmp,obj1->R,I1,1,3,1,3,1,1,1,1);
    }
#endif
    if (invert_matrix(I1,I1,3) == FAILURE) {
	free_matrix(M,1,15,1,15);
	free_matrix(I1,1,3,1,3);
	free_matrix(I2,1,3,1,3);
	free_matrix(Itmp,1,3,1,3);
	free_matrix(Itmp2,1,3,1,3);
	free_ivector(LUindx,1,15);
	free_vector(c,1,15);
	free_vector(tvec,1,3);
	free_vector(kcoll,1,3);
	return -1;
    }
    obj1->w[1] += I1[1][1]*kcoll[1]+I1[1][2]*kcoll[2]+I1[1][3]*kcoll[3];
    obj1->w[2] += I1[2][1]*kcoll[1]+I1[2][2]*kcoll[2]+I1[2][3]*kcoll[3];
    obj1->w[3] += I1[3][1]*kcoll[1]+I1[3][2]*kcoll[2]+I1[3][3]*kcoll[3];

    /* kcoll = c2 X R */
    kcoll[1] = c2[2]*Rz - c2[3]*Ry;
    kcoll[2] = c2[3]*Rx - c2[1]*Rz;
    kcoll[3] = c2[1]*Ry - c2[2]*Rx;

#ifdef TRANSLATE_I
    {
    float cx,cy,cz;
    /* c2 to MC's */
    cx = c2[1]*obj2->R[1][1] + c2[2]*obj2->R[1][2] + c2[3]*obj2->R[1][3];
    cy = c2[1]*obj2->R[2][1] + c2[2]*obj2->R[2][2] + c2[3]*obj2->R[2][3];
    cz = c2[1]*obj2->R[3][1] + c2[2]*obj2->R[3][2] + c2[3]*obj2->R[3][3];
    /* copy I to Itmp2 */
    for (i=1; i<=3; ++i) 
	for (j=1; j<=3; ++j) 
		Itmp2[i][j] = obj2->I[i][j];
    /* Apply translation terms */
    Itmp2[1][1] += obj2->mass * (cy*cy + cz*cz);
    Itmp2[2][2] += obj2->mass * (cx*cx + cz*cz);
    Itmp2[3][3] += obj2->mass * (cx*cx + cy*cy);
    Itmp2[1][2] -= obj2->mass * cx * cy;
    Itmp2[2][1]  = Itmp2[1][2];
    Itmp2[1][3] -= obj2->mass * cx * cz;
    Itmp2[3][1]  = Itmp2[1][3];
    Itmp2[2][3] -= obj2->mass * cy * cz;
    Itmp2[3][2]  = Itmp2[2][3];

    matrix_times_diag(obj2->Rinv,Itmp2,Itmp,1,3,1,3,1,1,1,1);
    multiply_matrices(Itmp,obj2->R,I2,1,3,1,3,1,1,1,1);
    }
#endif
    if (invert_matrix(I2,I2,3) == FAILURE) {
	free_matrix(M,1,15,1,15);
	free_matrix(I1,1,3,1,3);
	free_matrix(I2,1,3,1,3);
	free_matrix(Itmp,1,3,1,3);
	free_matrix(Itmp2,1,3,1,3);
	free_ivector(LUindx,1,15);
	free_vector(c,1,15);
	free_vector(tvec,1,3);
	free_vector(kcoll,1,3);
	return -2;
    }
    obj2->w[1] -= I2[1][1]*kcoll[1]+I2[1][2]*kcoll[2]+I2[1][3]*kcoll[3];
    obj2->w[2] -= I2[2][1]*kcoll[1]+I2[2][2]*kcoll[2]+I2[2][3]*kcoll[3];
    obj2->w[3] -= I2[3][1]*kcoll[1]+I2[3][2]*kcoll[2]+I2[3][3]*kcoll[3];

    free_matrix(M,1,15,1,15);
    free_matrix(I1,1,3,1,3);
    free_matrix(I2,1,3,1,3);
    free_matrix(Itmp,1,3,1,3);
    free_matrix(Itmp2,1,3,1,3);
    free_ivector(LUindx,1,15);
    free_vector(c,1,15);
    free_vector(tvec,1,3);
    free_vector(kcoll,1,3);
    return 0;
}
