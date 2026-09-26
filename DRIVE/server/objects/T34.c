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
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "prims.h"
#include "obj_common.h"
#include "demo_physics.h"

static int hull_dl = INVALID, hull_shadow_dl, turret_dl, turret_shadow_dl;
static int gun_dl, gun_shadow_dl, global_dl;

#define DEFAULT_RED	0.2
#define DEFAULT_GRN	0.4
#define DEFAULT_BLU	0.1


#define HULL_COLOR(fildes)				\
	surface_model(fildes,FALSE,6,0.1,0.3,0.2);	

#define TTOP_Y			(4.0)
#define TMID_Y			(2.5)
#define TBASE_R			(2.9)
#define TBASE_Y			(DECK_Y+0.5)
#define TURRET_XEXTENT		(2.9)
#define TURRET_ZMIN		(-5.1)
#define TURRET_ZMAX		(3.8)
#define DOME1_X			(0.8)
#define DOME2_X			(-0.4)
#define DOME_R			(0.6)
#define DOME_Z			(-3.7)

#define DECK_Y			(0.8)

#define TIRE_Y			(-2.5)
#define TIRE_IR			(1.2)
#define TIRE_OR			(1.4)
#define TIRE_X			(5.3)
#define TIRE_WIDTH		(0.4)
#define TIRE_Z1			(3.9)
#define TIRE_Z2			(0.8)
#define TIRE_Z3			(-2.7)
#define TIRE_Z4			(-5.7)
#define TIRE_Z5			(-8.6)
#define AXLE_RADIUS		(0.3)
#define WHEEL_FACETS		(8)
#define UH_Y			(-0.4)
#define FSPR_R			(0.8)
#define FSPR_Y			(-1.6)
#define FSPR_Z			(6.6)
#define RSPR_R			(1.0)
#define RSPR_Y			(-1.8)
#define RSPR_Z			(-11.3)
#define TREAD_THICKNESS 	(0.1)
#define TREAD_WIDTH		(1.6)
#define FENDER_CLEARANCE	(0.5)
#define FENDER_WIDTH		(TREAD_WIDTH+0.25)
#define FUEL_RADIUS		(0.6)
#define FUEL_FACETS		(6)
#define HULL_SIDE_X		(TIRE_X-TREAD_WIDTH-0.5)
#define FENDER_Y		(FSPR_Y+FSPR_R+FENDER_CLEARANCE)

static int fildes;


static int T34_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x,float y,float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = TTOP_Y;
    get_box_mc_normal(obj,x,y,z,sc->mc_normal);
    return(TRUE);
}


static int T34_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = TTOP_Y;
    return(TRUE);
}


#define ROTATION_RATE	(M_PI/8)	/* radians per second */
#define ELEVATION_RATE	(M_PI/16)	/* radians per second */
#define MAX_ELEVATION	DEGREES_TO_RADIANS(29)
#define MAX_DEPRESSION	DEGREES_TO_RADIANS(-4)

static RETURN_CONDITION apply_tank_physics(
    DRIVE_OBJECT *obj,
    float t_interval)
{
    DRIVE_OBJECT *gun    = obj->child_list;
    DRIVE_OBJECT *turret = obj->child_list->next;
    DRIVE_OBJECT *hull   = obj->child_list->next->next;
    RETURN_CONDITION ret;
    static float ymat[4][4] = IDENTITY4x4;
    static float xmat[4][4] = IDENTITY4x4;

    /* Treat this like a car */
    ret = apply_car_physics(obj,t_interval);

    /* Now apply xform to children */
    memcpy(hull->xform,obj->xform,sizeof(float)*16);

    /* Rotate turret */
    obj->angle += ROTATION_RATE*t_interval*obj->controls.aim_pointer_x;
    ymat[0][0] = ymat[2][2] = FCOS(obj->angle);
    ymat[2][0] = FSIN(obj->angle); ymat[0][2] = -ymat[2][0];
    concat_matrix(ymat,obj->xform,turret->xform);

    /* Lower-raise gun */
    obj->spacing += ELEVATION_RATE*t_interval*obj->controls.aim_pointer_y;
    if (obj->spacing > MAX_ELEVATION) obj->spacing = MAX_ELEVATION;
    else if (obj->spacing < MAX_DEPRESSION) obj->spacing = MAX_DEPRESSION;
    xmat[1][1] = xmat[2][2] = FCOS(obj->spacing);
    xmat[2][1] = FSIN(obj->spacing); xmat[1][2] = -xmat[2][1];
    concat_matrix(xmat,turret->xform,gun->xform);

    return(ret);
}


#define LNORMAL	-SIN45,0.0,-COS45
#define RNORMAL SIN45,0.0,-COS45

#if !defined(HOVERWARE_MODEL)
static void high_res_hull_model(
    void)
{
    static float upper_hull[] = {
	-4.7,	UH_Y,	5.3,
	-3.5,	DECK_Y,	2.5,

	4.7,	UH_Y,	5.3,
	3.5,	DECK_Y,	2.5,

	4.7,	UH_Y,	-11.5,
	3.5,	DECK_Y,	-10.3,

	-4.7,	UH_Y,	-11.5,
	-3.5,	DECK_Y,	-10.3,

	-4.7,	UH_Y,	5.3,
	-3.5,	DECK_Y,	2.5,
    };
    static float front_hull[] = {
	-4.7,	UH_Y,	5.3,
	4.7,	UH_Y,	5.3,

	-4.7,	-1.6,	7.4,
	4.7,	-1.6,	7.4,

	-4.7,	-2.8,	6.2,
	4.7,	-2.8,	6.2,
    };
    static float rear_hull[] = {
	4.7,	UH_Y,	-11.5,
	-4.7,	UH_Y,	-11.5,

	4.7,	-1.2,	-12.3,
	-4.7,	-1.2,	-12.3,

	4.7,	-2.8,	-11.5,
	-4.7,	-2.8,	-11.5,
    };
    static float lside_hull[] = {
	-HULL_SIDE_X,	UH_Y,	5.3,
	-HULL_SIDE_X,	-1.6,	7.4,
	-HULL_SIDE_X,	-2.8,	6.2,
	-HULL_SIDE_X,	-2.8,	-11.5,
	-HULL_SIDE_X,	-1.2,	-12.3,
	-HULL_SIDE_X,	UH_Y,	-11.5,
    };
    static float rside_hull[] = {
	HULL_SIDE_X,	UH_Y,	-11.5,
	HULL_SIDE_X,	-1.2,	-12.3,
	HULL_SIDE_X,	-2.8,	-11.5,
	HULL_SIDE_X,	-2.8,	6.2,
	HULL_SIDE_X,	-1.6,	7.4,
	HULL_SIDE_X,	UH_Y,	5.3,
    };
    static float bottom[] = {
	-HULL_SIDE_X,	-2.8,	-11.5,
	-HULL_SIDE_X,	-2.8,	6.2,
	HULL_SIDE_X,	-2.8,	6.2,
	HULL_SIDE_X,	-2.8,	-11.5,
    };
	
    static float deck[] = {
	-3.5,	DECK_Y,	-10.3,
	 3.5,	DECK_Y,	-10.3,
	 3.5,	DECK_Y,	2.5,
	-3.5,	DECK_Y,	2.5,
    };
    static float lhub1[] = {
	-TIRE_X+TIRE_WIDTH*2,	TIRE_Y,	TIRE_Z1, LNORMAL,
	-TIRE_X,		TIRE_Y,	TIRE_Z1+TIRE_OR, LNORMAL,
    };
    static float lhub2[] = {
	-TIRE_X+TIRE_WIDTH*2,	TIRE_Y,	TIRE_Z2, LNORMAL,
	-TIRE_X,		TIRE_Y,	TIRE_Z2+TIRE_OR, LNORMAL,
    };
    static float lhub3[] = {
	-TIRE_X+TIRE_WIDTH*2,	TIRE_Y,	TIRE_Z3, LNORMAL,
	-TIRE_X,		TIRE_Y,	TIRE_Z3+TIRE_OR, LNORMAL,
    };
    static float lhub4[] = {
	-TIRE_X+TIRE_WIDTH*2,	TIRE_Y,	TIRE_Z4, LNORMAL,
	-TIRE_X,		TIRE_Y,	TIRE_Z4+TIRE_OR, LNORMAL,
    };
    static float lhub5[] = {
	-TIRE_X+TIRE_WIDTH*2,	TIRE_Y,	TIRE_Z5, LNORMAL,
	-TIRE_X,		TIRE_Y,	TIRE_Z5+TIRE_OR, LNORMAL,
    };
    static float rhub1[] = {
	TIRE_X-TIRE_WIDTH*2,	TIRE_Y,	TIRE_Z1, RNORMAL,
	TIRE_X,			TIRE_Y,	TIRE_Z1+TIRE_OR, RNORMAL,
    };
    static float rhub2[] = {
	TIRE_X-TIRE_WIDTH*2,	TIRE_Y,	TIRE_Z2, RNORMAL,
	TIRE_X,			TIRE_Y,	TIRE_Z2+TIRE_OR, RNORMAL,
    };
    static float rhub3[] = {
	TIRE_X-TIRE_WIDTH*2,	TIRE_Y,	TIRE_Z3, RNORMAL,
	TIRE_X,			TIRE_Y,	TIRE_Z3+TIRE_OR, RNORMAL,
    };
    static float rhub4[] = {
	TIRE_X-TIRE_WIDTH*2,	TIRE_Y,	TIRE_Z4, RNORMAL,
	TIRE_X,			TIRE_Y,	TIRE_Z4+TIRE_OR, RNORMAL,
    };
    static float rhub5[] = {
	TIRE_X-TIRE_WIDTH*2,	TIRE_Y,	TIRE_Z5, RNORMAL,
	TIRE_X,			TIRE_Y,	TIRE_Z5+TIRE_OR, RNORMAL,
    };
    static float lfsprocket[] = {
	-TIRE_X+TIRE_WIDTH*2,	FSPR_Y,	FSPR_Z, LNORMAL,
	-TIRE_X,		FSPR_Y,	FSPR_Z+FSPR_R, LNORMAL,
    };
    static float rfsprocket[] = {
	TIRE_X-TIRE_WIDTH*2,	FSPR_Y,	FSPR_Z, RNORMAL,
	TIRE_X,			FSPR_Y,	FSPR_Z+FSPR_R, RNORMAL,
    };
    static float lrsprocket[] = { 
	-TIRE_X+TIRE_WIDTH*2,	RSPR_Y,	RSPR_Z, LNORMAL,
	-TIRE_X,		RSPR_Y,	RSPR_Z+RSPR_R, LNORMAL,
    };
    static float rrsprocket[] = {
	TIRE_X-TIRE_WIDTH*2,	RSPR_Y,	RSPR_Z, RNORMAL,
	TIRE_X,			RSPR_Y,	RSPR_Z+RSPR_R, RNORMAL,
    };
    static float ltread[] = {
	-TIRE_X+TREAD_WIDTH,	FSPR_Y+FSPR_R+TREAD_THICKNESS,	FSPR_Z,
	-TIRE_X,		FSPR_Y+FSPR_R+TREAD_THICKNESS,	FSPR_Z,
	-TIRE_X,		FSPR_Y+FSPR_R,			FSPR_Z,
	-TIRE_X+TREAD_WIDTH,	FSPR_Y+FSPR_R,           	FSPR_Z,

	-TIRE_X+TREAD_WIDTH,	RSPR_Y+RSPR_R+TREAD_THICKNESS,	RSPR_Z,
	-TIRE_X,		RSPR_Y+RSPR_R+TREAD_THICKNESS,	RSPR_Z,
	-TIRE_X,		RSPR_Y+RSPR_R,			RSPR_Z,
	-TIRE_X+TREAD_WIDTH,	RSPR_Y+RSPR_R,           	RSPR_Z,

	-TIRE_X+TREAD_WIDTH,	RSPR_Y+(RSPR_R+TREAD_THICKNESS)*COS45, RSPR_Z-(RSPR_R+TREAD_THICKNESS)*COS45,
	-TIRE_X,		RSPR_Y+(RSPR_R+TREAD_THICKNESS)*COS45, RSPR_Z-(RSPR_R+TREAD_THICKNESS)*COS45,
	-TIRE_X,		RSPR_Y+RSPR_R*COS45, RSPR_Z-RSPR_R*COS45,
	-TIRE_X+TREAD_WIDTH,	RSPR_Y+RSPR_R*COS45, RSPR_Z-RSPR_R*COS45,

	-TIRE_X+TREAD_WIDTH,	RSPR_Y,	RSPR_Z-(RSPR_R+TREAD_THICKNESS),
	-TIRE_X,		RSPR_Y,	RSPR_Z-(RSPR_R+TREAD_THICKNESS),
	-TIRE_X,		RSPR_Y,	RSPR_Z-RSPR_R,
	-TIRE_X+TREAD_WIDTH,	RSPR_Y,	RSPR_Z-RSPR_R,

	-TIRE_X+TREAD_WIDTH,	RSPR_Y-(RSPR_R+TREAD_THICKNESS)*COS45, RSPR_Z-(RSPR_R+TREAD_THICKNESS)*COS45,
	-TIRE_X,		RSPR_Y-(RSPR_R+TREAD_THICKNESS)*COS45, RSPR_Z-(RSPR_R+TREAD_THICKNESS)*COS45,
	-TIRE_X,		RSPR_Y-RSPR_R*COS45, RSPR_Z-RSPR_R*COS45,
	-TIRE_X+TREAD_WIDTH,	RSPR_Y-RSPR_R*COS45, RSPR_Z-RSPR_R*COS45,

	-TIRE_X+TREAD_WIDTH,	TIRE_Y-(TIRE_OR+TREAD_THICKNESS),	TIRE_Z5,
	-TIRE_X,		TIRE_Y-(TIRE_OR+TREAD_THICKNESS),	TIRE_Z5,
	-TIRE_X,		TIRE_Y-TIRE_OR,				TIRE_Z5,
	-TIRE_X+TREAD_WIDTH,	TIRE_Y-TIRE_OR,				TIRE_Z5,

	-TIRE_X+TREAD_WIDTH,	TIRE_Y-(TIRE_OR+TREAD_THICKNESS), TIRE_Z1+0.2,
	-TIRE_X,		TIRE_Y-(TIRE_OR+TREAD_THICKNESS), TIRE_Z1+0.2,
	-TIRE_X,		TIRE_Y-TIRE_OR,		 	  TIRE_Z1+0.2,
	-TIRE_X+TREAD_WIDTH,	TIRE_Y-TIRE_OR,			  TIRE_Z1+0.2,

	-TIRE_X+TREAD_WIDTH,	FSPR_Y-(FSPR_R+TREAD_THICKNESS)*COS45, FSPR_Z+(FSPR_R+TREAD_THICKNESS)*COS45,
	-TIRE_X,		FSPR_Y-(FSPR_R+TREAD_THICKNESS)*COS45, FSPR_Z+(FSPR_R+TREAD_THICKNESS)*COS45,
	-TIRE_X,		FSPR_Y-FSPR_R*COS45, FSPR_Z+FSPR_R*COS45,
	-TIRE_X+TREAD_WIDTH,	FSPR_Y-FSPR_R*COS45, FSPR_Z+FSPR_R*COS45,

	-TIRE_X+TREAD_WIDTH,	FSPR_Y,	FSPR_Z+FSPR_R+TREAD_THICKNESS,
	-TIRE_X,		FSPR_Y,	FSPR_Z+FSPR_R+TREAD_THICKNESS,
	-TIRE_X,		FSPR_Y,	FSPR_Z+FSPR_R,
	-TIRE_X+TREAD_WIDTH,	FSPR_Y,	FSPR_Z+FSPR_R,

	-TIRE_X+TREAD_WIDTH,	FSPR_Y+(FSPR_R+TREAD_THICKNESS)*COS45, FSPR_Z+(FSPR_R+TREAD_THICKNESS)*COS45,
	-TIRE_X,		FSPR_Y+(FSPR_R+TREAD_THICKNESS)*COS45, FSPR_Z+(FSPR_R+TREAD_THICKNESS)*COS45,
	-TIRE_X,		FSPR_Y+FSPR_R*COS45, FSPR_Z+FSPR_R*COS45,
	-TIRE_X+TREAD_WIDTH,	FSPR_Y+FSPR_R*COS45, FSPR_Z+FSPR_R*COS45,

	-TIRE_X+TREAD_WIDTH,	FSPR_Y+FSPR_R+TREAD_THICKNESS,	FSPR_Z,
	-TIRE_X,		FSPR_Y+FSPR_R+TREAD_THICKNESS,	FSPR_Z,
	-TIRE_X,		FSPR_Y+FSPR_R,			FSPR_Z,
	-TIRE_X+TREAD_WIDTH,	FSPR_Y+FSPR_R,           	FSPR_Z,
    };
    static float rtread[] = {
	TIRE_X-TREAD_WIDTH,	FSPR_Y+FSPR_R,           	FSPR_Z,
	TIRE_X,			FSPR_Y+FSPR_R,			FSPR_Z,
	TIRE_X,			FSPR_Y+FSPR_R+TREAD_THICKNESS,	FSPR_Z,
	TIRE_X-TREAD_WIDTH,	FSPR_Y+FSPR_R+TREAD_THICKNESS,	FSPR_Z,

	TIRE_X-TREAD_WIDTH,	RSPR_Y+RSPR_R,           	RSPR_Z,
	TIRE_X,			RSPR_Y+RSPR_R,			RSPR_Z,
	TIRE_X,			RSPR_Y+RSPR_R+TREAD_THICKNESS,	RSPR_Z,
	TIRE_X-TREAD_WIDTH,	RSPR_Y+RSPR_R+TREAD_THICKNESS,	RSPR_Z,

	TIRE_X-TREAD_WIDTH,	RSPR_Y+RSPR_R*COS45,
		RSPR_Z-RSPR_R*COS45,
	TIRE_X,			RSPR_Y+RSPR_R*COS45,
		RSPR_Z-RSPR_R*COS45,
	TIRE_X,			RSPR_Y+(RSPR_R+TREAD_THICKNESS)*COS45,
		RSPR_Z-(RSPR_R+TREAD_THICKNESS)*COS45,
	TIRE_X-TREAD_WIDTH,	RSPR_Y+(RSPR_R+TREAD_THICKNESS)*COS45,
		RSPR_Z-(RSPR_R+TREAD_THICKNESS)*COS45,

	TIRE_X-TREAD_WIDTH,	RSPR_Y,	RSPR_Z-RSPR_R,
	TIRE_X,			RSPR_Y,	RSPR_Z-RSPR_R,
	TIRE_X,			RSPR_Y,	RSPR_Z-(RSPR_R+TREAD_THICKNESS),
	TIRE_X-TREAD_WIDTH,	RSPR_Y,	RSPR_Z-(RSPR_R+TREAD_THICKNESS),

	TIRE_X-TREAD_WIDTH,	RSPR_Y-RSPR_R*COS45,
		RSPR_Z-RSPR_R*COS45,
	TIRE_X,			RSPR_Y-RSPR_R*COS45,
		RSPR_Z-RSPR_R*COS45,
	TIRE_X,			RSPR_Y-(RSPR_R+TREAD_THICKNESS)*COS45,
		RSPR_Z-(RSPR_R+TREAD_THICKNESS)*COS45,
	TIRE_X-TREAD_WIDTH,	RSPR_Y-(RSPR_R+TREAD_THICKNESS)*COS45,
		RSPR_Z-(RSPR_R+TREAD_THICKNESS)*COS45,

	TIRE_X-TREAD_WIDTH,	TIRE_Y-TIRE_OR,				TIRE_Z5,
	TIRE_X,			TIRE_Y-TIRE_OR,				TIRE_Z5,
	TIRE_X,			TIRE_Y-(TIRE_OR+TREAD_THICKNESS),	TIRE_Z5,
	TIRE_X-TREAD_WIDTH,	TIRE_Y-(TIRE_OR+TREAD_THICKNESS),	TIRE_Z5,

	TIRE_X-TREAD_WIDTH,	TIRE_Y-TIRE_OR,			  TIRE_Z1+0.2,
	TIRE_X,			TIRE_Y-TIRE_OR,			  TIRE_Z1+0.2,
	TIRE_X,			TIRE_Y-(TIRE_OR+TREAD_THICKNESS), TIRE_Z1+0.2,
	TIRE_X-TREAD_WIDTH,	TIRE_Y-(TIRE_OR+TREAD_THICKNESS), TIRE_Z1+0.2,

	TIRE_X-TREAD_WIDTH,	FSPR_Y-FSPR_R*COS45,
		FSPR_Z+FSPR_R*COS45,
	TIRE_X,			FSPR_Y-FSPR_R*COS45,
		FSPR_Z+FSPR_R*COS45,
	TIRE_X,			FSPR_Y-(FSPR_R+TREAD_THICKNESS)*COS45,
		FSPR_Z+(FSPR_R+TREAD_THICKNESS)*COS45,
	TIRE_X-TREAD_WIDTH,	FSPR_Y-(FSPR_R+TREAD_THICKNESS)*COS45,
		FSPR_Z+(FSPR_R+TREAD_THICKNESS)*COS45,

	TIRE_X-TREAD_WIDTH,	FSPR_Y,	FSPR_Z+FSPR_R,
	TIRE_X,			FSPR_Y,	FSPR_Z+FSPR_R,
	TIRE_X,			FSPR_Y,	FSPR_Z+FSPR_R+TREAD_THICKNESS,
	TIRE_X-TREAD_WIDTH,	FSPR_Y,	FSPR_Z+FSPR_R+TREAD_THICKNESS,

	TIRE_X-TREAD_WIDTH,	FSPR_Y+FSPR_R*COS45,
		FSPR_Z+FSPR_R*COS45,
	TIRE_X,			FSPR_Y+FSPR_R*COS45,
		FSPR_Z+FSPR_R*COS45,
	TIRE_X,			FSPR_Y+(FSPR_R+TREAD_THICKNESS)*COS45,
		FSPR_Z+(FSPR_R+TREAD_THICKNESS)*COS45,
	TIRE_X-TREAD_WIDTH,	FSPR_Y+(FSPR_R+TREAD_THICKNESS)*COS45,
		FSPR_Z+(FSPR_R+TREAD_THICKNESS)*COS45,

	TIRE_X-TREAD_WIDTH,	FSPR_Y+FSPR_R,           	FSPR_Z,
	TIRE_X,			FSPR_Y+FSPR_R,			FSPR_Z,
	TIRE_X,			FSPR_Y+FSPR_R+TREAD_THICKNESS,	FSPR_Z,
	TIRE_X-TREAD_WIDTH,	FSPR_Y+FSPR_R+TREAD_THICKNESS,	FSPR_Z,
    };
    static float lfender[] = {
	-TIRE_X+FENDER_WIDTH,	FSPR_Y+(FSPR_R+FENDER_CLEARANCE)*COS45,
		FSPR_Z+(FSPR_R+FENDER_CLEARANCE)*COS45,
	-TIRE_X,		FSPR_Y+(FSPR_R+FENDER_CLEARANCE)*COS45,
		FSPR_Z+(FSPR_R+FENDER_CLEARANCE)*COS45,

	-TIRE_X+FENDER_WIDTH,	FENDER_Y,	FSPR_Z,
	-TIRE_X,		FENDER_Y,	FSPR_Z,

	-TIRE_X+FENDER_WIDTH,	FENDER_Y,	RSPR_Z,
	-TIRE_X,		FENDER_Y,	RSPR_Z,

	-TIRE_X+FENDER_WIDTH,	RSPR_Y+(RSPR_R+FENDER_CLEARANCE)*COS45,
		RSPR_Z-(RSPR_R+FENDER_CLEARANCE)*COS45,
	-TIRE_X,		RSPR_Y+(RSPR_R+FENDER_CLEARANCE)*COS45,
		RSPR_Z-(RSPR_R+FENDER_CLEARANCE)*COS45,

    };
    static float rfender[] = {
	TIRE_X,		FSPR_Y+(FSPR_R+FENDER_CLEARANCE)*COS45,
		FSPR_Z+(FSPR_R+FENDER_CLEARANCE)*COS45,
	TIRE_X-FENDER_WIDTH,	FSPR_Y+(FSPR_R+FENDER_CLEARANCE)*COS45,
		FSPR_Z+(FSPR_R+FENDER_CLEARANCE)*COS45,

	TIRE_X,			FENDER_Y,	FSPR_Z,
	TIRE_X-FENDER_WIDTH,	FENDER_Y,	FSPR_Z,

	TIRE_X,			FENDER_Y,	RSPR_Z,
	TIRE_X-FENDER_WIDTH,	FENDER_Y,	RSPR_Z,

	TIRE_X,		RSPR_Y+(RSPR_R+FENDER_CLEARANCE)*COS45,
		RSPR_Z-(RSPR_R+FENDER_CLEARANCE)*COS45,
	TIRE_X-FENDER_WIDTH,	RSPR_Y+(RSPR_R+FENDER_CLEARANCE)*COS45,
		RSPR_Z-(RSPR_R+FENDER_CLEARANCE)*COS45,

    };
    static float toolbox_side[] = {
	-TIRE_X,		FENDER_Y,	4.1,
	-TIRE_X,		FENDER_Y,	-0.4,
	-TIRE_X,		FENDER_Y+0.5,	-0.4,
	-TIRE_X,		FENDER_Y+0.5,	4.1,
    };
    static float toolbox[] = {
	-TIRE_X+FENDER_WIDTH/2,	FENDER_Y,	4.1,
	-TIRE_X,		FENDER_Y,	4.1,
	
	-TIRE_X+FENDER_WIDTH/2,	FENDER_Y+0.5,	4.1,
	-TIRE_X,		FENDER_Y+0.5,	4.1,
	
	-TIRE_X+FENDER_WIDTH/2,	FENDER_Y+0.5,	-0.4,
	-TIRE_X,		FENDER_Y+0.5,	-0.4,
	
	-TIRE_X+FENDER_WIDTH/2,	FENDER_Y,	-0.4,
	-TIRE_X,		FENDER_Y,	-0.4,
    };
    int invis[1];
	

    HULL_COLOR(fildes);
    DIFFUSE_LIGHTING_ON(fildes);
    /*** HULL ***/
    quadrilateral_mesh(fildes,upper_hull,5,2,NULL);
    quadrilateral_mesh(fildes,front_hull,3,2,NULL);
    quadrilateral_mesh(fildes,rear_hull,3,2,NULL);
    polygon3d(fildes,deck,NUMPTS(deck),FALSE);
    polygon3d(fildes,lside_hull,NUMPTS(lside_hull),FALSE);
    polygon3d(fildes,rside_hull,NUMPTS(rside_hull),FALSE);
    polygon3d(fildes,bottom,NUMPTS(bottom),FALSE);

    /* toolbox */
    polygon3d(fildes,toolbox_side,NUMPTS(toolbox_side),FALSE);
    quadrilateral_mesh(fildes,toolbox,4,2,NULL);

    /* MG bulge */
    mesh_cone(fildes,0.8,0.8,TRUE,FALSE,6,
	1.2, UH_Y+0.2, 5.3,  1.2, UH_Y+0.2, 2.1);

    /* Fuel tanks */
    mesh_cone(fildes,FUEL_RADIUS,FUEL_RADIUS,TRUE,TRUE,FUEL_FACETS,
	-4.0, 1.0, -7.0,  -4.0, 1.0, -10.3);
    mesh_cone(fildes,FUEL_RADIUS,FUEL_RADIUS,TRUE,TRUE,FUEL_FACETS,
	 4.0, 1.0, -3.7,   4.0, 1.0, -10.3);
    mesh_cone(fildes,FUEL_RADIUS,FUEL_RADIUS,TRUE,TRUE,FUEL_FACETS,
	-4.9, 0.5, -11.1, -2.5, 0.5, -11.1);
    mesh_cone(fildes,FUEL_RADIUS,FUEL_RADIUS,TRUE,TRUE,FUEL_FACETS,
	 2.5, 0.5, -11.1,  4.9, 0.5, -11.1);

    /* Fenders */
    quadrilateral_mesh(fildes,lfender,4,2,NULL);
    quadrilateral_mesh(fildes,rfender,4,2,NULL);

    /* Headlight */
    mesh_cone(fildes,0.4,0.0,FALSE,FALSE,6,
	-4.3, 0.8, 3.3,	 -4.3, 0.8, 2.7);
    /* will add lit/non-lit piece later. */

    /* BLACK(fildes); */
    /* Tailpipes */
    mesh_cone(fildes,0.2,0.2,FALSE,FALSE,4,
	 1.6, -0.9, -12.3,    1.6, DECK_Y-0.2, -10.3);
    mesh_cone(fildes,0.2,0.2,FALSE,FALSE,4,
	-1.6, -0.9, -12.3,   -1.6, DECK_Y-0.2, -10.3);


    RUSTY_STEEL(fildes);
    mesh_surface_of_revolution(fildes,lhub1,NUMPTSNORMAL(lhub1),
	WHEEL_FACETS,TRUE, TIRE_X,TIRE_Y,TIRE_Z1, TIRE_X+1.0,TIRE_Y,TIRE_Z1);
    mesh_surface_of_revolution(fildes,lhub2,NUMPTSNORMAL(lhub2),
	WHEEL_FACETS,TRUE, TIRE_X,TIRE_Y,TIRE_Z2, TIRE_X+1.0,TIRE_Y,TIRE_Z2);
    mesh_surface_of_revolution(fildes,lhub3,NUMPTSNORMAL(lhub3),
	WHEEL_FACETS,TRUE, TIRE_X,TIRE_Y,TIRE_Z3, TIRE_X+1.0,TIRE_Y,TIRE_Z3);
    mesh_surface_of_revolution(fildes,lhub4,NUMPTSNORMAL(lhub4),
	WHEEL_FACETS,TRUE, TIRE_X,TIRE_Y,TIRE_Z4, TIRE_X+1.0,TIRE_Y,TIRE_Z4);
    mesh_surface_of_revolution(fildes,lhub5,NUMPTSNORMAL(lhub5),
	WHEEL_FACETS,TRUE, TIRE_X,TIRE_Y,TIRE_Z5, TIRE_X+1.0,TIRE_Y,TIRE_Z5);

    mesh_surface_of_revolution(fildes,rhub1,NUMPTSNORMAL(rhub1),
	WHEEL_FACETS,TRUE, TIRE_X,TIRE_Y,TIRE_Z1, TIRE_X-1.0,TIRE_Y,TIRE_Z1);
    mesh_surface_of_revolution(fildes,rhub2,NUMPTSNORMAL(rhub2),
	WHEEL_FACETS,TRUE, TIRE_X,TIRE_Y,TIRE_Z2, TIRE_X-1.0,TIRE_Y,TIRE_Z2);
    mesh_surface_of_revolution(fildes,rhub3,NUMPTSNORMAL(rhub3),
	WHEEL_FACETS,TRUE, TIRE_X,TIRE_Y,TIRE_Z3, TIRE_X-1.0,TIRE_Y,TIRE_Z3);
    mesh_surface_of_revolution(fildes,rhub4,NUMPTSNORMAL(rhub4),
	WHEEL_FACETS,TRUE, TIRE_X,TIRE_Y,TIRE_Z4, TIRE_X-1.0,TIRE_Y,TIRE_Z4);
    mesh_surface_of_revolution(fildes,rhub5,NUMPTSNORMAL(rhub5),
	WHEEL_FACETS,TRUE, TIRE_X,TIRE_Y,TIRE_Z5, TIRE_X-1.0,TIRE_Y,TIRE_Z5);

    mesh_surface_of_revolution(fildes,lfsprocket, NUMPTSNORMAL(lfsprocket),
	WHEEL_FACETS,TRUE, TIRE_X,FSPR_Y,FSPR_Z, TIRE_X+1.0,FSPR_Y,FSPR_Z);
    mesh_surface_of_revolution(fildes,lrsprocket, NUMPTSNORMAL(lrsprocket),
	WHEEL_FACETS,TRUE, TIRE_X,RSPR_Y,RSPR_Z, TIRE_X+1.0,RSPR_Y,RSPR_Z);
    mesh_surface_of_revolution(fildes,rfsprocket, NUMPTSNORMAL(rfsprocket),
	WHEEL_FACETS,TRUE, TIRE_X,FSPR_Y,FSPR_Z, TIRE_X-1.0,FSPR_Y,FSPR_Z);
    mesh_surface_of_revolution(fildes,rrsprocket, NUMPTSNORMAL(rrsprocket),
	WHEEL_FACETS,TRUE, TIRE_X,RSPR_Y,RSPR_Z, TIRE_X-1.0,RSPR_Y,RSPR_Z);

    /* MG */
    mesh_cone(fildes,0.1,0.1,TRUE,FALSE,3,
	1.2, UH_Y+0.2, 6.3,  1.2, UH_Y+0.2, 5.3);

    STEEL(fildes);
    surface_model(fildes,TRUE,12,1.0,1.0,1.0);
    quadrilateral_mesh(fildes,rtread,11,4,NULL);
    quadrilateral_mesh(fildes,ltread,11,4,NULL);

    /*** HEADLIGHT ***/
    invis[0] = LIGHTS_ON_BRAKES_OFF|LIGHTS_ON_BRAKES_ON;
    add_names_to_set(img_fildes,1,invis);
	SELF_LIT_ON(img_fildes);
	PAINT(fildes,1.0,1.0,1.0);
	circle(fildes,0.4,6,FALSE,
	    -4.3, 0.8, 3.3, -4.3,0.8,2.3);
	SELF_LIT_OFF(img_fildes);
    remove_all_names_from_set(img_fildes);

    invis[0] = LIGHTS_OFF_BRAKES_OFF|LIGHTS_OFF_BRAKES_ON;
    add_names_to_set(img_fildes,1,invis);
	WHITE_PLASTIC(img_fildes);
	circle(fildes,0.4,6,FALSE,
	    -4.3, 0.8, 3.3, -4.3,0.8,2.3);
    remove_all_names_from_set(img_fildes);

    RESTORE_DEFAULT_VERTEX_FORMAT(fildes);
    DIFFUSE_LIGHTING_OFF(fildes);
}


static void mid_res_hull_model(
    void)
{
    static float upper_hull[] = {
	-4.7,	UH_Y,	5.3,
	-3.5,	DECK_Y,	2.5,

	4.7,	UH_Y,	5.3,
	3.5,	DECK_Y,	2.5,

	4.7,	UH_Y,	-11.5,
	3.5,	DECK_Y,	-10.3,

	-4.7,	UH_Y,	-11.5,
	-3.5,	DECK_Y,	-10.3,

	-4.7,	UH_Y,	5.3,
	-3.5,	DECK_Y,	2.5,
    };
    static float front_hull[] = {
	-4.7,	UH_Y,	5.3,
	4.7,	UH_Y,	5.3,

	-4.7,	-1.6,	7.4,
	4.7,	-1.6,	7.4,

	-4.7,	-2.8,	6.2,
	4.7,	-2.8,	6.2,
    };
    static float rear_hull[] = {
	4.7,	UH_Y,	-11.5,
	-4.7,	UH_Y,	-11.5,

	4.7,	-1.2,	-12.3,
	-4.7,	-1.2,	-12.3,

	4.7,	-2.8,	-11.5,
	-4.7,	-2.8,	-11.5,
    };
    static float lside_hull[] = {
	-HULL_SIDE_X,	UH_Y,	5.3,
	-HULL_SIDE_X,	-1.6,	7.4,
	-HULL_SIDE_X,	-2.8,	6.2,
	-HULL_SIDE_X,	-2.8,	-11.5,
	-HULL_SIDE_X,	-1.2,	-12.3,
	-HULL_SIDE_X,	UH_Y,	-11.5,
    };
    static float rside_hull[] = {
	HULL_SIDE_X,	UH_Y,	-11.5,
	HULL_SIDE_X,	-1.2,	-12.3,
	HULL_SIDE_X,	-2.8,	-11.5,
	HULL_SIDE_X,	-2.8,	6.2,
	HULL_SIDE_X,	-1.6,	7.4,
	HULL_SIDE_X,	UH_Y,	5.3,
    };
    static float bottom[] = {
	-HULL_SIDE_X,	-2.8,	-11.5,
	-HULL_SIDE_X,	-2.8,	6.2,
	HULL_SIDE_X,	-2.8,	6.2,
	HULL_SIDE_X,	-2.8,	-11.5,
    };
	
    static float deck[] = {
	-3.5,	DECK_Y,	-10.3,
	 3.5,	DECK_Y,	-10.3,
	 3.5,	DECK_Y,	2.5,
	-3.5,	DECK_Y,	2.5,
    };
    static float ltread[] = {
	-TIRE_X,		FSPR_Y+FSPR_R,			FSPR_Z,
	-TIRE_X+TREAD_WIDTH,	FSPR_Y+FSPR_R+TREAD_THICKNESS,	FSPR_Z,

	-TIRE_X,		RSPR_Y+RSPR_R,			RSPR_Z,
	-TIRE_X+TREAD_WIDTH,	RSPR_Y+RSPR_R+TREAD_THICKNESS,	RSPR_Z,

	-TIRE_X,		RSPR_Y,	RSPR_Z-RSPR_R,
	-TIRE_X+TREAD_WIDTH,	RSPR_Y,	RSPR_Z-(RSPR_R+TREAD_THICKNESS),

	-TIRE_X,		TIRE_Y-TIRE_OR,				TIRE_Z5,
	-TIRE_X+TREAD_WIDTH,	TIRE_Y-(TIRE_OR+TREAD_THICKNESS),	TIRE_Z5,

	-TIRE_X,		TIRE_Y-TIRE_OR,		 	  TIRE_Z1+0.2,
	-TIRE_X+TREAD_WIDTH,	TIRE_Y-(TIRE_OR+TREAD_THICKNESS), TIRE_Z1+0.2,

	-TIRE_X,		FSPR_Y,	FSPR_Z+FSPR_R,
	-TIRE_X+TREAD_WIDTH,	FSPR_Y,	FSPR_Z+FSPR_R+TREAD_THICKNESS,

	-TIRE_X,		FSPR_Y+FSPR_R,			FSPR_Z,
	-TIRE_X+TREAD_WIDTH,	FSPR_Y+FSPR_R+TREAD_THICKNESS,	FSPR_Z,
    };
    static float rtread[] = {
	TIRE_X,			FSPR_Y+FSPR_R,			FSPR_Z,
	TIRE_X-TREAD_WIDTH,	FSPR_Y+FSPR_R+TREAD_THICKNESS,	FSPR_Z,

	TIRE_X,			RSPR_Y+RSPR_R,			RSPR_Z,
	TIRE_X-TREAD_WIDTH,	RSPR_Y+RSPR_R+TREAD_THICKNESS,	RSPR_Z,

	TIRE_X,			RSPR_Y,	RSPR_Z-RSPR_R,
	TIRE_X-TREAD_WIDTH,	RSPR_Y,	RSPR_Z-(RSPR_R+TREAD_THICKNESS),

	TIRE_X,			TIRE_Y-TIRE_OR,				TIRE_Z5,
	TIRE_X-TREAD_WIDTH,	TIRE_Y-(TIRE_OR+TREAD_THICKNESS),	TIRE_Z5,

	TIRE_X,			TIRE_Y-TIRE_OR,			  TIRE_Z1+0.2,
	TIRE_X-TREAD_WIDTH,	TIRE_Y-(TIRE_OR+TREAD_THICKNESS), TIRE_Z1+0.2,

	TIRE_X,			FSPR_Y,	FSPR_Z+FSPR_R,
	TIRE_X-TREAD_WIDTH,	FSPR_Y,	FSPR_Z+FSPR_R+TREAD_THICKNESS,

	TIRE_X,			FSPR_Y+FSPR_R,			FSPR_Z,
	TIRE_X-TREAD_WIDTH,	FSPR_Y+FSPR_R+TREAD_THICKNESS,	FSPR_Z,
    };
	

    DIFFUSE_LIGHTING_ON(fildes);
    HULL_COLOR(fildes);
    /*** HULL ***/
    quadrilateral_mesh(fildes,upper_hull,5,2,NULL);
    quadrilateral_mesh(fildes,front_hull,3,2,NULL);
    quadrilateral_mesh(fildes,rear_hull,3,2,NULL);
    polygon3d(fildes,deck,NUMPTS(deck),FALSE);
    polygon3d(fildes,lside_hull,NUMPTS(lside_hull),FALSE);
    polygon3d(fildes,rside_hull,NUMPTS(rside_hull),FALSE);
    polygon3d(fildes,bottom,NUMPTS(bottom),FALSE);

    /* Fuel tanks */
    mesh_cone(fildes,FUEL_RADIUS,FUEL_RADIUS,TRUE,TRUE,4,
	-4.0, 1.0, -7.0,  -4.0, 1.0, -10.3);
    mesh_cone(fildes,FUEL_RADIUS,FUEL_RADIUS,TRUE,TRUE,4,
	 4.0, 1.0, -3.7,   4.0, 1.0, -10.3);
    mesh_cone(fildes,FUEL_RADIUS,FUEL_RADIUS,TRUE,TRUE,4,
	-4.9, 0.5, -11.1, -2.5, 0.5, -11.1);
    mesh_cone(fildes,FUEL_RADIUS,FUEL_RADIUS,TRUE,TRUE,4,
	 2.5, 0.5, -11.1,  4.9, 0.5, -11.1);


    RUSTY_STEEL(fildes);
    circle(fildes,TIRE_OR,WHEEL_FACETS,FALSE,
	-TIRE_X, TIRE_Y, TIRE_Z1,  -TIRE_X+1.0, TIRE_Y, TIRE_Z1);
    circle(fildes,TIRE_OR,WHEEL_FACETS,FALSE,
	-TIRE_X, TIRE_Y, TIRE_Z2,  -TIRE_X+1.0, TIRE_Y, TIRE_Z2);
    circle(fildes,TIRE_OR,WHEEL_FACETS,FALSE,
	-TIRE_X, TIRE_Y, TIRE_Z3,  -TIRE_X+1.0, TIRE_Y, TIRE_Z3);
    circle(fildes,TIRE_OR,WHEEL_FACETS,FALSE,
	-TIRE_X, TIRE_Y, TIRE_Z4,  -TIRE_X+1.0, TIRE_Y, TIRE_Z4);
    circle(fildes,TIRE_OR,WHEEL_FACETS,FALSE,
	-TIRE_X, TIRE_Y, TIRE_Z5,  -TIRE_X+1.0, TIRE_Y, TIRE_Z5);

    circle(fildes,TIRE_OR,WHEEL_FACETS,FALSE,
	TIRE_X, TIRE_Y, TIRE_Z1,  TIRE_X-1.0, TIRE_Y, TIRE_Z1);
    circle(fildes,TIRE_OR,WHEEL_FACETS,FALSE,
	TIRE_X, TIRE_Y, TIRE_Z2,  TIRE_X-1.0, TIRE_Y, TIRE_Z2);
    circle(fildes,TIRE_OR,WHEEL_FACETS,FALSE,
	TIRE_X, TIRE_Y, TIRE_Z3,  TIRE_X-1.0, TIRE_Y, TIRE_Z3);
    circle(fildes,TIRE_OR,WHEEL_FACETS,FALSE,
	TIRE_X, TIRE_Y, TIRE_Z4,  TIRE_X-1.0, TIRE_Y, TIRE_Z4);
    circle(fildes,TIRE_OR,WHEEL_FACETS,FALSE,
	TIRE_X, TIRE_Y, TIRE_Z5,  TIRE_X-1.0, TIRE_Y, TIRE_Z5);

    circle(fildes,FSPR_R,WHEEL_FACETS,FALSE,
	-TIRE_X, FSPR_Y, FSPR_Z, -TIRE_X+1.0, FSPR_Y, FSPR_Z);
    circle(fildes,FSPR_R,WHEEL_FACETS,FALSE,
	 TIRE_X, FSPR_Y, FSPR_Z,  TIRE_X-1.0, FSPR_Y, FSPR_Z);

    circle(fildes,RSPR_R,WHEEL_FACETS,FALSE,
	-TIRE_X, RSPR_Y, RSPR_Z, -TIRE_X+1.0, RSPR_Y, RSPR_Z);
    circle(fildes,RSPR_R,WHEEL_FACETS,FALSE,
	 TIRE_X, RSPR_Y, RSPR_Z,  TIRE_X-1.0, RSPR_Y, RSPR_Z);

    STEEL(fildes);
    surface_model(fildes,TRUE,12,1.0,1.0,1.0);
    /* Do them in both directions */
    vertex_format(fildes,0,0,0,0,CLOCKWISE);
    quadrilateral_mesh(fildes,rtread,7,2,NULL);
    quadrilateral_mesh(fildes,ltread,7,2,NULL);
    vertex_format(fildes,0,0,0,0,COUNTER_CLOCKWISE);
    quadrilateral_mesh(fildes,rtread,7,2,NULL);
    quadrilateral_mesh(fildes,ltread,7,2,NULL);
    RESTORE_DEFAULT_SURFACE_MODEL(fildes);
    /* No need to RESTORE_DEFAULT_VERTEX_FORMAT, since last was default. */
    DIFFUSE_LIGHTING_OFF(fildes);
}


static void low_res_hull_model(
    void)
{
    static float upper_hull[] = {
	-4.7,	UH_Y,	5.3,
	-3.5,	DECK_Y,	2.5,

	4.7,	UH_Y,	5.3,
	3.5,	DECK_Y,	2.5,

	4.7,	UH_Y,	-11.5,
	3.5,	DECK_Y,	-10.3,

	-4.7,	UH_Y,	-11.5,
	-3.5,	DECK_Y,	-10.3,

	-4.7,	UH_Y,	5.3,
	-3.5,	DECK_Y,	2.5,
    };
    static float front_hull[] = {
	4.7,	UH_Y,	5.3,
	4.7,	-2.8,	6.2,
	-4.7,	-2.8,	6.2,
	-4.7,	UH_Y,	5.3,
    };
    static float rear_hull[] = {
	-4.7,	UH_Y,	-11.5,
	-4.7,	-2.8,	-11.5,
	4.7,	-2.8,	-11.5,
	4.7,	UH_Y,	-11.5,
    };
    static float bottom[] = {
	-HULL_SIDE_X,	-2.8,	-11.5,
	-HULL_SIDE_X,	-2.8,	6.2,
	HULL_SIDE_X,	-2.8,	6.2,
	HULL_SIDE_X,	-2.8,	-11.5,
    };
	
    static float deck[] = {
	-3.5,	DECK_Y,	-10.3,
	 3.5,	DECK_Y,	-10.3,
	 3.5,	DECK_Y,	2.5,
	-3.5,	DECK_Y,	2.5,
    };
    static float ltread[] = {
	-TIRE_X+TREAD_WIDTH,	FSPR_Y+FSPR_R+TREAD_THICKNESS,	FSPR_Z,
	-TIRE_X,		FSPR_Y+FSPR_R,			FSPR_Z,

	-TIRE_X+TREAD_WIDTH,	RSPR_Y+RSPR_R+TREAD_THICKNESS,	RSPR_Z,
	-TIRE_X,		RSPR_Y+RSPR_R,			RSPR_Z,

	-TIRE_X+TREAD_WIDTH,	TIRE_Y-(TIRE_OR+TREAD_THICKNESS),	TIRE_Z5,
	-TIRE_X,		TIRE_Y-TIRE_OR,				TIRE_Z5,

	-TIRE_X+TREAD_WIDTH,	TIRE_Y-(TIRE_OR+TREAD_THICKNESS), TIRE_Z1+0.2,
	-TIRE_X,		TIRE_Y-TIRE_OR,		 	  TIRE_Z1+0.2,

	-TIRE_X+TREAD_WIDTH,	FSPR_Y+FSPR_R+TREAD_THICKNESS,	FSPR_Z,
	-TIRE_X,		FSPR_Y+FSPR_R,			FSPR_Z,
    };
    static float ltread_side[] = {
	-TIRE_X,		FSPR_Y+FSPR_R,			FSPR_Z,
	-TIRE_X,		TIRE_Y-TIRE_OR,		 	  TIRE_Z1+0.2,
	-TIRE_X,		TIRE_Y-TIRE_OR,				TIRE_Z5,
	-TIRE_X,		RSPR_Y+RSPR_R,			RSPR_Z,
	-TIRE_X,		FSPR_Y+FSPR_R,			FSPR_Z,
    };
    static float rtread[] = {
	TIRE_X,			FSPR_Y+FSPR_R,			FSPR_Z,
	TIRE_X-TREAD_WIDTH,	FSPR_Y+FSPR_R+TREAD_THICKNESS,	FSPR_Z,

	TIRE_X,			RSPR_Y+RSPR_R,			RSPR_Z,
	TIRE_X-TREAD_WIDTH,	RSPR_Y+RSPR_R+TREAD_THICKNESS,	RSPR_Z,

	TIRE_X,			TIRE_Y-TIRE_OR,				TIRE_Z5,
	TIRE_X-TREAD_WIDTH,	TIRE_Y-(TIRE_OR+TREAD_THICKNESS),	TIRE_Z5,

	TIRE_X,			TIRE_Y-TIRE_OR,			  TIRE_Z1+0.2,
	TIRE_X-TREAD_WIDTH,	TIRE_Y-(TIRE_OR+TREAD_THICKNESS), TIRE_Z1+0.2,

	TIRE_X,			FSPR_Y+FSPR_R,			FSPR_Z,
	TIRE_X-TREAD_WIDTH,	FSPR_Y+FSPR_R+TREAD_THICKNESS,	FSPR_Z,
    };
    static float rtread_side[] = {
	TIRE_X,			FSPR_Y+FSPR_R,			FSPR_Z,
	TIRE_X,			RSPR_Y+RSPR_R,			RSPR_Z,
	TIRE_X,			TIRE_Y-TIRE_OR,				TIRE_Z5,
	TIRE_X,			TIRE_Y-TIRE_OR,			  TIRE_Z1+0.2,
	TIRE_X,			FSPR_Y+FSPR_R,			FSPR_Z,
    };
	

    HULL_COLOR(fildes);
    /*** HULL ***/
    quadrilateral_mesh(fildes,upper_hull,5,2,NULL);
    polygon3d(fildes,front_hull,NUMPTS(front_hull),FALSE);
    polygon3d(fildes,rear_hull,NUMPTS(rear_hull),FALSE);
    polygon3d(fildes,deck,NUMPTS(deck),FALSE);
    polygon3d(fildes,bottom,NUMPTS(bottom),FALSE);

    RUSTY_STEEL(fildes);
    polygon3d(fildes,ltread_side,NUMPTS(ltread_side),FALSE);
    polygon3d(fildes,rtread_side,NUMPTS(rtread_side),FALSE);

    STEEL(fildes);
    surface_model(fildes,TRUE,12,1.0,1.0,1.0);
    quadrilateral_mesh(fildes,rtread,5,2,NULL);
    quadrilateral_mesh(fildes,ltread,5,2,NULL);
    RESTORE_DEFAULT_SURFACE_MODEL(fildes);
}
#endif /* !HOVERWARE_MODEL */


static int create_hull_shadow_dl(
    void)
{
#define YB (TIRE_Y-(TIRE_OR+TREAD_THICKNESS))
    static float hull[] = {
	TIRE_X,			YB,		RSPR_Z,
	TIRE_X,			DECK_Y,		RSPR_Z,
	
	-TIRE_X,		YB,		RSPR_Z,
	-TIRE_X,		DECK_Y,		RSPR_Z,
	
	-TIRE_X,		YB,		FSPR_Z,
	-TIRE_X,		DECK_Y,		FSPR_Z,
	
	TIRE_X,			YB,		FSPR_Z,
	TIRE_X,			DECK_Y,		FSPR_Z,
	
	TIRE_X,			YB,		RSPR_Z,
	TIRE_X,			DECK_Y,		RSPR_Z,
    };
    static float bottom[] = {
	-TIRE_X,	YB,	RSPR_Z,
	-TIRE_X,	YB,	FSPR_Z,
	TIRE_X,		YB,	FSPR_Z,
	TIRE_X,		YB,	RSPR_Z,
    };
    static float deck[] = {
	TIRE_X,		DECK_Y,	RSPR_Z,
	TIRE_X,		DECK_Y,	FSPR_Z,
	-TIRE_X,	DECK_Y,	FSPR_Z,
	-TIRE_X,	DECK_Y,	RSPR_Z,
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
	SHADOW(img_fildes);
	TRANSPARENT_ON(img_fildes,0.5);

	/*** HULL ***/
	quadrilateral_mesh(fildes,hull,5,2,NULL);
	polygon3d(fildes,deck,NUMPTS(deck),FALSE);
	polygon3d(fildes,bottom,NUMPTS(bottom),FALSE);

	TRANSPARENT_OFF(img_fildes);
    close_segment(img_fildes);

    open_segment(img_fildes,seg,TRUE,FALSE);
	set_extent(img_fildes,mc_extent);
	cond_execute_segment(img_fildes,CI_PRUNE,FALSE,graphics_seg);
    close_segment(img_fildes);

    return(seg);
}


static int create_T34_hull_graphics(
    void)
{
#if HOVERWARE_MODEL
    hwObject
	*objects;
    int
	numObjects;

    numObjects = hwParseFile( "objects/T34.hw", &objects );
    return createHwSegmentFromObj( objects, numObjects );
#else
    int T34Seg,HighResSeg,MidResSeg,LowResSeg;
    static float mc_extent[2][3] = {
	-TIRE_X, TIRE_Y-TIRE_OR-TREAD_THICKNESS, 	-12.3,
	 TIRE_X, DECK_Y,				7.4,
    };

    T34Seg     = get_dl_segment();
    HighResSeg = get_dl_segment();
    MidResSeg  = get_dl_segment();
    LowResSeg  = get_dl_segment();

    fildes = img_fildes;

    open_segment(img_fildes,HighResSeg,FALSE,FALSE);
      high_res_hull_model();
    close_segment(img_fildes);

    open_segment(img_fildes,MidResSeg,FALSE,FALSE);
      mid_res_hull_model();
    close_segment(img_fildes);

    open_segment(img_fildes,LowResSeg,FALSE,FALSE);
      low_res_hull_model();
    close_segment(img_fildes);

    open_segment(img_fildes,T34Seg,FALSE,FALSE);
      set_extent(img_fildes,mc_extent);
      cond_return(img_fildes,CI_PRUNE,TRUE);

      set_cull_size(img_fildes,30.0);
      cond_execute_segment(img_fildes,CI_CULL,TRUE,LowResSeg);
      cond_return(img_fildes,CI_CULL,TRUE);

      set_cull_size(img_fildes,100.0);
      cond_execute_segment(img_fildes,CI_CULL,TRUE,MidResSeg);
      cond_return(img_fildes,CI_CULL,TRUE);

      /* if we get to here, it means the thing is big...draw the full model */
      execute_segment(img_fildes,HighResSeg);
    close_segment(img_fildes);

    return(T34Seg);
#endif
}


#if !defined(HOVERWARE_MODEL)
static void high_res_turret_model(
    void)
{
    static float turret_top[] = {
	-1.6,	TTOP_Y,	1.4,
	-2.5,	TTOP_Y,	-1.0,
	-1.4,	TTOP_Y,	-4.9,
	1.4,	TTOP_Y,	-4.9,
	2.5,	TTOP_Y,	-1.0,
	1.6,	TTOP_Y,	1.4,
    };
    static float rupper_turret[] = {
	0.0,	TMID_Y,	3.8,
	0.0,	TTOP_Y,	1.4,
	1.8,	TMID_Y,	3.3,
	1.6,	TTOP_Y,	1.4,
	2.9,	TMID_Y,	0.0,
	2.5,	TTOP_Y,	-1.0,
	2.9,	TMID_Y,	-1.6,
	1.4,	TTOP_Y,	-4.9,
	2.0,	TMID_Y,	-5.1,
	-1.4,	TTOP_Y,	-4.9,
	-2.0,	TMID_Y,	-5.1,
    };
    static float lupper_turret[] = {
	0.0,	TMID_Y,	3.8,
	0.0,	TTOP_Y,	1.4,
	-1.8,	TMID_Y,	3.3,
	-1.6,	TTOP_Y,	1.4,
	-2.9,	TMID_Y,	0.0,
	-2.5,	TTOP_Y,	-1.0,
	-2.9,	TMID_Y,	-1.6,
	-1.4,	TTOP_Y,	-4.9,
	-2.0,	TMID_Y,	-5.1,
    };
    static float lower_turret[] = {
	0.0,	TMID_Y,		3.8,
	0.0,	TBASE_Y,	TBASE_R,
	1.8,	TMID_Y,		3.3,
	1.8,	TBASE_Y,	TBASE_R,
	2.9,	TMID_Y,		0.0,
	2.9,	TBASE_Y,	0.0,
	2.9,	TMID_Y,		-1.6,
	2.0,	TBASE_Y,	-5.1,
	2.0,	TMID_Y,		-5.1,
	-2.0,	TBASE_Y,	-5.1,
	-2.0,	TMID_Y,		-5.1,
	-2.9,	TBASE_Y,	-1.6,
	-2.9,	TMID_Y,		-1.6,
	-2.9,	TBASE_Y,	0.0,
	-2.9,	TMID_Y,		0.0,
	-1.8,	TBASE_Y,	TBASE_R,
	-1.8,	TMID_Y,		3.3,
	0.0,	TBASE_Y,	TBASE_R,
	0.0,	TMID_Y,		3.8,
    };
    static float turret_bottom[] = {
	0.0,	TBASE_Y,	TBASE_R,
	1.8,	TBASE_Y,	TBASE_R,
	2.9,	TBASE_Y,	0.0,
	2.0,	TBASE_Y,	-5.1,
	-2.0,	TBASE_Y,	-5.1,
	-2.9,	TBASE_Y,	-1.6,
	-2.9,	TBASE_Y,	0.0,
	-1.8,	TBASE_Y,	TBASE_R,
    };
    static float dome1[] = {
	DOME1_X+DOME_R,		TTOP_Y,		DOME_Z, SIN30, COS30, 0.0,
	DOME1_X+DOME_R*0.7,	TTOP_Y+0.2,	DOME_Z, SIN15, COS15, 0.0,
	DOME1_X,		TTOP_Y+0.3,	DOME_Z,	0.0, 1.0, 0.0,
    };
    static float dome2[] = {
	DOME2_X+DOME_R,		TTOP_Y,		DOME_Z,	SIN30, COS30, 0.0,
	DOME2_X+DOME_R*0.7,	TTOP_Y+0.2,	DOME_Z, SIN15, COS15, 0.0,
	DOME2_X,		TTOP_Y+0.3,	DOME_Z,	0.0, 1.0, 0.0,
    };
	

    HULL_COLOR(fildes);
    DIFFUSE_LIGHTING_ON(fildes);

    /*** TURRET ***/
    /* HULL_COLOR(fildes); */
    polygon3d(fildes,turret_top,NUMPTS(turret_top),FALSE);
    polygon3d(fildes,turret_bottom,NUMPTS(turret_bottom),FALSE);

    mesh_cone(fildes,TBASE_R,TBASE_R,FALSE,FALSE,8,
	0.0,  DECK_Y, 0.0,  0.0, TBASE_Y, 0.0);
    /* turret */
    triangular_strip(fildes,rupper_turret,NUMPTS(rupper_turret),NULL);
    vertex_format(fildes,0,0,0,0,CLOCKWISE);
    triangular_strip(fildes,lupper_turret,NUMPTS(lupper_turret),NULL);
    triangular_strip(fildes,lower_turret,NUMPTS(lower_turret),NULL);
    vertex_format(fildes,0,0,0,0,COUNTER_CLOCKWISE);

    /* hatch */
    mesh_cone(fildes,1.0,1.0,FALSE,TRUE,12,
	-1.3,	TTOP_Y,	-2.1,	-1.3,	TTOP_Y+0.6, -2.1);

    /* periscopes */
    mesh_cone(fildes,0.25,0.25,FALSE,TRUE,6,
	-1.8,	TTOP_Y,	0.0,	-1.8,	TTOP_Y+0.3, 0.0);
    mesh_cone(fildes,0.25,0.25,FALSE,TRUE,6,
	1.2,	TTOP_Y,	0.8,	1.2,	TTOP_Y+0.3, 0.8);

    /* ventilators */
    mesh_surface_of_revolution(fildes,dome1,NUMPTSNORMAL(dome1),8,TRUE,
	DOME1_X, TTOP_Y, DOME_Z,  DOME1_X, TTOP_Y+1.0, DOME_Z);
    mesh_surface_of_revolution(fildes,dome2,NUMPTSNORMAL(dome2),8,TRUE,
	DOME2_X, TTOP_Y, DOME_Z,  DOME2_X, TTOP_Y+1.0, DOME_Z);

    /* No need to RESTORE_DEFAULT_VERTEX_FORMAT, since last was default. */
    DIFFUSE_LIGHTING_OFF(fildes);
}


static void high_res_gun_model(
    void)
{

    HULL_COLOR(fildes);
    DIFFUSE_LIGHTING_ON(fildes);

    /* gun */
    mesh_cone(fildes,0.5,0.5,FALSE,TRUE,6,
	0.0,	TMID_Y,	3.0,	0.0,	TMID_Y,	5.0);
    mesh_cone(fildes,0.25,0.25,FALSE,FALSE,6,
	0.0,	TMID_Y,	4.9,	0.0,	TMID_Y,	15.0);

    BLACK(fildes);
    circle(fildes,0.25,12,FALSE,
	0.0,	TMID_Y,	15.0,	0.0,	TMID_Y, 14.0);

    /* No need to RESTORE_DEFAULT_VERTEX_FORMAT, since last was default. */
    DIFFUSE_LIGHTING_OFF(fildes);
}


static void mid_res_turret_model(
    void)
{
    static float turret_top[] = {
	-1.6,	TTOP_Y,	1.4,
	-2.5,	TTOP_Y,	-1.0,
	-1.4,	TTOP_Y,	-4.9,
	1.4,	TTOP_Y,	-4.9,
	2.5,	TTOP_Y,	-1.0,
	1.6,	TTOP_Y,	1.4,
    };
    static float upper_turret[] = {
	0.0,	TMID_Y,	3.8,
	0.0,	TTOP_Y,	1.4,

	1.8,	TMID_Y,	3.3,
	1.6,	TTOP_Y,	1.4,

	2.9,	TMID_Y,	0.0,
	2.5,	TTOP_Y,	-1.0,

	2.0,	TMID_Y,	-5.1,
	1.4,	TTOP_Y,	-4.9,

	-2.0,	TMID_Y,	-5.1,
	-1.4,	TTOP_Y,	-4.9,

	-2.9,	TMID_Y,	-1.6,
	-2.5,	TTOP_Y,	-1.0,

	-1.8,	TMID_Y,	3.3,
	-1.6,	TTOP_Y,	1.4,

	0.0,	TMID_Y,	3.8,
	0.0,	TTOP_Y,	1.4,
    };
    static float lower_turret[] = {
	0.0,	DECK_Y,		TBASE_R,
	0.0,	TMID_Y,		3.8,

	1.8,	DECK_Y,		TBASE_R,
	1.8,	TMID_Y,		3.3,

	2.9,	DECK_Y,		0.0,
	2.9,	TMID_Y,		0.0,

	2.0,	DECK_Y,		-5.1,
	2.0,	TMID_Y,		-5.1,

	-2.0,	DECK_Y,		-5.1,
	-2.0,	TMID_Y,		-5.1,

	-2.9,	DECK_Y,		-1.6,
	-2.9,	TMID_Y,		-1.6,

	-1.8,	DECK_Y,		TBASE_R,
	-1.8,	TMID_Y,		3.3,

	0.0,	DECK_Y,		TBASE_R,
	0.0,	TMID_Y,		3.8,
    };
	

    DIFFUSE_LIGHTING_ON(fildes);
    HULL_COLOR(fildes);

    /*** TURRET ***/
    polygon3d(fildes,turret_top,NUMPTS(turret_top),FALSE);
    quadrilateral_mesh(fildes,upper_turret,8,2,NULL);
    quadrilateral_mesh(fildes,lower_turret,8,2,NULL);

    /* No need to RESTORE_DEFAULT_VERTEX_FORMAT, since last was default. */
    DIFFUSE_LIGHTING_OFF(fildes);
}


static void low_res_gun_model(
    void)
{

    DIFFUSE_LIGHTING_ON(fildes);
    HULL_COLOR(fildes);

    /* gun */
    mesh_cone(fildes,0.25,0.25,FALSE,FALSE,3,
	0.0,	TMID_Y,	3.0,	0.0,	TMID_Y,	15.0);


    /* No need to RESTORE_DEFAULT_VERTEX_FORMAT, since last was default. */
    DIFFUSE_LIGHTING_OFF(fildes);
}


static void low_res_turret_model(
    void)
{
    static float turret_top[] = {
	-1.8,	TTOP_Y,	3.3,
	-2.7,	TTOP_Y,	-4.9,
	 2.7,	TTOP_Y,	-4.9,
	 1.8,	TTOP_Y,	3.3,
    };
    static float turret[] = {
	1.8,	DECK_Y,		3.3,
	1.8,	TTOP_Y,		3.3,

	2.7,	DECK_Y,		-5.1,
	2.7,	TTOP_Y,		-4.9,

	-2.7,	DECK_Y,		-5.1,
	-2.7,	TTOP_Y,		-4.9,

	-1.8,	DECK_Y,		3.3,
	-1.8,	TTOP_Y,		3.3,

	1.8,	DECK_Y,		3.3,
	1.8,	TTOP_Y,		3.3
    };

    HULL_COLOR(fildes);

    /*** TURRET ***/
    polygon3d(fildes,turret_top,NUMPTS(turret_top),FALSE);
    quadrilateral_mesh(fildes,turret,5,2,NULL);
}
#endif /* !HOVERWARE_MODEL */


static int create_turret_shadow_dl(
    void)
{
    static float turret_top[] = {
	-1.8,	TTOP_Y,	3.3,
	-2.7,	TTOP_Y,	-4.9,
	 2.7,	TTOP_Y,	-4.9,
	 1.8,	TTOP_Y,	3.3,
    };
    static float turret[] = {
	1.8,	DECK_Y,		3.3,
	1.8,	TTOP_Y,		3.3,

	2.7,	DECK_Y,		-5.1,
	2.7,	TTOP_Y,		-4.9,

	-2.7,	DECK_Y,		-5.1,
	-2.7,	TTOP_Y,		-4.9,

	-1.8,	DECK_Y,		3.3,
	-1.8,	TTOP_Y,		3.3,

	1.8,	DECK_Y,		3.3,
	1.8,	TTOP_Y,		3.3
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
	SHADOW(img_fildes);
	TRANSPARENT_ON(img_fildes,0.5);

	/*** TURRET ***/
	polygon3d(fildes,turret_top,NUMPTS(turret_top),FALSE);
	quadrilateral_mesh(fildes,turret,5,2,NULL);

	TRANSPARENT_OFF(img_fildes);
    close_segment(img_fildes);

    open_segment(img_fildes,seg,TRUE,FALSE);
	set_extent(img_fildes,mc_extent);
	cond_execute_segment(img_fildes,CI_PRUNE,FALSE,graphics_seg);
    close_segment(img_fildes);

    return(seg);
}


static int create_gun_shadow_dl(
    void)
{
    int seg;

    seg = get_dl_segment();
    open_segment(img_fildes,seg,TRUE,FALSE);
	SHADOW(img_fildes);
	TRANSPARENT_ON(img_fildes,0.5);
	/* gun */
	mesh_cone(fildes,0.25,0.25,FALSE,FALSE,3,
	    0.0,	TMID_Y,	0.0,	0.0,	TMID_Y,	15.0);
	TRANSPARENT_OFF(img_fildes);
    close_segment(img_fildes);
    return(seg);
}


static int create_T34_turret_graphics(
    void)
{
#if HOVERWARE_MODEL
    hwObject
	*objects;
    int
	numObjects;

    numObjects = hwParseFile( "objects/T34_turret.hw", &objects );
    return createHwSegmentFromObj( objects, numObjects );
#else
    int T34_turretSeg,HighResSeg,MidResSeg,LowResSeg;
    static float mc_extent[2][3] = {
	-TURRET_XEXTENT, DECK_Y, 	TURRET_ZMIN,
	 TURRET_XEXTENT, TTOP_Y,	TURRET_ZMAX,
    };

    T34_turretSeg     = get_dl_segment();
    HighResSeg = get_dl_segment();
    MidResSeg  = get_dl_segment();
    LowResSeg  = get_dl_segment();

    fildes = img_fildes;

    open_segment(img_fildes,HighResSeg,FALSE,FALSE);
      high_res_turret_model();
    close_segment(img_fildes);

    open_segment(img_fildes,MidResSeg,FALSE,FALSE);
      mid_res_turret_model();
    close_segment(img_fildes);

    open_segment(img_fildes,LowResSeg,FALSE,FALSE);
      low_res_turret_model();
    close_segment(img_fildes);

    open_segment(img_fildes,T34_turretSeg,FALSE,FALSE);
      set_extent(img_fildes,mc_extent);
      cond_return(img_fildes,CI_PRUNE,TRUE);

      set_cull_size(img_fildes,30.0);
      cond_execute_segment(img_fildes,CI_CULL,TRUE,LowResSeg);
      cond_return(img_fildes,CI_CULL,TRUE);

      set_cull_size(img_fildes,100.0);
      cond_execute_segment(img_fildes,CI_CULL,TRUE,MidResSeg);
      cond_return(img_fildes,CI_CULL,TRUE);

      /* if we get to here, it means the thing is big...draw the full model */
      execute_segment(img_fildes,HighResSeg);
    close_segment(img_fildes);

    return(T34_turretSeg);
#endif
}


static int create_T34_gun_graphics(
    void)
{
#if HOVERWARE_MODEL
    hwObject
	*objects;
    int
	numObjects;

    numObjects = hwParseFile( "objects/T34_gun.hw", &objects );
    return createHwSegmentFromObj( objects, numObjects );
#else
    int T34_gunSeg,HighResSeg,LowResSeg;
    static float mc_extent[2][3] = {
	-0.5, 0.5, 3.0,
	 0.5, 0.5, 15.0,
    };


    T34_gunSeg = get_dl_segment();
    HighResSeg = get_dl_segment();
    LowResSeg  = get_dl_segment();

    fildes = img_fildes;

    open_segment(img_fildes,HighResSeg,FALSE,FALSE);
      high_res_gun_model();
    close_segment(img_fildes);

    open_segment(img_fildes,LowResSeg,FALSE,FALSE);
      low_res_gun_model();
    close_segment(img_fildes);

    open_segment(img_fildes,T34_gunSeg,FALSE,FALSE);
      set_extent(img_fildes,mc_extent);
      set_cull_size(img_fildes,30.0);
      cond_execute_segment(img_fildes,CI_CULL,TRUE,LowResSeg);
      cond_return(img_fildes,CI_CULL,TRUE);

      /* if we get to here, it means the thing is big...draw the full model */
      execute_segment(img_fildes,HighResSeg);
    close_segment(img_fildes);

    return(T34_gunSeg);
#endif
}


static int create_global_dl(
    int hl_dl, int tr_dl)
{
    float mats[3][4][4];
    int segs[3];

    hwIdentity( mats[0] );
    hwIdentity( mats[1] );
    hwIdentity( mats[2] );
    segs[0] = tr_dl;
    segs[1] = hl_dl;
    segs[2] = gun_dl;

    return createHwSegmentList( 3, segs, mats );
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

    /***** INIT AUXILIARY DATA *****/
    vaux->bbox_mc[PT_UFL][0] = -TIRE_X;
	    vaux->bbox_mc[PT_UFL][1] = TIRE_Y-TIRE_OR-TREAD_THICKNESS;
	    vaux->bbox_mc[PT_UFL][2] =  7.4;
    vaux->bbox_mc[PT_UFR][0] =  TIRE_X;
	    vaux->bbox_mc[PT_UFR][1] = TIRE_Y-TIRE_OR-TREAD_THICKNESS;
	    vaux->bbox_mc[PT_UFR][2] =  7.4;
    vaux->bbox_mc[PT_UBR][0] =  TIRE_X;
	    vaux->bbox_mc[PT_UBR][1] = TIRE_Y-TIRE_OR-TREAD_THICKNESS;
	    vaux->bbox_mc[PT_UBR][2] = -12.3;
    vaux->bbox_mc[PT_UBL][0] = -TIRE_X;
	    vaux->bbox_mc[PT_UBL][1] = TIRE_Y-TIRE_OR-TREAD_THICKNESS;
	    vaux->bbox_mc[PT_UBL][2] = -12.3;
    vaux->bbox_mc[PT_TFL][0] = -TIRE_X;
	    vaux->bbox_mc[PT_TFL][1] = TTOP_Y;
	    vaux->bbox_mc[PT_TFL][2] =  7.4;
    vaux->bbox_mc[PT_TFR][0] =  TIRE_X;
	    vaux->bbox_mc[PT_TFR][1] = TTOP_Y;
	    vaux->bbox_mc[PT_TFR][2] =  7.4;
    vaux->bbox_mc[PT_TBR][0] =  TIRE_X;
	    vaux->bbox_mc[PT_TBR][1] = TTOP_Y;
	    vaux->bbox_mc[PT_TBR][2] = -12.3;
    vaux->bbox_mc[PT_TBL][0] = -TIRE_X;
	    vaux->bbox_mc[PT_TBL][1] = TTOP_Y;
	    vaux->bbox_mc[PT_TBL][2] = -12.3;
    vaux->COG_mc[0] =
	    (vaux->bbox_mc[PT_UFR][0] + vaux->bbox_mc[PT_UFL][0]) / 2.0;
    vaux->COG_mc[1] =
	    (vaux->bbox_mc[PT_TFL][1] + vaux->bbox_mc[PT_UFL][1]) / 2.0;
    vaux->COG_mc[2] =
	    (vaux->bbox_mc[PT_UBR][2] + vaux->bbox_mc[PT_UFR][2]) / 2.0;


    vaux->coefficient_of_drag = 1.0;
    vaux->wheel_torque_mult   = 70.0  /* *pobj->I[YD][YD] */;
    vaux->best_turn_speed     = 0.0;
    vaux->max_obstacle_height = 4.0;

    vaux->horsepower          = 500.0;
    vaux->peak_power_rpm      = 4000.0;
    vaux->two_peak_power      = 0.5;
    vaux->gear_best_speed[0]  = MPH_TO_FPS(5.0);	/* reverse */
    vaux->gear_best_speed[1]  = 0.0;			/* neutral */
    vaux->gear_best_speed[2]  = MPH_TO_FPS(5.0);	/* 1st */
    vaux->gear_best_speed[3]  = MPH_TO_FPS(10.0);	/* 2nd */
    vaux->gear_best_speed[4]  = MPH_TO_FPS(15.0);	/* 3rd */
    vaux->gear_best_speed[5]  = MPH_TO_FPS(20.0);	/* 4th */
    vaux->gear_best_speed[6]  = MPH_TO_FPS(25.0);	/* 5th */
    vaux->offroad_performance = 1.0;
    vaux->thrust_mechanism    = TRACK_DRIVE;

    vaux->left_gauge_class  = GAUGE_ANALOG_MPH;
    vaux->right_gauge_class = GAUGE_ANALOG_RPM;
    vaux->max_speed         = 50.0;
    vaux->max_rpm           = 8000.0;
    vaux->max_altitude      = 25000.0;

    return(vaux);
}


void init_T34_object(
    DRIVE_OBJECT *obj)
{
    DRIVE_OBJECT *child,*grandchild;

    if (obj->color[0] == DEFAULT_OBJECT_COLOR) obj->color[0] = DEFAULT_RED;
    if (obj->color[1] == DEFAULT_OBJECT_COLOR) obj->color[1] = DEFAULT_GRN;
    if (obj->color[2] == DEFAULT_OBJECT_COLOR) obj->color[2] = DEFAULT_BLU;

    if (hull_dl == INVALID) {
    	hull_dl = create_T34_hull_graphics();
	hull_shadow_dl = create_hull_shadow_dl();
    	turret_dl = create_T34_turret_graphics();
	turret_shadow_dl = create_turret_shadow_dl();
    	gun_dl = create_T34_gun_graphics();
	gun_shadow_dl = create_gun_shadow_dl();
	global_dl = create_global_dl(hull_dl,turret_dl);
    }

    obj->display_list = INVALID;
    obj->global_display_list = global_dl;

    /***** INIT AUXILIARY DATA *****/
    if ((obj->vehicle_auxdata = init_vaux(obj)) == NULL) {
	return;
    }
    init_pobj(obj,70500.0,70.0);

    if ((obj->upd = (UpdateDisp_ptr) malloc(sizeof(UpdateDisp))) == NULL) {
	return;
    }
    obj->upd->color[0] = obj->color[0];
    obj->upd->color[1] = obj->color[1];
    obj->upd->color[2] = obj->color[2];
    obj->upd->invis_words = 1;
    obj->upd->invis[0] = LIGHTS_OFF_BRAKES_ON;

    obj->update_self = apply_tank_physics;
    obj->bound_mc[0] = -TIRE_X;
    obj->bound_mc[1] =  TIRE_Y-TIRE_OR-TREAD_THICKNESS;
    obj->bound_mc[2] = -12.3;
    obj->bound_mc[3] =  TIRE_X;
    obj->bound_mc[4] =  TTOP_Y;
    obj->bound_mc[5] =  7.4;
    update_wc_bounds(obj);

    /* apply brake */
    obj->controls.pointer_x = 0.0;
    obj->controls.pointer_y = -1.0;

    obj->surface_chars_xyz  = T34_surface_chars_xyz;
    obj->surface_chars_bbox = T34_surface_chars_bbox;

    obj->num_children = 3;

    /***** Create hull child *****/
    if ((child = (DRIVE_OBJECT *) malloc(sizeof(DRIVE_OBJECT))) == NULL) {
	fprintf(stderr,"Out of malloc space!\n");
	return;
    }
    /* First clone myself */
    memcpy(child,obj,sizeof(DRIVE_OBJECT));
    child->display_list = hull_dl;
    child->global_display_list = INVALID;
    /* Use same intersection routines. */
    /* I have no physics or controls */
    child->update_controls = NULL;
    child->update_self = NULL;
    child->pobj = NULL;
    child->vehicle_auxdata = NULL;
    child->aeroplane = NULL;
    child->update_self = NULL;
    add_object_to_list(&(obj->child_list),child);


    /***** Create hull shadow child *****/
    child->num_children = 1;
    if ((grandchild = (DRIVE_OBJECT *) malloc(sizeof(DRIVE_OBJECT))) == NULL) {
	fprintf(stderr,"Out of malloc space!\n");
	return;
    }
    /* First clone myself */
    memcpy(grandchild,child,sizeof(DRIVE_OBJECT));
    grandchild->idptr = &(object_id[SHADOW_ID_NUMBER]);
    grandchild->num_children = 0;
    grandchild->child_list = NULL;
    grandchild->display_list = hull_shadow_dl;
    grandchild->global_display_list = INVALID;
    /* Use same intersection routines. */
    /* I have no physics or controls */
    grandchild->update_controls = NULL;
    grandchild->pobj = NULL;
    grandchild->vehicle_auxdata = NULL;
    grandchild->aeroplane = NULL;
    grandchild->update_self = NULL;
    grandchild->upd = NULL;
    add_object_to_list(&(child->child_list),grandchild);


    /***** Create turret child *****/
    if ((child = (DRIVE_OBJECT *) malloc(sizeof(DRIVE_OBJECT))) == NULL) {
	fprintf(stderr,"Out of malloc space!\n");
	return;
    }
    /* First clone myself */
    memcpy(child,obj,sizeof(DRIVE_OBJECT));
    child->display_list = turret_dl;
    child->global_display_list = INVALID;
    /* Use same intersection routines. */
    /* I have no physics or controls */
    child->update_controls = NULL;
    child->update_self = NULL;
    child->pobj = NULL;
    child->vehicle_auxdata = NULL;
    child->aeroplane = NULL;
    child->update_self = NULL;
    add_object_to_list(&(obj->child_list),child);

    /***** Create turret shadow child *****/
    if ((grandchild = (DRIVE_OBJECT *) malloc(sizeof(DRIVE_OBJECT))) == NULL) {
	fprintf(stderr,"Out of malloc space!\n");
	return;
    }
    /* First clone myself */
    memcpy(grandchild,child,sizeof(DRIVE_OBJECT));
    grandchild->idptr = &(object_id[SHADOW_ID_NUMBER]);
    grandchild->num_children = 0;
    grandchild->child_list = NULL;
    grandchild->display_list = turret_shadow_dl;
    grandchild->global_display_list = INVALID;
    /* Use same intersection routines. */
    /* I have no physics or controls */
    grandchild->update_controls = NULL;
    grandchild->pobj = NULL;
    grandchild->vehicle_auxdata = NULL;
    grandchild->aeroplane = NULL;
    grandchild->update_self = NULL;
    grandchild->upd = NULL;
    add_object_to_list(&(child->child_list),grandchild);

    /***** Create gun child *****/
    if ((child = (DRIVE_OBJECT *) malloc(sizeof(DRIVE_OBJECT))) == NULL) {
	fprintf(stderr,"Out of malloc space!\n");
	return;
    }
    /* First clone myself */
    memcpy(child,obj,sizeof(DRIVE_OBJECT));
    child->display_list = gun_dl;
    child->global_display_list = INVALID;
    /* Use same intersection routines. */
    /* I have no physics or controls */
    child->update_controls = NULL;
    child->update_self = NULL;
    child->pobj = NULL;
    child->vehicle_auxdata = NULL;
    child->aeroplane = NULL;
    child->update_self = NULL;
    add_object_to_list(&(obj->child_list),child);

    /***** Create gun shadow child *****/
    if ((grandchild = (DRIVE_OBJECT *) malloc(sizeof(DRIVE_OBJECT))) == NULL) {
	fprintf(stderr,"Out of malloc space!\n");
	return;
    }
    /* First clone myself */
    memcpy(grandchild,child,sizeof(DRIVE_OBJECT));
    grandchild->idptr = &(object_id[SHADOW_ID_NUMBER]);
    grandchild->connection = 0;
    grandchild->num_children = 0;
    grandchild->child_list = NULL;
    grandchild->display_list = gun_shadow_dl;
    grandchild->global_display_list = INVALID;
    /* Use same intersection routines. */
    /* I have no physics or controls */
    grandchild->update_controls = NULL;
    grandchild->pobj = NULL;
    grandchild->vehicle_auxdata = NULL;
    grandchild->aeroplane = NULL;
    grandchild->update_self = NULL;
    grandchild->upd = NULL;
    add_object_to_list(&(child->child_list),grandchild);

    obj->aim_xform = &(child->xform);
    obj->fire_point[0] = 0.0;
    obj->fire_point[1] = TMID_Y;
    obj->fire_point[2] = 15.0+1.0;
}
