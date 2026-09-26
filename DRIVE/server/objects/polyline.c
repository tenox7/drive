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



#include <stdio.h>
#include <math.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"

#define Count			(obj->count)
#define Dimension		(obj->dimension)
#define Data			((float *) (obj->data))

static void create_polyline_graphics(
    DRIVE_OBJECT *obj)
{
    hwObject curr;

    curr = hwPolyline->create( hwPolyline );
    HW_MODIFY_3F(curr,hwStrColor,obj->color[0],obj->color[1],obj->color[2]);
    HW_OBJECT_NAMESET(curr,obj);
    if (Dimension == 3) {
	curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,Count*3), Data );
    }
    else {
	HW_MODIFY_1B( curr, hwStrHasFlags, HW_TRUE );
	curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,Count*4), Data );
    }

    obj->display_list = createHwSegmentFromObj( &curr, 1 );
}

static void find_polyline_mc_bounds(
    DRIVE_OBJECT *obj)
{
    float *fptr;
    int i;
    float x,y,z;

    fptr = Data;

    /* Initialize to first point */
    obj->bound_mc[0] = obj->bound_mc[3] = *fptr;
    obj->bound_mc[1] = obj->bound_mc[4] = *(fptr+1);
    obj->bound_mc[2] = obj->bound_mc[5] = *(fptr+2);

    /* And check the rest */
    for (i=1; i<Count; ++i) {
	fptr += Dimension;
	x = *fptr; y = *(fptr+1); z = *(fptr+2);
	if (x < obj->bound_mc[0]) obj->bound_mc[0] = x;
	if (y < obj->bound_mc[1]) obj->bound_mc[1] = y;
	if (z < obj->bound_mc[2]) obj->bound_mc[2] = z;
	if (x > obj->bound_mc[3]) obj->bound_mc[3] = x;
	if (y > obj->bound_mc[4]) obj->bound_mc[4] = y;
	if (z > obj->bound_mc[5]) obj->bound_mc[5] = z;
    }
}


void init_polyline_object(
    DRIVE_OBJECT *obj)
{
    /* Check for bogus data */
    if (Count <= 0) return;
    if ((Dimension != 3) && (Dimension != 4)) return;
    if (Data == NULL) return;

    /* Set up defaults */
    if ((obj->color[0] == DEFAULT_OBJECT_COLOR)
	    || (obj->color[1] == DEFAULT_OBJECT_COLOR)
	    || (obj->color[2] == DEFAULT_OBJECT_COLOR)) {
	obj->color[0] = obj->color[1] = obj->color[2] = CONCRETE_INTENSITY;
    }

    if (debug) printf(" inside init_polyline_object() routine \n");

    obj->num_children = 0;
    create_polyline_graphics(obj);

    /* No physics */
    obj->surface_chars_xyz  = NULL;
    obj->surface_chars_bbox = NULL;

    /* Initial (mc) bounding box values */
    find_polyline_mc_bounds(obj);
    /* And translate to WC's */
    update_wc_bounds(obj);
}
