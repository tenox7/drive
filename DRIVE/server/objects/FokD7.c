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


/* Code Module for the Fokker D.VII WWI aeroplane object */

#include <stdio.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "prims.h"
#include "obj_common.h"
#include "demo_physics.h"

static int fildes;
static int FokD7_dl = -1;
static int shadow_dl = -1;

#define DEFAULT_RED	0.0
#define DEFAULT_GRN	0.0
#define DEFAULT_BLU	1.0

#define FOKD7_HORSEPOWER	(185.0)
#define SCALE			(2.0)
#define ZF_FRONT		(-1.0)
#define ZF_2			(ZF_FRONT+0.25)
#define Y_AXLE			(-1.80)
#define TIRE_INNER_RADIUS	(0.43)
#define TIRE_OUTER_RADIUS	(0.54)
#define TIRE_MID_RADIUS		(0.50)

static void Hfuselage(
    void)
{
    static float fuselage_side_righta[] = {
	.47,	.05,	ZF_FRONT,1.0,	0.0,	0.0,
	.47,	.31,	ZF_2,	1.0,	0.0,	0.0,
	.47,	.58,	0.10,	1.0,	0.0,	0.0,
	.47,	.75,	1.62,	1.0,	0.0,	0.0,
	.47,	.70,	4.75,	.999,	0.0,	0.04,
	
	.47,	-.05,	ZF_FRONT,1.0,	0.0,	0.0,
	.47,	-.31,	ZF_2,	1.0,	0.0,	0.0,
	.47,	-.58,	0.10,	1.0,	0.0,	0.0,
	.47,	-.70,	1.62,	1.0,	0.0,	0.0,
	.47,	-.55,	4.75,	.999,	0.0,	0.04,
    };
    static float fuselage_side_rightb[] = {
	.27,	-.32,	6.57,	.985,	0.0,	.170,
	.47,	-.55,	4.75,	.999,	0.0,	0.04,
	.47,	.70,	4.75,	.999,	0.0,	0.04,
	.27,	.50,	6.57,	.985,	0.0,	.170,	
    };
    static float fuselage_white_cross_right[] = {
	.467,	.03,	4.78,	1.0,	0.0,	0.0,
	.417,	.03,	5.23,	1.0,	0.0,	0.0,
	.417,	-.42,	5.23,	1.0,	0.0,	0.0,
	.395,	-.42,	5.43,	1.0,	0.0,	0.0,
	.395,	.03,	5.43,	1.0,	0.0,	0.0,
	.346,	.03,	5.88,	1.0,	0.0,	0.0,
	.346,	.23,	5.88,	1.0,	0.0,	0.0,
	.395,	.23,	5.43,	1.0,	0.0,	0.0,
	.395,	.64,	5.43,	1.0,	0.0,	0.0,
	.417,	.64,	5.23,	1.0,	0.0,	0.0,
	.417,	.23,	5.23,	1.0,	0.0,	0.0,
	.467,	.23,	4.78,	1.0,	0.0,	0.0,
    };
    static float fuselage_black_cross_right[] = {
	.467,	.18,	4.79,	1.0,	0.0,	0.0,
	.412,	.18,	5.28,	1.0,	0.0,	0.0,
	.412,	.63,	5.28,	1.0,	0.0,	0.0,
	.401,	.63,	5.38,	1.0,	0.0,	0.0,
	.401,	.18,	5.38,	1.0,	0.0,	0.0,
	.346,	.18,	5.87,	1.0,	0.0,	0.0,
	.346,	.08,	5.87,	1.0,	0.0,	0.0,
	.401,	.08,	5.38,	1.0,	0.0,	0.0,
	.401,	-.41,	5.38,	1.0,	0.0,	0.0,
	.412,	-.41,	5.28,	1.0,	0.0,	0.0,
	.412,	.08,	5.28,	1.0,	0.0,	0.0,
	.467,	.08,	4.79,	1.0,	0.0,	0.0,
    };
    static float fuselage_side_rightc[] = {
	.01,	-.20,	7.63,	.969,	0.0,	.247,
	.27,	-.32,	6.57,	.985,	0.0,	.170,
	.27,	.50,	6.57,	.985,	0.0,	.170,	
	.00,	.45,	7.63,	.969,	0.0,	.247,
    };
    static float fuselage_side_lefta[] = {
	-.47,	.05,	ZF_FRONT,-1.0,	0.0,	0.0,
	-.47,	.31,	ZF_2,	-1.0,	0.0,	0.0,
	-.47,	.58,	0.10,	-1.0,	0.0,	0.0,
	-.47,	.75,	1.62,	-1.0,	0.0,	0.0,
	-.47,	.70,	4.75,	-.999,	0.0,	0.04,
	
	-.47,	-.05,	ZF_FRONT,-1.0,	0.0,	0.0,
	-.47,	-.31,	ZF_2,	-1.0,	0.0,	0.0,
	-.47,	-.58,	0.10,	-1.0,	0.0,	0.0,
	-.47,	-.70,	1.62,	-1.0,	0.0,	0.0,
	-.47,	-.55,	4.75,	-.999,	0.0,	0.04,
    };
    static float fuselage_side_leftb[] = {
	-.27,	-.32,	6.57,	-.985,	0.0,	.170,
	-.47,	-.55,	4.75,	-.999,	0.0,	0.04,
	-.47,	.70,	4.75,	-.999,	0.0,	0.04,
	-.27,	.50,	6.57,	-.985,	0.0,	.170,	
    };
    static float fuselage_white_cross_left[] = {
	-.467,	.03,	4.78,	-1.0,	0.0,	0.0,
	-.417,	.03,	5.23,	-1.0,	0.0,	0.0,
	-.417,	-.42,	5.23,	-1.0,	0.0,	0.0,
	-.395,	-.42,	5.43,	-1.0,	0.0,	0.0,
	-.395,	.03,	5.43,	-1.0,	0.0,	0.0,
	-.346,	.03,	5.88,	-1.0,	0.0,	0.0,
	-.346,	.23,	5.88,	-1.0,	0.0,	0.0,
	-.395,	.23,	5.43,	-1.0,	0.0,	0.0,
	-.395,	.64,	5.43,	-1.0,	0.0,	0.0,
	-.417,	.64,	5.23,	-1.0,	0.0,	0.0,
	-.417,	.23,	5.23,	-1.0,	0.0,	0.0,
	-.467,	.23,	4.78,	-1.0,	0.0,	0.0,
    };
    static float fuselage_black_cross_left[] = {
	-.467,	.08,	4.79,	-1.0,	0.0,	0.0,
	-.412,	.08,	5.28,	-1.0,	0.0,	0.0,
	-.412,	-.41,	5.28,	-1.0,	0.0,	0.0,
	-.401,	-.41,	5.38,	-1.0,	0.0,	0.0,
	-.401,	.08,	5.38,	-1.0,	0.0,	0.0,
	-.346,	.08,	5.87,	-1.0,	0.0,	0.0,
	-.346,	.18,	5.87,	-1.0,	0.0,	0.0,
	-.401,	.18,	5.38,	-1.0,	0.0,	0.0,
	-.401,	.63,	5.38,	-1.0,	0.0,	0.0,
	-.412,	.63,	5.28,	-1.0,	0.0,	0.0,
	-.412,	.18,	5.28,	-1.0,	0.0,	0.0,
	-.467,	.18,	4.79,	-1.0,	0.0,	0.0,
    };
    static float fuselage_side_leftc[] = {
	-.01,	-.20,	7.63,	-.969,	0.0,	.247,
	-.27,	-.32,	6.57,	-.985,	0.0,	.170,
	-.27,	.50,	6.57,	-.985,	0.0,	.170,	
	-.00,	.45,	7.63,	-.969,	0.0,	.247,
    };
    static float front[] = {
	-0.23,	1.01,	ZF_FRONT,0.0,	1.0,	0.0,
	-0.47,	0.75,	ZF_FRONT,-.489,	0.0,	-.873,
	-0.47,	-.05,	ZF_FRONT,-.489,	0.0,	-.873,
	-0.47,	-.31,	ZF_2,	-.489,	0.0,	-.873,
	-0.47,	-.58,	0.10,	-.400,	-.525,	-.751,

	0.00,	1.08,	ZF_FRONT,0.0,	1.0,	0.0,
	0.00,	0.97,	ZF_FRONT-.25,	0.0,	0.0,	-1.0,
	0.00,	-.10,	ZF_FRONT-.23,	0.0,	0.0,	-1.0,
	0.00,	-.40,	ZF_2,	0.0,	-.884,	-.467,
	0.00,	-.58,	0.10,	0.0,	-.982,	-.187,

	0.23,	1.01,	ZF_FRONT,0.0,	1.0,	0.0,
	0.47,	0.75,	ZF_FRONT,.489,	0.0,	-.873,
	0.47,	-.05,	ZF_FRONT,.489,	0.0,	-.873,
	0.47,	-.31,	ZF_2,	.489,	0.0,	-.873,
	0.47,	-.58,	0.10,	.400,	-.525,	-.751,
    };
    static float fuselage_front_top_right[] = {
	0.47,	0.05,	ZF_FRONT,.966,	.259,	0.0,
	0.47,	0.75,	ZF_FRONT,.707,	.707,	0.0,
	0.23,	1.01,	ZF_FRONT,.500,	.866,	0.0,

	0.47,	0.31,	ZF_2,	.866,	.500,	0.0,
	0.47,	0.75,	ZF_2,	.707,	.707,	0.0,
	0.23,	1.01,	ZF_2,	.500,	.866,	0.0,

	0.47,	0.58,	0.10,	.866,	.500,	0.0,
	0.47,	0.75,	0.10,	.707,	.707,	0.0,
	0.23,	1.01,	0.10,	.500,	.866,	0.0,

	0.47,	0.65,	0.68,	.866,	.500,	0.0,
	0.47,	0.75,	0.68,	.707,	.707,	0.0,
	0.23,	1.01,	0.68,	.500,	.866,	0.0,
    };
    static float fuselage_front_top_left[] = {
	-0.47,	0.05,	ZF_FRONT,-.966,	.259,	0.0,
	-0.47,	0.75,	ZF_FRONT,-.707,	.707,	0.0,
	-0.23,	1.01,	ZF_FRONT,-.500,	.866,	0.0,

	-0.47,	0.31,	ZF_2,	-.866,	.500,	0.0,
	-0.47,	0.75,	ZF_2,	-.707,	.707,	0.0,
	-0.23,	1.01,	ZF_2,	-.500,	.866,	0.0,

	-0.47,	0.58,	0.10,	-.866,	.500,	0.0,
	-0.47,	0.75,	0.10,	-.707,	.707,	0.0,
	-0.23,	1.01,	0.10,	-.500,	.866,	0.0,

	-0.47,	0.65,	0.68,	-.866,	.500,	0.0,
	-0.47,	0.75,	0.68,	-.707,	.707,	0.0,
	-0.23,	1.01,	0.68,	-.500,	.866,	0.0,
    };
    static float fuselage_top_mid[] = {
	0.47,	0.65,	0.68,	.866,	.500,	0.0,
	0.23,	1.01,	0.68,	.500,	.866,	0.0,
	0.0,	1.10,	0.68,	0.0,	1.0,	0.0,
	-0.23,	1.01,	0.68,	-.500,	.866,	0.0,
	-0.47,	0.65,	0.68,	-.866,	.500,	0.0,

	0.47,	0.75,	2.03,	.866,	.500,	0.0,
	0.23,	1.01,	2.03,	.500,	.866,	0.0,
	0.0,	1.10,	2.03,	0.0,	1.0,	0.0,
	-0.23,	1.01,	2.03,	-.500,	.866,	0.0,
	-0.47,	0.75,	2.03,	-.866,	.500,	0.0,
    };
    static float firewall[] = {
	0.42,	-0.70,	2.03,	0.0,	0.0,	1.0,
	0.42,	0.74,	2.03,	0.0,	0.0,	1.0,
	0.20,	0.98,	2.03,	0.0,	0.0,	1.0,
	0.0,	1.05,	2.03,	0.0,	0.0,	1.0,
	-0.20,	0.98,	2.03,	0.0,	0.0,	1.0,
	-0.42,	0.74,	2.03,	0.0,	0.0,	1.0,
	-0.42,	-0.70,	2.03,	0.0,	0.0,	1.0,
    };
    static float cockpit_front_right[] = {
	0.47,	0.75,	2.03,	.866,	.500,	0.0,
	0.23,	1.01,	2.03,	.500,	.866,	0.0,
	0.23,	1.01,	2.37,	.500,	.866,	0.0,
	0.43,	0.80,	2.54,	.707,	.707,	0.0,
	0.47,	0.72,	2.72,	.866,	.500,	0.0,
    };
    static float cockpit_front_left[] = {
	-0.47,	0.75,	2.03,	-.866,	.500,	0.0,
	-0.23,	1.01,	2.03,	-.500,	.866,	0.0,
	-0.23,	1.01,	2.37,	-.500,	.866,	0.0,
	-0.43,	0.80,	2.54,	-.707,	.707,	0.0,
	-0.47,	0.72,	2.72,	-.866,	.500,	0.0,
    };
    static float fuselage_top_rear1[] = {
	.47,	.69,	4.94,	.342,	.940,	0.0,
	.47,	.72,	2.72,	.707,	.707,	0.0,
	.27,	.77,	4.94,	.174,	.985,	0.0,
	.34,	.85,	3.03,	.500,	.866,	0.0,
	.00,	.80,	4.94,	0.0,	1.0,	0.0,
	.00,	.95,	3.12,	0.0,	1.0,	0.0,
	-.27,	.77,	4.94,	-.174,	.985,	0.0,
	-.34,	.85,	3.03,	-.500,	.866,	0.0,
	-.47,	.69,	4.94,	-.342,	.940,	0.0,
	-.47,	.72,	2.72,	-.707,	.707,	0.0,
    };
    static float fuselage_top_rear2[] = {
	.47,	.69,	4.94,	.342,	.940,	0.0,
	.27,	.77,	4.94,	.174,	.985,	0.0,
	.00,	.80,	4.94,	0.0,	1.0,	0.0,
	-.27,	.77,	4.94,	-.174,	.985,	0.0,
	-.47,	.69,	4.94,	-.342,	.940,	0.0,

	.35,	.55,	6.06,	0.0,	1.0,	0.0,
	.23,	.55,	6.06,	0.0,	1.0,	0.0,
	.00,	.55,	6.06,	0.0,	1.0,	0.0,
	-.23,	.55,	6.06,	0.0,	1.0,	0.0,
	-.35,	.55,	6.06,	0.0,	1.0,	0.0,
    };
    static float fuselage_bottom[] = {
	.47,	-.58,	0.10,	0.0,	-.991,	.137,
	.47,	-.70,	1.62,	0.0,	-1.00,	0.0,
	.47,	-.54,	4.90,	0.0,	-1.00,	0.0,
	.27,	-.32,	6.57,	0.0,	-.992,	-.124,
	.01,	-.20,	7.63,	0.0,	-.992,	-.124,

	-.47,	-.58,	0.10,	0.0,	-.991,	.137,
	-.47,	-.70,	1.62,	0.0,	-1.00,	0.0,
	-.47,	-.54,	4.90,	0.0,	-1.00,	0.0,
	-.27,	-.32,	6.57,	0.0,	-.992,	-.124,
	-.01,	-.20,	7.63,	0.0,	-.992,	-.124,
    };
	    

    hidden_surface(fildes,TRUE,FALSE);
    PAINT(fildes,1.0,0.0,0.0);
    /* fuselage */
    vertex_format(fildes,3,3,0,FALSE,CLOCKWISE|UNIT_NORMALS);
    quadrilateral_mesh(fildes,fuselage_side_righta,2,5,NULL);
    partial_polygon3d(fildes,fuselage_white_cross_right,
       (sizeof(fuselage_white_cross_right)/sizeof(float)/6),FALSE,TRUE);
    polygon3d(fildes,fuselage_side_rightb,4,FALSE);
    polygon3d(fildes,fuselage_side_rightc,4,FALSE);
    quadrilateral_mesh(fildes,fuselage_front_top_right,4,3,NULL);
    quadrilateral_mesh(fildes,fuselage_top_mid,2,5,NULL);
    polygon3d(fildes,cockpit_front_right,5,FALSE);
    triangular_strip(fildes,fuselage_top_rear1,10,NULL);
    quadrilateral_mesh(fildes,fuselage_top_rear2,2,5,NULL);
    quadrilateral_mesh(fildes,fuselage_bottom,2,5,NULL);
    vertex_format(fildes,3,3,0,FALSE,COUNTER_CLOCKWISE|UNIT_NORMALS);
    quadrilateral_mesh(fildes,fuselage_side_lefta, 2,5,NULL);
    partial_polygon3d(fildes,fuselage_white_cross_left,
	    (sizeof(fuselage_white_cross_left)/sizeof(float)/6),FALSE,TRUE);
    polygon3d(fildes,fuselage_side_leftb,4,FALSE);
    polygon3d(fildes,fuselage_side_leftc,4,FALSE);
    quadrilateral_mesh(fildes,fuselage_front_top_left,4,3,NULL);
    polygon3d(fildes,cockpit_front_left,5,FALSE);

    WOOD(fildes);
    polygon3d(fildes,firewall,7,FALSE);

    STEEL(fildes);
    quadrilateral_mesh(fildes,front,3,5,NULL);

    hidden_surface(fildes,TRUE,TRUE);
    PAINT(fildes,1.0,1.0,1.0);
    partial_polygon3d(fildes,fuselage_black_cross_right,
	    (sizeof(fuselage_black_cross_right)/sizeof(float)/6),
	    FALSE,FALSE);
    polygon3d(fildes,fuselage_white_cross_right,
	    (sizeof(fuselage_white_cross_right)/sizeof(float)/6),FALSE);
    partial_polygon3d(fildes,fuselage_black_cross_left,
	    (sizeof(fuselage_black_cross_left)/sizeof(float)/6),
	    FALSE,FALSE);
    polygon3d(fildes,fuselage_white_cross_left,
	    (sizeof(fuselage_white_cross_left)/sizeof(float)/6),FALSE);

    PAINT(fildes,0.0,0.0,0.0);
    polygon3d(fildes,fuselage_black_cross_right,
	    (sizeof(fuselage_black_cross_right)/sizeof(float)/6),FALSE);
    polygon3d(fildes,fuselage_black_cross_left,
	    (sizeof(fuselage_black_cross_left)/sizeof(float)/6),FALSE);

}


static void Hengine(
	void)
{
    static float engine_poly[] = {
	0.22,	0.93,	ZF_FRONT,
	0.22,	0.93,	0.68,
	-0.22,	0.93,	0.68,
	-0.22,	0.93,	ZF_FRONT,
    };
    float z;
    int i;

#define HENGINE_FACETS	4

    STEEL(fildes);
    polygon3d(fildes,engine_poly,4,FALSE);
    for (i=0, z=ZF_FRONT+0.15; i<6; ++i, z+=.233) {
	mesh_cone(fildes,0.1,0.1,FALSE,TRUE,HENGINE_FACETS,
		0.00,0.93,z, 0.00,1.10,z);
    }
}


static void Haileron_mtx(
	void)
{
    static float mat[4][4] = {
	{ 1.0,	0.0,	0.0,	0.0 },
	{ 0.0,	1.0,	0.0,	0.0 },
	{ 0.0,	0.0,	1.0,	0.0 },
	{ 0.0,	1.69,	1.90,	1.0 }
    };

    concat_transformation3d(fildes,mat,PRE,PUSH);
}


static void Hright_aileron(
	void)
{
    static float cross_right_white_aileron_out[] = {
	4.74,	-0.02,	0.32,
	4.74,	0.06,	0.00,
	4.68,	0.06,	0.00,
	4.68,	-0.02,	0.32,
    };
    static float cross_right_white_aileron_in[] = {
	4.40,	-0.02,	0.32,
	4.40,	0.06,	0.00,
	4.34,	0.06,	0.00,
	4.34,	-0.02,	0.32,
    };
    static float cross_right_black_aileron[] = {
	4.68,	-0.02,	0.32,
	4.68,	0.06,	0.00,
	4.40,	0.06,	0.00,
	4.40,	-0.02,	0.32,
    };

    static float aileron_right[] = {
	6.52,   0.01,   -0.13,
	3.57,   0.00,   0.09,
	3.57,   -0.06,  0.32,
	6.92,   -0.06,  0.32,
	7.12,   -0.06,  0.13,
	7.12,   0.01,   -0.23,
	6.91,   0.01,   -0.38,
	6.55,   -0.02,  -0.38,
    };

    hidden_surface(fildes,TRUE,FALSE);
    PAINT(fildes,0.0,0.2,1.0);
    vertex_format(fildes,0,0,0,FALSE,COUNTER_CLOCKWISE);
    polygon3d(fildes,aileron_right,
	    sizeof(aileron_right)/3/sizeof(float),FALSE);
    vertex_format(fildes,0,0,0,FALSE,CLOCKWISE);

    hidden_surface(fildes,TRUE,TRUE);
    PAINT(fildes,1.0,1.0,1.0);
    polygon3d(fildes,cross_right_white_aileron_out,
	    (sizeof(cross_right_white_aileron_out)/sizeof(float)/3),FALSE);
    polygon3d(fildes,cross_right_white_aileron_in,
	    (sizeof(cross_right_white_aileron_in)/sizeof(float)/3),FALSE);
    PAINT(fildes,0.0,0.0,0.0);
    polygon3d(fildes,cross_right_black_aileron,
	    (sizeof(cross_right_black_aileron)/sizeof(float)/3),FALSE);
}


static void Hleft_aileron(
	void)
{
    static float cross_left_white_aileron_out[] = {
	-4.68,	-0.02,	0.32,
	-4.68,	0.06,	0.00,
	-4.74,	0.06,	0.00,
	-4.74,	-0.02,	0.32,
    };
    static float cross_left_white_aileron_in[] = {
	-4.34,	-0.02,	0.32,
	-4.34,	0.06,	0.00,
	-4.40,	0.06,	0.00,
	-4.40,	-0.02,	0.32,
    };
    static float cross_left_black_aileron[] = {
	-4.40,	-0.02,	0.32,
	-4.40,	0.06,	0.00,
	-4.68,	0.06,	0.00,
	-4.68,	-0.02,	0.32,
    };

    static float aileron_left[] = {
	-6.52,   0.01,   -0.13,
	-3.57,   0.00,   0.09,
	-3.57,   -0.06,  0.32,
	-6.92,   -0.06,  0.32,
	-7.12,   -0.06,  0.13,
	-7.12,   0.01,   -0.23,
	-6.91,   0.01,   -0.38,
	-6.55,   -0.02,  -0.38,
    };

    hidden_surface(fildes,TRUE,FALSE);
    PAINT(fildes,0.0,0.2,1.0);
    vertex_format(fildes,0,0,0,FALSE,COUNTER_CLOCKWISE);
    polygon3d(fildes,aileron_left,
	    sizeof(aileron_left)/3/sizeof(float),FALSE);
    vertex_format(fildes,0,0,0,FALSE,CLOCKWISE);

    hidden_surface(fildes,TRUE,TRUE);
    PAINT(fildes,1.0,1.0,1.0);
    polygon3d(fildes,cross_left_white_aileron_out,
	    (sizeof(cross_left_white_aileron_out)/sizeof(float)/3),FALSE);
    polygon3d(fildes,cross_left_white_aileron_in,
	    (sizeof(cross_left_white_aileron_in)/sizeof(float)/3),FALSE);
    PAINT(fildes,0.0,0.0,0.0);
    polygon3d(fildes,cross_left_black_aileron,
	    (sizeof(cross_left_black_aileron)/sizeof(float)/3),FALSE);
}


static void Hwings(
	void)
{
    static float uwing_outer_front_right[] = {
	6.52, 	1.65, 	0.00,	0.00,	-SIN60,	-COS60,
	3.57,	1.65,	0.00,	0.00,	-SIN60,	-COS60,
	3.57,	1.88,	0.60,	0.00,	-1.00,	0.00,
	6.71,	1.88,	0.60,	0.00,	-1.00,	0.00,
	6.65,	1.75,	0.32,	0.00,	-SIN75,	-COS75,
    };
    static float uwing_outer_front_left[] = {
	-6.52, 	1.65, 	0.00,	0.00,	-SIN60,	-COS60,
	-3.57,	1.65,	0.00,	0.00,	-SIN60,	-COS60,
	-3.57,	1.88,	0.60,	0.00,	-1.00,	0.00,
	-6.71,	1.88,	0.60,	0.00,	-1.00,	0.00,
	-6.65,	1.75,	0.32,	0.00,	-SIN75,	-COS75,
    };
    static float uwing_outer_rear_right[] = {
	6.71,	1.88,	0.60,	0.00,	-1.00,	0.00,
	3.57,	1.88,	0.60,	0.00,	-1.00,	0.00,
	3.57,	1.69,	1.99,	0.00,	-SIN75,	COS75,
	6.52,	1.70,	1.77,	0.00,	-SIN75,	COS75,
	6.67,	1.75,	1.15,	0.00,	-SIN75,	COS75,
    };
    static float uwing_outer_rear_left[] = {
	-6.71,	1.88,	0.60,	0.00,	-1.00,	0.00,
	-3.57,	1.88,	0.60,	0.00,	-1.00,	0.00,
	-3.57,	1.69,	1.99,	0.00,	-SIN75,	COS75,
	-6.52,	1.70,	1.77,	0.00,	-SIN75,	COS75,
	-6.67,	1.75,	1.15,	0.00,	-SIN75,	COS75,
    };

    static float cross_right_white_front_out[] = {
	4.68,	1.92,	0.60,	0.00,	1.00,	0.00,
	4.74,	1.92,	0.60,	0.00,	1.00,	0.00,
	4.74,	1.69,	0.00,	0.00,	SIN60,	-COS60,
	4.68,	1.69,	0.00,	0.00,	SIN60,	-COS60,
    };
    static float cross_right_white_mid_out[] = {
	4.68,	1.92,	0.60,	0.00,	1.00,	0.00,
	4.68,	1.87,	0.98,	0.00,	SIN75,	COS75,
	5.66,	1.87,	0.98,	0.00,	SIN75,	COS75,
	5.66,	1.87,	0.92,	0.00,	SIN75,	COS75,
	4.74,	1.87,	0.92,	0.00,	SIN75,	COS75,
	4.74,	1.87,	0.92,	0.00,	SIN75,	COS75,
	4.74,	1.92,	0.60,	0.00,	1.00,	0.00,
    };
    static float cross_right_white_front_in[] = {
	4.40,	1.92,	0.60,	0.00,	1.00,	0.00,
	4.40,	1.69,	0.00,	0.00,	SIN60,	-COS60,
	4.34,	1.69,	0.00,	0.00,	SIN60,	-COS60,
	4.34,	1.92,	0.60,	0.00,	1.00,	0.00,
    };
    static float cross_right_white_mid_in[] = {
	3.42,	1.87,	0.98,	0.00,	SIN75,	COS75,
	4.40,	1.87,	0.98,	0.00,	SIN75,	COS75,
	4.40,	1.92,	0.60,	0.00,	1.00,	0.00,
	4.34,	1.92,	0.60,	0.00,	1.00,	0.00,
	4.34,	1.87,	0.92,	0.00,	SIN75,	COS75,
	3.42,	1.87,	0.92,	0.00,	SIN75,	COS75,
    };
    static float cross_right_black_front[] = {
	4.68,	1.92,	0.60,	0.00,	1.00,	0.00,
	4.68,	1.69,	0.00,	0.00,	SIN60,	-COS60,
	4.40,	1.69,	0.00,	0.00,	SIN60,	-COS60,
	4.40,	1.92,	0.60,	0.00,	1.00,	0.00,
    };
    static float cross_right_white_rear_out[] = {
	5.66,	1.82,	1.26,	0.00,	SIN75,	COS75,
	4.68,	1.82,	1.26,	0.00,	SIN75,	COS75,
	4.68,	1.75,	1.90,	0.00,	SIN75,	COS75,
	4.74,	1.75,	1.90,	0.00,	SIN75,	COS75,
	4.74,	1.82,	1.32,	0.00,	SIN75,	COS75,
	5.66,	1.82,	1.32,	0.00,	SIN75,	COS75,
    };
    static float cross_right_white_rear_in[] = {
	4.40,	1.75,	1.90,	0.00,	SIN75,	COS75,
	4.40,	1.82,	1.26,	0.00,	SIN75,	COS75,
	3.42,	1.82,	1.26,	0.00,	SIN75,	COS75,
	3.42,	1.82,	1.32,	0.00,	SIN75,	COS75,
	4.34,	1.82,	1.32,	0.00,	SIN75,	COS75,
	4.34,	1.75,	1.90,	0.00,	SIN75,	COS75,
    };
    static float cross_right_black_rear[] = {
	4.68,	1.87,	0.98,	0.00,	SIN75,	COS75,
	4.68,	1.92,	0.60,	0.00,	1.00,	0.00,
	4.40,	1.92,	0.60,	0.00,	1.00,	0.00,
	4.40,	1.87,	0.98,	0.00,	SIN75,	COS75,
	3.42,	1.87,	0.98,	0.00,	SIN75,	COS75,
	3.42,	1.82,	1.26,	0.00,	SIN75,	COS75,
	4.40,	1.82,	1.26,	0.00,	SIN75,	COS75,
	4.40,	1.75,	1.90,	0.00,	SIN75,	COS75,
	4.68,	1.75,	1.90,	0.00,	SIN75,	COS75,
	4.68,	1.82,	1.26,	0.00,	SIN75,	COS75,
	5.66,	1.82,	1.26,	0.00,	SIN75,	COS75,
	5.66,	1.87,	0.98,	0.00,	SIN75,	COS75,
    };
    static float uwing_front_center[] = {
	3.57,	1.65,	0.00,	0.00,	-SIN60,	-COS60,
	-3.57,	1.65,	0.00,	0.00,	-SIN60,	-COS60,
	-3.57,	1.88,	0.60,	0.00,	-1.00,	0.00,
	3.57,	1.88,	0.60,	0.00,	-1.00,	0.00,
    };
    static float uwing_back_center[] = {
	3.57,   1.88,   0.60,	0.00,	-1.00,	0.00,
	-3.57,	1.88,   0.60,	0.00,	-1.00,	0.00,
	-3.57,	1.67,   2.22,	0.00,	-SIN75,	COS75,
	-0.67,	1.67,   2.22,	0.00,	-SIN75,	COS75,
	0.00,   1.67,   2.02,	0.00,	-SIN75,	COS75,
	0.67,   1.67,   2.22,	0.00,	-SIN75,	COS75,
	3.57,   1.67,   2.22,	0.00,	-SIN75,	COS75,
    };
    static float lwing_front[] = {
	5.52,   -.65,   0.96,	0.00,	-SIN75,	COS75,
	-5.52,  -.65,   0.96,	0.00,	-SIN75,	COS75,
	-5.70,  -.59,   1.19,	0.00,	-SIN75,	COS75,
	-5.69,  -.49,   1.47,	0.00,	-SIN75,	COS75,
	5.69,   -.49,   1.47,	0.00,	-SIN75,	COS75,
	5.70,   -.59,   1.19,	0.00,	-SIN75,	COS75,
    };
    static float lwing_rear[] = {
	5.69,   -.49,   1.47,	0.00,	-SIN75,	COS75,
	-5.69,  -.49,   1.47,	0.00,	-SIN75,	COS75,
	-5.63,  -.55,   1.80,	0.00,	-SIN75,	COS75,
	-5.50,  -.59,   2.60,	0.00,	-SIN75,	COS75,
	5.50,   -.59,   2.60,	0.00,	-SIN75,	COS75,
	5.63,   -.55,   1.80,	0.00,	-SIN75,	COS75,
    };
    float mat[4][4];


    hidden_surface(fildes,TRUE,FALSE);
    PAINT(fildes,0.0,0.2,1.0);;
    vertex_format(fildes,3,3,0,FALSE,COUNTER_CLOCKWISE);
    polygon3d(fildes,uwing_outer_front_right,
	    sizeof(uwing_outer_front_right)/6/sizeof(float),FALSE);
    polygon3d(fildes,uwing_outer_rear_right,
	    sizeof(uwing_outer_rear_right)/6/sizeof(float),FALSE);
    polygon3d(fildes,uwing_front_center,
	    sizeof(uwing_front_center)/6/sizeof(float),FALSE);
    polygon3d(fildes,uwing_back_center,
	    sizeof(uwing_back_center)/6/sizeof(float),FALSE);
    vertex_format(fildes,3,3,0,FALSE,CLOCKWISE);
    polygon3d(fildes,uwing_outer_front_left,
	    sizeof(uwing_outer_front_left)/6/sizeof(float),FALSE);
    polygon3d(fildes,uwing_outer_rear_left,
	    sizeof(uwing_outer_rear_left)/6/sizeof(float),FALSE);
    vertex_format(fildes,3,3,0,FALSE,COUNTER_CLOCKWISE);
    polygon3d(fildes,lwing_front,
	    sizeof(lwing_front)/6/sizeof(float),FALSE);
    polygon3d(fildes,lwing_rear,
	    sizeof(lwing_rear)/6/sizeof(float),FALSE);

    vertex_format(fildes,3,3,0,FALSE,CLOCKWISE);
    hidden_surface(fildes,TRUE,TRUE);
    PAINT(fildes,1.0,1.0,1.0);
    polygon3d(fildes,cross_right_white_front_out,
	    (sizeof(cross_right_white_front_out)/sizeof(float)/6),FALSE);
    polygon3d(fildes,cross_right_white_front_in,
	    (sizeof(cross_right_white_front_in)/sizeof(float)/6),FALSE);
    polygon3d(fildes,cross_right_white_mid_out,
	    (sizeof(cross_right_white_mid_out)/sizeof(float)/6),FALSE);
    polygon3d(fildes,cross_right_white_mid_in,
	    (sizeof(cross_right_white_mid_in)/sizeof(float)/6),FALSE);
    polygon3d(fildes,cross_right_white_rear_out,
	    (sizeof(cross_right_white_rear_out)/sizeof(float)/6),FALSE);
    polygon3d(fildes,cross_right_white_rear_in,
	    (sizeof(cross_right_white_rear_in)/sizeof(float)/6),FALSE);

    PAINT(fildes,0.0,0.0,0.0);
    polygon3d(fildes,cross_right_black_front,
	    (sizeof(cross_right_black_front)/sizeof(float)/6),FALSE);
    polygon3d(fildes,cross_right_black_rear,
	    (sizeof(cross_right_black_rear)/sizeof(float)/6),FALSE);

    _hp_identity(mat);
    mat[3][0] = -(4.54*2.0);
    concat_transformation3d(fildes,mat,PRE,PUSH);
    PAINT(fildes,1.0,1.0,1.0);
    polygon3d(fildes,cross_right_white_front_out,
	    (sizeof(cross_right_white_front_out)/sizeof(float)/6),FALSE);
    polygon3d(fildes,cross_right_white_front_in,
	    (sizeof(cross_right_white_front_in)/sizeof(float)/6),FALSE);
    polygon3d(fildes,cross_right_white_mid_out,
	    (sizeof(cross_right_white_mid_out)/sizeof(float)/6),FALSE);
    polygon3d(fildes,cross_right_white_mid_in,
	    (sizeof(cross_right_white_mid_in)/sizeof(float)/6),FALSE);
    polygon3d(fildes,cross_right_white_rear_out,
	    (sizeof(cross_right_white_rear_out)/sizeof(float)/6),FALSE);
    polygon3d(fildes,cross_right_white_rear_in,
	    (sizeof(cross_right_white_rear_in)/sizeof(float)/6),FALSE);

    PAINT(fildes,0.0,0.0,0.0);
    polygon3d(fildes,cross_right_black_front,
	    (sizeof(cross_right_black_front)/sizeof(float)/6),FALSE);
    polygon3d(fildes,cross_right_black_rear,
	    (sizeof(cross_right_black_rear)/sizeof(float)/6),FALSE);
    pop_matrix(fildes);	
}


static void Hstruts(
	void)
{
#define STRUT_FACETS 3

    WOOD(fildes);
    /* outer, back to front */
    mesh_cone(fildes,.04,.04,FALSE,FALSE,STRUT_FACETS,
	     4.05,-0.52,1.88,  4.05,1.64,1.30);
    mesh_cone(fildes,.04,.04,FALSE,FALSE,STRUT_FACETS,
	     4.05,1.64,1.30,   4.05,-0.53,1.21);
    mesh_cone(fildes,.04,.04,FALSE,FALSE,STRUT_FACETS,
	     4.05,-0.53,1.21,  4.05,1.74,0.42);

    mesh_cone(fildes,.04,.04,FALSE,FALSE,STRUT_FACETS,
	    -4.05,-0.52,1.88, -4.05,1.64,1.30);
    mesh_cone(fildes,.04,.04,FALSE,FALSE,STRUT_FACETS,
	    -4.05,1.64,1.30,  -4.05,-0.53,1.21);
    mesh_cone(fildes,.04,.04,FALSE,FALSE,STRUT_FACETS,
	    -4.05,-0.53,1.21, -4.05,1.74,0.42);

    /* inner, back to front */
    mesh_cone(fildes,.04,.04,FALSE,FALSE,STRUT_FACETS,
	     0.47,0.73,0.91,   1.38,1.66,0.39);
    mesh_cone(fildes,.04,.04,FALSE,FALSE,STRUT_FACETS,
	     0.47,-0.56,0.09,  1.38,1.66,0.39);
    mesh_cone(fildes,.04,.04,FALSE,FALSE,STRUT_FACETS,
	     0.47,0.19,-0.34,  1.38,1.66,0.39);

    mesh_cone(fildes,.04,.04,FALSE,FALSE,STRUT_FACETS,
	    -0.47,0.73,0.91,  -1.38,1.66,0.39);
    mesh_cone(fildes,.04,.04,FALSE,FALSE,STRUT_FACETS,
	    -0.47,-0.56,0.09, -1.38,1.66,0.39);
    mesh_cone(fildes,.04,.04,FALSE,FALSE,STRUT_FACETS,
	    -0.47,0.19,-0.34, -1.38,1.66,0.39);
}


static void Htail(
	void) 
{
    static float upright_front[] = {
	0.0,	0.60,	6.12,
	0.0,	1.20,	7.24,
	0.0,	0.45,	7.24,
    };
    static float upright_rear[] = {
	0.0,	0.45,	7.24,
	0.0,	1.20,	7.24,
	0.0,	1.40,	7.63,
	0.0,	0.45,	7.63,
    };
    static float tail_plane[] = {
	1.78,   .47,    7.13,
	.39,    .56,    5.73,
	.35,    .54,    6.06,
	-.35,   .54,    6.06,
	-.39,   .56,    5.73,
	-1.78,  .47,    7.13,
	-1.78,  .45,    7.52,
	1.78,   .45,    7.52,
    };

    hidden_surface(fildes,TRUE,FALSE);
    PAINT(fildes,0.0,0.2,1.0);
    vertex_format(fildes,0,0,FALSE,FALSE,CLOCKWISE);
    polygon3d(fildes,upright_front,
	    (sizeof(upright_front)/sizeof(float)/3),FALSE);
    polygon3d(fildes,tail_plane,
	    (sizeof(tail_plane)/sizeof(float)/3),FALSE);
    PAINT(fildes,1.0,1.0,1.0);
    polygon3d(fildes,upright_rear,
	    (sizeof(upright_rear)/sizeof(float)/3),FALSE);

    hidden_surface(fildes,TRUE,TRUE);
    /* tail wheel */
    WOOD(fildes);
    mesh_cone(fildes,.04,.04,FALSE,FALSE,STRUT_FACETS,
	    0.0,-.52,7.76, 0.0,-.20,7.37);
    /* tail supports */
    mesh_cone(fildes,.03,.03,FALSE,FALSE,STRUT_FACETS,
	     0.02,-0.20,7.48,  1.44,0.42,7.45);
    mesh_cone(fildes,.03,.03,FALSE,FALSE,STRUT_FACETS,
	    -0.02,-0.20,7.48, -1.44,0.42,7.45);
}


static void Helevator_mtx(
	void)
{
    static float mat[4][4] = {
	{ 1.0,	0.0,	0.0,	0.0 },
	{ 0.0,	1.0,	0.0,	0.0 },
	{ 0.0,	0.0,	1.0,	0.0 },
	{ 0.0,	0.45,	7.52,	1.0 }
    };

    concat_transformation3d(fildes,mat,PRE,PUSH);
}


static void Helevator(
	void)
{
    static float elevator_right[] = {
	1.78,   0.0,    0.00,
	0.00,   0.0,    0.00,
	0.50,   0.0,    0.68,
	1.94,   0.0,    0.68,
	2.18,   0.0,    0.58,
	2.49,   0.0,    0.28,
	2.39,   0.0,    -0.08,
	2.08,   0.0,    -0.35,
	1.78,   0.0,    -0.39,
    };
    static float elevator_left[] = {
	-0.50,   0.0,    0.68,
	-0.00,   0.0,    0.00,
	-1.78,   0.0,    0.00,
	-1.78,   0.0,    -0.39,
	-2.08,   0.0,    -0.35,
	-2.39,   0.0,    -0.08,
	-2.49,   0.0,    0.28,
	-2.18,   0.0,    0.58,
	-1.94,   0.0,    0.68,
    };

    
    hidden_surface(fildes,TRUE,FALSE);
    PAINT(fildes,0.0,0.2,1.0);
    vertex_format(fildes,0,0,FALSE,FALSE,CLOCKWISE);
    polygon3d(fildes,elevator_right,
	    (sizeof(elevator_right)/sizeof(float)/3),FALSE);
    polygon3d(fildes,elevator_left,
	    (sizeof(elevator_left)/sizeof(float)/3),FALSE);
    hidden_surface(fildes,TRUE,TRUE);
}


static void Hrudder_mtx(
    void)
{
    static float mat[4][4] = {
	{ 1.0,	0.0,	0.0,	0.0 },
	{ 0.0,	1.0,	0.0,	0.0 },
	{ 0.0,	0.0,	1.0,	0.0 },
	{ 0.0,	0.00,	7.63,	1.0 }
    };

    concat_transformation3d(fildes,mat,PRE,PUSH);
}


static void Hrudder(
	void)
{
    static float rudderp[] = {
	0.0,    0.06,   0.45,
	0.0,    -.26,   0.00,
	0.0,    1.40,   0.00,
	0.0,    1.20,   -0.39,
	0.0,    1.57,   -0.29,
	0.0,    1.75,   -0.13,
	0.0,    1.77,   0.23,
	0.0,    1.62,   0.54,
	0.0,    1.25,   0.68,
	0.0,    0.49,   0.68,
    };
    static float rudder_cross[] = {
	0.0,	1.12,	0.01,
	0.0,	1.12,	0.31,
	0.0,	1.44,	0.31,
	0.0,	1.44,	0.39,
	0.0,	1.12,	0.39,
	0.0,	1.12,	0.67,
	0.0,	1.04,	0.67,
	0.0,	1.04,	0.39,
	0.0,	0.72,	0.39,
	0.0,	0.72,	0.31,
	0.0,	1.04,	0.31,
	0.0,	1.04,	0.01,
    };

    hidden_surface(fildes,TRUE,FALSE);
    PAINT(fildes,1.0,1.0,1.0);
    partial_polygon3d(fildes,rudder_cross,
	    (sizeof(rudder_cross)/sizeof(float)/3),FALSE,TRUE);
    polygon3d(fildes,rudderp,
	    (sizeof(rudderp)/sizeof(float)/3),FALSE);

    PAINT(fildes,0.0,0.0,0.0);
    polygon3d(fildes,rudder_cross,
	    (sizeof(rudder_cross)/sizeof(float)/3),FALSE);
    hidden_surface(fildes,TRUE,TRUE);

}



static void Hundercarriage(
	void)
{
    static float tire1[] = {
	1.13,	Y_AXLE+TIRE_INNER_RADIUS,	0.58,	-1.0,	0.0,	0.0,
	1.13,	Y_AXLE+TIRE_MID_RADIUS,	0.58,	-1.0,	0.0,	0.0,
	1.17,	Y_AXLE+TIRE_OUTER_RADIUS,	0.58,	0.0,	-1.0,	0.0,
	1.21,	Y_AXLE+TIRE_OUTER_RADIUS,	0.58,	0.0,	-1.0,	0.0,
	1.25,	Y_AXLE+TIRE_MID_RADIUS,	0.58,	1.0,	0.0,	0.0,
	1.25,	Y_AXLE+TIRE_INNER_RADIUS,	0.58,	1.0,	0.0,	0.0,
    };
    static float tire2[] = {
	-1.25,	Y_AXLE+TIRE_INNER_RADIUS,	0.58,	-1.0,	0.0,	0.0,
	-1.25,	Y_AXLE+TIRE_MID_RADIUS,	0.58,	-1.0,	0.0,	0.0,
	-1.21,	Y_AXLE+TIRE_OUTER_RADIUS,	0.58,	0.0,	-1.0,	0.0,
	-1.17,	Y_AXLE+TIRE_OUTER_RADIUS,	0.58,	0.0,	-1.0,	0.0,
	-1.13,	Y_AXLE+TIRE_MID_RADIUS,	0.58,	1.0,	0.0,	0.0,
	-1.13,	Y_AXLE+TIRE_INNER_RADIUS,	0.58,	1.0,	0.0,	0.0,
    };
    static float axle_plane[] = {
	1.08,   Y_AXLE-.11,  0.24,	0.0,	.985,	-.173,
	1.08,   Y_AXLE+.08,  0.57,	0.0,	1.0,	0.0,
	1.08,   Y_AXLE-.08,  1.32,	0.0,	.985,	.173,

	-1.08,  Y_AXLE-.11,  0.24,	0.0,	.985,	-.173,
	-1.08,  Y_AXLE+.08,  0.57,	0.0,	1.0,	0.0,
	-1.08,  Y_AXLE-.08,  1.32,	0.0,	.985,	1.73,
    };
    static float axle_plane_bottom[] = {
	1.08,   Y_AXLE-.08,  1.32,	0.0,	-1.0,	0.0,
	1.08,   Y_AXLE-.11,  0.24,	0.0,	-1.0,	0.0,
	-1.08,  Y_AXLE-.11,  0.24,	0.0,	-1.0,	0.0,
	-1.08,  Y_AXLE-.08,  1.32,	0.0,	-1.0,	0.0,
    };

    STEEL(fildes);
    /* struts */
    mesh_cone(fildes,.05,.05,FALSE,FALSE,STRUT_FACETS,
	     0.41,-0.71,1.17,  0.88,Y_AXLE,0.58);
    mesh_cone(fildes,.05,.05,FALSE,FALSE,STRUT_FACETS,
	     0.43,-0.57,0.12,  0.88,Y_AXLE,0.58);
    mesh_cone(fildes,.05,.05,FALSE,FALSE,STRUT_FACETS,
	    -0.41,-0.71,1.17, -0.88,Y_AXLE,0.58);
    mesh_cone(fildes,.05,.05,FALSE,FALSE,STRUT_FACETS,
	    -0.43,-0.57,0.12, -0.88,Y_AXLE,0.58);

    /* axle */
    mesh_cone(fildes,.05,.05,TRUE,TRUE,STRUT_FACETS,
	    1.40,Y_AXLE,0.58, 1.08,Y_AXLE,0.58);
    mesh_cone(fildes,.05,.05,TRUE,TRUE,STRUT_FACETS,
	    1.08,Y_AXLE,0.58, -1.40,Y_AXLE,0.58);

#define HWHEEL_FACETS 8

    PAINT(fildes,1.0,0.0,0.0);
    /* hub */
    mesh_cone(fildes,TIRE_INNER_RADIUS,.05,FALSE,FALSE,HWHEEL_FACETS,
	     1.25,Y_AXLE,0.58,  1.36,Y_AXLE,0.58);
    circle(fildes,TIRE_INNER_RADIUS,HWHEEL_FACETS,FALSE,
	     1.13,Y_AXLE,0.58,  2.00,Y_AXLE,0.58);
    mesh_cone(fildes,TIRE_INNER_RADIUS,.05,FALSE,FALSE,HWHEEL_FACETS,
	    -1.25,Y_AXLE,0.58, -1.36,Y_AXLE,0.58);
    circle(fildes,TIRE_INNER_RADIUS,HWHEEL_FACETS,FALSE,
	    -1.13,Y_AXLE,0.58, -2.00,Y_AXLE,0.58);

    RUBBER(fildes);
    /* tire */
    mesh_surface_of_revolution(fildes,tire1,
	    (sizeof(tire1)/sizeof(float)/6),HWHEEL_FACETS,TRUE,
	    1.40,Y_AXLE,0.58, 1.30,Y_AXLE,0.58);
    mesh_surface_of_revolution(fildes,tire2,
	    (sizeof(tire2)/sizeof(float)/6),HWHEEL_FACETS,TRUE,
	    1.40,Y_AXLE,0.58, 1.30,Y_AXLE,0.58);
    hidden_surface(fildes,TRUE,TRUE);

    PAINT(fildes,0.0,0.2,1.0);
    /* axle plane */
    vertex_format(fildes,3,3,0,0,COUNTER_CLOCKWISE|UNIT_NORMALS);
    quadrilateral_mesh(fildes,axle_plane,2,3,NULL);
    polygon3d(fildes,axle_plane_bottom,4,FALSE);
    vertex_format(fildes,0,0,FALSE,FALSE,COUNTER_CLOCKWISE);

}


static void Hmisc(
	void)
{
    static float stock_right[] = {
	 0.17,	1.20,	1.67,
	 0.17,	1.03,	1.67,
	 0.29,	1.03,	1.67,
	 0.29,	1.20,	1.67,
    };
    static float stock_left[] = {
	 -0.29,	1.20,	1.67,
	 -0.29,	1.03,	1.67,
	 -0.17,	1.03,	1.67,
	 -0.17,	1.20,	1.67,
    };
    static float path[] = {
	0.0,	0.0,	0.56,
    };

#define GUN_FACETS 4

    /* guns */
    STEEL(fildes);
    mesh_cone(fildes,0.12,0.12,TRUE,TRUE,GUN_FACETS,
	     0.23,1.11,0.93,  0.23,1.11,1.67);
    extrusion(fildes,stock_right,4,path,1,TRUE,TRUE);
    mesh_cone(fildes,0.12,0.12,TRUE,TRUE,GUN_FACETS,
	    -0.23,1.11,0.93, -0.23,1.11,1.67);
    extrusion(fildes,stock_left,4,path,1,FALSE,TRUE);
}


static void high_res_model(
	void)
{

    Hfuselage();
    Hwings();
    Haileron_mtx(); Hright_aileron(); pop_matrix(fildes);
    Haileron_mtx(); Hleft_aileron();  pop_matrix(fildes);
    Hstruts();
    Htail();
    Helevator_mtx(); Helevator(); pop_matrix(fildes);
    Hrudder_mtx(); Hrudder(); pop_matrix(fildes);
    Hundercarriage();
    Hengine();
    Hmisc();
}


static void Mfuselage(
	void)
{
    static float fuselage_side_right[] = {
	.47,	.05,	ZF_FRONT,
	.47,	.58,	0.10,
	.47,	.75,	1.62,
	.47,	.70,	4.75,
	.27,	.50,	6.57,
	.00,	.45,	7.63,
	
	.47,	-.05,	ZF_FRONT,
	.47,	-.58,	0.10,
	.47,	-.70,	1.62,
	.47,	-.55,	4.75,
	.27,	-.32,	6.57,
	.01,	-.20,	7.63,
    };
    static float fuselage_side_left[] = {
	-.47,	-.05,	ZF_FRONT,
	-.47,	-.58,	0.10,
	-.47,	-.70,	1.62,
	-.47,	-.55,	4.75,
	-.27,	-.32,	6.57,
	-.01,	-.20,	7.63,

	-.47,	.05,	ZF_FRONT,
	-.47,	.58,	0.10,
	-.47,	.75,	1.62,
	-.47,	.70,	4.75,
	-.27,	.50,	6.57,
	-.00,	.45,	7.63,
    };
    static float front[] = {
	0.23,	1.01,	ZF_FRONT,
	0.47,	0.75,	ZF_FRONT,
	0.47,	-.05,	ZF_FRONT,
	0.47,	-.58,	0.10,

	0.00,	1.08,	ZF_FRONT,
	0.00,	0.97,	ZF_FRONT-.25,
	0.00,	-.10,	ZF_FRONT-.23,
	0.00,	-.58,	0.10,	

	-0.23,	1.01,	ZF_FRONT,
	-0.47,	0.75,	ZF_FRONT,
	-0.47,	-.05,	ZF_FRONT,
	-0.47,	-.58,	0.10,
    };
    static float fuselage_top[] = {
	0.47,	0.05,	ZF_FRONT,
	0.0,	1.10,	ZF_FRONT,
	-0.47,	0.05,	ZF_FRONT,

	0.47,	0.31,	ZF_2,	
	0.0,	1.1,	ZF_2,	
	-0.47,	0.31,	ZF_2,	

	0.47,	0.58,	0.10,
	0.0,	1.1,	0.10,
	-0.47,	0.58,	0.10,

	0.47,	0.65,	0.68,
	0.0,	1.10,	0.68,
	-0.47,	0.65,	0.68,

	0.47,	0.75,	2.03,
	0.0,	1.10,	2.03,
	-0.47,	0.75,	2.03,

	.47,	.72,	2.72,
	0.0,	.95,	3.12,
	-.47,	.72,	2.72,

	.47,	.69,	4.94,
	0.0,	.80,	4.94,
	-.47,	.69,	4.94,

	.35,	.55,	6.06,
	0.0,	.55,	6.06,
	-.35,	.55,	6.06,
    };
    static float fuselage_bottom[] = {
	.47,	-.58,	0.10,
	.47,	-.70,	1.62,
	.47,	-.54,	4.90,
	.27,	-.32,	6.57,
	.01,	-.20,	7.63,

	-.47,	-.58,	0.10,	
	-.47,	-.70,	1.62,
	-.47,	-.54,	4.90,
	-.27,	-.32,	6.57,
	-.01,	-.20,	7.63,
    };
	    

    PAINT(fildes,1.0,0.0,0.0);
    vertex_format(fildes,0,0,0,FALSE,CLOCKWISE|UNIT_NORMALS);
    quadrilateral_mesh(fildes,fuselage_side_right,2,6,NULL);
    quadrilateral_mesh(fildes,fuselage_top,8,3,NULL);
    quadrilateral_mesh(fildes,fuselage_bottom,2,5,NULL);
    quadrilateral_mesh(fildes,fuselage_side_left, 2,6,NULL);

    STEEL(fildes);
    quadrilateral_mesh(fildes,front,3,4,NULL);
}


static void Mwings(
	void)
{
    static float uwing_front[] = {
	6.52, 	1.65, 	0.00,
	-6.52, 	1.65, 	0.00,
	-6.65,	1.75,	0.32,
	-6.71,	1.88,	0.60,
	6.71,	1.88,	0.60,
	6.65,	1.75,	0.32,
    };

    static float uwing_outer_rear_right[] = {
	6.71,	1.88,	0.60,
	3.57,	1.88,	0.60,
	3.57,	1.69,	1.99,
	6.52,	1.70,	1.77,
	6.67,	1.75,	1.15,
    };
    static float uwing_outer_rear_left[] = {
	-6.71,	1.88,	0.60,
	-3.57,	1.88,	0.60,
	-3.57,	1.69,	1.99,
	-6.52,	1.70,	1.77,
	-6.67,	1.75,	1.15,
    };

    static float uwing_back_center[] = {
	3.57,   1.88,   0.60,
	-3.57,	1.88,   0.60,
	-3.57,	1.67,   2.22,
	3.57,   1.67,   2.22,
    };
    static float lwing_front[] = {
	5.52,   -.65,   0.96,
	-5.52,  -.65,   0.96,
	-5.70,  -.59,   1.19,
	-5.69,  -.49,   1.47,
	5.69,   -.49,   1.47,
	5.70,   -.59,   1.19,
    };
    static float lwing_rear[] = {
	5.69,   -.49,   1.47,
	-5.69,  -.49,   1.47,
	-5.63,  -.55,   1.80,
	-5.50,  -.59,   2.60,
	5.50,   -.59,   2.60,
	5.63,   -.55,   1.80,
    };
    static float aileron_right[] = {
	6.52,   1.70,	1.87,
	3.57,   1.69,   1.99,
	3.57,   1.63,	2.22,
	6.92,   1.63,	2.22,
	7.12,   1.63,	2.03,
	7.12,   1.70,   1.67,
	6.91,   1.70,   1.52,
	6.55,   1.67,   1.52,
    };
    static float aileron_left[] = {
	-6.52,   1.70,   1.87,
	-3.57,   1.69,   1.99,
	-3.57,   1.63,   2.22,
	-6.92,   1.63,   2.22,
	-7.12,   1.63,   2.03,
	-7.12,   1.70,   1.67,
	-6.91,   1.70,	 1.52,
	-6.55,   1.67,   1.52,
    };

    PAINT(fildes,0.0,0.2,1.0);;
    vertex_format(fildes,0,0,0,FALSE,COUNTER_CLOCKWISE);
    polygon3d(fildes,uwing_front,
	    sizeof(uwing_front)/3/sizeof(float),FALSE);
    polygon3d(fildes,uwing_outer_rear_right,
	    sizeof(uwing_outer_rear_right)/3/sizeof(float),FALSE);
    polygon3d(fildes,uwing_back_center,
	    sizeof(uwing_back_center)/3/sizeof(float),FALSE);
    polygon3d(fildes,uwing_outer_rear_left,
	    sizeof(uwing_outer_rear_left)/3/sizeof(float),FALSE);
    polygon3d(fildes,lwing_front,
	    sizeof(lwing_front)/3/sizeof(float),FALSE);
    polygon3d(fildes,lwing_rear,
	    sizeof(lwing_rear)/3/sizeof(float),FALSE);
    polygon3d(fildes,aileron_right,
	    sizeof(aileron_right)/3/sizeof(float),FALSE);
    polygon3d(fildes,aileron_left,
	    sizeof(aileron_left)/3/sizeof(float),FALSE);

    vertex_format(fildes,0,0,0,FALSE,CLOCKWISE);
    polygon3d(fildes,uwing_front,
	    sizeof(uwing_front)/3/sizeof(float),FALSE);
    polygon3d(fildes,uwing_outer_rear_right,
	    sizeof(uwing_outer_rear_right)/3/sizeof(float),FALSE);
    polygon3d(fildes,uwing_back_center,
	    sizeof(uwing_back_center)/3/sizeof(float),FALSE);
    polygon3d(fildes,uwing_outer_rear_left,
	    sizeof(uwing_outer_rear_left)/3/sizeof(float),FALSE);
    polygon3d(fildes,lwing_front,
	    sizeof(lwing_front)/3/sizeof(float),FALSE);
    polygon3d(fildes,lwing_rear,
	    sizeof(lwing_rear)/3/sizeof(float),FALSE);
    polygon3d(fildes,aileron_right,
	    sizeof(aileron_right)/3/sizeof(float),FALSE);
    polygon3d(fildes,aileron_left,
	    sizeof(aileron_left)/3/sizeof(float),FALSE);
}


static void Mtail(
	void) 
{
    static float upright_front[] = {
	0.0,	0.60,	6.12,
	0.0,	1.20,	7.24,
	0.0,	0.45,	7.24,
    };
    static float upright_rear[] = {
	0.0,	0.45,	7.24,
	0.0,	1.20,	7.24,
	0.0,    1.20,   7.24,
	0.0,    1.57,   7.34,
	0.0,    1.75,   7.50,
	0.0,    1.77,   7.86,
	0.0,    1.62,   8.27,
	0.0,    1.25,   8.51,
	0.0,    0.49,   8.51,
	0.0,    0.06,   8.08,
	0.0,    -.26,   7.63,
    };
    static float tail_plane[] = {
	1.78,   .47,    7.13,
	.39,    .56,    5.73,
	.35,    .54,    6.06,
	-.35,   .54,    6.06,
	-.39,   .56,    5.73,
	-1.78,  .47,    7.13,
	-1.78,   0.45,    7.14,
	-2.08,   0.45,    7.18,
	-2.39,   0.45,    7.44,
	-2.49,   0.45,    7.80,
	-2.18,   0.45,    8.10,
	-1.94,   0.45,    8.20,
	-0.50,   0.45,    8.20,
	0.00,	0.45,	 7.52,
	0.50,   0.45,    8.20,
	1.94,   0.45,    8.20,
	2.18,   0.45,    8.10,
	2.49,   0.45,    7.80,
	2.39,   0.45,    7.44,
	2.08,   0.45,    7.18,
	1.78,   0.45,    7.14,
    };

    PAINT(fildes,0.0,0.2,1.0);
    vertex_format(fildes,0,0,FALSE,FALSE,COUNTER_CLOCKWISE);
    polygon3d(fildes,upright_front,
	    (sizeof(upright_front)/sizeof(float)/3),FALSE);
    polygon3d(fildes,tail_plane,
	    (sizeof(tail_plane)/sizeof(float)/3),FALSE);
    PAINT(fildes,1.0,1.0,1.0);
    polygon3d(fildes,upright_rear,
	    (sizeof(upright_rear)/sizeof(float)/3),FALSE);

    vertex_format(fildes,0,0,FALSE,FALSE,CLOCKWISE);
    PAINT(fildes,0.0,0.2,1.0);
    polygon3d(fildes,upright_front,
	    (sizeof(upright_front)/sizeof(float)/3),FALSE);
    polygon3d(fildes,tail_plane,
	    (sizeof(tail_plane)/sizeof(float)/3),FALSE);
    PAINT(fildes,1.0,1.0,1.0);
    polygon3d(fildes,upright_rear,
	    (sizeof(upright_rear)/sizeof(float)/3),FALSE);

}


static void Mundercarriage(
	void)
{
    static float tire1[] = {
	1.13,	Y_AXLE+TIRE_INNER_RADIUS,	0.58,	-1.0,	0.0,	0.0,
	1.19,	Y_AXLE+TIRE_OUTER_RADIUS,	0.58,	0.0,	-1.0,	0.0,
	1.25,	Y_AXLE+TIRE_INNER_RADIUS,	0.58,	1.0,	0.0,	0.0,
    };
    static float tire2[] = {
	-1.25,	Y_AXLE+TIRE_INNER_RADIUS,	0.58,	-1.0,	0.0,	0.0,
	-1.19,	Y_AXLE+TIRE_OUTER_RADIUS,	0.58,	0.0,	-1.0,	0.0,
	-1.13,	Y_AXLE+TIRE_INNER_RADIUS,	0.58,	1.0,	0.0,	0.0,
    };
    static float axle_plane[] = {
	1.08,   Y_AXLE-.11,  0.24,	0.0,	-.985,	-.173,
	-1.08,  Y_AXLE-.11,  0.24,	0.0,	-.985,	-.173,
	-1.08,  Y_AXLE-.08,  1.32,	0.0,	-.985,	1.73,
	1.08,   Y_AXLE-.08,  1.32,	0.0,	-.985,	.173,
    };
    static float axle_plane_rev[] = {
	1.08,   Y_AXLE-.08,  1.32,	0.0,	.985,	.173,
	-1.08,  Y_AXLE-.08,  1.32,	0.0,	.985,	1.73,
	-1.08,  Y_AXLE-.11,  0.24,	0.0,	.985,	-.173,
	1.08,   Y_AXLE-.11,  0.24,	0.0,	.985,	-.173,
    };


#define MWHEEL_FACETS	6
    PAINT(fildes,1.0,0.0,0.0);
    /* hub */
    mesh_cone(fildes,TIRE_INNER_RADIUS,.05,FALSE,FALSE,MWHEEL_FACETS,
	     1.25,Y_AXLE,0.58,  1.36,Y_AXLE,0.58);
    mesh_cone(fildes,TIRE_INNER_RADIUS,.05,FALSE,FALSE,MWHEEL_FACETS,
	    -1.25,Y_AXLE,0.58, -1.36,Y_AXLE,0.58);

    RUBBER(fildes);
    /* tire */
    mesh_surface_of_revolution(fildes,tire1,
	    (sizeof(tire1)/sizeof(float)/6),MWHEEL_FACETS,TRUE,
	    1.40,Y_AXLE,0.58, 1.30,Y_AXLE,0.58);
    mesh_surface_of_revolution(fildes,tire2,
	    (sizeof(tire2)/sizeof(float)/6),MWHEEL_FACETS,TRUE,
	    1.40,Y_AXLE,0.58, 1.30,Y_AXLE,0.58);

    PAINT(fildes,0.0,0.2,1.0);
    /* axle plane */
    vertex_format(fildes,3,3,0,0,COUNTER_CLOCKWISE|UNIT_NORMALS);
    polygon3d(fildes,axle_plane,4,FALSE);
    polygon3d(fildes,axle_plane_rev,4,FALSE);

}

static void mid_res_model(
	void)
{

    Mfuselage();
    Mwings();
    Mtail();
    Mundercarriage();
}


static void Lfuselage(
	int change_colors)
{
    static float fuselage_side_right[] = {
	.00,	.45,	7.63,
	.47,	.75,	1.62,
	.47,	1.01,	ZF_FRONT,
	
	.01,	-.20,	7.63,
	.47,	-.70,	1.62,
	.47,	-.05,	ZF_FRONT,
    };
    static float fuselage_side_left[] = {
	-.47,	1.01,	ZF_FRONT,
	-.47,	.75,	1.62,
	-.00,	.45,	7.63,
	
	-.47,	-.05,	ZF_FRONT,
	-.47,	-.70,	1.62,	
	-.01,	-.20,	7.63,	
    };
    static float front[] = {
	-.47,	1.01,	ZF_FRONT,
	-.47,	-.05,	ZF_FRONT,
	.47,	-.05,	ZF_FRONT,
	.47,	1.01,	ZF_FRONT,
    };
    static float fuselage_top[] = {
	-0.47,	1.01,	ZF_FRONT,
	0.47,	1.01,	ZF_FRONT,

	-0.47,	0.75,	2.03,
	0.47,	0.75,	2.03,

	-.35,	.55,	6.06,
	.35,	.55,	6.06,
    };
    static float fuselage_bottom[] = {
	-.47,	-.05,	ZF_FRONT,
	-.47,	-.70,	1.62,
	-.01,	-.20,	7.63,

	.47,	-.05,	ZF_FRONT,
	.47,	-.70,	1.62,
	.01,	-.20,	7.63,
    };
	    

    if (change_colors) PAINT(fildes,1.0,0.0,0.0);
    /* fuselage */
    quadrilateral_mesh(fildes,fuselage_side_right,2,3,NULL);
    quadrilateral_mesh(fildes,fuselage_top,3,2,NULL);
    quadrilateral_mesh(fildes,fuselage_bottom,2,3,NULL);
    quadrilateral_mesh(fildes,fuselage_side_left, 2,3,NULL);
    if (change_colors) STEEL(fildes);
    polygon3d(fildes,front,4,FALSE);

}


static void Lwings(
	int change_colors)
{
    static float uwing[] = {
	6.52, 	1.65, 	0.00,
	-6.52, 	1.65, 	0.00,
	-6.71,	1.67,   2.22,
	6.71,	1.67,   2.22,	
    };
    static float uwing_rev[] = {
	6.71,	1.67,   2.22,
	-6.71,	1.67,   2.22,
	-6.52, 	1.65, 	0.00,
	6.52, 	1.65, 	0.00,
    };

    static float lwing[] = {
	5.52,   -.65,   0.96,
	-5.52,  -.65,   0.96,
	-5.50,  -.59,   2.60,
	5.50,   -.59,   2.60,
    };
    static float lwing_rev[] = {
	5.50,   -.59,   2.60,
	-5.50,  -.59,   2.60,
	-5.52,  -.65,   0.96,
	5.52,   -.65,   0.96,
    };


    if (change_colors) PAINT(fildes,0.0,0.2,1.0);;
    polygon3d(fildes,uwing,4,FALSE);
    polygon3d(fildes,uwing_rev,4,FALSE);
    polygon3d(fildes,lwing,4,FALSE);
    polygon3d(fildes,lwing_rev,4,FALSE);

}


static void Ltail(
	int change_colors) 
{
    static float upright_front[] = {
	0.0,	0.60,	6.12,
	0.0,	1.20,	7.24,
	0.0,	0.45,	7.24,
    };
    static float upright_front_rev[] = {
	0.0,	0.45,	7.24,
	0.0,	1.20,	7.24,
	0.0,	0.60,	6.12,
    };
    static float upright_rear[] = {
	0.0,	0.45,	7.24,
	0.0,    1.20,   7.34,
	0.0,    1.77,   7.86,
	0.0,    1.25,   8.51,
	0.0,    0.49,   8.51,
	0.0,    -.26,   7.63,
    };
    static float upright_rear_rev[] = {
	0.0,    -.26,   7.63,
	0.0,    0.49,   8.51,
	0.0,    1.25,   8.51,
	0.0,    1.77,   7.86,
	0.0,    1.20,   7.34,
	0.0,	0.45,	7.24,
    };
    static float tail_plane[] = {
	.39,    .56,    5.73,
	2.49,   0.45,    8.20,
	-2.49,   0.45,    8.20,
	-.39,    .56,    5.73,
    };
    static float tail_plane_rev[] = {
	-.39,    .56,    5.73,
	-2.49,   0.45,    8.20,
	2.49,   0.45,    8.20,
	.39,    .56,    5.73,
    };

    if (change_colors) PAINT(fildes,0.0,0.2,1.0);
    polygon3d(fildes,upright_front,3,FALSE);
    polygon3d(fildes,upright_front_rev,3,FALSE);
    polygon3d(fildes,tail_plane,4,FALSE);
    polygon3d(fildes,tail_plane_rev,4,FALSE);

    if (change_colors) PAINT(fildes,1.0,1.0,1.0);
    polygon3d(fildes,upright_rear,6,FALSE);
    polygon3d(fildes,upright_rear_rev,6,FALSE);
}


static void Lundercarriage(
	int change_colors)
{
    static float axle_plane[] = {
	1.08,   Y_AXLE-.11,  0.24,
	-1.08,  Y_AXLE-.11,  0.24,
	-1.08,  Y_AXLE-.08,  1.32,
	1.08,   Y_AXLE-.08,  1.32,
    };
    static float axle_plane_rev[] = {
	1.08,   Y_AXLE-.08,  1.32,
	-1.08,  Y_AXLE-.08,  1.32,
	-1.08,  Y_AXLE-.11,  0.24,
	1.08,   Y_AXLE-.11,  0.24,
    };


#define LWHEEL_FACETS	4
    if (change_colors) PAINT(fildes,1.0,0.0,0.0);
    /* hub */
    mesh_cone(fildes,TIRE_INNER_RADIUS,.05,FALSE,FALSE,LWHEEL_FACETS,
	     1.25,Y_AXLE,0.58,  1.36,Y_AXLE,0.58);
    mesh_cone(fildes,TIRE_INNER_RADIUS,.05,FALSE,FALSE,LWHEEL_FACETS,
	    -1.25,Y_AXLE,0.58, -1.36,Y_AXLE,0.58);

    if (change_colors) PAINT(fildes,0.0,0.2,1.0);
    polygon3d(fildes,axle_plane,4,FALSE);
    polygon3d(fildes,axle_plane_rev,4,FALSE);

}


static void low_res_model(
    void)
{
    Lfuselage(TRUE);
    Lwings(TRUE);
    Ltail(TRUE);
    Lundercarriage(TRUE);
}


static int create_shadow_dl(
    void)
{
    static float mat[4][4] = {
	{ SCALE, 0.0, 0.0,  0.0 },
	{ 0.0, SCALE, 0.0,  0.0 },
	{ 0.0, 0.0, -SCALE, 0.0 },
	{ 0.0, 0.0,   0.0,  1.0 }
    };
    /* Make it small so it isn't seen unless center is seen. */
    static float mc_extent[2][3] = {
	{ -0.1, 0.0, -0.1 },
	{  0.1, 0.1,  0.1 }
    };
    int seg,graphics_seg;


    seg = get_dl_segment();
    graphics_seg = get_dl_segment();

    open_segment(img_fildes,graphics_seg,TRUE,FALSE);
	concat_transformation3d(fildes,mat,PRE,PUSH);
	SHADOW(img_fildes);
	TRANSPARENT_ON(img_fildes,0.5);
	Lfuselage(FALSE);
	Lwings(FALSE);
	Ltail(FALSE);
	Lundercarriage(FALSE);
	TRANSPARENT_OFF(img_fildes);
	pop_matrix(fildes);
    close_segment(img_fildes);

    open_segment(img_fildes,seg,TRUE,FALSE);
	set_extent(img_fildes,mc_extent);
	cond_execute_segment(img_fildes,CI_PRUNE,FALSE,graphics_seg);
    close_segment(img_fildes);

    return(seg);
}


static AEROPLANE *init_aeroplane(
    DRIVE_OBJECT *obj)
{
    AEROPLANE *pptr;

    if ((pptr = (AEROPLANE *) malloc(sizeof(AEROPLANE))) == NULL) {
	return(NULL);
    }

    pptr->pitch = 0.0;
    pptr->roll = 0.0;
    pptr->yaw = 0.0;
    pptr->angle_of_attack = 0.0;
    pptr->yaw_angle_of_attack = 0.0;
    pptr->speed = 0.0;

    /* these things are constants */
    pptr->COG_to_aileron  =  10.0;
    pptr->COG_to_elevator =  13.0;
    pptr->COG_to_rudder   =  13.0;
    pptr->aileron_area    =  32.0;
    pptr->elevator_area   =  20.0;
    pptr->rudder_area     =  10.0;
    pptr->fuselage_area   =  45.0;
    pptr->engine_torque   =   0.0;
    pptr->aspect_ratio    =   5.8;
    pptr->wing_area       = 128.0;
    pptr->front_area      =  30.0;
    pptr->prop_power      = FOKD7_HORSEPOWER;
    pptr->Cd_parasitic    =   0.06;

    return(pptr);
}


static VEHICLE_AUXDATA *init_vaux(
    DRIVE_OBJECT *obj)
{
    static VEHICLE_AUXDATA *vaux = NULL;

    if (vaux != NULL) {
	return(vaux);
    }

    /***** INIT AUXILIARY DATA *****/
    if ((vaux = (VEHICLE_AUXDATA *) malloc(sizeof(VEHICLE_AUXDATA))) == NULL) {
	return(NULL);
    }
    vaux->bbox_mc[PT_UFL][0] = SCALE * -6.71;
	    vaux->bbox_mc[PT_UFL][1] = SCALE * (Y_AXLE-TIRE_OUTER_RADIUS);
	    vaux->bbox_mc[PT_UFL][2] =  SCALE * -ZF_FRONT;
    vaux->bbox_mc[PT_UFR][0] =  SCALE * 6.71;
	    vaux->bbox_mc[PT_UFR][1] = SCALE * (Y_AXLE-TIRE_OUTER_RADIUS);
	    vaux->bbox_mc[PT_UFR][2] =  SCALE * -ZF_FRONT;
    vaux->bbox_mc[PT_UBR][0] = SCALE *  2.5;
	    vaux->bbox_mc[PT_UBR][1] = SCALE * -0.52;
	    vaux->bbox_mc[PT_UBR][2] = SCALE * -7.76;
    vaux->bbox_mc[PT_UBL][0] = SCALE * -2.5;
	    vaux->bbox_mc[PT_UBL][1] = SCALE * -0.52;
	    vaux->bbox_mc[PT_UBL][2] = SCALE * -7.76;

    vaux->bbox_mc[PT_TFL][0] = SCALE * -6.71;
	    vaux->bbox_mc[PT_TFL][1] = SCALE * 1.88;
	    vaux->bbox_mc[PT_TFL][2] =  SCALE * -ZF_FRONT;
    vaux->bbox_mc[PT_TFR][0] =  SCALE * 6.71;
	    vaux->bbox_mc[PT_TFR][1] = SCALE * 1.88;
	    vaux->bbox_mc[PT_TFR][2] =  SCALE * -ZF_FRONT;
    vaux->bbox_mc[PT_TBR][0] =  SCALE * 2.5;
	    vaux->bbox_mc[PT_TBR][1] = SCALE * 1.77;
	    vaux->bbox_mc[PT_TBR][2] = SCALE * -7.76;
    vaux->bbox_mc[PT_TBL][0] = SCALE * -2.5;
	    vaux->bbox_mc[PT_TBL][1] = SCALE * 1.77;
	    vaux->bbox_mc[PT_TBL][2] = SCALE * -7.76;
    vaux->COG_mc[0] =
	    (vaux->bbox_mc[PT_UFR][0] + vaux->bbox_mc[PT_UFL][0]) / 2.0;
    vaux->COG_mc[1] =
	    (vaux->bbox_mc[PT_TFL][1] + vaux->bbox_mc[PT_UFL][1]) / 2.0;
    vaux->COG_mc[2] =
	    (vaux->bbox_mc[PT_UBR][2] + vaux->bbox_mc[PT_UFR][2]) / 2.0;

    vaux->coefficient_of_drag = 0.010;
    vaux->wheel_torque_mult   = 75.0 /* *pobj->I[YD][YD] */;
    vaux->best_turn_speed     = MPH_TO_FPS(25.0);
    vaux->max_obstacle_height = 1.0;

    vaux->horsepower          = FOKD7_HORSEPOWER;
    vaux->peak_power_rpm      = 3500.0;
    vaux->two_peak_power      = 0.0;
    vaux->gear_best_speed[0]  = MPH_TO_FPS(15.0);	/* reverse */
    vaux->gear_best_speed[1]  = 0.0;			/* neutral */
    vaux->gear_best_speed[2]  = MPH_TO_FPS(10.0);	/* 1st */
    vaux->gear_best_speed[3]  = MPH_TO_FPS(25.0);	/* 2nd */
    vaux->gear_best_speed[4]  = MPH_TO_FPS(40.0);	/* 3rd */
    vaux->gear_best_speed[5]  = MPH_TO_FPS(60.0);	/* 4th */
    vaux->gear_best_speed[6]  = MPH_TO_FPS(90.0);	/* 5th */
    vaux->offroad_performance = 0.0;
    vaux->thrust_mechanism    = THRUSTER;

    vaux->left_gauge_class  = GAUGE_ANALOG_MPH;
    vaux->right_gauge_class = GAUGE_ANALOG_ALTITUDE;
    vaux->max_speed         = 150.0;
    vaux->max_rpm           = 8000.0;
    vaux->max_altitude      = 25000.0;

    return(vaux);
}

static int FokD7_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = 2.38;
    get_box_mc_normal(obj,x,y,z,sc->mc_normal);
    return(TRUE);
}


static int FokD7_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = 2.38;
    return(TRUE);
}


static void create_FokD7_graphics(
    DRIVE_OBJECT *obj)
{
    int PlaneSeg;
    int HighResSeg;
    int MidResSeg;
    int LowResSeg;
    static float mc_extent[2][3] = {
	{ -6.71, Y_AXLE, -8.51 },
	{  6.71,  1.67,    -ZF_FRONT }
    };
    static float mat[4][4] = {
	{ SCALE, 0.0, 0.0,  0.0 },
	{ 0.0, SCALE, 0.0,  0.0 },
	{ 0.0, 0.0, -SCALE, 0.0 },
	{ 0.0, 0.0,   0.0,  1.0 }
    };


    fildes = img_fildes;


    PlaneSeg = get_dl_segment();
    HighResSeg = get_dl_segment();
    MidResSeg = get_dl_segment();
    LowResSeg = get_dl_segment();

    obj->display_list = PlaneSeg;

    open_segment(img_fildes,HighResSeg,FALSE,FALSE);
	DIFFUSE_LIGHTING_ON(fildes);
	concat_transformation3d(fildes,mat,PRE,PUSH);
        high_res_model();
	pop_matrix(fildes);
	RESTORE_DEFAULT_VERTEX_FORMAT(fildes);
	DIFFUSE_LIGHTING_OFF(fildes);
    close_segment(img_fildes);

    open_segment(img_fildes,MidResSeg,FALSE,FALSE);
	DIFFUSE_LIGHTING_ON(fildes);
	concat_transformation3d(fildes,mat,PRE,PUSH);
        mid_res_model();
	pop_matrix(fildes);
	RESTORE_DEFAULT_VERTEX_FORMAT(fildes);
	DIFFUSE_LIGHTING_OFF(fildes);
    close_segment(img_fildes);

    open_segment(img_fildes,LowResSeg,FALSE,FALSE);
	concat_transformation3d(fildes,mat,PRE,PUSH);
        low_res_model();
	pop_matrix(fildes);
	RESTORE_DEFAULT_VERTEX_FORMAT(fildes);
    close_segment(img_fildes);

    open_segment(img_fildes,PlaneSeg,FALSE,FALSE);
      set_extent(img_fildes,mc_extent);
      cond_return(img_fildes,CI_PRUNE,TRUE);

      set_cull_size(img_fildes,30.0);
      cond_execute_segment(img_fildes,CI_CULL,TRUE,LowResSeg);
      cond_return(img_fildes,CI_CULL,TRUE);

      set_cull_size(img_fildes,60.0);
      cond_execute_segment(img_fildes,CI_CULL,TRUE,MidResSeg);
      cond_return(img_fildes,CI_CULL,TRUE);
      /* if we get to here, it means the thing is big...draw the full model */
      execute_segment(img_fildes,HighResSeg);
    close_segment(img_fildes);
}


void init_FokD7_object(
    DRIVE_OBJECT *obj)
{
    DRIVE_OBJECT *child;

    if(debug) printf(" inside init_FokD7_object() routine \n");
    if (obj->color[0] == DEFAULT_OBJECT_COLOR) obj->color[0] = DEFAULT_RED;
    if (obj->color[1] == DEFAULT_OBJECT_COLOR) obj->color[1] = DEFAULT_GRN;
    if (obj->color[2] == DEFAULT_OBJECT_COLOR) obj->color[2] = DEFAULT_BLU;


    if (FokD7_dl == -1) {
    	create_FokD7_graphics(obj);
	FokD7_dl = obj->display_list;
	shadow_dl = create_shadow_dl();
    }
    else {
	obj->display_list = FokD7_dl;
    }

    if ((obj->vehicle_auxdata = init_vaux(obj)) == NULL) {
	return;
    }
    if ((obj->aeroplane = init_aeroplane(obj)) == NULL) {
	return;
    }
    init_pobj(obj,1500.0,75.0);

    /* obj->update_self = apply_plane_physics; */
    obj->update_self = apply_car_physics;
    obj->bound_mc[0] = -3.30;
    obj->bound_mc[1] = -0.90;
    obj->bound_mc[2] = -7.20;
    obj->bound_mc[3] =  3.30;
    obj->bound_mc[4] =  2.40;
    obj->bound_mc[5] =  6.43;
    update_wc_bounds(obj);

    /* apply brake */
    obj->controls.pointer_x = 0.0;
    obj->controls.pointer_y = -1.0;

    obj->surface_chars_xyz  = FokD7_surface_chars_xyz;
    obj->surface_chars_bbox = FokD7_surface_chars_bbox;

    /* Create shadow child */
    obj->num_children = 1;
    if ((child = (DRIVE_OBJECT *) malloc(sizeof(DRIVE_OBJECT))) == NULL) {
	fprintf(stderr,"Out of malloc space!\n");
	return;
    }
    /* First clone myself */
    memcpy(child,obj,sizeof(DRIVE_OBJECT));
    child->idptr = &(object_id[SHADOW_ID_NUMBER]);
    child->num_children = 0;
    child->child_list = NULL;
    child->display_list = shadow_dl;
    /* Use same intersection routines. */
    /* I have no physics or controls */
    child->update_controls = NULL;
    child->pobj = NULL;
    child->vehicle_auxdata = NULL;
    child->aeroplane = NULL;
    child->update_self = NULL;
    child->upd = NULL;
    child->connection = 0;
    add_object_to_list(&(obj->child_list),child);

    obj->aim_xform = &(obj->xform);
    obj->fire_point[0] = 0.0;
    obj->fire_point[1] = 1.11;
    obj->fire_point[2] = 1.67+1.0;
}
