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


/*                     CIRCLE
 *
 * This routine draws a circle.
 *
 * fildes   -- int, file descriptor
 * radius   -- float, circle radius
 * facets   -- int, number of facets
 * partial  -- bool, use partial_polygon instead of polygon
 * xc,yc,zc -- float, location of the center of circle
 * xn,yn,zn -- float, point on normal vector from center
 */
void circle(
    int fildes,
    float radius,
    int facets,
    int partial,
    float xc, float yc, float zc,
    float xn, float yn, float zn)
{
    double ang=0.0,d_ang;
    float *circle,*cptr;
    int i;
    float mat[4][4];

    if ((cptr = circle = _prim_malloc_object(facets*3)) == NULL) {
	perror("circle");
	return;
    }

    d_ang = (2.0*M_PI)/facets;
    for (i=0; i<facets; ++i) {
	ang += d_ang;
	/* x,y,z's */
	*cptr++ = FCOS(ang) * radius;
	*cptr++ = FSIN(ang) * radius;
	*cptr++ = 0.0;
    }

    _prim_construct_orientation_matrix(mat,xc,yc,zc,xn,yn,zn);
    _prim_xform_points(circle,facets,3,FALSE,mat);
    _prim_push_vertex_format(fildes,0,0,0,FALSE,COUNTER_CLOCKWISE);
    if (partial) partial_polygon3d(fildes,circle,facets,FALSE,TRUE);
    else polygon3d(fildes,circle,facets,FALSE);
    _prim_pop_vertex_format(fildes);

    _prim_free_object(circle);
}
