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



/* TRIMMED PLANE MODULE */
#include <stdio.h>
#include "prims.ih"

struct vector3d {
	float x,y,z;
};
typedef struct vector3d VECTOR3D;

static int does_trimming_curves, last_fildes = -1;

/* set does_trimming_curves if possible and set last fildes */
static int check_trimming_curves(
    int fildes)
{
    /* DISABLE FOR NOW */
    does_trimming_curves = FALSE;
    return(FALSE);
}

static VECTOR3D cross_product(
    VECTOR3D p,
    VECTOR3D r)
{
	VECTOR3D c;

	c.x = p.y*r.z - p.z*r.y;
	c.y = p.z*r.x - p.x*r.z;
	c.z = p.x*r.y - p.y*r.x;

	return(c);
}

/* Given vectors along the u and v parametric axis (v1 & v2 repectively)
 * and a reference point p0, what is the u and v of the point p?
 * Pre-compute a bunch of the cross vectors for speed.
 */
static void find_uv(
    VECTOR3D p0Xv1,
    VECTOR3D p0Xv2,
    VECTOR3D v1Xv2,
    VECTOR3D v1,
    VECTOR3D v2,
    VECTOR3D p,
    float *u, float *v)
{
	VECTOR3D pXv1,pXv2;

	pXv1 = cross_product(p,v1);
	pXv2 = cross_product(p,v2);

	if (v1Xv2.x != 0.0) {
		*u = (pXv2.x-p0Xv2.x)/v1Xv2.x;
		*v = (pXv1.x-p0Xv1.x)/(-v1Xv2.x);
	}
	else if (v1Xv2.y != 0.0) {
		*u = (pXv2.y-p0Xv2.y)/v1Xv2.y;
		*v = (pXv1.y-p0Xv1.y)/(-v1Xv2.y);
	}
	else {
		*u = (pXv2.z-p0Xv2.z)/v1Xv2.z;
		*v = (pXv1.z-p0Xv1.z)/(-v1Xv2.z);
	}
}

/* Given a reference point p0 and vectors along the u and v parametric
 * axes and parametric values u and v, what is the point in 3d space?
 */
static VECTOR3D point(
    VECTOR3D p0,
    VECTOR3D vec_u,
    VECTOR3D vec_v,
    float u, float v)
{
	VECTOR3D p;

	p.x = p0.x + u*vec_u.x + v*vec_v.x;
	p.y = p0.y + u*vec_u.y + v*vec_v.y;
	p.z = p0.z + u*vec_u.z + v*vec_v.z;

	return(p);
}


/* Use trimming curves to form the outline of the polygon */
static void trim_curve_plane(
    int fildes,
    float *clist, int poly_size,
    int order,
    int rational,
    int raw)
{
    VECTOR3D p0,p1,p2,v1,v2,v3,v4,va,vb,vc;
    float *ptr,*ptr2,*trim,*poly;
    float plane[12];
    float u,v,x,y,z,umin,umax,vmin,vmax;
    int i,true_poly_size;
    static float twopt_linear_bezier[] = {0.0, 0.0, 1.0, 1.0};
    float u_int,v_int,u_ext,v_ext;
    int coord_type;

    inquire_curve_resolution(fildes,&coord_type,
	    &u_int,&v_int,&u_ext,&v_ext);
    curve_resolution(fildes,DC_VALUES,1000.0,1000.0,10.0,10.0);

    rational = (rational != 0);
    if (rational || raw) true_poly_size = poly_size;
    else true_poly_size = poly_size + order - 1;

    if (rational) {
	    /* de-rationalize the polygon until we're almost done */
	    if ((ptr = poly = _prim_malloc_object(poly_size*(3+rational)))
			    == NULL) {
		    perror("trim_curve_plane");
		    return;
	    }

	    ptr2 = clist;
	    for (i=0;i<poly_size;++i) {
		    x = *(ptr2+3);
		    *ptr++ = *ptr2++ / x;
		    *ptr++ = *ptr2++ / x;
		    *ptr++ = *ptr2++ / x;
		    *ptr++ = *ptr2++;
	    }
    }
    else poly = clist;

    
    /* find three distinct, non-colinear points */
    ptr = poly;
    p0.x = *ptr++; p0.y = *ptr++; p0.z = *ptr++; if (rational) ++ptr;

    i=1; 
    while (++i <= poly_size) {
	    x = *ptr++; y = *ptr++; z = *ptr++; if (rational) ++ptr;
	    v1.x = x - p0.x;
	    v1.y = y - p0.y;
	    v1.z = z - p0.z;
	    if ((v1.x != 0.0) || (v1.y != 0.0) || (v1.z != 0.0)) break;
    }

    if (i > poly_size) {
	    printf("ERROR:  COULD NOT FIND TWO DISTINCT POINTS\n");
	    return;
    }

    while (++i <= poly_size) {
	    v2.x = *ptr++ - x;
	    v2.y = *ptr++ - y;
	    v2.z = *ptr++ - z; if (rational) ++ptr;
	    /* v3 is the normal to the plane of the polygon */
	    v3 = cross_product(v1,v2);
	    if ((v3.x != 0.0) || (v3.y != 0.0) || (v3.z != 0.0)) break;
    }
    
    if (i > poly_size) {
	    printf("ERROR:  COULD NOT FIND THREE NON-COLINEAR POINTS\n");
	    return;
    }

    /* v2 is the reference vector for the v parameter */
    v2 = cross_product(v3,v1);

    /* find the mininum and maximum u,v on the outline */
    ptr = poly+3+rational;
    umin = umax = 0.0;
    vmin = vmax = 0.0;
    va = cross_product(p0,v1);
    vb = cross_product(p0,v2);
    vc = cross_product(v1,v2);
    for(i=1;i<poly_size;++i) {
	    p2.x = *ptr++; p2.y = *ptr++; p2.z = *ptr++;
		    if (rational) ++ptr;
	    find_uv(va,vb,vc,v1,v2,p2,&u,&v);
	    if (u > umax) umax = u;
	    else if (u < umin) umin = u;
	    if (v > vmax) vmax = v;
	    else if (v < vmin) vmin = v;
    }

    /* make the plane bigger */
    u = umax-umin;
    umax += u; umin -= u;
    v = vmax-vmin;
    vmax += v; vmin -= v;

    /* re-parameterize the surface for (0.0,0.0)-(1.0,1.0) */
    p1 = point(p0,v1,v2,umin,vmin);
    v3 = point(p0,v1,v2,umax,vmin);
	    v3.x -= p1.x; v3.y -= p1.y; v3.z -= p1.z;
    v4 = point(p0,v1,v2,umin,vmax);
	    v4.x -= p1.x; v4.y -= p1.y; v4.z -= p1.z;

    /* define a trimming curve using the u&v's of the input points */
    if ((ptr2 = trim = _prim_malloc_object(true_poly_size*(2+rational)))
		    == NULL) {
	    perror("trim_curve_plane");
	    if (rational) _prim_free_object(poly);
	    return;
    }
    ptr = poly;
    va = cross_product(p1,v3);
    vb = cross_product(p1,v4);
    vc = cross_product(v3,v4);
    for(i=0;i<poly_size;++i) {
	    p2.x = *ptr++; p2.y = *ptr++; p2.z = *ptr++;
	    find_uv(va,vb,vc,v3,v4,p2,&u,&v);
	    if (rational) {
		    /* We de-rationalized at the beginning.
		     * Re-rationalize now.
		     */
		    x = *ptr++;
		    *ptr2++ = u * x;
		    *ptr2++ = v * x;
		    *ptr2++ = x;
	    }
	    else {
		    *ptr2++ = u;
		    *ptr2++ = v;
	    }
    }
    /* now repeat them as necessary so the poly will be closed */
    ptr = trim;
    for (;i<true_poly_size;++i) {
	    *ptr2++ = *ptr++;
	    *ptr2++ = *ptr++;
	    if (rational) *ptr2++ = *ptr++;
    }
    define_trimming_curve(fildes,trim,true_poly_size,order,rational);

    ptr = plane;
    /* now send the plane of the poly down as the spline surface */
    *ptr++ = p1.x; *ptr++ = p1.y; *ptr++ = p1.z;
    p0 = point(p1,v3,v4,1.0,0.0);
    *ptr++ = p0.x; *ptr++ = p0.y; *ptr++ = p0.z; 
    p0 = point(p1,v3,v4,0.0,1.0);
    *ptr++ = p0.x; *ptr++ = p0.y; *ptr++ = p0.z; 
    p0 = point(p1,v3,v4,1.0,1.0);
    *ptr++ = p0.x; *ptr++ = p0.y; *ptr++ = p0.z; 

    _prim_push_u_knot(fildes,twopt_linear_bezier,4);
    _prim_push_v_knot(fildes,twopt_linear_bezier,4);
    spline_surface(fildes,plane,2,2,LINEAR,LINEAR,NONRATIONAL);
    _prim_pop_knots(fildes);

    _prim_free_object(trim);
    if (rational) _prim_free_object(poly);
    curve_resolution(fildes,coord_type,u_int,v_int,u_ext,v_ext);
}


/* This routine draws a planar polygon trimmed by the spline curve
 * specified by the control points.  It is used by various primitive
 * calls.
 */
static void dumb_trimmed_plane_center(
    int fildes,
    float *clist, int poly_size,
    int order, 
    int rational,
    float xc, float yc, float zc,
    int raw)
{
    int i,true_poly_size;
    static float twopt_linear_bezier[] = {0.0, 0.0, 1.0, 1.0};
    float *surface,*ptr,*ptr2;
    int coord_type;
    float u_int,v_int,u_ext,v_ext,a;

    inquire_curve_resolution(fildes,&coord_type,
	    &u_int,&v_int,&u_ext,&v_ext);
    curve_resolution(fildes,coord_type,u_int,10000.0,u_ext,10000.0);

    if (rational || raw) true_poly_size = poly_size;
    else true_poly_size = poly_size + order - 1;

    if ((ptr2 = surface = _prim_malloc_object((3+rational)*true_poly_size*2))
		    == NULL) {
	perror("trimmed_plane_center");
	return;
    }

    ptr = clist;
    for (i=0;i<poly_size;++i) {
	    *ptr2++ = *ptr++;
	    *ptr2++ = *ptr++;
	    *ptr2++ = *ptr++;
	    if (rational) *ptr2++ = *ptr++;
    }
    ptr = clist;
    for (;i<true_poly_size;++i) {
	    *ptr2++ = *ptr++;
	    *ptr2++ = *ptr++;
	    *ptr2++ = *ptr++;
	    if (rational) *ptr2++ = *ptr++;
    }

    ptr = clist+3;
    if (rational)
	    for (i=0;i<true_poly_size;++i) {
		    a = *ptr;
		    *ptr2++ = xc * a;
		    *ptr2++ = yc * a;
		    *ptr2++ = zc * a;
		    *ptr2++ = a;
		    ptr += 4;
	    }
    else
	    for (i=0;i<true_poly_size;++i) {
		    *ptr2++ = xc;
		    *ptr2++ = yc;
		    *ptr2++ = zc;
	    }

    if (!raw) _prim_push_default_u(fildes);
    _prim_push_v_knot(fildes,twopt_linear_bezier,4);
    spline_surface(fildes,surface,true_poly_size,2,
	    order,LINEAR,rational);
    _prim_pop_v_knot(fildes);
    if (!raw) _prim_pop_u_knot(fildes);

    _prim_free_object(surface);
    curve_resolution(fildes,coord_type,u_int,v_int,u_ext,v_ext);
}


/* This routine finds the center of a polygon and calls
 * dumb_trimmed_plane_center.
 */
static void dumb_trimmed_plane(
    int fildes,
    float *clist, int poly_size,
    int order,
    int rational,
    int raw)
{
    int i;
    float *ptr;
    float xs,ys,zs,a;


    rational = (rational != FALSE);

    /* find centroid */
    ptr = clist;
    xs = ys = zs = 0.0;
    for (i=0;i<poly_size;++i) {
	    if (rational) a = *(ptr+3);
	    else a = 1.0;
	    xs += *ptr++ / a;
	    ys += *ptr++ / a;
	    zs += *ptr++ / a;
	    if (rational) ++ptr;
    }
    xs /= poly_size; ys /= poly_size; zs /= poly_size;
	    
    dumb_trimmed_plane_center(fildes,clist,poly_size,order,rational,
	    xs,ys,zs,raw);
}


/* Use trimming curves if possible, and a "polar view" if not */
void _prim_trimmed_plane_center(
    int fildes,
    float *clist, int poly_size,
    int order,
    int rational,
    float xc, float yc, float zc,
    int raw)
{
    if (fildes == last_fildes) {
	if (does_trimming_curves)
		trim_curve_plane(fildes,clist,poly_size,order,rational,raw);
	else dumb_trimmed_plane_center(fildes,clist,poly_size,order,
		rational,xc,yc,zc,raw);
    }
    else if (check_trimming_curves(fildes))
		trim_curve_plane(fildes,clist,poly_size,order,rational,raw);
	else dumb_trimmed_plane_center(fildes,clist,poly_size,order,
		rational,xc,yc,zc,raw);
}
	

/* Use trimming curves if possible, and a "polar view" if not */
void _prim_trimmed_plane(
    int fildes,
    float *clist, int poly_size,
    int order,
    int rational,
    int raw)
{
    if (fildes == last_fildes) {
	if (does_trimming_curves)
		trim_curve_plane(fildes,clist,poly_size,order,rational,raw);
	else dumb_trimmed_plane(fildes,clist,poly_size,order,
		rational,raw);
    }
    else if (check_trimming_curves(fildes))
		trim_curve_plane(fildes,clist,poly_size,order,rational,raw);
	else dumb_trimmed_plane(fildes,clist,poly_size,order,
		rational,raw);
}


/*                     TRIMMED_PLANE
 * 
 * This routine draws a planar polygon trimmed by the spline curve
 * defined by clist.  Points will be added as needed to insure that
 * the polygon is closed.
 *
 * fildes    -- integer; file descriptor
 * clist     -- float*; the control points of the trimming curve
 * poly_size -- integer; the number of vertices in clist
 * order     -- integer; the order of the trimming curve
 * rational  -- boolean; whether the control points are rational
 *               (4 floats per vertex)
 * raw       -- boolean; if true, the input polygon is already closed
 *               and use the current u_knot_vector.
 */
void trimmed_plane(
    int fildes,
    float *clist, int poly_size,
    int order,
    int rational,
    int raw)
{
    if ((order == LINEAR) && !rational) {
	polygon3d(fildes,clist,poly_size,FALSE);
    }
    else {
	if (!raw) _prim_push_default_u(fildes);
	_prim_trimmed_plane(fildes,clist,poly_size,order,rational,raw);
	if (!raw) _prim_pop_u_knot(fildes);
    }
}
