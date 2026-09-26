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


#ifndef _PHYSICS_H_INCLUDED
#define _PHYSICS_H_INCLUDED

#define PHYSICS_VERSION	2

#ifndef SUCCESS
# define SUCCESS	0
#endif
#ifndef FAILURE
# define FAILURE	(-1)
#endif

#if !defined(LIBNUM_XINDEX)
# define LIBNUM_XINDEX 1
#endif
#define XD	(LIBNUM_XINDEX)
#define YD	(LIBNUM_XINDEX+1)
#define ZD	(LIBNUM_XINDEX+2)


/* The following structure contains the full physical state of
 * an object.
 */
typedef struct {
    /**** CONSTANTS ****/
    float mass;
    MATRIX I,		/* Inertial tensor matrix (MC) */
           Iinv;	/* its inverse */

    /**** DIFFERENTIAL STATE VARIABLES ****/
    VECTOR x;		/* position of COG (wc) */
    QUATERNION *Q;	/* rotational position (to wc) */
    VECTOR p; 	 	/* linear momentum (wc) */
    VECTOR w;		/* angular velocity (wc) */

    /**** AUXILIARY VARIABLES ****/
    VECTOR v;		/* linear velocity (wc) = p/mass */
    MATRIX R,		/* 3x3 rotational matrix for Q:  MC->WC */
	   Rinv;	/* WC->MC, transpose of R */
    VECTOR L;	  	/* angular momentum (wc) */
} PHYSICAL_OBJECT;


/**** EXTERNAL ENTRYPOINTS ****/
extern int init_physics_state(
    void);
extern void close_physics_state(
    void);

extern PHYSICAL_OBJECT *allocate_physical_object(
    void);
extern void free_physical_object(
    PHYSICAL_OBJECT *obj);

extern void compute_state(
    PHYSICAL_OBJECT *object,
    const VECTOR Ftotal,
    const VECTOR Ttotal,
    const float t1,
    const float t2);
extern void simple_compute_state(
    PHYSICAL_OBJECT *obj,
    const VECTOR Ftot,
    const VECTOR Ttot,
    const float t1,
    const float t2);
extern void update_angular_momentum(
    PHYSICAL_OBJECT *obj);
extern float energy(
    const PHYSICAL_OBJECT *whichobj,
    const float g);

/* Returns SUCCESS or FAILURE */
extern int solve_collision(
    PHYSICAL_OBJECT *obj1, PHYSICAL_OBJECT *obj2,
    const VECTOR c1, const VECTOR c2,
    const VECTOR icoll, const VECTOR jcoll,
    const float elast);

#endif /*  _PHYSICS_H_INCLUDED */
