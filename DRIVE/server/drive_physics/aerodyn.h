/* $Source: /cvsroot/hoverball/HB_HW/DRIVE/server/drive_physics/aerodyn.h,v $
 * $Revision: 1.1 $
 * $Date: 2004/02/20 00:44:29 $
 *
 * Copyright 1991 Daryl Poe
 *
 */

#ifndef _AERODYN_H_INCLUDED
#define _AERODYN_H_INCLUDED


/* Approximate values for airfoil NACA 23012, 24" chord, R=6e6,
 * standard roughness
 */

#define Cl0	0.15 
#define CRITICAL_ANGLE (14.0*M_PI/180.0)
#define COEFFICIENT_OF_DRAG(a) \
    ((ABS(a) < CRITICAL_ANGLE) ? \
	(0.0103 - (6.012e-4 * 180.0 / M_PI) * (a) \
		+ (1.360e-4 * 180.0 * 180.0 / M_PI / M_PI) * (a) * (a)) : \
	    1.5)

#define COEFFICIENT_OF_LIFT(a) \
    ((ABS(a) < CRITICAL_ANGLE) ? \
	(Cl0 + (.098 * 180.0 / M_PI) * (a)) : \
	0.0)

#define CENTERED_COEFFICIENT_OF_LIFT_NO_STALL(a) \
    ((.098 * 180.0 / M_PI) * (a))

#define CENTERED_COEFFICIENT_OF_DRAG_NO_STALL(a) \
    (0.0103 \
	- (6.012e-4 * 180.0 / M_PI) * (a) \
	+ (1.360e-4 * 180.0 * 180.0 / M_PI / M_PI) * (a) * (a))

#define RHO_ZERO		0.00237691999	/* slug/ft^3 */
#define AIR_DENSITY(h) \
	(RHO_ZERO * pow((double)(1 - .000006875 * (h)),4.2561))

#define GRAVITY_ACCELERATION_FPS	32.2	/* ft/s^2 */
#define SPAN_EFFICIENCY_FACTOR		0.8
#define CONTROL_TO_ANGLE_OF_ATTACK	(5.0*M_PI/180.0)
#define ELEVATOR_EFFICIENCY		0.8
#define AILERON_EFFICIENCY		1.0
#define RUDDER_EFFICIENCY		0.5

#define X_AXIS		1
#define Y_AXIS		2
#define Z_AXIS		3

#define PITCH_AXIS	X_AXIS
#define YAW_AXIS	Y_AXIS
#define ROLL_AXIS	Z_AXIS

#define LATERAL_AXIS	X_AXIS
#define UP_AXIS		Y_AXIS
#define THRUST_AXIS	Z_AXIS


#endif /* _AERODYN_H_INCLUDED */
