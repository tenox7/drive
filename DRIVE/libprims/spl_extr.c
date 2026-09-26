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
#include "prims.ih"


/*                     SPLINE_EXTRUSION
 *
 * This procedure forms a spline surface by sweeping the polygon
 * given in clist along a path.  At each step of the path, the normal 
 * of the offset polygon remains equal to the normal of the original
 * polygon, thus, the back face will be "parallel" to the front face,
 * regardless of the path. Each triple in "path" is a vector relative
 * to the origin.  The v-knot vectors are set so that the extrusion
 * front and back faces are exactly those specified by clist and 
 * clist offset by the last vector in the path.  A number of vertices
 * may be duplicated at the end of the polygon to insure that the
 * curve is closed.
 *
 * fildes     -- integer; file descriptor
 * clist      -- float*; a simple list of the polygon's vertices
 * poly_size  -- integer; the number of vertices in clist
 * path       -- float*; a list of vectors, relative to the origin,
 *                to offset clist by to get the next set of control
 *                points in the extrusion
 * path_size  -- integer; the number of vectors in path
 * order_u    -- integer; the order of the spline curve going around
 *                the vertices of clist
 * order_v    -- integer; the order of the spline curve in the 
				  direction of the extrusion
 * front_face -- boolean; whether to draw the front face
 * back_face  -- boolean; whether to draw the back face
 * rational   -- boolean; whether the points in clist are rational
 *                b-spline control points (4 floats per vertex)
 * raw        -- boolean; if true, do not duplicate points or change
 *                the u_knot_vector
 *
 */
void spline_extrusion(
    int fildes,
    float *clist, int poly_size,
    float *path,  int path_size,
    int order_u,  int order_v,
    int front_face, int back_face,
    int rational,
    int raw)
{
    float *object,*ptr1,*ptr2,*ptr3=NULL,*path_ptr,*vknot,x;
    int i,j,true_poly_size,true_path_size,vknot_size,dims;

    rational = (rational != 0);
    dims = 3 + rational;

    if (front_face)
	_prim_trimmed_plane(fildes,clist,poly_size,order_u,rational,raw);

    if (raw) true_poly_size = poly_size;
    else true_poly_size = poly_size + order_u - 1;

    true_path_size = path_size + order_v - 1;
    vknot_size = true_path_size + order_v;
    if ((ptr1 = object = _prim_malloc_object(true_poly_size*true_path_size*dims))
		    == NULL) {
	perror("spline_extrusion");
	return;
    }
    if ((vknot = _prim_malloc_object(vknot_size)) == NULL) {
	perror("spline_extrusion");
	_prim_free_object(ptr1);
	return;
    }

    /* copy the poly */
    ptr2 = clist;
    for (i=0;i<poly_size*dims;++i) *ptr1++ = *ptr2++;
    /* duplicate points as needed */
    ptr2 = clist;
    for (;i<true_poly_size*dims;++i) *ptr1++ = *ptr2++;

    /* repeat this first surface as needed */
    for (i=0;i<(order_v-1)/2;++i) {
	    ptr2 = object;
	    for (j=0;j<true_poly_size*dims;++j) *ptr1++ = *ptr2++;
    }

    /* now make the rest of the surface */
    path_ptr = path;
    for (i=0;i<path_size;++i) {
	    ptr2 = object;
	    ptr3 = ptr1;
	    for (j=0;j<true_poly_size;++j) {
		    *ptr1++ = *ptr2++ + *path_ptr;
		    *ptr1++ = *ptr2++ + *(path_ptr+1);
		    *ptr1++ = *ptr2++ + *(path_ptr+2);
		    if (rational) *ptr1++ = *ptr2++;
	    }
	    path_ptr += 3;
    }

    /* repeat that last surface as needed */
    for (i=0;i<(order_v-2)/2;++i) {
	    ptr2 = ptr3;
	    for (j=0;j<true_poly_size*dims;++j) *ptr1++ = *ptr2++;
    }

    _prim_push_vertex_order(fildes,TOGGLE_ORDER);
    if (back_face) {
	_prim_trimmed_plane(fildes,ptr3,poly_size,order_u,rational,raw);
    }

    ptr1 = vknot; x = 0.0;
    for (i=0;i<order_v-1;++i) *ptr1++ = x;
    for (i=0;i<path_size+1;++i) *ptr1++ = x++;
    for (i=0;i<order_v-1;++i)   *ptr1++ = (float) path_size;

    _prim_push_vertex_format(fildes,3,3,0,FALSE,SAME_ORDER);
    if (!raw) _prim_push_default_u(fildes);
    _prim_push_v_knot(fildes,vknot,vknot_size);
    spline_surface(fildes,object,true_poly_size,true_path_size,
	    order_u,order_v,rational);
    if (!raw) _prim_pop_u_knot(fildes);
    _prim_pop_v_knot(fildes);
    _prim_pop_vertex_format(fildes);

    _prim_free_object(vknot);
    _prim_free_object(object);
    _prim_pop_vertex_format(fildes);
}
