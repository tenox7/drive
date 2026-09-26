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


/* Code Module for the silo segment object */


#include <stdio.h>
#include <math.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"

#define DEFAULT_HEIGHT	(50.0)
#define DEFAULT_RADIUS	(10.0)
#define OVERHANG	(0.0)
#define FACETS		(12)

#define Height			(obj->size[SIZE_HEIGHT])
#define Radius			(obj->radius)

typedef struct _silo_list {
    float radius,height;
    unsigned int nameset_bits;
    int dl_number;
    struct _silo_list *next;
} SILO_LIST;
static SILO_LIST *silo_list=NULL;



static int silo_surface_chars_xyz(
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

static int silo_surface_chars_bbox(
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


static void create_silo_graphics(
    DRIVE_OBJECT *obj)
{
    float
	capRadius,
	capHeight;
    hwObject
	curr, oList[5];
    int
	nObjs = 0;

    capRadius = Radius * 1.155;
    capHeight = capRadius * 0.5;

    curr = hwCone->create( hwCone );
    HW_MODIFY_1F( curr, hwStrRadius, Radius );
    HW_MODIFY_1I( curr, hwStrGraphN, 2 );
    HW_MODIFY_1I( curr, hwStrGraphM, FACETS+1 );
    HW_MODIFY_3F( curr, hwStrRotate, 90.0, 0.0, 0.0 );
    HW_MODIFY_1F( curr, hwStrHeight, Height-capHeight );
    HW_MODIFY_3F( curr, hwStrColor, 1.0, 0.0, 0.0 );
    oList[nObjs++] = curr;

    curr = hwSphere->create( hwSphere );
    HW_MODIFY_1F( curr, hwStrRadius, capRadius );
    HW_MODIFY_2F( curr, hwStrLatRange, 30.0, 90.0 );
    HW_MODIFY_1I( curr, hwStrGraphN, 3 );
    HW_MODIFY_1I( curr, hwStrGraphM, (FACETS+1) );
    ALUMINUM_HW( curr );
    HW_MODIFY_1F( curr, hwStrShininess, 0.1 );
    HW_MODIFY_3F( curr, hwStrSpecColor, 0.5, 0.5, 0.5 );
    HW_MODIFY_3F( curr, hwStrPos, 0.0, Height-capRadius, 0.0 );
    HW_MODIFY_3F( curr, hwStrRotate, 90.0, 0.0, 0.0 );
    oList[nObjs++] = curr;

    obj->display_list = createHwSegmentFromObj( oList, nObjs );
}


void init_silo_object(
    DRIVE_OBJECT *obj)
{
    SILO_LIST *rl;

    if (Height == 0.0) Height = DEFAULT_HEIGHT;
    if (Radius == 0.0) Radius = DEFAULT_RADIUS;

    if(debug) printf(" inside init_silo_object() routine \n");

    obj->num_children = 0;

    /* See if we've created one like this before... */
    rl = silo_list;
    while (rl != NULL) {
	if (IS_NEAR(rl->radius,Radius)
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
	if ((rl = (SILO_LIST *) malloc(sizeof(SILO_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}
	rl->radius = Radius;
	rl->height = Height;
	rl->nameset_bits = obj->nameset_bits;
	rl->next   = silo_list;
    	create_silo_graphics(obj);
	rl->dl_number = obj->display_list;
	silo_list  = rl;
    }

    obj->surface_chars_xyz  = silo_surface_chars_xyz;
    obj->surface_chars_bbox = silo_surface_chars_bbox;

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
    elevate_object_to_terrain_height((SCENE *) obj->scene,obj,TRUE);
}
