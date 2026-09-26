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


#ifndef _OBJECT_H_INCLUDED
#define _OBJECT_H_INCLUDED

#include "hw.h"
#include "libnum.h"
#include "physics.h"
#include "controls.h"
#include "global.h"

#define HOVERWARE_MODEL 1

/* It is assumed that all moving objects will be defined such that
 * in their modelling coordinates, their front points down the -Z
 * axis, their top points up the Y axis, and their right side is on
 * the X axis.
 */

/************************ CONVENIENCE MACROS **************************/
/* Points of the physics bounding box.  T=top, U=underside, L=left,
 * R=Right, F=front, B=back.
 */
#define PT_UFL	0
#define PT_UFR	1
#define PT_UBR	2
#define PT_UBL	3
#define PT_TFL	4
#define PT_TFR	5
#define PT_TBR	6
#define PT_TBL	7

#define NUM_GEARS	7
#define NEUTRAL_GEAR	1



typedef struct {
    float pitch;			/* in radians */
    float roll;				/* in radians */
    float yaw;				/* in radians */
    float angle_of_attack;		/* in radians, pitch + relative wind */
    float yaw_angle_of_attack;		/* in radians, rel. wind on rudder */
    float speed;			/* len(phys->v) */

    /* these things are constants */
    float COG_to_aileron;		/* in ft */
    float COG_to_elevator;		/* in ft */
    float COG_to_rudder;		/* in ft */
    float aileron_area;			/* in ft^2 */
    float elevator_area;		/* in ft^2 */
    float rudder_area;			/* in ft^2 */
    float fuselage_area;		/* in ft^2, from side */
    float engine_torque;		/* at full throttle, incl. prop */
    float aspect_ratio;			/* of wings */
    float wing_area;			/* in ft^2 */
    float front_area;			/* in ft^2 */
    float prop_power;			/* in lbft/s at full throttle */
    float Cd_parasitic;			/* Cd at normal flying attitude */
} AEROPLANE;


/* Values for fields that objects can get from scene files */

#define OBJ_TYPE		(1<<0)
#define OBJ_LABEL		(1<<1)
#define OBJ_MATRIX		(1<<2)
#define OBJ_XROT		(1<<3)
#define OBJ_YROT		(1<<4)
#define OBJ_ZROT		(1<<5)
#define OBJ_POSITION		(1<<6)
#define OBJ_DESTINATION		(1<<7)
#define OBJ_X			(1<<8)
#define OBJ_Y			(1<<9)
#define OBJ_Z			(1<<10)
#define OBJ_SIZE		(1<<11)
#define OBJ_COLOR		(1<<12)
#define OBJ_LENGTH		(1<<13)
#define OBJ_WIDTH		(1<<14)
#define OBJ_HEIGHT		(1<<15)
#define OBJ_RADIUS		(1<<16)
#define OBJ_ANIMATION_BITS	(1<<17)
#define OBJ_ANGLE		(1<<18)
#define OBJ_COUNT		(1<<19)
#define OBJ_SPACING		(1<<20)
#define OBJ_DIMENSION		(1<<21)
#define OBJ_DATA		(1<<22)

#define OBJ_POSITION_FIELDS \
  (OBJ_POSITION|OBJ_X|OBJ_Y|OBJ_Z|OBJ_XROT|OBJ_YROT|OBJ_ZROT|OBJ_MATRIX)

#define OBJ_DEFAULT	\
  (OBJ_POSITION_FIELDS | OBJ_ANIMATION_BITS )

#define OBJ_SIZE_FIELDS \
  ( OBJ_SIZE|OBJ_LENGTH|OBJ_WIDTH|OBJ_HEIGHT)

/* Choices for thrust_mechanism */
typedef enum {
    REAR_WHEEL_DRIVE	= 0,
    FRONT_WHEEL_DRIVE	= 1,
    TRACK_DRIVE		= 2,
    THRUSTER		= 3,
    MOTORCYCLE		= 4,
    BINARY_THRUSTER	= 5
} THRUST_MECHANISM;

/************************ PHYSICS AUXDATA ******************************
 * This structure contains additional information necessary to perform
 * the physics on the object.  Only one is allocated per object type;
 * it is pointed to by all objects of that type.
 **********************************************************************/
typedef struct {
    float COG_mc[3];	 	/* Center of gravity (COG), in MCs. */
    float bbox_mc[8][3];	/* Bounding box, in MCs, for physics 
				 * calculations.  Should be in order:
				 *  front lower left, front lower right,
				 *  rear lower right, rear lower left,
				 *  front upper left, front upper right,
				 *  rear upper right, rear upper left.
				 */
    float coefficient_of_drag;	/* for air friction */
    float wheel_torque_mult;	/* turning efficiency factor */
    float best_turn_speed;	/* speed at which turning radius grows (FPS) */
    float max_obstacle_height;	/* how far can we climb? */

    float horsepower;
    float peak_power_rpm;	/* rpms at which engine eff == 1 */
    float two_peak_power;	/* engine eff at rpm = 2*peak */
    float gear_best_speed[NUM_GEARS];
				/* speed at which each gear hits peak RPM */
    float offroad_performance;	/* 0.0-1.0. How does roughness affect speed?
				 *  1.0 = very little; 0.0 = dramatically.
				 */
    THRUST_MECHANISM thrust_mechanism;

    /* Information to get the gauges set up correctly */
    GAUGE_CLASS	left_gauge_class;
    GAUGE_CLASS	right_gauge_class;
    float max_speed;		/* MPH */
    float max_rpm;
    float max_altitude;		/* feet */
} VEHICLE_AUXDATA;

typedef struct {
    float normal[3];
    float angle;  /* angle car is facing  -- 0 is X axis*/
    float ref[3]; /* reference point */
} SIMPLE_DATA;


#define Y_BADVALUE  (-1000000.0)

/* Values for object_id flags */
#define OBJECTCLASS_DYNAMIC		(1 << 0)  /* Does matrix ever change? */
#define OBJECTCLASS_DRIVEABLE		(1 << 1)  /* Is it driveable? */
#define OBJECTCLASS_CHECKPOINT		(1 << 2)  /* Is it a checkpoint? */
#define OBJECTCLASS_WORMHOLE  		(1 << 4)  /* Is it a wormhole? */
#define OBJECTCLASS_TANK      		(1 << 5)  /* Is it a Tank? */
#define OBJECTCLASS_MOTORCYCLE 		(1 << 6)  /* Is it a Motorcycle? */
#define OBJECTCLASS_SPACESHIP  		(1 << 7)  /* Is it a Spaceship? */
#define OBJECTCLASS_AIRPLANE  		(1 << 8)  /* Is it a Airplane? */
#define OBJECTCLASS_CAR       		(1 << 9)  /* Is it a Car? */
#define OBJECTCLASS_UFO       		(1 << 10)  /* Is it a UFO? */

#define VEHICLE_MASK \
    (OBJECTCLASS_TANK | OBJECTCLASS_MOTORCYCLE | OBJECTCLASS_SPACESHIP | \
    OBJECTCLASS_AIRPLANE | OBJECTCLASS_CAR | OBJECTCLASS_UFO)


typedef struct {
    char name[NAME_STRLEN];	/* Normal name */
    char lcname[NAME_STRLEN];	/* all lower case */
    unsigned int flags;		/* See values above... */
    int  number;		/* Object number */
    void (*init_routine)(
	void *obj);
    unsigned int valid_fields;	/* Which fields are used by this object class */
} OBJECT_ID;
extern OBJECT_ID object_id[];
extern OBJECT_ID *find_object(
    int objnum,
    char *objname);

extern int scene_ed;		/* 1 if SceneEd is being run, 0 otherwise */

/********************* MC SURFACE CHARACTERISTICS *************************
 * The information in this structure is filled out by an object's intersection
 * routines if the object is intersected.  All coordinates are in MCs --
 * the calling routine is responsible for turning the coordinates into WCs
 * if necessary.
 **********************************************************************/
typedef struct {
    float mc_x,mc_y,mc_z;	/* MC point under the point or box passed in. */
    float mc_normal[3];		/* MC surface normal there -- a unit vector */
    float friction;		/* Coefficient of friction */
    float roughness;		/* Vary up an down by this amount. */
    float roughness_frequency;	/* And at this frequency. */
} MC_SURFACE_CHARACTERISTICS; 

typedef enum {
    OPERATION_NONE		= -1,
    OPERATION_REPLACE		= 0,
    OPERATION_ADD		= 1,
    OPERATION_MULTIPLY		= 2,
    OPERATION_HISTORIC		= 3,
    OPERATION_SCENE_REL_REPLACE	= 4
} OPERATION;

typedef struct {
    OPERATION operation;
    float operand[3];
} WORMHOLE_OPERATION;

typedef struct {
    WORMHOLE_OPERATION position;
    WORMHOLE_OPERATION angle;
    WORMHOLE_OPERATION angular_velocity;
    WORMHOLE_OPERATION linear_velocity;
} WORMHOLE_DATA;

typedef unsigned int SECONDS;

/******************************** SceneEd_OBJECT *****************************
 * This structure contains additional information required by the Scene Editor
 * that is not kept around explicitly in the DRIVE_OBJECT structure 
 ************************************************************************/
typedef struct scene_ed_struct {
    float xrot;			/* to keep X rotation value */
    float yrot;			/* to keep Y rotation value */
    float zrot;			/* to keep Z rotation value */
    float x, y, z;		/* hold the X,Y,Z position */
    float length;		/* hold the length */
    float width;		/* hold the width */
    float height;		/* hold the height */
    float angle;		/* hold the unconverted angle */
    int	  has_xform;		/* was an explicit matrix specified? */
    matrix3d  orig_xform;	/* to keep specified xform */
    char  *comments;		/* comments about object */
    int   highlight;		/* whether or not to highlight this object */
    float destx, desty, destz;	/* For old wormhole objects */

} SceneEd_OBJECT;


typedef enum {
    RETURN_OK		= 0,
    RETURN_DELETE_ME	= 1
} RETURN_CONDITION;


/******************************** OBJECT *******************************
 * This structure contains all information about each object in the
 * scene.
 **********************************************************************/
typedef struct object_struct {
    OBJECT_ID *idptr;
    int num_children;
    int display_list;		/* Indexed DL for HW object hash table */
    int global_display_list;		/* for display of objects w/children */
    matrix3d  xform;
    matrix3d  ixform;

    float size[3];			/* from scene file */
    float color[3];			/* from scene file */
    float radius;			/* from scene file */
    float angle;			/* from scene file, in radians */
    char  subtype[NAME_STRLEN];		/* from scene file */
    char  label[LABEL_STRLEN];		/* from scene file */
    float spacing;			/* from scene file */
    int count;				/* from scene file */
    int dimension;			/* from scene file */
    float *data;			/* from scene file */
    unsigned int nameset_bits;		/* for mini-animations */

    float  bound_mc[6];			/* opposite corners in MCs */
    float  bound_wc[6];			/* opposite corners in WCs */

    CONTROLS controls;			/* where the user input goes */
    void (*update_controls)(
	struct object_struct *obj);
					/* routine to update user input */
    PHYSICAL_OBJECT *pobj;		/* mass, speed, etc. */
    VEHICLE_AUXDATA *vehicle_auxdata;	/* see above */
    AEROPLANE *aeroplane;		/* see above */
    SIMPLE_DATA     sdata;		/* see above */
    void            *additional_data;	/* for the object's use */
    RETURN_CONDITION (*update_self)(
	struct object_struct *obj,
	float t_interval);		/* apply physics, etc. */

    int (*surface_chars_xyz)(
	struct object_struct *obj,
	float x,float y, float z,
	MC_SURFACE_CHARACTERISTICS *mc_surf_char);
	/* Give me surface chars at point. x,y,z in MCs.
	 * If this routine is called, it is assumed that the point is
	 * within the MC bounding box of the object, so the routine only
	 * needs to check collision if the object doesn't completely
	 * fill the MC bounding box.  The return value is true iff
	 * the object is actually intersected.
	 */

    int (*surface_chars_bbox)(
	struct object_struct *obj,
	float bbox_mc[6],
	MC_SURFACE_CHARACTERISTICS *mc_surf_char);
	/* Give me surface chars at highest point of the object within
	 * the MC bounding box provided. If this routine is called,
	 * it is assumed that the provided MC bounding box intersects
	 * the MC bounding box of the object, so the routine only
	 * needs to check overlap if the object doesn't completely
	 * fill the MC bounding box.  The return value is true iff
	 * the object is actually intersected.
	 */

    void (*collision_routine)(
	struct object_struct *hitting_obj,
	struct object_struct *hit_obj,
	float t_interval);
	/* Routine to call when there's a collision with this object.
	 * This routine defaults to the standard collision handling
	 * routine do_collision(), though special objects override
	 * to do their own collision handling.
	 */

    struct object_struct *child_list;
    struct object_struct *previous,*next;
    void *connection;			/* identifies player objects (connection_type *) */
    UpdateDisp  *upd;		/* object has special display needs */
    void *scene;		/* Which scene is this object in? */
    SECONDS start_lapsec;	/* when lap started (seconds) */
    SECONDS start_lapusec;	/* when lap started (micro seconds) */
    SECONDS checkpoints;	/* word for checkpoint bits */
    struct object_struct *last_checkpt; /* last checkpt visited */
    float   time_to_checkpt;	/* how many seconds to last checkpt */
    int     turbo_boosts;	/* number of turbo boosts used by object */
    int     ammo_used;		/* number of shots fired by object */
    int     lights_on;		/* headlights on? */
    matrix3d *aim_xform;	/* pointer to aim matrix */
    float fire_point[3];	/* in MCs, run thru aim_mtx */
    SECONDS last_shot;		/* when was shell last fired? */
    SceneEd_OBJECT  *scene_ed;	/* pointer to extra data for SceneEd */

} DRIVE_OBJECT;


/********************* WC SURFACE CHARACTERISTICS *************************
 * After the intersection routines have found the highest object under the
 * point or bounding box, they transform the MC values returned by the object
 * into WCs and return this structure.
 **********************************************************************/
typedef struct {
    float wc_x,wc_y,wc_z;	/* WC point under the point or box passed in. */
    float wc_normal[3];		/* WC normal there -- a unit vector. */
    float friction;		/* Coefficient of friction */
    float roughness;		/* Vary up an down by this amount. */
    float roughness_frequency;	/* And at this frequency. */
    DRIVE_OBJECT *whichobj;	/* Which object is being intersected. */
} WC_SURFACE_CHARACTERISTICS; 



#define INVALID			(-1)

extern void complete_object(
    DRIVE_OBJECT *object_list[],
    DRIVE_OBJECT *new_obj);
extern void surface_chars(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    WC_SURFACE_CHARACTERISTICS *wc_sc);
extern void intersect_checkpoint(
    DRIVE_OBJECT *vehicle,
    DRIVE_OBJECT *chk);


/*** From read_scene.c ***/
extern void add_object_to_list(
    DRIVE_OBJECT *object_list[],
    DRIVE_OBJECT *object);

/*** From physics ***/
extern void update_wc_bounds(
    DRIVE_OBJECT *obj);

/***** Routines provided by the obj_utils module. *****/
extern void get_box_mc_normal(
    DRIVE_OBJECT *obj,
    float mcx, float mcy, float mcz,
    float mc_normal[3]);
extern void get_pole_mc_normal(
    DRIVE_OBJECT *obj,
    float mcx, float mcy, float mcz,
    float mc_normal[3]);

#define SIZE_LENGTH	0
#define SIZE_WIDTH	1
#define SIZE_HEIGHT	2

#define MAX_OBSTACLE_CLEARANCE	4.0	/* biggest max_obstacle_height */

/***************************************************************
 * HoverWare utility functions
 **************************************************************/
#define	MAX_FDS		256
struct HwObjCache {
    int
	segnum,
	objListSize,
	*segList,
	sentList[MAX_FDS/32];
    float
	(*matList)[4][4];
    hwObject
	*objList;
    struct HwObjCache
	*next;
    struct IPCMsg
	*msg;
};

extern int createHwSegmentFromObj( hwObject *oList, int nObjs );
extern int createHwSegmentList( int nSegs, int *segs, float (*mats)[4][4] );
extern void createHwSegmentFromMsg( struct IPCMsg *msg );
extern struct HwObjCache *findHwSegment( int segnum );
extern void clearSegmentSent( int sock );

#endif /*  _OBJECT_H_INCLUDED */
