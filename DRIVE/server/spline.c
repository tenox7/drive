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


/* Code Module for splines */

#include <stdio.h>
#ifdef __linux__
# include <values.h>
#endif
#include "global.h"
#include "drive_server.h"

#define TINY (2.2e-308)
#ifndef MAXFLOAT
# define MAXFLOAT 1e38
#endif

/*****************************************************************
 * barycentrictest
 * 
 * 	Checks to see if a point is inside a triangle.
 *
 */
boolean_type barycentrictest(
    float x, float z,			/* Point to test. */
    float *p0, float *p1, float *p2)	/* Vertices of triangle. */
{
    float alpha, beta;
    float u0, u1, u2, v0, v1, denom;
    
    if ( ( u1 = *p0 - *p2 ) == 0.0 )
    {
	/* Zero area test - optional. */
	if ( ( u2 = *p1 - *p2 ) == 0.0 )
	    return FALSE;
	
	/* Compute intersection point */
	if ( ( ( beta = ( x - *p2 ) / u2 ) < 0.0 ) || ( beta > 1.0 ))
	    return FALSE;
	
	if ( ( v1 = *(p0+2) - *(p2+2) ) == 0.0 )
	    return FALSE;
	
	if ( ( alpha = ( z - *(p2+2) - beta *
			( *(p1+2) - *(p2+2) ) ) / v1 ) < 0.0 )
	    return FALSE;
    }
    else
    {
	if ( ( denom = ( *(p1+2) - *(p2+2) ) * u1 -
	      ( u2 = *p1 - *p2 ) *
	      ( v1 = *(p0+2) - *(p2+2) ) ) == 0.0 )
	    return FALSE;
	
	/* Compute intersection point & subtract 0 vertex */
	u0 = x - *p2;
	v0 = z - *(p2+2) ;
	
	if (((( beta = ( v0 * u1 - u0 * v1 ) / denom ) ) < 0.0 ) ||
	    ( beta > 1.0 ) )
	    return FALSE;
	
	if ( ( alpha = ( u0 - beta * u2 ) / u1 ) < 0.0 )
	    return FALSE;
    }
    
    /* check gamma */
    if ( alpha + beta <= 1.0 )
	return TRUE;
    else
	return FALSE;
}


/*****************************************************************
 * curve_coeff
 * 
 * 	Calculates b-spline derivative coefficients.
 *
 */
void curve_coeff(
    SPLINE_CURVE *c,	/* Curve. */
    int r,			/* Derivative (i.e 1=first derivative). */
    int index,			/* Coefficient index. */
    float *result)		/* Pointer to storage for result. */
{
    int coord;
    float *va, *vb;
    float term0, term1;

    /* Zero'th derivative returns original control vertex. */
    if (r == 0)
    {
	for (coord=0; coord<c->dimension; coord++)
	    result[coord] = c->polygon[index * c->dimension + coord];
    }
    else
    {
	term0 = (float)(c->order - r);
	term1 = c->kvector[index + c->order - r] - c->kvector[index];

	va = (float *)malloc( c->dimension * sizeof(float) );
	vb = (float *)malloc( c->dimension * sizeof(float) );
	if ( (va == NULL) || (vb == NULL) )
	{
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}
	
	curve_coeff( c, r-1, index,   va );
	curve_coeff( c, r-1, index-1, vb );

	if ( fabs(term1) > TINY )
	{
	    for (coord=0; coord<c->dimension; coord++)
		result[coord] = term0 * ((va[coord] - vb[coord]) / term1);
	}
	else
	{
	    for (coord=0; coord<c->dimension; coord++)
		result[coord] = 0.0;
	}

	free( va );
	free( vb );
    }
}


/*****************************************************************
 * curve_eval
 * 
 * 	Evaluates b-spline (or r'th derivative) at t.
 *
 */
void curve_eval(
    SPLINE_CURVE *c,	/* Curve. */
    int r,			/* Derivative (i.e 1=first derivative). */
    float t,			/* Parametric value to evaluate. */
    float *result)		/* Pointer to storage for result. */
{
    int i, coord, p, end;	/* Loop index. */
    int j;			/* Index into knot vector for defined range. */
    float *vtemp;		/* Temporary vertex storage. */
    float term0, term1;		/* Temporary vars for intermediate values. */
    int order;

    /* Find j such that kvector[j] <= t < kvector[j+1] */
    for (j=0; j<c->size + c->order - 1; j++)
	if ( (t >= c->kvector[j]) && (t < c->kvector[j+1]) )
	    break;

    /* Calculate derivative coefficients. */
    vtemp = (float *)malloc( c->size * c->dimension * sizeof(float) );
    if (vtemp == NULL)
    {
	fprintf(stderr,"Out of malloc space!\n");
	return;
    }
    for (i=r; i<c->size; i++)
	curve_coeff( c, r, i, &(vtemp[i * c->dimension]) );

    /* Decrease the order by the derivative. */
    order = c->order - r;

    /* Fix the "from the left" problem at the end of the curve. */
    term0 = fabs( c->kvector[c->size] - t );
    term1 = fabs( c->kvector[c->size + order - 1] - t );
    if ( (term0 < TINY) && (term1 < TINY) )
    {
	for (coord=0; coord<c->dimension; coord++)
	    result[coord] = vtemp[(c->size - 1) * c->dimension + coord];
	free( vtemp );
	return;
    }

    /* De Boor's algorithm. */
    for (p=1; p<order; p++)
    {
	end = j - order + 1 + p;
	for (i=j; i>=end; i--)
	{
	    term0 = (t - c->kvector[i]) /
		(c->kvector[i + order - p] - c->kvector[i]);
	    term1 = (c->kvector[i + order - p] - t) /
		(c->kvector[i + order - p] - c->kvector[i]);

	    for (coord=0; coord<c->dimension; coord++)
		vtemp[i * c->dimension + coord] =
		    term0 * vtemp[i * c->dimension + coord]
			+ term1 * vtemp[(i-1) * c->dimension + coord];
	}
    }

    /* Copy result. */
    for (coord=0; coord<c->dimension; coord++)
	result[coord] = vtemp[j * c->dimension + coord];

    free( vtemp );
}


/*****************************************************************
 * curve_amax
 * 
 * 	Calculates an approximation to the maximum acceleration
 *	over the span "index" from the second derivative control
 *	points.
 */
float curve_amax(
    SPLINE_CURVE *c,	/* Curve. */
    int index)			/* Span index. */
{
    int i, coord, low, high;
    float *result;
    float accel;
    float amax = 0.0;		/* Approximate maximum acceleration. */
    int r = 2;			/* Derivative. */
    
    result = (float *)malloc( c->dimension * sizeof(float) );
    if (result == NULL)
    {
	fprintf(stderr,"Out of malloc space!\n");
	return(-MAXFLOAT);
    }

    /* Find range of control points involved in this span. */
    low = MAX( 0, index - c->order + 1 + r );
    high = MIN( c->size - 1, index );

    /* Calculate magnitudes of control vertices. */
printf("-------acc--------\n");
    for (i=low; i<=high; i++)
    {
	curve_coeff( c, r, i, result ); 
printf("%f %f %f\n", result[0], result[1], result[2] );
	accel = 0.0;
	for (coord=0; coord<3; coord++)
	    accel += result[coord] * result[coord];
	amax = MAX(amax, FSQRT( accel ));
    }
    
    free( result );
    return amax;
}


/*****************************************************************
 * curve_vavg
 * 
 * 	Calculates an approximation to the average velocity
 *	over the span "index" from the first derivative control
 *	points.
 */
float curve_vavg(
    SPLINE_CURVE *c,	/* Curve. */
    int index)			/* Span index. */
{
    int i, coord, low, high;
    float *result, velocity;
    float vavg = 0.0;		/* Average velocity. */
    int r = 1;			/* Derivative. */
    
    result = (float *)malloc( c->dimension * sizeof(float) );
    if (result == NULL)
    {
	fprintf(stderr,"Out of malloc space!\n");
	return(-MAXFLOAT);
    }

    /* Find range of control points involved in this span. */
    low = MAX( 0, index - c->order + 1 + r );
    high = MIN( c->size - 1, index );

printf("-------vel--------\n");
    /* Calculate average velocity. */
    for (i=low; i<=high; i++)
    {
	curve_coeff( c, r, i, result ); 
printf("%f %f %f\n", result[0], result[1], result[2] );
	velocity = 0.0;
	for (coord=0; coord<3; coord++)
	    velocity += result[coord] * result[coord];
	vavg += FSQRT( velocity );
    }
    
    /* Compute average. */
    vavg /= (float)(high - low + 1);

    free( result );
    return vavg;
}


/*****************************************************************
 * curve_open
 *
 * 	Create an open uniform knot vector.
 */
void curve_open(
    SPLINE_CURVE *c)	/* Curve. */
{
    int knot;
    int knotval = 0;

    c->kvector =
	(float *)malloc((c->size+c->order) * c->dimension * sizeof(float));
    if (c->kvector == NULL)
    {
	fprintf(stderr,"Out of malloc space!\n");
	return;
    }

    /* Front end. */
    for (knot=0; knot<c->order; knot++)
	c->kvector[knot] = (float)knotval;

    /* Middle. */
    knotval++;
    while ( knot < c->size )
    {
	c->kvector[knot] = (float)knotval;
	knotval++;
	knot++;
    }

    /* Back end. */
    while ( knot < c->size+c->order )
    {
	c->kvector[knot] = (float)knotval;
	knot++;
    }
}


/*****************************************************************
 * curve_length
 *
 * 	Measure length of control polygon for specified span of
 *	curve.
 */
float curve_length(
    SPLINE_CURVE *c,		/* Curve. */
    int index)			/* Segment index. */
{
    float *p0, *p1;
    float length = 0.0;
    float deltax, deltay, deltaz;
    int vertex;

    p0 = (c->polygon) + (c->dimension) * index;
    p1 = (c->polygon) + (c->dimension) * (index+1);

    for (vertex=0; vertex<c->order-1; vertex++)
    {
	deltax = *p1 - *p0;
	deltay = *(p1+1) - *(p0+1);
	deltaz = *(p1+2) - *(p0+2);

	length += FHYPOT3( deltax, deltay, deltaz );
	
	p0 += c->dimension;
	p1 += c->dimension;
    }

    return length;
}
