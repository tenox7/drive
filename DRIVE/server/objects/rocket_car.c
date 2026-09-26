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


/* Code Module for the rocket_car car object */


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

static int invis[10];
static int rocket_car_dl = -1, shadow_dl = -1;

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
    vaux->bbox_mc[PT_UFL][0] = -2.0;
	    vaux->bbox_mc[PT_UFL][1] =  0.0;
	    vaux->bbox_mc[PT_UFL][2] = 10.0;
    vaux->bbox_mc[PT_UFR][0] =  2.0;
	    vaux->bbox_mc[PT_UFR][1] =  0.0;
	    vaux->bbox_mc[PT_UFR][2] = 10.0;
    vaux->bbox_mc[PT_UBR][0] =  2.0;
	    vaux->bbox_mc[PT_UBR][1] =  0.0;
	    vaux->bbox_mc[PT_UBR][2] = -7.0;
    vaux->bbox_mc[PT_UBL][0] = -2.0;
	    vaux->bbox_mc[PT_UBL][1] =  0.0;
	    vaux->bbox_mc[PT_UBL][2] = -7.0;
    vaux->bbox_mc[PT_TFL][0] = -2.0;
	    vaux->bbox_mc[PT_TFL][1] =  3.0;
	    vaux->bbox_mc[PT_TFL][2] = 10.0;
    vaux->bbox_mc[PT_TFR][0] =  2.0;
	    vaux->bbox_mc[PT_TFR][1] =  3.0;
	    vaux->bbox_mc[PT_TFR][2] = 10.0;
    vaux->bbox_mc[PT_TBR][0] =  2.0;
	    vaux->bbox_mc[PT_TBR][1] =  3.0;
	    vaux->bbox_mc[PT_TBR][2] = -7.0;
    vaux->bbox_mc[PT_TBL][0] = -2.0;
	    vaux->bbox_mc[PT_TBL][1] =  3.0;
	    vaux->bbox_mc[PT_TBL][2] = -7.0;
    vaux->COG_mc[0] =
	    (vaux->bbox_mc[PT_UFR][0] + vaux->bbox_mc[PT_UFL][0]) / 2.0;
    vaux->COG_mc[1] =
	    (vaux->bbox_mc[PT_TFL][1] + vaux->bbox_mc[PT_UFL][1]) / 2.0;
    vaux->COG_mc[2] =
	    (vaux->bbox_mc[PT_UBR][2] + vaux->bbox_mc[PT_UFR][2]) / 2.0;

    vaux->coefficient_of_drag = 0.003;
    vaux->wheel_torque_mult   = 50.0 /* *pobj->I[2][2] */;
    vaux->best_turn_speed     = MPH_TO_FPS(200.0);
    vaux->max_obstacle_height = 1.0;

    vaux->horsepower          = 2000.0;
    vaux->peak_power_rpm      = 3500.0;
    vaux->two_peak_power      = 0.0;
    vaux->gear_best_speed[0]  = MPH_TO_FPS(15.0);	/* reverse */
    vaux->gear_best_speed[1]  = 0.0;			/* neutral */
    vaux->gear_best_speed[2]  = MPH_TO_FPS(50.0);	/* 1st */
    vaux->gear_best_speed[3]  = MPH_TO_FPS(100.0);	/* 2nd */
    vaux->gear_best_speed[4]  = MPH_TO_FPS(200.0);	/* 3rd */
    vaux->gear_best_speed[5]  = MPH_TO_FPS(400.0);	/* 4th */
    vaux->gear_best_speed[6]  = MPH_TO_FPS(600.0);	/* 5th */
    vaux->offroad_performance = 0.0;
    vaux->thrust_mechanism    = BINARY_THRUSTER;

    vaux->left_gauge_class  = GAUGE_ANALOG_MPH;
    vaux->right_gauge_class = GAUGE_ANALOG_RPM;
    vaux->max_speed         = 1000.0;
    vaux->max_rpm           = 8000.0;
    vaux->max_altitude      = 25000.0;

    return(vaux);
}


static int rocket_car_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = 2.38;
    get_box_mc_normal(obj,x,y,z,sc->mc_normal);
    return(TRUE);
}


static int rocket_car_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = 2.38;
    return(TRUE);
}




#define BODY_FACETS	8
#define TIRE_FACETS	12
static void draw_car(
    boolean_type shadow)
{
    static float body[] = {
	0.0,	2.8,	-3.0,	0.0,	COS0,	SIN0,
	0.0,	2.7,	7.0,	0.0,	COS0,	SIN0,
	0.0,	2.5,	8.0,	0.0,	COS15,	SIN15,
	0.0,	2.2,	9.0,	0.0,	COS30,	SIN30,
	0.0,	1.5,	10.0,	0.0,	COS60,	SIN60,
    };
    static float flame[] = {
	0.0,	1.5,	-7.0,	0.0,	COS0,	SIN0,
	0.0,	1.8,	-6.0,	0.0,	COS30,	-SIN30,
	0.0,	2.5,	-5.0,	0.0,	COS0,	SIN0,
	0.0, 	2.0,	-4.0,	0.0,	COS30,	SIN30,
    };
    static float upfin1[] = {
	0.0, 2.8, -1.0,
	0.0, 4.0, -3.0,
	0.0, 2.8, -3.0,
    };
    static float upfin2[] = {
	0.0, 2.8, -3.0,
	0.0, 4.0, -3.0,
	0.0, 2.8, -1.0,
    };
    static float hfin1[] = {
	 0.0, 1.5,  3.0,
	 3.0, 1.5, -3.0,
	-3.0, 1.5, -3.0,
    };
    static float hfin2[] = {
	-3.0, 1.5, -3.0,
	 3.0, 1.5, -3.0,
	 0.0, 1.5,  3.0,
    };

    if (shadow) {
	SHADOW(img_fildes);
	TRANSPARENT_ON(img_fildes,0.5);
    }
    mesh_surface_of_revolution(img_fildes,body,sizeof(body)/sizeof(float)/6,
	BODY_FACETS, TRUE, 0.0,1.5,0.0,	0.0,1.5,100.0);
    circle(img_fildes,2.8-1.5,BODY_FACETS*2,FALSE, 0.0,1.5,-3.0, 0.0,1.5,4.0);
    polygon3d(img_fildes,upfin1,sizeof(upfin1)/sizeof(float)/3,FALSE);
    polygon3d(img_fildes,upfin2,sizeof(upfin2)/sizeof(float)/3,FALSE);
    polygon3d(img_fildes,hfin1, sizeof(hfin1) /sizeof(float)/3,FALSE);
    polygon3d(img_fildes,hfin2, sizeof(hfin2) /sizeof(float)/3,FALSE);


    /* Tires */
    if (!shadow) RUBBER(img_fildes);
    mesh_cone(img_fildes,1.0,1.0,TRUE,TRUE,TIRE_FACETS,
	0.8,1.0,-2.0, 1.0,1.0,-2.0);
    mesh_cone(img_fildes,1.0,1.0,TRUE,TRUE,TIRE_FACETS,
	-0.8,1.0,-2.0, -1.0,1.0,-2.0);
    mesh_cone(img_fildes,0.5,0.5,TRUE,TRUE,TIRE_FACETS,
	-0.2,0.5,8.0, 0.2,0.5,8.0);

    if (!shadow) {
	PAINT(img_fildes,0.0,0.0,0.0);
	circle(img_fildes,1.0,8,FALSE, 0.0,1.5,-4.0, 0.0,1.5,5.0);
    }

    if (!shadow) ALUMINUM(img_fildes);
    /* Needle */
    mesh_cone(img_fildes,0.1,0.0,FALSE,FALSE,4,
	0.0,1.5,9.9, 0.0,1.5,11.0);
    /* Exhaust cone */
    mesh_cone(img_fildes,1.0,0.5,FALSE,FALSE,8,
	0.0,1.5,-4.0, 0.0,1.5,-3.0);

    if (!shadow) {
	/* Cockpit */
	ALUMINUM(img_fildes);
	mesh_sphere(img_fildes,1.0,4,6, 0.0,2.25,0.0);

	/* Flame */
	invis[0] = LIGHTS_ON_BRAKES_OFF|LIGHTS_OFF_BRAKES_OFF;
	add_names_to_set(img_fildes,1,invis);
	    TRANSPARENT_ON(img_fildes,0.7);
	    SELF_LIT_ON(img_fildes);
	    PAINT(img_fildes,1.0,0.5,0.0);
	    mesh_surface_of_revolution(img_fildes,flame,
		sizeof(flame)/sizeof(float)/6,
		5, TRUE, 0.0,1.51,0.0,	0.0,1.49,100.0);
	    PAINT(img_fildes,1.0,0.0,0.0);
	    mesh_surface_of_revolution(img_fildes,flame,
		sizeof(flame)/sizeof(float)/6,
		7, TRUE, 0.0,1.49,0.0,	0.0,1.51,100.0);
	    SELF_LIT_OFF(img_fildes);
	    TRANSPARENT_OFF(img_fildes);
	remove_all_names_from_set(img_fildes);
    }
    else {
	TRANSPARENT_OFF(img_fildes);
    }

    /* No need to RESTORE_DEFAULT_VERTEX_FORMAT, since last was default. */
    DIFFUSE_LIGHTING_OFF(img_fildes);
}


static int create_shadow_dl(
    void)
{
    /* Make it small so it isn't seen unless center is seen. */
    static float mc_extent[2][3] = {
	{ -0.1, 0.0, -0.1 },
	{  0.1, 0.1,  0.1 }
    };
    int seg,graphics_seg;

    seg = get_dl_segment();
    graphics_seg = get_dl_segment();

    open_segment(img_fildes,graphics_seg,TRUE,FALSE);
	draw_car(TRUE);
    close_segment(img_fildes);

    open_segment(img_fildes,seg,TRUE,FALSE);
	set_extent(img_fildes,mc_extent);
	cond_execute_segment(img_fildes,CI_PRUNE,FALSE,graphics_seg);
    close_segment(img_fildes);

    return(seg);
}


static void create_rocket_car_graphics(
    DRIVE_OBJECT *obj)
{
#if 1
    hwObject
	*objects;
    hwInt32
	numObjects;

    numObjects = hwParseFile( "objects/rocket.hw", &objects );
    obj->display_list = createHwSegmentFromObj( objects, numObjects );
#else
    int RocketCarSeg;
    int HighResSeg;

    float mc_extent[2][3];

    mc_extent[0][0] = -2.0;
    mc_extent[0][1] = -0.0;
    mc_extent[0][2] = -7.00;
    mc_extent[1][0] =  2.0;
    mc_extent[1][1] =  3.0;
    mc_extent[1][2] = 10.0;

    RocketCarSeg = get_dl_segment();
    HighResSeg = get_dl_segment();

    obj->display_list = RocketCarSeg;

    open_segment(img_fildes,HighResSeg,FALSE,FALSE);
      draw_car(FALSE);
    close_segment(img_fildes);


    open_segment(img_fildes,RocketCarSeg,FALSE,FALSE);
      set_extent(img_fildes,mc_extent);
      cond_return(img_fildes,CI_PRUNE,TRUE);

      execute_segment(img_fildes,HighResSeg);
    close_segment(img_fildes);
#endif
}


void init_rocket_car_object(
    DRIVE_OBJECT *obj)
{
    DRIVE_OBJECT *child;

    if(debug) printf(" inside init_rocket_car_object() routine \n");

    obj->num_children = 0;

    if (rocket_car_dl == -1) {
    	create_rocket_car_graphics(obj);
	rocket_car_dl = obj->display_list;
	shadow_dl = create_shadow_dl();
    }
    else {
	obj->display_list = rocket_car_dl;
    }

    if (obj->color[0] == DEFAULT_OBJECT_COLOR) obj->color[0] = DEFAULT_RED;
    if (obj->color[1] == DEFAULT_OBJECT_COLOR) obj->color[1] = DEFAULT_GRN;
    if (obj->color[2] == DEFAULT_OBJECT_COLOR) obj->color[2] = DEFAULT_BLU;

    if ((obj->vehicle_auxdata = init_vaux(obj)) == NULL) {
	return;
    }
    init_pobj(obj,2000.0,25.0);
    obj->upd = (UpdateDisp *) malloc( sizeof(UpdateDisp) );
    obj->upd->color[0] = obj->color[0];
    obj->upd->color[1] = obj->color[1];
    obj->upd->color[2] = obj->color[2];
    obj->upd->invis_words = 1;
    /* These are things you want to draw */
    obj->upd->invis[0] = LIGHTS_OFF_BRAKES_ON|BACKUP_LIGHTS_OFF|WHEELS_CENTER;

    obj->update_self = apply_car_physics;
    obj->bound_mc[0] = -2.00;
    obj->bound_mc[1] =  0.00;
    obj->bound_mc[2] = -7.00;
    obj->bound_mc[3] =  2.00;
    obj->bound_mc[4] =  3.00;
    obj->bound_mc[5] = 10.00;
    update_wc_bounds(obj);

    /* apply brake */
    obj->controls.pointer_x = 0.0;
    obj->controls.pointer_y = -1.0;

    obj->surface_chars_xyz  = rocket_car_surface_chars_xyz;
    obj->surface_chars_bbox = rocket_car_surface_chars_bbox;

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
    obj->fire_point[1] = 1.5;
    obj->fire_point[2] = 10.0+1.0;
}
