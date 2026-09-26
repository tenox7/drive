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


/* Code Module for the curveguardraild road object */

#include <math.h>
#include <stdio.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"

#define DEFAULT_ANGLE		DEGREES_TO_RADIANS(180.0)
#define DEFAULT_RADIUS		(100.0)
#define DEFAULT_HEIGHT		(5.0)
#define DEFAULT_WIDTH		(5.0)

#define MAX_SEGMENTS		64

#define RAIL_WIDTH      0.5
#define RAIL_TOP        3.0
#define RAIL_BOTTOM     2.0
#define RAIL_MID1       (RAIL_BOTTOM+(RAIL_TOP-RAIL_BOTTOM)/4.0)
#define RAIL_MID2       (RAIL_BOTTOM+(RAIL_TOP-RAIL_BOTTOM)/2.0)
#define RAIL_MID3       (RAIL_BOTTOM+(RAIL_TOP-RAIL_BOTTOM)*3.0/4.0)
#define POST_WIDTH      0.5
#define POST_HEIGHT     (RAIL_TOP+0.5)
#define POST_SPACING    25.0

#define WIDTH		(POST_WIDTH+RAIL_WIDTH*2.0)



#define SUBOBJECT_ANGLE		(15.0*M_PI/180.0)

#define MAX_GRAPHICS_CHORD	20.0
#define MAX_GRAPHICS_ANGLE	(30.0*M_PI/180.0)

#define TANANGLE(length,radius) \
	(2.0 * (float) atan((double)((length)/(2.0*(radius)))))

#define Angle			(obj->angle)
#define Radius			(obj->radius)

typedef struct _curveguardrail_list {
    float angle,radius;
    unsigned int nameset_bits;
    int dl_number;
    struct _curveguardrail_list *next;
} CURVEGUARDRAIL_LIST;
static CURVEGUARDRAIL_LIST *curveguardrail_list=NULL;

typedef struct {
    int x,y,z;
} TRIPLE;

static TRIPLE corner[8] = {
    { 0,        1,      2 },
    { 0,        1,      5 },
    { 3,        1,      2 },
    { 3,        1,      5 },
    { 0,        4,      2 },
    { 0,        4,      5 },
    { 3,        4,      2 },
    { 3,        4,      5 },
};


static int curveguardrail_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    float r2,rmin,angle;

    /* check to see if we're really on the curveguardrail. */
    r2 = x*x + z*z;
    if (r2 > Radius*Radius) return(FALSE);
    rmin = Radius-WIDTH;
    if (r2 < rmin*rmin) return(FALSE);
    /* now check angle */
    angle = FATAN2(z,x);
    if (Angle > 0.0) {
	if ((angle > Angle) || (angle < 0.0)) return(FALSE);
    }
    else {
	if ((angle < Angle) || (angle > 0.0)) return(FALSE);
    }

    sc->mc_y = RAIL_TOP;
    if (y < sc->mc_y - 0.2) {
	r2 = FSQRT(r2);
	if (r2 < (Radius - WIDTH/2.0)) {
	    /* on inside */
	    sc->mc_normal[0] = -x/r2;
	    sc->mc_normal[1] = 0.0;
	    sc->mc_normal[2] = -z/r2;
	}
	else {
	    /* on outside */
	    sc->mc_normal[0] = x/r2;
	    sc->mc_normal[1] = 0.0;
	    sc->mc_normal[2] = z/r2;
	}
    }
    return(TRUE);
}


static int curveguardrail_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    float r2,rout2,rin2,angle,x,z;
    int inside,outside,i;

    /* Check to see if you can find a point inside the outside wall
     * and a point outside the inside wall.  If so, the wall is penetrated.
     */
    inside = outside = FALSE;
    rout2 = Radius*Radius;
    rin2  = (Radius - WIDTH) * (Radius - WIDTH);
    for (i=0; i<4; ++i) {
	x = bbox_mc[corner[i].x];
	z = bbox_mc[corner[i].z];
	/* check angle */
	angle = FATAN2(z,x);
	if (Angle > 0.0) {
	    if ((angle > Angle) || (angle < 0.0)) continue;
	}
	else {
	    if ((angle < Angle) || (angle > 0.0)) continue;
	}
	/* good angle, check wall sidedness */
	r2 = x*x + z*z;
	if (r2 <= rout2) inside  = TRUE;
	if (r2 >= rin2)  outside = TRUE;
    }

    if (inside && outside) {
	sc->mc_y = RAIL_TOP;
	return(TRUE);
    }
    else {
	return(FALSE);
    }
}

static int rotated_post(
    float x,float z,
    float ca, float sa,
    hwObject *oList)
{

    static float postmesh[5][2][3] = {
	{{ -POST_WIDTH/2.0,	0.0,		-POST_WIDTH/2.0 },
	{ -POST_WIDTH/2.0,	POST_HEIGHT,	-POST_WIDTH/2.0 }},

	{{ -POST_WIDTH/2.0,	0.0,		POST_WIDTH/2.0 },
	{ -POST_WIDTH/2.0,	POST_HEIGHT,	POST_WIDTH/2.0 }},

	{{ POST_WIDTH/2.0,		0.0,		POST_WIDTH/2.0 },
	{ POST_WIDTH/2.0,		POST_HEIGHT,	POST_WIDTH/2.0 }},

	{{ POST_WIDTH/2.0,		0.0,		-POST_WIDTH/2.0 },
	{ POST_WIDTH/2.0,		POST_HEIGHT,	-POST_WIDTH/2.0 }},

	{{ -POST_WIDTH/2.0,	0.0,		-POST_WIDTH/2.0 },
	{ -POST_WIDTH/2.0,	POST_HEIGHT,	-POST_WIDTH/2.0 }},
    };
    static float top[4][3] = {
	{ -POST_WIDTH/2.0,	POST_HEIGHT,	-POST_WIDTH/2.0 },
	{ POST_WIDTH/2.0,		POST_HEIGHT,	-POST_WIDTH/2.0 },
	{ POST_WIDTH/2.0,		POST_HEIGHT,	POST_WIDTH/2.0 },
	{ -POST_WIDTH/2.0,	POST_HEIGHT,	POST_WIDTH/2.0 }
    };
    float postmeshr[5][2][3],topr[4][3];
    int i,j, nObjs = 0;
    hwObject curr;

    for (i=0; i<5; ++i) {
	for (j=0; j<2; ++j) {
	    postmeshr[i][j][0] = ca*postmesh[i][j][0] - sa*postmesh[i][j][2] + x;
	    postmeshr[i][j][1] = postmesh[i][j][1];
	    postmeshr[i][j][2] = sa*postmesh[i][j][0] + ca*postmesh[i][j][2] + z;
	}
    }
    for (i=0; i<4; ++i) {
	topr[i][0] = ca*top[i][0] - sa*top[i][2] + x;
	topr[i][1] = POST_HEIGHT;
	topr[i][2] = sa*top[i][0] + ca*top[i][2] + z;
    }

    curr = hwMesh->create( hwMesh );
    HW_MODIFY_1I( curr, hwStrGraphN, 5 );
    HW_MODIFY_1I( curr, hwStrGraphM, 2 );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,5*2*3), postmeshr );
    oList[nObjs++] = curr;

    curr = hwPolygon->create( hwPolygon );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,4*3), topr );
    oList[nObjs++] = curr;

    return nObjs;
}



static void create_curveguardrail_graphics(
    DRIVE_OBJECT *obj)
{
    static float rail1[5][3] = {
	{ POST_WIDTH/2.0,		RAIL_BOTTOM,		0.0 },
	{ POST_WIDTH/2.0,		RAIL_TOP,		0.0 },
	{ POST_WIDTH/2.0+RAIL_WIDTH,	RAIL_MID3,		0.0 },
	{ POST_WIDTH/2.0+RAIL_WIDTH/2.0,RAIL_MID2,		0.0 },
	{ POST_WIDTH/2.0+RAIL_WIDTH,	RAIL_MID1,		0.0 },
    };
    static float rail2[5][3] = {
	{ -POST_WIDTH/2.0,		RAIL_BOTTOM,		0.0 },
	{ -POST_WIDTH/2.0-RAIL_WIDTH,	RAIL_MID1,		0.0 },
	{ -POST_WIDTH/2.0-RAIL_WIDTH/2.0,RAIL_MID2,		0.0 },
	{ -POST_WIDTH/2.0-RAIL_WIDTH,	RAIL_MID3,		0.0 },
	{ -POST_WIDTH/2.0,		RAIL_TOP,		0.0 }
    };
    float rail1r[5][3],rail2r[5][3];
    float *mesh1,*mesh2,*fptr1,*fptr2;
    float d_angle,angle1,ca,sa,angle;
    int segments,i,j;
    hwObject curr, oList[1000];
    int nObjs = 0;


    d_angle = POST_SPACING/Radius;
    if ((segments = (int) (Angle/d_angle) + 2) > MAX_SEGMENTS) {
	segments = MAX_SEGMENTS;
	d_angle = Angle/(MAX_SEGMENTS-1) + 0.001;
    }


    if ((mesh1 = (float *) malloc(segments*15*sizeof(float))) == NULL) {
	fprintf(stderr,"Out of malloc space!\n");
	return;
    }
    if ((mesh2 = (float *) malloc(segments*15*sizeof(float))) == NULL) {
	fprintf(stderr,"Out of malloc space!\n");
	free(mesh1);
	return;
    }

    if (Angle < 0.0) {
	angle1 = Angle;
    }
    else {
	angle1 = 0.0;
    }

    /* Offset rails by radius */
    memcpy(rail1r,rail1,5*3*sizeof(float));
    memcpy(rail2r,rail2,5*3*sizeof(float));
    for (i=0; i<5; ++i) {
	rail1r[i][0] += Radius;
	rail2r[i][0] += Radius;
    }

    angle = angle1;
    fptr1 = mesh1;
    fptr2 = mesh2;
    i = 0;
    do {
	ca = FCOS(angle);
	sa = FSIN(angle);
	j = nObjs;
	nObjs += rotated_post(ca*Radius,sa*Radius,ca,sa,oList+nObjs);
	while( j < nObjs ) {
	    WOOD_HW( oList[j] );
	    j++;
	}
	for (j=0; j<5; ++j) {
	    *fptr1++ = ca * rail1r[j][0] - sa * rail1r[j][2];
	    *fptr1++ = rail1r[j][1];
	    *fptr1++ = sa * rail1r[j][0] + ca * rail1r[j][2];

	    *fptr2++ = ca * rail2r[j][0] - sa * rail2r[j][2];
	    *fptr2++ = rail2r[j][1];
	    *fptr2++ = sa * rail2r[j][0] + ca * rail2r[j][2];
	}

	if (angle >= Angle) {
	    ++i;
	    break;
	}
	else if ((angle += d_angle) > Angle) angle = Angle;
    } while ((++i) < segments);

    curr = hwMesh->create( hwMesh );
    HW_MODIFY_1I( curr, hwStrGraphN, i );
    HW_MODIFY_1I( curr, hwStrGraphM, 5 );
    STEEL_HW(curr);
    curr->modify( curr, hwStrData,
		    HW_MAKE_TYPE(HW_TYPE_FLOAT,i*5*3), mesh1 );
    oList[nObjs++] = curr;

    curr = hwMesh->create( hwMesh );
    HW_MODIFY_1I( curr, hwStrGraphN, i );
    HW_MODIFY_1I( curr, hwStrGraphM, 5 );
    STEEL_HW(curr);
    curr->modify( curr, hwStrData,
		    HW_MAKE_TYPE(HW_TYPE_FLOAT,i*5*3), mesh2 );
    oList[nObjs++] = curr;

    free(mesh1);
    free(mesh2);

    /* TBD: LOD */
    curr = hwGroup->create( hwGroup );
    curr->modify( curr, hwStrChildren,
		HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
    HW_OBJECT_NAMESET(curr,obj);

    obj->display_list = createHwSegmentFromObj( &curr, 1 );
}


#define MINX	0
#define MINY	1
#define MINZ	2
#define MAXX	3
#define MAXY	4
#define MAXZ	5

static void init_bounds(
    DRIVE_OBJECT *obj)
{
    float inside_radius = (Radius - WIDTH);

    /* Initialize bounding box values */
    obj->bound_mc[MINY] = 0.0;
    obj->bound_mc[MAXY] = RAIL_TOP + BBOX_MARGIN;
    obj->bound_mc[MAXX] = Radius;

    if (Angle > 0.0) {
	if (Angle < (M_PI/2.0)) {
	    obj->bound_mc[MINX] = FCOS(Angle) * inside_radius;
	    obj->bound_mc[MINZ] = 0.0;
	    obj->bound_mc[MAXZ] = FSIN(Angle) * Radius;
	}
	else if (Angle < M_PI) {
	    obj->bound_mc[MINX] = FCOS(Angle) * Radius;
	    obj->bound_mc[MINZ] = 0.0;
	    obj->bound_mc[MAXZ] = Radius;
	}
	else if (Angle < (M_PI*3.0/2.0)) {
	    obj->bound_mc[MINX] = -Radius;
	    obj->bound_mc[MINZ] = FSIN(Angle) * Radius;
	    obj->bound_mc[MAXZ] = Radius;
	}
	else {
	    obj->bound_mc[MINX] = -Radius;
	    obj->bound_mc[MINZ] = -Radius;
	    obj->bound_mc[MAXZ] = Radius;
	}
    }
    else {
	if (Angle > (-M_PI/2.0)) {
	    obj->bound_mc[MINX] = FCOS(Angle) * inside_radius;
	    obj->bound_mc[MINZ] = FSIN(Angle) * Radius;
	    obj->bound_mc[MAXZ] = 0.0;
	}
	else if (Angle > (-M_PI)) {
	    obj->bound_mc[MINX] = FCOS(Angle) * Radius;
	    obj->bound_mc[MINZ] = -Radius;
	    obj->bound_mc[MAXZ] = 0.0;
	}
	else if (Angle > (-M_PI*3.0/2.0)) {
	    obj->bound_mc[MINX] = -Radius;
	    obj->bound_mc[MINZ] = -Radius;
	    obj->bound_mc[MAXZ] = FSIN(Angle) * Radius;
	}
	else {
	    obj->bound_mc[MINX] = -Radius;
	    obj->bound_mc[MINZ] = -Radius;
	    obj->bound_mc[MAXZ] = Radius;
	}
    }

    update_wc_bounds(obj);
}


void init_curveguardrail_object(
    DRIVE_OBJECT *obj)
{
    CURVEGUARDRAIL_LIST *rl;

    if (Angle  == DEFAULT_OBJECT_ANGLE)  Angle  = DEFAULT_ANGLE;
    if (Radius == DEFAULT_OBJECT_RADIUS) Radius = DEFAULT_RADIUS;

    /* See if we've created one like this before... */
    rl = curveguardrail_list;
    while (rl != NULL) {
	if (IS_NEAR(rl->angle,Angle)
		&& (rl->nameset_bits == obj->nameset_bits)
		&& IS_NEAR(rl->radius,Radius)) {
	    break;
	}
	/* else */
	rl = rl->next;
    }
    if (rl != NULL) {
	/* Good -- I have one like this already. */
	obj->display_list = rl->dl_number;
    }
    else {
	/* Nope -- gotta create a new one. */
	if ((rl = (CURVEGUARDRAIL_LIST *)
		malloc(sizeof(CURVEGUARDRAIL_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}
	rl->angle  = Angle;
	rl->radius = Radius;
	rl->nameset_bits = obj->nameset_bits;
	rl->next   = curveguardrail_list;
    	create_curveguardrail_graphics(obj);
	rl->dl_number = obj->display_list;
	curveguardrail_list  = rl;
    }

    /* xform, ixform initialized elsewhere */

    obj->surface_chars_xyz  = curveguardrail_surface_chars_xyz;
    obj->surface_chars_bbox = curveguardrail_surface_chars_bbox;
    init_bounds(obj);

    /* Now create subobjects to break the wall up into pieces the
     * physics routines can handle (concave objects are bad).
     */
    obj->num_children = 0.0;
    {
	float angle1,angle2;
	static float mat[4][4] = IDENTITY4x4;
	DRIVE_OBJECT *child,*child1;

	/* Create the first child object */
	if ((child1 = (DRIVE_OBJECT *) malloc(sizeof(DRIVE_OBJECT))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}
	/* First, clone the parent */
	memcpy(child1,obj,sizeof(DRIVE_OBJECT));
	/* Then change the relevant parts */
	child1->num_children = 0;
	child1->display_list = 0;
	child1->angle = MIN(SUBOBJECT_ANGLE,Angle);
	init_bounds(child1);
	add_object_to_list(&(obj->child_list),child1);
	++obj->num_children;

	angle1 = SUBOBJECT_ANGLE;
	while (angle1 < Angle) {
	    /* Create the next child object */
	    if ((child = (DRIVE_OBJECT *) malloc(sizeof(DRIVE_OBJECT))) == NULL) {
		fprintf(stderr,"Out of malloc space!\n");
		break;
	    }
	    /* Clone the first child */
	    memcpy(child,child1,sizeof(DRIVE_OBJECT));
	    if ((angle2 = angle1 + SUBOBJECT_ANGLE) > Angle) {
		angle2 = Angle;
		child->angle = Angle - angle1;
	    }
	    /* Change the rotation matrix */
	    mat[0][0] = mat[2][2] = FCOS(angle1);
	    mat[2][0] = -FSIN(angle1);
	    mat[0][2] = -mat[2][0];
	    concat_matrix(mat,obj->xform,child->xform);
	    _hp_invert(child->xform,child->ixform,0);
	    init_bounds(child);
	    add_object_to_list(&(obj->child_list),child);
	    ++obj->num_children;

	    angle1 = angle2;
	}
    }
}
