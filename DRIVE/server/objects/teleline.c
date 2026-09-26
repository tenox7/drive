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


/* Code Module for the teleline segment object */


#include <stdio.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"

#define Length	(obj->size[SIZE_LENGTH])
#define Height	(obj->size[SIZE_HEIGHT])
#define Radius	(obj->radius)
#define Spacing (obj->spacing)
#define HMid	(Height*0.8)

#define DEFAULT_TELELINE_LENGTH		(500.0)
#define DEFAULT_TELELINE_SPACING	(100.0)
#define DEFAULT_TELELINE_HEIGHT		(20.0)
#define DEFAULT_TELELINE_RADIUS		(0.4)
#define LINE_GRANULARITY		(16)
#define WIRE_OFFSET			(0.25)

#define XBAR_LENGTH			(8.0)
#define XBAR_WIDTH			(1.0)
#define XBAR_HEIGHT			(0.5)
#define XBAR_OFFSET			(-2.0)


typedef struct _teleline_list {
    float length, height, radius, spacing;
    unsigned int nameset_bits;
    int dl_number;
    struct _teleline_list *next;
} TELELINE_LIST;
static TELELINE_LIST *teleline_list=NULL;


static int teleline_surface_chars_xyz(
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

static int teleline_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    float x,y,z;

    find_bbox_point_closest_to_point(bbox_mc, 0.0,0.0,0.0, &x,&y,&z);
    if (POINT_IN_BBOX(bbox_mc,0.0,(bbox_mc[1]+bbox_mc[4])/2.0,0.0)) {
        /* Center of pillar is in bounding box. */
        /* Return point on pillar opposite closest bbox point */
        NORMALIZE3(x,y,z);
        sc->mc_x = -x * Radius;
        sc->mc_y = obj->size[SIZE_HEIGHT];
        sc->mc_z = -z * Radius;
        return(TRUE);
    }

    /* else */
    sc->mc_x = x;
    sc->mc_y = obj->size[SIZE_HEIGHT];
    sc->mc_z = z;
    return(TRUE);
}


static hwObject single_pole(
    DRIVE_OBJECT *obj,
    float z)
{
    hwObject curr, oList[10];
    int nObjs = 0;

    curr = hwBox->create( hwBox );
    HW_MODIFY_3F( curr, hwStrScale,XBAR_LENGTH/2.,XBAR_HEIGHT/2.,XBAR_WIDTH/2.);
    HW_MODIFY_3F( curr, hwStrPos, 0.0, Height+XBAR_OFFSET+XBAR_HEIGHT/2.0, z );
    oList[nObjs++] = curr;

    curr = hwCone->create( hwCone );
    HW_MODIFY_1I( curr, hwStrGraphN, 2 );
    HW_MODIFY_1I( curr, hwStrGraphM, 5 );
    HW_MODIFY_1F( curr, hwStrRadius, Radius );
    HW_MODIFY_1F( curr, hwStrHeight, Height );
    HW_MODIFY_3F( curr, hwStrRotate, 90.0, 0.0, 0.0 );
    HW_MODIFY_3F( curr, hwStrPos, 0.0, 0.0, z );
    oList[nObjs++] = curr;

    curr = hwGroup->create( hwGroup );
    curr->modify( curr, hwStrChildren,
		HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
    return curr;
}


static int create_wires(
    DRIVE_OBJECT *obj,
    int segments,
    hwObject *oList )
{
    float *pline,*fptr,*fptr2;
    float z,b,zinc;
    int i,j;
    hwObject curr;

    if ((pline = (float *) malloc(3*sizeof(float)*LINE_GRANULARITY*segments))
	    == NULL) {
	return(-1);
    }

    /* The equation for the wires is the catenary (cosh):
     * height = a/2*(e^(-x/b)+e^(x/b))
     *  where a is the height at the midpoint,
     *        b is l/(2*ln((h+sqrt(h^2-a^2))/a))
     *     where l is the spacing between the poles
     *           h is the height at the endpoints.
     */
    b = Spacing / 
	(2.0*FLN((Height + FSQRT(Height*Height - HMid*HMid)) / HMid));
    fptr = (float *) pline;
    zinc = Spacing / (LINE_GRANULARITY-1);
    for (z = -Spacing/2.0; z <= Spacing/2.0+.0001; z += zinc) {
	*fptr++ = (XBAR_LENGTH/3.0);
	*fptr++ = HMid/2.0*(FEXP(-z/b) + FEXP(z/b))
		+ (XBAR_OFFSET + XBAR_HEIGHT + WIRE_OFFSET);
	*fptr++ = z + Spacing/2.0;
    }
    /* Duplicate to the rest of the segments. */
    for (i=1,z=Spacing; i<segments; ++i,z+=Spacing) {
	fptr2 = (float *) pline;
	for (j=0; j<LINE_GRANULARITY; ++j) {
	    *fptr++ = *fptr2++;
	    *fptr++ = *fptr2++;
	    *fptr++ = *fptr2++ + z;
	}
    }

    curr = hwPolyline->create( hwPolyline );
    HW_MODIFY_3F( curr, hwStrColor, 0.5, 0.5, 0.5 );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,LINE_GRANULARITY*segments*3),
		pline );
    oList[0] = curr;

    fptr = (float *) pline;
    for (i=0; i<LINE_GRANULARITY*segments; ++i) {
	*fptr = -(XBAR_LENGTH/3.0);
	fptr += 3;
    }
    curr = hwPolyline->create( hwPolyline );
    HW_MODIFY_3F( curr, hwStrColor, 0.5, 0.5, 0.5 );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,LINE_GRANULARITY*segments*3),
		pline );
    oList[1] = curr;

    free(pline);

    return 2;
}


static void create_teleline_graphics(
    DRIVE_OBJECT *obj)
{
    float z;
    int i;
    hwObject curr, oList[100];
    int nObjs = 0;

    for (z=0.0,i=0; i<obj->num_children; z+=Spacing,++i) {
	curr = single_pole(obj,z);
	WOOD_HW( curr );
	oList[nObjs++] = curr;
    }
    nObjs += create_wires(obj,obj->num_children-1,oList+nObjs);

    curr = hwGroup->create( hwGroup );
    curr->modify( curr, hwStrChildren,
		HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );

    HW_OBJECT_NAMESET(curr,obj);
    obj->display_list = createHwSegmentFromObj( &curr, 1 );
}


void init_teleline_object(
    DRIVE_OBJECT *obj)
{
    TELELINE_LIST *ll;
    float z;
    int i;
    DRIVE_OBJECT *child;
    static float mat[4][4] = IDENTITY4x4;

    if (Length <= 0.0)  Length  = DEFAULT_TELELINE_LENGTH;
    if (Height <= 0.0)  Height  = DEFAULT_TELELINE_HEIGHT;
    if (Radius <= 0.0)  Radius  = DEFAULT_TELELINE_RADIUS;
    if (Spacing <= 0.0) Spacing = DEFAULT_TELELINE_SPACING;

    if(debug) printf(" inside init_teleline_%d_object() routine \n",
	(int) Height);

    obj->num_children = Length/Spacing;

    /* See if we've created one like this before... */
    ll = teleline_list;
    while (ll != NULL)
    {
	if (IS_NEAR(ll->height,Height)
		&& IS_NEAR(ll->radius,Radius)
	        && IS_NEAR(ll->spacing,Spacing)
	        && (ll->nameset_bits == obj->nameset_bits)
	        && IS_NEAR(ll->length,Length))
	    break;
	ll = ll->next;
    }

    if (ll != NULL) 
    {
	/* Good -- I have one like this already. */
	obj->display_list = ll->dl_number;
    }
    else
    {
	/* Nope -- gotta create a new one. */
	if ((ll = (TELELINE_LIST *) malloc(sizeof(TELELINE_LIST))) == NULL)
	{
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}

	ll->length  = Length;
	ll->height  = Height;
	ll->radius  = Radius;
	ll->spacing = Spacing;
	ll->nameset_bits = obj->nameset_bits;
	ll->next    = teleline_list;

    	create_teleline_graphics(obj);

	ll->dl_number = obj->display_list;
	teleline_list  = ll;
    }


    /* Initial (mc) bounding box values */
    obj->bound_mc[0] = -Radius;
    obj->bound_mc[1] = 0.0;
    obj->bound_mc[2] = -Radius;
    obj->bound_mc[3] = Radius;
    obj->bound_mc[4] = Height;
    obj->bound_mc[5] = Radius + Length;
    update_wc_bounds(obj);

    obj->surface_chars_xyz  = teleline_surface_chars_xyz;
    obj->surface_chars_bbox = teleline_surface_chars_bbox;

    /* Now create subobjects for each pole */
    for (i=0,z=0.0; i<obj->num_children; ++i,z+=Spacing) {
	if ((child = (DRIVE_OBJECT *) malloc(sizeof(DRIVE_OBJECT))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    break;
	}
	/* as a good starting point, clone myself */
	memcpy(child,obj,sizeof(DRIVE_OBJECT));
	/* now change relevant parts */
	child->num_children = 0;
	child->child_list = NULL;
	child->display_list = 0;
	mat[3][2] = z;
	concat_matrix(mat,obj->xform,child->xform);
	_hp_invert(child->xform,child->ixform,0);
	child->bound_mc[0] = -Radius;
	child->bound_mc[1] = 0.0;
	child->bound_mc[2] = -Radius;
	child->bound_mc[3] = Radius;
	child->bound_mc[4] = Height;
	child->bound_mc[5] = Radius;
	update_wc_bounds(child);
	add_object_to_list(&(obj->child_list),child);
    }
}
