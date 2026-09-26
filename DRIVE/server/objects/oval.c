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


/* Code Module for the oval segment object */


#include <stdio.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"

/* In modelling coordinates,
 * all of the ovals are circles of radius 1.0 centered at 0,0.
 * The xform is used to stretch them to the proper shape.
 * The only difference between different ovals is the number of
 * facets around the circle's edge.
 */

#define DEFAULT_OVAL_SIZE	100.0
#define EPSILON			(1.0e-6)
#define MAX_FACET_SIZE		5.0
#define MIN_FACETS		8
#define MAX_FACETS		32

#define Length				(obj->size[SIZE_LENGTH])
#define Width				(obj->size[SIZE_WIDTH])

typedef struct _oval_list {
    int facets;
    float r,g,b;
    boolean_type self_lit,transparent;
    unsigned int nameset_bits;
    int dl_number;
    struct _oval_list *next;
} OVAL_LIST;
static OVAL_LIST *oval_list=NULL;


static int oval_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    if (x*x + z*z <= 1.0) {
	sc->mc_y = 0.0;
	return(TRUE);
    }
    else {
	return(FALSE);
    }
}


static void create_oval_graphics(
    DRIVE_OBJECT *obj,
    boolean_type transparent,
    boolean_type self_lit,
    int facets)
{
    hwObject curr;

    curr = hwDisc->create( hwDisc );
    HW_MODIFY_1F( curr, hwStrRadius, 1.0 );
    HW_MODIFY_1I( curr, hwStrGraphN, (facets+1) );
    if( transparent )	HW_MODIFY_1F( curr, hwStrTransparency, 0.5 );
    if( self_lit )	HW_MODIFY_1B( curr, hwStrBright, HW_TRUE );
    HW_MODIFY_3F( curr, hwStrRotate, -90.0, 0.0, 0.0 );
    HW_MODIFY_3F( curr, hwStrColor,
		obj->color[0], obj->color[1], obj->color[2] );
    HW_OBJECT_NAMESET( curr, obj );

    obj->display_list = createHwSegmentFromObj( &curr, 1 );
}


static void do_oval_object(
    DRIVE_OBJECT *obj,
    boolean_type transparent,
    boolean_type self_lit)
{
    OVAL_LIST *ll;
    int facets;
    static float stretchmat[4][4] = IDENTITY4x4;

    if (Width  == DEFAULT_OBJECT_SIZE) Width  = DEFAULT_OVAL_SIZE;
    if (Length == DEFAULT_OBJECT_SIZE) Length = Width;

    if ((obj->color[0] == DEFAULT_OBJECT_COLOR)
	    || (obj->color[1] == DEFAULT_OBJECT_COLOR)
	    || (obj->color[2] == DEFAULT_OBJECT_COLOR)) {
	obj->color[0] = obj->color[1] = obj->color[2] =
	    ASPHALT_INTENSITY;
    }

    obj->num_children = 0;

    /* How many facets do we need? */
    if ((facets = M_PI*(Length+Width)/MAX_FACET_SIZE) < MIN_FACETS)
	facets = MIN_FACETS;
    else if (facets > MAX_FACETS) facets = MAX_FACETS;

    /* See if we've created one like this before... */
    ll = oval_list;
    while (ll != NULL) {
	if ((ABS(ll->facets - facets) < 2)
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
	if ((ll = (OVAL_LIST *) malloc(sizeof(OVAL_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}
	ll->facets = facets;
	ll->r      = obj->color[0];
	ll->g      = obj->color[1];
	ll->b      = obj->color[2];
	ll->transparent = transparent;
	ll->self_lit = self_lit;
	ll->nameset_bits = obj->nameset_bits;
	ll->next   = oval_list;
    	create_oval_graphics(obj,transparent,self_lit,facets);
	ll->dl_number = obj->display_list;
	oval_list  = ll;
    }

    obj->surface_chars_xyz  = oval_surface_chars_xyz;

    /* Stretch the thing to its proper size */
    stretchmat[0][0] = Width/2.0;
    stretchmat[2][2] = Length/2.0;
    concat_matrix(stretchmat,obj->xform,obj->xform);
    stretchmat[0][0] = 2.0/Width;
    stretchmat[2][2] = 2.0/Length;
    concat_matrix(obj->ixform,stretchmat,obj->ixform);

    /* Initialize bounding box values */
    obj->bound_mc[0] = -1.0;
    obj->bound_mc[1] = 0.0;
    obj->bound_mc[2] = -1.0;
    obj->bound_mc[3] = 1.0;
    obj->bound_mc[4] = BBOX_MARGIN;
    obj->bound_mc[5] = 1.0;

    update_wc_bounds(obj);
}


void init_oval_object(
    DRIVE_OBJECT *obj)
{
    do_oval_object(obj,FALSE,FALSE);
}


void init_oval_light_object(
    DRIVE_OBJECT *obj)
{
    do_oval_object(obj,FALSE,TRUE);
}


void init_oval_shadow_object(
    DRIVE_OBJECT *obj)
{
    obj->color[0] = obj->color[1] = obj->color[2] = SHADOW_INTENSITY;
    obj->xform[3][1] += ROAD_FLOAT;
	
    do_oval_object(obj,TRUE,FALSE);

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
