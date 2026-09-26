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
#include "prims.ih"



/*                              SPLINE_SPHERE
 *
 * This procedure draws a rational spline-surface sphere of specified
 * radius centered at x,y,z.
 *
 * fildes -- integer; file descriptor
 * radius -- float; sphere radius
 * x,y,z  -- floats; the center of the sphere
 */
void spline_sphere(
    int fildes,
    float radius,
    float x, float y, float z)
{
    static float object[9][5][4] = {
	{{ 0.0, 	1.0, 			0.0, 		1.0 },
	{ M_SQRT1_2,	M_SQRT1_2, 		0.0, 		M_SQRT1_2 },
	{ 1.0,		0.0,			0.0,		1.0 },
	{ M_SQRT1_2,	-M_SQRT1_2,		0.0,		M_SQRT1_2 },
	{ 0.0,		-1.0,			0.0,		1.0 }},

	{{ 0.0, 	M_SQRT1_2, 		0.0, 		M_SQRT1_2 },
	{ 0.5,		0.5,	 		0.5, 		0.5 },
	{ M_SQRT1_2,	0.0,			M_SQRT1_2,	M_SQRT1_2 },
	{ 0.5,		-0.5,			0.5,		0.5 },
	{ 0.0,		-M_SQRT1_2,		0.0,		M_SQRT1_2 }},

	{{ 0.0, 	1.0, 			0.0, 		1.0 },
	{ 0.0,		M_SQRT1_2, 		M_SQRT1_2, 	M_SQRT1_2 },
	{ 0.0,		0.0,			1.0,		1.0 },
	{ 0.0,		-M_SQRT1_2,		M_SQRT1_2,	M_SQRT1_2 },
	{ 0.0,		-1.0,			0.0,		1.0 }},

	{{ 0.0, 	M_SQRT1_2,		0.0, 		M_SQRT1_2 },
	{ -0.5,		0.5,	 		0.5, 		0.5 },
	{ -M_SQRT1_2,	0.0,			M_SQRT1_2,	M_SQRT1_2 },
	{ -0.5,		-0.5,			0.5,		0.5 },
	{ 0.0,		-M_SQRT1_2,		0.0,		M_SQRT1_2 }},

	{{ 0.0, 	1.0, 			0.0, 		1.0 },
	{ -M_SQRT1_2,	M_SQRT1_2, 		0.0, 		M_SQRT1_2 },
	{ -1.0,		0.0,			0.0,		1.0 },
	{ -M_SQRT1_2,	-M_SQRT1_2,		0.0,		M_SQRT1_2 },
	{ 0.0,		-1.0,			0.0,		1.0 }},

	{{ 0.0, 	M_SQRT1_2,		0.0, 		M_SQRT1_2 },
	{ -0.5,		0.5,	 		-0.5, 		0.5 },
	{ -M_SQRT1_2,	0.0,			-M_SQRT1_2,	M_SQRT1_2 },
	{ -0.5,		-0.5,			-0.5,		0.5 },
	{ 0.0,		-M_SQRT1_2,		0.0,		M_SQRT1_2 }},

	{{ 0.0, 	1.0, 			0.0, 		1.0 },
	{ 0.0,		M_SQRT1_2, 		-M_SQRT1_2,	M_SQRT1_2 },
	{ 0.0,		0.0,			-1.0,		1.0 },
	{ 0.0,		-M_SQRT1_2,		-M_SQRT1_2,	M_SQRT1_2 },
	{ 0.0,		-1.0,			0.0,		1.0 }},

	{{ 0.0, 	M_SQRT1_2,		0.0, 		M_SQRT1_2 },
	{ 0.5,		0.5,	 		-0.5, 		0.5 },
	{ M_SQRT1_2,	0.0,			-M_SQRT1_2,	M_SQRT1_2 },
	{ 0.5,		-0.5,			-0.5,		0.5 },
	{ 0.0,		-M_SQRT1_2,		0.0,		M_SQRT1_2 }},

	{{ 0.0, 	1.0, 			0.0, 		1.0 },
	{ M_SQRT1_2,	M_SQRT1_2, 		0.0, 		M_SQRT1_2 },
	{ 1.0,		0.0,			0.0,		1.0 },
	{ M_SQRT1_2,	-M_SQRT1_2,		0.0,		M_SQRT1_2 },
	{ 0.0,		-1.0,			0.0,		1.0 }}
    };
    static float uknot[] = { 0.0, 0.0, 0.0, 1.0, 1.0, 2.0, 2.0, 2.0 };
    static float vknot[] = { 0.0, 0.0, 0.0, 1.0, 1.0, 2.0, 2.0,
	    3.0, 3.0, 4.0, 4.0, 4.0 };
    float mat[4][4];

    /* scale and translate */
    _hp_identity(mat);
    mat[0][0] = mat[1][1] = mat[2][2] = radius;
    mat[3][0] = x; mat[3][1] = y; mat[3][2] = z;

    concat_transformation3d(fildes,mat,PRE,PUSH);

    _prim_push_vertex_format(fildes,3,3,0,FALSE,COUNTER_CLOCKWISE);
    _prim_push_u_knot(fildes,uknot,sizeof(uknot)/sizeof(float));
    _prim_push_v_knot(fildes,vknot,sizeof(vknot)/sizeof(float));
    spline_surface(fildes,(float *) object,5,9,QUADRATIC,QUADRATIC,TRUE);
    _prim_pop_knots(fildes);
    _prim_pop_vertex_format(fildes);
    pop_matrix(fildes);
}
