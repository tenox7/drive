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
#include <math.h>
#include "prims.ih"

/* Internal procedure to draw a spline surface from the rational points
 * of poly rotated about the y-axis.
 */
static void _spline_surface_of_revolution(
    int fildes,
    float *poly, int poly_size,
    int u_order)
{
    float *ptr1,*ptr2,*object;
    int i,j;
    float x,y,z;
    static float vknot[] = { 0.0, 0.0, 0.0, 1.0, 1.0, 2.0, 2.0,
	    3.0, 3.0, 4.0, 4.0, 4.0 };

    ptr1 = poly;
    if ((ptr2 = object = _prim_malloc_object(36*poly_size)) == NULL) {
	perror("spline_surface_of_revolution");
	return;
    }
    for (i=0;i<poly_size*4;++i) *ptr2++ = *ptr1++;

    /* -45 degrees */
    ptr1 = object;
    for (i=0;i<poly_size;++i) {
	    x = *ptr1++; y = *ptr1++; z = *ptr1++;
	    *ptr2++ = M_SQRT1_2 * (x+z);
	    *ptr2++ = M_SQRT1_2 * y;
	    *ptr2++ = M_SQRT1_2 * (z-x);
	    *ptr2++ = *ptr1++ * M_SQRT1_2;
    }

    /* -90 degrees */
    ptr1 = object;
    for (i=0;i<poly_size;++i) {
	    x = *ptr1++; y = *ptr1++; z = *ptr1++;
	    *ptr2++ =  z;
	    *ptr2++ =  y;
	    *ptr2++ = -x;
	    *ptr2++ = *ptr1++;
    }

    /* -135 degrees */
    ptr1 = object + poly_size*4;
    for (i=0;i<poly_size;++i) {
	    x = *ptr1++; y = *ptr1++; z = *ptr1++;
	    *ptr2++ =  z;
	    *ptr2++ =  y;
	    *ptr2++ = -x;
	    *ptr2++ = *ptr1++;
    }

    /* the next four octants */
    ptr1 = object;
    for (j=0;j<4;++j)
	    for (i=0;i<poly_size;++i) {
		    *ptr2++ = - *ptr1++;
		    *ptr2++ = *ptr1++;
		    *ptr2++ = - *ptr1++;
		    *ptr2++ = *ptr1++;
	    }

    /* repeat the first u-poly */
    ptr1 = object;
    for (i=0;i<poly_size*4;++i) *ptr2++ = *ptr1++;

    _prim_push_vertex_format(fildes,3,3,0,FALSE,SAME_ORDER);
    _prim_push_v_knot(fildes,vknot,sizeof(vknot)/sizeof(float));
    spline_surface(fildes,object,poly_size,9,u_order,QUADRATIC,TRUE);
    _prim_pop_v_knot(fildes);
    _prim_pop_vertex_format(fildes);

    _prim_free_object(object);
}


/*                     SPLINE_SURFACE_OF_REVOLUTION
 *
 * This procedure forms a rational spline surface by rotation the
 * specified polygon around an arbitrary axis. The control points can
 * be in rational form and u_knot_vector can be specified by the user.
 *
 * fildes    -- integer; file descriptor
 * poly      -- float*; the control polygon
 * poly_size -- integer; the number of vertices in poly
 * u_order   -- integer;the order of the curve in the u direction, which
 *                corresponds to poly and the cross section of the 
 *                surface.
 * rational  -- boolean; whether the vertices are in rational form
 *                (four floats per vertex)
 * x1,y1,z1,x2,y2,z2 -- floats; the axis of rotation
 */
void spline_surface_of_revolution(
    int fildes,
    float *poly, int poly_size,
    int u_order,
    int rational,
    float x1, float y1, float z1,
    float x2, float y2, float z2)
{
    float dx,dy,dz,x,y,z,h,length,alpha;
    float cos_theta,sin_theta,cos_phi,sin_phi;
    float mat[4][4],*fptr,tmp;
    int i;
    float *ptr1,*ptr2,*object;

    rational = (rational != 0);

    dx = x2-x1; dy = y2-y1; dz = z2-z1;
    length = FSQRT(dx*dx+dy*dy+dz*dz);
    alpha  = FSQRT(dx*dx+dz*dz);
    if (alpha == 0.0) {
	    sin_theta = 0.0;
	    cos_theta = (dy>0.0)?1.0:-1.0;
	    sin_phi = 0.0;
	    cos_phi = 1.0;
    }
    else {
	    sin_theta = alpha/length;
	    cos_theta = dy/length;
	    sin_phi   = dx/alpha;
	    cos_phi   = dz/alpha;
    }

    fptr = (float *) mat;
    *fptr++ = cos_phi;
    *fptr++ = sin_phi*sin_theta;
    *fptr++ = sin_phi*cos_theta;
    *fptr++ = 0.0;

    *fptr++ = 0.0;
    *fptr++ = cos_theta;
    *fptr++ = -sin_theta;
    *fptr++ = 0.0;

    *fptr++ = -sin_phi;
    *fptr++ = cos_phi*sin_theta;
    *fptr++ = cos_phi*cos_theta;
    *fptr++ = 0.0;

    *fptr++ = -x1*mat[0][0]                 -z1*mat[2][0];
    *fptr++ = -x1*mat[0][1] -y1*mat[1][1] -z1*mat[2][1];
    *fptr++ = -x1*mat[0][2] -y1*mat[1][2] -z1*mat[2][2];
    *fptr++ = 1.0;

/**********
    _hp_identity(mat1);
    mat1[3][0] = -x1;
    mat1[3][1] = -y1;
    mat1[3][2] = -z1;

    _hp_identity(mat2);
    mat2[0][0] =  cos_phi; mat2[0][2] = sin_phi;
    mat2[2][0] = -sin_phi; mat2[2][2] = cos_phi;

    _hp_identity(mat3);
    mat3[1][1] = cos_theta; mat3[1][2] = -sin_theta;
    mat3[2][1] = sin_theta; mat3[2][2] = cos_theta;

    concat_matrix(mat1,mat2,rmat);
    concat_matrix(rmat,mat3,mat);
*********/

    /* make the axis of revolution the y-axis */
    ptr1 = poly;
    if ((ptr2 = object = _prim_malloc_object(4*poly_size)) == NULL) {
	perror("spline_surface_of_revolution");
	return;
    }
    if (rational) for (i=0;i<poly_size;++i) {
	    x = *ptr1++;
	    y = *ptr1++;
	    z = *ptr1++;
	    h = *ptr1++;
	    *ptr2++ = x*mat[0][0] + y*mat[1][0]
		    + z*mat[2][0] + mat[3][0] * h;
	    *ptr2++ = x*mat[0][1] + y*mat[1][1] 
		    + z*mat[2][1] + mat[3][1] * h;
	    *ptr2++ = x*mat[0][2] + y*mat[1][2] 
		    + z*mat[2][2] + mat[3][2] * h;
	    *ptr2++ = h;
    }
    else for (i=0;i<poly_size;++i) {
	    x = *ptr1++;
	    y = *ptr1++;
	    z = *ptr1++;
	    *ptr2++ = x*mat[0][0] + y*mat[1][0] 
		    + z*mat[2][0] + mat[3][0];
	    *ptr2++ = x*mat[0][1] + y*mat[1][1] 
		    + z*mat[2][1] + mat[3][1];
	    *ptr2++ = x*mat[0][2] + y*mat[1][2] 
		    + z*mat[2][2] + mat[3][2];
	    *ptr2++ = 1.0;
    }


    /* push something on the stack to put the axis back */

    /* permute the rotation matrix and restore the translation */
    tmp = mat[0][1]; mat[0][1] = mat[1][0]; mat[1][0] = tmp;
    tmp = mat[0][2]; mat[0][2] = mat[2][0]; mat[2][0] = tmp;
    tmp = mat[1][2]; mat[1][2] = mat[2][1]; mat[2][1] = tmp;
    mat[3][0] = x1; mat[3][1] = y1; mat[3][2] = z1;

    concat_transformation3d(fildes,mat,PRE,PUSH);

    _spline_surface_of_revolution(fildes,object,poly_size,u_order);

    pop_matrix(fildes);
    _prim_free_object(object);
}
