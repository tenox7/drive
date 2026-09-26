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


/* TRUNCATED CONE MODULE */
#include <stdio.h>
#include <math.h>
#include "prims.ih"


/*                     MESH_CONE
 *
 * This routine draws a cone or cylinder with optional end caps.
 *
 * fildes--int, file descriptor
 * b_rad --float, bottom radius
 * t_rad --float, top radius
 * b_cap --int, cap the bottom?
 * t_cap --int, cap the top?
 * facets   -- int, number of facets
 * xb,yb,zb -- float, location of the center of the bottom
 * xt,yt,zt -- float, location of the center of the top
 */
void mesh_cone(
    int fildes,
    float b_rad, float t_rad,
    int   b_cap, int   t_cap,
    int facets,
    float xb, float yb, float zb,
    float xt, float yt, float zt)
{
    float ang,d_ang;
    float *cone,*bptr,*tptr,ca,sa,raddiff;
    int i;
    float height = HYPOT3(xt-xb,yt-yb,zt-zb);
    float mat[4][4];
    float mag;

    /* Don't let the radii be zero -- this results on degenerate and
     * silly-looking polygons on many devices.
     */
    if (t_rad == 0.0) {
	if (b_rad == 0.0) {
	    t_rad = b_rad = height/1000000.0;
	}
	else t_rad = b_rad/1000000.0;
    }
    else if (b_rad == 0.0) b_rad = t_rad/1000000.0;

    ang     = 0.0;
    d_ang   = (2.0*M_PI)/facets;
    raddiff = b_rad - t_rad;

    if ((bptr = cone = _prim_malloc_object((facets+1)*6*2)) == NULL) {
	perror("mesh_cone");
	return;
    }

    tptr = bptr + (facets+1)*6;
    for (i=0; i<=facets; ++i) {
	sa = FSIN(ang);
	ca = FCOS(ang);
	ang += d_ang;
	/* x,y,z's */
	*bptr++ = ca*b_rad;
	*tptr++ = ca*t_rad;
	*bptr++ = sa*b_rad;
	*tptr++ = sa*t_rad;
	*bptr++ = 0.0;
	*tptr++ = height;
	/* normals */
	mag = height*ca *height*ca;
	mag +=  height*sa *  height*sa;
	mag += raddiff * raddiff;
	mag = FSQRT(mag);

	*bptr++ = *tptr++ = (height*ca)/mag;;
	*bptr++ = *tptr++ = (height*sa)/mag;
	*bptr++ = *tptr++ = (raddiff)/mag;
    }

    _prim_construct_orientation_matrix(mat,xb,yb,zb,xt,yt,zt);
    _prim_xform_points(cone,2*(facets+1),6,TRUE,mat);
    _prim_push_vertex_format(fildes,3,3,0,FALSE,CLOCKWISE);
    quadrilateral_mesh(fildes,cone,2,(facets+1),NULL);
    _prim_pop_vertex_format(fildes);

    if (b_cap) {
	_prim_push_vertex_format(fildes,3,0,0,FALSE,COUNTER_CLOCKWISE);
	polygon3d(fildes,cone,facets,FALSE);
	_prim_pop_vertex_format(fildes);
    }

    if (t_cap) {
	_prim_push_vertex_format(fildes,3,0,0,FALSE,CLOCKWISE);
	polygon3d(fildes,cone+(facets+1)*6,facets,FALSE);
	_prim_pop_vertex_format(fildes);
    }

    _prim_free_object(cone);
}


void arrow(
    int fildes,
    float x1, float y1, float z1,
    float x2, float y2, float z2,
    float r,  float g,  float b,
    float coneheight)
{
    float pline[15],*fptr;
    float dx,dy,dz,len;

    line_color(fildes,r,g,b);
    fptr = pline;
    *fptr++ = x1;
    *fptr++ = y1;
    *fptr++ = z1;

    *fptr++ = x2;
    *fptr++ = y2;
    *fptr++ = z2;
    _prim_push_vertex_format(fildes,0,0,0,0,COUNTER_CLOCKWISE);
    polyline3d(fildes,pline,2,FALSE);
    _prim_pop_vertex_format(fildes);

    dx = x2 - x1;
    dy = y2 - y1;
    dz = z2 - z1;
    len = FSQRT(dx*dx+dy*dy+dz*dz);

    if (len > 0.0) {
	fill_color(fildes,r,g,b);
	mesh_cone(fildes,coneheight/4.0,0.0,FALSE,FALSE,8,
	    x2-coneheight*dx/len,y2-coneheight*dy/len,z2-coneheight*dz/len,
	    x2,y2,z2);
    }
}
