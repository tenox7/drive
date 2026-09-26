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


/* Root-finding routines */

#include <stdio.h>
#include <math.h>
#include "libnum.h"

#define EPSILON		(1.0e-12)
#define FABS(x)		(((x)<0.0) ? (-(x)) : (x))
#define DABS(x)		FABS(x)
#define FSGN(x)		(((x)<0.0) ? -1.0 : 1.0)
#define DSGN(x)		FSGN(x)
#define MAXITER		128

/************************* num_findroot ***************************************
 * Finds the root of equation funcd between x1 and x2 within the specified
 * accuracy.
 *
 * Returns the root it found.
 *****************************************************************************/
float num_findroot(
    ROOT_ROUTINE funcd,		/* input -- user-supplied function */
    float x1,		/* input -- starting bound */
    float x2,		/* input -- ending bound */
    float accuracy,	/* input -- find solution within +-accuracy */
    void *function_data)	/* passthru -- passed to the function */
{
    int j;
    float df,dx,dxold,f,fhi,flo;
    float xhi,xlo,rts;

    (*funcd)(x1,&flo,&df,function_data);
    (*funcd)(x2,&fhi,&df,function_data);
    if (flo*fhi >= 0.0) nrerror("Root must be bracketed!");
    if (flo < 0.0) {
	xlo = x1; xhi = x2;
    }
    else {
	float swap;
	xhi = x1; xlo = x2;
	swap = flo; flo = fhi; fhi = swap;
    }

    rts = 0.5*(x1+x2);
    dx = (dxold = FABS(x2-x1));

    (*funcd)(rts,&f,&df,function_data);
    for (j=0; j<MAXITER; ++j) {
	if ((((rts-xhi)*df-f)*((rts-xlo)*df-f) >= 0.0) 
		|| (FABS(2.0*f) > FABS(dxold*df))) {
	    /* Need to use bisection */
	    dxold = dx;
	    dx = 0.5*(xhi-xlo);
	    rts = xlo+dx;
	}
	else {
	    /* Use Newton's method -- faster converging usually */
	    dxold = dx;
	    dx = f/df;
	    rts -= dx;
	}
	if (FABS(dx) < accuracy) return(rts);
	(*funcd)(rts,&f,&df,function_data);
	if (f < 0.0) {
	    xlo = rts; flo = f;
	}
	else {
	    xhi = rts; fhi = f;
	}
    }

    /* else failed to converge */
    nrerror("No convergence in num_findroot");
    return(0.0);
}


/*********************** num_quadratic_solution *******************************
 * Find x such that c2*x^2 + c1*x + c0 = 0.
 * Returns number of real roots (as opposed to imaginary).
 *****************************************************************************/
int num_quadratic_solution(
    float c2,		/* input -- coefficient of x^2 */
    float c1,		/* input -- coefficient of x^1 */
    float c0,		/* input -- coefficient of x^0 */
    float *r0,			/* output -- root1 */
    float *r1)			/* output -- root2 */
{
    if (FABS(c2) < EPSILON) {
	/* solve a linear equation */
	if (FABS(c1) < EPSILON) {
	    return(0);
	}
	else {
	    *r0 = *r1 = -c0 / c1;
	    return(1);
	}
    }
    else {
	double d,q;
	double dc2 = (double) c2;
	double dc1 = (double) c1;
	double dc0 = (double) c0;
	if ((d = dc1*dc1 - 4.0*dc2*dc0) < 0.0) {
	    return(0);
	}
	q = -0.5*(dc1+DSGN(dc1)*sqrt(d));
	*r0 = (float) (q/dc2);
	*r1 = (float) (dc0/q);
	return(2);
    }
}


/*********************** num_cubic_solution **********************************
 * Find x such that c3*x^3 + c2*x^2 + c1*x + c0 = 0.
 * Returns number of real roots (as opposed to imaginary).
 *****************************************************************************/
int num_cubic_solution(
    float c3,	/* input -- coefficient of x^3 */
    float c2,	/* input -- coefficient of x^2 */
    float c1,	/* input -- coefficient of x^1 */
    float c0,	/* input -- coefficient of x^0 */
    float *r0,		/* output -- root 1 */
    float *r1,		/* output -- root 2 */
    float *r2)		/* output -- root 3 */
{
    double dc3,dc2,dc1,dc0;
    double q,r,k,s,t;

    if (FABS(c3) < EPSILON) {
	return(num_quadratic_solution(c2,c1,c0,r0,r1));
    }
    /* Normalize so cubic coefficient is 1.0 */
    dc3 = (double) c3;
    dc2 = ((double) c2)/dc3;
    dc1 = ((double) c1)/dc3;
    dc0 = ((double) c0)/dc3;

    q = (dc2*dc2 - 3.0*dc1) / 9.0;
    r = (2.0*dc2*dc2*dc2 - 9.0*dc2*dc1 + 27.0*dc0) / 54.0;
    t = dc2/3.0;
    if ((k = q*q*q - r*r) >= 0.0) {
	double theta;
	theta = acos(r/sqrt(q*q*q));
	s = -2.0*sqrt(q);
	*r0 = (float) (s*cos(theta             /3.0) - t);
	*r1 = (float) (s*cos((theta+(2.0*M_PI))/3.0) - t);
	*r2 = (float) (s*cos((theta+(4.0*M_PI))/3.0) - t);
	return(3);
    }
    else {
	s = pow(sqrt(-k)+DABS(r),(1.0/3.0));
	*r0 = (float) (-DSGN(r)*(s + q/s) - t);
	return(1);
    }
}
