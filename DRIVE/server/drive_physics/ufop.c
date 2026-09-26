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
#include <math.h>
#include "libnum.h"
#include "physics.h"
#include "object.h"
#include "demo_physics.h"
#include "scene.h"
#include "local_physics.h"

#define MAX_ROTATION_RATE	(M_PI/2.0)		/* radians/s */
#define MAX_SPEED		MPH_TO_FPS(400.0) 	/* feet/s */
#define MAX_Y_SPEED		(50.0)	 		/* feet/s */


static void ufo_forces_torques(
    DRIVE_OBJECT *obj,
    float Force[], float Torque[],
    float accel_position,
    float rotate_position,
    float d_altitude_position)
{
    PHYSICAL_OBJECT *pobj = obj->pobj;
    float k;

    pobj->w[YD] = rotate_position * MAX_ROTATION_RATE;
    pobj->w[XD] = pobj->w[ZD] = 0.0;

    if (accel_position > 0.0) {
	k = accel_position * accel_position;
	pobj->v[XD] = pobj->R[ZD][XD] * k * MAX_SPEED;
	pobj->v[ZD] = pobj->R[ZD][ZD] * k * MAX_SPEED;
    }
    else {
	pobj->v[XD] = pobj->v[ZD] = 0.0;
    }
    pobj->v[YD] = d_altitude_position * MAX_Y_SPEED;

    pobj->p[XD] = pobj->v[XD] * pobj->mass;
    pobj->p[YD] = pobj->v[YD] * pobj->mass;
    pobj->p[ZD] = pobj->v[ZD] * pobj->mass;

    if (obj->upd != NULL) {
	obj->upd->invis[0] &= ~(LIGHTS_MASK|BACKUP_MASK|WHEELS_MASK);
	if (obj->lights_on) obj->upd->invis[0] |= LIGHTS_ON_BRAKES_OFF;
	else obj->upd->invis[0] |= LIGHTS_OFF_BRAKES_OFF;
    }
}


static void execute_ufo_physics(
    DRIVE_OBJECT *obj,
    float t_interval)
{
    float Ftot[4],Ttot[4];
    PHYSICAL_OBJECT *pobj = obj->pobj;

    travelling_backwards = FALSE;

    transform_object(obj);

    /* make sure it ends up above ground */
    displace_object(obj,t_interval,FALSE);

    Ftot[1] = Ftot[2] = Ftot[3]
	= Ttot[1] = Ttot[2] = Ttot[3]
	= 0.0;

    ufo_forces_torques(obj,Ftot,Ttot,
	(float) obj->controls.pointer_y,
	(float) obj->controls.pointer_x,
	(float) obj->controls.altitude_accel);

    simple_compute_state(pobj,Ftot,Ttot,0.0,t_interval);

    /* copy updated R and x to xform */
    update_xforms(obj,pobj);

    /* Update the WC bounding box for this vehicle. */
    update_wc_bounds(obj);
}


RETURN_CONDITION apply_ufo_physics(
    DRIVE_OBJECT *obj,
    float t_interval)
{
    PHYSICAL_OBJECT *pobj = obj->pobj;

    /* If not moving and on ground and not changing, no need for all this... */
    if (TRIVECTOR_IS_ZERO(pobj->p) /* not rotating */
            && TRIVECTOR_IS_ZERO(pobj->w) /* not moving */
	    && (ABS(obj->bound_wc[1]) < ON_GROUND_EPSILON) /* on ground */
	    && (ABS(obj->controls.pointer_x) < SHORTCUT_EPSILON) /* no turn */
	    && (obj->controls.pointer_y <= 0.0)  /* not accelerating */
	    && (pobj->R[YD][YD] > COS45)) {	 /* on wheels */
	return(RETURN_OK);
    }

    while (t_interval > EPSILON) {
	if (t_interval > PHYSICS_TIMESLICE) {
	    execute_ufo_physics(obj,PHYSICS_TIMESLICE);
	    t_interval -= PHYSICS_TIMESLICE;
	}
	else {
	    execute_ufo_physics(obj,t_interval);
	    t_interval = 0.0;
	}
    }

    update_shadow_matrix(obj,surchar);

    /* Don't cross any checkpoints */
    obj->checkpoints = 0;
    obj->last_checkpt = NULL;
    obj->time_to_checkpt = 1010.0;

    /* Make sure I haven't crossed scenes */
    obj->scene = (void *) check_scenes(obj,pobj->x[XD],pobj->x[ZD]);

    return(RETURN_OK);
}
