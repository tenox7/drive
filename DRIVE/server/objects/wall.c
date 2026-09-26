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


/* Code Module for various walls */

#include <stdio.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "prims.h"
#include "obj_common.h"
#include "demo_physics.h"

/* #define INTEGRATE_WALL_SIZE */

#define Length		(obj->size[SIZE_LENGTH])
#define Width		(obj->size[SIZE_WIDTH])
#define Height		(obj->size[SIZE_HEIGHT])
#define DEFAULT_LENGTH	(100.0)
#define DEFAULT_WIDTH	(5.0)
#define DEFAULT_HEIGHT	(4.0)

typedef struct _wall_list {
    float r,g,b;
    float width,height,length;
    unsigned int nameset_bits;
    int dl_number;
    struct _wall_list *next;
} WALL_LIST;
static WALL_LIST *wall_list = NULL;


static int wall_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = 1.0;
    get_box_mc_normal(obj,x,y,z,sc->mc_normal);
    return(TRUE);
}


static int wall_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = 1.0;
    return(TRUE);
}



static void create_wall_graphics(
    DRIVE_OBJECT *obj)
{
    float list[6];
    float *ptr;
    hwObject
	wall;

    wall = hwBox->create(hwBox);
    wall->name = 0;

    ptr = list;
#ifdef INTEGRATE_WALL_SIZE
    *ptr++ = -0.5 * Width;	/* Lower Left Front Corner */
    *ptr++ = 0.0;
    *ptr++ = 0.0;
    *ptr++ = 0.5 * Width;	/* Upper Right Back Corner */
    *ptr++ = Height;
    *ptr++ = Length;
#else
    *ptr++ = -0.5;	/* Lower Left Front Corner */
    *ptr++ = 0.0;
    *ptr++ = 0.0;
    *ptr++ = 0.5;	/* Upper Right Back Corner */
    *ptr++ = 1.0;
    *ptr++ = 1.0;
#endif

    HW_MODIFY_3F(wall, hwStrColor,obj->color[0],obj->color[1],obj->color[2]);
    wall->modify(wall, hwStrData, HW_MAKE_TYPE(HW_TYPE_FLOAT, 6), list);
    HW_OBJECT_NAMESET(wall,obj);

    obj->display_list = createHwSegmentFromObj( &wall, 1);
}


/* a wall unelevated above terrain */
void init_fillwall_object(
    DRIVE_OBJECT *obj)
{
    static float scalemat[4][4] = IDENTITY4x4;
    WALL_LIST *wl;


    if (Length == DEFAULT_OBJECT_SIZE) Length = DEFAULT_LENGTH;
    if (Width  == DEFAULT_OBJECT_SIZE) Width  = DEFAULT_WIDTH;
    if (Height == DEFAULT_OBJECT_SIZE) Height = DEFAULT_HEIGHT;
    if ((obj->color[0] == DEFAULT_OBJECT_COLOR)
	    || (obj->color[1] == DEFAULT_OBJECT_COLOR)
	    || (obj->color[2] == DEFAULT_OBJECT_COLOR)) {
	/* use concrete */
	obj->color[0] = obj->color[1] = obj->color[2] = CONCRETE_INTENSITY;
    }

    /* Look for one the same color */
    wl = wall_list;
    while (wl != NULL) {
	if (IS_NEAR(wl->r,obj->color[0])
		&& IS_NEAR(wl->g,obj->color[1])
		&& IS_NEAR(wl->b,obj->color[2])
#ifdef INTEGRATE_WALL_SIZE
		&& IS_NEAR(wl->height,Height)
		&& IS_NEAR(wl->width,Width)
		&& IS_NEAR(wl->length,Length)
#endif
		&& (wl->nameset_bits == obj->nameset_bits)) {
	    break;
	}
	wl = wl->next;
    }
    if (wl != NULL) {
	/* Reuse this one. */
	obj->display_list = wl->dl_number;
    }
    else {
	/* Gotta create a new one. */
	if ((wl = (WALL_LIST *) malloc(sizeof(WALL_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space.\n");
	    return;
	}
	wl->r = obj->color[0];
	wl->g = obj->color[1];
	wl->b = obj->color[2];
#ifdef INTEGRATE_WALL_SIZE
	wl->width = Width;
	wl->length = Length;
	wl->height = Height;
#endif
	wl->nameset_bits = obj->nameset_bits;
	create_wall_graphics(obj);
	wl->dl_number = obj->display_list;
	wl->next = wall_list;
	wall_list = wl;
    }

#ifndef INTEGRATE_WALL_SIZE
    /* Scale it up to the proper size */
    scalemat[0][0] = Width;
    scalemat[1][1] = Height;
    scalemat[2][2] = Length;
    concat_matrix(scalemat,obj->xform,obj->xform);

    scalemat[0][0] = 1.0/Width;
    scalemat[1][1] = 1.0/Height;
    scalemat[2][2] = 1.0/Length;
    concat_matrix(obj->ixform,scalemat,obj->ixform);
#endif

    obj->surface_chars_xyz  = wall_surface_chars_xyz;
    obj->surface_chars_bbox = wall_surface_chars_bbox;

#ifdef INTEGRATE_WALL_SIZE
    obj->bound_mc[0] = -0.5 * Width;
    obj->bound_mc[1] =  0.0;
    obj->bound_mc[2] =  0.0;
    obj->bound_mc[3] =  0.5 * Width;
    obj->bound_mc[4] =  1.0 * Height;
    obj->bound_mc[5] =  1.0 * Length;
#else
    obj->bound_mc[0] = -0.5;
    obj->bound_mc[1] =  0.0;
    obj->bound_mc[2] =  0.0;
    obj->bound_mc[3] =  0.5;
    obj->bound_mc[4] =  1.0;
    obj->bound_mc[5] =  1.0;
#endif

    /* Update WC bounding box */
    update_wc_bounds(obj);
}


void init_wall_object(
    DRIVE_OBJECT *obj)
{
    init_fillwall_object(obj);
    elevate_object_to_terrain_height((SCENE *) obj->scene,obj,TRUE);
}
