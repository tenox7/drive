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
#include <math.h>
#include "prims.ih"


/*                     SPLINE_CONE
 *
 * This routine draws a cone or cylinder with optional end caps.
 *
 * fildes--int, file descriptor
 * b_rad --float, bottom radius
 * t_rad --float, top radius
 * b_cap --int, cap the bottom?
 * t_cap --int, cap the top?
 * xb,yb,zb -- float, location of the center of the bottom
 * xt,yt,zt -- float, location of the center of the top
 */
void spline_cone(
    int fildes,
    float b_rad, float t_rad,
    int   b_cap, int   t_cap,
    float xb, float yb, float zb,
    float xt, float yt, float zt)
{
    static float pt[2][9][4] = {
       {{ 1.0,	 	0.0,	 	 0.0,	 	1.0 },
	{ M_SQRT1_2,	 M_SQRT1_2,	 0.0,		M_SQRT1_2 },
	{ 0.0,	 	1.0,	 	 0.0,	 	1.0 },
	{ -M_SQRT1_2,	 M_SQRT1_2,	 0.0,	 	M_SQRT1_2 },
	{ -1.0,	 	0.0,		 0.0,	 	1.0 },
	{ -M_SQRT1_2,	 -M_SQRT1_2, 0.0,		M_SQRT1_2 },
	{ 0.0,	 	-1.0,	 	 0.0,	 	1.0 },
	{ M_SQRT1_2,	 -M_SQRT1_2, 0.0,		M_SQRT1_2 },
	{ 1.0,	 	0.0,	 	 0.0,	 	1.0 }},

       {{ 1.0,	 	0.0,	 	1.0,	 	1.0 },
	{ M_SQRT1_2,	M_SQRT1_2,	M_SQRT1_2,	M_SQRT1_2 },
	{ 0.0,	 	1.0,	 	1.0,	 	1.0 },
	{ -M_SQRT1_2,	M_SQRT1_2,	M_SQRT1_2,	M_SQRT1_2 },
	{ -1.0,	 	0.0,	 	1.0,	 	1.0 },
	{ -M_SQRT1_2,	-M_SQRT1_2, M_SQRT1_2,	M_SQRT1_2 },
	{ 0.0,	 	-1.0,	 	1.0,	 	1.0 },
	{ M_SQRT1_2,	-M_SQRT1_2, M_SQRT1_2,	M_SQRT1_2 },
	{ 1.0,	 	0.0,	 	1.0,	 	1.0 }}
    };
    static float uknot[] = { 0.0,0.0,0.0,1.0,1.0,2.0,2.0,3.0,3.0,4.0,4.0,4.0 };
    static float vknot[] = { 0.0,0.0,1.0,1.0 };
    float height = HYPOT3(xt-xb,yt-yb,zt-zb);
    float mat[4][4];

    pt[0][0][0] = pt[0][2][1] = pt[0][8][0] = b_rad;
    pt[0][1][0] = pt[0][1][1] = pt[0][3][1] = pt[0][7][0] = b_rad*M_SQRT1_2;
    pt[0][3][0] = pt[0][5][0] = pt[0][5][1] = pt[0][7][1] = -pt[0][1][0];
    pt[0][4][0] = pt[0][6][1] = -b_rad;

    pt[1][0][0] = pt[1][2][1] = pt[1][8][0] = t_rad;
    pt[1][1][0] = pt[1][1][1] = pt[1][3][1] = pt[1][7][0] = t_rad*M_SQRT1_2;
    pt[1][3][0] = pt[1][5][0] = pt[1][5][1] = pt[1][7][1] = -pt[1][1][0];
    pt[1][4][0] = pt[1][6][1] = -t_rad;

    pt[1][0][2] = pt[1][2][2] = pt[1][4][2] = pt[1][6][2] = pt[1][8][2]
	= height;
    pt[1][1][2] = pt[1][3][2] = pt[1][5][2] = pt[1][7][2]
	= height*M_SQRT1_2;

    _prim_push_u_knot(fildes,uknot,sizeof(uknot)/sizeof(float));
    if (t_cap) {
	_prim_push_vertex_order(fildes,CLOCKWISE);
	_prim_trimmed_plane_center(fildes,&pt[1][0][0],9,QUADRATIC,TRUE,
	    0.0,0.0,height,TRUE);
	_prim_pop_vertex_format(fildes);
    }

    _prim_push_orientation_matrix(fildes,mat,xb,yb,zb,xt,yt,zt);
    _prim_push_vertex_format(fildes,3,3,0,FALSE,CLOCKWISE);
    _prim_push_v_knot(fildes,vknot,sizeof(vknot)/sizeof(float));
    spline_surface(fildes,(float *) pt,9,2,QUADRATIC,LINEAR,TRUE);
    _prim_pop_vertex_format(fildes);

    if (b_cap) {
	_prim_push_vertex_order(fildes,COUNTER_CLOCKWISE);
	_prim_trimmed_plane_center(fildes,(float *) pt,9,QUADRATIC,TRUE,
	    0.0,0.0,0.0,TRUE);
	_prim_pop_vertex_format(fildes);
    }

    _prim_pop_knots(fildes);
    pop_matrix(fildes);
}
