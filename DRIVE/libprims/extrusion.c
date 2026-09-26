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


/* EXTRUSION MODULE */
#include <stdio.h>
#include <math.h>
#include "prims.ih"


/*                     EXTRUSION
 *
 * This procedure forms an extruded object by sweeping the polygon
 * given in clist along a path.  At each step of the path, the normal 
 * of the offset polygon remains equal to the normal of the original
 * polygon, thus, the back face will be "parallel" to the front face,
 * regardless of the path. Each triple in "path" is a vector relative
 * to the origin.
 *
 * fildes     -- integer; file descriptor
 * clist      -- float*; a simple list of the polygon's vertices
 * poly_size  -- integer; the number of vertices in clist
 * path       -- float*; a list of vectors, relative to the origin,
 *                to offset clist by to get the next set of control
 *                points in the extrusion
 * path_size  -- integer; the number of vectors in path
 * front_face -- boolean; whether to draw the front face
 * back_face  -- boolean; whether to draw the back face
 *
 */

void extrusion(
    int fildes,
    float *clist, int poly_size,
    float *path,  int path_size,
    int front_face, int back_face)
{
	int i,j;
	float *poly,*oldpoly,*ptr1,*ptr2,*ptr3,*path_ptr=path;
	float side[12];

	/* copy the polygon to temporary storage */
	if ((poly = ptr1 = _prim_malloc_object((poly_size+1)*3)) == NULL) {
	    perror("extrusion");
	    return;
	}
	if ((oldpoly     = _prim_malloc_object((poly_size+1)*3)) == NULL) {
	    perror("extrusion");
	    _prim_free_object(poly);
	    return;
	}

	ptr2 = clist;
	for (i=0;i<3*poly_size;++i) *ptr1++ = *ptr2++;
	/* add the first point back to the end */
	ptr2 = clist;
	*ptr1++ = *ptr2++;
	*ptr1++ = *ptr2++;
	*ptr1++ = *ptr2++;

	/* do the front */
	if (front_face) polygon3d(fildes,poly,poly_size,FALSE);

	/* For each segment of the extrusion, draw the sides of the object*/
	for (j=0;j<path_size;++j) {
		/* Copy the old poly and form the back face by moving the
		 * original poly along the path.
		 */
		ptr1 = oldpoly;
		ptr2 = poly;
		for (i=0;i<poly_size+1;++i) {
			*ptr1++  = *ptr2;
			*ptr1++  = *(ptr2+1);
			*ptr1++  = *(ptr2+2);
			*ptr2++ += *path_ptr;
			*ptr2++ += *(path_ptr+1);
			*ptr2++ += *(path_ptr+2);
		}
		
		/* Do the sides */
		ptr1 = oldpoly;
		ptr2 = poly;
		for (i=0;i<poly_size;++i) {
			ptr3 = side;
			*ptr3++ = *ptr1++; *ptr3++ = *ptr1++; *ptr3++ = *ptr1++;
			*ptr3++ = *ptr2++; *ptr3++ = *ptr2++; *ptr3++ = *ptr2++;
			*ptr3++ = *ptr2; *ptr3++ = *(ptr2+1); *ptr3++ = *(ptr2+2);
			*ptr3++ = *ptr1; *ptr3++ = *(ptr1+1); *ptr3++ = *(ptr1+2);
			polygon3d(fildes,side,4,FALSE);
		}
		
		path_ptr += 3;
	}

	if (back_face) {
		/* do the back face */
		_prim_push_vertex_order(fildes,TOGGLE_ORDER);
		polygon3d(fildes,poly,poly_size,FALSE);
		_prim_pop_vertex_format(fildes);
	}

	/* clean up */
	_prim_free_object(oldpoly);
	_prim_free_object(poly);
}


static void rotmat(
    float *mat,
    float nx, float ny, float nz)
{
	float len,alpha,ct,st,cp,sp;

	if ((len = FSQRT(nx*nx + ny*ny + nz*nz)) != 0.0) {
		nx /= len;
		ny /= len;
		nz /= len;
		alpha = FSQRT(nx*nx + nz*nz);
		if (alpha == 0.0) {
			st = (ny>0.0)?1.0:-1.0;
			ct = 0.0;
			sp = 0.0;
			cp = 1.0;
		}
		else {
			st = ny;
			ct = alpha;
			sp = nz/alpha;
			cp = nx/alpha;
		}
	}
	else {
		ct = cp = 1.0;
		st = sp = 0.0;
	}

	*mat++ = cp*ct;
	*mat++ = -cp*st;
	*mat++ = -sp;

	*mat++ = st;
	*mat++ = ct;
	*mat++ = 0.0;

	*mat++ = sp*ct;
	*mat++ = -sp*st;
	*mat++ = cp;
}


/*                     TUBE_EXTRUSION
 *
 * This procedure forms an extruded object by sweeping the polygon
 * given in clist along a path.  At each step of the path, the normal 
 * of the offset polygon remains parallel to the path, thus, the face
 * remains perpendicular to the path.
 *
 * fildes     -- integer; file descriptor
 * clist      -- float*; a simple list of the polygon's vertices
 * poly_size  -- integer; the number of vertices in clist
 * path       -- float*; a list of vectors, relative to the origin,
 *                to offset clist by to get the next set of control
 *                points in the extrusion
 * path_size  -- integer; the number of vectors in path
 * front_face -- boolean; whether to draw the front face
 * back_face  -- boolean; whether to draw the back face
 *
 */

void tube_extrusion(
    int fildes,
    float *clist, int poly_size,
    float *path,  int path_size,
    int front_face, int back_face)
{
	int i,j;
	float *mesh,*ptr1,*ptr2,*ptr3,*path_ptr,*tpoly;
	float nx,ny,nz,ax,ay,az,bx,by,bz;
	float x,y,z,offsetx,offsety,offsetz,orx,ory,orz;
	float mat[3][3];

	/* get temporary storage */
	if ((mesh = _prim_malloc_object((poly_size+1)*(path_size+1)*3)) == NULL) {
	    perror("tube_extrusion");
	    return;
	}
	if ((tpoly = _prim_malloc_object((poly_size+1)*3)) == NULL) {
	    perror("tube_extrusion");
	    _prim_free_object(mesh);
	    return;
	}

	/* find rough center of object */
	ptr1 = clist;
	orx = ory = orz = 0.0;
	for (i=0; i<poly_size; ++i) {
		orx += *ptr1++;
		ory += *ptr1++;
		orz += *ptr1++;
	}
	orx /= (float) poly_size;
	ory /= (float) poly_size;
	orz /= (float) poly_size;

	/* find three non-colinear points */
	ax = *(clist+3) - *(clist);
	ay = *(clist+4) - *(clist+1);
	az = *(clist+5) - *(clist+2);

	ptr2 = clist+6;
	do {
		bx = *ptr2++ - *(clist+3);
		by = *ptr2++ - *(clist+4);
		bz = *ptr2++ - *(clist+5);
		nx = ay*bz - by*az;
		ny = az*bx - bz*ax;
		nz = ax*by - bx*ay;
	} while ((nx == 0.0) && (ny == 0.0) && (nz == 0.0));

	/* compute rotation matrix so normal lies on x axis */
	rotmat((float *) mat,nx,ny,nz);

	/* copy the original poly rotated that way */
	ptr1 = clist;
	ptr2 = tpoly;
	for (i=0; i<poly_size; ++i) {
		x = *ptr1++ - orx;
		y = *ptr1++ - ory;
		z = *ptr1++ - orz;
		*ptr2++ = mat[0][0]*x + mat[1][0]*y + mat[2][0]*z;
		*ptr2++ = mat[0][1]*x + mat[1][1]*y + mat[2][1]*z;
		*ptr2++ = mat[0][2]*x + mat[1][2]*y + mat[2][2]*z;
	}

	/* add the first point back to the end */
	ptr1 = clist;
	x = *ptr1++ - orx;
	y = *ptr1++ - ory;
	z = *ptr1++ - orz;
	*ptr2++ = mat[0][0]*x + mat[1][0]*y + mat[2][0]*z;
	*ptr2++ = mat[0][1]*x + mat[1][1]*y + mat[2][1]*z;
	*ptr2++ = mat[0][2]*x + mat[1][2]*y + mat[2][2]*z;

	/* construct the mesh */
	path_ptr = path;
	ptr2 = mesh;
	offsetx = orx;
	offsety = ory;
	offsetz = orz;
	for (j=0; j<=path_size; ++j) {
		/* normal should be average of this path and next */
		if ((j == 0) || (j == path_size)) {
			/* first normal should be same as first path vector */
			nx = *(path_ptr);
			ny = *(path_ptr+1);
			nz = *(path_ptr+2);
		}
		else {
			nx = *(path_ptr  ) + *(path_ptr+3);
			ny = *(path_ptr+1) + *(path_ptr+4);
			nz = *(path_ptr+2) + *(path_ptr+5);
			path_ptr += 3;
		}
		rotmat((float *) mat,nx,ny,nz);
		/* rotate from poly with normal on X to poly with this normal */

		ptr1 = tpoly;
		ptr3 = ptr2;
		for (i=0; i<(poly_size+1); ++i) {
			x = *ptr1++;
			y = *ptr1++;
			z = *ptr1++;
			/* reverse rotation */
			*ptr2++ = mat[0][0]*x + mat[0][1]*y + mat[0][2]*z + offsetx;
			*ptr2++ = mat[1][0]*x + mat[1][1]*y + mat[1][2]*z + offsety;
			*ptr2++ = mat[2][0]*x + mat[2][1]*y + mat[2][2]*z + offsetz;
		}
		offsetx += *path_ptr;
		offsety += *(path_ptr+1);
		offsetz += *(path_ptr+2);

		if ((j == 0)
				&& front_face) {
			polygon3d(fildes,ptr3,poly_size,FALSE);
		}
		else if ((j == path_size)
				&& back_face) {
			polygon3d(fildes,ptr3,poly_size,FALSE);
		}
	}
	quadrilateral_mesh(fildes,mesh,path_size+1,poly_size+1,FALSE);


	if (back_face) {
		/* do the back face */
		_prim_push_vertex_order(fildes,TOGGLE_ORDER);
		polygon3d(fildes,mesh+(path_size)*3,poly_size,FALSE);
		_prim_pop_vertex_format(fildes);
	}

	/* clean up */
	_prim_free_object(tpoly);
	_prim_free_object(mesh);
}
