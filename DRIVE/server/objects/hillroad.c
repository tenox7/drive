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


/* Code Module for the hillroad segment object */


#include <stdio.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"
#include "scene.h"


#define DEFAULT_ROAD_LENGTH	500.0
#define DEFAULT_ROAD_HEIGHT	0.0
#define EPSILON 		(1e-3)
#define MAX_DROP		2.0
#define T_THRESHOLD		(.045)

#define Length				(obj->size[SIZE_LENGTH])
#define Width				(obj->size[SIZE_WIDTH])

/* Rounds up to nearest multiple of m, skips one if equal */
#define NEAREST_MULTIPLE_UP(k,m) \
    (((k) >= 0.0) ? \
	((float) ((int) ((k)/(m)) + 1) * (m)) : \
	((float) ((int) (((k)+EPSILON)/(m))) * (m)))

/* Rounds down to nearest multiple of m, skips one if equal */
#define NEAREST_MULTIPLE_DOWN(k,m) \
    (((k) > 0.0) ? \
	((float) ((int) (((k)-EPSILON)/(m))) * (m)) : \
	((float) ((int) ((k)/(m)) - 1) * (m)))

/* Rounds down to nearest multiple of m */
#define ROUND_DOWN_CENTERED(k,mult) \
    (((k) > 0.0) ? \
	((float) ((int) (((k)+((mult)/2.0))/(mult)))*(mult) - ((mult)/2.0)) : \
	((float) ((int) (((k)-((mult)/2.0)+EPSILON)/(mult)))*(mult) \
	    - ((mult)/2.0)))

static float max_height;

typedef struct {
    float t;
    float y; /* MCs */
} INFLECTION_POINT;


static int hillroad_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    INFLECTION_POINT *infl = (INFLECTION_POINT *) (obj->additional_data);
    float t = z/Length;
    float p;

    if ((t < 0.0) || (t > 1.0)) return(FALSE);

    sc->friction = 1.0;
    sc->roughness = 0.0;
    sc->roughness_frequency = 0.0;

    while (t > infl->t) ++infl;
    if (IS_NEAR(t,infl->t)) {
	sc->mc_y = infl->y;
    }
    else {
	/* Linear interpolation */
	p = (infl->t - t) / (infl->t - (infl-1)->t);
	sc->mc_y = infl->y * (1.0-p) + (infl-1)->y * p;
    }
    sc->mc_y += ROAD_FLOAT;

    sc->mc_normal[0] = 0.0;
    sc->mc_normal[1] =  ((infl-1)->t - infl->t) * Length;
    sc->mc_normal[2] = -((infl-1)->y - infl->y);
    NORMALIZE2(sc->mc_normal[1],sc->mc_normal[2]);

    /* Override values returned by the hill */
    return(TRUE);
}



#define DELTA	(SCENE_SIZE/(GROUND_MESH_SIZE-1))

static int horizontal_intersections(
    float t[],
    float x1, float z1, float x2, float z2)	/* WCs */
{
    float zstart,zend,z;
    float dz = z2-z1;
    float ti;
    int n;
  
    n = 0;
    if (z1 < z2) {
	zstart = NEAREST_MULTIPLE_UP(z1,DELTA) - (DELTA/2);
	zend   = NEAREST_MULTIPLE_DOWN(z2,DELTA) + (DELTA/2) + EPSILON;
	for (z=zstart; z <= zend; z += DELTA) {
	    ti = (z-z1)/dz;
	    if ((ti > 0.0) && (ti < 1.0)) t[n++] = ti;
	}
    }
    else {
	zstart = NEAREST_MULTIPLE_DOWN(z1,DELTA) + (DELTA/2);
	zend   = NEAREST_MULTIPLE_UP(z2,DELTA) - (DELTA/2) - EPSILON;
	for (z=zstart; z >= zend; z -= DELTA) {
	    ti = (z-z1)/dz;
	    if ((ti > 0.0) && (ti < 1.0)) t[n++] = ti;
	}
    }

    t[n] = 1.0;
    return(n);
}


static int vertical_intersections(
    float t[],
    float x1, float z1, float x2, float z2)	/* WCs */
{
    float xstart,xend,x;
    float dx = x2-x1;
    float ti;
    int n;
    
    n = 0;
    if (x1 < x2) {
	xstart = NEAREST_MULTIPLE_UP(x1,DELTA) - (DELTA/2);
	xend   = NEAREST_MULTIPLE_DOWN(x2,DELTA) + (DELTA/2) + EPSILON;
	for (x=xstart; x <= xend; x += DELTA) {
	    ti = (x-x1)/dx;
	    if ((ti > 0.0) && (ti < 1.0)) t[n++] = ti;
	}
    }
    else {
	xstart = NEAREST_MULTIPLE_DOWN(x1,DELTA) + (DELTA/2);
	xend   = NEAREST_MULTIPLE_UP(x2,DELTA) - (DELTA/2) - EPSILON;
	for (x=xstart; x >= xend; x -= DELTA) {
	    ti = (x-x1)/dx;
	    if ((ti > 0.0) && (ti < 1.0)) t[n++] = ti;
	}
    }

    t[n] = 1.0;
    return(n);
}


static int diagonal_intersections(
    float t[],
    float x1, float z1, float x2, float z2)	/* WCs */
{
    float xa;
    float xstart,xend,x,ti;
    float xc = (x1+x2)/2.0;
    float zc = (z1+z2)/2.0;
    float z0;
    int n;

    /* Do we need to march left to right or vice versa? */
    xa = (x2-x1)*COS45 - (z2-z1)*SIN45;
    z0 = ROUND_DOWN_CENTERED(zc,SCENE_SIZE);

    n = 0;
    if (xa >= 0.0) {
	xstart = ROUND_DOWN_CENTERED(xc,SCENE_SIZE) - SCENE_SIZE;
	xend   = xstart + (SCENE_SIZE*2);
	for (x=xstart; x<xend; x += DELTA) {
	    ti = ((x-z0) - (x1-z1)) / ((x2-x1)-(z2-z1));
	    if ((ti > 0.0) && (ti < 1.0)) t[n++] = ti;
	}
    }
    else {
	xstart = ROUND_DOWN_CENTERED(xc,SCENE_SIZE) + SCENE_SIZE;
	xend   = xstart - (SCENE_SIZE*2);
	for (x=xstart; x>xend; x -= DELTA) {
	    ti = ((x-z0) - (x1-z1)) / ((x2-x1)-(z2-z1));
	    if ((ti > 0.0) && (ti < 1.0)) t[n++] = ti;
	}
    }

    t[n] = 1.0;
    return(n);
}


static int cmp_floats(
    const void *f1p, const void *f2p)
{
    float f1 = *((float *) f1p);
    float f2 = *((float *) f2p);

    if (f1 < f2) return(-1);
    else if (f1 > f2) return(1);
    else return(0);
}


#define SET_POSITION(i,t)				\
{   							\
    /* Is the point higher on left or right? */		\
    xl[i] = wcx1 + (t)*dx1;				\
    zl[i] = wcz1 + (t)*dz1;				\
    yl[i] = hill_y_wc(hill,xl[i],zl[i]) + ROAD_FLOAT;	\
							\
    xr[i] = wcx3 + (t)*dx3;				\
    zr[i] = wcz3 + (t)*dz3;				\
    yr[i] = hill_y_wc(hill,xr[i],zr[i]) + ROAD_FLOAT;	\
							\
    /* Make high on the left, low on the right */	\
    if (yr[i] > yl[i]) {				\
	float _tmp;					\
	_tmp = yl[i]; yl[i] = yr[i]; yr[i] = _tmp;	\
    }							\
}

static void make_hillroad_mesh(
    DRIVE_OBJECT *obj,
    DRIVE_OBJECT *hill,
    float zstart, float len,
    int *mesh_size,
    float *mesh)
{
    float mcx1,mcz1,mcx2,mcz2,mcx3,mcz3,mcx4,mcz4;
    float wcx1,wcz1,wcx2,wcz2,wcx3,wcz3,wcx4,wcz4;
    float dx1,dz1,dx3,dz3;
    float fjunk,l,old_ymin;
    float dh,dv,dd;
    float *fptr;
    int i,j;
    int num_pts;
    float  t[(GROUND_MESH_SIZE+3)*9];
    float xr[(GROUND_MESH_SIZE+3)*9],
	  yr[(GROUND_MESH_SIZE+3)*9],
	  zr[(GROUND_MESH_SIZE+3)*9];
    float xl[(GROUND_MESH_SIZE+3)*9],
  	  yl[(GROUND_MESH_SIZE+3)*9],
	  zl[(GROUND_MESH_SIZE+3)*9];
    INFLECTION_POINT *infl;

    mcx1 = -Width/2.0;   mcz1 = zstart;
    mcx2 = -Width/2.0;   mcz2 = zstart + len;
    mcx3 =  Width/2.0;   mcz3 = zstart;
    mcx4 =  Width/2.0;   mcz4 = zstart + len;
    mc_to_wc(obj,mcx1,0.0,mcz1, &wcx1,&fjunk,&wcz1);
    mc_to_wc(obj,mcx2,0.0,mcz2, &wcx2,&fjunk,&wcz2);
    mc_to_wc(obj,mcx3,0.0,mcz3, &wcx3,&fjunk,&wcz3);
    mc_to_wc(obj,mcx4,0.0,mcz4, &wcx4,&fjunk,&wcz4);

    /* There are three axes to intersect with.
     * Ignore any that run parallel to the road.
     */
    l = HYPOT2(wcx2-wcx1,wcz2-wcz1);
    dh = ABS((wcx2-wcx1)/l);
    dv = ABS((wcz2-wcz1)/l);
    dd = ABS(((wcz2-wcz1)*M_SQRT1_2 + (wcx2-wcx1)*M_SQRT1_2)/l);

    t[0] = 0.0; num_pts = 1;

    if (dh < (1.0-EPSILON)) {
	/* Intersections with horizontal lines */
	i = horizontal_intersections(&(t[num_pts]),wcx1,wcz1,wcx2,wcz2);
	num_pts += i;
	i = horizontal_intersections(&(t[num_pts]),wcx3,wcz3,wcx4,wcz4);
	num_pts += i;
    }

    if (dv < (1.0-EPSILON)) {
	/* Intersections with vertical lines */
	i = vertical_intersections(&(t[num_pts]),wcx1,wcz1,wcx2,wcz2);
	num_pts += i;
	i = vertical_intersections(&(t[num_pts]),wcx3,wcz3,wcx4,wcz4);
	num_pts += i;
    }

    if (dd < (1.0-EPSILON)) {
	/* Intersections with diagonal lines */
	i = diagonal_intersections(&(t[num_pts]),wcx1,wcz1,wcx2,wcz2);
	num_pts += i;
	i = diagonal_intersections(&(t[num_pts]),wcx3,wcz3,wcx4,wcz4);
	num_pts += i;
    }

    t[num_pts] = 1.0; ++num_pts;

    /* Sort the intersection points */
    qsort((void *) t,num_pts,sizeof(float),cmp_floats);

    /* Remove duplicates */
    for (i=1; i<num_pts-1; ++i) {
	if (IS_NEAR(t[i],t[i-1])) {
	    for (j=i; j<num_pts-1; ++j) t[j] = t[j+1];
	    --num_pts;
	}
    }

    dx1 = wcx2-wcx1;  dz1 = wcz2-wcz1;
    dx3 = wcx4-wcx3;  dz3 = wcz4-wcz3;

    /* Set up the initial arrays */
    for (i=0; i<num_pts; ++i) {
	SET_POSITION(i,t[i]);
    }

    /* Smoothing */
    for (i=1; i<num_pts-1; ++i) {
	if ((yl[i-1] - yl[i] > MAX_DROP)
		&& (yl[i+1] - yl[i] > MAX_DROP)) {
	    float t1,t2;
	    /* insert a smoothing point here, raise the road level */
	    t1 = (t[i] + t[i-1]) / 2.0;
	    t2 = (t[i] + t[i+1]) / 2.0;

	    /* Shuffle everyone up a notch */
	    for (j=num_pts+1; j>i; --j) {
		xl[j+1] = xl[j]; yl[j+1] = yl[j]; zl[j+1] = zl[j];
		xr[j+1] = xr[j]; yr[j+1] = yr[j]; zr[j+1] = zr[j];
		t[j+1] = t[j];
	    }
	    /* Increment num_pts, since we've added a new one. */
	    ++num_pts;

	    /* Set the old point using a t halfway between the old and
	     * the previous t.
	     */
	    old_ymin = yr[i];
	    SET_POSITION(i,t1);
	    /* Add a new point at a t halfway between the old and next */
	    SET_POSITION(i+1,t2);
	    t[i]   = t1;
	    t[i+1] = t2;

	    /* Set it to run deep enough to hit the low point of the valley */
	    yr[i]   = old_ymin;
	    yr[i+1] = old_ymin;

	    /* Now raise it some more */
	    yl[i]   = (yl[i] + yl[i-1]) / 2.0;
	    yl[i+1] = (yl[i+1] + yl[i+2]) / 2.0;

	    /* No need to check the next one or even the one following that. */
	    i += 2;
	}
    }

    if ((infl = (INFLECTION_POINT *)
	    malloc(num_pts*sizeof(INFLECTION_POINT))) == NULL) {
	fprintf(stderr,"Out of space!!!\n");
	exit(1);
    }
    obj->additional_data = (void *) infl;

    /* Convert from WCs to MCs */
    fptr = (float *) mesh;
    for (i=0; i<num_pts; ++i) {
	wc_to_mc(obj, xr[i],yr[i],zr[i], fptr,fptr+1,fptr+2);
	fptr += 3;
    }
    for (i=0; i<num_pts; ++i) {
	wc_to_mc(obj, xr[i],yl[i],zr[i], fptr,fptr+1,fptr+2);
	infl->t = t[i];
	if ((infl->y = *(fptr+1)) > max_height) max_height = infl->y;
	fptr += 3;
	++infl;
    }
    for (i=0; i<num_pts; ++i) {
	wc_to_mc(obj, xl[i],yl[i],zl[i], fptr,fptr+1,fptr+2);
	fptr += 3;
    }
    for (i=0; i<num_pts; ++i) {
	wc_to_mc(obj, xl[i],yr[i],zl[i], fptr,fptr+1,fptr+2);
	fptr += 3;
    }

    *mesh_size = num_pts;
}


static int add_stripe(
    DRIVE_OBJECT *obj,
    float x1, float x2,
    float t1, float t2,
    INFLECTION_POINT *p1,
    float *pgon )
{
    INFLECTION_POINT *p2 = p1 + 1;
    float p,y1,y2,tend;
    int result;

    if (t2 > p2->t) {
	/* break the stripe up into two parts */
	tend = p2->t;
    }
    else {
	tend = t2;
    }

    p  = (t1 - p1->t) / (p2->t - p1->t);
    y1 = p1->y * (1.0-p) + p2->y * p + ROAD_FLOAT;

    p  = (tend - p1->t) / (p2->t - p1->t);
    y2 = p1->y * (1.0-p) + p2->y * p + ROAD_FLOAT;

    pgon[0*3+0] = pgon[3*3+0] = x1;
    pgon[1*3+0] = pgon[2*3+0] = x2;

    pgon[0*3+1] = pgon[1*3+1] = y1;
    pgon[2*3+1] = pgon[3*3+1] = y2;

    pgon[0*3+2] = pgon[1*3+2] = t1  *Length;
    pgon[2*3+2] = pgon[3*3+2] = tend*Length;

    result = 4;

    if (tend != t2) {
	/* finish it. */
	result += add_stripe(obj,x1,x2,tend,t2,p2,pgon+3*result);
    }
    return result;
}


static hwObject add_hillroad_stripes(
    DRIVE_OBJECT *obj)
{
    INFLECTION_POINT *infl = (INFLECTION_POINT *) (obj->additional_data);
    INFLECTION_POINT *iptr;
    int lanes,lane;
#if !defined(HOVERWARE_MODEL)
    int stripe_seg;
#endif
    float z,z2,x,t1,t2;
    static float pgon[4096*3];
    int numPts = 0;
    hwObject curr;

    if (Width < (LANE_WIDTH*4.0-0.5)) {
	/* dotted line only */
	iptr = infl;
	for (z=0.0; z<Length; z+=ROAD_LINE_SPACING) {
	    t1 = z/Length;
	    while (iptr->t <= t1) ++iptr; /* go to the first one past t1 */
	    --iptr;  /* back up one */
	    if ((z2 = z + ROAD_LINE_LENGTH) > Length) z2 = Length;
	    t2 = z2/Length;

	    numPts += add_stripe(obj,
		    (-ROAD_LINE_WIDTH/2.0),(ROAD_LINE_WIDTH/2.0),
		    t1,t2,iptr,pgon+3*numPts);	
	}
    }
    else {
	/* More than two lanes */
	lanes = ((int) (Width/(LANE_WIDTH*2.0))) * 2;
	for (lane = 0, x = -LANE_WIDTH*(lanes/2-1);
		lane < lanes-1;
		++lane, x += LANE_WIDTH) {
	    if (lane == (lanes/2-1)) {
		/* Center stripes */
		numPts += add_stripe(obj,(-ROAD_LINE_WIDTH/2.0)-ROAD_LINE_WIDTH,
		    (ROAD_LINE_WIDTH/2.0)-ROAD_LINE_WIDTH,
		    0.0,1.0,infl,pgon+3*numPts);	
		numPts += add_stripe(obj,(-ROAD_LINE_WIDTH/2.0)+ROAD_LINE_WIDTH,
		    (ROAD_LINE_WIDTH/2.0)+ROAD_LINE_WIDTH,
		    0.0,1.0,infl,pgon+3*numPts);
	    }
	    else {
		/* striped lane markers */
		iptr = infl;
		for (z=0.0; z<Length; z += ROAD_LINE_SPACING) {
		    t1 = z/Length;
		    while (iptr->t <= t1) ++iptr;
			/* go to the first one past t1 */
		    --iptr;  /* back up one */
		    if ((z2 = z + ROAD_LINE_LENGTH) > Length) z2 = Length;
		    t2 = z2/Length;

		    numPts += add_stripe(obj,x-(ROAD_LINE_WIDTH/2.0),
			x+(ROAD_LINE_WIDTH/2.0),
			t1,t2,iptr, pgon+3*numPts);	
		}
	    }
	}
    }
    curr = hwQuads->create( hwQuads );
    curr->modify( curr, hwStrData, HW_MAKE_TYPE(HW_TYPE_FLOAT,3*numPts), pgon );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
    HW_OBJECT_NAMESET(curr,obj);
    ROAD_LINE_YELLOW_HW(curr);

    return curr;
}


static void create_hillroad_graphics(
    DRIVE_OBJECT *obj,
    DRIVE_OBJECT *hill)
{
    float
	mesh[(GROUND_MESH_SIZE+3)*18+8][3];
    int
	mesh_size;
    hwObject
	oList[10], curr;
    int
	nObjs = 0;

    make_hillroad_mesh(obj, hill, 0.0, Length,
	&mesh_size, (float *) mesh);

    if (Width > (LANE_WIDTH*2.0-0.5)) {
	curr = add_hillroad_stripes(obj);
	oList[nObjs++] = curr;
	/* TBD: LOD */
    }

    curr = hwMesh->create( hwMesh );
    HW_MODIFY_1I( curr, hwStrGraphN, 4 );
    HW_MODIFY_1I( curr, hwStrGraphM, mesh_size );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,4*mesh_size*3), mesh );
    HW_OBJECT_NAMESET(curr,obj);
    ASPHALT_HW(curr);
    oList[nObjs++] = curr;

    curr = hwGroup->create( hwGroup );
    curr->modify( curr, hwStrChildren,
		HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
    obj->display_list = createHwSegmentFromObj( &curr, 1 );
}


void init_hillroad_object(
    DRIVE_OBJECT *obj)
{
    DRIVE_OBJECT *hill;

    if (Length <= 0.0) Length = DEFAULT_ROAD_LENGTH;
    if (Width <= 0.0)  Width  = DEFAULT_ROAD_WIDTH;

    if ((hill = find_hill_in_scene((SCENE *) (obj->scene))) == NULL) {
	return;
    }

    create_hillroad_graphics(obj,hill);

    obj->num_children = 0;
    obj->surface_chars_xyz  = hillroad_surface_chars_xyz;

    /* Initialize bounding box values */
    obj->bound_mc[0] = -Width/2.0;
    obj->bound_mc[1] = 0.0;
    obj->bound_mc[2] = 0.0;
    obj->bound_mc[3] = Width/2.0;
    obj->bound_mc[4] = max_height + BBOX_MARGIN;
    obj->bound_mc[5] = Length;

    update_wc_bounds(obj);
}
