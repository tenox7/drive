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


/* Code Module for the minivan car object */


#include <stdio.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "prims.h"
#include "obj_common.h"
#include "demo_physics.h"

#define DEFAULT_RED	(FLOATRAND(1.0))
#define DEFAULT_GRN	(FLOATRAND(1.0))
#define DEFAULT_BLU	(FLOATRAND(1.0))


#define BODY_COLOR \
	surface_model(img_fildes,TRUE,4,1.0,1.0,1.0); 

#define TIRE_RADIUS	1.2
#define TIRE_WIDTH	0.8
/*
#define HUB_Y (1.0-TIRE_RADIUS)
#define FHUB_Z	(-(2.32+4.78)/2.0)
*/
#define HUB_Y (-1.7)
#define FHUB_Z	(4.80)
#define BHUB_Z	(-4.50)

void _minivan_update_self();
static int minivan_dl = -1;
static int shadow_dl = -1;

static int minivan_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = 3.75;
    get_box_mc_normal(obj,x,y,z,sc->mc_normal);
    return(TRUE);
}


static int minivan_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = 3.75;
    return(TRUE);
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
    vaux->bbox_mc[PT_UFL][0] = 	-3.0;
	    vaux->bbox_mc[PT_UFL][1] = -3.3;
	    vaux->bbox_mc[PT_UFL][2] =  8.0;

    vaux->bbox_mc[PT_UFR][0] =  	 3.0;
	    vaux->bbox_mc[PT_UFR][1] = -3.3;
	    vaux->bbox_mc[PT_UFR][2] =  8.0;

    vaux->bbox_mc[PT_UBR][0] =  	 3.0;
	    vaux->bbox_mc[PT_UBR][1] = -3.3;
	    vaux->bbox_mc[PT_UBR][2] = -7.5;

    vaux->bbox_mc[PT_UBL][0] = 	-3.0;
	    vaux->bbox_mc[PT_UBL][1] = -3.3;
	    vaux->bbox_mc[PT_UBL][2] = -7.5;

    vaux->bbox_mc[PT_TFL][0] = 	-3.0;
	    vaux->bbox_mc[PT_TFL][1] =  3.75;
	    vaux->bbox_mc[PT_TFL][2] =  8.0;

    vaux->bbox_mc[PT_TFR][0] =  	3.0;
	    vaux->bbox_mc[PT_TFR][1] = 3.75;
	    vaux->bbox_mc[PT_TFR][2] =  8.0;

    vaux->bbox_mc[PT_TBR][0] =  	3.0;
	    vaux->bbox_mc[PT_TBR][1] = 3.75;
	    vaux->bbox_mc[PT_TBR][2] = -7.5;

    vaux->bbox_mc[PT_TBL][0] = 	-3.0;
	    vaux->bbox_mc[PT_TBL][1] = 3.75;
	    vaux->bbox_mc[PT_TBL][2] = -7.5;
    vaux->COG_mc[0] =
	    (vaux->bbox_mc[PT_UFR][0] + vaux->bbox_mc[PT_UFL][0]) / 2.0;
    vaux->COG_mc[1] =
	    (vaux->bbox_mc[PT_TFL][1] + vaux->bbox_mc[PT_UFL][1]) / 2.0;
    vaux->COG_mc[2] =
	    (vaux->bbox_mc[PT_UBR][2] + (vaux->bbox_mc[PT_UFR][2] * 2.0)) / 2.0;

    vaux->coefficient_of_drag = 0.030;
    vaux->wheel_torque_mult   = 75.0 /* *pobj->I[2][2] */;
    vaux->best_turn_speed     = MPH_TO_FPS(25.0);
    vaux->max_obstacle_height = 2.0;

    vaux->horsepower          = 150.0;
    vaux->peak_power_rpm      = 3500.0;
    vaux->two_peak_power      = 0.3;
    vaux->gear_best_speed[0]  = MPH_TO_FPS(15.0);	/* reverse */
    vaux->gear_best_speed[1]  = 0.0;			/* neutral */
    vaux->gear_best_speed[2]  = MPH_TO_FPS(15.0);	/* 1st */
    vaux->gear_best_speed[3]  = MPH_TO_FPS(35.0);	/* 2nd */
    vaux->gear_best_speed[4]  = MPH_TO_FPS(55.0);	/* 3rd */
    vaux->gear_best_speed[5]  = MPH_TO_FPS(70.0);	/* 4th */
    vaux->gear_best_speed[6]  = MPH_TO_FPS(100.0);	/* 5th */
    vaux->offroad_performance = 0.5;
    vaux->thrust_mechanism    = REAR_WHEEL_DRIVE;

    vaux->left_gauge_class  = GAUGE_ANALOG_MPH;
    vaux->right_gauge_class = GAUGE_ANALOG_RPM;
    vaux->max_speed         = 100.0;
    vaux->max_rpm           = 8000.0;
    vaux->max_altitude      = 25000.0;

    return(vaux);
}


#define ZF_FRONT	(-1.0)
#define ZF_2		(ZF_FRONT+0.25)

static int create_shadow_dl(
    void)
{
#if 1
    return INVALID;
#else
    static float body[] = {
	 2.85, 1.25, -7.25,
	-2.85, 1.25, -7.25,
	-2.85, 1.25,  5.25,
	 2.85, 1.25,  5.25,
	 2.85, 1.25, -7.25,

	 2.85, -2.0, -7.25,
	-2.85, -2.0, -7.25,
	-2.85, -2.0,  8.0,
	 2.85, -2.0,  8.0,
	 2.85, -2.0, -7.25,
    };
    static float top[] = {
	 2.85, 3.6, -7.25,
	-2.85, 3.6, -7.25,
	-2.85, 3.6, 2.25,
	 2.85, 3.6, 2.25,
	 2.85, 3.6, -7.25,

	 2.85, 1.25, -7.25,
	-2.85, 1.25, -7.25,
	-2.85, 1.25, 5.25,
	 2.85, 1.25, 5.25,
	 2.85, 1.25, -7.25,
    };

    static float lid[] = {
	 2.85, 3.6, 2.25,
	-2.85, 3.6, 2.25,
	-2.85, 3.6, -7.25,
	 2.85, 3.6, -7.25,
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
	quadrilateral_mesh(img_fildes,body,2,5,NULL);
	polygon3d(img_fildes,lid,NUMPTS(lid),NULL);
	quadrilateral_mesh(img_fildes,top,2,5,NULL);
	TRANSPARENT_OFF(img_fildes);
    close_segment(img_fildes);

    open_segment(img_fildes,seg,TRUE,FALSE);
	set_extent(img_fildes,mc_extent);
	cond_execute_segment(img_fildes,CI_PRUNE,FALSE,graphics_seg);
    close_segment(img_fildes);

    return(seg);
#endif
}


static void create_minivan_graphics(
    DRIVE_OBJECT *obj)
{
    hwObject
	*objects;
    int
	numObjects;

    numObjects = hwParseFile( "objects/Minivan.hw", &objects );
    obj->display_list = createHwSegmentFromObj( objects, numObjects );
}


void init_minivan_object(
    DRIVE_OBJECT *obj)
{
    DRIVE_OBJECT *child;

    if(debug) printf(" inside init_minivan_object() routine \n");

    if (obj->color[0] == DEFAULT_OBJECT_COLOR) obj->color[0] = DEFAULT_RED;
    if (obj->color[1] == DEFAULT_OBJECT_COLOR) obj->color[1] = DEFAULT_GRN;
    if (obj->color[2] == DEFAULT_OBJECT_COLOR) obj->color[2] = DEFAULT_BLU;

    if (minivan_dl == -1) {
    	create_minivan_graphics(obj);
	minivan_dl = obj->display_list;
	shadow_dl = create_shadow_dl();
    }
    else {
	obj->display_list = minivan_dl;
    }

    if ((obj->vehicle_auxdata = init_vaux(obj)) == NULL) {
	return;
    }
    init_pobj(obj,2000.0,55.0);
    obj->update_self = apply_car_physics;

    obj->upd = (UpdateDisp_ptr)malloc( sizeof(UpdateDisp) );
    obj->upd->color[0] = obj->color[0];
    obj->upd->color[1] = obj->color[1];
    obj->upd->color[2] = obj->color[2];
    obj->upd->invis_words = 1;
    obj->upd->invis[0] = LIGHTS_OFF_BRAKES_ON|BACKUP_LIGHTS_OFF;

    obj->bound_mc[0] = -3.0;
    obj->bound_mc[1] = -2.9;
    obj->bound_mc[2] = -7.5;
    obj->bound_mc[3] =  3.0;
    obj->bound_mc[4] =  3.75;
    obj->bound_mc[5] =  8.0;
    update_wc_bounds(obj);

    /* apply brake */
    obj->controls.pointer_x = 0.0;
    obj->controls.pointer_y = -1.0;

    obj->surface_chars_xyz  = minivan_surface_chars_xyz;
    obj->surface_chars_bbox = minivan_surface_chars_bbox;

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
    obj->fire_point[2] = 8.0+1.0;
}
