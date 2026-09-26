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


/* Code Module for the flames object */


#include <stdio.h>
#include <math.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"
#include "scene.h"

#define DEFAULT_HEIGHT	10.0
#define DEFAULT_LENGTH	10.0
#define DEFAULT_WIDTH	10.0
#define DEFAULT_COUNT	10

#define Length			(obj->size[SIZE_LENGTH])
#define Width			(obj->size[SIZE_WIDTH])
#define Height			(obj->size[SIZE_HEIGHT])
#define Count			(obj->count)

typedef struct _flames_list {
    int count;
    int dl_number;
    struct _flames_list *next;
} FLAMES_LIST;
static FLAMES_LIST *flames_list=NULL;



static void create_flames_graphics(
    DRIVE_OBJECT *obj)
{
    int i,j,names;
    float poly[4][6];
    float xcenter, zcenter, height, phi, s_phi, c_phi;
    hwObject curr, oList[128];
    int nObjs = 0;


    /* Do constants */
    poly[0][3] = 0.5; poly[0][4] = 0.0; poly[0][5] = 0.0;
    poly[1][3] = 1.0; poly[1][4] = 0.4; poly[1][5] = 0.0;
    poly[2][3] = 1.0; poly[2][4] = 1.0; poly[2][5] = 0.0;
    poly[3][3] = 1.0; poly[3][4] = 0.4; poly[3][5] = 0.0;

    for (i=0; i<Count; ++i) {
	xcenter = BOUNDED_FLOATRAND(-0.5,0.5);
	zcenter = BOUNDED_FLOATRAND(-0.5,0.5);
	for (j=0; j<4; ++j) {
	    height = BOUNDED_FLOATRAND(0.4,1.0);
	    phi = FLOATRAND(M_PI*2.0);
	    s_phi = FSIN(phi);
	    c_phi = FCOS(phi);
	    names = TIMED_NAMESET(j)
		| TIMED_NAMESET(j+4)
		| TIMED_NAMESET(j+8)
		| TIMED_NAMESET(j+12);
	    poly[0][0] = xcenter;
		poly[0][1] = 0.0;
		poly[0][2] = zcenter;
	    poly[1][0] = xcenter + height/5.0 * c_phi;
		poly[1][1] = height/3.0 + FLOATRAND(height/10.0);
		poly[1][2] = zcenter + height/5.0 * s_phi;
	    poly[2][0] = xcenter
		    + BOUNDED_FLOATRAND(-height/5.0,height/5.0);
		poly[2][1] = height;
		poly[2][2] = zcenter
		    + BOUNDED_FLOATRAND(-height/5.0,height/5.0);
	    poly[3][0] = xcenter - height/5.0 * c_phi;
		poly[3][1] = height/3.0 + FLOATRAND(height/10.0); 
		poly[3][2] = zcenter - height/5.0 * s_phi;
	    curr = hwPolygon->create( hwPolygon );
	    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
	    HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
	    HW_MODIFY_1B( curr, hwStrHasRGB, HW_TRUE );
	    HW_MODIFY_1I( curr, hwStrVisibility, names );
	    curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,4*6), poly );
	    oList[nObjs++] = curr;
	}
    }

    curr = hwGroup->create( hwGroup );
    curr->modify( curr, hwStrChildren,
		HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
    obj->display_list = createHwSegmentFromObj( &curr, 1 );
}


void init_flames_object(
    DRIVE_OBJECT *obj)
{
    static float scalemat[4][4] = IDENTITY4x4;
    FLAMES_LIST *rl;


    if (Height == DEFAULT_OBJECT_SIZE) Height = DEFAULT_HEIGHT;
    if (Length == DEFAULT_OBJECT_SIZE) Length = DEFAULT_LENGTH;
    if (Width  == DEFAULT_OBJECT_SIZE) Width  = DEFAULT_WIDTH;
    if (Count  == DEFAULT_OBJECT_COUNT) Count = DEFAULT_COUNT;

    obj->num_children = 0;

    /* See if we've created one like this before... */
    rl = flames_list;
    while (rl != NULL) {
	if (rl->count == Count) break;
	/* else */
	rl = rl->next;
    }
    if (rl != NULL) {
	/* Good -- I have one like this already. */
	obj->display_list = rl->dl_number;
    }
    else {
	/* Nope -- gotta create a new one. */
	if ((rl = (FLAMES_LIST *) malloc(sizeof(FLAMES_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}
	rl->count = Count;
	rl->next  = flames_list;
    	create_flames_graphics(obj);
	rl->dl_number = obj->display_list;
	flames_list  = rl;
    }

    /* Scale it */
    scalemat[0][0] = Width;
    scalemat[1][1] = Height;
    scalemat[2][2] = Length;
    concat_matrix(scalemat,obj->xform,obj->xform);

    scalemat[0][0] = 1.0/Width;
    scalemat[1][1] = 1.0/Height;
    scalemat[2][2] = 1.0/Length;
    concat_matrix(obj->ixform,scalemat,obj->ixform);

    /* Initial (mc) bounding box values */
    obj->bound_mc[0] = -0.5;
    obj->bound_mc[1] = 0.0;
    obj->bound_mc[2] = -0.5;
    obj->bound_mc[3] = 0.5;
    obj->bound_mc[4] = 1.0;
    obj->bound_mc[5] = 0.5;
    update_wc_bounds(obj);
}
