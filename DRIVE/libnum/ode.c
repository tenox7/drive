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


/* Ordinary differential equation solvers */

#include <stdio.h>
#include "libnum.h"

#define FABS(x) ((x)<0.0 ? (-(x)) : (x))

#define IMAX	11
#define NUSE	7
#define SHRINK	.95
#define GROW	1.2
#define MAXSTP	10000
#define TINY	1.0e-8


/*
 *  rzextr
 *
 * Use diagonal rational function extrapolation to evaluate nv functions
 * at X=0 by fitting a diagonal rational fuction to a sequence of 
 * estimates with progressively smaller values X=xest, and corresponding
 * function vectors yest[1..nv].  This call is number iest in the
 * sequence of calls.  The extrapolation used at most the last nuse
 * estimates.  Extrapolated function values are output as yz[1..nv],
 * and their estimated error is output as dy[1..nv].
 */
static int rzextr(
    int iest,
    float xest,
    VECTOR yest,
    VECTOR yz,
    VECTOR dy,
    int nv,
    int nuse,
    VECTOR x,
    MATRIX d)
{
    int m1,k,j;
    float yy,v,ddy=1,c,b1,b;
    VECTOR fx;

    if ((fx = vector(1,nuse)) == NULL) {
	return(FAILURE);
    }
    x[iest] =  xest;
    if (iest == 1) {
	    for (j=1; j<=nv; ++j) {
		    yz[j] = d[j][1] = dy[j] = yest[j];
	    }
    }
    else {
	    if (iest < nuse) m1 = iest;
	    else m1 = nuse;
	    for (k=1; k<=m1-1; ++k)
		    fx[k+1] = x[iest-k]/xest;
	    for (j=1; j<=nv; ++j) {
		    yy = yest[j];
		    v = d[j][1];
		    c = d[j][1] = yy;
		    for (k=2; k<=m1; ++k) {
			    b1 = fx[k] * v;
			    b = b1-c;
			    if (b) {
				    b = (c-v)/b;
				    ddy = c*b;
				    c = b1*b;
			    }
			    else {
				    ddy = v;
			    }
			    if (k != m1) v = d[j][k];
			    d[j][k] = ddy;
			    yy += ddy;
		    }
		    dy[j] = ddy;
		    yz[j] = yy;
	    }
    }

    free_vector(fx,1,nuse);
    return(SUCCESS);
}


/*               mmid
 * 
 * Modified midpoint step.
 *
 * Input:
 *    y[1..nv]:     dependent variables values at xstart
 *    dydx[1..nv]:  dependent variable derivitives at xstart
 *    htot:         total step to be made
 *    nstep:        number of substeps to be made
 * Output:
 *    yout[1..nv]:  
 */
static int mmid(
    VECTOR y,
    VECTOR dydx,
    int nv,
    float xs,
    float htot,
    int nstep,
    VECTOR yout,
    ODE_DERIVATIVES_ROUTINE derivs)
{
    int n,i;
    float x,swap,h2,h;
    VECTOR ym,yn;


    if ((ym = vector(1,nv)) == NULL) {
	return(FAILURE);
    }
    if ((yn = vector(1,nv)) == NULL) {
	free_vector(ym,1,nv);
	return(FAILURE);
    }

    h = htot/nstep;
    /* First step */
    for (i=1; i<=nv; ++i) {
	    ym[i] = yn[i] = y[i];
	    yn[i] += h*dydx[i];
    }
    x = xs+h;
    /* Use yout for temporary derivative storage */
    (*derivs)(x,yn,yout);
    h2 = 2.0 * h;
    for (n=2; n<=nstep; ++n) {
	    for (i=1; i<=nv; ++i) {
		    swap = ym[i] + h2*yout[i];
		    ym[i] = yn[i];
		    yn[i] = swap;
	    }
	    x += h;
	    (*derivs)(x,yn,yout);
    }

    /* Last step */
    for (i=1; i<=nv; ++i) {
	    yout[i] = 0.5*(ym[i] + yn[i] + h*yout[i]);
    }

    free_vector(yn,1,nv);
    free_vector(ym,1,nv);
    return(SUCCESS);
}



/*
 *          Burlish-Stoer stepper
 *
 * Input:
 *    y[1..nv]:     dependent variables values at xstart
 *    dydx[1..nv]:  dependent variable derivitives at xstart
 *    *xx:          starting x
 *    htry:         stepsize to be attempted
 *    eps:          required accuracy
 *    yscal[1..nv]: how to scale the errors in y to obtain error
 * Output:
 *    y[1..nv]: new values of dependent variable
 *    *xx:      final x
 *    *hdid:    actual stepsize
 *    *hnext:   estimated next stepsize
 * Also:
 *    derives:  calculates rhs deriviatives
 *
 */
static int bsstep(
    VECTOR y,
    VECTOR dydx,
    int nv,
    float *xx,
    float htry,
    float eps,
    VECTOR yscal,
    float *hdid,
    float *hnext,
    ODE_DERIVATIVES_ROUTINE derivs)
{
    int i,j;
    float xsav,xest,h,errmax,temp;
    VECTOR ysav,dysav,yseq,yerr;
    MATRIX d;
    static int nseq[IMAX+1] = {0,2,4,6,8,12,16,24,32,48,64,96};
    static float x[IMAX+1];

    if ((ysav  = vector(1,nv)) == NULL) {
	return(FAILURE);
    }
    if ((dysav = vector(1,nv)) == NULL) {
	free_vector(ysav,1,nv);
	return(FAILURE);
    }
    if ((yseq  = vector(1,nv)) == NULL) {
	free_vector(dysav,1,nv);
	free_vector(ysav,1,nv);
	return(FAILURE);
    }
    if ((yerr  = vector(1,nv)) == NULL) {
	free_vector(yseq,1,nv);
	free_vector(dysav,1,nv);
	free_vector(ysav,1,nv);
	return(FAILURE);
    }
    if ((d     = matrix(1,nv,1,NUSE)) == NULL) {
	free_vector(yerr,1,nv);
	free_vector(yseq,1,nv);
	free_vector(dysav,1,nv);
	free_vector(ysav,1,nv);
	return(FAILURE);
    }

    h = htry;
    xsav = *xx;

    /* save the starting values */
    COPY_VECTOR(y,ysav,1,nv,1);
    COPY_VECTOR(dydx,dysav,1,nv,1);

    while (1) {
	for (i=1; i<=IMAX; ++i) {
	    /* Evaluate sequence of modified midpoint integrations */
	    if (mmid(ysav,dysav,nv,xsav,h,nseq[i],yseq,derivs) == FAILURE) {
		free_matrix(d,1,nv,1,NUSE);
		free_vector(yerr,1,nv);
		free_vector(yseq,1,nv);
		free_vector(dysav,1,nv);
		free_vector(ysav,1,nv);
		return(FAILURE);
	    }
	    /* Square error, since error series is even */
	    temp = h/nseq[i];
	    xest = (temp *= temp);
	    /* Perform rational function extrapolation */
	    if (rzextr(i,xest,yseq,y,yerr,nv,NUSE,&x[1],d) == FAILURE) {
		free_matrix(d,1,nv,1,NUSE);
		free_vector(yerr,1,nv);
		free_vector(yseq,1,nv);
		free_vector(dysav,1,nv);
		free_vector(ysav,1,nv);
		return(FAILURE);
	    }
	    errmax = 0.0;
	    for (j=1; j<=nv; ++j)
		/* Check local truncation error */
		if (errmax < (temp = FABS(yerr[j]/yscal[j]))) errmax = temp;
	    /* Scale accuracy relative to tolerance */
	    if ((errmax /= eps) < 1.0) {
		/* Step converged */
		*xx += h;
		*hdid = h;
		if (i == NUSE) *hnext = h * SHRINK;
		else if (i == (NUSE-1)) *hnext = h * GROW;
		else *hnext = h*nseq[NUSE-1]/nseq[i];
		free_matrix(d,1,nv,1,NUSE);
		free_vector(yerr,1,nv);
		free_vector(yseq,1,nv);
		free_vector(dysav,1,nv);
		free_vector(ysav,1,nv);
		return(SUCCESS);
	    }
	}
	/* Step has failed (which is unusual).  Try smaller step */
	h *= .25;
	for (i=1; i<=(IMAX-NUSE)/2; ++i) h /= 2.0;
	if ((*xx+h) == (*xx)) {
	    nrerror("Step size underflow in BSSTEP");
	    free_matrix(d,1,nv,1,NUSE);
	    free_vector(yerr,1,nv);
	    free_vector(yseq,1,nv);
	    free_vector(dysav,1,nv);
	    free_vector(ysav,1,nv);
	    return(FAILURE);
	}
    }
}


typedef int (*ODE_STEPPER_FUNCTION)(
	float *,
	float *,
	int,
	float *,
	float,
	float,
	float *,
	float *,
	float *,
	ODE_DERIVATIVES_ROUTINE derivs);

/*
 *                     odeint
 *
 * Ordinary Differential Equation integrator with adaptive stepsize
 * control.  Integrate from starting values ystart[1..nv] from x1 to
 * x2 with accuracy eps.  Intermediate results are discarded.
 * h1 should be set as a guessed first stepsize, hmin as the minimum
 * allowed stepsize.  On output, nok and nbad are the number of good 
 * and bad (but retried and fixed) steps taken, and ystart is replaced
 * by values corresponding to x=x2.  derivs is the user-supplied routine
 * for calculating the right-hand side derivatives, while stepper is
 * the stepper routine.
 */
static int odeint(
    VECTOR ystart,
    int nv,
    float x1,
    float x2,
    float eps,
    float h1,
    float hmin,
    int *nok,
    int *nbad,
    ODE_DERIVATIVES_ROUTINE derivs,
    ODE_STEPPER_FUNCTION stepper)
{
    int nstp,i;
    float x,hnext,hdid,h;
    VECTOR yscal,y,dydx;

    if ((yscal = vector(1,nv)) == NULL) {
	return(FAILURE);
    }
    if ((y     = vector(1,nv)) == NULL) {
	free_vector(yscal,1,nv);
	return(FAILURE);
    }
    if ((dydx  = vector(1,nv)) == NULL) {
	free_vector(y,1,nv);
	free_vector(yscal,1,nv);
	return(FAILURE);
    }

    x = x1;
    if (x2 > x1) h = FABS(h1);
    else h = -FABS(h1);
    *nok = *nbad = 0;
    COPY_VECTOR(ystart,y,1,nv,1);
    /* Take at most MAXSTP steps. */
    for (nstp=1; nstp<= MAXSTP; ++nstp) {
	(*derivs)(x,y,dydx);
	/* General-purpose scaling to monitor accuracy */
	for (i=1; i<=nv; ++i)
	    yscal[i] = FABS(y[i])+FABS(dydx[i]*h)+TINY;
	/* If step can overshoot end, cut down stepsize */
	if ((x+h-x2)*(x+h-x1) > 0.0) h = x2-x;
	if ((*stepper)(y,dydx,nv,&x,h,eps,yscal,&hdid,&hnext,derivs)
		== FAILURE) {
	    free_vector(dydx,1,nv);
	    free_vector(y,1,nv);
	    free_vector(yscal,1,nv);
	    return(FAILURE);
	}
	if (hdid == h) ++(*nok);
	else ++(*nbad);
	if ((x-x2)*(x2-x1) >= 0.0) {
	    /* Done! */
	    COPY_VECTOR(y,ystart,1,nv,1);
	    free_vector(dydx,1,nv);
	    free_vector(y,1,nv);
	    free_vector(yscal,1,nv);
	    return(SUCCESS);
	}
	if (FABS(hnext) <= hmin) {
	    nrerror("Step size too small in ODEINT");
	    free_vector(dydx,1,nv);
	    free_vector(y,1,nv);
	    free_vector(yscal,1,nv);
	    return(FAILURE);
	}
	h = hnext;
    }
    nrerror("Too many steps in ODEINT");
    free_vector(dydx,1,nv);
    free_vector(y,1,nv);
    free_vector(yscal,1,nv);
    return(FAILURE);
}


int bs_odeint(
    VECTOR ystart,
    int nv,
    float x1,
    float x2,
    float eps,
    float h1,
    float hmin,
    int *nok,
    int *nbad,
    ODE_DERIVATIVES_ROUTINE derivs)
{
    return(odeint(ystart,nv,x1,x2,eps,h1,hmin,nok,nbad,derivs,bsstep));
}
