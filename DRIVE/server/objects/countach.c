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


/* Code Module for the countach car object */

#include <stdio.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "prims.h"
#include "obj_common.h"
#include "demo_physics.h"

#define HIGH_WHEEL_FACETS 12

#define DEFAULT_RED	(FLOATRAND(1.0))
#define DEFAULT_GRN	(FLOATRAND(1.0))
#define DEFAULT_BLU	(FLOATRAND(1.0))

#if !defined(HOVERWARE_MODEL)
static float identity[4][4] = IDENTITY4x4;
static int invis[10];
#endif 

static int countach_dl = -1, shadow_dl = -1;

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
    vaux->bbox_mc[PT_UFL][0] = -2.5;
	    vaux->bbox_mc[PT_UFL][1] = -0.9;
	    vaux->bbox_mc[PT_UFL][2] =  4.0;
    vaux->bbox_mc[PT_UFR][0] =  2.5;
	    vaux->bbox_mc[PT_UFR][1] = -0.9;
	    vaux->bbox_mc[PT_UFR][2] =  4.0;
    vaux->bbox_mc[PT_UBR][0] =  2.5;
	    vaux->bbox_mc[PT_UBR][1] = -0.9;
	    vaux->bbox_mc[PT_UBR][2] = -4.0;
    vaux->bbox_mc[PT_UBL][0] = -2.5;
	    vaux->bbox_mc[PT_UBL][1] = -0.9;
	    vaux->bbox_mc[PT_UBL][2] = -4.0;
    vaux->bbox_mc[PT_TFL][0] = -2.5;
	    vaux->bbox_mc[PT_TFL][1] = 2.0;
	    vaux->bbox_mc[PT_TFL][2] =  4.0;
    vaux->bbox_mc[PT_TFR][0] =  2.5;
	    vaux->bbox_mc[PT_TFR][1] = 2.0;
	    vaux->bbox_mc[PT_TFR][2] =  4.0;
    vaux->bbox_mc[PT_TBR][0] =  2.5;
	    vaux->bbox_mc[PT_TBR][1] = 2.0;
	    vaux->bbox_mc[PT_TBR][2] = -4.0;
    vaux->bbox_mc[PT_TBL][0] = -2.5;
	    vaux->bbox_mc[PT_TBL][1] = 2.0;
	    vaux->bbox_mc[PT_TBL][2] = -4.0;
    vaux->COG_mc[0] =
	    (vaux->bbox_mc[PT_UFR][0] + vaux->bbox_mc[PT_UFL][0]) / 2.0;
    vaux->COG_mc[1] =
	    (vaux->bbox_mc[PT_TFL][1] + vaux->bbox_mc[PT_UFL][1]) / 2.0;
    vaux->COG_mc[2] =
	    (vaux->bbox_mc[PT_UBR][2] + vaux->bbox_mc[PT_UFR][2]) / 2.0;

    vaux->coefficient_of_drag = 0.030;
    vaux->wheel_torque_mult   = 75.0 /* *pobj->I[2][2] */;
    vaux->best_turn_speed     = MPH_TO_FPS(25.0);
    vaux->max_obstacle_height = 1.0;

    vaux->horsepower          = 200.0;
    vaux->peak_power_rpm      = 3500.0;
    vaux->two_peak_power      = 0.0;
    vaux->gear_best_speed[0]  = MPH_TO_FPS(15.0);	/* reverse */
    vaux->gear_best_speed[1]  = 0.0;			/* neutral */
    vaux->gear_best_speed[2]  = MPH_TO_FPS(20.0);	/* 1st */
    vaux->gear_best_speed[3]  = MPH_TO_FPS(45.0);	/* 2nd */
    vaux->gear_best_speed[4]  = MPH_TO_FPS(70.0);	/* 3rd */
    vaux->gear_best_speed[5]  = MPH_TO_FPS(100.0);	/* 4th */
    vaux->gear_best_speed[6]  = MPH_TO_FPS(130.0);	/* 5th */
    vaux->offroad_performance = 0.0;
    vaux->thrust_mechanism    = REAR_WHEEL_DRIVE;

    vaux->left_gauge_class  = GAUGE_ANALOG_MPH;
    vaux->right_gauge_class = GAUGE_ANALOG_RPM;
    vaux->max_speed         = 200.0;
    vaux->max_rpm           = 8000.0;
    vaux->max_altitude      = 25000.0;

    return(vaux);
}


static int countach_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = 2.38;
    get_box_mc_normal(obj,x,y,z,sc->mc_normal);
    return(TRUE);
}


static int countach_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = 2.38;
    return(TRUE);
}


#define SHADOW_MARGIN 0.4
static int create_shadow_dl(
    void)
{
#if 1
    return 0;
#else
    static float body[] = {
	-3.00,	0.00,	6.43,
	 3.00,	0.00,	6.43,

	-1.52,	2.29,	-2.77,
	 1.52,	2.29,	-2.77,

	-2.75,	1.00,	-7.17 + SHADOW_MARGIN,
	 2.75,	1.00,	-7.17 + SHADOW_MARGIN,

	-3.00,	0.00,	-7.17 + SHADOW_MARGIN,
	 3.00,	0.00,	-7.17 + SHADOW_MARGIN,

	-3.00,	0.00,	6.43,
	 3.00,	0.00,	6.43,
    };
    static float side1[] = {
	-3.00,	0.00,	-7.17 + SHADOW_MARGIN,
	-2.75,	1.00,	-7.17 + SHADOW_MARGIN,
	-1.52,	2.29,	-2.77,
	-3.00,	0.00,	6.43,
    };
    static float side2[] = {
	 3.00,	0.00,	6.43,
	 1.52,	2.29,	-2.77,
	 2.75,	1.00,	-7.17 + SHADOW_MARGIN,
	 3.00,	0.00,	-7.17 + SHADOW_MARGIN,
    };
    /* Make it small so it isn't seen unless center is seen. */
    static float mc_extent[2][3] = {
	-0.1, 0.0, -0.1,
	 0.1, 0.1,  0.1
    };
    int seg,graphics_seg;

    seg = get_dl_segment();
    graphics_seg = get_dl_segment();

    open_segment(img_fildes,graphics_seg,TRUE,FALSE);
	SHADOW(img_fildes);
	TRANSPARENT_ON(img_fildes,0.5);
	quadrilateral_mesh(img_fildes,body,5,2,NULL);
	polygon3d(img_fildes,side1,NUMPTS(side1),NULL);
	polygon3d(img_fildes,side2,NUMPTS(side2),NULL);
	TRANSPARENT_OFF(img_fildes);
    close_segment(img_fildes);

    open_segment(img_fildes,seg,TRUE,FALSE);
	set_extent(img_fildes,mc_extent);
	cond_execute_segment(img_fildes,CI_PRUNE,FALSE,graphics_seg);
    close_segment(img_fildes);

    return(seg);
#endif
}

#define BODY_COLOR(fildes) \
	surface_model((fildes),TRUE,4,1.0,1.0,1.0); 

#define ZF_FRONT	(-1.0)
#define ZF_2		(ZF_FRONT+0.25)


#if !defined(HOVERWARE_MODEL)
static void high_res_model(
    void)
{
    static float hood[] = {
	-2.81,	0.00,	5.88,	-SIN20,	-COS45,		SIN45,
	-2.60,	0.23,	6.24,	-SIN20,	-COS60,		SIN60,
	-2.60,	0.52,	6.43,	-SIN20,	COS20*COS45,	SIN45,
	-3.03,	1.44,	3.89,	-SIN20,	COS20*COS20,	SIN20,
	-3.01,	1.51,	2.83,	-SIN20,	COS20,		0.0,

	-2.13,	0.00,	5.88,	0.0,	-COS45,		SIN45,
	-2.13,	0.23,	6.24,	0.0,	-COS60,		SIN60,
	-2.13,	0.52,	6.43,	0.0,	COS45,		SIN45,
	-2.22,	1.54,	3.89,	0.0,	COS20,		SIN20,
	-2.22,	1.61,	2.83,	0.0,	1.0,		0.0,

	-1.60,	0.00,	5.88,	SIN15,	-COS45,		SIN45,
	-1.60,	0.23,	6.24,	SIN15,	-COS60,		SIN60,
	-1.60,	0.52,	6.43,	SIN15,	COS15*COS45,	SIN45,
	-1.54,	1.44,	4.00,	SIN15,	COS15*COS20,	SIN20,
	-1.50,	1.51,	3.00,	SIN15,	COS15,		0.0,

	-1.50,	0.00,	5.88,	0.0,	-COS45,		SIN45,
	-1.50,	0.23,	6.24,	0.0,	-COS60,		SIN60,
	-1.50,	0.52,	6.43,	0.0,	COS45,		SIN45,
	-1.44,	1.44,	4.00,	0.0,	COS20,		SIN20,
	-1.40,	1.51,	3.00,	0.0,	1.0,		0.0,

	 1.50,	0.00,	5.88,	0.0,	-COS45,		SIN45,
	 1.50,	0.23,	6.24,	0.0,	-COS60,		SIN60,
	 1.50,	0.52,	6.43,	0.0,	COS45,		SIN45,
	 1.44,	1.44,	4.00,	0.0,	COS20,		SIN20,
	 1.40,	1.51,	3.00,	0.0,	1.0,		0.0,

	 1.60,	0.00,	5.88,	-SIN15,	-COS45,		SIN45,
	 1.60,	0.23,	6.24,	-SIN15,	-COS60,		SIN60,
	 1.60,	0.52,	6.43,	-SIN15,	COS15*COS45,	SIN45,
	 1.54,	1.44,	4.00,	-SIN15,	COS15*COS20,	SIN20,
	 1.50,	1.51,	3.00,	-SIN15,	COS15,		0.0,

	 2.13,	0.00,	5.88,	0.0,	-COS45,		SIN45,
	 2.13,	0.23,	6.24,	0.0,	-COS60,		SIN60,
	 2.13,	0.52,	6.43,	0.0,	COS45,		SIN45,
	 2.22,	1.54,	3.89,	0.0,	COS20,		SIN20,
	 2.22,	1.61,	2.83,	0.0,	1.0,		0.0,

	 2.81,	0.00,	5.88,	SIN20,	-COS45,		SIN45,
	 2.60,	0.23,	6.24,	SIN20,	-COS60,		SIN60,
	 2.60,	0.52,	6.43,	SIN20,	COS20*COS45,	SIN45,
	 3.03,	1.44,	3.89,	SIN20,	COS20*COS20,	SIN20,
	 3.01,	1.51,	2.83,	SIN20,	COS20,		0.0,
    };
    static float lheadlight[] = {
	-2.60,	0.75,	6.00,
	-2.70,	0.90,	5.60,
	-1.70,	0.95,	5.60,
	-1.70,	0.80,	6.00,
    };
    static float rheadlight[] = {
	1.70,	0.80,	6.00,
	1.70,	0.95,	5.60,
	2.70,	0.90,	5.60,
	2.60,	0.75,	6.00,
    };
    static float lyrunninglight[] = {
	-2.85,	0.60,	5.35,
	-2.95,	0.60,	4.85,
	-2.95,	0.68,	4.85,
	-2.85,	0.68,	5.35,
    };
    static float lrrunninglight[] = {
	-2.95,	1.02,	-5.90,
	-2.90,	1.02,	-6.40,
	-2.90,	1.10,	-6.40,
	-2.95,	1.10,	-5.90,
    };
    static float ryrunninglight[] = {
	2.85,	0.68,	5.35,
	2.95,	0.68,	4.85,
	2.95,	0.60,	4.85,
	2.85,	0.60,	5.35,
    };
    static float rrrunninglight[] = {
	2.95,	1.10,	-5.90,
	2.90,	1.10,	-6.40,
	2.90,	1.02,	-6.40,
	2.95,	1.02,	-5.90,
    };
    static float lside_mid[] = {
	-2.75,	0.23,	-6.15,	-COS0,	-SIN0 *COS30,	-SIN30,
	-2.75,	1.00,	-7.17,	-COS0,	 SIN0 *COS30,	-SIN30,
	-3.00,	1.11,	-5.40,	-COS0,	 SIN0 *COS15,	-SIN15,
	-2.76,	1.42,	-7.17,	-COS20,	 SIN20*COS15,	-SIN15,
	-3.00,	1.10,	-4.50,	-COS0,	 SIN0,	0.0,
	-2.80,	1.57,	-5.60,	-COS20,	 SIN20,	0.0,
	-3.00,	0.23,	-3.27,	-COS0,	-SIN0,	0.0,
	-2.84,	1.72,	-4.50,	-COS20,	 SIN20,	0.0,
	-3.10,	0.23,	2.32,	-COS0,	-SIN0,	0.0,
	-2.80,	1.75,	-2.77,	-COS20,	 SIN20,	0.0,
	-3.10,	0.80,	2.50,	-COS0,	 SIN0,	0.0,
	-2.95,	1.45,	2.23,	-COS20,	 SIN20,	0.0,
	-3.10,	1.10,	3.40,	-COS0,	 SIN0,	0.0,
	-3.01,	1.51,	2.83,	-COS20,	 SIN20,	0.0,
	-3.10,	0.95,	4.00,	-COS0,	 SIN0,	0.0,
	-3.03,	1.44,	3.89,	-COS20,	 SIN20,	0.0,
	-3.00,	0.23,	4.78,	-COS0,	-SIN0 *COS15,	SIN15,
	-2.95,	1.35,	4.40,	-COS20,	 SIN20*COS15,	SIN15,
	-2.60,	0.23,	6.24,	-COS0,	-SIN0 *COS30,	SIN30,
	-2.60,	0.52,	6.43,	-COS20,	 SIN20*COS30,	SIN30,
    };
    static float lfront_flare[] = {
	-3.10,	0.00,	2.32,	-COS30,	-SIN30,	0.0,
	-3.10,	0.23,	2.32,	-COS0,	-SIN0,	0.0,
	-3.10,	0.80,	2.50,	-COS0,	 SIN0,	0.0,
	-3.10,	1.10,	3.40,	-COS0,	 SIN0,	0.0,
	-3.10,	0.95,	4.00,	-COS0,	 SIN0,	0.0,
	-3.00,	0.23,	4.78,	-COS0,	-SIN0 *COS15,	SIN15,
	-3.00,	0.00,	4.78,	-COS20,	-SIN20,	0.0,
	-2.81,	0.00,	6.00,	-COS20, -SIN20, 0.0,

	-3.30,	0.00,	2.42,	0.0,	SIN0,	-COS0,
	-3.30,	0.23,	2.42,	0.0,	SIN0,	-COS0,
	-3.30,	0.73,	2.57,	0.0,	SIN60,	-COS60,
	-3.30,	1.00,	3.40,	0.0,	SIN90,	COS90,
	-3.30,	0.90,	3.95,	0.0,	SIN60,	COS60,
	-3.20,	0.13,	4.68,	0.0,	SIN0,	COS0,
	-3.10, -0.10,	4.78,	-1.0,	0.0,	0.0,
	-2.91, -0.10,	5.98,	-1.0,	0.0,	0.0,

	-3.30,	0.00,	2.52,	-1.0,	0.0,	0.0,
	-3.30,	0.23,	2.52,	-1.0,	0.0,	0.0,
	-3.30,	0.66,	2.64,	-1.0,	0.0,	0.0,
	-3.30,	0.90,	3.40,	-1.0,	0.0,	0.0,
	-3.30,	0.85,	3.90,	-1.0,	0.0,	0.0,
	-3.20,	0.13,	4.58,	-1.0,	0.0,	0.0,
	-3.10, -0.20,	4.78,	-COS20,  SIN20,	0.0,
	-2.91, -0.20,	5.88,	-COS20,  SIN20,  0.0,
    };
    static float rfront_flare[] = {
	3.30,	0.00,	2.52,	-1.0,	0.0,	0.0,
	3.30,	0.23,	2.52,	-1.0,	0.0,	0.0,
	3.30,	0.66,	2.64,	-1.0,	0.0,	0.0,
	3.30,	0.90,	3.40,	-1.0,	0.0,	0.0,
	3.30,	0.85,	3.90,	-1.0,	0.0,	0.0,
	3.20,	0.13,	4.58,	-1.0,	0.0,	0.0,
	3.10, -0.20,	4.78,	-COS20,  SIN20,	0.0,
	2.91, -0.20,	5.88,	-COS20,  SIN20,  0.0,

	3.30,	0.00,	2.42,	0.0,	SIN0,	-COS0,
	3.30,	0.23,	2.42,	0.0,	SIN0,	-COS0,
	3.30,	0.73,	2.57,	0.0,	SIN60,	-COS60,
	3.30,	1.00,	3.40,	0.0,	SIN90,	COS90,
	3.30,	0.90,	3.95,	0.0,	SIN60,	COS60,
	3.20,	0.13,	4.68,	0.0,	SIN0,	COS0,
	3.10, -0.10,	4.78,	-1.0,	0.0,	0.0,
	2.91, -0.10,	5.98,	-1.0,	0.0,	0.0,

	3.10,	0.00,	2.32,	-COS30,	-SIN30,	0.0,
	3.10,	0.23,	2.32,	-COS0,	-SIN0,	0.0,
	3.10,	0.80,	2.50,	-COS0,	 SIN0,	0.0,
	3.10,	1.10,	3.40,	-COS0,	 SIN0,	0.0,
	3.10,	0.95,	4.00,	-COS0,	 SIN0,	0.0,
	3.00,	0.23,	4.78,	-COS0,	-SIN0 *COS15,	SIN15,
	3.00,	0.00,	4.78,	-COS20,	-SIN20,	0.0,
	2.81,	0.00,	6.00,	-COS20, -SIN20, 0.0,
    };
    static float fflare[] = {
	 2.81,	0.00,	6.00,
	 2.91, -0.20,	5.88,
	-2.91, -0.20,	5.88,
	-2.81,	0.00,	6.00,
    };
    static float lrear_flare[] = {
	-2.75,	0.23,	-6.15,	-COS0,	-SIN0 *COS30,	-SIN30,
	-3.00,	1.11,	-5.40,	-COS0,	 SIN0 *COS15,	-SIN15,
	-3.00,	1.10,	-4.50,	-COS0,	 SIN0,	0.0,
	-3.00,	0.23,	-3.27,	-COS0,	-SIN0,	0.0,
	-3.00,	0.00,	-3.27,	-COS30,	-SIN30,	0.0,

	-3.05,	0.23,	-5.95,	0.0,	SIN0,	-COS0,
	-3.30,	1.00,	-5.26,	0.0,	SIN60,	-COS60,
	-3.30,	0.90,	-4.50,	0.0,	SIN90,	-COS90,
	-3.30,	0.09,	-3.41,	0.0,	SIN60,	COS60,
	-3.30,	0.00,	-3.47,	0.0,	SIN0,	COS0,

	-3.05,	0.23,	-5.85,	-1.0,	0.0,	0.0,
	-3.30,	0.93,	-5.19,	-1.0,	0.0,	0.0,
	-3.30,	0.80,	-4.50,	-1.0,	0.0,	0.0,
	-3.30,	0.01,	-3.48,	-1.0,	0.0,	0.0,
	-3.30,	0.00,	-3.57,	-1.0,	0.0,	0.0,
    };
    static float rrear_flare[] = {
	3.05,	0.23,	-5.85,	-1.0,	0.0,	0.0,
	3.30,	0.93,	-5.19,	-1.0,	0.0,	0.0,
	3.30,	0.80,	-4.50,	-1.0,	0.0,	0.0,
	3.30,	0.01,	-3.48,	-1.0,	0.0,	0.0,
	3.30,	0.00,	-3.57,	-1.0,	0.0,	0.0,

	3.05,	0.23,	-5.95,	0.0,	SIN0,	-COS0,
	3.30,	1.00,	-5.26,	0.0,	SIN60,	-COS60,
	3.30,	0.90,	-4.50,	0.0,	SIN90,	-COS90,
	3.30,	0.09,	-3.41,	0.0,	SIN60,	COS60,
	3.30,	0.00,	-3.47,	0.0,	SIN0,	COS0,

	2.75,	0.23,	-6.15,	-COS0,	-SIN0 *COS30,	-SIN30,
	3.00,	1.11,	-5.40,	-COS0,	 SIN0 *COS15,	-SIN15,
	3.00,	1.10,	-4.50,	-COS0,	 SIN0,	0.0,
	3.00,	0.23,	-3.27,	-COS0,	-SIN0,	0.0,
	3.00,	0.00,	-3.27,	-COS30,	-SIN30,	0.0,
    };
    static float ldoor_triangle[] = {
	-2.95,	1.10,	-0.35,
	-3.05,	0.85,	-1.50,
	-3.05,	0.82,	-2.00,
	-2.97,	1.38,	-2.00,
	-2.97,	1.35,	-1.50,
	-2.95,	1.10,	-0.35,
    };
    static float rdoor_triangle[] = {
	2.95,	1.10,	-0.35,
	2.97,	1.35,	-1.50,
	2.97,	1.38,	-2.00,
	3.05,	0.82,	-2.00,
	3.05,	0.85,	-1.50,
	2.95,	1.10,	-0.35,
    };
    static float lside_bot1[] = {
	-3.00,	0.00,	4.78,
	-3.00,	0.23,	4.78,
	-2.60,	0.23,	6.24,
	-2.81,	0.00,	5.88,
    };
    static float lside_bot2[] = {
	-3.10,	0.00,	2.32,
	-3.00,	0.00,	-3.27,
	-3.00,	0.23,	-3.27,
	-3.10,	0.23,	2.32,
    };
    static float rside_mid[] = {
	2.60,	0.23,	6.24,	COS0,	-SIN0 *COS30,	SIN30,
	2.60,	0.52,	6.43,	COS20,	 SIN20*COS30,	SIN30,
	3.00,	0.23,	4.78,	COS0,	-SIN0 *COS15,	SIN15,
	2.95,	1.35,	4.40,	COS20,	 SIN20*COS15,	SIN15,
	3.10,	0.95,	4.00,	COS0,	 SIN0,	0.0,
	3.03,	1.44,	3.89,	COS20,	 SIN20,	0.0,
	3.10,	1.10,	3.40,	COS0,	 SIN0,	0.0,
	3.01,	1.51,	2.83,	COS20,	 SIN20,	0.0,
	3.10,	0.80,	2.50,	COS0,	 SIN0,	0.0,
	2.95,	1.45,	2.23,	COS20,	 SIN20,	0.0,
	3.10,	0.23,	2.32,	COS0,	-SIN0,	0.0,
	2.80,	1.75,	-2.77,	COS20,	 SIN20,	0.0,
	3.00,	0.23,	-3.27,	COS0,	-SIN0,	0.0,
	2.84,	1.72,	-4.50,	COS20,	 SIN20,	0.0,
	3.00,	1.10,	-4.50,	COS0,	 SIN0,	0.0,
	2.80,	1.57,	-5.60,	COS20,	 SIN20,	0.0,
	3.00,	1.11,	-5.40,	COS0,	 SIN0 *COS15,	-SIN15,
	2.76,	1.42,	-7.17,	COS20,	 SIN20*COS15,	-SIN15,
	2.75,	0.23,	-6.15,	COS0,	-SIN0 *COS30,	-SIN30,
	2.75,	1.00,	-7.17,	COS0,	 SIN0 *COS30,	-SIN30,
    };
    static float rside_bot1[] = {
	2.81,	0.00,	5.88,
	2.60,	0.23,	6.24,
	3.00,	0.23,	4.78,
	3.00,	0.00,	4.78,
    };
    static float rside_bot2[] = {
	3.10,	0.23,	2.32,
	3.00,	0.23,	-3.27,
	3.00,	0.00,	-3.27,
	3.10,	0.00,	2.32,
    };
    static float lrear[] = {
	-2.76,	1.42,	-7.17,	-SIN5,	COS5,	0.0,
	-1.56,	1.72,	-7.17,	-SIN5,	COS5,	0.0,
	-2.80,	1.57,	-5.60,	-SIN10,	COS10,	0.0,
	-1.30,	2.00,	-5.60,	-SIN10,	COS10,	0.0,
	-2.84,	1.72,	-4.50,	-SIN15,	COS15,	0.0,
	-1.00,	2.19,	-4.50,	-SIN15,	COS15,	0.0,
	-2.80,	1.75,	-2.77,	-SIN20,	COS20,	0.0,
	-0.82,	2.29,	-2.77,	-SIN20,	COS20,	0.0,
    };
    static float llouver[] = {
	-2.50,	1.64,	-6.10,
	-1.90,	1.82,	-6.10,
	-1.90,	1.94,	-5.00,
	-2.50,	1.79,	-5.00,
    };
    static float rlouver[] = {
	2.50,	1.79,	-5.00,
	1.90,	1.94,	-5.00,
	1.90,	1.82,	-6.10,
	2.50,	1.64,	-6.10,
    };
    static float rrear[] = {
	2.80,	1.75,	-2.77,	SIN20,	COS20,	0.0,
	0.82,	2.29,	-2.77,	SIN20,	COS20,	0.0,
	2.84,	1.72,	-4.50,	SIN15,	COS15,	0.0,
	1.00,	2.19,	-4.50,	SIN15,	COS15,	0.0,
	2.80,	1.57,	-5.60,	SIN10,	COS10,	0.0,
	1.30,	2.00,	-5.60,	SIN10,	COS10,	0.0,
	2.76,	1.42,	-7.17,	SIN5,	COS5,	0.0,
	1.56,	1.72,	-7.17,	SIN5,	COS5,	0.0,
    };
    static float top[] = {
	-1.52,	2.29,	-2.77,
	1.52,	2.29,	-2.77,
	1.52,	2.38,	0.48,
	-1.52,	2.38,	0.48,
    };

    static float front_window[] = {
	-2.70,	1.51,	2.63,	-SIN45,	COS75*COS45,	SIN75,
	-1.52,	2.38,	0.48,	-SIN45,	COS75*COS45,	SIN75,
	-2.22,	1.61,	2.83,	-SIN20,	COS75*COS20,	SIN75,
	-1.42,	2.38,	0.48,	-SIN20,	COS75*COS20,	SIN75,
	-1.50,	1.51,	3.00,	0.0,	COS75,		SIN75,
	-1.32,	2.38,	0.48,	0.0,	COS75,		SIN75,
	 1.50,	1.51,	3.00,	0.0,	COS75,		SIN75,
	 1.32,	2.38,	0.48,	0.0,	COS75,		SIN75,
	 2.22,	1.61,	2.83,	SIN20,	COS75*COS20,	SIN75,
	 1.42,	2.38,	0.48,	SIN20,	COS75*COS20,	SIN75,
	 2.70,	1.51,	2.63,	SIN45,	COS75*COS45,	SIN75,
	 1.52,	2.38,	0.48,	SIN45,	COS75*COS45,	SIN75,
    };

    static float lside_top[] = {
	-2.22,	1.61,	2.83,	-SIN20,	COS20,	0.0,
	-3.01,	1.51,	2.83,	-SIN20,	COS20,	0.0,
	-2.70,	1.51,	2.63,	-SIN20,	COS20,	0.0,
	-2.95,	1.45,	2.23,	-SIN20,	COS20,	0.0,
	-2.60,	1.50,	2.23,	-SIN20,	COS20,	0.0,
	-2.80,	1.75,	-2.77,	-SIN20,	COS20,	0.0,
	-2.49,	1.85,	-2.77,	-SIN20,	COS20,	0.0,
    };
    static float rside_top[] = {
	2.49,	1.85,	-2.77,	SIN20,	COS20,	0.0,
	2.80,	1.75,	-2.77,	SIN20,	COS20,	0.0,
	2.60,	1.50,	2.23,	SIN20,	COS20,	0.0,
	2.95,	1.45,	2.23,	SIN20,	COS20,	0.0,
	2.70,	1.51,	2.63,	SIN20,	COS20,	0.0,
	3.01,	1.51,	2.83,	SIN20,	COS20,	0.0,
	2.22,	1.61,	2.83,	SIN20,	COS20,	0.0,
    };

    static float flwinrib[] = {
	-2.70,	1.51,	2.43,
	-1.52,	2.38,	0.28,
	-1.52,	2.38,	0.48,
	-2.70,	1.51,	2.63,
    };
    static float frwinrib[] = {
	2.70,	1.51,	2.63,
	1.52,	2.38,	0.48,
	1.52,	2.38,	0.28,
	2.70,	1.51,	2.43,
    };

    static float lwin[] = {
	-1.52,	2.38,	0.28,
	-2.70,	1.51,	2.43,
	-2.70,	1.51,	-1.21,
	-1.52,	2.40,	-1.21,
    };
    static float rwin[] = {
	1.52,	2.40,	-1.21,
	2.70,	1.51,	-1.21,
	2.70,	1.51,	2.43,
	1.52,	2.38,	0.28,
    };

    static float blwinrib[] = {
	-2.70,	1.51,	-1.41,
	-1.52,	2.40,	-1.41,
	-1.52,	2.40,	-1.21,
	-2.70,	1.51,	-1.21,
    };
    static float brwinrib[] = {
	2.70,	1.51,	-1.21,
	1.52,	2.40,	-1.21,
	1.52,	2.40,	-1.41,
	2.70,	1.51,	-1.41,
    };
    static float bwin[] = {
	-1.52,	2.29,	-2.77,
	-1.52,	1.90,	-2.77,
	1.52,	1.90,	-2.77,
	1.52,	2.29,	-2.77,
    };
    static float blwin[] = {
	-2.49,	1.85,	-2.77,
	-1.52,	2.29,	-2.77,
	-1.52,	2.40,	-1.41,
	-2.70,	1.51,	-1.41,
    };
    static float brwin[] = {
	2.70,	1.51,	-1.41,
	1.52,	2.40,	-1.41,
	1.52,	2.29,	-2.77,
	2.49,	1.85,	-2.77,
    };
    static float rear_deck[] = {
	-0.82,	2.29,	-2.77,	COS15,	SIN15*COS30,	-SIN30,
	-1.00,	2.19,	-4.50,	COS15,	SIN15*COS30,	-SIN30,
	-1.30,	2.00,	-5.60,	COS15,	SIN15*COS30,	-SIN30,
	-1.56,	1.72,	-7.17,	COS15,	SIN15*COS30,	-SIN30,

	-0.62,	1.90,	-2.77,	COS15,	SIN15*COS30,	-SIN30,
	-0.80,	1.89,	-4.50,	COS15,	SIN15*COS30,	-SIN30,
	-1.10,	1.80,	-5.60,	COS15,	SIN15*COS30,	-SIN30,
	-1.46,	1.62,	-7.17,	COS15,	SIN15*COS30,	-SIN30,

	-0.61,	1.90,	-2.77,	0.0,	COS15,		-SIN15,
	-0.79,	1.89,	-4.50,	0.0,	COS15,		-SIN15,
	-1.09,	1.80,	-5.60,	0.0,	COS15,		-SIN15,
	-1.45,	1.62,	-7.17,	0.0,	COS15,		-SIN15,

	 0.61,	1.90,	-2.77,	0.0,	COS15,		-SIN15,
	 0.79,	1.89,	-4.50,	0.0,	COS15,		-SIN15,
	 1.09,	1.80,	-5.60,	0.0,	COS15,		-SIN15,
	 1.45,	1.62,	-7.17,	0.0,	COS15,		-SIN15,

	 0.62,	1.90,	-2.77,	-COS15,	SIN15*COS30,	-SIN30,
	 0.80,	1.89,	-4.50,	-COS15,	SIN15*COS30,	-SIN30,
	 1.10,	1.80,	-5.60,	-COS15,	SIN15*COS30,	-SIN30,
	 1.46,	1.62,	-7.17,	-COS15,	SIN15*COS30,	-SIN30,

	 0.82,	2.29,	-2.77,	-COS15,	SIN15*COS30,	-SIN30,
	 1.00,	2.19,	-4.50,	-COS15,	SIN15*COS30,	-SIN30,
	 1.30,	2.00,	-5.60,	-COS15,	SIN15*COS30,	-SIN30,
	 1.56,	1.72,	-7.17,	-COS15,	SIN15*COS30,	-SIN30,
    };
	
    static float uback[] = {
	-1.46,	0.80,	-7.17,
	 1.46,	0.80,	-7.17,
	 1.46,	1.62,	-7.17,
	-1.46,	1.62,	-7.17,
    };
    static float lbrakearea[] = {
	-1.56,	0.70,	-7.17,
	-1.46,	0.80,	-7.17,
	-1.46,	1.62,	-7.17,
	-1.56,	1.72,	-7.17,
	-2.76,	1.42,	-7.17,
	-2.75,	1.00,	-7.17,
    };
    static float lybrake[] = {
	-2.64,	1.10,	-7.20,
	-2.31,	1.10,	-7.20,
	-2.31,	1.32,	-7.20,
	-2.64,	1.32,	-7.20,
    };
    static float lwbrake[] = {
	-1.88,	1.10,	-7.20,
	-1.55,	1.10,	-7.20,
	-1.55,	1.32,	-7.20,
	-1.88,	1.32,	-7.20,
    };
    static float rbrakearea[] = {
	 1.46,	0.80,	-7.17,
	 1.56,	0.70,	-7.17,
	 2.75,	1.00,	-7.17,
	 2.76,	1.42,	-7.17,
	 1.56,	1.72,	-7.17,
	 1.46,	1.62,	-7.17,
    };
    static float rybrake[] = {
	2.64,	1.32,	-7.20,
	2.31,	1.32,	-7.20,
	2.31,	1.10,	-7.20,
	2.64,	1.10,	-7.20,
    };
    static float rwbrake[] = {
	1.88,	1.32,	-7.20,
	1.55,	1.32,	-7.20,
	1.55,	1.10,	-7.20,
	1.88,	1.10,	-7.20,
    };
    static float lback[] = {
	 2.75,	0.23,	-6.15,
	 2.75,	1.00,	-7.17,
	 1.56,	0.23,	-6.00,
	 1.56,	0.70,	-7.17,
	 1.46,	0.23,	-6.00,
	 1.46,	0.80,	-7.17,
	-1.46,	0.23,	-6.00,
	-1.46,	0.80,	-7.17,
	-1.56,	0.23,	-6.00,
	-1.56,	0.70,	-7.17,
	-2.75,	0.23,	-6.15,
	-2.75,	1.00,	-7.17,
    };
    static float lintake_cover[] = {
	-2.55,	1.76,	-2.77,
	-2.66,	1.82,	-4.10,
	-2.65,	2.10,	-2.77,
	-2.64,	1.92,	-4.10,
	-1.66,	2.49,	-2.77,
	-1.70,	2.13,	-3.80,
	-1.56,	2.10,	-2.77,
	-1.60,	1.85,	-3.80,
    };
    static float lintake[] = {
	-2.55,	1.76,	-2.78,
	-2.65,	2.10,	-2.78,
	-1.66,	2.49,	-2.78,
	-1.56,	2.10,	-2.78,
    };
    static float rintake_cover[] = {
	 1.56,	2.10,	-2.77,
	 1.60,	1.85,	-3.80,
	 1.66,	2.49,	-2.77,
	 1.70,	2.13,	-3.80,
	 2.65,	2.10,	-2.77,
	 2.64,	1.92,	-4.10,
	 2.55,	1.76,	-2.77,
	 2.66,	1.82,	-4.10,
    };
    static float rintake[] = {
	 1.56,	2.10,	-2.78,
	 1.66,	2.49,	-2.78,
	 2.65,	2.10,	-2.78,
	 2.55,	1.76,	-2.78,
    };
#define SPOILER_HALFWIDTH	2.50
#define SPOILER_HALFHEIGHT	0.08
#define SPOILER_Y		2.40
    static float spoiler[] = {
	-SPOILER_HALFWIDTH,	SPOILER_Y,			-6.0,
	-SPOILER_HALFWIDTH,	SPOILER_Y+SPOILER_HALFHEIGHT,	-6.5,
	-SPOILER_HALFWIDTH,	SPOILER_Y+SPOILER_HALFHEIGHT,	-7.0,
	-SPOILER_HALFWIDTH,	SPOILER_Y-SPOILER_HALFHEIGHT,	-6.7,
	-SPOILER_HALFWIDTH,	SPOILER_Y,			-6.0,

	0.0,			SPOILER_Y,			-5.8,
	0.0,			SPOILER_Y+SPOILER_HALFHEIGHT,	-6.3,
	0.0,			SPOILER_Y+SPOILER_HALFHEIGHT,	-6.8,
	0.0,			SPOILER_Y-SPOILER_HALFHEIGHT,	-6.5,
	0.0,			SPOILER_Y,			-5.8,

	SPOILER_HALFWIDTH,	SPOILER_Y,			-6.0,
	SPOILER_HALFWIDTH,	SPOILER_Y+SPOILER_HALFHEIGHT,	-6.5,
	SPOILER_HALFWIDTH,	SPOILER_Y+SPOILER_HALFHEIGHT,	-7.0,
	SPOILER_HALFWIDTH,	SPOILER_Y-SPOILER_HALFHEIGHT,	-6.7,
	SPOILER_HALFWIDTH,	SPOILER_Y,			-6.0,
    };
#define SPOILER_CAP_HALFHT	0.12
    static float lspoiler_end1[] = {
	-SPOILER_HALFWIDTH-.05,	SPOILER_Y-SPOILER_CAP_HALFHT,	-7.05,
	-SPOILER_HALFWIDTH-.05,	SPOILER_Y+SPOILER_CAP_HALFHT,	-7.20,
	-SPOILER_HALFWIDTH-.05,	SPOILER_Y+SPOILER_CAP_HALFHT,	-5.95,
	-SPOILER_HALFWIDTH-.05,	SPOILER_Y,			-5.90,
	-SPOILER_HALFWIDTH-.05,	SPOILER_Y-SPOILER_CAP_HALFHT,	-6.00,
    };
    static float lspoiler_end2[] = {
	-SPOILER_HALFWIDTH-.05,	SPOILER_Y-SPOILER_CAP_HALFHT,	-6.00,
	-SPOILER_HALFWIDTH-.05,	SPOILER_Y,			-5.90,
	-SPOILER_HALFWIDTH-.05,	SPOILER_Y+SPOILER_CAP_HALFHT,	-5.95,
	-SPOILER_HALFWIDTH-.05,	SPOILER_Y+SPOILER_CAP_HALFHT,	-7.20,
	-SPOILER_HALFWIDTH-.05,	SPOILER_Y-SPOILER_CAP_HALFHT,	-7.05,
    };
    static float rspoiler_end1[] = {
	SPOILER_HALFWIDTH+.05,	SPOILER_Y-SPOILER_CAP_HALFHT,	-6.00,
	SPOILER_HALFWIDTH+.05,	SPOILER_Y,			-5.90,
	SPOILER_HALFWIDTH+.05,	SPOILER_Y+SPOILER_CAP_HALFHT,	-5.95,
	SPOILER_HALFWIDTH+.05,	SPOILER_Y+SPOILER_CAP_HALFHT,	-7.20,
	SPOILER_HALFWIDTH+.05,	SPOILER_Y-SPOILER_CAP_HALFHT,	-7.05,
    };
    static float rspoiler_end2[] = {
	SPOILER_HALFWIDTH+.05,	SPOILER_Y-SPOILER_CAP_HALFHT,	-7.05,
	SPOILER_HALFWIDTH+.05,	SPOILER_Y+SPOILER_CAP_HALFHT,	-7.20,
	SPOILER_HALFWIDTH+.05,	SPOILER_Y+SPOILER_CAP_HALFHT,	-5.95,
	SPOILER_HALFWIDTH+.05,	SPOILER_Y,			-5.90,
	SPOILER_HALFWIDTH+.05,	SPOILER_Y-SPOILER_CAP_HALFHT,	-6.00,
    };
#define SPOILER_POST_BOTTOM 1.40
    static float lspoiler_post[] = {
	-1.50,	SPOILER_POST_BOTTOM,	-4.8,
	-1.60,	SPOILER_POST_BOTTOM,	-6.0,
	-1.50,	SPOILER_POST_BOTTOM,	-6.4,
	-1.40,	SPOILER_POST_BOTTOM,	-6.0,
	-1.50,	SPOILER_POST_BOTTOM,	-4.8,

	-1.50,	SPOILER_Y-0.05,		-6.0,
	-1.60,	SPOILER_Y-0.05,		-6.5,
	-1.50,	SPOILER_Y-0.05,		-6.7,
	-1.40,	SPOILER_Y-0.05,		-6.5,
	-1.50,	SPOILER_Y-0.05,		-6.0,
    };
    static float rspoiler_post[] = {
	1.50,	SPOILER_Y-0.05,		-6.0,
	1.60,	SPOILER_Y-0.05,		-6.5,
	1.50,	SPOILER_Y-0.05,		-6.7,
	1.40,	SPOILER_Y-0.05,		-6.5,
	1.50,	SPOILER_Y-0.05,		-6.0,

	1.50,	SPOILER_POST_BOTTOM,	-4.8,
	1.60,	SPOILER_POST_BOTTOM,	-6.0,
	1.50,	SPOILER_POST_BOTTOM,	-6.4,
	1.40,	SPOILER_POST_BOTTOM,	-6.0,
	1.50,	SPOILER_POST_BOTTOM,	-4.8,
    };

#define TIRE_RADIUS	0.95
#define TIRE_WIDTH	0.8
#define HUB_Y (1.0-TIRE_RADIUS)
#define FHUB_Z	((2.32+4.78)/2.0)
#define BHUB_Z	(-4.80)
    static float fltire[] = {
	-3.1+TIRE_WIDTH,	HUB_Y,			FHUB_Z,
	-3.1+TIRE_WIDTH,	HUB_Y-TIRE_RADIUS,	FHUB_Z,
	-3.1,			HUB_Y-TIRE_RADIUS,	FHUB_Z,
	-3.1,			HUB_Y-TIRE_RADIUS*0.5,	FHUB_Z,
    };
    static float frtire[] = {
	 3.1-TIRE_WIDTH,	HUB_Y,			FHUB_Z,
	 3.1-TIRE_WIDTH,	HUB_Y-TIRE_RADIUS,	FHUB_Z,
	 3.1,			HUB_Y-TIRE_RADIUS,	FHUB_Z,
	 3.1,			HUB_Y-TIRE_RADIUS*0.5,	FHUB_Z,
    };
    static float bltire[] = {
	-3.1+TIRE_WIDTH,	HUB_Y,			BHUB_Z,
	-3.1+TIRE_WIDTH,	HUB_Y-TIRE_RADIUS,	BHUB_Z,
	-3.1,			HUB_Y-TIRE_RADIUS,	BHUB_Z,
	-3.1,			HUB_Y-TIRE_RADIUS*0.5,	BHUB_Z,
    };
    static float brtire[] = {
	 3.1-TIRE_WIDTH,	HUB_Y,			BHUB_Z,
	 3.1-TIRE_WIDTH,	HUB_Y-TIRE_RADIUS,	BHUB_Z,
	 3.1,			HUB_Y-TIRE_RADIUS,	BHUB_Z,
	 3.1,			HUB_Y-TIRE_RADIUS*0.5,	BHUB_Z,
    };

    static float flhubcap[] = {
	-3.1,		HUB_Y-TIRE_RADIUS*0.5,	FHUB_Z,
		0.0, 1.0, 0.0,
	-3.1+0.15,	HUB_Y-TIRE_RADIUS*0.3,	FHUB_Z,
		0.0, 1.0, 0.0,
	-3.0+0.15,	HUB_Y-TIRE_RADIUS*0.3+0.1, FHUB_Z,
		-1.0, 0.0, 0.0,
	-3.0+0.15,	HUB_Y,			FHUB_Z,
		-1.0, 0.0, 0.0,
    };
    static float frhubcap[] = {
	 3.1,		HUB_Y-TIRE_RADIUS*0.5,	FHUB_Z,
		0.0, 1.0, 0.0,
	 3.1-0.15,	HUB_Y-TIRE_RADIUS*0.3,	FHUB_Z,
		0.0, 1.0, 0.0,
	 3.0-0.15,	HUB_Y-TIRE_RADIUS*0.3+0.1,	FHUB_Z,
		1.0, 0.0, 0.0,
	 3.0-0.15,	HUB_Y,			FHUB_Z,
		1.0, 0.0, 0.0,
    };
    static float blhubcap[] = {
	-3.1,		HUB_Y-TIRE_RADIUS*0.5,	BHUB_Z,
		0.0, 1.0, 0.0,
	-3.1+0.25,	HUB_Y-TIRE_RADIUS*0.3,	BHUB_Z,
		0.0, 1.0, 0.0,
	-3.0+0.25,	HUB_Y-TIRE_RADIUS*0.3+0.1,	BHUB_Z,
		-1.0, 0.0, 0.0,
	-3.0+0.25,	HUB_Y,			BHUB_Z,
		-1.0, 0.0, 0.0,
    };
    static float brhubcap[] = {
	 3.1,		HUB_Y-TIRE_RADIUS*0.5,	BHUB_Z,
		0.0, 1.0, 0.0,
	 3.1-0.25,	HUB_Y-TIRE_RADIUS*0.3,	BHUB_Z,
		0.0, 1.0, 0.0,
	 3.0-0.25,	HUB_Y-TIRE_RADIUS*0.3+0.1,	BHUB_Z,
		1.0, 0.0, 0.0,
	 3.0-0.25,	HUB_Y,			BHUB_Z,
		1.0, 0.0, 0.0,
    };

    static float underside[] = {
	 2.81,			0.00,	5.88,
	 3.00,			0.00,	FHUB_Z+TIRE_RADIUS*1.1,
	 3.00-TIRE_WIDTH*1.1,	0.00,	FHUB_Z+TIRE_RADIUS*1.1,
	 3.00-TIRE_WIDTH*1.1,	0.00,	FHUB_Z-TIRE_RADIUS*1.1,
	 3.00,			0.00,	FHUB_Z-TIRE_RADIUS*1.1,
	 3.00,			0.00,	BHUB_Z+TIRE_RADIUS*1.1,
	 3.00-TIRE_WIDTH*1.1,	0.00,	BHUB_Z+TIRE_RADIUS*1.1,
	 3.00-TIRE_WIDTH*1.1,	0.00,	-6.0,

	-3.00+TIRE_WIDTH*1.1,	0.00,	-6.0,
	-3.00+TIRE_WIDTH*1.1,	0.00,	BHUB_Z+TIRE_RADIUS*1.1,
	-3.00,			0.00,	BHUB_Z+TIRE_RADIUS*1.1,
	-3.00,			0.00,	FHUB_Z-TIRE_RADIUS*1.1,
	-3.00+TIRE_WIDTH*1.1,	0.00,	FHUB_Z-TIRE_RADIUS*1.1,
	-3.00+TIRE_WIDTH*1.1,	0.00,	FHUB_Z+TIRE_RADIUS*1.1,
	-3.00,			0.00,	FHUB_Z+TIRE_RADIUS*1.1,
	-2.81,	0.00,	5.88,
    };
    static float mat1[4][4] = IDENTITY4x4;
    static float mat2[4][4] = IDENTITY4x4;
    static float mat3[4][4] = IDENTITY4x4;
    float rmat[4][4];
#define DRAW_FRONT_LEFT_TIRE \
    RUBBER(img_fildes); \
    mesh_surface_of_revolution(img_fildes,fltire,NUMPTS(fltire), \
	HIGH_WHEEL_FACETS,FALSE, \
	-3.1,HUB_Y,FHUB_Z, -4.1,HUB_Y,FHUB_Z); \
    CHROME(img_fildes); \
    mesh_surface_of_revolution(img_fildes,flhubcap,NUMPTSNORMAL(flhubcap), \
	HIGH_WHEEL_FACETS,TRUE, \
	-3.1,HUB_Y,FHUB_Z, -4.1,HUB_Y,FHUB_Z); \

#define DRAW_FRONT_RIGHT_TIRE \
    RUBBER(img_fildes); \
    mesh_surface_of_revolution(img_fildes,frtire,NUMPTS(frtire), \
	HIGH_WHEEL_FACETS,FALSE, \
	 3.1,HUB_Y,FHUB_Z, 4.1,HUB_Y,FHUB_Z); \
    CHROME(img_fildes); \
    mesh_surface_of_revolution(img_fildes,frhubcap,NUMPTSNORMAL(frhubcap), \
	HIGH_WHEEL_FACETS,TRUE, \
	 3.1,HUB_Y,FHUB_Z, 4.1,HUB_Y,FHUB_Z);


    DIFFUSE_LIGHTING_ON(img_fildes);
    BODY_COLOR(img_fildes);
    vertex_format(img_fildes,3,3,0,0,COUNTER_CLOCKWISE);
    quadrilateral_mesh(img_fildes,hood,8,5,NULL);
    triangular_strip(img_fildes,lside_top,NUMPTSNORMAL(lside_top),NULL);
    triangular_strip(img_fildes,rside_top,NUMPTSNORMAL(rside_top),NULL);
    triangular_strip(img_fildes,lrear,NUMPTSNORMAL(lrear),NULL);
    triangular_strip(img_fildes,rrear,NUMPTSNORMAL(rrear),NULL);
    quadrilateral_mesh(img_fildes,rear_deck,6,4,NULL);
    triangular_strip(img_fildes,lside_mid,NUMPTSNORMAL(lside_mid),NULL);
    triangular_strip(img_fildes,rside_mid,NUMPTSNORMAL(rside_mid),NULL);
    quadrilateral_mesh(img_fildes,lfront_flare,3,8,NULL);
    quadrilateral_mesh(img_fildes,rfront_flare,3,8,NULL);
    quadrilateral_mesh(img_fildes,lrear_flare,3,5,NULL);
    quadrilateral_mesh(img_fildes,rrear_flare,3,5,NULL);
    vertex_format(img_fildes,0,0,0,0,COUNTER_CLOCKWISE);

    polygon3d(img_fildes,fflare,NUMPTS(fflare),NULL);

    polygon3d(img_fildes,lside_bot1,NUMPTS(lside_bot1),NULL);
    polygon3d(img_fildes,lside_bot2,NUMPTS(lside_bot2),NULL);
    polygon3d(img_fildes,flwinrib,NUMPTS(flwinrib),NULL);
    polygon3d(img_fildes,blwinrib,NUMPTS(blwinrib),NULL);
    quadrilateral_mesh(img_fildes,lintake_cover,4,2,NULL);

    polygon3d(img_fildes,rside_bot1,NUMPTS(rside_bot1),NULL);
    polygon3d(img_fildes,rside_bot2,NUMPTS(rside_bot2),NULL);
    polygon3d(img_fildes,frwinrib,NUMPTS(frwinrib),NULL);
    polygon3d(img_fildes,brwinrib,NUMPTS(brwinrib),NULL);
    quadrilateral_mesh(img_fildes,rintake_cover,4,2,NULL);


    polygon3d(img_fildes,top,NUMPTS(top),NULL);

    polygon3d(img_fildes,uback,NUMPTS(uback),NULL);
    quadrilateral_mesh(img_fildes,lback,6,2,NULL);

    quadrilateral_mesh(img_fildes,spoiler,3,5,NULL);
    polygon3d(img_fildes,lspoiler_end1,NUMPTS(lspoiler_end1),NULL);
    polygon3d(img_fildes,lspoiler_end2,NUMPTS(lspoiler_end2),NULL);
    polygon3d(img_fildes,rspoiler_end1,NUMPTS(rspoiler_end1),NULL);
    polygon3d(img_fildes,rspoiler_end2,NUMPTS(rspoiler_end2),NULL);
    quadrilateral_mesh(img_fildes,rspoiler_post,2,5,NULL);
    quadrilateral_mesh(img_fildes,lspoiler_post,2,5,NULL);

    GREY_GLASS(img_fildes);
    surface_model(img_fildes,TRUE,2,1.0,1.0,1.0);
    vertex_format(img_fildes,3,3,0,0,COUNTER_CLOCKWISE);
    triangular_strip(img_fildes,front_window,NUMPTSNORMAL(front_window),NULL);
    vertex_format(img_fildes,0,0,0,0,COUNTER_CLOCKWISE);
    polygon3d(img_fildes,lwin,NUMPTS(lwin),NULL);
    polygon3d(img_fildes,blwin,NUMPTS(blwin),NULL);
    polygon3d(img_fildes,rwin,NUMPTS(rwin),NULL);
    polygon3d(img_fildes,brwin,NUMPTS(brwin),NULL);
    polygon3d(img_fildes,bwin,NUMPTS(bwin),NULL);
    RESTORE_DEFAULT_SURFACE_MODEL(img_fildes);


    /*** BRAKELIGHTS ***/
    /* fully on */
    invis[0] = LIGHTS_OFF_BRAKES_ON|LIGHTS_ON_BRAKES_ON;
    add_names_to_set(img_fildes,1,invis);
	SELF_LIT_ON(img_fildes);
	PAINT(img_fildes,1.0,0.0,0.0);
	polygon3d(img_fildes,lbrakearea,NUMPTS(lbrakearea),NULL);
	polygon3d(img_fildes,rbrakearea,NUMPTS(rbrakearea),NULL);
	SELF_LIT_OFF(img_fildes);
    remove_all_names_from_set(img_fildes);

    /* half on */
    invis[0] = LIGHTS_ON_BRAKES_OFF;
    add_names_to_set(img_fildes,1,invis);
	SELF_LIT_ON(img_fildes);
	PAINT(img_fildes,0.7,0.0,0.0);
	polygon3d(img_fildes,lbrakearea,NUMPTS(lbrakearea),NULL);
	polygon3d(img_fildes,rbrakearea,NUMPTS(rbrakearea),NULL);
	SELF_LIT_OFF(img_fildes);
    remove_all_names_from_set(img_fildes);

    /* fully off */
    invis[0] = LIGHTS_OFF_BRAKES_OFF;
    add_names_to_set(img_fildes,1,invis);
	RED_PLASTIC(img_fildes);
	polygon3d(img_fildes,lbrakearea,NUMPTS(lbrakearea),NULL);
	polygon3d(img_fildes,rbrakearea,NUMPTS(rbrakearea),NULL);
    remove_all_names_from_set(img_fildes);


    /*** HEADLIGHTS AND RUNNING LIGHTS ***/
    /* on */
    invis[0] = LIGHTS_ON_BRAKES_OFF|LIGHTS_ON_BRAKES_ON;
    add_names_to_set(img_fildes,1,invis);
	SELF_LIT_ON(img_fildes);
	PAINT(img_fildes,1.0,0.0,0.0);
	polygon3d(img_fildes,lrrunninglight,NUMPTS(lrrunninglight),NULL);
	polygon3d(img_fildes,rrrunninglight,NUMPTS(rrrunninglight),NULL);
	PAINT(img_fildes,1.0,1.0,1.0);
	polygon3d(img_fildes,lheadlight,NUMPTS(lheadlight),NULL);
	polygon3d(img_fildes,rheadlight,NUMPTS(rheadlight),NULL);
	PAINT(img_fildes,1.0,1.0,0.0);
	polygon3d(img_fildes,rybrake,NUMPTS(rybrake),NULL);
	polygon3d(img_fildes,lybrake,NUMPTS(lybrake),NULL);
	polygon3d(img_fildes,lyrunninglight,NUMPTS(lyrunninglight),NULL);
	polygon3d(img_fildes,ryrunninglight,NUMPTS(ryrunninglight),NULL);
	SELF_LIT_OFF(img_fildes);
    remove_all_names_from_set(img_fildes);

    /* off */
    invis[0] = LIGHTS_OFF_BRAKES_OFF|LIGHTS_OFF_BRAKES_ON;
    add_names_to_set(img_fildes,1,invis);
	RED_PLASTIC(img_fildes);
	polygon3d(img_fildes,lrrunninglight,NUMPTS(lrrunninglight),NULL);
	polygon3d(img_fildes,rrrunninglight,NUMPTS(rrrunninglight),NULL);
	WHITE_PLASTIC(img_fildes);
	polygon3d(img_fildes,lheadlight,NUMPTS(lheadlight),NULL);
	polygon3d(img_fildes,rheadlight,NUMPTS(rheadlight),NULL);
	YELLOW_PLASTIC(img_fildes);
	polygon3d(img_fildes,rybrake,NUMPTS(rybrake),NULL);
	polygon3d(img_fildes,lybrake,NUMPTS(lybrake),NULL);
	polygon3d(img_fildes,lyrunninglight,NUMPTS(lyrunninglight),NULL);
	polygon3d(img_fildes,ryrunninglight,NUMPTS(ryrunninglight),NULL);
    remove_all_names_from_set(img_fildes);


    /*** BACK-UP LIGHTS ***/
    /* on */
    invis[0] = BACKUP_LIGHTS_ON;
    add_names_to_set(img_fildes,1,invis);
	SELF_LIT_ON(img_fildes);
	PAINT(img_fildes,1.0,1.0,1.0);
	polygon3d(img_fildes,rwbrake,NUMPTS(rwbrake),NULL);
	polygon3d(img_fildes,lwbrake,NUMPTS(lwbrake),NULL);
	SELF_LIT_OFF(img_fildes);
    remove_all_names_from_set(img_fildes);

    /* off */
    invis[0] = BACKUP_LIGHTS_OFF;
    add_names_to_set(img_fildes,1,invis);
	WHITE_PLASTIC(img_fildes);
	polygon3d(img_fildes,rwbrake,NUMPTS(rwbrake),NULL);
	polygon3d(img_fildes,lwbrake,NUMPTS(lwbrake),NULL);
    remove_all_names_from_set(img_fildes);

    /* Gun on hood */
    invis[0] = CAR_GUN_MASK;
    add_names_to_set(img_fildes,1,invis);
	ALUMINUM(img_fildes);
    mesh_cone(img_fildes,0.5,0.5,TRUE,TRUE,6,
	0.0, 1.5, 4.0,  0.0, 1.5, 5.5);
    mesh_cone(img_fildes,0.2,0.2,TRUE,TRUE,6,
	0.0, 1.5, 5.4,  0.0, 1.5, 7.0);


    remove_all_names_from_set(img_fildes);


    BLACK_PLASTIC(img_fildes);
    polygon3d(img_fildes,lintake,NUMPTS(lintake),NULL);
    polygon3d(img_fildes,rintake,NUMPTS(rintake),NULL);
    polygon3d(img_fildes,underside,NUMPTS(underside),NULL);
    polygon3d(img_fildes,llouver,NUMPTS(llouver),NULL);
    polygon3d(img_fildes,rlouver,NUMPTS(rlouver),NULL);
    polygon3d(img_fildes,ldoor_triangle,NUMPTS(ldoor_triangle),NULL);
    polygon3d(img_fildes,rdoor_triangle,NUMPTS(ldoor_triangle),NULL);


    RUBBER(img_fildes);
    mesh_surface_of_revolution(img_fildes,bltire,NUMPTS(bltire),
	HIGH_WHEEL_FACETS,FALSE,
	-3.1,HUB_Y,BHUB_Z, -4.1,HUB_Y,BHUB_Z);
    mesh_surface_of_revolution(img_fildes,brtire,NUMPTS(brtire),
	HIGH_WHEEL_FACETS,FALSE,
	 3.1,HUB_Y,BHUB_Z, 4.1,HUB_Y,BHUB_Z);

    CHROME(img_fildes);
    mesh_surface_of_revolution(img_fildes,blhubcap,NUMPTSNORMAL(blhubcap),
	HIGH_WHEEL_FACETS,TRUE,
	-3.1,HUB_Y,BHUB_Z, -4.1,HUB_Y,BHUB_Z);
    mesh_surface_of_revolution(img_fildes,brhubcap,NUMPTSNORMAL(brhubcap),
	HIGH_WHEEL_FACETS,TRUE,
	 3.1,HUB_Y,BHUB_Z, 4.1,HUB_Y,BHUB_Z);


    invis[0] = WHEELS_FAR_LEFT;
    add_names_to_set(img_fildes,1,invis);
	mat1[3][0] = 3.1; mat1[3][1] = -HUB_Y; mat1[3][2] = -FHUB_Z;
	mat2[0][0] = mat2[2][2] = FCOS(FAR_WHEEL_ANGLE);
	mat2[0][2] = FSIN(FAR_WHEEL_ANGLE);
	mat2[2][0] = -mat2[0][2];
	mat3[3][0] = -3.1; mat3[3][1] = HUB_Y; mat3[3][2] = FHUB_Z;
	concat_matrix(mat1,mat2,rmat);
	concat_matrix(rmat,mat3,rmat);
	concat_transformation3d(img_fildes,rmat,PRE,PUSH);
	DRAW_FRONT_LEFT_TIRE;
	pop_matrix(img_fildes);

	mat1[3][0] = -3.1; mat1[3][1] = -HUB_Y; mat1[3][2] = -FHUB_Z;
	mat3[3][0] = 3.1; mat3[3][1] = HUB_Y; mat3[3][2] = FHUB_Z;
	concat_matrix(mat1,mat2,rmat);
	concat_matrix(rmat,mat3,rmat);
	concat_transformation3d(img_fildes,rmat,PRE,PUSH);
	DRAW_FRONT_RIGHT_TIRE;
	pop_matrix(img_fildes);
    remove_all_names_from_set(img_fildes);

    invis[0] = WHEELS_LEFT;
    add_names_to_set(img_fildes,1,invis);
	mat1[3][0] = 3.1; mat1[3][1] = -HUB_Y; mat1[3][2] = -FHUB_Z;
	mat2[0][0] = mat2[2][2] = FCOS(NEAR_WHEEL_ANGLE);
	mat2[0][2] = FSIN(NEAR_WHEEL_ANGLE);
	mat2[2][0] = -mat2[0][2];
	mat3[3][0] = -3.1; mat3[3][1] = HUB_Y; mat3[3][2] = FHUB_Z;
	concat_matrix(mat1,mat2,rmat);
	concat_matrix(rmat,mat3,rmat);
	concat_transformation3d(img_fildes,rmat,PRE,PUSH);
	DRAW_FRONT_LEFT_TIRE;
	pop_matrix(img_fildes);

	mat1[3][0] = -3.1; mat1[3][1] = -HUB_Y; mat1[3][2] = -FHUB_Z;
	mat3[3][0] = 3.1; mat3[3][1] = HUB_Y; mat3[3][2] = FHUB_Z;
	concat_matrix(mat1,mat2,rmat);
	concat_matrix(rmat,mat3,rmat);
	concat_transformation3d(img_fildes,rmat,PRE,PUSH);
	DRAW_FRONT_RIGHT_TIRE;
	pop_matrix(img_fildes);
    remove_all_names_from_set(img_fildes);

    invis[0] = WHEELS_CENTER;
    add_names_to_set(img_fildes,1,invis);
	concat_transformation3d(img_fildes,identity,PRE,PUSH);
	DRAW_FRONT_LEFT_TIRE;
	DRAW_FRONT_RIGHT_TIRE;
	pop_matrix(img_fildes);
    remove_all_names_from_set(img_fildes);

    invis[0] = WHEELS_RIGHT;
    add_names_to_set(img_fildes,1,invis);
	mat1[3][0] = 3.1; mat1[3][1] = -HUB_Y; mat1[3][2] = -FHUB_Z;
	mat2[0][0] = mat2[2][2] = FCOS(NEAR_WHEEL_ANGLE);
	mat2[0][2] = FSIN(-NEAR_WHEEL_ANGLE);
	mat2[2][0] = -mat2[0][2];
	mat3[3][0] = -3.1; mat3[3][1] = HUB_Y; mat3[3][2] = FHUB_Z;
	concat_matrix(mat1,mat2,rmat);
	concat_matrix(rmat,mat3,rmat);
	concat_transformation3d(img_fildes,rmat,PRE,PUSH);
	DRAW_FRONT_LEFT_TIRE;
	pop_matrix(img_fildes);

	mat1[3][0] = -3.1; mat1[3][1] = -HUB_Y; mat1[3][2] = -FHUB_Z;
	mat3[3][0] = 3.1; mat3[3][1] = HUB_Y; mat3[3][2] = FHUB_Z;
	concat_matrix(mat1,mat2,rmat);
	concat_matrix(rmat,mat3,rmat);
	concat_transformation3d(img_fildes,rmat,PRE,PUSH);
	DRAW_FRONT_RIGHT_TIRE;
	pop_matrix(img_fildes);
    remove_all_names_from_set(img_fildes);

    invis[0] = WHEELS_FAR_RIGHT;
    add_names_to_set(img_fildes,1,invis);
	mat1[3][0] = 3.1; mat1[3][1] = -HUB_Y; mat1[3][2] = -FHUB_Z;
	mat2[0][0] = mat2[2][2] = FCOS(FAR_WHEEL_ANGLE);
	mat2[0][2] = FSIN(-FAR_WHEEL_ANGLE);
	mat2[2][0] = -mat2[0][2];
	mat3[3][0] = -3.1; mat3[3][1] = HUB_Y; mat3[3][2] = FHUB_Z;
	concat_matrix(mat1,mat2,rmat);
	concat_matrix(rmat,mat3,rmat);
	concat_transformation3d(img_fildes,rmat,PRE,PUSH);
	DRAW_FRONT_LEFT_TIRE;
	pop_matrix(img_fildes);

	mat1[3][0] = -3.1; mat1[3][1] = -HUB_Y; mat1[3][2] = -FHUB_Z;
	mat3[3][0] = 3.1; mat3[3][1] = HUB_Y; mat3[3][2] = FHUB_Z;
	concat_matrix(mat1,mat2,rmat);
	concat_matrix(rmat,mat3,rmat);
	concat_transformation3d(img_fildes,rmat,PRE,PUSH);
	DRAW_FRONT_RIGHT_TIRE;
	pop_matrix(img_fildes);
    remove_all_names_from_set(img_fildes);



    /* No need to RESTORE_DEFAULT_VERTEX_FORMAT, since last was default. */
    DIFFUSE_LIGHTING_OFF(img_fildes);
}


static void mid_res_model(
    void)
{
    static float hood[] = {
	-2.81,	0.00,	5.88,
	-2.60,	0.52,	6.43,
	-3.03,	1.44,	3.89,
	-3.01,	1.51,	2.83,

	 2.81,	0.00,	5.88,
	 2.60,	0.52,	6.43,
	 3.03,	1.44,	3.89,
	 3.01,	1.51,	2.83,
    };
    static float lside_mid[] = {
	-2.75,	1.00,	-7.17,
	-2.75,	0.00,	-6.15,
	-2.76,	1.42,	-7.17,
	-3.00,	0.23,	-3.27,
	-2.80,	1.75,	-2.77,
	-3.10,	0.00,	2.32,
	-2.95,	1.45,	2.23,
	-3.10,	0.80,	2.50,
	-3.01,	1.51,	2.83,
	-3.10,	1.00,	3.40,
	-3.03,	1.44,	3.89,
	-3.10,	0.85,	4.00,
	-2.95,	1.35,	4.40,
	-3.00,	0.00,	4.78,
	-2.60,	0.52,	6.43,
	-2.60,	0.00,	6.24,
    };
    static float rside_mid[] = {
	2.60,	0.00,	6.24,
	2.60,	0.52,	6.43,
	3.00,	0.00,	4.78,
	2.95,	1.35,	4.40,
	3.10,	0.85,	4.00,
	3.03,	1.44,	3.89,
	3.10,	1.00,	3.40,
	3.01,	1.51,	2.83,
	3.10,	0.80,	2.50,
	2.95,	1.45,	2.23,
	3.10,	0.00,	2.32,
	2.80,	1.75,	-2.77,
	3.00,	0.23,	-3.27,
	2.76,	1.42,	-7.17,
	2.75,	0.00,	-6.15,
	2.75,	1.00,	-7.17,
    };
    static float lrear[] = {
	-2.76,	1.42,	-7.17,
	-1.56,	1.72,	-7.17,
	-1.52,	2.29,	-2.77,
	-2.80,	1.75,	-2.77,
    };
    static float rrear[] = {
	2.80,	1.75,	-2.77,
	1.52,	2.29,	-2.77,
	1.56,	1.72,	-7.17,
	2.76,	1.42,	-7.17,
    };
    static float top[] = {
	-1.52,	2.29,	-2.77,
	1.52,	2.29,	-2.77,
	1.52,	2.38,	0.48,
	-1.52,	2.38,	0.48,
    };


    static float lside_top[] = {
	-2.49,	1.61,	2.83,
	-3.01,	1.51,	2.83,
	-2.80,	1.75,	-2.77,
	-2.49,	1.75,	-2.77,
    };
    static float rside_top[] = {
	2.49,	1.75,	-2.77,
	2.80,	1.75,	-2.77,
	3.01,	1.51,	2.83,
	2.49,	1.61,	2.83,
    };

    static float win[] = {
	1.52,	2.29,	-2.77,
	2.49,	1.85,	-2.77,
	1.52,	2.38,	0.48,
	2.70,	1.51,	2.83,
	-1.52,	2.38,	0.48,
	-2.70,	1.51,	2.83,
	-1.52,	2.29,	-2.77,
	-2.49,	1.85,	-2.77,
    };

    static float rear_deck[] = {
	-1.52,	2.29,	-2.77,
	-1.56,	1.72,	-7.17,
	 1.56,	1.72,	-7.17,
	 1.52,	2.29,	-2.77,
    };
    static float uback[] = {
	-1.56,	0.70,	-7.17,
	 1.56,	0.70,	-7.17,
	 1.56,	1.72,	-7.17,
	-1.56,	1.72,	-7.17,
    };
    static float lbrakearea[] = {
	-1.56,	0.70,	-7.17,
	-1.56,	1.72,	-7.17,
	-2.76,	1.42,	-7.17,
	-2.75,	1.00,	-7.17,
    };
    static float rbrakearea[] = {
	 1.56,	0.70,	-7.17,
	 2.75,	1.00,	-7.17,
	 2.76,	1.42,	-7.17,
	 1.56,	1.72,	-7.17,
    };
    static float lback[] = {
	 2.75,	0.23,	-6.15,
	 2.75,	1.00,	-7.17,
	 1.56,	0.23,	-6.00,
	 1.56,	0.70,	-7.17,
	-1.56,	0.23,	-6.00,
	-1.56,	0.70,	-7.17,
	-2.75,	0.23,	-6.15,
	-2.75,	1.00,	-7.17,
    };

    static float fltire[] = {
	-3.1+TIRE_WIDTH,	HUB_Y,			FHUB_Z,
	-3.1+TIRE_WIDTH,	HUB_Y-TIRE_RADIUS,	FHUB_Z,
	-3.1,			HUB_Y-TIRE_RADIUS,	FHUB_Z,
	-3.1,			HUB_Y-TIRE_RADIUS*0.5,	FHUB_Z,
    };
    static float frtire[] = {
	 3.1-TIRE_WIDTH,	HUB_Y,			FHUB_Z,
	 3.1-TIRE_WIDTH,	HUB_Y-TIRE_RADIUS,	FHUB_Z,
	 3.1,			HUB_Y-TIRE_RADIUS,	FHUB_Z,
	 3.1,			HUB_Y-TIRE_RADIUS*0.5,	FHUB_Z,
    };
    static float bltire[] = {
	-3.1+TIRE_WIDTH,	HUB_Y,			BHUB_Z,
	-3.1+TIRE_WIDTH,	HUB_Y-TIRE_RADIUS,	BHUB_Z,
	-3.1,			HUB_Y-TIRE_RADIUS,	BHUB_Z,
	-3.1,			HUB_Y-TIRE_RADIUS*0.5,	BHUB_Z,
    };
    static float brtire[] = {
	 3.1-TIRE_WIDTH,	HUB_Y,			BHUB_Z,
	 3.1-TIRE_WIDTH,	HUB_Y-TIRE_RADIUS,	BHUB_Z,
	 3.1,			HUB_Y-TIRE_RADIUS,	BHUB_Z,
	 3.1,			HUB_Y-TIRE_RADIUS*0.5,	BHUB_Z,
    };

    static float flhubcap[] = {
	-3.1,	HUB_Y-TIRE_RADIUS*0.5,	FHUB_Z,
	-3.1,	HUB_Y,			FHUB_Z,
    };
    static float frhubcap[] = {
	 3.1,	HUB_Y-TIRE_RADIUS*0.5,	FHUB_Z,
	 3.1,	HUB_Y,			FHUB_Z,
    };
    static float blhubcap[] = {
	-3.1,	HUB_Y-TIRE_RADIUS*0.5,	BHUB_Z,
	-3.1,	HUB_Y,			BHUB_Z,
    };
    static float brhubcap[] = {
	 3.1,	HUB_Y-TIRE_RADIUS*0.5,	BHUB_Z,
	 3.1,	HUB_Y,			BHUB_Z,
    };

    static float underside[] = {
	 2.81,			0.00,	5.88,
	 3.00-TIRE_WIDTH*1.1,	0.23,	-6.0,
	-3.00+TIRE_WIDTH*1.1,	0.23,	-6.0,
	-2.81,			0.00,	5.88,
    };


    BODY_COLOR(img_fildes);
    quadrilateral_mesh(img_fildes,hood,2,4,NULL);

    polygon3d(img_fildes,lside_top,NUMPTS(lside_top),NULL);
    vertex_format(img_fildes,0,0,0,0,CLOCKWISE);
    triangular_strip(img_fildes,lside_mid,NUMPTS(lside_mid),NULL);
    vertex_format(img_fildes,0,0,0,0,COUNTER_CLOCKWISE);
    polygon3d(img_fildes,lrear,NUMPTS(lrear),NULL);

    polygon3d(img_fildes,rside_top,NUMPTS(rside_top),NULL);
    triangular_strip(img_fildes,rside_mid,NUMPTS(rside_mid),NULL);
    polygon3d(img_fildes,rrear,NUMPTS(rrear),NULL);

    polygon3d(img_fildes,rear_deck,NUMPTS(rear_deck),NULL);

    polygon3d(img_fildes,top,NUMPTS(top),NULL);

    polygon3d(img_fildes,uback,NUMPTS(uback),NULL);
    quadrilateral_mesh(img_fildes,lback,4,2,NULL);

    GREY_GLASS(img_fildes);
    surface_model(img_fildes,TRUE,2,1.0,1.0,1.0);
    quadrilateral_mesh(img_fildes,win,4,2,NULL);
    RESTORE_DEFAULT_SURFACE_MODEL(img_fildes);

#define MID_WHEEL_FACETS 6

    RUBBER(img_fildes);
    mesh_surface_of_revolution(img_fildes,fltire,NUMPTS(fltire),
	MID_WHEEL_FACETS,FALSE,
	-3.1,HUB_Y,FHUB_Z, -4.1,HUB_Y,FHUB_Z);
    mesh_surface_of_revolution(img_fildes,frtire,NUMPTS(frtire),
	MID_WHEEL_FACETS,FALSE,
	 3.1,HUB_Y,FHUB_Z, 4.1,HUB_Y,FHUB_Z);
    mesh_surface_of_revolution(img_fildes,bltire,NUMPTS(bltire),
	MID_WHEEL_FACETS,FALSE,
	-3.1,HUB_Y,BHUB_Z, -4.1,HUB_Y,BHUB_Z);
    mesh_surface_of_revolution(img_fildes,brtire,NUMPTS(brtire),
	MID_WHEEL_FACETS,FALSE,
	 3.1,HUB_Y,BHUB_Z, 4.1,HUB_Y,BHUB_Z);

    CHROME(img_fildes);
    /* No specular for max speed */
    mesh_surface_of_revolution(img_fildes,flhubcap,NUMPTS(flhubcap),
	MID_WHEEL_FACETS,FALSE,
	-3.1,HUB_Y,FHUB_Z, -4.1,HUB_Y,FHUB_Z);
    mesh_surface_of_revolution(img_fildes,frhubcap,NUMPTS(frhubcap),
	MID_WHEEL_FACETS,FALSE,
	 3.1,HUB_Y,FHUB_Z, 4.1,HUB_Y,FHUB_Z);
    mesh_surface_of_revolution(img_fildes,blhubcap,NUMPTS(blhubcap),
	MID_WHEEL_FACETS,FALSE,
	-3.1,HUB_Y,BHUB_Z, -4.1,HUB_Y,BHUB_Z);
    mesh_surface_of_revolution(img_fildes,brhubcap,NUMPTS(brhubcap),
	MID_WHEEL_FACETS,FALSE,
	 3.1,HUB_Y,BHUB_Z, 4.1,HUB_Y,BHUB_Z);

    RED_PLASTIC(img_fildes);
    /* no specular for max speed */
    polygon3d(img_fildes,lbrakearea,NUMPTS(lbrakearea),NULL);
    polygon3d(img_fildes,rbrakearea,NUMPTS(rbrakearea),NULL);

    BLACK_PLASTIC(img_fildes);
    polygon3d(img_fildes,underside,NUMPTS(underside),NULL);

    /* No need to RESTORE_DEFAULT_VERTEX_FORMAT, since last was default. */
}


static void low_res_model(
    void)
{
    static float body[] = {
	-2.75,	1.00,	-7.17,
	-2.75,	0.00,	-6.00,
	 2.75,	1.00,	-7.17,
	 2.75,	0.00,	-6.00,
	 3.01,	1.51,	2.83,
	 2.60,	0.00,	6.43,
	-3.01,	1.51,	2.83,
	-2.60,	0.00,	6.43,
	-2.75,	1.00,	-7.17,
	-2.75,	0.00,	-6.00,
    };
    static float top[] = {
	-1.52,	2.29,	-2.77,
	1.52,	2.29,	-2.77,
	1.52,	2.38,	0.48,
	-1.52,	2.38,	0.48,
    };

    static float win[] = {
	1.52,	2.29,	-2.77,
	2.85,	1.20,	-2.77,
	1.52,	2.38,	0.48,
	3.01,	1.51,	2.83,
	-1.52,	2.38,	0.48,
	-3.01,	1.51,	2.83,
	-1.52,	2.29,	-2.77,
	-2.85,	1.20,	-2.77,
    };
    static float rear_deck[] = {
	-2.75,	1.00,	-7.17,
	 2.75,	1.00,	-7.17,
	-2.85,	1.20,	-2.77,
	 2.85,	1.20,	-2.77,
	-1.52,	2.29,	-2.77,
	 1.52,	2.29,	-2.77,
    };

    static float fltire[] = {
	-3.05,	HUB_Y-TIRE_RADIUS,	FHUB_Z,
	-3.05,	HUB_Y,			FHUB_Z-TIRE_RADIUS,
	-3.05,	HUB_Y+TIRE_RADIUS,	FHUB_Z,
	-3.05,	HUB_Y,			FHUB_Z+TIRE_RADIUS,
    };
    static float frtire[] = {
	 3.05,	HUB_Y,			FHUB_Z+TIRE_RADIUS,
	 3.05,	HUB_Y+TIRE_RADIUS,	FHUB_Z,
	 3.05,	HUB_Y,			FHUB_Z-TIRE_RADIUS,
	 3.05,	HUB_Y-TIRE_RADIUS,	FHUB_Z,
    };
    static float bltire[] = {
	-3.05,	HUB_Y-TIRE_RADIUS,	BHUB_Z,
	-3.05,	HUB_Y,			BHUB_Z-TIRE_RADIUS,
	-3.05,	HUB_Y+TIRE_RADIUS,	BHUB_Z,
	-3.05,	HUB_Y,			BHUB_Z+TIRE_RADIUS,
    };
    static float brtire[] = {
	 3.05,	HUB_Y,			BHUB_Z+TIRE_RADIUS,
	 3.05,	HUB_Y+TIRE_RADIUS,	BHUB_Z,
	 3.05,	HUB_Y,			BHUB_Z-TIRE_RADIUS,
	 3.05,	HUB_Y-TIRE_RADIUS,	BHUB_Z,
    };

    static float underside[] = {
	-2.60,	0.00,	6.43,
	 2.60,	0.00,	6.43,
	 2.75,	0.00,	-6.00,
	-2.75,	0.00,	-6.00,
    };


    BODY_COLOR(img_fildes);
    quadrilateral_mesh(img_fildes,body,5,2,NULL);
    polygon3d(img_fildes,top,NUMPTS(top),NULL);
    quadrilateral_mesh(img_fildes,rear_deck,3,2,NULL);

    GREY_GLASS(img_fildes);
    surface_model(img_fildes,TRUE,2,1.0,1.0,1.0);
    quadrilateral_mesh(img_fildes,win,4,2,NULL);
    RESTORE_DEFAULT_SURFACE_MODEL(img_fildes);

    RUBBER(img_fildes);
    polygon3d(img_fildes,underside,NUMPTS(underside),NULL);
    polygon3d(img_fildes,fltire,NUMPTS(fltire),NULL);
    polygon3d(img_fildes,frtire,NUMPTS(frtire),NULL);
    polygon3d(img_fildes,bltire,NUMPTS(bltire),NULL);
    polygon3d(img_fildes,brtire,NUMPTS(brtire),NULL);
}
#endif /* !HOVERWARE_MODEL */


static void create_countach_graphics(
    DRIVE_OBJECT *obj)
{
#if defined(HOVERWARE_MODEL)
    hwObject
	*objects;
    hwInt32
	numObjects;

    numObjects = hwParseFile( "objects/racecar.hw", &objects );
    obj->display_list = createHwSegmentFromObj( objects, numObjects );
#else
    int CountachSeg;
    int HighResSeg;
    int MidResSeg;
    int LowResSeg;

    float mc_extent[2][3];


    mc_extent[0][0] = -3.0 ;
    mc_extent[0][1] = -1.5;
    mc_extent[0][2] = -7.17;
    mc_extent[1][0] = 2.5 ;
    mc_extent[1][1] = 3.5;
    mc_extent[1][2] = 6.43;



    CountachSeg = get_dl_segment();
    HighResSeg = get_dl_segment();
    MidResSeg = get_dl_segment();
    LowResSeg = get_dl_segment();

    obj->display_list = CountachSeg;

    open_segment(img_fildes,HighResSeg,FALSE,FALSE);
      high_res_model();
    close_segment(img_fildes);

    open_segment(img_fildes,MidResSeg,FALSE,FALSE);
      mid_res_model();
    close_segment(img_fildes);

    open_segment(img_fildes,LowResSeg,FALSE,FALSE);
      low_res_model();
    close_segment(img_fildes);

    open_segment(img_fildes,CountachSeg,FALSE,FALSE);
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
#endif
}


void init_countach_object(
    DRIVE_OBJECT *obj)
{
    DRIVE_OBJECT *child;

    if(debug) printf(" inside init_countach_object() routine \n");

    obj->num_children = 0;

    if (countach_dl == -1) {
    	create_countach_graphics(obj);
	countach_dl = obj->display_list;
	shadow_dl = create_shadow_dl();
    }
    else {
	obj->display_list = countach_dl;
    }

    if (obj->color[0] == DEFAULT_OBJECT_COLOR) obj->color[0] = DEFAULT_RED;
    if (obj->color[1] == DEFAULT_OBJECT_COLOR) obj->color[1] = DEFAULT_GRN;
    if (obj->color[2] == DEFAULT_OBJECT_COLOR) obj->color[2] = DEFAULT_BLU;

    if ((obj->vehicle_auxdata = init_vaux(obj)) == NULL) {
	return;
    }
    init_pobj(obj,2000.0,75.0);
    obj->upd = (UpdateDisp *) malloc( sizeof(UpdateDisp) );
    obj->upd->color[0] = obj->color[0];
    obj->upd->color[1] = obj->color[1];
    obj->upd->color[2] = obj->color[2];
    obj->upd->invis_words = 1;
    /* These are things you want to draw */
    obj->upd->invis[0] = LIGHTS_OFF_BRAKES_ON|BACKUP_LIGHTS_OFF|WHEELS_CENTER;

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

    obj->surface_chars_xyz  = countach_surface_chars_xyz;
    obj->surface_chars_bbox = countach_surface_chars_bbox;

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
    obj->fire_point[1] = 2.0;
    obj->fire_point[2] = 6.43+1.0;
}
