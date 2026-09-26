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


/* Code Module for the bump segment object */

#include <stdio.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"

#define DEFAULT_WIDTH		2.0
#define DEFAULT_LENGTH		10.0
#define DEFAULT_HEIGHT		0.5

#define Length				(obj->size[SIZE_LENGTH])
#define Width				(obj->size[SIZE_WIDTH])
#define	Height				(obj->size[SIZE_HEIGHT])

typedef struct _bump_list {
    float length,width,height;
    unsigned int nameset_bits;
    int dl_number;
    struct _bump_list *next;
} BUMP_LIST;
static BUMP_LIST *bump_list=NULL;


static int bump_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    float x0,y0,z0;

    x = ABS(x); z = ABS(z);

    sc->mc_y = Height;

    if ((x0 = Width/2.0 - Height*1.5) < x) {
   	x -= x0; 
	sc->mc_y = Height * (1.0 - x/(Height*1.5));
    }

    if ((z0 = Length/2.0 - Height*1.5) < z) {
   	z -= z0; 
	y0 = Height * (1.0 - z/(Height*1.5));
	if (y0 < sc->mc_y) sc->mc_y = y0;
    }

    sc->roughness = 0.0;
    return(TRUE);
}


static void create_bump_graphics(
    DRIVE_OBJECT *obj)
{
    float mesh[10][3];
    hwObject curr, oList[10];
    int nObjs = 0;

    mesh[0][0] = -Width/2.0;
    mesh[0][1] = 0.0;
    mesh[0][2] = -Length/2.0;

    mesh[1][0] = Width/2.0;
    mesh[1][1] = 0.0;
    mesh[1][2] = -Length/2.0;

    mesh[2][0] = Width/2.0;
    mesh[2][1] = 0.0;
    mesh[2][2] = Length/2.0;

    mesh[3][0] = -Width/2.0;
    mesh[3][1] = 0.0;
    mesh[3][2] = Length/2.0;

    mesh[4][0] = -Width/2.0;
    mesh[4][1] = 0.0;
    mesh[4][2] = -Length/2.0;


    mesh[5][0] = -Width/2.0 + Height*1.5;
    mesh[5][1] = Height;
    mesh[5][2] = -Length/2.0 + Height*1.5;

    mesh[6][0] = Width/2.0 - Height*1.5;
    mesh[6][1] = Height;
    mesh[6][2] = -Length/2.0 + Height*1.5;

    mesh[7][0] = Width/2.0 - Height*1.5;
    mesh[7][1] = Height;
    mesh[7][2] = Length/2.0 - Height*1.5;

    mesh[8][0] = -Width/2.0 + Height*1.5;
    mesh[8][1] = Height;
    mesh[8][2] = Length/2.0 - Height*1.5;

    mesh[9][0] = -Width/2.0 + Height*1.5;
    mesh[9][1] = Height;
    mesh[9][2] = -Length/2.0 + Height*1.5;


    curr = hwPolygon->create( hwPolygon );
    ROAD_LINE_YELLOW_HW(curr);
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrTwoSided, HW_TRUE );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,4*3), &mesh[5][0] );
    oList[nObjs++] = curr;

    curr = hwMesh->create( hwMesh );
    ROAD_LINE_YELLOW_HW(curr);
    HW_MODIFY_1I( curr, hwStrGraphN, 2 );
    HW_MODIFY_1I( curr, hwStrGraphM, 5 );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,2*5*3), mesh );
    oList[nObjs++] = curr;

    curr = hwGroup->create( hwGroup );
    curr->modify( curr, hwStrChildren,
		HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
    HW_OBJECT_NAMESET(curr,obj);
    obj->display_list = createHwSegmentFromObj( &curr, 1 );
}


void init_bump_object(
    DRIVE_OBJECT *obj)
{
    BUMP_LIST *rl;


    if (Length <= 0.0) Length = DEFAULT_LENGTH;
    if (Width  <= 0.0) Width  = DEFAULT_WIDTH;
    if (Height <= 0.0) Height = DEFAULT_HEIGHT;

    if (Height > Width/3.0)  Height = Width/3.0;
    if (Height > Length/3.0) Height = Length/3.0;

    obj->num_children = 0;

    /* See if we've created one like this before... */
    rl = bump_list;
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
	if ((rl = (BUMP_LIST *) malloc(sizeof(BUMP_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}
	rl->length = Length;
	rl->width  = Width;
	rl->height = Height;
	rl->nameset_bits = obj->nameset_bits;
	rl->next   = bump_list;
    	create_bump_graphics(obj);
	rl->dl_number = obj->display_list;
	bump_list  = rl;
    }

    obj->surface_chars_xyz  = bump_surface_chars_xyz;

    /* Initialize bounding box values */
    obj->bound_mc[0] = -Width/2.0;
    obj->bound_mc[1] = 0.0;
    obj->bound_mc[2] = -Length/2.0;
    obj->bound_mc[3] = Width/2.0;
    obj->bound_mc[4] = Height + BBOX_MARGIN;
    obj->bound_mc[5] = Length/2.0;

    update_wc_bounds(obj);
}
