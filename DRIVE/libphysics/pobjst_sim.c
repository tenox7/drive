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
#include "libnum.h"
#include "physics.h"
#include "physics.ih"

#define OLD_METHOD 1


/* This overly-simplistic view of the world thinks that the
 * variables of motion are independent and we can avoid nasty
 * ordinary differential equations.  If the time step is small,
 * not a bad assumption!
 */


void simple_compute_state(
    PHYSICAL_OBJECT *obj,
    const VECTOR Ftot, const VECTOR Ttot,
    const float t1, const float t2)
{
    float dt = (t2 - t1);
    float vtmp[3+LIBNUM_XINDEX];
    /* Assume these will be reused. */
    static QUATERNION *dQ = NULL;
    static MATRIX IinvWC = NULL,Mtmp = NULL;
#ifdef DEBUG2
    MATRIX wDual = ALLOCATE_MATRIX(3,3);
    MATRIX dRdt = ALLOCATE_MATRIX(3,3);
    MATRIX dQ2dRdt = ALLOCATE_MATRIX(3,3);
#endif 
    

    if (IinvWC == NULL) {
	if ((IinvWC = ALLOCATE_MATRIX(3,3)) == NULL) {
	    return;
	}
	if ((Mtmp = ALLOCATE_MATRIX(3,3)) == NULL) {
	    FREE_MATRIX(IinvWC,3,3);
	    IinvWC = NULL;
	    return;
	}
	if ((dQ   = allocate_quaternion()) == NULL) {
	    FREE_MATRIX(IinvWC,3,3);
	    FREE_MATRIX(Mtmp,3,3);
	    IinvWC = NULL;
	    return;
	}
    }

    /* dx/dt = v so dx =~ v*dt */
    obj->x[XD] += obj->v[XD] * dt;
    obj->x[YD] += obj->v[YD] * dt;
    obj->x[ZD] += obj->v[ZD] * dt;

    /* dp/dt = F so dp =~ F*dt */
    obj->p[XD] += Ftot[XD] * dt;
    obj->p[YD] += Ftot[YD] * dt;
    obj->p[ZD] += Ftot[ZD] * dt;

#ifdef DEBUG2
    /* dRdt = dual(w)*R */
    vector_dual(obj->w,wDual);
    MATxMAT(wDual,obj->R,dRdt,3,3);
    /* dQ/dt = w*Q/2 */
    vtmp[XD] = 0.5 * obj->w[XD];
    vtmp[YD] = 0.5 * obj->w[YD];
    vtmp[ZD] = 0.5 * obj->w[ZD];
    VECxQ(vtmp,obj->Q,dQ);
    quaternion_to_matrix(dQ,dQ2dRdt);
#endif

    /* dQ/dt = .5*w*Q ~~ dQ = (.5*w*dt)*Q */
    vtmp[XD] = 0.5 * obj->w[XD] * dt;
    vtmp[YD] = 0.5 * obj->w[YD] * dt;
    vtmp[ZD] = 0.5 * obj->w[ZD] * dt;
    VECxQ(vtmp,obj->Q,dQ);
    obj->Q->s     += dQ->s;
    obj->Q->r[XD] += dQ->r[XD];
    obj->Q->r[YD] += dQ->r[YD];
    obj->Q->r[ZD] += dQ->r[ZD];

#ifdef OLD_METHOD
    /* dL/dt = T, w=Iinv*L ~~ dw = Iinv*(T*dt) */
    /* first convert Iinv to world coordinates */
    MATxDIAG(obj->Rinv,obj->Iinv,Mtmp,3,3);
    MATxMAT(Mtmp,obj->R,IinvWC,3,3);
    /* what about LxW? */
    vtmp[XD] = Ttot[XD] * dt;
    vtmp[YD] = Ttot[YD] * dt;
    vtmp[ZD] = Ttot[ZD] * dt;
    { float vtmp2[4];
    MATxVEC(IinvWC,vtmp,vtmp2,3,3);
    obj->w[XD] += vtmp2[XD];
    obj->w[YD] += vtmp2[YD];
    obj->w[ZD] += vtmp2[ZD];
    }
#else
    /* dwdt = IinvWC * (L x w + Ttot) */
    /* IinvWC = R * IinvMC * Rinv */
    MATxDIAG(obj->R,obj->Iinv,Mtmp,3,3);
    MATxMAT(Mtmp,obj->Rinv,IinvWC,3,3);
    CROSS_PRODUCT(obj->L,obj->w,vtmp);
    vtmp[XD] += Ttot[XD];
    vtmp[YD] += Ttot[YD];
    vtmp[ZD] += Ttot[ZD];
    {   float dwdt[3+LIBNUM_XINDEX];
	MATxVEC(IinvWC,vtmp,dwdt,3,3);
	obj->w[XD] += dwdt[XD] * dt;
	obj->w[YD] += dwdt[YD] * dt;
	obj->w[ZD] += dwdt[ZD] * dt;
    }
#endif /* OLD_METHOD else */

    /* AUX:  v from p:   v = p/m */
    obj->v[XD] = obj->p[XD] / obj->mass;
    obj->v[YD] = obj->p[YD] / obj->mass;
    obj->v[ZD] = obj->p[ZD] / obj->mass;
    /* AUX: R and Rinv from Q */
    quaternion_to_matrix(obj->Q,obj->R);
    CP_MATRIX(obj->R,obj->Rinv,3,3);
    TRANSPOSE(obj->Rinv,3,3);
    /* AUX: L  from I,R,w*/
    update_angular_momentum(obj);

#ifdef DEBUG2
    FREE_MATRIX(wDual,3,3);
    FREE_MATRIX(dRdt,3,3);
    FREE_MATRIX(dQ2dRdt,3,3);
#endif 
    /* Don't free the temporaries so we don't have to realloc next time. */
}
