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


/* Code Module for the ramp segment object */


#include <stdio.h>
#include <math.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"

#define DEFAULT_HEIGHT (10.0)
#define DEFAULT_LENGTH (100.0)
#define DEFAULT_WIDTH	DEFAULT_ROAD_WIDTH

#define Length			(obj->size[SIZE_LENGTH])
#define Width			(obj->size[SIZE_WIDTH])
#define Height			(obj->size[SIZE_HEIGHT])
#define BankWidth		(Height/2.0)

typedef struct _ramp_list {
    float length,width,height;
    unsigned int nameset_bits;
    int dl_number;
    struct _ramp_list *next;
} RAMP_LIST;
static RAMP_LIST *ramp_list=NULL;



static int ramp_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    float yp,nx,ny,nz,fullwidth,top;
    float absx = ABS(x);

    if (absx <= Width/2.0) {
	/* on ramp itself */
	sc->mc_y = z * (Height/Length);
	if ((y > sc->mc_y - BBOX_MARGIN)
		|| (z < Length-5.0)) {
	    ny = Length;
	    nz = -Height;
	    NORMALIZE2(ny,nz);
	    sc->mc_normal[0] = 0.0;
	    sc->mc_normal[1] = ny;
	    sc->mc_normal[2] = nz;
	}
	else {
	    /* Must have run into the end */
	    sc->mc_normal[0] = 0.0;
	    sc->mc_normal[1] = 0.0;
	    sc->mc_normal[2] = 1.0;
	}
	return(TRUE);
    }
    else {
	fullwidth = Width/2.0 + BankWidth*z/Length;
	if (absx < fullwidth) {
	    /* On bank! */
	    top = Height * z/Length;
	    yp = (1.0 - (absx-Width/2.0)/(fullwidth-Width/2.0)) * top;
	    /* To keep people from climbing the walls, bump this to 5.0
	     * if we're far enough into the ramp.
	     */
	    if ((yp < 5.0) && (top > 5.0)) yp = 5.0;
	    sc->mc_y = yp;
	    if ((y > sc->mc_y - BBOX_MARGIN)
		    || (z < Length-5.0)
		    || (x > Width/2.0+BankWidth-3.0)) {
		nx = Length*Height;
		ny = Length*BankWidth;
		nz = -Height*BankWidth;
		NORMALIZE3(nx,ny,nz);
		if (x <= 0.0) nx = -nx;
		sc->mc_normal[0] = nx;
		sc->mc_normal[1] = ny;
		sc->mc_normal[2] = nz;
	    }
	    else {
		/* Must have run into the end */
		sc->mc_normal[0] = 0.0;
		sc->mc_normal[1] = 0.0;
		sc->mc_normal[2] = 1.0;
	    }
	    return(TRUE);
	}
    }
    /* else */
    return(FALSE);
}


static int ramp_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    return(checkbounds_bbox(obj,bbox_mc,sc,ramp_surface_chars_xyz));
}


static void create_ramp_graphics(
    DRIVE_OBJECT *obj)
{
    float pgon[8][3];
    float *ptr;
    float *stripe_data,z,z2;
    int
	stripe_count,
	actual_stripe_count,
	cur=0;
    hwObject
	objList[5],
	rampGroup;

    cur =0;
    objList[cur] = hwPolygon->create(hwPolygon);

    ASPHALT_HW(objList[cur]);
    ptr = (float *) pgon;
    *ptr++ = -Width/2.0;
	*ptr++ = 0.0;
	*ptr++ = 0.0;
    *ptr++ = -Width/2.0;
	*ptr++ = Height;
	*ptr++ = Length;
    *ptr++ = Width/2.0;
	*ptr++ = Height;
	*ptr++ = Length;
    *ptr++ = Width/2.0;
	*ptr++ = 0.0;
	*ptr++ = 0.0;

    HW_MODIFY_1I(objList[cur], hwStrGraphN, 4);
    objList[cur]->modify(objList[cur], hwStrData,  /* Ramp polygon */
	HW_MAKE_TYPE(HW_TYPE_FLOAT,12), pgon);

    cur++;
    objList[cur] = hwPolygon->create(hwPolygon);
    GRASS_HW(objList[cur]);

    ptr = (float *) pgon;
    *ptr++ = -Width/2.0;
	*ptr++ = 0.0;
	*ptr++ = 0.0;
    *ptr++ = -Width/2.0 - BankWidth;
	*ptr++ = 0.0;
	*ptr++ = Length;
    *ptr++ = -Width/2.0;
	*ptr++ = Height;
	*ptr++ = Length;
    objList[cur]->modify(objList[cur], hwStrData,  
	HW_MAKE_TYPE(HW_TYPE_FLOAT,9), pgon);
    HW_MODIFY_1I(objList[cur], hwStrGraphN, 3);

    cur++;
    objList[cur] = hwPolygon->create(hwPolygon);
    GRASS_HW(objList[cur]);
    NEGATE_X(pgon,3);
    objList[cur]->modify(objList[cur], hwStrData, 
	HW_MAKE_TYPE(HW_TYPE_FLOAT,9), pgon);
    HW_MODIFY_1I(objList[cur], hwStrGraphN, 3);

    cur++;
    objList[cur] = hwPolygon->create(hwPolygon);
    ptr = (float *) pgon;
    *ptr++ = Width/2.0 + BankWidth;
	*ptr++ = 0.0;
	*ptr++ = Length;
    *ptr++ = Width/2.0;
	*ptr++ = Height;
	*ptr++ = Length;
    *ptr++ = -Width/2.0;
	*ptr++ = Height;
	*ptr++ = Length;
    *ptr++ = -Width/2.0 - BankWidth;
	*ptr++ = 0.0;
	*ptr++ = Length;
    objList[cur]->modify(objList[cur], hwStrData, 
	HW_MAKE_TYPE(HW_TYPE_FLOAT,12), pgon);
    HW_MODIFY_1I(objList[cur], hwStrGraphN, 4);

    cur++;
    objList[cur] = hwQuads->create(hwQuads);
    ROAD_LINE_YELLOW_HW(objList[cur]);
    HW_MODIFY_1B(objList[cur], hwStrBackface, HW_TRUE);

    stripe_count = (Length / ROAD_LINE_SPACING)+1;
    stripe_data = 
	(float *) malloc( stripe_count * 12 *sizeof(float));

    ptr = stripe_data;

    actual_stripe_count = 0;
    for (z=0; z<Length; z += ROAD_LINE_SPACING) {
	if ((z2 = z + ROAD_LINE_LENGTH) > Length) z2 = Length;

	*ptr++ = ROAD_LINE_WIDTH/2.0; /* x */
	*ptr++ = z/Length * Height + ROAD_LINE_FLOAT;
	*ptr++ = z;

	*ptr++ = ROAD_LINE_WIDTH/2.0; /* x */
	*ptr++ = z2/Length * Height + ROAD_LINE_FLOAT;
	*ptr++ = z2;

	*ptr++ = -ROAD_LINE_WIDTH/2.0; /* x */
	*ptr++ = z2/Length * Height + ROAD_LINE_FLOAT;
	*ptr++ = z2;

	*ptr++ = -ROAD_LINE_WIDTH/2.0;
	*ptr++ = z/Length * Height + ROAD_LINE_FLOAT;
	*ptr++ = z;
	actual_stripe_count++;
    }
    objList[cur]->modify(objList[cur], hwStrData, 
	HW_MAKE_TYPE(HW_TYPE_FLOAT,12 * actual_stripe_count), stripe_data);

    cur++;

    rampGroup = hwGroup->create(hwGroup);
    rampGroup->modify(rampGroup, hwStrChildren, 
	HW_MAKE_TYPE(HW_TYPE_OBJECT,cur), objList);

    obj->display_list = createHwSegmentFromObj(&rampGroup, 1);
    free(stripe_data);

}


void init_ramp_object(
    DRIVE_OBJECT *obj)
{
    RAMP_LIST *rl;

    if (Length <= 0.0)
	Length = DEFAULT_LENGTH;
    if (Width <= 0.0)
	Width = DEFAULT_WIDTH;
    if (Height <= 0.0)
	Height = DEFAULT_HEIGHT;

    if(debug) printf(" inside init_ramp_object() routine \n");

    obj->num_children = 0;

    /* See if we've created one like this before... */
    rl = ramp_list;
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
	if ((rl = (RAMP_LIST *) malloc(sizeof(RAMP_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}
	rl->length = Length;
	rl->width  = Width;
	rl->height = Height;
	rl->nameset_bits = obj->nameset_bits;
	rl->next   = ramp_list;
    	create_ramp_graphics(obj);
	rl->dl_number = obj->display_list;
	ramp_list  = rl;
    }

    obj->surface_chars_xyz  = ramp_surface_chars_xyz;
    obj->surface_chars_bbox = ramp_surface_chars_bbox;

    /* Initial (mc) bounding box values */
    obj->bound_mc[0] = -Width/2.0 - BankWidth;
    obj->bound_mc[1] = 0.0;
    obj->bound_mc[2] = 0.0;
    obj->bound_mc[3] = Width/2.0 + BankWidth;
    obj->bound_mc[4] = Height;
    obj->bound_mc[5] = Length;

    /* apply the object's xform matrix to the bounding box to put it in
     * world coordinates */
    update_wc_bounds(obj);
}


