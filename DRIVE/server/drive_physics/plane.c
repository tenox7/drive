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
#include "aerodyn.h"
#include "drive_server.h"

#define SIGNUM(a) ((a) > 0.0 ? 1.0 : -1.0)

#define ELEVATORS_ENABLED
#define AILERONS_ENABLED
#define RUDDER_ENABLED

static void update_plane_state(
    DRIVE_OBJECT *obj)
{
    PHYSICAL_OBJECT *pobj = obj->pobj;
    AEROPLANE *pptr = obj->aeroplane;
    float ftmp;

    /* Figure auxiliary stuff */
    pptr->speed = LENGTH_TRIVECTOR(pobj->v);

    if (pptr->speed == 0.0) {
	pptr->angle_of_attack = pptr->pitch;
	pptr->yaw_angle_of_attack = 0.0;
    }
    else {
	double Vpcx,Vpcy,Vpcz;

	/* find velocity vector in PC's */
	/* velocity vector * Rt */
	Vpcx = pobj->v[XD] * pobj->R[XD][XD] 
	     + pobj->v[YD] * pobj->R[XD][YD] 
	     + pobj->v[ZD] * pobj->R[XD][ZD];
	Vpcy = pobj->v[XD] * pobj->R[YD][XD] 
	     + pobj->v[YD] * pobj->R[YD][YD] 
	     + pobj->v[ZD] * pobj->R[YD][ZD];
	Vpcz = pobj->v[XD] * pobj->R[ZD][XD] 
	     + pobj->v[YD] * pobj->R[ZD][YD] 
	     + pobj->v[ZD] * pobj->R[ZD][ZD];
	pptr->angle_of_attack = FATAN2(-Vpcy,Vpcz);
	pptr->yaw_angle_of_attack = FATAN2(-Vpcx,Vpcz);
    }

    if ((ftmp = pobj->R[ZD][YD]) == 1.0) {
	pptr->pitch = -M_PI/2.0;
	pptr->yaw   = 0.0;
	pptr->roll  = -FATAN2(pobj->R[XD][XD],-pobj->R[YD][XD]);
    }
    else if (ftmp == -1.0) {
	pptr->pitch = M_PI/2.0;
	pptr->yaw   = 0.0;
	pptr->roll  = -FATAN2(pobj->R[XD][XD],-pobj->R[YD][XD]);
    }
    else {
	pptr->pitch = -FSIN(-ftmp);
	ftmp = FSQRT(1.0 - ftmp*ftmp);
	pptr->yaw = -FATAN2(-pobj->R[ZD][XD]/ftmp,
	    pobj->R[ZD][ZD]/ftmp);
	pptr->roll = -FATAN2(pobj->R[XD][YD]/ftmp,
	    pobj->R[YD][YD]/ftmp);
    }
}


static void aerodynamic_forces_torques(
    DRIVE_OBJECT *obj,
    float Force[], float Torque[],
    float elevator_position,
    float aileron_position,
    float rudder_position,
    float throttle_position)
{
    PHYSICAL_OBJECT *pobj = obj->pobj;
    AEROPLANE *pptr = obj->aeroplane;
    float ftmp;
    float dynamic_pressure,Cl,Cd,drag,lift,thrust,air_density;
    float wing_drag,ind_drag,para_drag,cos_aoa,sin_aoa,speed;
    float force_pc[ZD+1],force_wc[ZD+1],torque_pc[ZD+1];
    float w_pc[ZD+1];
    float cos_yaoa,sin_yaoa;


    sin_yaoa = FSIN(pptr->yaw_angle_of_attack);
    cos_yaoa = FCOS(pptr->yaw_angle_of_attack);

    /* Translate angular velocity to PCs */
    w_pc[XD] = pobj->w[XD] * pobj->R[XD][XD]
		 + pobj->w[YD] * pobj->R[XD][YD]
		 + pobj->w[ZD] * pobj->R[XD][ZD];
    w_pc[YD] = pobj->w[XD] * pobj->R[YD][XD]
		 + pobj->w[YD] * pobj->R[YD][YD]
		 + pobj->w[ZD] * pobj->R[YD][ZD];
    w_pc[ZD] = pobj->w[XD] * pobj->R[ZD][XD]
		 + pobj->w[YD] * pobj->R[ZD][YD]
		 + pobj->w[ZD] * pobj->R[ZD][ZD];

    sin_aoa = FSIN(pptr->angle_of_attack);
    cos_aoa = FCOS(pptr->angle_of_attack);

    torque_pc[PITCH_AXIS] = 
	torque_pc[YAW_AXIS] =
	torque_pc[ROLL_AXIS] =
	0.0;

    /* dynamic pressure */
    air_density = AIR_DENSITY(pobj->x[YD]);
    dynamic_pressure = 0.5 * air_density * pptr->speed * pptr->speed;
	    
    Cl = COEFFICIENT_OF_LIFT(pptr->angle_of_attack);
    Cd = COEFFICIENT_OF_DRAG(pptr->angle_of_attack);

    /********************** WINGS **************************/
    /* wing drag */
    wing_drag = Cd * dynamic_pressure * pptr->wing_area;

    /* induced drag */
    ind_drag = (Cl*Cl/(SPAN_EFFICIENCY_FACTOR*M_PI*pptr->aspect_ratio))
	    * dynamic_pressure * pptr->wing_area;

    drag = wing_drag + ind_drag;
    lift = Cl * dynamic_pressure * pptr->wing_area;

    /* transform wing lift and drag into plane coordinates */
    force_pc[LATERAL_AXIS] = 0.0;
    force_pc[UP_AXIS] = sin_aoa * drag + cos_aoa * lift;
    force_pc[THRUST_AXIS] = -(cos_aoa * drag - sin_aoa * lift);

    /* parasitic drag */
    para_drag = pptr->Cd_parasitic * dynamic_pressure
	* (cos_yaoa * pptr->front_area + ABS(sin_yaoa) * pptr->fuselage_area);
    /* Parasitic drag is in WC's opposite direction of movement */
    force_wc[XD] = (pobj->v[XD]/pptr->speed) * -para_drag;
    force_wc[YD] = (pobj->v[YD]/pptr->speed) * -para_drag;
    force_wc[ZD] = (pobj->v[ZD]/pptr->speed) * -para_drag;
    force_pc[XD] += force_wc[XD] * pobj->R[XD][XD]
		      + force_wc[YD] * pobj->R[XD][YD]
		      + force_wc[ZD] * pobj->R[XD][ZD];
    force_pc[YD] += force_wc[XD] * pobj->R[YD][XD]
		      + force_wc[YD] * pobj->R[YD][YD]
		      + force_wc[ZD] * pobj->R[YD][ZD];
    force_pc[ZD] += force_wc[XD] * pobj->R[ZD][XD]
		      + force_wc[YD] * pobj->R[ZD][YD]
		      + force_wc[ZD] * pobj->R[ZD][ZD];

#ifdef ELEVATORS_ENABLED
    /******************** ELEVATORS **********************/
    {
    float aoa_elevator;

    /* assume AOAtail == AOAwing, for now */
    aoa_elevator = pptr->angle_of_attack
	    + elevator_position * CONTROL_TO_ANGLE_OF_ATTACK;
    Cl = CENTERED_COEFFICIENT_OF_LIFT_NO_STALL(aoa_elevator);
    Cd = CENTERED_COEFFICIENT_OF_DRAG_NO_STALL(aoa_elevator);
    lift = Cl * dynamic_pressure * pptr->elevator_area
	    * ELEVATOR_EFFICIENCY;
    drag = Cd * dynamic_pressure * pptr->elevator_area
	    * ELEVATOR_EFFICIENCY;

    /* again, translate these into plane coordinates. */
    /* drag applies directly */
    force_pc[THRUST_AXIS] -= cos_aoa * drag - sin_aoa * lift;
    /* elevator lift applies a Torque around the pitch axis */
    torque_pc[PITCH_AXIS] = (sin_aoa * drag + cos_aoa * lift)
	    * pptr->COG_to_elevator;
    /* compensate for coming counter-rotational Torque by fudging */
    torque_pc[PITCH_AXIS] /= 10.0;

#define COUNTER_PITCH_TORQUE
#ifdef COUNTER_PITCH_TORQUE
    /* counterpressure due to rotational speed acting on horizontal
     * stabilizer.
     */
    speed  = w_pc[PITCH_AXIS] * pptr->COG_to_elevator;
    ftmp   = 0.5 * air_density * speed * speed * 2.0
	    * pptr->elevator_area * pptr->COG_to_elevator;
    if (w_pc[PITCH_AXIS] > 0.0) ftmp = -ftmp;
    torque_pc[PITCH_AXIS] += ftmp;
#endif /* COUNTER_PITCH_TORQUE */

    }
#endif

#ifdef AILERONS_ENABLED
    /******************** AILERONS **********************/
    {
    float aoa_aileron,aileron_lift,aileron_drag;
    float pc_lift_left,pc_lift_right,pc_drag_left,pc_drag_right;
    float Cl_aileron,Cd_aileron;

    /* Treat the ailerons like small wings */
    aoa_aileron = pptr->angle_of_attack
	    - aileron_position * CONTROL_TO_ANGLE_OF_ATTACK;
    Cl_aileron = CENTERED_COEFFICIENT_OF_LIFT_NO_STALL(aoa_aileron);
    Cd_aileron = CENTERED_COEFFICIENT_OF_DRAG_NO_STALL(aoa_aileron);

    /* aileron drag */
    aileron_drag = Cd_aileron * dynamic_pressure * pptr->wing_area;
    /* ignore induced drag for ailerons */
    aileron_lift = Cl_aileron * dynamic_pressure * pptr->aileron_area
	    * AILERON_EFFICIENCY;

    /* transform aileron lift and drag into plane coordinates */
    pc_lift_right = sin_aoa * aileron_drag + cos_aoa * aileron_lift;
    pc_drag_right = cos_aoa * aileron_drag - sin_aoa * aileron_lift;

    aoa_aileron = pptr->angle_of_attack
	    - aileron_position * CONTROL_TO_ANGLE_OF_ATTACK / 2.0;
    Cl_aileron = CENTERED_COEFFICIENT_OF_LIFT_NO_STALL(aoa_aileron);
    Cd_aileron = CENTERED_COEFFICIENT_OF_DRAG_NO_STALL(aoa_aileron);

    /* aileron drag */
    aileron_drag = Cd_aileron * dynamic_pressure * pptr->wing_area;
    /* ignore induced drag for ailerons */
    aileron_lift = Cl_aileron * dynamic_pressure * pptr->aileron_area
	    * AILERON_EFFICIENCY;

    /* transform aileron lift and drag into plane coordinates */
    pc_lift_left = sin_aoa * aileron_drag + cos_aoa * aileron_lift;
    pc_drag_left = cos_aoa * aileron_drag - sin_aoa * aileron_lift;

    /* add lift & drag to totals */
    force_pc[UP_AXIS] += pc_lift_right + pc_lift_left;
    force_pc[THRUST_AXIS] -= pc_drag_right + pc_drag_left;

    torque_pc[ROLL_AXIS] =
	    pptr->COG_to_aileron * (pc_lift_right - pc_lift_left);

#define COUNTER_ROLL_TORQUE
#ifdef COUNTER_ROLL_TORQUE
    ftmp = 0.5 * dynamic_pressure * pptr->wing_area / 100.0
	    * pptr->COG_to_aileron * -w_pc[ROLL_AXIS] * 3.0
	    / (M_PI / 2.0);
    torque_pc[ROLL_AXIS] += ftmp;

#endif 

    }
#endif


#ifdef RUDDER_ENABLED	
    /********************** RUDDER *************************/
    {
    float aoa_rudder;

    aoa_rudder = pptr->yaw_angle_of_attack
	    + rudder_position * CONTROL_TO_ANGLE_OF_ATTACK;

    /* torque_pc[YAW_AXIS] = -1000.0*aoa_rudder; */

    sin_aoa = FSIN(aoa_rudder);
    cos_aoa = FCOS(aoa_rudder);

    Cl = CENTERED_COEFFICIENT_OF_LIFT_NO_STALL(aoa_rudder);
    Cd = CENTERED_COEFFICIENT_OF_DRAG_NO_STALL(aoa_rudder);
    lift = Cl * dynamic_pressure * pptr->rudder_area
	    * RUDDER_EFFICIENCY;
    drag = Cd * dynamic_pressure * pptr->rudder_area
	    * RUDDER_EFFICIENCY;

    /* again, translate these into plane coordinates. */
    /* drag applies directly */
    force_pc[THRUST_AXIS] -= cos_aoa * drag - sin_aoa * lift;
    /* rudder lift applies a Torque around the yaw axis */
    torque_pc[YAW_AXIS] = (sin_aoa * drag + cos_aoa * lift)
	    * pptr->COG_to_rudder;
    /* compensate for coming counter-rotational Torque by fudging */
    torque_pc[YAW_AXIS] /= 200.0;

#define YAW_COUNTERROTATION
#ifdef YAW_COUNTERROTATION
    /* counterpressure due to rotational speed acting on rudder */
    speed  = w_pc[YAW_AXIS] * pptr->COG_to_rudder;
    ftmp   = 40.0 * air_density * speed * speed
	    * pptr->rudder_area * pptr->COG_to_rudder;
    if (w_pc[YAW_AXIS] > 0.0) ftmp = -ftmp;
    torque_pc[YAW_AXIS] += ftmp;

    /* counterpressure due to rotational speed acting on fuselage */
    speed  = w_pc[YAW_AXIS] * pptr->COG_to_rudder/2.0;
    ftmp   = 40.0 * air_density * speed * speed
	    * pptr->fuselage_area * pptr->COG_to_rudder;
    if (w_pc[YAW_AXIS] > 0.0) ftmp = -ftmp;
    torque_pc[YAW_AXIS] += ftmp;
#endif

#define FUSELAGE_EFFECT
#ifdef FUSELAGE_EFFECT
    /* pressure due to relative wind on fuselage */

    Cl = CENTERED_COEFFICIENT_OF_LIFT_NO_STALL(pptr->yaw_angle_of_attack);
    Cd = CENTERED_COEFFICIENT_OF_DRAG_NO_STALL(pptr->yaw_angle_of_attack);
    lift = Cl * dynamic_pressure * pptr->fuselage_area;
    drag = Cd * dynamic_pressure * pptr->fuselage_area;

    force_pc[THRUST_AXIS] -= drag;
    force_pc[LATERAL_AXIS] += lift;
#endif
    }
#endif


    /********************** THRUST *************************/
    thrust = HORSEPOWER_TO_POUNDS(pptr->prop_power)
	* throttle_position
	* POWER_TO_FORCE(pptr->speed);
    force_pc[THRUST_AXIS] += thrust;

    /********* TRANSFORM FORCES TO WC'S **************/
    Force[XD] = force_pc[XD] * pobj->R[XD][XD]
		  + force_pc[YD] * pobj->R[YD][XD]
		  + force_pc[ZD] * pobj->R[ZD][XD];
    Force[YD] = force_pc[XD] * pobj->R[XD][YD]
		  + force_pc[YD] * pobj->R[YD][YD]
		  + force_pc[ZD] * pobj->R[ZD][YD];
    Force[ZD] = force_pc[XD] * pobj->R[XD][ZD]
		  + force_pc[YD] * pobj->R[YD][ZD]
		  + force_pc[ZD] * pobj->R[ZD][ZD];

    /* finally, gravity */
    Force[UP_AXIS] -= pobj->mass * GRAVITY_ACCELERATION_FPS;

    Torque[XD] = torque_pc[XD] * pobj->R[XD][XD]
		   + torque_pc[YD] * pobj->R[YD][XD]
		   + torque_pc[ZD] * pobj->R[ZD][XD];
    Torque[YD] = torque_pc[XD] * pobj->R[XD][YD]
		   + torque_pc[YD] * pobj->R[YD][YD]
		   + torque_pc[ZD] * pobj->R[ZD][YD];
    Torque[ZD] = torque_pc[XD] * pobj->R[XD][ZD]
		   + torque_pc[YD] * pobj->R[YD][ZD]
		   + torque_pc[ZD] * pobj->R[ZD][ZD];
}


static void execute_plane_physics(
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
	    set_vehicle_upright(obj);
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

    /* Update the plane stuff */
    update_plane_state(obj);

    ZERO_TRIVECTOR(Ftot);
    ZERO_TRIVECTOR(Ttot);

    if (class != OBJECT_IN_AIR)  {
	/* On ground */
	Ftot[YD] = -pobj->mass * GRAVITY_CONSTANT;
	do_ground_vehicle_forces(obj,Ftot,Ttot);
	/* rotational damping */
	pobj->w[XD] *= 1.0 + ave_friction * (ROTATION_DAMPING_X - 1.0);
	pobj->w[YD] *= 1.0 + ave_friction * (ROTATION_DAMPING_Y - 1.0);
	pobj->w[ZD] *= 1.0 + ave_friction * (ROTATION_DAMPING_Z - 1.0);
    }
    else {
	aerodynamic_forces_torques(obj,Ftot,Ttot,
	    (float) obj->controls.pointer_y,
	    (float) obj->controls.pointer_x,
	    (float) 0.0, /* rudder */
	    (float) 1.0);
    }

    simple_compute_state(pobj,Ftot,Ttot,0.0,t_interval);

    /* copy updated R and x to xform */
    update_xforms(obj,pobj);

    /* Update the WC bounding box for this vehicle. */
    update_wc_bounds(obj);
}


RETURN_CONDITION apply_plane_physics(
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
	    execute_plane_physics(obj,PHYSICS_TIMESLICE);
	    t_interval -= PHYSICS_TIMESLICE;
	}
	else {
	    execute_plane_physics(obj,t_interval);
	    t_interval = 0.0;
	}
    }

    update_shadow_matrix(obj,surchar);

    /* Make sure I haven't crossed scenes */
    obj->scene = (void *) check_scenes(obj,pobj->x[XD],pobj->x[ZD]);

    return(RETURN_OK);
}
