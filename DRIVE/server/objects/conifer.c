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


/* Code Module for the conifer object */

#include <stdio.h>
#include <math.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"

#define DEFAULT_HEIGHT	(10.0)
#define DEFAULT_RADIUS	(1.0)

#define Height			(obj->size[SIZE_HEIGHT])
#define Radius			(obj->radius)

typedef struct _conifer_list {
    float radius,height;
    float r,g,b;
    unsigned int nameset_bits;
    int dl_number;
    struct _conifer_list *next;
} CONIFER_LIST;
static CONIFER_LIST *conifer_list=NULL;



static int conifer_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    if (x*x + z*z < Radius*Radius) {
	sc->mc_y = obj->size[SIZE_HEIGHT];
	get_pole_mc_normal(obj,x,y,z,sc->mc_normal);
	return(TRUE);
    }
    /* else */
    return(FALSE);
}

static int conifer_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    float x,y,z;

    find_bbox_point_closest_to_point(bbox_mc, 0.0,0.0,0.0, &x,&y,&z);
	
    if (x*x + z*z < Radius*Radius) {
	sc->mc_y = obj->size[SIZE_HEIGHT];
	return(TRUE);
    }
    /* else */
    return(FALSE);
}

#define MIN_SEG_HEIGHT 5.0
#define MAX_SEG_HEIGHT 15.0
#define MID_SEG_HEIGHT ((MIN_SEG_HEIGHT+MAX_SEG_HEIGHT)/2.0)

void matToAngles( float Mat[4][4], float Angles[3] )
{
    float
	XVec[3], YVec[3],
	TMat[4][4], IMat[4][4], Mat1[4][4], Mat2[4][4],
	Rotate[3],
	dd;

    /* See how X and Y unit direction vectors get xformed */
    XVec[0] = 1.; XVec[1] = 0.; XVec[2] = 0.;
    hwTransform( Mat, XVec );
    dd = sqrt(XVec[0]*XVec[0] + XVec[1]*XVec[1] + XVec[2]*XVec[2]);
    if( dd > 0.0 ) dd = 1.0 / dd;
    XVec[0] *= dd; XVec[1] *= dd; XVec[2] *= dd;

    YVec[0] = 0.; YVec[1] = 1.; YVec[2] = 0.;
    hwTransform( Mat, YVec );
    dd = sqrt(YVec[0]*YVec[0] + YVec[1]*YVec[1] + YVec[2]*YVec[2]);
    if( dd > 0.0 ) dd = 1.0 / dd;
    YVec[0] *= dd; YVec[1] *= dd; YVec[2] *= dd;

    /* Figure out Y rotation angle */
    Rotate[1] = XVec[2];
    if( Rotate[1] < -1. )	Rotate[1] = -1.;
    if( Rotate[1] > 1. )	Rotate[1] = 1.;
    Rotate[1] = -acos( Rotate[1] ) + M_PI / 2.0;

    /* Figure out Z rotation angle */
    Rotate[2] = -atan2( XVec[1], XVec[0] );

    /* Push YVec through inverse of new mat to see what X rot to do */
    Rotate[0] = 0.0;
    hwRotateY( Mat1, Rotate[1] * 180.0 / M_PI );
    hwRotateZ( Mat2, Rotate[2] * 180.0 / M_PI );
    hwMatMult( TMat, Mat1, Mat2 );
    if( hwInvertMat( TMat, IMat ) ) {
	hwTransform( IMat, YVec );
	Rotate[0] = -atan2( YVec[2], YVec[1] );
    }

    Angles[0] = Rotate[0] * 180.0 / M_PI;
    Angles[1] = Rotate[1] * 180.0 / M_PI;
    Angles[2] = Rotate[2] * 180.0 / M_PI;
}

extern void _prim_construct_orientation_matrix(
    float mat[4][4],
    float x1, float y1, float z1,
    float x2, float y2, float z2);

void getRotate
(
    float rot[3],
    float x1, float y1, float z1,
    float x2, float y2, float z2
)
{
    float mat[4][4], dd;
    x2 -= x1; y2 -= y1; z2 -= z1;
    dd = sqrt(x2*x2 + y2*y2 + z2*z2 );
    dd = 1.0f / dd;
    x2 *= dd; y2 *= dd; z2 *= dd;
    _prim_construct_orientation_matrix( mat, 0., 0., 0., x2, y2, z2 );
    matToAngles( mat, rot );
}

static void create_conifer_graphics(
    DRIVE_OBJECT *obj)
{
    float y,ytop,maxradius,radius,angle,x,z, rot[3], dd;
    hwObject curr, oList[100];
    int n, nObjs = 0;

    curr = hwCone->create( hwCone );
    WOOD_HW( curr );
    HW_MODIFY_2F( curr, hwStrRadius, Height/30.0, 0.0 );
    HW_MODIFY_1F( curr, hwStrHeight, Height );
    HW_MODIFY_1I( curr, hwStrGraphN, 2 );
    HW_MODIFY_1I( curr, hwStrGraphM, 4 );
    HW_MODIFY_3F( curr, hwStrRotate, 90.0, 0.0, 0.0 );
    oList[nObjs++] = curr;

    y = BOUNDED_FLOATRAND(Height/8.0,Height/3.0);
    do {
	ytop = BOUNDED_FLOATRAND((Height+y)/2.0,Height);
	maxradius = (Height - y)/2.5;
	radius = FLOATRAND(maxradius/3.5);
	angle = FLOATRAND(2*M_PI);
	x = FCOS(angle) * radius;
	z = FSIN(angle) * radius;
	n = 4 + ZINTRAND(4);

	dd = ytop - y;
	dd = sqrt( x*x + dd*dd + z*z );
	getRotate( rot, x, y, z, 0.0, ytop, 0.0 );

	curr = hwCone->create( hwCone );
	HW_MODIFY_3F( curr, hwStrColor,
		    obj->color[0], obj->color[1], obj->color[2] );
	HW_MODIFY_2F( curr, hwStrRadius, maxradius, 0.0 );
	HW_MODIFY_1F( curr, hwStrHeight, dd );
	HW_MODIFY_1I( curr, hwStrGraphN, 2 );
	HW_MODIFY_1I( curr, hwStrGraphM, n + 1 );
	HW_MODIFY_3F( curr, hwStrRotate, rot[0], rot[1], rot[2] );
	HW_MODIFY_3F( curr, hwStrPos, x, y, z );
	oList[nObjs++] = curr;

	curr = hwDisc->create( hwDisc );
	HW_MODIFY_3F( curr, hwStrColor,
		    obj->color[0], obj->color[1], obj->color[2] );
	HW_MODIFY_1I( curr, hwStrGraphN, n + 1 );
	HW_MODIFY_1F( curr, hwStrRadius, maxradius );
	HW_MODIFY_3F( curr, hwStrRotate, rot[0], rot[1], rot[2] );
	HW_MODIFY_3F( curr, hwStrPos, x, y, z );
	/*HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );*/
	/*HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );*/
	oList[nObjs++] = curr;

	y += (ytop - y)/2.0;
    } while (y < (Height*.8));

    curr = hwGroup->create( hwGroup );
    curr->modify( curr, hwStrChildren,
		HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
    HW_OBJECT_NAMESET(curr,obj);
    obj->display_list = createHwSegmentFromObj( &curr, 1 );
}


void init_conifer_object(
    DRIVE_OBJECT *obj)
{
    CONIFER_LIST *rl;

    if (Height == 0.0) Height = DEFAULT_HEIGHT;
    if (Radius == 0.0) Radius = DEFAULT_RADIUS;
    if ((obj->color[0] == DEFAULT_OBJECT_COLOR)
	    || (obj->color[1] == DEFAULT_OBJECT_COLOR)
	    || (obj->color[2] == DEFAULT_OBJECT_COLOR)) {
	obj->color[0] = BOUNDED_FLOATRAND(0.0,0.3);
	obj->color[1] = BOUNDED_FLOATRAND(0.5,0.9);
	obj->color[2] = BOUNDED_FLOATRAND(0.0,0.6);
    }


    if(debug) printf(" inside init_conifer_object() routine \n");

    obj->num_children = 0;

    /* See if we've created one like this before... */
    rl = conifer_list;
    while (rl != NULL) {
	if (IS_NEAR(rl->radius,Radius)
		&& IS_NEAR(rl->height,Height)
		&& (rl->nameset_bits == obj->nameset_bits)
		&& (ABS(rl->r - obj->color[0]) < 0.1)
		&& (ABS(rl->g - obj->color[1]) < 0.1)
		&& (ABS(rl->b - obj->color[2]) < 0.1)) {
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
	if ((rl = (CONIFER_LIST *) malloc(sizeof(CONIFER_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}
	rl->radius = Radius;
	rl->height = Height;
	rl->r      = obj->color[0];
	rl->g      = obj->color[1];
	rl->b      = obj->color[2];
	rl->nameset_bits = obj->nameset_bits;
	rl->next   = conifer_list;
    	create_conifer_graphics(obj);
	rl->dl_number = obj->display_list;
	conifer_list  = rl;
    }

    obj->surface_chars_xyz  = conifer_surface_chars_xyz;
    obj->surface_chars_bbox = conifer_surface_chars_bbox;

    /* Initial (mc) bounding box values */
    obj->bound_mc[0] = -Radius;
    obj->bound_mc[1] = 0.0;
    obj->bound_mc[2] = -Radius;
    obj->bound_mc[3] = Radius;
    obj->bound_mc[4] = Height;
    obj->bound_mc[5] = Radius;

    /* apply the object's xform matrix to the bounding box to put it in
     * world coordinates */
    update_wc_bounds(obj);

    elevate_object_to_terrain_height((SCENE *) obj->scene,obj,FALSE);
}
