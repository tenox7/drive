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


/* Code Module for the flat segment object */


#include <stdio.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"

#define MAX_SINGLE_PGON_SIZE	(SCENE_SIZE/4.0)
#define DEFAULT_FLAT_SIZE	100.0
#define DEFAULT_LAWN_SIZE	100.0
#define DEFAULT_SIDEWALK_WIDTH	5.0
#define DEFAULT_SIDEWALK_LENGTH	100.0

#define EPSILON			(1.0e-6)

#define Length				(obj->size[SIZE_LENGTH])
#define Width				(obj->size[SIZE_WIDTH])

typedef struct _flat_list {
    float length,width;
    float r,g,b;
    int transparent;
    int self_lit;
    unsigned int nameset_bits;
    int dl_number;
    struct _flat_list *next;
} FLAT_LIST;
static FLAT_LIST *flat_list=NULL;


static int ice_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = 0.0;
    sc->friction = 0.1;
    return(TRUE);
}


static int flat_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = 0.0;
    return(TRUE);
}


static void create_flat_graphics(
    DRIVE_OBJECT *obj,
    int transparent,
    int self_lit)
{
    hwObject
	flatObj;

    if ((Length <= MAX_SINGLE_PGON_SIZE)
	    && (Width <= MAX_SINGLE_PGON_SIZE)) {
	float pgon[8][3],*ptr;

	ptr = (float *) pgon;
	flatObj = hwPolygon->create(hwPolygon);
	HW_MODIFY_1B(flatObj, hwStrBackface, HW_TRUE);
	HW_MODIFY_1B(flatObj, hwStrFlipNormals, HW_TRUE);

	*ptr++ = -Width/2.0;
	    *ptr++ = ROAD_FLOAT;
	    *ptr++ = -Length/2.0;
	*ptr++ = Width/2.0;
	    *ptr++ = ROAD_FLOAT;
	    *ptr++ = -Length/2.0;
	*ptr++ = Width/2.0;
	    *ptr++ = ROAD_FLOAT;
	    *ptr++ = Length/2.0;
	*ptr++ = -Width/2.0;
	    *ptr++ = ROAD_FLOAT;
	    *ptr++ = Length/2.0;
	flatObj->modify(flatObj, hwStrData,
	    HW_MAKE_TYPE(HW_TYPE_FLOAT, 12), pgon);
    }
    else {
	/* Do it as a mesh. */
	int xsize,zsize;
	float *mesh,*fptr;
	int i,j;
	float x,z;

	xsize = (int) (Width/MAX_SINGLE_PGON_SIZE  - EPSILON) + 2;
	zsize = (int) (Length/MAX_SINGLE_PGON_SIZE - EPSILON) + 2;

	flatObj = hwMesh->create(hwMesh);
	HW_MODIFY_1I(flatObj, hwStrGraphM, xsize);
	HW_MODIFY_1I(flatObj, hwStrGraphN, zsize);

	if ((mesh = (float *) malloc(xsize*zsize*3*sizeof(float)))
		== NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}

	fptr = mesh;
	for (z = -Length/2.0, j = 0;
		j < zsize;
		++j, z += MAX_SINGLE_PGON_SIZE) {
	    if (z > Length/2.0) z = Length/2.0;
	    /* Do all but the last */
	    for (x = -Width/2.0, i = 0;
		    i < (xsize-1);
		    ++i, x += MAX_SINGLE_PGON_SIZE) {
		*fptr++ = x;
		*fptr++ = ROAD_FLOAT;
		*fptr++ = z;
	    }
	    /* Do the last one. */
	    *fptr++ = Width/2.0;
	    *fptr++ = ROAD_FLOAT;
	    *fptr++ = z;
	}
	flatObj->modify(flatObj, hwStrData,
	    HW_MAKE_TYPE(HW_TYPE_FLOAT, xsize * zsize * 3), mesh);

	free(mesh);
    }
    if( self_lit )
	HW_MODIFY_1B(flatObj, hwStrBright, HW_TRUE);
    if( transparent )
	HW_MODIFY_1F(flatObj, hwStrBright, 0.5);
    HW_MODIFY_3F(flatObj, hwStrColor, 
	obj->color[0],obj->color[1],obj->color[2]);

    obj->display_list = createHwSegmentFromObj(&flatObj,1);
}


static void do_flat_object(
    DRIVE_OBJECT *obj,
    int transparent,
    int self_lit)
{
    FLAT_LIST *ll;
    if (Length == DEFAULT_OBJECT_SIZE) Length = DEFAULT_FLAT_SIZE;
    if (Width  == DEFAULT_OBJECT_SIZE) Width  = DEFAULT_FLAT_SIZE;
    if ((obj->color[0] == DEFAULT_OBJECT_COLOR)
	    || (obj->color[1] == DEFAULT_OBJECT_COLOR)
	    || (obj->color[2] == DEFAULT_OBJECT_COLOR)) {
	obj->color[0] = obj->color[1] = obj->color[2] =
	    ASPHALT_INTENSITY;
    }

    if(debug) printf(" inside init_flat_object() routine \n");

    obj->num_children = 0;

    /* See if we've created one like this before... */
    ll = flat_list;
    while (ll != NULL) {
	if (IS_NEAR(ll->length,Length)
		&& IS_NEAR(ll->width,Width)
		&& IS_NEAR(ll->r,obj->color[0])
		&& IS_NEAR(ll->g,obj->color[1])
		&& IS_NEAR(ll->b,obj->color[2])
		&& (ll->nameset_bits == obj->nameset_bits)
		&& (ll->transparent == transparent)
		&& (ll->self_lit == self_lit)) {
	    break;
	}
	/* else */
	ll = ll->next;
    }
    if (ll != NULL) {
	/* Good -- I have one like this already. */
	obj->display_list = ll->dl_number;
    }
    else {
	/* Nope -- gotta create a new one. */
	if ((ll = (FLAT_LIST *) malloc(sizeof(FLAT_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}
	ll->length = Length;
	ll->width  = Width;
	ll->r      = obj->color[0];
	ll->g      = obj->color[1];
	ll->b      = obj->color[2];
	ll->transparent = transparent;
	ll->nameset_bits = obj->nameset_bits;
	ll->self_lit = self_lit;
	ll->next   = flat_list;
    	create_flat_graphics(obj,transparent,self_lit);
	ll->dl_number = obj->display_list;
	flat_list  = ll;
    }

    obj->surface_chars_xyz  = flat_surface_chars_xyz;

    /* Initialize bounding box values */
    obj->bound_mc[0] = -Width/2.0;
    obj->bound_mc[1] = 0.0;
    obj->bound_mc[2] = -Length/2.0;
    obj->bound_mc[3] = Width/2.0;
    obj->bound_mc[4] = BBOX_MARGIN;
    obj->bound_mc[5] = Length/2.0;

    update_wc_bounds(obj);
}


void init_flat_object(
    DRIVE_OBJECT *obj)
{
    do_flat_object(obj,FALSE,FALSE);
}


void init_light_object(
    DRIVE_OBJECT *obj)
{
    do_flat_object(obj,FALSE,TRUE);
}


void init_lawn_object(
    DRIVE_OBJECT *obj)
{
    if (Length == DEFAULT_OBJECT_SIZE) Length = DEFAULT_LAWN_SIZE;
    if (Width  == DEFAULT_OBJECT_SIZE) Width  = DEFAULT_LAWN_SIZE;
    obj->color[0] = 0.25;
    obj->color[1] = 0.67;
    obj->color[2] = 0.25;
	
    do_flat_object(obj,FALSE,FALSE);
}


void init_ice_object(
    DRIVE_OBJECT *obj)
{
    if (Length == DEFAULT_OBJECT_SIZE) Length = DEFAULT_LAWN_SIZE;
    if (Width  == DEFAULT_OBJECT_SIZE) Width  = DEFAULT_LAWN_SIZE;
    obj->color[0] = 1.0;
    obj->color[1] = 1.0;
    obj->color[2] = 1.0;
	
    do_flat_object(obj,FALSE,FALSE);

    /* Override surface characteristics with "ice" */
    obj->surface_chars_xyz  = ice_surface_chars_xyz;
}


void init_sidewalk_object(
    DRIVE_OBJECT *obj)
{
    if (Length == DEFAULT_OBJECT_SIZE) Length = DEFAULT_SIDEWALK_LENGTH;
    if (Width  == DEFAULT_OBJECT_SIZE) Width  = DEFAULT_SIDEWALK_WIDTH;
    obj->color[0] = obj->color[1] = obj->color[2] = CONCRETE_INTENSITY;
	
    do_flat_object(obj,FALSE,FALSE);
}


void init_shadow_object(
    DRIVE_OBJECT *obj)
{
    obj->color[0] = obj->color[1] = obj->color[2] = SHADOW_INTENSITY;
    obj->xform[3][1] += ROAD_FLOAT;
	
    do_flat_object(obj,TRUE,FALSE);

    /* Can't be parked on a shadow. */
    obj->surface_chars_xyz  = NULL;
    obj->bound_mc[0] = 0.0;
    obj->bound_mc[1] = 0.0;
    obj->bound_mc[2] = 0.0;
    obj->bound_mc[3] = EPSILON;
    obj->bound_mc[4] = EPSILON;
    obj->bound_mc[5] = EPSILON;
    update_wc_bounds(obj);
}
