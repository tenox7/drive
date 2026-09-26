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


/* Code Module for the police car object */


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

#define INCH(x)	((x)/12.0)

#define TIRE_RADIUS	INCH(14.0)
#define TIRE_WIDTH	INCH(10.0)

/* X values. */
#define BUMPER_X	INCH(34.0)
#define MID_X		INCH(32.0)
#define DOOR_X		INCH(30.0)
#define HIGH_X		INCH(22.0)
#define WINDOW_X	INCH(25.0)

/* Y values. */
#define MIN_Y		-INCH(7.0)
#define HUB_Y 		0.0
#define BUMPER_Y	INCH(8.0)
#define TIRE_Y		TIRE_RADIUS+INCH(1.0)
#define MID_Y		INCH(16.0)
#define DOOR_Y		INCH(22.0)
#define MAX_Y		INCH(40.0)
#define WINDOW_Y	INCH(37.0)

/* Z values. */
#define MIN_Z		-INCH(100.0)
#define BHUB_Z		-INCH(54.0)
#define BBHUB_Z		(BHUB_Z - TIRE_RADIUS)
#define FBHUB_Z		(BHUB_Z + TIRE_RADIUS)
#define MID_Z		0.0
#define FHUB_Z		INCH(56.0)
#define BFHUB_Z		(FHUB_Z - TIRE_RADIUS)
#define FFHUB_Z		(FHUB_Z + TIRE_RADIUS)
#define MAX_Z		INCH(100.0)

static int police_dl = -1;
static int shadow_dl = -1;

static int police_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = INCH(40.0);
    get_box_mc_normal(obj,x,y,z,sc->mc_normal);
    return(TRUE);
}


static int police_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = INCH(40.0);
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
    vaux->bbox_mc[PT_UFL][0] = -BUMPER_X;
            vaux->bbox_mc[PT_UFL][1] = -TIRE_RADIUS;
	    vaux->bbox_mc[PT_UFL][2] =  MAX_Z;
    vaux->bbox_mc[PT_UFR][0] =  BUMPER_X;
	    vaux->bbox_mc[PT_UFR][1] = -TIRE_RADIUS;
	    vaux->bbox_mc[PT_UFR][2] =  MAX_Z;
    vaux->bbox_mc[PT_UBR][0] =  BUMPER_X;
	    vaux->bbox_mc[PT_UBR][1] = -TIRE_RADIUS;
	    vaux->bbox_mc[PT_UBR][2] =  MIN_Z;
    vaux->bbox_mc[PT_UBL][0] = -BUMPER_X;
	    vaux->bbox_mc[PT_UBL][1] = -TIRE_RADIUS;
	    vaux->bbox_mc[PT_UBL][2] =  MIN_Z;
    vaux->bbox_mc[PT_TFL][0] = -BUMPER_X;
	    vaux->bbox_mc[PT_TFL][1] =  MAX_Y;
	    vaux->bbox_mc[PT_TFL][2] =  MAX_Z;
    vaux->bbox_mc[PT_TFR][0] =  BUMPER_X;
	    vaux->bbox_mc[PT_TFR][1] =  MAX_Y;
	    vaux->bbox_mc[PT_TFR][2] =  MAX_Z;
    vaux->bbox_mc[PT_TBR][0] =  BUMPER_X;
	    vaux->bbox_mc[PT_TBR][1] =  MAX_Y;
	    vaux->bbox_mc[PT_TBR][2] =  MIN_Z;
    vaux->bbox_mc[PT_TBL][0] = -BUMPER_X;
	    vaux->bbox_mc[PT_TBL][1] =  MAX_Y;
	    vaux->bbox_mc[PT_TBL][2] =  MIN_Z;
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

    vaux->horsepower          = 205.0;
    vaux->peak_power_rpm      = 3550.0;
    vaux->two_peak_power      = 0.0;
    vaux->gear_best_speed[0]  = MPH_TO_FPS(15.0);	/* reverse */
    vaux->gear_best_speed[1]  = 0.0;			/* neutral */
    vaux->gear_best_speed[2]  = MPH_TO_FPS(20.0);	/* 1st */
    vaux->gear_best_speed[3]  = MPH_TO_FPS(45.0);	/* 2nd */
    vaux->gear_best_speed[4]  = MPH_TO_FPS(70.0);	/* 3rd */
    vaux->gear_best_speed[5]  = MPH_TO_FPS(100.0);	/* 4th */
    vaux->gear_best_speed[6]  = MPH_TO_FPS(130.0);	/* 5th */
    vaux->offroad_performance = 0.25;
    vaux->thrust_mechanism    = REAR_WHEEL_DRIVE;

    vaux->left_gauge_class  = GAUGE_ANALOG_MPH;
    vaux->right_gauge_class = GAUGE_ANALOG_RPM;
    vaux->max_speed         = 150.0;
    vaux->max_rpm           = 8000.0;
    vaux->max_altitude      = 25000.0;

    return(vaux);
}



#define BODY_COLOR(fildes) \
	surface_model((fildes),TRUE,4,1.0,1.0,1.0); 

#define TOP_FL0	-WINDOW_X,		WINDOW_Y,		0.83
#define TOP_BL0	-WINDOW_X,		WINDOW_Y,		-2.33

#define TOP_FL1	-HIGH_X,		MAX_Y-INCH(1.0),	1.00
#define TOP_BL1	-HIGH_X,		MAX_Y-INCH(1.0), 	-INCH(40.0)

#define TOP_FL2	-1.00,			MAX_Y,			1.17
#define TOP_BL2	-1.00,			MAX_Y,		 	-INCH(46.0)

#define TOP_FR2	 1.00,			MAX_Y,			1.17
#define TOP_BR2	 1.00,			MAX_Y,		 	-INCH(46.0)

#define TOP_FR1	 HIGH_X,		MAX_Y-INCH(1.0),	1.00
#define TOP_BR1	 HIGH_X,		MAX_Y-INCH(1.0), 	-INCH(40.0)

#define TOP_FR0	 WINDOW_X,		WINDOW_Y,		0.83
#define TOP_BR0	 WINDOW_X,		WINDOW_Y,		-2.33


static int create_shadow_dl(
    void)
{

    /******************************************************************
     * Doors
     *
     */
    static float side_right[] =
    {
	BUMPER_X,	HUB_Y,		-INCH(95.0),
	BUMPER_X,	HUB_Y,		INCH(95.0),
	DOOR_X, 	DOOR_Y,		INCH(95.0),
	DOOR_X, 	DOOR_Y,		-INCH(95.0),
    };

    static float side_left[] =
    {
	-DOOR_X, 	DOOR_Y,		-INCH(95.0),
	-DOOR_X, 	DOOR_Y,		INCH(95.0),
	-BUMPER_X,	HUB_Y,		INCH(95.0),
	-BUMPER_X,	HUB_Y,		-INCH(95.0),
    };


    /******************************************************************
     * Top
     *
     */
    static float top[] = { TOP_FL1, TOP_BL1, TOP_BR1, TOP_FR1 };


    /******************************************************************
     * Windows
     *
     */
    static float rear_window[] =
    {
	DOOR_X,		DOOR_Y, -INCH(52.0),
	TOP_BR1,			
	TOP_BL1,			
	-DOOR_X,	DOOR_Y, -INCH(52.0),
    };


    static float front_window[] =
    {
	-DOOR_X, DOOR_Y,	INCH(34.0),
	TOP_FL1,			
	TOP_FR1,			
	DOOR_X,	 DOOR_Y,	INCH(34.0),	
    };


    static float right_window[] = {
	TOP_BR1,
	DOOR_X,		DOOR_Y,			-INCH(52.0),
	DOOR_X,		DOOR_Y,			INCH(34.0),
	TOP_FR1,
    };

    static float left_window[] = {
	TOP_FL1,
	-DOOR_X,	DOOR_Y,			INCH(34.0),
	-DOOR_X,	DOOR_Y,			-INCH(52.0),
	TOP_BL1,
    };

    /******************************************************************
     * Trunk and hood
     *
     */
    static float hood[] =
    {
	DOOR_X,		DOOR_Y,		INCH(34.0),
	DOOR_X, 	DOOR_Y,		INCH(95.0),	 
	-DOOR_X, 	DOOR_Y,		INCH(95.0),	 
	-DOOR_X,	DOOR_Y,		INCH(34.0),
    };


    static float trunk[] =
    {
	-DOOR_X,		DOOR_Y, 	-INCH(52.0),
	-DOOR_X,		DOOR_Y,		-INCH(95.0),
	DOOR_X,			DOOR_Y,		-INCH(95.0),
	DOOR_X,			DOOR_Y,		-INCH(52.0),
    };


    /******************************************************************
     * Front and rear.
     *
     */
    static float front[] =
    {
	DOOR_X,		DOOR_Y,	    INCH(95.0),
	BUMPER_X,	HUB_Y,	    INCH(95.0),
	-BUMPER_X,	HUB_Y,	    INCH(95.0),
	-DOOR_X,	DOOR_Y,	    INCH(95.0),
    };

    static float rear[] =
    {
	-DOOR_X, 	DOOR_Y,		-INCH(95.0),
	-BUMPER_X,	HUB_Y,		-INCH(95.0),
	BUMPER_X,	HUB_Y,		-INCH(95.0),
	DOOR_X, 	DOOR_Y,		-INCH(95.0),
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
	polygon3d( img_fildes, (float *) top, NUMPTS( top ), NULL );
	polygon3d( img_fildes, (float *) side_right, NUMPTS( side_right ), NULL );
	polygon3d( img_fildes, (float *) side_left, NUMPTS( side_left ), NULL );
	polygon3d( img_fildes, (float *) front, NUMPTS( front ), NULL );
	polygon3d( img_fildes, (float *) rear, NUMPTS( rear ), NULL );
	polygon3d( img_fildes, (float *) hood, NUMPTS( hood ), NULL );
	polygon3d( img_fildes, (float *) trunk, NUMPTS( trunk ), NULL );

	polygon3d( img_fildes, (float *) front_window, NUMPTS( front_window ), NULL );
	polygon3d( img_fildes, (float *) rear_window,  NUMPTS( rear_window ),  NULL );
	polygon3d( img_fildes, (float *) left_window,  NUMPTS( left_window ),  NULL);
	polygon3d( img_fildes, (float *) right_window, NUMPTS( right_window ), NULL);
	TRANSPARENT_OFF(img_fildes);
    close_segment(img_fildes);


    open_segment(img_fildes,seg,TRUE,FALSE);
	set_extent(img_fildes,mc_extent);
	cond_execute_segment(img_fildes,CI_PRUNE,FALSE,graphics_seg);
    close_segment(img_fildes);

    return(seg);

}


static void create_police_graphics(
    DRIVE_OBJECT *obj)
{
    hwObject
	*objects;
    int
	numObjects;

    numObjects = hwParseFile( "objects/Police.hw", &objects );
    obj->display_list = createHwSegmentFromObj( objects, numObjects );
}


void init_police_object(
    DRIVE_OBJECT *obj)
{
    DRIVE_OBJECT *child;

    if(debug) printf(" inside init_police_object() routine \n");

    if (obj->color[0] == DEFAULT_OBJECT_COLOR) obj->color[0] = DEFAULT_RED;
    if (obj->color[1] == DEFAULT_OBJECT_COLOR) obj->color[1] = DEFAULT_GRN;
    if (obj->color[2] == DEFAULT_OBJECT_COLOR) obj->color[2] = DEFAULT_BLU;

    if (police_dl == -1) {
    	create_police_graphics(obj);
	police_dl = obj->display_list;
	shadow_dl = create_shadow_dl();
    }
    else {
	obj->display_list = police_dl;
    }

    if ((obj->vehicle_auxdata = init_vaux(obj)) == NULL) {
	return;
    }
    init_pobj(obj,2000.0,75.0);
    obj->upd = (UpdateDisp_ptr)malloc( sizeof(UpdateDisp) );
    obj->upd->color[0] = obj->color[0];
    obj->upd->color[1] = obj->color[1];
    obj->upd->color[2] = obj->color[2];
    obj->upd->invis_words = 1;
    obj->upd->invis[0] = LIGHTS_OFF_BRAKES_ON|BACKUP_LIGHTS_OFF;
    obj->spacing = 0.0;

    obj->update_self = apply_car_physics;
    obj->bound_mc[0] = -BUMPER_X;
    obj->bound_mc[1] = -TIRE_RADIUS;
    obj->bound_mc[2] =  MIN_Z;
    obj->bound_mc[3] =  BUMPER_X;
    obj->bound_mc[4] =  MAX_Y;
    obj->bound_mc[5] =  MAX_Z;
    update_wc_bounds(obj);

    /* apply brake */
    obj->controls.pointer_x = 0.0;
    obj->controls.pointer_y = -1.0;

    obj->surface_chars_xyz  = police_surface_chars_xyz;
    obj->surface_chars_bbox = police_surface_chars_bbox;

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
    obj->fire_point[2] = MAX_Z+1.0;
}
