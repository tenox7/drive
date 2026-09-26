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


/* Code Module for the twistramp segment object */


#include <stdio.h>
#include <math.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"

#define DEFAULT_WIDTH	DEFAULT_ROAD_WIDTH
#define DEFAULT_HEIGHT (DEFAULT_ROAD_WIDTH+10.0)
#define DEFAULT_LENGTH (DEFAULT_HEIGHT*4.0)

#define Length			(obj->size[SIZE_LENGTH])
#define Width			(obj->size[SIZE_WIDTH])
#define Height			(obj->size[SIZE_HEIGHT])
#define RBankWidth		(Height/2.0)

/* Change these to change the end slope */
#define Y_GIVEN_X_END(x) \
    ((x)+Height-(Width/2.0))
#define LBankWidth		((Height-Width)/2.0)

#define Y_GIVEN_XZ(x,z) \
    (Y_GIVEN_X_END(x) * (z)/Length * (z)/Length)

typedef struct _twistramp_list {
    float length,width,height;
    unsigned int nameset_bits;
    int dl_number;
    struct _twistramp_list *next;
} RAMP_LIST;
static RAMP_LIST *twistramp_list=NULL;



static int twistramp_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    float yp,nx,ny,nz,fullwidth,top;
    float absx = ABS(x);

    if (absx <= Width/2.0) {
	/* on twistramp itself */
	sc->mc_y = Y_GIVEN_XZ(x,z);
	if ((y > sc->mc_y - BBOX_MARGIN)
		|| (z < Length-5.0)) {
	    ny = Length;
	    nz = -Y_GIVEN_X_END(x);
	    NORMALIZE2(ny,nz);
	    sc->mc_normal[0] = 0.0;
	    sc->mc_normal[1] = ny;
	    sc->mc_normal[2] = nz;
	}
	else {
	    /* Must have run into the end */
	    sc->mc_normal[0] = 0.0;
	    sc->mc_normal[1] = 0.0;
	    sc->mc_normal[2] = 1.0;
	}
	return(TRUE);
    }
    else {
	fullwidth = Width/2.0 + LBankWidth*z/Length;
	if ((x < -Width/2.0) && (x > -fullwidth)) {
	    /* On left bank! */
	    top = Y_GIVEN_XZ(-Width/2.0,z);
	    yp = (1.0 - (absx-Width/2.0)/(fullwidth-Width/2.0)) * top;
	    /* To keep people from climbing the walls, bump this to 5.0
	     * if we're far enough into the twistramp.
	     */
	    if ((yp < 5.0) && (top > 5.0)) yp = 5.0;
	    sc->mc_y = yp;
	    if ((y > sc->mc_y - BBOX_MARGIN)
		    || (z < Length-5.0)
		    || (x > Width/2.0+LBankWidth-3.0)) {
		nx = Length*(Height-Width);
		ny = Length*LBankWidth;
		nz = -(Height-Width)*LBankWidth;
		NORMALIZE3(nx,ny,nz);
		if (x <= 0.0) nx = -nx;
		sc->mc_normal[0] = nx;
		sc->mc_normal[1] = ny;
		sc->mc_normal[2] = nz;
	    }
	    else {
		/* Must have run into the end */
		sc->mc_normal[0] = 0.0;
		sc->mc_normal[1] = 0.0;
		sc->mc_normal[2] = 1.0;
	    }
	    return(TRUE);
	}
	fullwidth = Width/2.0 + RBankWidth*z/Length;
	if ((x > Width/2.0) && (x < fullwidth)) {
	    /* On left bank! */
	    top = Y_GIVEN_XZ(Width/2.0,z);
	    yp = (1.0 - (absx-Width/2.0)/(fullwidth-Width/2.0)) * top;
	    /* To keep people from climbing the walls, bump this to 5.0
	     * if we're far enough into the twistramp.
	     */
	    if ((yp < 5.0) && (top > 5.0)) yp = 5.0;
	    sc->mc_y = yp;
	    if ((y > sc->mc_y - BBOX_MARGIN)
		    || (z < Length-5.0)
		    || (x > Width/2.0+RBankWidth-3.0)) {
		nx = Length*Height;
		ny = Length*RBankWidth;
		nz = -Height*RBankWidth;
		NORMALIZE3(nx,ny,nz);
		if (x <= 0.0) nx = -nx;
		sc->mc_normal[0] = nx;
		sc->mc_normal[1] = ny;
		sc->mc_normal[2] = nz;
	    }
	    else {
		/* Must have run into the end */
		sc->mc_normal[0] = 0.0;
		sc->mc_normal[1] = 0.0;
		sc->mc_normal[2] = 1.0;
	    }
	    return(TRUE);
	}
    }
    /* else */
    return(FALSE);
}


static int twistramp_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    return(checkbounds_bbox(obj,bbox_mc,sc,twistramp_surface_chars_xyz));
}


#define MESH_DIVISIONS 8

static void create_twistramp_graphics(
    DRIVE_OBJECT *obj)
{
    float pgon[8][3];
    float mesh[MESH_DIVISIONS][2][3];
    float *ptr;
    float z,dz,x,dx;
    int i;
    hwObject curr, oList[20];
    int nObjs = 0;

    /* the twistramp itself */
    ptr = (float *) mesh;
    dz = Length/(MESH_DIVISIONS-1);
    for (z=0.0,i=0; i<MESH_DIVISIONS; z+=dz,++i) {
	*ptr++ = Width/2.0;
	    *ptr++ = Y_GIVEN_XZ(Width/2.0,z);
	    *ptr++ = z;
	*ptr++ = -Width/2.0;
	    *ptr++ = Y_GIVEN_XZ(-Width/2.0,z);
	    *ptr++ = z;
    }
    curr = hwMesh->create( hwMesh );
    HW_MODIFY_1I( curr, hwStrGraphN, MESH_DIVISIONS );
    HW_MODIFY_1I( curr, hwStrGraphM, 2 );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
    ASPHALT_HW( curr );
    curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,MESH_DIVISIONS*2*3),
			mesh );
    oList[nObjs++] = curr;

    /* The sides */
    ptr = (float *) mesh;
    dx = -LBankWidth/(MESH_DIVISIONS-1);
    for (z=0.0,x=(-Width/2.0),i=0; i<MESH_DIVISIONS; z+=dz,x+=dx,++i) {
	*ptr++ = -Width/2.0;
	    *ptr++ = Y_GIVEN_XZ(-Width/2.0,z);
	    *ptr++ = z;
	*ptr++ = x;
	    *ptr++ = 0.0;
	    *ptr++ = z;
    }
    curr = hwMesh->create( hwMesh );
    HW_MODIFY_1I( curr, hwStrGraphN, MESH_DIVISIONS );
    HW_MODIFY_1I( curr, hwStrGraphM, 2 );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
    GRASS_HW( curr );
    curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,MESH_DIVISIONS*2*3),
			mesh );
    oList[nObjs++] = curr;

    ptr = (float *) mesh;
    dx = RBankWidth/(MESH_DIVISIONS-1);
    for (z=0.0,x=(Width/2.0),i=0; i<MESH_DIVISIONS; z+=dz,x+=dx,++i) {
	*ptr++ = x;
	    *ptr++ = 0.0;
	    *ptr++ = z;
	*ptr++ = Width/2.0;
	    *ptr++ = Y_GIVEN_XZ(Width/2.0,z);
	    *ptr++ = z;
    }
    curr = hwMesh->create( hwMesh );
    HW_MODIFY_1I( curr, hwStrGraphN, MESH_DIVISIONS );
    HW_MODIFY_1I( curr, hwStrGraphM, 2 );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
    GRASS_HW( curr );
    curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,MESH_DIVISIONS*2*3),
			mesh );
    oList[nObjs++] = curr;


    /* The far end */
    ptr = (float *) pgon;
    *ptr++ = -Width/2.0 - LBankWidth;
	*ptr++ = 0.0;
	*ptr++ = Length;
    *ptr++ = -Width/2.0;
	*ptr++ = Y_GIVEN_X_END(-Width/2.0);
	*ptr++ = Length;
    *ptr++ = Width/2.0;
	*ptr++ = Y_GIVEN_X_END(Width/2.0);
	*ptr++ = Length;
    *ptr++ = Width/2.0 + RBankWidth;
	*ptr++ = 0.0;
	*ptr++ = Length;

    curr = hwPolygon->create( hwPolygon );
    GRASS_HW( curr );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,4*3), pgon );
    oList[nObjs++] = curr;

    /* TBD: LOD */
    curr = hwGroup->create( hwGroup );
    curr->modify( curr, hwStrChildren,
		HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
    HW_OBJECT_NAMESET(curr,obj);

    obj->display_list = createHwSegmentFromObj( &curr, 1 );
}


void init_twistramp_object(
    DRIVE_OBJECT *obj)
{
    RAMP_LIST *rl;

    if (Length <= 0.0)
	Length = DEFAULT_LENGTH;
    if (Width <= 0.0)
	Width = DEFAULT_WIDTH;
    if (Height <= 0.0)
	Height = DEFAULT_HEIGHT;

    if(debug) printf(" inside init_twistramp_object() routine \n");

    obj->num_children = 0;

    /* See if we've created one like this before... */
    rl = twistramp_list;
    while (rl != NULL) {
	if (IS_NEAR(rl->length,Length)
		&& IS_NEAR(rl->width,Width)
		&& IS_NEAR(rl->height,Height)
		&& (rl->nameset_bits == obj->nameset_bits)) {
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
	if ((rl = (RAMP_LIST *) malloc(sizeof(RAMP_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}
	rl->length = Length;
	rl->width  = Width;
	rl->height = Height;
	rl->nameset_bits = obj->nameset_bits;
	rl->next   = twistramp_list;
    	create_twistramp_graphics(obj);
	rl->dl_number = obj->display_list;
	twistramp_list  = rl;
    }

    obj->surface_chars_xyz  = twistramp_surface_chars_xyz;
    obj->surface_chars_bbox = twistramp_surface_chars_bbox;

    /* Initial (mc) bounding box values */
    obj->bound_mc[0] = -Width/2.0 - LBankWidth;
    obj->bound_mc[1] = 0.0;
    obj->bound_mc[2] = 0.0;
    obj->bound_mc[3] = Width/2.0 + RBankWidth;
    obj->bound_mc[4] = Height;
    obj->bound_mc[5] = Length;

    /* apply the object's xform matrix to the bounding box to put it in
     * world coordinates */
    update_wc_bounds(obj);
}
