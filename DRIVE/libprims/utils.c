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



#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "prims.ih"

void _prim_construct_orientation_matrix(
    float mat[4][4],
    float x1, float y1, float z1,
    float x2, float y2, float z2)
{
    float height,dx,dy,dz,dx2,dz2;
    float sin_theta,cos_theta,sin_phi,cos_phi,alpha;
    float *fptr;

    dx = x2-x1; dy = y2-y1; dz = z2-z1;
    dx2 = dx*dx;
    dz2 = dz*dz;
    height = FSQRT(dx2+dy*dy+dz2);
    alpha  = FSQRT(dx2+dz2);

    if (alpha == 0.0) {
	    sin_theta = (dy > 0.0) ? 1.0 : -1.0;
	    cos_theta = 0.0;
	    sin_phi = 0.0;
	    cos_phi = 1.0;
    }
    else {
	    sin_theta = dy/height;
	    cos_theta = alpha/height;
	    sin_phi   = dx/alpha;
	    cos_phi   = dz/alpha;
    }

    fptr = (float *) mat;
    *fptr++ = cos_phi;
    *fptr++ = 0.0;
    *fptr++ = -sin_phi;
    *fptr++ = 0.0;

    *fptr++ = -sin_theta*sin_phi;
    *fptr++ = cos_theta;
    *fptr++ = -sin_theta*cos_phi;
    *fptr++ = 0.0;

    *fptr++ = cos_theta*sin_phi;
    *fptr++ = sin_theta;
    *fptr++ = cos_theta*cos_phi;
    *fptr++ = 0.0;

    *fptr++ = x1;
    *fptr++ = y1;
    *fptr++ = z1;
    *fptr++ = 1.0;
}


void _prim_push_orientation_matrix(
    int fildes,
    float mat[4][4],
    float x1, float y1, float z1,
    float x2, float y2, float z2)
{
    _prim_construct_orientation_matrix(mat,x1,y1,z1,x2,y2,z2);
    concat_transformation3d(fildes,mat,PRE,PUSH);
}


#define MAX_NUM_OBJECTS		4
#undef DEFAULT_OBJECT_SIZE
#define DEFAULT_OBJECT_SIZE	256
static float default_object[MAX_NUM_OBJECTS][DEFAULT_OBJECT_SIZE];
static int num_objects = 0;

float *_prim_malloc_object(
    int size)
{
    float *object;

    if ((size <= DEFAULT_OBJECT_SIZE)
	    && (num_objects < MAX_NUM_OBJECTS)) {
	object = &(default_object[num_objects][0]);
	++num_objects;
	return(object);
    }
    else {
	if ((object = (float *) malloc((unsigned int)size*sizeof(float)))
		== NULL) {
	    return(NULL);
	}
	else {
	    return(object);
	}
    }
}

void _prim_free_object(
    void *object)
{
    if (object == &(default_object[num_objects-1][0])) {
	--num_objects;
    }
    else if (((unsigned int) object
		< (unsigned int) &(default_object[0][0]))
	    || ((unsigned int) object
		> (unsigned int) &(default_object[MAX_NUM_OBJECTS-1][0]))) {
	free((void *) object);
    }
    else {
	fprintf(stderr,"Error: objects freed in wrong order.\n");
    }
}


void _prim_xform_points(
    float *points, int num_points,
    int coord_per_vertex,
    int normals,
    float xform[4][4])
{
    float x,y,z;
    int i;

    for (i=0; i<num_points; ++i) {
	x = *(points);
	y = *(points+1);
	z = *(points+2);

	*(points)   = x * xform[0][0] + y * xform[1][0] + z * xform[2][0]
	    + xform[3][0];
	*(points+1) = x * xform[0][1] + y * xform[1][1] + z * xform[2][1]
	    + xform[3][1];
	*(points+2) = x * xform[0][2] + y * xform[1][2] + z * xform[2][2]
	    + xform[3][2];

	if (normals) {
	    x = *(points+3);
	    y = *(points+4);
	    z = *(points+5);

	    *(points+3) = x * xform[0][0] + y * xform[1][0] + z * xform[2][0];
	    *(points+4) = x * xform[0][1] + y * xform[1][1] + z * xform[2][1];
	    *(points+5) = x * xform[0][2] + y * xform[1][2] + z * xform[2][2];
	}

	points += coord_per_vertex;
    }
}
