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


/* Code Module for the railroad segment object */


#include <stdio.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"

#define DEFAULT_WIDTH		8.0
#define DEFAULT_LENGTH		500.0
#define RAIL_HEIGHT		0.25
#define RAIL_WIDTH		0.25
#define BED_MARGIN		2.0
#define BED_WIDTH		(BED_MARGIN+TIE_HEIGHT*2.0)
#define TIE_HEIGHT		1.0
#define TIE_WIDTH		1.0
#define TIE_MARGIN		2.0
#define TIE_SPACING		15.0

#define MAX_RAILROAD_SEGMENT_LENGTH	(TIE_SPACING*20.0)
    /* MAX_RAILROAD_SEGMENT_LENGTH needs to be an even multiple
     * of TIE_SPACING.
     */

#define Length				(obj->size[SIZE_LENGTH])
#define Width				(obj->size[SIZE_WIDTH])

typedef struct _railroad_list {
    float length,width;
    unsigned int nameset_bits;
    int dl_number;
    struct _railroad_list *next;
} RAILROAD_LIST;
static RAILROAD_LIST *railroad_list=NULL;


static int railroad_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    x = ABS(x);
    if (x > Width/2.0 + BED_MARGIN) {
	x -= (Width/2.0 + BED_MARGIN);
	x /= (BED_WIDTH - BED_MARGIN);
	sc->mc_y = TIE_HEIGHT * (1.0 - x);
    }
    else {
	sc->mc_y = TIE_HEIGHT;
    }
    sc->roughness = 0.5;
    sc->roughness_frequency = TIE_SPACING;
    return(TRUE);
}


static hwObject both_res_model(
    DRIVE_OBJECT *obj,
    float zstart, float len,
    int high_res)
{
    float pgon[256*3],*ptr;
    float z;
    hwObject curr, oList[10];
    int nObjs = 0;

    /* rails */
    ptr = pgon;
    *ptr++ = -Width/2.0 + TIE_MARGIN;
    *ptr++ = TIE_HEIGHT+ RAIL_HEIGHT;
    *ptr++ = zstart;

    *ptr++ = -Width/2.0 + TIE_MARGIN + RAIL_WIDTH;
    *ptr++ = TIE_HEIGHT+ RAIL_HEIGHT;
    *ptr++ = zstart;

    *ptr++ = -Width/2.0 + TIE_MARGIN + RAIL_WIDTH;
    *ptr++ = TIE_HEIGHT+ RAIL_HEIGHT;
    *ptr++ = zstart + len;

    *ptr++ = -Width/2.0 + TIE_MARGIN;
    *ptr++ = TIE_HEIGHT+ RAIL_HEIGHT;
    *ptr++ = zstart + len;

    *ptr++ = Width/2.0 - TIE_MARGIN;
    *ptr++ = TIE_HEIGHT+ RAIL_HEIGHT;
    *ptr++ = zstart;

    *ptr++ = Width/2.0 - TIE_MARGIN + RAIL_WIDTH;
    *ptr++ = TIE_HEIGHT+ RAIL_HEIGHT;
    *ptr++ = zstart;

    *ptr++ = Width/2.0 - TIE_MARGIN + RAIL_WIDTH;
    *ptr++ = TIE_HEIGHT+ RAIL_HEIGHT;
    *ptr++ = zstart + len;

    *ptr++ = Width/2.0 - TIE_MARGIN;
    *ptr++ = TIE_HEIGHT+ RAIL_HEIGHT;
    *ptr++ = zstart + len;

    curr = hwQuads->create( hwQuads );
    CHROME_HW(curr);
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrTwoSided, HW_TRUE );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,(ptr-pgon)), pgon );
    oList[nObjs++] = curr;

    if( high_res ) {
	/* ties */
	ptr = pgon;
	for (z=zstart; z<zstart+len; z += TIE_SPACING) {
	    *ptr++ = -Width/2.0;
	    *ptr++ = TIE_HEIGHT;
	    *ptr++ = z;

	    *ptr++ = Width/2.0;
	    *ptr++ = TIE_HEIGHT;
	    *ptr++ = z;

	    *ptr++ = Width/2.0;
	    *ptr++ = TIE_HEIGHT;
	    *ptr++ = z + TIE_WIDTH;

	    *ptr++ = -Width/2.0;
	    *ptr++ = TIE_HEIGHT;
	    *ptr++ = z + TIE_WIDTH;
	}

	curr = hwQuads->create( hwQuads );
	WOOD_HW(curr);
	HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
	HW_MODIFY_1B( curr, hwStrTwoSided, HW_TRUE );
	curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,(ptr-pgon)), pgon );
	oList[nObjs++] = curr;
    }

    /* bed */
    ptr = pgon;
    if( high_res ) {
	*ptr++ = -Width/2.0 - BED_WIDTH;
	*ptr++ = 0.0;
	*ptr++ = zstart + len;

	*ptr++ = -Width/2.0 - BED_WIDTH;
	*ptr++ = 0.0;
	*ptr++ = zstart;

	*ptr++ = -Width/2.0 - BED_MARGIN;
	*ptr++ = TIE_HEIGHT-0.1;
	*ptr++ = zstart;

	*ptr++ = -Width/2.0 - BED_MARGIN;
	*ptr++ = TIE_HEIGHT-0.1;
	*ptr++ = zstart + len;

	*ptr++ = Width/2.0 + BED_WIDTH;
	*ptr++ = 0.0;
	*ptr++ = zstart;

	*ptr++ = Width/2.0 + BED_WIDTH;
	*ptr++ = 0.0;
	*ptr++ = zstart + len;

	*ptr++ = Width/2.0 + BED_MARGIN;
	*ptr++ = TIE_HEIGHT-0.1;
	*ptr++ = zstart + len;

	*ptr++ = Width/2.0 + BED_MARGIN;
	*ptr++ = TIE_HEIGHT-0.1;
	*ptr++ = zstart;
    }

    *ptr++ = -Width/2.0 - BED_MARGIN;
    *ptr++ = TIE_HEIGHT-0.1;
    *ptr++ = zstart;

    *ptr++ = Width/2.0 + BED_MARGIN;
    *ptr++ = TIE_HEIGHT-0.1;
    *ptr++ = zstart;

    *ptr++ = Width/2.0 + BED_MARGIN;
    *ptr++ = TIE_HEIGHT-0.1;
    *ptr++ = zstart + len;

    *ptr++ = -Width/2.0 - BED_MARGIN;
    *ptr++ = TIE_HEIGHT-0.1;
    *ptr++ = zstart + len;

    curr = hwQuads->create( hwQuads );
    HW_MODIFY_3F( curr, hwStrColor, 0.3, 0.3, 0.3 );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrTwoSided, HW_TRUE );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,(ptr-pgon)), pgon );
    oList[nObjs++] = curr;

    curr = hwGroup->create( hwGroup );
    curr->modify( curr, hwStrChildren,
		HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
    return curr;
}


static hwObject add_railroad_graphics(
    DRIVE_OBJECT *obj,
    float zstart, float len)
{
    /* TBD: LOD */
    return both_res_model(obj,zstart,len,TRUE);
}


static void create_railroad_graphics( DRIVE_OBJECT *obj )
{
    float z,len;
    hwObject oList[100];
    int nObjs = 0;

    for (z=0.0; z<Length; z += MAX_RAILROAD_SEGMENT_LENGTH) {
	if (Length - z < MAX_RAILROAD_SEGMENT_LENGTH) len = Length - z;
	else len = MAX_RAILROAD_SEGMENT_LENGTH;

	oList[nObjs++] = add_railroad_graphics(obj,z,len);
    }

    obj->display_list = createHwSegmentFromObj( oList, nObjs );
}


void init_railroad_object(
    DRIVE_OBJECT *obj)
{
    RAILROAD_LIST *rl;


    if (Length <= 0.0) Length = DEFAULT_LENGTH;
    if (Width <= 0.0)  Width  = DEFAULT_WIDTH;

    if(debug) printf(" inside init_railroad_object() routine \n");

    obj->num_children = 0;

    /* See if we've created one like this before... */
    rl = railroad_list;
    while (rl != NULL) {
	if (IS_NEAR(rl->length,Length)
		&& IS_NEAR(rl->width,Width)
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
	if ((rl = (RAILROAD_LIST *) malloc(sizeof(RAILROAD_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}
	rl->length = Length;
	rl->width  = Width;
	rl->nameset_bits = obj->nameset_bits;
	rl->next   = railroad_list;
    	create_railroad_graphics(obj);
	rl->dl_number = obj->display_list;
	railroad_list  = rl;
    }

    obj->surface_chars_xyz  = railroad_surface_chars_xyz;

    /* Initialize bounding box values */
    obj->bound_mc[0] = -Width/2.0 - BED_WIDTH;
    obj->bound_mc[1] = 0.0;
    obj->bound_mc[2] = 0.0;
    obj->bound_mc[3] = Width/2.0 + BED_WIDTH;
    obj->bound_mc[4] = TIE_HEIGHT + BBOX_MARGIN;
    obj->bound_mc[5] = Length;

    update_wc_bounds(obj);
}
