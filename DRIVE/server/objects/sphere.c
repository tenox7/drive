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


/* Code Module for the sphere object */


#include <stdio.h>
#include <math.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"

#define DEFAULT_RADIUS	(10.0)

#define Radius			(obj->radius)

typedef struct _sphere_list {
    float radius;
    float r,g,b;
    unsigned int nameset_bits;
    int dl_number;
    struct _sphere_list *next;
} SPHERE_LIST;
static SPHERE_LIST *sphere_list=NULL;



static int sphere_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    float r;

    if (y < 0.0) {
	/* under sphere */
	if (x*x + y*y + z*z > Radius*Radius) return(FALSE);
    }

    r = HYPOT2(x,z);
    if (r > Radius) {
	return(FALSE);
    }
    else {
	sc->mc_y = FSQRT(Radius*Radius - x*x - z*z);
	sc->mc_normal[0] = x/Radius;
	sc->mc_normal[1] = 1.0 - r/Radius;
	sc->mc_normal[2] = z/Radius;
	return(TRUE);
    }
}


static int sphere_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    float x,y,z;

    find_bbox_point_closest_to_point(bbox_mc, 0.0,0.0,0.0, &x,&y,&z);

    if (POINT_IN_BBOX(bbox_mc,0.0,0.0,0.0)) {
	/* Center of sphere is in bounding box. */
	/* Return point on sphere opposite closest bbox point */
	NORMALIZE3(x,y,z);
	sc->mc_x = -x * Radius;
	sc->mc_y = -z * Radius;
	sc->mc_z = -z * Radius;
	return(TRUE);
    }
    else {
	return(sphere_surface_chars_xyz(obj,x,y,z,sc));
    }
}


static void create_sphere_graphics(
    DRIVE_OBJECT *obj)
{
    int lats,longs;
    hwObject curr;

    if ((longs = (int) Radius + 2.0) > 16) longs = 16;
    if (longs < 6) longs = 6;
    if ((lats = longs/2) < 4) lats = 4;

    curr = hwSphere->create( hwSphere );
    HW_MODIFY_1I( curr, hwStrGraphN, lats );
    HW_MODIFY_1I( curr, hwStrGraphM, longs );
    HW_MODIFY_3F( curr, hwStrColor,
		obj->color[0], obj->color[1], obj->color[2] );
    HW_MODIFY_1F( curr, hwStrRadius, Radius );
    HW_OBJECT_NAMESET( curr, obj );

    obj->display_list = createHwSegmentFromObj( &curr, 1 );
}


void init_sphere_object(
    DRIVE_OBJECT *obj)
{
    SPHERE_LIST *rl;

    if (Radius == DEFAULT_OBJECT_RADIUS) Radius = DEFAULT_RADIUS;
    if ((obj->color[0] == DEFAULT_OBJECT_COLOR)
	    || (obj->color[1] == DEFAULT_OBJECT_COLOR)
	    || (obj->color[2] == DEFAULT_OBJECT_COLOR)) {
	obj->color[0] = obj->color[1] = obj->color[2] = CONCRETE_INTENSITY;
    }

    obj->num_children = 0;

    /* See if we've created one like this before... */
    rl = sphere_list;
    while (rl != NULL) {
	if (IS_NEAR(rl->radius,Radius)
		&& IS_NEAR(rl->r,obj->color[0])
		&& IS_NEAR(rl->g,obj->color[1])
		&& IS_NEAR(rl->b,obj->color[2])
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
	if ((rl = (SPHERE_LIST *) malloc(sizeof(SPHERE_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}
	rl->radius = Radius;
	rl->next   = sphere_list;
	rl->r      = obj->color[0];
	rl->g      = obj->color[1];
	rl->b      = obj->color[2];
	rl->nameset_bits = obj->nameset_bits;
    	create_sphere_graphics(obj);
	rl->dl_number = obj->display_list;
	sphere_list  = rl;
    }

    obj->surface_chars_xyz  = sphere_surface_chars_xyz;
    obj->surface_chars_bbox = sphere_surface_chars_bbox;

    /* Initial (mc) bounding box values */
    obj->bound_mc[0] = -Radius;
    obj->bound_mc[1] = -Radius;
    obj->bound_mc[2] = -Radius;
    obj->bound_mc[3] = Radius;
    obj->bound_mc[4] = Radius;
    obj->bound_mc[5] = Radius;

    /* Apply the object's xform matrix to the bounding box to put it in
     * world coordinates.
     */
    update_wc_bounds(obj);
}
