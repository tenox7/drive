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


/* Numerical integration through Gauss-Legendre method */

#include <stdio.h>
#include <math.h>
#include "libnum.h"
#define FABS(x)		 ((x)<0.0 ? (-(x)) : (x))
#define EPS 		(3.0e-11)
#define FIRST_N		5
#define THRESHOLD	(1.0e-4)	/* relative to answer */
#define MAX_ITERATIONS	8


static void derive_gauss_legendre_values(
    double x1, double x2,	/* Lower and upper integration values */
    double x[], double w[],	/* Evaluation abscissas and weights */
    int n)			/* size of x and w arrays */
{
    int m,j,i;
    double z1,z,xm,xl,pp,p3,p2,p1;

    m = (n+1)/2;	/* roots are symmetric -- only do lower half */
    xm = 0.5*(x2+x1);
    xl = 0.5*(x2-x1);
    for (i=1; i<=m; ++i) {
	/* Approximate the root */
	z = cosf((M_PI*(i-0.25)/(n+0.5)));
	/* Refine using Newton's method */
	do {
	    p1 = 1.0; p2 = 0.0;
	    for (j=1; j<=n; ++j) {
		p3 = p2;
		p2 = p1;
		p1 = ((2.0*j-1.0)*z*p2-(j-1.0)*p3)/j;
	    }
	    /* Now compute derivative of Legendre polynomial */
	    pp = n*(z*p1-p2)/(z*z-1.0);
	    z1 = z;
	    z = z1 - p1/pp;
	} while (FABS(z-z1) > EPS);
	x[i] = xm - xl*z;
	x[n+1-i] = xm+xl*z;	/* Symmetric component */
	w[i] = 2.0*xl/((1.0-z*z)*pp*pp);
	w[n+1-i] = w[i];
    }
}


/* Gauss-Legendre integration of n-points */
static float qgauss_n(
    float (*func)(float),
    float a, float b,
    int n)
{
    int j;
    float s;
    DVECTOR x,w;

    x = dvector(1,n);
    w = dvector(1,n);
    derive_gauss_legendre_values(a,b,x,w,n);

    s = 0;
    for (j=1; j<=n; ++j) {
	s += w[j] * (*func)(x[j]);
    }

    free_dvector(x,1,n);
    free_dvector(w,1,n);
    return (s);
}


/* Gauss-Legendre integration with variable point count */
float gauss_legendre_integrate(
    float (*func)(float),
    float a, float b)
{
    float v,vlast;
    int n,count;


    n = FIRST_N;
    v = qgauss_n(func,a,b,n);
    count = 0;

    do {
	vlast = v;
	n *= 2;
	v = qgauss_n(func,a,b,n);

	/* Stop if the answer hasn't changed much */
	if (vlast < EPS) {
	    if (FABS(v) < EPS) break;
	}
	else {
	    if (FABS((v-vlast)/v) < THRESHOLD) break;
	}
    } while (++count < MAX_ITERATIONS);

    return(v);
}
