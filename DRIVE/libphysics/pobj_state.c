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

#define Y_xx	(LIBNUM_XINDEX)
#define Y_xy	(LIBNUM_XINDEX+1)
#define Y_xz	(LIBNUM_XINDEX+2)
#define Y_Qs	(LIBNUM_XINDEX+3)
#define Y_Qrx	(LIBNUM_XINDEX+4)
#define Y_Qry	(LIBNUM_XINDEX+5)
#define Y_Qrz	(LIBNUM_XINDEX+6)
#define Y_px	(LIBNUM_XINDEX+7)
#define Y_py	(LIBNUM_XINDEX+8)
#define Y_pz	(LIBNUM_XINDEX+9)
#define Y_wx	(LIBNUM_XINDEX+10)
#define Y_wy	(LIBNUM_XINDEX+11)
#define Y_wz	(LIBNUM_XINDEX+12)
#define Y_MAX	Y_wz

#define EPSILON (1.0e-3)

#if defined(__linux__) || defined(WIN32)
# define VOLATILE
#else
# define VOLATILE volatile
#endif

static VOLATILE PHYSICAL_OBJECT *obj;
static VOLATILE VECTOR Ftot,Ttot;

static VOLATILE MATRIX Rtrans,Iinv,Itmp;
static VOLATILE QUATERNION *Qtmp;
static VOLATILE VECTOR halfw,state;

int init_physics_state(
    void)
{

    if ((Rtrans = ALLOCATE_MATRIX(3,3)) == NULL) {
	return(FAILURE);
    }
    if ((Iinv = ALLOCATE_MATRIX(3,3)) == NULL) {
	FREE_MATRIX(Rtrans,3,3);
	return(FAILURE);
    }
    if ((Itmp = ALLOCATE_MATRIX(3,3)) == NULL) {
	FREE_MATRIX(Rtrans,3,3);
	FREE_MATRIX(Iinv,3,3);
	return(FAILURE);
    }
    if ((Qtmp = allocate_quaternion()) == NULL) {
	FREE_MATRIX(Rtrans,3,3);
	FREE_MATRIX(Iinv,3,3);
	FREE_MATRIX(Itmp,3,3);
	return(FAILURE);
    }
    if ((halfw = ALLOCATE_VECTOR(3)) == NULL) {
	free_quaternion(Qtmp);
	FREE_MATRIX(Rtrans,3,3);
	FREE_MATRIX(Iinv,3,3);
	FREE_MATRIX(Itmp,3,3);
	return(FAILURE);
    }
    if ((state = ALLOCATE_VECTOR(Y_MAX)) == NULL) {
        FREE_VECTOR(halfw,3);
	free_quaternion(Qtmp);
	FREE_MATRIX(Rtrans,3,3);
	FREE_MATRIX(Iinv,3,3);
	FREE_MATRIX(Itmp,3,3);
	return(FAILURE);
    }

    return(SUCCESS);
}


/*ARGSUSED*/
static void state_derivs(
    float t,
    VECTOR cstate,
    VECTOR dcstate_dt)
{
    MATRIX Mtmp,I,Iinv_local;
    float vtmp[4],vtmp2[4];

    if ((I = ALLOCATE_MATRIX(3,3)) == NULL) {
	return;
    }
    if ((Iinv_local = ALLOCATE_MATRIX(3,3)) == NULL) {
	FREE_MATRIX(I,3,3);
	return;
    }
    if ((Mtmp = ALLOCATE_MATRIX(3,3)) == NULL) {
	FREE_MATRIX(I,3,3);
	FREE_MATRIX(Iinv_local,3,3);
	return;
    }

    /***** cstate to object *****/
    obj->x[XD]    = cstate[Y_xx];
    obj->x[YD]    = cstate[Y_xy];
    obj->x[ZD]    = cstate[Y_xz];
    obj->Q->s    = cstate[Y_Qs];
    obj->Q->r[XD] = cstate[Y_Qrx];
    obj->Q->r[YD] = cstate[Y_Qry];
    obj->Q->r[ZD] = cstate[Y_Qrz];
    obj->p[XD]    = cstate[Y_px];
    obj->p[YD]    = cstate[Y_py];
    obj->p[ZD]    = cstate[Y_pz];
    obj->w[XD]    = cstate[Y_wx];
    obj->w[YD]    = cstate[Y_wy];
    obj->w[ZD]    = cstate[Y_wz];

    /***** compute auxiliary variables *****/
    obj->v[XD] = obj->p[XD]/obj->mass;
    obj->v[YD] = obj->p[YD]/obj->mass;
    obj->v[ZD] = obj->p[ZD]/obj->mass;

    quaternion_to_matrix(obj->Q,obj->R);

    CP_MATRIX(obj->R,obj->Rinv,3,3);
    TRANSPOSE(obj->Rinv,3,3);

#define R_METHOD_1
#ifdef R_METHOD_1
    MATxDIAG(obj->Rinv,obj->I,Mtmp,3,3);
    MATxMAT(Mtmp,obj->R,I,3,3);
    MATxVEC(I,(float *) obj->w, (float *) obj->L,3,3);

    MATxDIAG(obj->Rinv,obj->Iinv,Mtmp,3,3);
    MATxMAT(Mtmp,obj->R,Iinv_local,3,3);
#else
    MATxDIAG(obj->R,obj->I,Mtmp,3,3);
    MATxMAT(Mtmp,obj->Rinv,I,3,3);
    MATxVEC(I,obj->w,obj->L,3,3);

    MATxDIAG(obj->R,obj->Iinv_local,Mtmp,3,3);
    MATxMAT(Mtmp,obj->Rinv,Iinv_local,3,3);
#endif

    /***** compute differential values *****/
    halfw[XD] = obj->w[XD]/2.0;
    halfw[YD] = obj->w[YD]/2.0;
    halfw[ZD] = obj->w[ZD]/2.0;
    VECxQ(halfw,obj->Q,(QUATERNION *) Qtmp);

    dcstate_dt[Y_xx]  = obj->v[XD];
    dcstate_dt[Y_xy]  = obj->v[YD];
    dcstate_dt[Y_xz]  = obj->v[ZD];
    dcstate_dt[Y_Qs]  = Qtmp->s;
    dcstate_dt[Y_Qrx] = Qtmp->r[XD];
    dcstate_dt[Y_Qry] = Qtmp->r[YD];
    dcstate_dt[Y_Qrz] = Qtmp->r[ZD];
    dcstate_dt[Y_px]  = Ftot[XD];
    dcstate_dt[Y_py]  = Ftot[YD];
    dcstate_dt[Y_pz]  = Ftot[ZD];

    /* dw/dt = Iinv_local*(Lxw + T) */
    CROSS_PRODUCT((float *) obj->L,(float *) obj->w,vtmp);
    vtmp[XD] += Ttot[XD];
    vtmp[YD] += Ttot[YD];
    vtmp[ZD] += Ttot[ZD];
    MATxVEC(Iinv_local,vtmp,vtmp2,3,3);
    dcstate_dt[Y_wx] = vtmp2[XD];
    dcstate_dt[Y_wy] = vtmp2[YD];
    dcstate_dt[Y_wz] = vtmp2[ZD];

#if 0
    dcstate_dt[Y_wx]  =
	    (Ttot[XD] - (obj->I[ZD][ZD] - obj->I[YD][YD])*obj->w[ZD]*obj->w[YD])
	    / obj->I[XD][XD];
    dcstate_dt[Y_wy]  = 
	    (Ttot[YD] - (obj->I[XD][XD] - obj->I[ZD][ZD])*obj->w[XD]*obj->w[ZD])
	    / obj->I[YD][YD];
    dcstate_dt[Y_wz]  = 
	    (Ttot[ZD] - (obj->I[YD][YD] - obj->I[XD][XD])*obj->w[YD]*obj->w[XD])
	    / obj->I[ZD][ZD];
#endif


    FREE_MATRIX(I,3,3);
    FREE_MATRIX(Iinv_local,3,3);
    FREE_MATRIX(Mtmp,3,3);
}


void close_physics_state(
    void)
{
    free_quaternion((QUATERNION *) Qtmp);
    FREE_MATRIX(Itmp,3,3);
    FREE_MATRIX(Iinv,3,3);
    FREE_MATRIX(Rtrans,3,3);
    FREE_VECTOR(halfw,3);
    FREE_VECTOR(state,Y_MAX);
}


void compute_state(
    PHYSICAL_OBJECT *object,
    const VECTOR Ftotal,
    const VECTOR Ttotal,
    const float t1,
    const float t2)
{
    int nok,nbad;

    /***** set global variables *****/
    obj = object;
    Ftot = Ftotal;
    Ttot = Ttotal;

    /***** setup initial state *****/
    state[Y_xx]  = obj->x[XD];
    state[Y_xy]  = obj->x[YD];
    state[Y_xz]  = obj->x[ZD];
    state[Y_Qs]  = obj->Q->s;
    state[Y_Qrx] = obj->Q->r[XD];
    state[Y_Qry] = obj->Q->r[YD];
    state[Y_Qrz] = obj->Q->r[ZD];
    state[Y_px]  = obj->p[XD];
    state[Y_py]  = obj->p[YD];
    state[Y_pz]  = obj->p[ZD];
    state[Y_wx]  = obj->w[XD];
    state[Y_wy]  = obj->w[YD];
    state[Y_wz]  = obj->w[ZD];

    bs_odeint(state,Y_MAX,t1,t2,EPSILON,(t2-t1)/5.0,(t2-t1)/20.0,
	    &nok,&nbad,state_derivs);

    normalize_quaternion(obj->Q);
}


float energy(
    const PHYSICAL_OBJECT *whichobj,
    const float g)
{
    float e;

    /* kinetic:  E = .5mv^2 = .5p^2/m */
    e = 0.5 * (whichobj->p[XD] * whichobj->p[XD]
	     + whichobj->p[YD] * whichobj->p[YD]
	     + whichobj->p[ZD] * whichobj->p[ZD]) / whichobj->mass;

    /* rotational:  E = .5w*L */
    e += 0.5 * (whichobj->w[XD] * whichobj->L[XD]
	      + whichobj->w[YD] * whichobj->L[YD]
	      + whichobj->w[ZD] * whichobj->L[ZD]);

    /* potential:  E = mgy */
    e += whichobj->mass * g * whichobj->x[YD];

    return(e);
}
