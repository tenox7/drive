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


#ifndef _DEMO_PHYSICS_H_INCLUDED
#define _DEMO_PHYSICS_H_INCLUDED

/***** CONSTANTS *****/
#define GRAVITY_CONSTANT 	(32.17)
#define DAMPING_FACTOR		(0.95)

/* Stuff related to explosions */
#define EXPLOSION_TIME_INTERVAL	(0.5)
#define EXPLOSION_BASE_MASS	POUNDS_TO_SLUGS(2000.0)
#define EXPLOSION_BASE_ACCEL	200.0
#define EXPLOSION_BASE_FORCE	\
    (EXPLOSION_BASE_MASS*EXPLOSION_BASE_ACCEL/EXPLOSION_TIME_INTERVAL)
#define EXPLOSION_BASE_MOMENT	(EXPLOSION_BASE_MASS*50.0)
#define EXPLOSION_BASE_ROT	(M_PI/2.0)
#define EXPLOSION_BASE_TORQUE	\
    (EXPLOSION_BASE_MOMENT*EXPLOSION_BASE_ROT/EXPLOSION_TIME_INTERVAL)


/* Don't really want to include starbase.c.h, so just do concat_matrix. */
#ifndef __STARBASE_C_H
extern void concat_matrix(
    float mat1[4][4],
    float mat2[4][4],
    float rmat[4][4]);
#endif /* not __STARBASE_C_H */

/***** EXTERNAL ENTRYPOINTS *****/
extern RETURN_CONDITION apply_car_physics(
    DRIVE_OBJECT *obj,
    float t_interval);
extern RETURN_CONDITION apply_plane_physics(
    DRIVE_OBJECT *obj,
    float t_interval);
extern RETURN_CONDITION apply_ufo_physics(
    DRIVE_OBJECT *obj,
    float t_interval);
extern RETURN_CONDITION apply_xfighter_physics(
    DRIVE_OBJECT *obj,
    float t_interval);
extern RETURN_CONDITION apply_ballistic_physics(
    DRIVE_OBJECT *obj,
    float t_interval);
extern RETURN_CONDITION simple_physics(
    DRIVE_OBJECT *obj,
    float t_interval);
extern float engine_efficiency(
    int gear,
    float speed,
    VEHICLE_AUXDATA *vaux);
extern void intersect_objects_xyz(
    DRIVE_OBJECT *myobj,
    float x, float y, float z,
    WC_SURFACE_CHARACTERISTICS *wc_sc);
extern void intersect_objects_bbox(
    DRIVE_OBJECT *myobj,
    WC_SURFACE_CHARACTERISTICS *wc_sc);
extern void mc_to_wc(
    DRIVE_OBJECT *obj,
    float mx,  float my,  float mz,
    float *wx, float *wy, float *wz);
extern void wc_to_mc(
    DRIVE_OBJECT *obj,
    float wx, float wy, float wz,
    float *mx, float *my, float *mz);
extern void mc_normal_to_wc_normal(
    DRIVE_OBJECT *obj,
    float mc_normal[],
    float wc_normal[]);
extern int bboxes_intersect(
    float bbox1[],
    float bbox2[]);
extern int point_in_object(
    DRIVE_OBJECT *obj,
    float wcx, float wcy, float wcz);
extern void find_bbox_point_closest_to_point(
    float bbox[],
    float cx, float cy, float cz,
    float *intx, float *inty, float *intz);
extern void init_physics(
    void);
extern void find_bbox_point_closest_to_point(
    float bbox[6],
    float cx, float cy, float cz,
    float *intx, float *inty, float *intz);
extern void reset_rotation_matrices(
    DRIVE_OBJECT *obj,
    float yangle);		/* radians -- rotation around Y axis */
extern void reset_vehicle_physics(
    DRIVE_OBJECT *obj,
    float x, float y, float z,	/* new center */
    float yangle);		/* radians -- rotation around Y axis */
extern boolean_type set_vehicle_upright(
    DRIVE_OBJECT *obj);
extern void do_collision(
    DRIVE_OBJECT *hitting_obj, DRIVE_OBJECT *hit_obj,
    float t_interval);
extern float get_rough_time_estimate(
    DRIVE_OBJECT *obj1, DRIVE_OBJECT *obj2,
    float t_interval,
    float vx, float vy, float vz);
extern void update_xforms(
    DRIVE_OBJECT *obj,
    PHYSICAL_OBJECT *pobj);

#endif /* _DEMO_PHYSICS_H_INCLUDED */
