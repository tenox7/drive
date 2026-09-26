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


#include <math.h>
#include <stdio.h>
#include "prims.ih"

/* Internal procedure to draw a mesh surface from the 3D points
 * of poly rotated about the y-axis.
 */
static void _mesh_surface_of_revolution(
    int fildes,
    float *poly, int poly_size,
    int facets,
    int cpv,
    float height,
    float times_around,
    float mat[4][4])
{
    float *object,*fptr,*origptr;
    float x,y,z,sa,ca,sda,cda,tmp;
    float offset,d_offset;
    int i,j,count;
    float d_ang;

    count = facets * times_around + 1;
    if ((fptr = object = _prim_malloc_object(cpv*poly_size*count)) == NULL) {
	perror("mesh_surface_of_revolution");
	return;
    }

    d_ang = (2.0*M_PI)/facets;
    cda = FCOS(d_ang); sda = FSIN(d_ang);
    ca = 1.0; sa = 0.0;
    offset = 0.0;
    d_offset = height/facets/times_around;
    /* now revolve around the y axis */
    for (i=0; i<count; ++i) {
	origptr = poly;
	for (j=0; j<poly_size; ++j) {
	    x  = *origptr++;
	    y  = *origptr++;
	    z  = *origptr++;
	    *fptr++ = ca*x - sa*z;
	    *fptr++ = y + offset;
	    *fptr++ = sa*x + ca*z;
	    if (cpv == 6) {
		x  = *origptr++;
		y  = *origptr++;
		z  = *origptr++;
		*fptr++ = ca*x - sa*z;
		*fptr++ = y;
		*fptr++ = sa*x + ca*z;
	    }
	}
	tmp = ca*cda - sa*sda;
	sa  = ca*sda + sa*cda;
	ca  = tmp;
	offset += d_offset;
    }

    /* Get it oriented right */
    _prim_xform_points(object,count*poly_size,cpv,(cpv==6),mat);

    /* draw it! */
    _prim_push_vertex_format(fildes,cpv-3,cpv-3,0,FALSE,TOGGLE_ORDER);
    quadrilateral_mesh(fildes,object,count,poly_size,NULL);
    _prim_pop_vertex_format(fildes);

    _prim_free_object(object);
}


static void _orient_msor(
    int fildes,
    float *poly, int poly_size,
    int facets,
    int normals,
    float x1, float y1, float z1,
    float x2, float y2, float z2,
    float offset,
    float times_around,
    int orient)
{
    float dx,dy,dz,x,y,z,length,alpha;
    float cos_theta,sin_theta,cos_phi,sin_phi;
    float mat[4][4],*fptr,tmp;
    int i,cpv;
    float *ptr1,*ptr2,*object;
    float xval, yval, zval, mag;

    if (normals) cpv = 6;
    else cpv = 3;

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

    /* make the axis of revolution the y-axis */
    if (orient) {
	ptr1 = poly;
	if ((ptr2 = object = _prim_malloc_object(cpv*poly_size)) == NULL) {
	    perror("mesh_surface_of_revolution");
	    return;
	}

	for (i=0; i<poly_size; ++i) {
	    x = *ptr1++;
	    y = *ptr1++;
	    z = *ptr1++;
	    *ptr2++ = x*mat[0][0] + y*mat[1][0] 
		    + z*mat[2][0] + mat[3][0];
	    *ptr2++ = x*mat[0][1] + y*mat[1][1] 
		    + z*mat[2][1] + mat[3][1];
	    *ptr2++ = x*mat[0][2] + y*mat[1][2] 
		    + z*mat[2][2] + mat[3][2];
	    if (normals) {
		x = *ptr1++;
		y = *ptr1++;
		z = *ptr1++;

		xval = x*mat[0][0] + y*mat[1][0] + z*mat[2][0];
		yval = x*mat[0][1] + y*mat[1][1] + z*mat[2][1];
		zval = x*mat[0][2] + y*mat[1][2] + z*mat[2][2];

		mag = xval*xval + yval*yval + zval*zval;
		mag = FSQRT(mag);

		*ptr2++ = xval / mag;
		*ptr2++ = yval / mag;
		*ptr2++ = zval / mag;
	    }
	}
    }
    else {
	object = poly;
    }

    /* push something on the stack to put the axis back */

    /* permute the rotation matrix and restore the translation */
    tmp = mat[0][1]; mat[0][1] = mat[1][0]; mat[1][0] = tmp;
    tmp = mat[0][2]; mat[0][2] = mat[2][0]; mat[2][0] = tmp;
    tmp = mat[1][2]; mat[1][2] = mat[2][1]; mat[2][1] = tmp;
    mat[3][0] = x1; mat[3][1] = y1; mat[3][2] = z1;

    _mesh_surface_of_revolution(fildes,object,poly_size,facets,
	cpv,offset,times_around,mat);

    if (orient) _prim_free_object(object);
}


/*                     MESH_SURFACE_OF_REVOLUTION
 *
 * This procedure forms a rational mesh surface by rotation the
 * specified polygon around an arbitrary axis. The control points can
 * be in rational form and u_knot_vector can be specified by the user.
 *
 * fildes    -- integer; file descriptor
 * poly      -- float*; the control polygon
 * poly_size -- integer; the number of vertices in poly
 * facets    -- number of facets in the revolution
 * normals   -- are there normals in the data?
 * x1,y1,z1,x2,y2,z2 -- floats; the axis of rotation
 */
void mesh_surface_of_revolution(
    int fildes,
    float *poly, int poly_size,
    int facets,
    int normals,
    float x1, float y1, float z1,
    float x2, float y2, float z2)
{
    _orient_msor(fildes,poly,poly_size,facets,normals,
	    x1,y1,z1,x2,y2,z2,0.0,1.0,TRUE);
}


void mesh_torus(
    int fildes,
    int facets,
    int rev_steps,
    float minor_radius, float major_radius,
    float x1, float y1, float z1,
    float x2, float y2, float z2)
{
    float ang;
    float sa,ca;
    float *poly,*pptr;
    float x,y,xt;
    int i;

    if ((poly = _prim_malloc_object((facets+1)*6)) == NULL) {
	perror("mesh_torus");
	return;
    }

    ang = 2.0*M_PI/(float) facets;
    sa = FSIN(ang);
    ca = FCOS(ang);

    pptr = poly;
    x = minor_radius; y=0;
    for (i=0; i<(facets+1); ++i) {
	*pptr++ = x + major_radius;
	*pptr++ = y;
	*pptr++ = 0.0;
	*pptr++ = x;
	*pptr++ = y;
	*pptr++ = 0.0;
	xt = x*ca - y*sa;
	y  = x*sa + y*ca;
	x = xt;
    }

    _orient_msor(fildes,poly,facets+1,rev_steps,TRUE,
	    x1,y1,z1,x2,y2,z2,0.0,1.0,FALSE);
    _prim_free_object(poly);
}


void mesh_helix(
    int fildes,
    int facets,
    int rev_steps,
    float minor_radius, float major_radius,
    float x1, float y1, float z1,
    float x2, float y2, float z2,
    float times_around)
{
    float ang;
    float sa,ca;
    float *poly,*pptr;
    float x,y,xt,offset,dx,dy,dz;
    int i;

    if ((poly = _prim_malloc_object((facets+1)*6)) == NULL) {
	perror("mesh_helix");
	return;
    }

    ang = 2.0*M_PI/(float) facets;
    sa = FSIN(ang);
    ca = FCOS(ang);

    pptr = poly;
    x = minor_radius; y=0;
    for (i=0; i<(facets+1); ++i) {
	*pptr++ = x + major_radius;
	*pptr++ = y;
	*pptr++ = 0.0;
	*pptr++ = x;
	*pptr++ = y;
	*pptr++ = 0.0;
	xt = x*ca - y*sa;
	y  = x*sa + y*ca;
	x = xt;
    }

    dx = x2-x1; dy = y2-y1; dz = z2-z1;
    offset = FSQRT(dx*dx+dy*dy+dz*dz)/times_around;

    _orient_msor(fildes,poly,facets+1,rev_steps,TRUE,
	    x1,y1,z1,x2,y2,z2,offset,times_around,FALSE);
    _prim_free_object(poly);
}


void mesh_sphere(
    int fildes,
    float radius,
    int latitudes, int longitudes,
    float x, float y, float z)
{
    float *outline,*fptr;
    float angle,d_angle;
    float ca,sa;
    int i;

    if ((outline = _prim_malloc_object((latitudes+1)*6)) == NULL) {
	perror("mesh_sphere");
	return;
    }

    d_angle = M_PI/latitudes;

    fptr = outline;
    for (i=0,angle = -M_PI/2.0; i<latitudes+1; ++i,angle+=d_angle) {
	ca = FCOS(angle);
	sa = FSIN(angle);
	*fptr++ = x + ca*radius;
	*fptr++ = y + sa*radius;
	*fptr++ = z;
	*fptr++ = ca;
	*fptr++ = sa;
	*fptr++ = 0.0;
    }

    mesh_surface_of_revolution(fildes,outline,latitudes+1,longitudes,TRUE,
	x,y,z,x,y+1.0,z);

    _prim_free_object(outline);
}
