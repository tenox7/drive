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


/* Code Module for the parking_lot segment object */


#include <stdio.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"

#define DEFAULT_PARKING_LOT_LENGTH	170.0
#define DEFAULT_PARKING_LOT_WIDTH	200.0
#define DEFAULT_PARKING_LOT_HEIGHT	0.0
#define AISLE_WIDTH			30.0
#define PARKING_WIDTH			15.0
#define PARKING_DEPTH			20.0

#define Z_TOP  \
     (len - AISLE_WIDTH - PARKING_DEPTH - i*( 2*PARKING_DEPTH + AISLE_WIDTH))

#define Z_BOTTOM  \
     (len - AISLE_WIDTH - PARKING_DEPTH - LOT_LINE_WIDTH - i*( 2*PARKING_DEPTH + AISLE_WIDTH))

#define LOT_LINE_DELTA			0.1
#define LOT_LINE_FLOAT			(ROAD_FLOAT+0.1)
#define LOT_LINE_WIDTH			0.8

#define Length				(obj->size[SIZE_LENGTH])
#define Width				(obj->size[SIZE_WIDTH])
#define Height				(obj->size[SIZE_HEIGHT])

typedef struct _parking_lot_list {
    float length,width,height;
    unsigned int nameset_bits;
    int dl_number;
    struct _parking_lot_list *next;
} PARKING_LOT_LIST;
static PARKING_LOT_LIST *parking_lot_list=NULL;


static int parking_lot_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = 0.0;
    return(TRUE);
}


static void create_parking_lot_graphics(
    DRIVE_OBJECT *obj)
{
    float pgon[4096],*ptr;
    float z;
    int len,wid;
    int h_stripes,v_stripes,h,i;
    hwObject curr, oList[10];
    int nObjs = 0;


    wid = Width/2.0;
    len = Length/2.0;

    if( (Width >= 2 * AISLE_WIDTH + PARKING_WIDTH) && 
      (Length > 3*AISLE_WIDTH + 2*PARKING_DEPTH) )
    {
	/* Make the stripes */

	/* First, draw the horizontal stripes */
	h_stripes=(Length - AISLE_WIDTH)/(2 * PARKING_DEPTH + AISLE_WIDTH);
	ptr = pgon;
	for(i=0;i<h_stripes;i++)
	{
	    *ptr++ = wid - AISLE_WIDTH;  /* x */
	    *ptr++ = LOT_LINE_FLOAT;     /* y */
	    *ptr++ = Z_TOP;/* z */

	    *ptr++ = wid - AISLE_WIDTH;  /* x */
	    *ptr++ = LOT_LINE_FLOAT + LOT_LINE_DELTA;     /* y */
	    *ptr++ = Z_BOTTOM;

	    *ptr++ = -wid + AISLE_WIDTH;  	/* x */
	    *ptr++ = LOT_LINE_FLOAT;     	/* y */
	    *ptr++ = Z_BOTTOM;

	    *ptr++ = -wid + AISLE_WIDTH;  	/* x */
	    *ptr++ = LOT_LINE_FLOAT;     	/* y */
	    *ptr++ = Z_TOP;
	}

	/* Now, do the vertical stripes */
	v_stripes = (Width - 2*AISLE_WIDTH) / PARKING_WIDTH;
	for(h=0;h<h_stripes;h++)
	{
	    z = len - AISLE_WIDTH - PARKING_DEPTH - 
	      h*( 2*PARKING_DEPTH + AISLE_WIDTH);
	    for(i=0;i<v_stripes;i++)
	    {
		*ptr++ = -wid + AISLE_WIDTH + i*PARKING_WIDTH;
		*ptr++ = LOT_LINE_FLOAT;
		*ptr++ = z + PARKING_DEPTH;

		*ptr++ = -wid + AISLE_WIDTH + i*PARKING_WIDTH+LOT_LINE_WIDTH;
		*ptr++ = LOT_LINE_FLOAT;
		*ptr++ = z + PARKING_DEPTH;

		*ptr++ = -wid + AISLE_WIDTH + i*PARKING_WIDTH+LOT_LINE_WIDTH;
		*ptr++ = LOT_LINE_FLOAT;
		*ptr++ = z - PARKING_DEPTH;

		*ptr++ = -wid + AISLE_WIDTH + i*PARKING_WIDTH;
		*ptr++ = LOT_LINE_FLOAT;
		*ptr++ = z - PARKING_DEPTH;
	    }
	}
	curr = hwQuads->create( hwQuads );
	ROAD_LINE_YELLOW_HW(curr);
	i = ptr - pgon;
	curr->modify( curr, hwStrData, HW_MAKE_TYPE(HW_TYPE_FLOAT,i), pgon );
	oList[nObjs++] = curr;
    }

    ptr = pgon;
    *ptr++ = -wid; *ptr++ = ROAD_FLOAT; *ptr++ = -len;
    *ptr++ = -wid; *ptr++ = ROAD_FLOAT; *ptr++ =  len;
    *ptr++ =  wid; *ptr++ = ROAD_FLOAT; *ptr++ =  len;
    *ptr++ =  wid; *ptr++ = ROAD_FLOAT; *ptr++ = -len;
    curr = hwPolygon->create( hwPolygon );
    ASPHALT_HW(curr);
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,4*3), pgon );
    oList[nObjs++] = curr;

    if( Height > 0.0 )
    {
	float mesh[30];

	ptr = pgon;
	*ptr++ =  wid; *ptr++ = -Height; *ptr++ = -len;
	*ptr++ =  wid; *ptr++ = -Height; *ptr++ =  len;
	*ptr++ = -wid; *ptr++ = -Height; *ptr++ =  len;
	*ptr++ = -wid; *ptr++ = -Height; *ptr++ = -len;

	curr = hwPolygon->create( hwPolygon );
	DARK_CONCRETE_HW(curr);
	curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,4*3), pgon );
	oList[nObjs++] = curr;

	/* draw the sides */
	CONCRETE_HW(curr);
	ptr = mesh;
	*ptr++ = -wid; *ptr++ = ROAD_FLOAT; *ptr++ = -len;
	*ptr++ = -wid; *ptr++ = ROAD_FLOAT; *ptr++ =  len;
	*ptr++ =  wid; *ptr++ = ROAD_FLOAT; *ptr++ =  len;
	*ptr++ =  wid; *ptr++ = ROAD_FLOAT; *ptr++ = -len;
	*ptr++ = -wid; *ptr++ = ROAD_FLOAT; *ptr++ = -len;

	*ptr++ = -wid; *ptr++ = -Height; *ptr++ = -len;
	*ptr++ = -wid; *ptr++ = -Height; *ptr++ =  len;
	*ptr++ =  wid; *ptr++ = -Height; *ptr++ =  len;
	*ptr++ =  wid; *ptr++ = -Height; *ptr++ = -len;
	*ptr++ = -wid; *ptr++ = -Height; *ptr++ = -len;

	curr = hwMesh->create( hwMesh);
	CONCRETE_HW(curr);
	HW_MODIFY_1I( curr, hwStrGraphN, 2 );
	HW_MODIFY_1I( curr, hwStrGraphM, 5 );
	curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,2*5*3), mesh );
	oList[nObjs++] = curr;

    }

    /* TBD: LOD */
    curr = hwGroup->create( hwGroup );
    curr->modify( curr, hwStrChildren,
		HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
    HW_OBJECT_NAMESET(curr,obj);

    obj->display_list = createHwSegmentFromObj( &curr, 1 );
}


void init_parking_lot_object(
    DRIVE_OBJECT *obj)
{
    PARKING_LOT_LIST *rl;
    if (Length <= 0.0) Length = DEFAULT_PARKING_LOT_LENGTH;
    if (Width <= 0.0)  Width  = DEFAULT_PARKING_LOT_WIDTH;

    if(debug) printf(" inside init_parking_lot_%d_object() \n",(int) Length);

    obj->num_children = 0;

    /* See if we've created one like this before... */
    rl = parking_lot_list;
    while (rl != NULL) {
	if (IS_NEAR(rl->length,Length)
		&& IS_NEAR(rl->height,Height)
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
	if ((rl = (PARKING_LOT_LIST *) malloc(sizeof(PARKING_LOT_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}
	rl->length = Length;
	rl->width  = Width;
	rl->height = Height;
	rl->nameset_bits = obj->nameset_bits;
	rl->next   = parking_lot_list;
    	create_parking_lot_graphics(obj);
	rl->dl_number = obj->display_list;
	parking_lot_list  = rl;
    }

    obj->surface_chars_xyz   = parking_lot_surface_chars_xyz;

    /* Initialize bounding box values */
    obj->bound_mc[0] = -Width/2.0;
    obj->bound_mc[1] = -BBOX_MARGIN - Height;
    obj->bound_mc[2] = -Length/2.0;
    obj->bound_mc[3] = Width/2.0;
    obj->bound_mc[4] = BBOX_MARGIN;
    obj->bound_mc[5] = Length/2.0;

    update_wc_bounds(obj);
}
