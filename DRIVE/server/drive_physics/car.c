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


static void apply_contact_force(
    DRIVE_OBJECT *obj,
    VECTOR Ftot, VECTOR Ttot)
{
    /* Apply gravity not countered by ground force.  The object is
     * pulled straight down by gravity.  The ground supplies a
     * counter-force parallel to its normal such that the gravity
     * force and the counter-force cancel in the direction of
     * the normal -- thus, the only force remaining is perpendicular
     * to the normal.  We're trying to find that force that runs parallel
     * to the normal so we can add it in and cancel the gravity in that
     * direction.
     */
    PHYSICAL_OBJECT *pobj = obj->pobj;
    float dot,k,k1,d;
    float fx,fy,fz;
    float vx,vy,vz;
    int p,i;

    /* "dot" is dot product of gravity force (G)
     * and ground normal vector (N = ave_n[xyz]).
     */
    dot = pobj->mass*GRAVITY_CONSTANT * ave_ny;

    fx = ave_nx * dot; fy = ave_ny * dot; fz = ave_nz * dot;
    Ftot[XD] += fx;
    Ftot[YD] += (fy*0.98);  /* scale it down to keep things stuck to road. */
    Ftot[ZD] += fz;

    /* Now, if < 3 wheels on ground, apply torque. */
    if ((contact_points > 0) && (contact_points < 3)) {
	/* See below for an explanation... */
	dot = upper_nx*ave_nx + upper_ny*ave_ny + upper_nz*ave_nz;
	k1 = CONTACT_TORQUE_MULT / (float) contact_points
	    * (1.0 - CONTACT_TORQUE_FACTOR * ABS(dot));

	for (i=0; i<4; ++i) {
	    p = lower_point[i];
	    if ((d = bbox_wc[p][1] - surchar[i].wc_y) < ON_GROUND_EPSILON) {
		/* Apply torque.
		 *
		 * Assume force is even at each contacting wheel, so divide
		 * force by number of contact points.
		 *
		 * Scale it to be proportional to how far the tire has
		 * penetrated into the "ON_GROUND_EPSILON" zone.
		 *
		 * Also scale it down by the dot product of the normal to the
		 * upper car surface and the normal to the ground.  That
		 * will result in less torque when the car is almost level
		 * with the ground.
		 */
		if (d < 0.0) d = 0.0;
		k = (CONTACT_TORQUE_BASE - d/ON_GROUND_EPSILON) * k1;

		vx = (bbox_wc[p][0] - COG_wc[0]);
		vy = (bbox_wc[p][1] - COG_wc[1]);
		vz = (bbox_wc[p][2] - COG_wc[2]);

		Ttot[XD] += (vy*fz - vz*fy) * k;
		Ttot[YD] += (vz*fx - vx*fz) * k;
		Ttot[ZD] += (vx*fy - vy*fx) * k;
	    }
	}
    }
}


static void do_sliding_friction(
    DRIVE_OBJECT *obj,
    VECTOR Ftot, VECTOR Ttot,
    float multiplier)
{
    float ffriction,velocity,f;
    PHYSICAL_OBJECT *pobj = obj->pobj;

    velocity = LENGTH_TRIVECTOR(pobj->v);
    if ((contact_points)
	    && (velocity > VELOCITY_EPSILON)) {
	/**** SIMPLIFICATION -- assume it goes through the COG ****/
	/* Frictional force */
	ffriction = -ave_friction * (float) contact_points / 4.0
		* pobj->mass * GRAVITY_CONSTANT * multiplier;
	if (velocity < SLIDING_FRICTION_V_EPSILON) {
	    /* Keep it from applying too much force when
	     * basically stationary.
	     */
	    f = LENGTH_TRIVECTOR(Ftot);
	    if (f > pobj->mass) {
		if (-ffriction > f) ffriction = f * -1.1 / velocity;
	    }
	    else {
		ffriction *= (1.0/SLIDING_FRICTION_V_EPSILON);
	    }
	}
	else {
	    ffriction /= velocity;
	}
	/* Apply opposite of current velocity */
	ADDTO_TRIVECTOR_MULT(Ftot, pobj->v, ffriction);
    }
}

#define TRACK_DRIVE_DEAD_SPOT	0.05


/* Apply forces according to the control settings. */
static void do_controls(
    DRIVE_OBJECT *obj,
    VECTOR Ftot, VECTOR Ttot,
    float accbrake, float wheel,
    int gear)
{
    double angle;
    float kx,kz,eff;
    float wx,wy,wz,divisor;
    float force,vxz,velocity,dot;
    float fx,fy,fz;
    PHYSICAL_OBJECT *pobj = obj->pobj;
    VEHICLE_AUXDATA *vaux = obj->vehicle_auxdata;


    if ((ABS(pobj->v[XD]) > VELOCITY_EPSILON)
	    || (ABS(pobj->v[YD]) > VELOCITY_EPSILON)
	    || (ABS(pobj->v[ZD]) > VELOCITY_EPSILON)) {
	vxz = HYPOT2(pobj->v[XD],pobj->v[ZD]);
	velocity = LENGTH_TRIVECTOR(pobj->v);
    }
    else {
	vxz = velocity = 0.0;
    }

    if (vaux->thrust_mechanism == TRACK_DRIVE) {
	/* Make a dead spot in the center */
	if (wheel < -TRACK_DRIVE_DEAD_SPOT)
	    wheel = (wheel+TRACK_DRIVE_DEAD_SPOT) / 
	        (1.0-TRACK_DRIVE_DEAD_SPOT);
	else if (wheel > TRACK_DRIVE_DEAD_SPOT)
	    wheel = (wheel-TRACK_DRIVE_DEAD_SPOT) / 
	        (1.0-TRACK_DRIVE_DEAD_SPOT);
	else
	    wheel = 0.0;
    }

    /* Get direction of wheel in WCs */
    if (wheel == 0.0) {
	/* Straight down the MC Z axis */
	wx = pobj->R[ZD][XD];
	wy = pobj->R[ZD][YD];
	wz = pobj->R[ZD][ZD];
    }
    else {
	/* MC Z Axis plus part of X */
	angle = DEGREES_TO_RADIANS(wheel*MAX_WHEEL_DEGREES);
	kx = FSIN(angle);
	kz = FCOS(angle);
	/* Transform to world coordinates */
	wx = kx * pobj->R[XD][XD] + kz * pobj->R[ZD][XD];
	wy = kx * pobj->R[XD][YD] + kz * pobj->R[ZD][YD];
	wz = kx * pobj->R[XD][ZD] + kz * pobj->R[ZD][ZD];
    }

    if (obj->upd != NULL) {
	/* set what to draw */
	obj->upd->invis[0] &= ~(LIGHTS_MASK|BACKUP_MASK|WHEELS_MASK);
	if (accbrake > 0.0) {
	    if (obj->lights_on) obj->upd->invis[0] |= LIGHTS_ON_BRAKES_OFF;
	    else obj->upd->invis[0] |= LIGHTS_OFF_BRAKES_OFF;
	}
	else
	{
	    if (obj->lights_on) obj->upd->invis[0] |= LIGHTS_ON_BRAKES_ON;
	    else obj->upd->invis[0] |= LIGHTS_OFF_BRAKES_ON;
	}

	if (gear < 0) obj->upd->invis[0] |= BACKUP_LIGHTS_ON;
	else obj->upd->invis[0] |= BACKUP_LIGHTS_OFF;

	if      (wheel < -0.6) obj->upd->invis[0] |= WHEELS_FAR_LEFT;
	else if (wheel < -0.2) obj->upd->invis[0] |= WHEELS_LEFT;
	else if (wheel <  0.2) obj->upd->invis[0] |= WHEELS_CENTER;
	else if (wheel <  0.6) obj->upd->invis[0] |= WHEELS_RIGHT;
	else                   obj->upd->invis[0] |= WHEELS_FAR_RIGHT;
    }

    if (accbrake > 0.0) {
	/* Accelerator pressed. */
	eff = engine_efficiency(gear,vxz,vaux);
	force = HORSEPOWER_TO_POUNDS(vaux->horsepower) / 2.0
	    * accbrake
	    * POWER_TO_FORCE(vxz)
	    * eff;

	/* SIMPLIFICATION -- assume force is inline with COG and
	 * is directly down MC Z axis.
	 */
	switch (vaux->thrust_mechanism) {
	    case REAR_WHEEL_DRIVE:
	    case MOTORCYCLE:
		if (bbox_wc[PT_UBR][1] - surchar[PT_UBR].wc_y
			< (ON_GROUND_EPSILON*1.5)) {
		    /* Right wheel on ground */
		    ADDTO_TRIVECTOR_MULT(Ftot, pobj->R[ZD], force);
		}
		if (bbox_wc[PT_UBL][1] - surchar[PT_UBL].wc_y
			< (ON_GROUND_EPSILON*1.5)) {
		    /* Left wheel on ground */
		    ADDTO_TRIVECTOR_MULT(Ftot, pobj->R[ZD], force);
		}
		break;

	    case FRONT_WHEEL_DRIVE:
		if (bbox_wc[PT_UFR][1] - surchar[PT_UFR].wc_y
			< (ON_GROUND_EPSILON*1.5)) {
		    /* Right wheel on ground */
		    ADDTO_TRIVECTOR_MULT(Ftot, pobj->R[ZD],
			force * surchar[PT_UFR].friction);
		}
		if (bbox_wc[PT_UFL][1] - surchar[PT_UFL].wc_y
			< (ON_GROUND_EPSILON*1.5)) {
		    /* Left wheel on ground */
		    ADDTO_TRIVECTOR_MULT(Ftot, pobj->R[ZD],
			force * surchar[PT_UFL].friction);
		}
		break;

	    case TRACK_DRIVE:
		if (bbox_wc[PT_UFR][1] - surchar[PT_UFR].wc_y
			< (ON_GROUND_EPSILON*1.5)) {
		    /* Right wheel on ground */
		    ADDTO_TRIVECTOR_MULT(Ftot, pobj->R[ZD],
			force * surchar[PT_UFR].friction);
		}
		else if (bbox_wc[PT_UBR][1] - surchar[PT_UBR].wc_y
			< (ON_GROUND_EPSILON*1.5)) {
		    /* Right wheel on ground */
		    ADDTO_TRIVECTOR_MULT(Ftot, pobj->R[ZD],
			force * surchar[PT_UBR].friction);
		}
		if (bbox_wc[PT_UFL][1] - surchar[PT_UFL].wc_y
			< (ON_GROUND_EPSILON*1.5)) {
		    /* Left wheel on ground */
		    ADDTO_TRIVECTOR_MULT(Ftot, pobj->R[ZD],
			force * surchar[PT_UFL].friction);
		}
		else if (bbox_wc[PT_UBL][1] - surchar[PT_UBL].wc_y
			< (ON_GROUND_EPSILON*1.5)) {
		    /* Left wheel on ground */
		    ADDTO_TRIVECTOR_MULT(Ftot, pobj->R[ZD],
			force * surchar[PT_UBL].friction);
		}
		break;

	    case THRUSTER:
		ADDTO_TRIVECTOR_MULT(Ftot, pobj->R[ZD],
		    force * 2.0);
		break;

	    case BINARY_THRUSTER:
		ADDTO_TRIVECTOR_MULT(Ftot, pobj->R[ZD],
		    HORSEPOWER_TO_POUNDS(vaux->horsepower) 
			* POWER_TO_FORCE(vxz));
		break;
	}
    }
    else if ((accbrake < 0.0)
		&& (vxz > VELOCITY_EPSILON)) {
	/* Brake pressed. */
	do_sliding_friction(obj,Ftot,Ttot,-accbrake);
    }

    /***** CHANGE DIRECTION TO MATCH +/- MC Z AXIS *****/
    if (vxz > VELOCITY_EPSILON) {
	/* slide if velocity is too different from direction pointed */
	dot = ABS(DOTPRODUCT_TRIVECTOR(pobj->v,pobj->R[ZD]))
		/ velocity;
	if (dot > ave_friction) dot = ave_friction;
	if (travelling_backwards) {
	    ADD_TRIVECTORS_2MULT(pobj->v, pobj->v, 1.0-dot,
	        pobj->R[ZD], -dot*velocity);
	}
	else {
	    ADD_TRIVECTORS_2MULT(pobj->v, pobj->v, 1.0-dot,
	        pobj->R[ZD],  dot*velocity);
	}
	COPY_TRIVECTOR_MULT(pobj->p, pobj->v, pobj->mass);
	if (dot < 0.8) 
	    do_sliding_friction(obj,Ftot,Ttot,(1.0-dot)*0.25);
    }

    /***** WHEEL *****/
    if ((vxz > VELOCITY_EPSILON)
	    || (vaux->thrust_mechanism == TRACK_DRIVE)) {
	if (vaux->thrust_mechanism == TRACK_DRIVE) {
	    /* Can turn without moving. */
	    /* Cross MC Z axis->WCs with wheel for torque */
	    fx = pobj->R[ZD][YD]*wz - pobj->R[ZD][ZD]*wy;
	    fy = pobj->R[ZD][ZD]*wx - pobj->R[ZD][XD]*wz;
	    fz = pobj->R[ZD][XD]*wy - pobj->R[ZD][YD]*wx;
	}
	else if (((bbox_wc[PT_UFL][1] - surchar[PT_UFL].wc_y)
			< ON_GROUND_EPSILON)
		|| ((bbox_wc[PT_UFR][1] - surchar[PT_UFR].wc_y)
			< ON_GROUND_EPSILON)) {
	    /* Wheels on ground. */
	    /***** SIMPLIFICATION *****/
	    /* Cross velocity vector into wheel vector.  This gives a torque. */
	    if (vxz > vaux->best_turn_speed) {
		divisor = vxz
		    * FPOW(vxz/vaux->best_turn_speed,TURN_DIVISOR_POWER);
	    }
	    else {
		divisor = vaux->best_turn_speed;
	    }
#ifndef OLD_TURN
            if(divisor >= 300.0 ) divisor = 300.0;
#endif
	    fx = (pobj->v[YD]*wz - pobj->v[ZD]*wy) / divisor;
	    fy = (pobj->v[ZD]*wx - pobj->v[XD]*wz) / divisor;
	    fz = (pobj->v[XD]*wy - pobj->v[YD]*wx) / divisor;
	}
	else {
	    fx = fy = fz = 0.0;
	}

	Ttot[XD] += fx*vaux->wheel_torque_mult*ave_friction;
	Ttot[YD] += fy*vaux->wheel_torque_mult*ave_friction;
	Ttot[ZD] += fz*vaux->wheel_torque_mult*ave_friction;

	if  (vaux->thrust_mechanism == MOTORCYCLE) {
	    float tilt,desired,torque;

	    tilt = ASIN(obj->ixform[1][0]);
	    desired = -wheel*MOTORCYCLE_LEAN_ANGLE*vxz/MPH_TO_FPS(80.0);
	    if (desired > MOTORCYCLE_LEAN_ANGLE)
		desired = MOTORCYCLE_LEAN_ANGLE;
	    else if (desired < -MOTORCYCLE_LEAN_ANGLE)
		desired = -MOTORCYCLE_LEAN_ANGLE;
	    torque = (tilt-desired)*MOTORCYCLE_TILT_TORQUE_MULT;
	    /* Transform from MCs to WCs */
	    Ttot[XD] += torque*obj->xform[2][0];
	    Ttot[YD] += torque*obj->xform[2][1];
	    Ttot[ZD] += torque*obj->xform[2][2];
	}
    }
}


void do_ground_vehicle_forces(
    DRIVE_OBJECT *obj,
    VECTOR Ftot,VECTOR Ttot)
{
    PHYSICAL_OBJECT *pobj = obj->pobj;

    if (class != OBJECT_IN_AIR)  {
	apply_contact_force(obj,Ftot,Ttot);

	/* If resting on its wheels... */
	if (class == OBJECT_ON_BOTTOM) {
	    /* Apply forces generated by wheels */
	    do_controls(obj,Ftot,Ttot,
		obj->controls.pointer_y,obj->controls.pointer_x,
		obj->controls.gear);
	}
	else {
	    do_sliding_friction(obj,Ftot,Ttot,4.0);
	}
    }
    else if ((obj->vehicle_auxdata->thrust_mechanism == THRUSTER)
	    || (obj->vehicle_auxdata->thrust_mechanism == BINARY_THRUSTER)) {
	/* A quick hack for now */
	VEHICLE_AUXDATA *vaux = obj->vehicle_auxdata;
	float vxz,eff,force;

	vxz = HYPOT2(pobj->v[XD],pobj->v[ZD]);
	if (obj->vehicle_auxdata->thrust_mechanism == THRUSTER) {
	    eff = engine_efficiency(obj->controls.gear,vxz,vaux);
	    force = HORSEPOWER_TO_POUNDS(vaux->horsepower) / 2.0
		* MAX(obj->controls.pointer_y,0.0)
		* POWER_TO_FORCE(vxz)
		* eff;
	}
	else if (obj->controls.pointer_y > 0.0) {
	    force = HORSEPOWER_TO_POUNDS(vaux->horsepower)
		* POWER_TO_FORCE(vxz);
	}
	else {
	    force = 0.0;
	}
	ADDTO_TRIVECTOR_MULT(Ftot, pobj->R[ZD], force);
	if (obj->upd != NULL) {
	    /* set what to draw */
	    obj->upd->invis[0] &= ~(LIGHTS_MASK);
	    if (obj->controls.pointer_y > 0.0) {
		obj->upd->invis[0] |= obj->lights_on ?
		    LIGHTS_ON_BRAKES_OFF : LIGHTS_OFF_BRAKES_OFF;
	    }
	    else {
		obj->upd->invis[0] |= obj->lights_on ?
		    LIGHTS_ON_BRAKES_ON : LIGHTS_OFF_BRAKES_ON;
	    }
	}
    }

    do_air_drag(obj,Ftot,Ttot);
}


static void execute_car_physics(
    DRIVE_OBJECT *obj,
    float t_interval)
{
    float Ftot[4],Ttot[4];
    PHYSICAL_OBJECT *pobj = obj->pobj;

    /* Take dot product of velocity and MC Z.  If negative, backwards */
    travelling_backwards = 
    	(pobj->v[XD]*pobj->R[ZD][XD]
	+ pobj->v[ZD]*pobj->R[ZD][ZD] < 0.0);

    transform_object(obj);
    classify_object(obj);

    /* Restart if not on wheels and not moving */
    if ((class != OBJECT_ON_BOTTOM)
	    && (class != OBJECT_IN_AIR)
	    && (ABS(pobj->v[XD]) < SHORTCUT_EPSILON)
	    && (ABS(pobj->v[YD]) < SHORTCUT_EPSILON)
	    && (ABS(pobj->v[ZD]) < SHORTCUT_EPSILON)) {
	/* Reset this guy. */
	if (server_mode & SERVER_MODE_WRECK_UPRIGHT) {
	    if (obj->connection != NULL)
		server_upright((connection_type *) obj->connection, NULL);
	    else set_vehicle_upright(obj);
	    travelling_backwards = FALSE;
	    transform_object(obj);
	    classify_object(obj);
	}
	else if (server_mode & SERVER_MODE_WRECK_RESTART) {
	    if (obj->connection != NULL)
		server_restart_client((connection_type *) obj->connection, NULL);
	    travelling_backwards = FALSE;
	    transform_object(obj);
	    classify_object(obj);
	}
    }

    /* make sure it ends up above ground */
    displace_object(obj,t_interval,TRUE);

    ZERO_TRIVECTOR(Ftot);
    ZERO_TRIVECTOR(Ttot);

    Ftot[YD] = -pobj->mass * GRAVITY_CONSTANT;

    do_ground_vehicle_forces(obj,Ftot,Ttot);

    simple_compute_state(pobj,Ftot,Ttot,0.0,t_interval);


    /* copy updated R and x to xform */
    update_xforms(obj,pobj);

    if (contact_points > 0) {
	/* rotational damping */
	pobj->w[XD] *= 1.0 + (ROTATION_DAMPING_X - 1.0);
	pobj->w[YD] *= 1.0 + ave_friction * (ROTATION_DAMPING_Y - 1.0);
	pobj->w[ZD] *= 1.0 + (ROTATION_DAMPING_Z - 1.0);
    }

    /* Update the WC bounding box for this car. */
    update_wc_bounds(obj);
}


RETURN_CONDITION apply_car_physics(
    DRIVE_OBJECT *obj,
    float t_interval)
{
    PHYSICAL_OBJECT *pobj = obj->pobj;

    /* If not moving and on ground and not changing, no need for all this... */
    if (       (ABS(pobj->p[XD]) < SHORTCUT_EPSILON) /* not rotating */
	    && (ABS(pobj->p[YD]) < SHORTCUT_EPSILON)
	    && (ABS(pobj->p[ZD]) < SHORTCUT_EPSILON)
	    && (ABS(pobj->w[XD]) < SHORTCUT_EPSILON) /* not moving */
	    && (ABS(pobj->w[YD]) < SHORTCUT_EPSILON)
	    && (ABS(pobj->w[ZD]) < SHORTCUT_EPSILON)
	    && (ABS(obj->bound_wc[1]) < ON_GROUND_EPSILON) /* on ground */
	    && (ABS(obj->controls.pointer_x) < SHORTCUT_EPSILON) /* no turn */
	    && (obj->controls.pointer_y <= 0.0)  /* not accelerating */
	    && (pobj->R[YD][YD] > COS45)) {	 /* on wheels */
	return(RETURN_OK);
    }

    while (t_interval > EPSILON) {
	if (t_interval > PHYSICS_TIMESLICE) {
	    execute_car_physics(obj,PHYSICS_TIMESLICE);
	    t_interval -= PHYSICS_TIMESLICE;
	}
	else {
	    execute_car_physics(obj,t_interval);
	    t_interval = 0.0;
	}
    }

    update_shadow_matrix(obj,surchar);

    /* Make sure I haven't crossed scenes */
    obj->scene = (void *) check_scenes(obj,pobj->x[XD],pobj->x[ZD]);

    return(RETURN_OK);
}
