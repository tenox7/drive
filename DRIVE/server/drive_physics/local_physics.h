/* $Source: /cvsroot/hoverball/HB_HW/DRIVE/server/drive_physics/local_physics.h,v $
 * $Revision: 1.1 $
 * $Date: 2004/02/20 00:44:29 $
 */

#ifndef _LOCAL_PHYSICS_H_INCLUDED
#define _LOCAL_PHYSICS_H_INCLUDED

#include <stdio.h>
#include <math.h>
#include "libnum.h"
#include "physics.h"
#include "object.h"
#include "demo_physics.h"
#include "scene.h"

extern float invsquareroot[1024];
#define INVSQUAREROOT_SIZE (sizeof(invsquareroot)/sizeof(float))

#define DEBUG_COLLISIONS		TRUE
#define PHYSICS_TIMESLICE		(0.025)	/* Seconds per iteration      */
#define SIN45 				(0.70710678)
#define COS45 				SIN45
#define VELOCITY_EPSILON		(1.0e-5)
#define SHORTCUT_EPSILON		(1.0e-3)
#define EPSILON				(1.0e-5)
#define COLLISION_ELASTICITY		(0.25)
#define COLLISION_OFFSET		(0.5)	/* Feet back from coll pt     */
#define MAX_WHEEL_DEGREES		(25.0)	/* How far can turn wheel     */
#define TURN_DIVISOR_POWER		(1.2)	/* Affects turns at hi speed  */
#define BASE_EFFICIENCY			(0.1)	/* Engine efficiency          */
#define POWER_TO_FORCE(velocity) 					       \
    (((velocity) < 2.0) ?						       \
	BASE_EFFICIENCY :						       \
	(((velocity) < ((float) INVSQUAREROOT_SIZE)) ?			       \
	    BASE_EFFICIENCY * invsquareroot[(int)(velocity)] :		       \
		BASE_EFFICIENCY / FSQRT(velocity)))

#define ON_GROUND_EPSILON		(0.15)	/* feet                       */
#define CONTACT_TORQUE_MULT		(2.5)	/* Speed of rotation to grnd  */
#define CONTACT_TORQUE_FACTOR		(0.75)  /* Reduction if nearly upright*/
#define CONTACT_TORQUE_BASE		(1.05)	/* -1 = mult at ON_GROUND_E   */
#define ROTATION_DAMPING_X		(0.7)	/* How much to damp rotation  */
#define ROTATION_DAMPING_Y		(0.7)   /*    if not in air.          */
#define ROTATION_DAMPING_Z		(0.7)   
#define SLIDING_FRICTION_V_EPSILON	(5.0)	/* fps */
#define INHERENT_DRAG			(250.0)
#define DRAG_MULT			(0.5)
#define MOTORCYCLE_LEAN_ANGLE		DEGREES_TO_RADIANS(35.0)
#define MOTORCYCLE_TILT_TORQUE_MULT	(-200.0)

extern int img_fildes;
extern int
    lower_point[4],	/* the lower four points of the object */
    contact_points,	/* number points nearly touching ground */
    class,		/* see OBJECT_ON_* macros below */
    travelling_backwards;
extern WC_SURFACE_CHARACTERISTICS
    surchar[4];		/* the surface below those points */
extern float
    ave_friction;	/* average surface friction factor */
extern float
    ave_nx,ave_ny,ave_nz;	/* average surface normal */
extern float
    upper_nx,upper_ny,upper_nz;	/* normal of the side now on top */
extern float
    COG_wc[3],
    bbox_wc[8][3];

#define OBJECT_ON_BOTTOM	0
#define OBJECT_ON_LEFT		1
#define OBJECT_ON_RIGHT		2
#define OBJECT_ON_FRONT		3
#define OBJECT_ON_REAR		4
#define OBJECT_ON_TOP		5
#define OBJECT_IN_AIR		6


/* A table of cube roots for fast work */
extern float unitcuberoot[1024];
extern float _crtmp;
#define UNITCUBEROOT_SIZE (sizeof(unitcuberoot)/sizeof(float))
#define CUBEROOT(k)							\
    (((_crtmp = (k)) < 0) ? 0 :						\
	((_crtmp >= 1.0) ? 1.0 : 					\
	    unitcuberoot[(int)(_crtmp*(UNITCUBEROOT_SIZE-1))]))
#define OFFROAD_SCALE_FACTOR	1000.0
#define OFFROAD_FACTOR(vaux,roughness)					\
    (((roughness) <= 0.0) ? 0.0 :					\
	(OFFROAD_SCALE_FACTOR 						\
	* CUBEROOT((roughness)/(vaux)->max_obstacle_height)		\
	* (1.0 - (vaux)->offroad_performance)))

/* From common.c */
extern void transform_object(
    DRIVE_OBJECT *obj);
extern void displace_object(
    DRIVE_OBJECT *obj,
    float t_interval,
    int check_collisions);
extern void classify_object(
    DRIVE_OBJECT *obj);
extern void update_shadow_matrix(
    DRIVE_OBJECT *obj,
    WC_SURFACE_CHARACTERISTICS surface[4]);
extern void do_air_drag(
    DRIVE_OBJECT *obj,
    VECTOR Ftot, VECTOR Ttot);

/* From car.c */
extern void do_ground_vehicle_forces(
    DRIVE_OBJECT *obj,
    VECTOR Ftot,
    VECTOR Ttot);


/* From intersect.c */
extern void find_collision_point(
    DRIVE_OBJECT *obj1, DRIVE_OBJECT *obj2,		/* IN: the objects */
    float t_interval,			/* IN: time step interval */
    float *v1x, float *v1y, float *v1z,	/* IN+OUT: WC velocity */
    float *v2x, float *v2y, float *v2z,	/* IN+OUT: WC velocity */
    float *intx, float *inty, float *intz,/* OUT: intersection point */
    float *t_int);			/* OUT: time of intersection */
extern void find_collision_normal(
    DRIVE_OBJECT *obj1, DRIVE_OBJECT *obj2,
    float vx, float vy, float vz,
    float collx, float colly, float collz,
    float t_int,
    float wc_normal[]);

#endif /* _LOCAL_PHYSICS_H_INCLUDED */
