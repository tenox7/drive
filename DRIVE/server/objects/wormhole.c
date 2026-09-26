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


/* Code Module for the wormhole segment object */


#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"
#include "message.h"
#include "connection.h"

#define Length	(obj->size[SIZE_LENGTH])
#define Height	(obj->size[SIZE_HEIGHT])
#define Width	(obj->size[SIZE_WIDTH])
#define Radius	(obj->radius)

#define DEFAULT_WORMHOLE_LENGTH		(1.0)
#define DEFAULT_WORMHOLE_HEIGHT		(40.0)
#define DEFAULT_WORMHOLE_WIDTH		(60.0)
#define DEFAULT_WORMHOLE_ANGLE		(0.0)
#define DEFAULT_WORMEHOLE_X		(0.0)
#define DEFAULT_WORMEHOLE_Y		(1.0)
#define DEFAULT_WORMEHOLE_Z		(0.0)

#if !defined(PHYSICS_VERSION)
# define PHYSICS_VERSION 1
#endif

#if PHYSICS_VERSION == 1
# define INV_ROTMAT(obj)	((obj)->Ri)
#else
# define INV_ROTMAT(obj)	((obj)->Rinv)
#endif

#ifndef EPSILON
# define EPSILON (1.0e-6)
#endif

extern OBJECT_ID object_id[];
extern int turbo_boosts;

typedef struct _wormhole_list {
    float length, height, width;
    int type;
    unsigned int nameset_bits;
    int dl_number;
    struct _wormhole_list *next;
} WORMHOLE_LIST;
static WORMHOLE_LIST *wormhole_list=NULL;


static int pole_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
#if 0
    if (x*x + z*z < Radius*Radius) {
	sc->mc_y = Height;
	get_pole_mc_normal(obj,x,y,z,sc->mc_normal);
	return(TRUE);
    }
    /* else */
#endif
    return(FALSE);
}

static int pole_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = 0.0;
    return(TRUE);
}


#define PGON_OFFSET	(-0.002)
#define TYPE_NORMAL	0
#define TYPE_BLACK	1
#define TYPE_INVISIBLE	2

static void create_wormhole_graphics(
    DRIVE_OBJECT *obj,
    int type)
{
    float pgon[4][3];
    float mesh[3][3][3];
    hwObject
	curr, oList[5];
    int
	nObjs = 0;

    /* Define lines that outline the width/height dimensions of the wormhole,
     * in placed in the center of the Length range */
    
    if( type == TYPE_INVISIBLE) 
    {
	obj->display_list = INVALID;
	return;
    }

    pgon[0][0] = -Width/2.0;
    pgon[0][1] = 0.0;
    pgon[0][2] = 0.0;

    pgon[1][0] = Width/2.0;
    pgon[1][1] = 0.0;
    pgon[1][2] = 0.0;

    pgon[2][0] = Width/2.0;
    pgon[2][1] = Height;
    pgon[2][2] = 0.0;

    pgon[3][0] = -Width/2.0;
    pgon[3][1] = Height;
    pgon[3][2] = 0.0;

    mesh[0][0][0] = -Width/2.0;
    mesh[0][0][1] = 0.0;
    mesh[0][0][2] = PGON_OFFSET;

    mesh[0][1][0] = 0.0;
    mesh[0][1][1] = 0.0;
    mesh[0][1][2] = 0.0;

    mesh[0][2][0] = Width/2.0;
    mesh[0][2][1] = 0.0;
    mesh[0][2][2] = PGON_OFFSET;

    mesh[1][0][0] = -Width/2.0;
    mesh[1][0][1] = Height/2.0;
    mesh[1][0][2] = 0.0;

    mesh[1][1][0] = 0.0;
    mesh[1][1][1] = Height/2.0;
    mesh[1][1][2] = -PGON_OFFSET;

    mesh[1][2][0] = Width/2.0;
    mesh[1][2][1] = Height/2.0;
    mesh[1][2][2] = 0.0;

    mesh[2][0][0] = -Width/2.0;
    mesh[2][0][1] = Height;
    mesh[2][0][2] = PGON_OFFSET;

    mesh[2][1][0] = 0.0;
    mesh[2][1][1] = Height;
    mesh[2][1][2] = 0.0;

    mesh[2][2][0] = Width/2.0;
    mesh[2][2][1] = Height;
    mesh[2][2][2] = PGON_OFFSET;

    curr = hwQuads->create( hwQuads );
    HW_MODIFY_1B( curr, hwStrTwoSided, HW_TRUE );
    HW_MODIFY_3F( curr, hwStrColor, 0.0, 0.0, 0.0 );
    HW_OBJECT_NAMESET( curr, obj );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,4*3), pgon );
    oList[nObjs++] = curr;

    if( type == TYPE_NORMAL ) {
	curr = hwMesh->create( hwMesh );
	HW_MODIFY_1B( curr, hwStrTwoSided, HW_TRUE );
	HW_MODIFY_1I( curr, hwStrGraphN, 3 );
	HW_MODIFY_1I( curr, hwStrGraphM, 3 );
	HW_MODIFY_3F( curr, hwStrColor, 1.0, 0.0, 1.0 );
	HW_OBJECT_NAMESET( curr, obj );
	curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,3*3*3), mesh );
	oList[nObjs++] = curr;
    }

    if( nObjs > 1 ) {
	curr = hwGroup->create( hwGroup );
	curr->modify( curr, hwStrChildren,
		HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
    }
    else {
	curr = oList[0];
    }

    obj->display_list = createHwSegmentFromObj( &curr, 1 );
}


static void make_wormhole_object(
    DRIVE_OBJECT *obj,
    int type)
{
    WORMHOLE_LIST *wh;

    if (Length <= 0.0)  Length  = DEFAULT_WORMHOLE_LENGTH;
    if (Height <= 0.0)  Height  = DEFAULT_WORMHOLE_HEIGHT;
    if (Width <= 0.0)   Width  = DEFAULT_WORMHOLE_WIDTH;

    if(debug) printf(" inside init_wormhole_%d_object() routine \n",(int) Height);

    obj->num_children = 0;

    /* See if we've created one like this before... */
    wh = wormhole_list;
    while (wh != NULL)
    {
	if (IS_NEAR(wh->height,Height)
		&& IS_NEAR(wh->width,Width)
		&& IS_NEAR(wh->length,Length)
		&& (wh->nameset_bits == obj->nameset_bits)
		&& (wh->type == type)) {
	    break;
	}
	wh = wh->next;
    }

    if (wh != NULL) 
    {
	/* Good -- I have one like this already. */
	obj->display_list = wh->dl_number;
    }
    else
    {
	/* Nope -- gotta create a new one. */
	if ((wh = (WORMHOLE_LIST *) malloc(sizeof(WORMHOLE_LIST))) == NULL)
	{
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}

	wh->length  = Length;
	wh->height  = Height;
	wh->width   = Width;
	wh->nameset_bits = obj->nameset_bits;
	wh->type    = type;
	wh->next    = wormhole_list;

    	create_wormhole_graphics(obj,type);

	wh->dl_number = obj->display_list;
	wormhole_list  = wh;
    }


    /* Initial (mc) bounding box values */
    obj->bound_mc[0] = -Width / 2.0;
    obj->bound_mc[1] = 0.0;
    obj->bound_mc[2] =  -Length / 2.0;
    obj->bound_mc[3] = Width / 2.0;
    obj->bound_mc[4] = Height;
    obj->bound_mc[5] =  Length / 2.0;

    update_wc_bounds(obj);

    obj->surface_chars_xyz  = pole_surface_chars_xyz;
    obj->surface_chars_bbox = pole_surface_chars_bbox;

    elevate_object_to_terrain_height((SCENE *) obj->scene,obj,FALSE);
}


void init_wormhole_object(
    DRIVE_OBJECT *obj)
{
    make_wormhole_object(obj,TYPE_NORMAL);
}

void init_black_wormhole_object(
    DRIVE_OBJECT *obj)
{
    make_wormhole_object(obj,TYPE_BLACK);
}

void init_invis_wormhole_object(
    DRIVE_OBJECT *obj)
{
    make_wormhole_object(obj,TYPE_INVISIBLE);
}


static void apply_wormhole_operation(
    DRIVE_OBJECT *obj,
    WORMHOLE_OPERATION *op,
    float data[3])
{
    switch (op->operation) {
	case OPERATION_NONE:
	    break;
	case OPERATION_REPLACE:
	case OPERATION_HISTORIC:
	    data[0] = op->operand[0];
	    data[1] = op->operand[1];
	    data[2] = op->operand[2];
	    break;
	case OPERATION_SCENE_REL_REPLACE:
	    {   SCENE *scene = (SCENE *) (obj->scene);
		data[0] = op->operand[0] + scene->xscene * SCENE_SIZE;
		data[1] = op->operand[1];
		data[2] = op->operand[2] + scene->zscene * SCENE_SIZE;
	    }
	    break;
	case OPERATION_ADD:
	    data[0] += op->operand[0];
	    data[1] += op->operand[1];
	    data[2] += op->operand[2];
	    break;
	case OPERATION_MULTIPLY:
	    data[0] *= op->operand[0];
	    data[1] *= op->operand[1];
	    data[2] *= op->operand[2];
	    break;
    }
}


void intersect_wormhole(
    DRIVE_OBJECT *obj,
    DRIVE_OBJECT *wormhole)
{
    PHYSICAL_OBJECT *pobj = obj->pobj;
    WORMHOLE_DATA *wormhole_data =
	(WORMHOLE_DATA *) wormhole->data;
    float angle[3];

    if( wormhole_data->angle.operation == OPERATION_HISTORIC)
    {
	if (ABS(pobj->R[ZD][YD]) >= (1.0-EPSILON)) {
	    /* pointing up or down */
	    angle[0] = 0.0;
	}
	else {
	    angle[0] = FATAN2(-pobj->R[ZD][XD],pobj->R[ZD][ZD]);
	}
	angle[1] = angle[2] = 0.0;
    }
    else {
	/* it is not terribly meaningful to do anything except replace
	 * the angle, so we will simplify by just starting with zero.
	 * Note that if the Angle Operation was to do nothing, we will
	 * just skip the code that generates the quaternion
	 */
	angle[0] = angle[1] = angle[2] = 0.0;
    }

    apply_wormhole_operation(obj,
	&(wormhole_data->position), (float *) &(obj->xform[3][0]));
    apply_wormhole_operation(obj,
	&(wormhole_data->angle), angle);
    apply_wormhole_operation(obj,
	&(wormhole_data->angular_velocity), &(pobj->w[XD]));
    apply_wormhole_operation(obj,
	&(wormhole_data->linear_velocity),  &(pobj->v[XD]));

    /* Historically, vehicles passing through wormholes were reset to be
     * level with the ground, and rotated about the Y axis by the specifed
     * angle.  We will preserve this for the old courses, but new wormholes
     * can specify rotation about all three axes.
     */

    if( wormhole_data->angle.operation == OPERATION_HISTORIC)
    {
	obj->xform[0][0] = obj->xform[2][2] = FCOS(angle[0]);
	obj->xform[0][2] = FSIN(angle[0]);
	obj->xform[2][0] = -obj->xform[0][2];
	obj->xform[0][1] = obj->xform[0][3] = 
	    obj->xform[1][0] = obj->xform[1][2] = obj->xform[1][3] = 
	    obj->xform[2][1] = obj->xform[2][3] = 0.0;
	obj->xform[1][1] = obj->xform[3][3] = 1.0;
	/* obj->xform[3][0..2] already set */

	{
	    float angle2 = angle[0];

	    while (angle2 < -M_PI) angle2 += (2.0*M_PI);
	    while (angle2 >  M_PI) angle2 -= (2.0*M_PI);
	    angle2 /= 2.0;

	    pobj->Q->s    = FCOS(angle2);
	    pobj->Q->r[XD] = 0.0;
	    pobj->Q->r[YD] = -FSIN(angle2);
	    pobj->Q->r[ZD] = 0.0;
	    quaternion_to_matrix(pobj->Q,pobj->R);
	    invert_matrix(pobj->R,INV_ROTMAT(pobj),3);
	}
    }
    else if(wormhole_data->angle.operation != OPERATION_NONE)
    {
	/* Create a rotation about X, Y and Z, and update the object's
	 * matrix to match.
	 */
	matrix3d xrot, yrot, zrot, comp;
	int r,c;

	_hp_identity(comp);
	_hp_identity(xrot);
	_hp_identity(yrot);
	_hp_identity(zrot);

	yrot[0][0] = yrot[2][2] = FCOS(angle[1]);
	yrot[0][2] = FSIN(angle[1]);
	yrot[2][0] = -yrot[0][2];

	xrot[1][1] = xrot[2][2] = FCOS(angle[0]);
	xrot[1][2] = FSIN(angle[0]);
	xrot[2][1] = -xrot[1][2];

	zrot[0][0] = zrot[1][1] = FCOS(angle[2]);
	zrot[0][1] = FSIN(angle[2]);
	zrot[1][0] = -zrot[0][1];

	concat_matrix(yrot, comp, comp);
	concat_matrix(xrot, comp, comp);
	concat_matrix(zrot, comp, comp);
	/* Copy everything to the object's matrix except translations */
	for(r=0; r<3; r++)
	    for(c=0; c<4; c++)
		obj->xform[r][c] = comp[r][c];

	{
	    float angle2 = angle[0];

	    while (angle2 < -M_PI) angle2 += (2.0*M_PI);
	    while (angle2 >  M_PI) angle2 -= (2.0*M_PI);
	    angle2 /= 2.0;

	    cmatrix4x4_to_quaternion(obj->xform, pobj->Q);
	    quaternion_to_matrix(pobj->Q,pobj->R);
	    invert_matrix(pobj->R,INV_ROTMAT(pobj),3);
	}

    }

    /* Update auxiliary physics data */
    pobj->x[XD] = obj->xform[3][0];
    pobj->x[YD] = obj->xform[3][1];
    pobj->x[ZD] = obj->xform[3][2];

    pobj->p[XD] = pobj->v[XD]*pobj->mass;
    pobj->p[YD] = pobj->v[YD]*pobj->mass;
    pobj->p[ZD] = pobj->v[ZD]*pobj->mass;

    /* L = Iw */
    update_angular_momentum(pobj);

    if (wormhole_data->position.operation == OPERATION_NONE) {
	/* Move the vehicle past the checkpoint
	 * so it's not intersected again.
	 */
	float t;
	float vx,vy,vz,velocity;

	vx = pobj->v[XD]; vy = pobj->v[YD]; vz = pobj->v[ZD];
	velocity = FHYPOT3(vx,vy,vz);
	if (ABS(velocity) < EPSILON) {
	    /* Go the direction you're pointed */
	    vx = pobj->R[ZD][XD];
	    vy = pobj->R[ZD][YD];
	    vz = pobj->R[ZD][ZD];
	}
	else {
	    /* Normalize so we get a high-accuracy guess */
	    vx /= velocity; vy /= velocity; vz /= velocity;
	}
	t = get_rough_time_estimate(obj,wormhole,0.1,vx,vy,vz) + 0.05;
	vx *= t; vy *= t; vz *= t;
	pobj->x[XD] += vx; pobj->x[YD] += vy; pobj->x[ZD] += vz;
	obj->xform[3][0] += vx; obj->xform[3][1] += vy; obj->xform[3][2] += vz;
    }

    /* Now, update the object's notion as to where it is */
    obj->scene = (void *) check_scenes(obj,pobj->x[XD],pobj->x[ZD]);
}
