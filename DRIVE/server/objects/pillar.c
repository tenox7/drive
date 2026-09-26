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


/* Code Module for the cone object */


#include <stdio.h>
#include <math.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"

#define DEFAULT_HEIGHT	(10.0)
#define DEFAULT_RADIUS	(1.0)

#define Height			(obj->size[SIZE_HEIGHT])
#define Width			(obj->size[SIZE_WIDTH])
#define Radius			(obj->radius)

typedef struct _cone_list {
    float width,radius,height;
    float r,g,b;
    unsigned int nameset_bits;
    int dl_number;
    struct _cone_list *next;
} CONE_LIST;
static CONE_LIST *cone_list=NULL;


static boolean_type coneheight(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    float *height)
{
    float a,b,r;

    a = HYPOT2(x,z);

    if (Radius <= Width) {
	/* top smaller than base */
	if (a < Radius) {
	    /* on top */
	    *height = Height;
	    return(TRUE);
	}
	else if (a < Width) {
	    /* no need to check Radius = Width, would have taken first if */
	    *height = (1.0 - (a-Radius)/(Width-Radius)) * Height;
	    return(TRUE);
	}
	else {
	    return(FALSE);
	}
    }
    else {
	/* bigger on top than on bottom */
	if ((y >= Height-BBOX_MARGIN) 
		|| (a < Width)) {
	    /* on top */
	    *height = Height;
	    return(TRUE);
	}
	else {
	    b = (y/Height);
	    r = (1.0-b)*Width + b*Radius;
	    if (a < r) {
		*height = Height;
		return(TRUE);
	    }
	    else {
		return(FALSE);
	    }
	}
    }
}



static int cone_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    float yc;

    if (coneheight(obj,x,y,z,&yc)) {
	sc->mc_y = yc;
	get_pole_mc_normal(obj,x,y,z,sc->mc_normal);
	if (!IS_NEAR(yc,Height)
		&& !IS_NEAR(Height,0.0)) {
	    /* on side */
	    sc->mc_normal[1] = (Width-Radius)/Height;
	    NORMALIZE3(sc->mc_normal[0],sc->mc_normal[1],sc->mc_normal[2]);
	}
	return(TRUE);
    }
    /* else */
    return(FALSE);
}


static int cone_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    float x,y,z;

    find_bbox_point_closest_to_point(bbox_mc, 0.0,0.0,0.0, &x,&y,&z);

    if (POINT_IN_BBOX_XZ(bbox_mc,0.0,0.0)) {
	/* Center of cone is in bounding box. */
	/* Return point on cone opposite closest bbox point */
	NORMALIZE3(x,y,z);
	sc->mc_x = -x * Radius;
	sc->mc_y = Height;
	sc->mc_z = -z * Radius;
	return(TRUE);
    }
    else {
	return(cone_surface_chars_xyz(obj,x,y,z,sc));
    }
}


static void create_cone_graphics(
    DRIVE_OBJECT *obj)
{
    int facets;
    float maxradius = MAX(Radius,Width);
    int i=0;
    hwObject
	coneGroup,
	objList[3];


    /* First, create the hwCone */
    objList[i] = hwCone->create(hwCone);
    HW_MODIFY_3F(objList[i], hwStrColor, 
	obj->color[0],obj->color[1],obj->color[2]);

    facets = maxradius*2.0 + 2.0;
    if (facets < 8*maxradius/Height) facets = 8*maxradius/Height;
    if (facets > 24) facets = 24;
    else if (facets < 3) facets = 3;

    /* Hmm.  Looks like 'Width' refers to the Bottom Radius, and 
    ** Radius is the top radius */
    HW_MODIFY_2F(objList[i], hwStrRadius, Width, Radius);
    HW_MODIFY_1F(objList[i], hwStrHeight, Height);
    HW_MODIFY_1I(objList[i], hwStrGraphN, 2 );
    HW_MODIFY_1I(objList[i], hwStrGraphM, facets + 1 );
    /* HoverWare creates the cone extruded along the Z axis.  Rotate it
    ** 90 degrees about the X axis to stand it upright */
    HW_MODIFY_3F(objList[i], hwStrRotate, 90,0,0);
    i++;   /* Move to the next object */


    /* Now, create the caps.  Use hwDisc */
    /* Bottom cap */
    if( Width > 0.01 ) {
	objList[i] = hwDisc->create(hwDisc);
	HW_MODIFY_3F(objList[i], hwStrColor, 
	    obj->color[0],obj->color[1],obj->color[2]);
	
	HW_MODIFY_3F(objList[i], hwStrRotate, 90,0,0);
	HW_MODIFY_1I(objList[i], hwStrGraphN, facets+1);
	HW_MODIFY_1F(objList[i], hwStrRadius, Width );
	i++;
    }

    /* Top Cap */
    if( Radius > 0.01 ) {
	objList[i] = hwDisc->create(hwDisc);
	HW_MODIFY_3F(objList[i], hwStrColor, 
	    obj->color[0],obj->color[1],obj->color[2]);

	HW_MODIFY_3F( objList[i], hwStrPos, 0, Height, 0 );
	HW_MODIFY_3F(objList[i], hwStrRotate, 90,0,0);
	HW_MODIFY_1I(objList[i], hwStrGraphN, facets+1);
	HW_MODIFY_1F(objList[i], hwStrRadius, Radius);
	HW_MODIFY_1B(objList[i], hwStrBackface, HW_TRUE);
	HW_MODIFY_1B(objList[i], hwStrFlipNormals, HW_TRUE);
	i++;
    }

    /* Now, create the group and assign the obj->display_list */
    coneGroup = hwGroup->create(hwGroup);
    coneGroup->modify(coneGroup, hwStrChildren, HW_MAKE_TYPE(HW_TYPE_OBJECT,i),
	objList);

    obj->display_list = createHwSegmentFromObj(&coneGroup, 1);
}


void init_cone_object(
    DRIVE_OBJECT *obj)
{
    CONE_LIST *rl;
    float maxradius;

    if (Height == DEFAULT_OBJECT_SIZE)   Height = DEFAULT_HEIGHT;
    if (Radius == DEFAULT_OBJECT_RADIUS) Radius = DEFAULT_RADIUS;
    if (Width  == DEFAULT_OBJECT_SIZE)   Width  = DEFAULT_RADIUS;
    if ((obj->color[0] == DEFAULT_OBJECT_COLOR)
	    || (obj->color[1] == DEFAULT_OBJECT_COLOR)
	    || (obj->color[2] == DEFAULT_OBJECT_COLOR)) {
	obj->color[0] = obj->color[1] = obj->color[2] = CONCRETE_INTENSITY;
    }

    if(debug) printf(" inside init_cone_object() routine \n");

    obj->num_children = 0;

    /* See if we've created one like this before... */
    rl = cone_list;
    while (rl != NULL) {
	if (IS_NEAR(rl->radius,Radius)
		&& IS_NEAR(rl->width,Width)
		&& IS_NEAR(rl->height,Height)
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
	if ((rl = (CONE_LIST *) malloc(sizeof(CONE_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}
	rl->radius = Radius;
	rl->width  = Width;
	rl->height = Height;
	rl->next   = cone_list;
	rl->r      = obj->color[0];
	rl->g      = obj->color[1];
	rl->b      = obj->color[2];
	rl->nameset_bits = obj->nameset_bits;
    	create_cone_graphics(obj);
	rl->dl_number = obj->display_list;
	cone_list  = rl;
    }

    obj->surface_chars_xyz  = cone_surface_chars_xyz;
    obj->surface_chars_bbox = cone_surface_chars_bbox;

    /* Initial (mc) bounding box values */
    maxradius = MAX(Radius,Width);
    obj->bound_mc[0] = -maxradius;
    obj->bound_mc[1] = 0.0;
    obj->bound_mc[2] = -maxradius;
    obj->bound_mc[3] = maxradius;
    obj->bound_mc[4] = Height;
    obj->bound_mc[5] = maxradius;

    /* apply the object's xform matrix to the bounding box to put it in
     * world coordinates */
    update_wc_bounds(obj);
}


void init_pillar_object(
    DRIVE_OBJECT *obj)
{
    /* Simply a cone with equal top and bottom radius */
    Width = Radius;

    init_cone_object(obj);
}
