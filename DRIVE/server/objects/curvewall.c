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


/* Code Module for the curvewall road object */

#include <math.h>
#include <stdio.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"

#define DRAW_BOTTOM	TRUE

#define DEFAULT_ANGLE		DEGREES_TO_RADIANS(180.0)
#define DEFAULT_RADIUS		(100.0)
#define DEFAULT_HEIGHT		(5.0)
#define DEFAULT_WIDTH		(5.0)

#define MAX_SUBOBJECT_CHORD	(25.0)
#define MAX_SUBOBJECT_ANGLE	(15.0*M_PI/180.0)

#define TANANGLE(length,radius) \
	(2.0 * (float) atan((double)((length)/(2.0*(radius)))))

#define Height			(obj->size[SIZE_HEIGHT])
#define Width			(obj->size[SIZE_WIDTH])
#define Angle			(obj->angle)
#define Radius			(obj->radius)

extern int scene_ed;

typedef struct _curvewall_list {
    float width,angle,radius,height;
    float r,g,b;
    unsigned int nameset_bits;
    int dl_number;
    struct _curvewall_list *next;
} CURVEWALL_LIST;
static CURVEWALL_LIST *curvewall_list=NULL;

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


static int curvewall_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    float r2,rmin,angle;

    /* check to see if we're really on the curvewall. */
    r2 = x*x + z*z;
    if (r2 > Radius*Radius) return(FALSE);
    rmin = Radius-Width;
    if (r2 < rmin*rmin) return(FALSE);
    /* now check angle */
    angle = FATAN2(z,x);
    if (Angle > 0.0) {
	if ((angle > Angle) || (angle < 0.0)) return(FALSE);
    }
    else {
	if ((angle < Angle) || (angle > 0.0)) return(FALSE);
    }

    sc->mc_y = Height;
    if (y < sc->mc_y - 0.2) {
	r2 = FSQRT(r2);
	if (r2 < (Radius - Width/2.0)) {
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


static int curvewall_surface_chars_bbox(
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
    rin2  = (Radius - Width) * (Radius - Width);
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
	sc->mc_y = Height;
	return(TRUE);
    }
    else {
	return(FALSE);
    }
}


static void create_curvewall_graphics(
    DRIVE_OBJECT *obj)
{
    float *curvewall,*ptr;
    float pgon[4][3];
    float phi,sinp,cosp,d_phi;
    float angle1,angle2;
    float radius2 = Radius - Width;
    int   i,segments;
    int 
	cw=0;
    hwObject
	wGroup,
	objList[6];



    if ((d_phi = MAX_CURVE_CHORD/Radius) > MAX_CURVE_SUBANGLE)
	d_phi = MAX_CURVE_SUBANGLE;
    if (Angle > 0.0) {
	angle1 = 0.0;
	angle2 = Angle;
	segments = (int) (Angle/d_phi) + 2;
    }
    else {
	angle1 = Angle;
	angle2 = 0.0;
	segments = (int) (-Angle/d_phi) + 2;
    }
    if ((curvewall = (float *) malloc(segments*12*sizeof(float))) == NULL) {
	fprintf(stderr,"Out of malloc space!\n");
	return;
    }

    /* outside wall */
    ptr = curvewall;
    phi = angle1;
    i = 0;
    do {
	sinp = FSIN(phi);
	cosp = FCOS(phi);

	*ptr++ = Radius * cosp;
	*ptr++ = Height;
	*ptr++ = Radius * sinp;
	*ptr++ = cosp;
	*ptr++ = 0.0;
	*ptr++ = sinp;

	*ptr++ = Radius * cosp;
	*ptr++ = 0.0;
	*ptr++ = Radius * sinp;
	*ptr++ = cosp;
	*ptr++ = 0.0;
	*ptr++ = sinp;

	if (phi >= angle2) {
	    ++i;
	    break;
	}
	else if ((phi += d_phi) > angle2) phi = angle2;
    } while ((++i) < segments);

    objList[cw] = hwMesh->create(hwMesh);
    objList[cw]->name = 0;
    HW_MODIFY_1I(objList[cw],hwStrGraphN, i);
    HW_MODIFY_1I(objList[cw],hwStrGraphM, 2);
    HW_MODIFY_1B(objList[cw],hwStrHasNormals, HW_TRUE);
    HW_MODIFY_3F(objList[cw],hwStrColor,
		       obj->color[0],obj->color[1],obj->color[2]);
    objList[cw]->modify(objList[cw], hwStrData, 
		HW_MAKE_TYPE(HW_TYPE_FLOAT, i*2*6), curvewall);
    cw++;

    /* inside wall */
    ptr = curvewall;
    phi = angle1;
    i = 0;
    do {
	sinp = FSIN(phi);
	cosp = FCOS(phi);

	*ptr++ = radius2 * cosp;
	*ptr++ = 0.0;
	*ptr++ = radius2 * sinp;
	*ptr++ = -cosp;
	*ptr++ = 0.0;
	*ptr++ = -sinp;

	*ptr++ = radius2 * cosp;
	*ptr++ = Height;
	*ptr++ = radius2 * sinp;
	*ptr++ = -cosp;
	*ptr++ = 0.0;
	*ptr++ = -sinp;

	if (phi >= angle2) {
	    ++i;
	    break;
	}
	else if ((phi += d_phi) > angle2) phi = angle2;
    } while ((++i) < segments);

    objList[cw] = hwMesh->create(hwMesh);
    objList[cw]->name = 0;
    HW_MODIFY_1B(objList[cw],hwStrHasNormals, HW_TRUE);
    HW_MODIFY_1I(objList[cw],hwStrGraphN, i);
    HW_MODIFY_1I(objList[cw],hwStrGraphM, 2);
    HW_MODIFY_3F(objList[cw],hwStrColor,
		       obj->color[0],obj->color[1],obj->color[2]);
    objList[cw]->modify(objList[cw], hwStrData, 
		HW_MAKE_TYPE(HW_TYPE_FLOAT, i*2*6), curvewall);
    cw++;

	/* top */
    ptr = curvewall;
    phi = angle1;
    i = 0;
    do {
	sinp = FSIN(phi);
	cosp = FCOS(phi);

	*ptr++ = radius2 * cosp;
	*ptr++ = Height;
	*ptr++ = radius2 * sinp;
	*ptr++ = 0.0;
	*ptr++ = 1.0;
	*ptr++ = 0.0;

	*ptr++ = Radius * cosp;
	*ptr++ = Height;
	*ptr++ = Radius * sinp;
	*ptr++ = 0.0;
	*ptr++ = 1.0;
	*ptr++ = 0.0;

	if (phi >= angle2) {
	    ++i;
	    break;
	}
	else if ((phi += d_phi) > angle2) phi = angle2;
    } while ((++i) < segments);

    objList[cw] = hwMesh->create(hwMesh);
    objList[cw]->name = 0;
    HW_MODIFY_1B(objList[cw],hwStrHasNormals, HW_TRUE);
    HW_MODIFY_1I(objList[cw],hwStrGraphN, i);
    HW_MODIFY_1I(objList[cw],hwStrGraphM, 2);
    HW_MODIFY_3F(objList[cw],hwStrColor,
		       obj->color[0],obj->color[1],obj->color[2]);
    objList[cw]->modify(objList[cw], hwStrData, 
		HW_MAKE_TYPE(HW_TYPE_FLOAT, i*2*6), curvewall);
    cw++;

#ifdef DRAW_BOTTOM
	/* bottom */
    ptr = curvewall;
    phi = angle1;
    i = 0;
    do {
	sinp = FSIN(phi);
	cosp = FCOS(phi);

	*ptr++ = Radius * cosp;
	*ptr++ = 0.0;
	*ptr++ = Radius * sinp;
	*ptr++ = 0.0;
	*ptr++ =-1.0;
	*ptr++ = 0.0;

	*ptr++ = radius2 * cosp;
	*ptr++ = 0.0;
	*ptr++ = radius2 * sinp;
	*ptr++ = 0.0;
	*ptr++ =-1.0;
	*ptr++ = 0.0;

	if (phi >= angle2) {
	    ++i;
	    break;
	}
	else if ((phi += d_phi) > angle2) phi = angle2;
    } while ((++i) < segments);

    objList[cw] = hwMesh->create(hwMesh);
    objList[cw]->name = 0;
    HW_MODIFY_1B(objList[cw],hwStrHasNormals, HW_TRUE);
    HW_MODIFY_1I(objList[cw],hwStrGraphN, i);
    HW_MODIFY_1I(objList[cw],hwStrGraphM, 2);
    HW_MODIFY_3F(objList[cw],hwStrColor,
		       obj->color[0],obj->color[1],obj->color[2]);
    objList[cw]->modify(objList[cw], hwStrData, 
		HW_MAKE_TYPE(HW_TYPE_FLOAT, i*2*6), curvewall);
    cw++;
#endif /* DRAW_BOTTOM */

    /* end caps */
    ptr = (float *) pgon;
    *ptr++ = radius2;
    *ptr++ = 0.0;
    *ptr++ = 0.0;

    *ptr++ = Radius;
    *ptr++ = 0.0;
    *ptr++ = 0.0;

    *ptr++ = Radius;
    *ptr++ = Height;
    *ptr++ = 0.0;

    *ptr++ = radius2;
    *ptr++ = Height;
    *ptr++ = 0.0;

    objList[cw] = hwPolygon->create(hwPolygon);
    objList[cw]->name = 0;
    HW_MODIFY_1B(objList[cw],hwStrBackface, HW_TRUE);
    HW_MODIFY_1B(objList[cw],hwStrFlipNormals, HW_TRUE);
    HW_MODIFY_3F(objList[cw],hwStrColor,
		   obj->color[0],obj->color[1],obj->color[2]);
    objList[cw]->modify(objList[cw], hwStrData, 
		HW_MAKE_TYPE(HW_TYPE_FLOAT, 4*3), pgon);
    cw++;

    sinp = FSIN(Angle);
    cosp = FCOS(Angle);
    ptr = (float *) pgon;
    *ptr++ = Radius * cosp;
    *ptr++ = 0.0;
    *ptr++ = Radius * sinp;

    *ptr++ = radius2 * cosp;
    *ptr++ = 0.0;
    *ptr++ = radius2 * sinp;

    *ptr++ = radius2 * cosp;
    *ptr++ = Height;
    *ptr++ = radius2 * sinp;

    *ptr++ = Radius * cosp;
    *ptr++ = Height;
    *ptr++ = Radius * sinp;

    objList[cw] = hwPolygon->create(hwPolygon);
    objList[cw]->name = 0;
    HW_MODIFY_1B(objList[cw],hwStrBackface, HW_TRUE);
    HW_MODIFY_1B(objList[cw],hwStrFlipNormals, HW_TRUE);
    HW_MODIFY_3F(objList[cw],hwStrColor,
		   obj->color[0],obj->color[1],obj->color[2]);
    objList[cw]->modify(objList[cw], hwStrData, 
		HW_MAKE_TYPE(HW_TYPE_FLOAT, 4*3), pgon);
    cw++;

    free(curvewall);

    wGroup = hwGroup->create(hwGroup);
    wGroup->name = 0;
    wGroup->modify(wGroup, hwStrChildren, 
	HW_MAKE_TYPE(HW_TYPE_OBJECT, cw),objList);

    obj->display_list = createHwSegmentFromObj(&wGroup, 1);
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
    float inside_radius = (Radius - Width);

    /* Initialize bounding box values */
    obj->bound_mc[MINY] = 0.0;
    obj->bound_mc[MAXY] = Height + BBOX_MARGIN;
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


void init_curvewall_object(
    DRIVE_OBJECT *obj)
{
    CURVEWALL_LIST *rl;

    if (Width  == DEFAULT_OBJECT_SIZE)   Width  = DEFAULT_WIDTH;
    if (Angle  == DEFAULT_OBJECT_ANGLE)  Angle  = DEFAULT_ANGLE;
    if (Radius == DEFAULT_OBJECT_RADIUS) Radius = DEFAULT_RADIUS;
    if (Height == DEFAULT_OBJECT_SIZE)   Height = DEFAULT_HEIGHT;
    if ((obj->color[0] == DEFAULT_OBJECT_COLOR)
	    || (obj->color[1] == DEFAULT_OBJECT_COLOR)
	    || (obj->color[2] == DEFAULT_OBJECT_COLOR)) {
	obj->color[0] = obj->color[1] = obj->color[2] = CONCRETE_INTENSITY;
    }

    /* If the Width is > than the radius, we have a degenerate condition which
     * may cause some serious problems.  So, don't allow it to happen.
     */

    if( Width > Radius) Width = Radius;

    /* See if we've created one like this before... */
    rl = curvewall_list;
    while (rl != NULL) {
	if (IS_NEAR(rl->width,Width)
		&& IS_NEAR(rl->height,Height)
		&& IS_NEAR(rl->angle,Angle)
		&& IS_NEAR(rl->radius,Radius)
		&& (rl->nameset_bits == obj->nameset_bits)
		&& IS_NEAR(rl->r,obj->color[0])
		&& IS_NEAR(rl->g,obj->color[1])
		&& IS_NEAR(rl->b,obj->color[2])) {
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
	if ((rl = (CURVEWALL_LIST *) malloc(sizeof(CURVEWALL_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}
	rl->width  = Width;
	rl->angle  = Angle;
	rl->height = Height;
	rl->radius = Radius;
	rl->r      = obj->color[0];
	rl->g      = obj->color[1];
	rl->b      = obj->color[2];
	rl->nameset_bits = obj->nameset_bits;
	rl->next   = curvewall_list;
    	create_curvewall_graphics(obj);
	rl->dl_number = obj->display_list;
	curvewall_list  = rl;
    }

    /* xform, ixform initialized elsewhere */

    /* Let the physics be handled in the children */
    obj->surface_chars_xyz  = NULL;
    obj->surface_chars_bbox = NULL;

    init_bounds(obj);

    /* Now create subobjects to break the wall up into pieces the
     * physics routines can handle (concave objects are bad).
     */
    obj->num_children = 0;
    {
	float angle1,angle2,d_angle;
	static float mat[4][4] = IDENTITY4x4;
	DRIVE_OBJECT *child,*child1;

	/* Pick an angle that will give a maximum chord of SUBOBJECT_CHORD */
	if (obj->radius < 0.001) d_angle = MAX_SUBOBJECT_ANGLE;
	else {
	    if ((d_angle = MAX_SUBOBJECT_CHORD/obj->radius)
		    > MAX_SUBOBJECT_ANGLE) {
		d_angle = MAX_SUBOBJECT_ANGLE;
	    }
	}

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
	child1->angle = MIN(d_angle,Angle);
	child1->surface_chars_xyz  = curvewall_surface_chars_xyz;
	child1->surface_chars_bbox = curvewall_surface_chars_bbox;
	init_bounds(child1);
	add_object_to_list(&(obj->child_list),child1);
	++obj->num_children;

	angle1 = d_angle;
	while (angle1 < Angle) {
	    /* Create the next child object */
	    if ((child = (DRIVE_OBJECT *) malloc(sizeof(DRIVE_OBJECT))) == NULL) {
		fprintf(stderr,"Out of malloc space!\n");
		break;
	    }
	    /* Clone the first child */
	    memcpy(child,child1,sizeof(DRIVE_OBJECT));
	    if ((angle2 = angle1 + d_angle) > Angle) {
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
