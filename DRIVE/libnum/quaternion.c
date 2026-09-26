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


/* quaternion routines */
/* Explanations of quaternions can be found in the Physically-Based
 * Modelling course notes from SIGGRAPH '88, Section E by R. Barzel and
 * A. Barr, p. E-55.
 */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "libnum.h"

#define sqrtf sqrt

QUATERNION *allocate_quaternion(
    void)
{
    QUATERNION *q;

    if ((q = (QUATERNION *) malloc(sizeof(QUATERNION))) == NULL) {
	nrerror("malloc error in allocate_quaternion()");
	return(NULL);
    }

    if ((q->r = vector(1,3)) == NULL) {
	free((void *) q);
	return(NULL);
    }
    return(q);
}

void free_quaternion(
    QUATERNION *q)
{
    free_vector(q->r,1,3);
    free((void *) q);
}


void multiply_quaternions(
    QUATERNION *q1,
    QUATERNION *q2,
    QUATERNION *q3)
{
    static float cross[4];

    q3->s = q1->s*q2->s - dot_product(q1->r,q2->r,1,3,1);

    cross_product(q1->r,q2->r,cross,1,1,1);
    q3->r[1] = q1->s*q2->r[1] + q2->s*q1->r[1] + cross[1];
    q3->r[2] = q1->s*q2->r[2] + q2->s*q1->r[2] + cross[2];
    q3->r[3] = q1->s*q2->r[3] + q2->s*q1->r[3] + cross[3];
}


/* Multiply vector v[1..3] by q and give resulting quaternion */
void vector_times_quaternion(
    VECTOR v,
    QUATERNION *q,
    QUATERNION *result)
{
    static float cross[4];

    result->s = -dot_product(v,q->r,1,3,1);

    cross_product(v,q->r,cross,1,1,1);
    result->r[1] = q->s*v[1] + cross[1];
    result->r[2] = q->s*v[2] + cross[2];
    result->r[3] = q->s*v[3] + cross[3];
}


/* Give the equivalent matrix for the quaternion.
 * M[1..3][1..3] is modified.
 */
void quaternion_to_matrix(
    QUATERNION *q,
    MATRIX M)
{
    register float w,x,y,z;
    register float wx,wy,wz;
    register float xy,xz,yz;
    float len;

    len  = q->s*q->s;
    len += q->r[1]*q->r[1];
    len += q->r[2]*q->r[2];
    len += q->r[3]*q->r[3];
    if ((len != 0.0) && (len != 1.0)) {
	    len = (float) sqrt((double) len);
	    
	    w = q->s/len;
	    x = q->r[1]/len;
	    y = q->r[2]/len;
	    z = q->r[3]/len;
    }
    else {
	    w = q->s;
	    x = q->r[1];
	    y = q->r[2];
	    z = q->r[3];
    }

    wx = wy = wz = w;
    xy = xz = x;
    yz = y;
    wx *= x;
    wy *= y;
    wz *= z;
    xy *= y;
    xz *= z;
    yz *= z;
    x  *= x;
    y  *= y;
    z  *= z;

    M[1][1] = 1.0 - 2.0*(y+z);
    M[1][2] = 2.0 * (xy+wz);
    M[1][3] = 2.0 * (xz-wy);
    M[2][1] = 2.0 * (xy-wz);
    M[2][2] = 1.0 - 2.0*(x+z);
    M[2][3] = 2.0 * (yz+wx);
    M[3][1] = 2.0 * (xz+wy);
    M[3][2] = 2.0 * (yz-wx);
    M[3][3] = 1.0 - 2.0*(x+y);
}


#define EPSILON (1e-20)
void normalize_quaternion(
    QUATERNION *q)
{
    float len;

    len  = q->s*q->s;
    len += q->r[1]*q->r[1];
    len += q->r[2]*q->r[2];
    len += q->r[3]*q->r[3];

    if ((len < EPSILON)
	    && (len > -EPSILON)) {
	q->s    = 1.0;
	q->r[1] = 0.0;
	q->r[2] = 0.0;
	q->r[3] = 0.0;
    }
    else if (len == 1.0) {
	return;
    }
    else {
	len = (float) sqrt((double) len);
	q->s    /= len;
	q->r[1] /= len;
	q->r[2] /= len;
	q->r[3] /= len;
    }
}


/* This routine takes a C-style 4x4 matrix (float[4][4]) [as opposed
 * to the matrix styles used internally by this module] and converts
 * it to a quaternion.  See Shoemake, ACM, V19, #3, 1985.
 */
void cmatrix4x4_to_quaternion(
    float mat[4][4],
    QUATERNION *q)
{
    float wsq,xsq,ysq;
    float w,x,y,z;	

    wsq = (1.0/4.0) * (1.0 + mat[0][0] + mat[1][1] + mat[2][2]);
    if (wsq > EPSILON) {
	w = sqrtf(wsq);
	x = (mat[1][2] - mat[2][1]) / (4.0*w);
	y = (mat[2][0] - mat[0][2]) / (4.0*w);
	z = (mat[0][1] - mat[1][0]) / (4.0*w);
    }
    else {
	w = 0.0;
	xsq = (-1.0/2.0) * (mat[1][1] + mat[2][2]);
	if (xsq > EPSILON) {
	    x = sqrtf(xsq);
	    y = mat[0][1] / (2.0*x);
	    z = mat[0][2] / (2.0*x);
	}
	else {
	    x = 0.0;
	    ysq = (1.0/2.0) * (1.0-mat[2][2]);
	    if (ysq > EPSILON) {
		y = sqrtf(ysq);
		z = mat[1][2] / (2.0*y);
	    }
	    else {
		y = 0.0;
		z = 1.0;
	    }
	}
    }

    q->s = w;
    q->r[1] = x;
    q->r[2] = y;
    q->r[3] = z;
}


void quaternion_to_cmatrix4x4(
    QUATERNION *q,
    float mat[4][4])
{
    float w = q->s;
    float x = q->r[1];
    float y = q->r[2];
    float z = q->r[3];

    mat[0][0] = 1 - 2*y*y - 2*z*z;
    mat[0][1] =     2*x*y + 2*w*z;
    mat[0][2] =     2*x*z - 2*w*y;
    mat[0][3] = 0.0;

    mat[1][0] =     2*x*y - 2*w*z;
    mat[1][1] = 1 - 2*x*x - 2*z*z;
    mat[1][2] =     2*y*z + 2*w*x;
    mat[1][3] = 0.0;

    mat[2][0] =     2*x*z + 2*w*y;
    mat[2][1] =     2*y*z - 2*w*x;
    mat[2][2] = 1 - 2*x*x - 2*y*y;
    mat[2][3] = 0.0;

    mat[3][0] = 0.0;
    mat[3][1] = 0.0;
    mat[3][2] = 0.0;
    mat[3][3] = 1.0;
}
