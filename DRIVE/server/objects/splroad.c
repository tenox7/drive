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


/* Code Module for the splroad segment object */


#include <stdio.h>
#if defined(__linux__)
# include <values.h>
#endif
#if defined(WIN32)
#include <float.h>
#endif
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"

#define DEFAULT_ROAD_COLOR	ASPHALT_INTENSITY

#define TINY (2.2e-308)
#define SPLINE_MAX_DIM		7
#define DEFAULT_SPLROAD_HEIGHT	0.0

#define Width				(obj->size[SIZE_WIDTH])
#define Height				(obj->size[SIZE_HEIGHT])
#define Count				(obj->count)
#define Dimension			(obj->dimension)
#define Data				(obj->data)

#if defined(__linux__) || defined(WIN32)
# define MAXFLOAT	FLT_MAX
#endif

extern int scene_ed;

float global_width;

typedef struct _splroad_list {
    float width, height;
    int dl_number;
    SPLINE_CURVE c;
    struct _splroad_list *next;
} SPLROAD_LIST;
static SPLROAD_LIST *splroad_list=NULL;

/*****************************************************************
 * plane_height
 * 
 * 	Calculates height on triangle.  The plane throught the
 *	point p0 with a normal vector sc->mc_normal has the
 *	equation:
 *		sc->mc_normal[0] * (x - p0[0]) +
 *		sc->mc_normal[1] * (y - p0[1]) +
 *		sc->mc_normal[2] * (z - p0[2]) == 0
 *	Uses this equation to compute y from (x,z).
 */
static float plane_height(
    float x, float z, float *p0,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    float factor0, factor1;
    float y = 0.0;

    /* Avoid divide by zero (road is nearly vertical) and calculate
     * height.
     */
    if ( IS_NEAR(sc->mc_normal[1],0.0) )
	y = *(p0+1);
    else
    {
	factor0 = sc->mc_normal[0] * (x - *p0);
	factor1 = sc->mc_normal[2] * (z - *(p0+2));
	y = *(p0+1) - (factor0 + factor1) / sc->mc_normal[1];
    }
    return y;
}


/*****************************************************************
 * splroad_surface_chars_xyz
 * 
 * 	Calculates height and normal for spline road.
 *
 */
static int splroad_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    float *p0, *p1, *p2;
    float v0[3], v1[3];
    int vert, coord;

    /* Just deal with the height. */
    sc->mc_x = x;
    sc->mc_z = z;

    /* Work through triangles on road to find x,z intersection
     * and height.
     */
    for (vert=0; vert<obj->count-2; vert++)
    {
	p0 = (float *)(obj->additional_data)+(vert*3);
	p1 = (float *)(obj->additional_data)+((vert+1)*3);
	p2 = (float *)(obj->additional_data)+((vert+2)*3);

	/* Check for x, z in triangle. */
	if ( barycentrictest( x, z, p0, p1, p2 ) )
	{
	    /* Calculate normal. */
	    for (coord=0; coord<3; coord++)
	    {
		v0[coord] = *(p1+coord) - *(p0+coord);
		v1[coord] = *(p2+coord) - *(p0+coord);
	    }
	    NORMALIZE3(v0[0],v0[1],v0[2]);
	    NORMALIZE3(v1[0],v1[1],v1[2]);
	    sc->mc_normal[0] = v0[1] * v1[2] - v0[2] * v1[1];
	    sc->mc_normal[1] = v0[2] * v1[0] - v0[0] * v1[2];
	    sc->mc_normal[2] = v0[0] * v1[1] - v0[1] * v1[0];

	    /* Make sure normal points the right direction! */
	    if (sc->mc_normal[1] < 0.0)
	    {
		sc->mc_normal[0] = -sc->mc_normal[0];
		sc->mc_normal[1] = -sc->mc_normal[1];
		sc->mc_normal[2] = -sc->mc_normal[2];
	    }

	    if (debug)
	    {
		printf("p0 %f %f %f\n", p0[0],p0[1],p0[2]);
		printf("p1 %f %f %f\n", p1[0],p1[1],p1[2]);
		printf("p2 %f %f %f\n", p2[0],p2[1],p2[2]);
		printf("v0 %f %f %f\n", v0[0],v0[1],v0[2]);
		printf("v1 %f %f %f\n", v1[0],v1[1],v1[2]);
		printf("%f %f %f\n",
		       sc->mc_normal[0],sc->mc_normal[1],sc->mc_normal[2]);
	    }
	    sc->mc_y = plane_height( x, z, p0, sc );
	    if (debug) printf("y = %f (%f)\n",sc->mc_y, y);

	    return TRUE;
	}
    }
    return FALSE;
}


/*****************************************************************
 * splroad_evaluate
 *
 * 	Evaluate a single point on a spline road.
 */
static void splroad_evaluate(
    DRIVE_OBJECT *obj,
    DRIVE_OBJECT *child,
    SPLINE_CURVE *c,		/* Curve. */
    int index,			/* Span index. */
    float t,			/* Parametric value to evaluate. */
    float *clist)		/* Polygon vertex list. */
{
    float *fptr;
    float center[SPLINE_MAX_DIM]; /* Point on curve. */
    float tangent[SPLINE_MAX_DIM]; /* Tangent to curve at point. */
    float binormal[SPLINE_MAX_DIM]; /* Tangent to curve at point. */
    int iw = 0, inx = 0, iny = 0, inz = 0;	/* Index to extra data. */
    float road_width;
    int coord;
    boolean_type doing_normal = FALSE;
    boolean_type doing_width = FALSE;

    /* Figure out what data we have specified. */
    switch(c->dimension)
    {
      case 3:
	break;
      case 4:
	doing_width = TRUE;
	iw = 3;
	break;
      case 6:
	doing_normal = TRUE;
	inx = 3; iny = 4; inz = 5;
	break;
      case 7:
	doing_normal = TRUE;
	doing_width = TRUE;	
	inx = 3; iny = 4; inz = 5; iw = 6;
	break;
      default:
	fprintf(stderr,"Invalid spline dimension.  Ignoring extra data\n");
	break;
    }

    /* Initialize pointer to current place in vertex list. */
    fptr = &(clist[index * 6]);

    /* Evaluate curve and first derivative. */
    curve_eval( c, 0, t, center );
    curve_eval( c, 1, t, tangent );
    NORMALIZE3( tangent[0], tangent[1], tangent[2] );
    
    /* Have a normal. */
    if (doing_normal)
    {
	/* Tangent cross normal gives binormal. */
	binormal[0] = tangent[1] * center[inz] - tangent[2] * center[iny];
	binormal[1] = tangent[2] * center[inx] - tangent[0] * center[inz];
	binormal[2] = tangent[0] * center[iny] - tangent[1] * center[inx];
	NORMALIZE3( binormal[0], binormal[1], binormal[2] );
    }
    /* No normal means the road is parallel to ground. */
    else
    {
	/* Tangent cross [0,1,0] gives binormal. */
	binormal[0] = -tangent[2];
	binormal[1] = 0.0;
	binormal[2] = tangent[0];
	NORMALIZE3( binormal[0], binormal[1], binormal[2] );
    }

    if (debug)
    {
	printf("tangent: %f %f %f\n", tangent[0], tangent[1], tangent[2] );
	printf("binormal: %f %f %f\n", binormal[0], binormal[1], binormal[2] );
    }

    if (doing_width)
	road_width = center[iw] / 2.0; /* Width value in coord. */
    else
	road_width = global_width / 2.0; /* Width from file. */

    /* Compute left side of road and update bounding boxes. */
    for (coord=0; coord<3; coord++)
    {
	*fptr = center[coord] + binormal[coord] * road_width;
	obj->bound_mc[coord] = MIN( obj->bound_mc[coord], *fptr );
	obj->bound_mc[coord+3] = MAX( obj->bound_mc[coord+3], *fptr );
	child->bound_mc[coord] = MIN( child->bound_mc[coord], *fptr );
	child->bound_mc[coord+3] = MAX( child->bound_mc[coord+3], *fptr );
	fptr++;
    }

    /* Compute right side of road and update bounding boxes. */
    for (coord=0; coord<3; coord++)
    {
	*fptr = center[coord] - binormal[coord] * road_width;
	obj->bound_mc[coord] = MIN( obj->bound_mc[coord], *fptr );
	obj->bound_mc[coord+3] = MAX( obj->bound_mc[coord+3], *fptr );
	child->bound_mc[coord] = MIN( child->bound_mc[coord], *fptr );
	child->bound_mc[coord+3] = MAX( child->bound_mc[coord+3], *fptr );
	fptr++;
    }
}

/* Increase this to get fewer polygons.  Decrease to get more. */
#define GRAPHICS_DIVISOR	(75.0)
/* Increase this to get smoother physics.  Decrease to get coarser. */
#define PHYSICS_MULTIPLIER	(3)

/*****************************************************************
 * add_splroad_graphics
 *
 * 	Builds a road display segment from a single polynomial
 *	span of the spline.
 */
static int add_splroad_graphics(
    DRIVE_OBJECT *obj,
    DRIVE_OBJECT *child,
    hwObject *oList,
    SPLINE_CURVE *c,		/* Curve. */
    int index,			/* Span index. */
    int evaluations)		/* Number of times to evaluate span. */
{
    float t, t_inc;
    float *graphics_list, *gptr;
    float *physics_list;
    float *bottom_list = NULL, *bptr;
    int i;
    int n_graphics, n_physics, n_bottom;
    int nObjs = 0;
    hwObject curr;

    /* Allocate space for triangle strips. */
    n_graphics = (evaluations+1) * 2;
    n_physics = (evaluations * PHYSICS_MULTIPLIER + 1) * 2;
    n_bottom = n_graphics * 2;
    graphics_list = (float *)malloc( 3 * n_graphics * sizeof(float) );
    physics_list = (float *)malloc( 3 * n_physics * sizeof(float) );
    if (Height > 0.0)
	bottom_list = (float *)malloc( 3 * n_bottom * sizeof(float) );
    if ((graphics_list == NULL) ||
	(physics_list == NULL) ||
	((Height > 0.0) && (bottom_list == NULL)))
    {
	fprintf(stderr,"Out of malloc space!\n");
	return(-1);
    }

    /* Evaluate this span "evaluations" times for graphics. */
    t = c->kvector[index];
    t_inc = (c->kvector[index+1] - c->kvector[index]) / (float)evaluations;
    for (i=0; i<evaluations; i++)
    {
	splroad_evaluate( obj, child, c, i, t, graphics_list );
	t += t_inc;
    }
    /* Make sure to evaluate the final point on the span. */
    splroad_evaluate( obj, child, c, evaluations,
		     c->kvector[index+1], graphics_list );

    /* Evaluate this span "evaluations * multiplier" times for physics. */
    t = c->kvector[index];
    t_inc = (c->kvector[index+1] - c->kvector[index])
	/ (float)(evaluations * PHYSICS_MULTIPLIER);
    for (i=0; i<evaluations*PHYSICS_MULTIPLIER; i++)
    {
	splroad_evaluate( obj, child, c, i, t, physics_list );
	t += t_inc;
    }
    /* Make sure to evaluate the final point on the span. */
    splroad_evaluate( obj, child, c, evaluations*PHYSICS_MULTIPLIER,
		     c->kvector[index+1], physics_list );

    /* Build sides and bottom. */
    if ( Height > 0.0 )
    {
    }

    /* Add fudge to y values of bounding box. */
    child->bound_mc[1] -= Height + BBOX_MARGIN;
    child->bound_mc[4] += BBOX_MARGIN;

    /* Save physics_list for physics operations. */
    child->count = n_physics;
    child->additional_data = (void *)physics_list;

    /* Draw road. */
    curr = hwStrip->create( hwStrip );
    if(		(obj->color[0] == DEFAULT_ROAD_COLOR)
	&&	(obj->color[1] == DEFAULT_ROAD_COLOR)
	&&	(obj->color[2] == DEFAULT_ROAD_COLOR) )
    {
	ASPHALT_HW( curr );
    }
    else {
	ROAD_COLOR_HW(curr, obj->color[0], obj->color[1], obj->color[2]);
    }
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
    curr->modify( curr, hwStrData,
		    HW_MAKE_TYPE(HW_TYPE_FLOAT,n_graphics*3),
		    graphics_list );
    oList[nObjs++] = curr;

    /* Draw sides and bottom. */
    if (Height > 0.0)
    {
	gptr = graphics_list;
	bptr = bottom_list;
	for (i=0; i<n_graphics; i++)
	{
	    *bptr++ = *(gptr+3);
		*bptr++ = *(gptr+4);
		*bptr++ = *(gptr+5);
	    *bptr++ = *(gptr+3);
		*bptr++ = *(gptr+4) - Height;
		*bptr++ = *(gptr+5);
	    *bptr++ = *gptr;
		*bptr++ = *(gptr+1) - Height;
		*bptr++ = *(gptr+2);
	    *bptr++ = *gptr;
		*bptr++ = *(gptr+1);
		*bptr++ = *(gptr+2);
	    i++;
	    gptr += 6;
	}
	curr = hwMesh->create( hwMesh );
	DARK_CONCRETE_HW( curr );
	HW_MODIFY_1I( curr, hwStrGraphN, n_bottom/4 );
	HW_MODIFY_1I( curr, hwStrGraphM, 4 );
	curr->modify( curr, hwStrData,
		    HW_MAKE_TYPE(HW_TYPE_FLOAT,n_bottom*3),
		    bottom_list );
	oList[nObjs++] = curr;
    }

    /* Define stripes. */
    {
	int lanes;			/* Number of lanes. */
	float road_width, w0, w1;	/* Road widths. */
	float *p0, *p1, *p2, *p3; 	/* Vertices of quads. */
	int stripes, stripe;		/* Number of stripes. */
	float spoly[4096], *fptr; 	/* Stripe polygon. */
	float t0, t1, t2, t3;		/* Parametric variables. */
	int n, lns_frm_ctr;		/* Distance in lanes from ctr. */


	/* Traverse the road polygon list looking at each quad. */
	fptr = (float *)spoly;
        for (i=0; i<n_graphics-2; i+=2)
	{
	    /* Use the quad. */
	    p0 = graphics_list + i * 3;
	    p1 = p0 + 3;
	    p2 = p1 + 3;
	    p3 = p2 + 3;

	    /* Find widths of each end of quad. */
	    w0 = FHYPOT3( *p1 - *p0, *(p1+1) - *(p0+1), *(p1+2) - *(p0+2) );
	    w1 = FHYPOT3( *p3 - *p2, *(p3+1) - *(p2+1), *(p3+2) - *(p2+2) );
	    road_width = MIN( w0, w1 );

	    /* Compute number of stripes for this quad. */
	    lanes = ((int)(road_width/(LANE_WIDTH*2.0))) * 2;
	    stripes = lanes - 1;
	    if (lanes < 2)
		continue;

	    /* Draw stripes. */
	    for (stripe=0; stripe<stripes; stripe++)
	    {
		/* Alternate stripes exept for center of wide road. */
		if (!((stripe == (lanes-2)/2) && (lanes > 2)) && (i%4))
		    continue;

		 /* Linear interpolation along edges to get pos. */
		 lns_frm_ctr = stripe - ((stripes - 1) / 2);
		 t0 = t1 = w0/2.0;
		 t2 = t3 = w1/2.0;
		 t0 += (float)lns_frm_ctr * LANE_WIDTH - ROAD_LINE_WIDTH/2;
		 t1 += (float)lns_frm_ctr * LANE_WIDTH + ROAD_LINE_WIDTH/2;
		 t2 += (float)lns_frm_ctr * LANE_WIDTH + ROAD_LINE_WIDTH/2;
		 t3 += (float)lns_frm_ctr * LANE_WIDTH - ROAD_LINE_WIDTH/2;
		 t0 /= w0; t1 /= w0;
		 t2 /= w1; t3 /= w1;

		 /* Compute polygon vertices. */
		 *fptr++ = (1.0 - t0) * *p0 + t0 * *p1;
		     *fptr++ = (1.0 - t0) * *(p0+1) + t0 * *(p1+1)
		         + ROAD_LINE_FLOAT;
		     *fptr++ = (1.0 - t0) * *(p0+2) + t0 * *(p1+2);

		 *fptr++ = (1.0 - t1) * *p0 + t1 * *p1;
		     *fptr++ = (1.0 - t1) * *(p0+1) + t1 * *(p1+1)
		         + ROAD_LINE_FLOAT;
		     *fptr++ = (1.0 - t1) * *(p0+2) + t1 * *(p1+2);

		 *fptr++ = (1.0 - t2) * *p2 + t2 * *p3;
		     *fptr++ = (1.0 - t2) * *(p2+1) + t2 * *(p3+1)
		         + ROAD_LINE_FLOAT;
		     *fptr++ = (1.0 - t2) * *(p2+2) + t2 * *(p3+2);

		 *fptr++ = (1.0 - t3) * *p2 + t3 * *p3;
		     *fptr++ = (1.0 - t3) * *(p2+1) + t3 * *(p3+1)
		         + ROAD_LINE_FLOAT;
		     *fptr++ = (1.0 - t3) * *(p2+2) + t3 * *(p3+2);
	    }
	}
	n = fptr - (float *)spoly;
	if( n > 0 ) {
	    curr = hwQuads->create( hwQuads );
	    ROAD_LINE_YELLOW_HW( curr );
	    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
	    HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
	    curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,n), spoly );
	    oList[nObjs++] = curr;
	}
    }

    if (Height > 0.0)
	free(bottom_list);
    free(graphics_list);

    return nObjs;
}

/*****************************************************************
 * create_splroad_graphics
 *
 * 	Traverse a B-spline space curve.
 */
 void create_splroad_graphics(
    DRIVE_OBJECT *obj,
    SPLROAD_LIST *road)
{
    DRIVE_OBJECT *child;	/* Which child object we're working on. */
    int i;
    float *fptr;
    float length;		/* Control polygon length. */
    int evaluations = 30;	/* Change this to get more/less polys. */
    hwObject oList[1024];
    int nObjs = 0;

    /* Don't define illegal splines. */
    if (road->c.size < road->c.order)
	return;

    /* If we have normals, normalize. */
    fptr = road->c.polygon;
    if (road->c.dimension > 4)
    {
	for (i=0; i<road->c.size; i++)
	{
	    NORMALIZE3( fptr[3], fptr[4], fptr[5] );
	    fptr += road->c.dimension;
	}
    }

    /* For each span, [kvector[i], kvector[i+1]], evaluate the
     * appropriate number of times.
     */
    child = obj->child_list;
    for (i=road->c.order-1; i<road->c.size; i++)
    {
	length = curve_length( &(road->c), i - road->c.order + 1 );
	evaluations = MAX((int)(length / 75.0),1);

	nObjs += add_splroad_graphics( obj, child, oList + nObjs, &(road->c), i,
			     evaluations );
	child = child->next;
    }

    /* Get display list segment.  TBD: LOD */
    obj->display_list = createHwSegmentFromObj( oList, nObjs );

    /* Add fudge to y values of bounding box. */
    obj->bound_mc[1] -= Height + BBOX_MARGIN;
    obj->bound_mc[4] += BBOX_MARGIN;
}

/*****************************************************************
 * find_inital_splroad_bbox
 * 	
 * 	Uses the convex hull property of B-spline control polygons
 *	to set the initial bounding box.
 */
 void find_inital_splroad_bbox(
    DRIVE_OBJECT *obj,
    SPLROAD_LIST *road)
{
    float *fptr;
    int i, coord;

    fptr = road->c.polygon;

    for (i=0; i<road->c.size; i++)
    {
	/* Extra coords. */
	if (road->c.dimension > 3)
	{
	    for (coord=3; coord<road->c.dimension; coord++)
		fptr++;
	}
    }

}


/*****************************************************************
 * init_splroad_object
 * 	
 */
void init_splroad_object(
    DRIVE_OBJECT *obj)
{
    DRIVE_OBJECT *child;
    SPLROAD_LIST *rl;
    int span;

    /* Set up defaults. */
    if (Width <= 0.0)
	Width  = DEFAULT_ROAD_WIDTH;
    if (Height <= 0.0)
	Height  = DEFAULT_SPLROAD_HEIGHT;
    if (obj->color[0] == DEFAULT_OBJECT_COLOR)
	obj->color[0] = DEFAULT_ROAD_COLOR;
    if (obj->color[1] == DEFAULT_OBJECT_COLOR)
	obj->color[1] = DEFAULT_ROAD_COLOR;
    if (obj->color[2] == DEFAULT_OBJECT_COLOR)
	obj->color[2] = DEFAULT_ROAD_COLOR;

    if(debug) printf(" inside init_splroad_%d_object() routine \n",(int)Width);

    /* Create a new one. (Not easy to tell if it's the same as others. */
    if ((rl = (SPLROAD_LIST *) malloc(sizeof(SPLROAD_LIST))) == NULL) {
	fprintf(stderr,"Out of malloc space!\n");
	return;
    }

    /* Physics routines are in children. */
    obj->surface_chars_xyz = NULL;

    /* Initialize (mc) bounding box. */
    obj->bound_mc[0] = MAXFLOAT;
    obj->bound_mc[1] = MAXFLOAT;
    obj->bound_mc[2] = MAXFLOAT;
    obj->bound_mc[3] = -MAXFLOAT;
    obj->bound_mc[4] = -MAXFLOAT;
    obj->bound_mc[5] = -MAXFLOAT;

    /* Set up color. */
    if ((obj->color[0] == DEFAULT_OBJECT_COLOR)
	    || (obj->color[1] == DEFAULT_OBJECT_COLOR)
	    || (obj->color[2] == DEFAULT_OBJECT_COLOR)) {
	/* use concrete */
	obj->color[0] = obj->color[1] = obj->color[2] = ASPHALT_INTENSITY;
    }

    /* Number of children is number of spans in this curve.  For a
     * curve, n - k + 1 spans will be defined where n is number of
     * control points and k is the order.
     */
    obj->child_list = NULL;
    obj->num_children = Count - 3 + 1;

    /* Now create subobjects for each polynomial span. */
    for (span=0; span<obj->num_children; span++)
    {
	if ((child = (DRIVE_OBJECT *)malloc(sizeof(DRIVE_OBJECT))) == NULL)
	{
	    fprintf(stderr,"Out of malloc space!\n");
	    break;
	}
	/* As a good starting point, clone myself. */
	memcpy(child,obj,sizeof(DRIVE_OBJECT));
	/* Now change relevant parts. */
	child->num_children = 0;
	child->child_list = NULL;
	child->display_list = 0;
	child->surface_chars_xyz = splroad_surface_chars_xyz;
	add_object_to_list(&(obj->child_list),child);
    }

    global_width = Width;
    rl->width  = Width;
    rl->height  = Height;
    rl->next   = splroad_list;

    /* Curve parameters. */
    rl->c.order = 3;		/* Always quadratic. */
    rl->c.size = Count;		/* Number of control points. */
    rl->c.dimension = Dimension; /* Spline coordinate dimension. */
    rl->c.polygon = Data;	/* Control polygon. */
    curve_open( &(rl->c) );	/* Always open uniform knot vector. */

    /* Create display. */
    create_splroad_graphics( obj, rl );

    /* NOTE:  If we are in the scene editor, then we CANNOT free the 
    * control polygon -- we need it later!!! */
    if( ! scene_ed )
    {
	free( rl->c.polygon );
	free( rl->c.kvector );
    }

    /* Update bounds. */
    update_wc_bounds(obj);
    child = obj->child_list;
    while (child)
    {
	update_wc_bounds( child );
	child = child->next;
    }

    rl->dl_number = obj->display_list;
    splroad_list  = rl;
}
