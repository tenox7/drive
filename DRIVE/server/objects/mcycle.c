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


/* Code Module for the mcycle car object */


#include <stdio.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "prims.h"
#include "obj_common.h"
#include "demo_physics.h"

#define DEFAULT_RED	1.0
#define DEFAULT_GRN	1.0
#define DEFAULT_BLU	1.0

#define HIGH_WHEEL_FACETS 12

extern void c_set_cull_size();

#define Z_FTIRE		2.0
#define Z_RTIRE		-3.0
#define TIRE_FACETS	16
#define TIRE_RADIUS	1.0
#define TIRE_SIZE	0.2
#define HUB_RADIUS	0.5
#define HUB_RADIUS2	0.1
#define SEAT_WIDTH	0.5
#define SEAT_THICKNESS	(0.25)
#define YSEAT		(TIRE_RADIUS*2+.25+SEAT_THICKNESS)
#define REARSEAT	(Z_RTIRE+.5)
#define BODY_WIDTH	(SEAT_WIDTH)
#define BODY_OFFGROUND	TIRE_RADIUS
#define GLASSTOP	(YSEAT+1.0)
#define SHZ2		0.75
#define YHEADLIGHT	(TIRE_RADIUS*2+1.0)
#define ZHEADLIGHT	(Z_FTIRE-0.25)
#define BRAKERADIUS	(0.15)


#define SHOULDER	0.5
#define TORSO_THICKNESS	0.75
#define HEADHEIGHT	(GLASSTOP+0.3)
#define HEADRADIUS	0.35
#define HIPZ		(-1.5)
#define ARMRADIUS	0.15
#define LEGRADIUS	0.25
#define ANKLERADIUS	0.1
#define LEGLENGTH	1.2


static int mcycle_dl = -1, shadow_dl = -1;

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
    vaux->bbox_mc[PT_UFL][0] = -BODY_WIDTH;
	    vaux->bbox_mc[PT_UFL][1] = 0.0;
	    vaux->bbox_mc[PT_UFL][2] = Z_FTIRE+TIRE_RADIUS;
    vaux->bbox_mc[PT_UFR][0] =  BODY_WIDTH;
	    vaux->bbox_mc[PT_UFR][1] = 0.0;
	    vaux->bbox_mc[PT_UFR][2] = Z_FTIRE+TIRE_RADIUS;
    vaux->bbox_mc[PT_UBR][0] =  BODY_WIDTH;
	    vaux->bbox_mc[PT_UBR][1] = 0.0;
	    vaux->bbox_mc[PT_UBR][2] = Z_RTIRE-TIRE_RADIUS;
    vaux->bbox_mc[PT_UBL][0] = -BODY_WIDTH;
	    vaux->bbox_mc[PT_UBL][1] = 0.0;
	    vaux->bbox_mc[PT_UBL][2] = Z_RTIRE-TIRE_RADIUS;
    vaux->bbox_mc[PT_TFL][0] = -BODY_WIDTH;
	    vaux->bbox_mc[PT_TFL][1] = GLASSTOP;
	    vaux->bbox_mc[PT_TFL][2] = Z_FTIRE+TIRE_RADIUS;
    vaux->bbox_mc[PT_TFR][0] =  BODY_WIDTH;
	    vaux->bbox_mc[PT_TFR][1] = GLASSTOP;
	    vaux->bbox_mc[PT_TFR][2] = Z_FTIRE+TIRE_RADIUS;
    vaux->bbox_mc[PT_TBR][0] =  BODY_WIDTH;
	    vaux->bbox_mc[PT_TBR][1] = YSEAT;
	    vaux->bbox_mc[PT_TBR][2] = Z_RTIRE-TIRE_RADIUS;
    vaux->bbox_mc[PT_TBL][0] = -BODY_WIDTH;
	    vaux->bbox_mc[PT_TBL][1] = YSEAT;
	    vaux->bbox_mc[PT_TBL][2] = Z_RTIRE-TIRE_RADIUS;
    vaux->COG_mc[0] =
	    (vaux->bbox_mc[PT_UFR][0] + vaux->bbox_mc[PT_UFL][0]) / 2.0;
    vaux->COG_mc[1] =
	    (vaux->bbox_mc[PT_TFL][1] + vaux->bbox_mc[PT_UFL][1]) / 4.0;
    vaux->COG_mc[2] =
	    (vaux->bbox_mc[PT_UBR][2] + vaux->bbox_mc[PT_UFR][2]) / 2.0;

    vaux->coefficient_of_drag = 0.005;
    vaux->wheel_torque_mult   = 75.0 /* *pobj->I[2][2] */;
    vaux->best_turn_speed     = MPH_TO_FPS(25.0);
    vaux->max_obstacle_height = 1.0;

    vaux->horsepower          = 100.0;
    vaux->peak_power_rpm      = 3500.0;
    vaux->two_peak_power      = 0.0;
    vaux->gear_best_speed[0]  = MPH_TO_FPS(5.0);	/* reverse */
    vaux->gear_best_speed[1]  = 0.0;			/* neutral */
    vaux->gear_best_speed[2]  = MPH_TO_FPS(20.0);	/* 1st */
    vaux->gear_best_speed[3]  = MPH_TO_FPS(40.0);	/* 2nd */
    vaux->gear_best_speed[4]  = MPH_TO_FPS(65.0);	/* 3rd */
    vaux->gear_best_speed[5]  = MPH_TO_FPS(90.0);	/* 4th */
    vaux->gear_best_speed[6]  = MPH_TO_FPS(120.0);	/* 5th */
    vaux->offroad_performance = 0.7;
    vaux->thrust_mechanism    = MOTORCYCLE;

    vaux->left_gauge_class  = GAUGE_ANALOG_MPH;
    vaux->right_gauge_class = GAUGE_ANALOG_RPM;
    vaux->max_speed         = 150.0;
    vaux->max_rpm           = 8000.0;
    vaux->max_altitude      = 25000.0;

    return(vaux);
}


static int mcycle_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = 2.38;
    get_box_mc_normal(obj,x,y,z,sc->mc_normal);
    return(TRUE);
}


static int mcycle_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = 2.38;
    return(TRUE);
}

static void create_mcycle_graphics(
    DRIVE_OBJECT *obj)
{
    hwObject
	*objects;
    int
	numObjects;

    numObjects = hwParseFile( "objects/mcycle.hw", &objects );
    obj->display_list = createHwSegmentFromObj( objects, numObjects );
}


#define SHADOW_MARGIN 0.4
static int create_shadow_dl(
    void)
{
#if 0
    static float ftireshadow[] = {
	-TIRE_SIZE/2,	TIRE_RADIUS,	Z_FTIRE-TIRE_RADIUS,
	 TIRE_SIZE/2,	TIRE_RADIUS,	Z_FTIRE-TIRE_RADIUS,
	 TIRE_SIZE/2,	TIRE_RADIUS,	Z_FTIRE+TIRE_RADIUS,
	-TIRE_SIZE/2,	TIRE_RADIUS,	Z_FTIRE+TIRE_RADIUS,
    };
    static float rtireshadow[] = {
	-TIRE_SIZE/2,	TIRE_RADIUS,	Z_RTIRE-TIRE_RADIUS,
	 TIRE_SIZE/2,	TIRE_RADIUS,	Z_RTIRE-TIRE_RADIUS,
	 TIRE_SIZE/2,	TIRE_RADIUS,	Z_RTIRE+TIRE_RADIUS,
	-TIRE_SIZE/2,	TIRE_RADIUS,	Z_RTIRE+TIRE_RADIUS,
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
	motorcycle_low(img_fildes,FALSE);
	polygon3d(img_fildes,ftireshadow,4,FALSE);
	polygon3d(img_fildes,rtireshadow,4,FALSE);
	TRANSPARENT_OFF(img_fildes);
    close_segment(img_fildes);

    open_segment(img_fildes,seg,TRUE,FALSE);
	set_extent(img_fildes,mc_extent);
	cond_execute_segment(img_fildes,CI_PRUNE,FALSE,graphics_seg);
    close_segment(img_fildes);

    return(seg);
#else
    return INVALID;
#endif
}

void init_mcycle_object(
    DRIVE_OBJECT *obj)
{
    DRIVE_OBJECT *child;

    if (obj->color[0] == DEFAULT_OBJECT_COLOR) obj->color[0] = DEFAULT_RED;
    if (obj->color[1] == DEFAULT_OBJECT_COLOR) obj->color[1] = DEFAULT_GRN;
    if (obj->color[2] == DEFAULT_OBJECT_COLOR) obj->color[2] = DEFAULT_BLU;

    if(debug) printf(" inside init_mcycle_object() routine \n");

    obj->num_children = 0;

    if (mcycle_dl == -1) {
    	create_mcycle_graphics(obj);
	mcycle_dl = obj->display_list;
	shadow_dl = create_shadow_dl();
    }
    else {
	obj->display_list = mcycle_dl;
    }

    if ((obj->vehicle_auxdata = init_vaux(obj)) == NULL) {
	return;
    }
    init_pobj(obj,500.0,75.0);
    obj->upd = (UpdateDisp_ptr)malloc( sizeof(UpdateDisp) );
    obj->upd->color[0] = obj->color[0];
    obj->upd->color[1] = obj->color[1];
    obj->upd->color[2] = obj->color[2];
    obj->upd->invis_words = 1;
    /* These are things you want to draw */
    obj->upd->invis[0] = LIGHTS_OFF_BRAKES_ON|BACKUP_LIGHTS_OFF|WHEELS_CENTER;

    obj->update_self = apply_car_physics;
    obj->bound_mc[0] = -BODY_WIDTH;
    obj->bound_mc[1] =  0.0;
    obj->bound_mc[2] =  Z_RTIRE-TIRE_RADIUS;
    obj->bound_mc[3] =  BODY_WIDTH;
    obj->bound_mc[4] =  GLASSTOP;
    obj->bound_mc[5] =  Z_FTIRE+TIRE_RADIUS;
    update_wc_bounds(obj);

    /* apply brake */
    obj->controls.pointer_x = 0.0;
    obj->controls.pointer_y = -1.0;

    obj->surface_chars_xyz  = mcycle_surface_chars_xyz;
    obj->surface_chars_bbox = mcycle_surface_chars_bbox;

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
