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


/* Code Module for the ufo object */


#include <stdio.h>
#include <math.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"
#include "scene.h"
#include "demo_physics.h"

#define DEFAULT_HEIGHT	(6.0)
#define DEFAULT_RADIUS	(7.0)
#define DEFAULT_COLOR	(0.3)

#define DEFAULT_RED	DEFAULT_COLOR
#define DEFAULT_GRN	DEFAULT_COLOR
#define DEFAULT_BLU	DEFAULT_COLOR

#define ROTATION_RATE 	(45.0*M_PI/180.0)

#define Height			(obj->size[SIZE_HEIGHT])
#define Radius			(obj->radius)

typedef struct _ufo_list {
    float radius,height;
    int dl_number;
    int shadow_dl_number;
    struct _ufo_list *next;
} UFO_LIST;
static UFO_LIST *ufo_list=NULL;



#ifdef UFOS_CAN_COLLIDE
static int ufo_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    if (x*x + z*z < Radius*Radius) {
	sc->mc_y = obj->size[SIZE_HEIGHT];
	get_pole_mc_normal(obj,x,y,z,sc->mc_normal);
	return(TRUE);
    }
    /* else */
    return(FALSE);
}

static int ufo_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    float x,y,z;

    find_bbox_point_closest_to_point(bbox_mc, 0.0,0.0,0.0, &x,&y,&z);
	
    if (x*x + z*z < Radius*Radius) {
	sc->mc_y = obj->size[SIZE_HEIGHT];
	return(TRUE);
    }
    /* else */
    return(FALSE);
}
#endif /* UFOS CAN COLLIDE */

#define MAXSPEED  30.0
#define MAXSPEEDY 5.0
#define MINHEIGHT 10.0
#define MAXHEIGHT 100.0
static float vx,vy,vz;

static RETURN_CONDITION move_yourself(
    DRIVE_OBJECT *obj,
    float t_interval)
{
    SCENE *myscene = (SCENE *) obj->scene;
    float minx = myscene->xscene * SCENE_SIZE - SCENE_SIZE/2.0;
    float minz = myscene->zscene * SCENE_SIZE - SCENE_SIZE/2.0;
    float maxx = minx + SCENE_SIZE;
    float maxz = minz + SCENE_SIZE;
    static float mat[4][4] = IDENTITY4x4;
    float angle;

    /* rotate */
    angle = ROTATION_RATE * t_interval;
    mat[0][0] = mat[2][2] = FCOS(angle);
    mat[2][0] = FSIN(angle);
    mat[0][2] = -mat[2][0];
    concat_matrix(mat,obj->xform,obj->xform);

    /* and move */
    if (ZINTRAND(20*60) == 1) {
	vx = BOUNDED_FLOATRAND(-MAXSPEED,MAXSPEED);
	vy = BOUNDED_FLOATRAND(-MAXSPEEDY,MAXSPEEDY);
	vz = BOUNDED_FLOATRAND(-MAXSPEED,MAXSPEED);
    }
    /* Check scene boundries */
    if (obj->xform[3][0] < minx)		vx = FLOATRAND(MAXSPEED);
    else if (obj->xform[3][0] > maxx)		vx = -FLOATRAND(MAXSPEED);
    if (obj->xform[3][1] < MINHEIGHT)           vy = FLOATRAND(MAXSPEEDY);
    else if (obj->xform[3][1] > MAXHEIGHT)	vy = -FLOATRAND(MAXSPEEDY);
    if (obj->xform[3][2] < minz)		vz = FLOATRAND(MAXSPEED);
    else if (obj->xform[3][2] > maxz)		vz = -FLOATRAND(MAXSPEED);

    obj->xform[3][0] += vx*t_interval;
    obj->xform[3][1] += vy*t_interval;
    obj->xform[3][2] += vz*t_interval;

    return(RETURN_OK);
}

#define BULGE 2.0

static int create_ufo_graphics(
    DRIVE_OBJECT *obj)
{
    hwObject
	*objects;
    int
	i, numObjects;

    numObjects = hwParseFile( "objects/ufo.hw", &objects );
    for( i = 0; i < numObjects; i++ ) {
	HW_MODIFY_3F( objects[i], hwStrScale, Radius, Radius, Radius );
    }
    return createHwSegmentFromObj( objects, numObjects );
}


static int create_shadow_graphics(
    DRIVE_OBJECT *obj)
{
    int   ufo_shadow_seg;
    float outline[4][3];


    outline[0][0] = 0.0;
    outline[0][1] = 0.0;
    outline[0][2] = 0.0;

    outline[1][0] = Radius/2.0;
    outline[1][1] = 0.0;
    outline[1][2] = 0.0;

    outline[2][0] = Radius;
    outline[2][1] = Radius/4.0;
    outline[2][2] = 0.0;

    outline[3][0] = 0.0;
    outline[3][1] = Radius;
    outline[3][2] = 0.0;

    ufo_shadow_seg = get_dl_segment();

    open_segment(img_fildes,ufo_shadow_seg,0,0);
	SHADOW(img_fildes);
	TRANSPARENT_ON(img_fildes,0.5);

	mesh_surface_of_revolution(img_fildes,(float *) outline,4,8,FALSE,
	    0.0,0.0,0.0, 0.0,1.0,0.0);

	TRANSPARENT_OFF(img_fildes);
    close_segment(img_fildes);

    return(ufo_shadow_seg);
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
    vaux->bbox_mc[PT_UFL][0] = -Radius;
	    vaux->bbox_mc[PT_UFL][1] = 0.0;
	    vaux->bbox_mc[PT_UFL][2] =  Radius;
    vaux->bbox_mc[PT_UFR][0] =  Radius;
	    vaux->bbox_mc[PT_UFR][1] = 0.0;
	    vaux->bbox_mc[PT_UFR][2] =  Radius;
    vaux->bbox_mc[PT_UBR][0] =  Radius;
	    vaux->bbox_mc[PT_UBR][1] = 0.0;
	    vaux->bbox_mc[PT_UBR][2] = -Radius;
    vaux->bbox_mc[PT_UBL][0] = -Radius;
	    vaux->bbox_mc[PT_UBL][1] = 0.0;
	    vaux->bbox_mc[PT_UBL][2] = -Radius;
    vaux->bbox_mc[PT_TFL][0] = -Radius;
	    vaux->bbox_mc[PT_TFL][1] = Height;
	    vaux->bbox_mc[PT_TFL][2] =  Radius;
    vaux->bbox_mc[PT_TFR][0] =  Radius;
	    vaux->bbox_mc[PT_TFR][1] = Height;
	    vaux->bbox_mc[PT_TFR][2] =  Radius;
    vaux->bbox_mc[PT_TBR][0] =  Radius;
	    vaux->bbox_mc[PT_TBR][1] = Height;
	    vaux->bbox_mc[PT_TBR][2] = -Radius;
    vaux->bbox_mc[PT_TBL][0] = -Radius;
	    vaux->bbox_mc[PT_TBL][1] = Height;
	    vaux->bbox_mc[PT_TBL][2] = -Radius;
    vaux->COG_mc[0] =
	    (vaux->bbox_mc[PT_UFR][0] + vaux->bbox_mc[PT_UFL][0]) / 2.0;
    vaux->COG_mc[1] =
	    (vaux->bbox_mc[PT_TFL][1] + vaux->bbox_mc[PT_UFL][1]) / 2.0;
    vaux->COG_mc[2] =
	    (vaux->bbox_mc[PT_UBR][2] + vaux->bbox_mc[PT_UFR][2]) / 2.0;


    vaux->coefficient_of_drag = 0.200;
    vaux->wheel_torque_mult   = 70.0  /* *pobj->I[2][2] */;
    vaux->best_turn_speed     = 0.0;
    vaux->max_obstacle_height = 40.0;

    vaux->horsepower          = 500.0;
    vaux->peak_power_rpm      = 4000.0;
    vaux->two_peak_power      = 0.5;
    vaux->gear_best_speed[0]  = MPH_TO_FPS(50.0);	/* reverse */
    vaux->gear_best_speed[1]  = 0.0;			/* neutral */
    vaux->gear_best_speed[2]  = MPH_TO_FPS(50.0);	/* 1st */
    vaux->gear_best_speed[3]  = MPH_TO_FPS(100.0);	/* 2nd */
    vaux->gear_best_speed[4]  = MPH_TO_FPS(150.0);	/* 3rd */
    vaux->gear_best_speed[5]  = MPH_TO_FPS(200.0);	/* 4th */
    vaux->gear_best_speed[6]  = MPH_TO_FPS(300.0);	/* 5th */
    vaux->offroad_performance = 1.0;
    vaux->thrust_mechanism    = THRUSTER;

    vaux->left_gauge_class  = GAUGE_DIGITAL_MPH;
    vaux->right_gauge_class = GAUGE_DIGITAL_ALTITUDE;
    vaux->max_speed         = 300.0;
    vaux->max_rpm           = 8000.0;
    vaux->max_altitude      = 25000.0;

    return(vaux);
}



void init_ufo_object(
    DRIVE_OBJECT *obj)
{
    UFO_LIST *rl;
    DRIVE_OBJECT *child;
    int player_controlled = (obj->connection != NULL);


    if (Height == 0.0) Height = DEFAULT_HEIGHT;
    if (Radius == 0.0) Radius = DEFAULT_RADIUS;
    if (obj->color[0] == DEFAULT_OBJECT_COLOR) obj->color[0] = DEFAULT_RED;
    if (obj->color[1] == DEFAULT_OBJECT_COLOR) obj->color[1] = DEFAULT_GRN;
    if (obj->color[2] == DEFAULT_OBJECT_COLOR) obj->color[2] = DEFAULT_BLU;


    if(debug) printf(" inside init_ufo_object() routine \n");

    /* See if we've created one like this before... */
    rl = ufo_list;
    while (rl != NULL) {
	if ((rl->radius == Radius)
		&& (rl->height == Height)) {
	    break;
	}
	/* else */
	rl = rl->next;
    }
    if (rl != NULL) {
	/* Good -- I have one like this already. */
	obj->display_list = rl->dl_number;
    }
    else {
	/* Nope -- gotta create a new one. */
	if ((rl = (UFO_LIST *) malloc(sizeof(UFO_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}
	rl->radius = Radius;
	rl->height = Height;
	rl->next   = ufo_list;
	rl->dl_number = obj->display_list = create_ufo_graphics(obj);
	rl->shadow_dl_number = create_shadow_graphics(obj);
	ufo_list  = rl;
    }

    if ((obj->upd = (UpdateDisp_ptr) malloc(sizeof(UpdateDisp))) == NULL) {
	return;
    }
    obj->upd->invis_words = 1;
    obj->upd->color[0] = obj->color[0];
    obj->upd->color[1] = obj->color[1];
    obj->upd->color[2] = obj->color[2];
    obj->upd->invis[0] = LIGHTS_OFF_BRAKES_OFF|BACKUP_LIGHTS_OFF|WHEELS_CENTER;

    /***** INIT AUXILIARY DATA *****/
    if ((obj->vehicle_auxdata = init_vaux(obj)) == NULL) {
	return;
    }
    init_pobj(obj,1.0,70.0);

    /* Don't collide with UFOs */
#ifdef UFO_COLLISIONS_ENABLED
    obj->surface_chars_xyz  = ufo_surface_chars_xyz;
    obj->surface_chars_bbox = ufo_surface_chars_bbox;
#endif /* UFO_COLLISIONS_ENABLED */

    /* Initial (mc) bounding box values */
    obj->bound_mc[0] = -Radius;
    obj->bound_mc[1] = 0.0;
    obj->bound_mc[2] = -Radius;
    obj->bound_mc[3] = Radius;
    obj->bound_mc[4] = Height;
    obj->bound_mc[5] = Radius;

    if (player_controlled) {
	obj->update_self = apply_ufo_physics;
    }
    else {
	obj->update_self = move_yourself;
	vx = BOUNDED_FLOATRAND(-MAXSPEED,MAXSPEED);
	vy = BOUNDED_FLOATRAND(-MAXSPEEDY,MAXSPEEDY);
	vz = BOUNDED_FLOATRAND(-MAXSPEED,MAXSPEED);
    }

    /* apply the object's xform matrix to the bounding box to put it in
     * world coordinates */
    update_wc_bounds(obj);

    if (player_controlled) {
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
	child->display_list = rl->shadow_dl_number;
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
    }
    else {
	obj->num_children = 0;
    }

    obj->aim_xform = &(obj->xform);
    obj->fire_point[0] = 0.0;
    obj->fire_point[1] = Radius/4;
    obj->fire_point[2] = Radius+1.0;
}
