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
#include "drive_server.h"

#define BALLISTIC_TIMESLICE	(PHYSICS_TIMESLICE/10)

static boolean_type execute_ballistic_physics(
    DRIVE_OBJECT *obj,
    float t_interval)
{
    float Ftot[3+LIBNUM_XINDEX],Ttot[3+LIBNUM_XINDEX];
    PHYSICAL_OBJECT *pobj = obj->pobj;

    transform_object(obj);

    /* shortcut for classify_object */
    class = OBJECT_IN_AIR;
    lower_point[0] = 0;
    lower_point[1] = 1;
    lower_point[2] = 2;
    lower_point[3] = 3;
    upper_nx = pobj->R[YD][XD];
    upper_ny = pobj->R[YD][YD];
    upper_nz = pobj->R[YD][ZD];

    /* make sure it ends up above ground */
    displace_object(obj,t_interval,TRUE);

    ZERO_TRIVECTOR(Ftot);
    ZERO_TRIVECTOR(Ttot);
    Ftot[YD] = -pobj->mass * GRAVITY_CONSTANT;

    /* do_air_drag(obj,Ftot,Ttot); */

    simple_compute_state(pobj,Ftot,Ttot,0.0,t_interval);

    /* copy updated R and x to xform */
    update_xforms(obj,pobj);

    if (contact_points > 0) {
	/* Touching an object */
	cause_explosion(obj->scene,
	    obj->xform[3][0],obj->xform[3][1],obj->xform[3][2],
	    EXPLOSION_RADIUS,EXPLOSION_BASE_FORCE,EXPLOSION_BASE_TORQUE);
	return(FALSE);
    }

    /* Update the WC bounding box for this ballistic. */
    update_wc_bounds(obj);

    /* Make sure I haven't crossed scenes */
    obj->scene = (void *) check_scenes(obj,pobj->x[XD],pobj->x[ZD]);

    return(TRUE);
}


RETURN_CONDITION apply_ballistic_physics(
    DRIVE_OBJECT *obj,
    float t_interval)
{
    PHYSICAL_OBJECT *pobj = obj->pobj;

    if (obj->pobj == NULL) return(RETURN_DELETE_ME);

    /* If not moving and on ground and not changing, no need for all this... */
    if (       (ABS(pobj->p[XD]) < SHORTCUT_EPSILON) /* not moving */
	    && (ABS(pobj->p[YD]) < SHORTCUT_EPSILON)
	    && (ABS(pobj->p[ZD]) < SHORTCUT_EPSILON)
	    && (ABS(obj->bound_wc[1]) < ON_GROUND_EPSILON)) { /* on ground */
	cause_explosion(obj->scene,
	    obj->xform[3][0],obj->xform[3][1],obj->xform[3][2],
	    EXPLOSION_RADIUS,EXPLOSION_BASE_FORCE,EXPLOSION_BASE_TORQUE);
	return(RETURN_DELETE_ME);
    }

    while (t_interval > EPSILON) {
	if (t_interval > BALLISTIC_TIMESLICE) {
	    if (!execute_ballistic_physics(obj,BALLISTIC_TIMESLICE))
		return(RETURN_DELETE_ME);
	    t_interval -= BALLISTIC_TIMESLICE;
	}
	else {
	    if (!execute_ballistic_physics(obj,t_interval))
		return(RETURN_DELETE_ME);
	    t_interval = 0.0;
	}
    }

    return(RETURN_OK);
}
