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


/* Code Module for the rubble object */


#include <stdio.h>
#include <math.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"
#include "scene.h"

#define DEFAULT_HEIGHT	5.0
#define DEFAULT_LENGTH	10.0
#define DEFAULT_WIDTH	10.0
#define DEFAULT_COUNT	10

#define DEFAULT_RED	(0.1)
#define DEFAULT_GRN	(0.1)
#define DEFAULT_BLU	(0.1)

#define Length			(obj->size[SIZE_LENGTH])
#define Width			(obj->size[SIZE_WIDTH])
#define Height			(obj->size[SIZE_HEIGHT])

typedef struct _rubble_list {
    float color[3];
    int dl_number;
    struct _rubble_list *next;
} RUBBLE_LIST;
static RUBBLE_LIST *rubble_list = NULL;

#define MESH_SIZE	5
#define DELTA		(1.0/(MESH_SIZE-1))


static int rubble_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = 1.0;
    get_box_mc_normal(obj,x,y,z,sc->mc_normal);
    return(TRUE);
}


static int rubble_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = 1.0;
    return(TRUE);
}



static void create_rubble_graphics(
    DRIVE_OBJECT *obj)
{
    float mesh[MESH_SIZE][MESH_SIZE][3];
    int i,j;
    float x,z;
    hwObject curr;


    for (z= -0.5,j=0; j<MESH_SIZE; z+=DELTA,++j) {
	for (x= -0.5,i=0; i<MESH_SIZE; x+=DELTA,++i) {
	    mesh[j][i][0] = x + BOUNDED_FLOATRAND(-(DELTA/2),(DELTA/2));
	    if ((j == 0) || (j == (MESH_SIZE-1))
		    || (i == 0) || (i == (MESH_SIZE-1))) {
		mesh[j][i][1] = 0.0;
	    }
	    else {
		mesh[j][i][1] = BOUNDED_FLOATRAND(0.5,1.0);
	    }
	    mesh[j][i][2] = z + BOUNDED_FLOATRAND(-(DELTA/2),(DELTA/2));
	}
    }
    curr = hwMesh->create( hwMesh );
    HW_MODIFY_3F( curr, hwStrColor,
		obj->color[0],obj->color[1],obj->color[2]);
    HW_MODIFY_1I( curr, hwStrGraphN, MESH_SIZE );
    HW_MODIFY_1I( curr, hwStrGraphM, MESH_SIZE );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,MESH_SIZE*MESH_SIZE*3), mesh );

    obj->display_list = createHwSegmentFromObj( &curr, 1 );
}


void init_rubble_object(
    DRIVE_OBJECT *obj)
{
    static float scalemat[4][4] = IDENTITY4x4;
    RUBBLE_LIST *rl;

    if (Height == DEFAULT_OBJECT_SIZE) Height = DEFAULT_HEIGHT;
    if (Length == DEFAULT_OBJECT_SIZE) Length = DEFAULT_LENGTH;
    if (Width  == DEFAULT_OBJECT_SIZE) Width  = DEFAULT_WIDTH;
    if (obj->color[0] == DEFAULT_OBJECT_COLOR) obj->color[0] = DEFAULT_RED;
    if (obj->color[1] == DEFAULT_OBJECT_COLOR) obj->color[1] = DEFAULT_GRN;
    if (obj->color[2] == DEFAULT_OBJECT_COLOR) obj->color[2] = DEFAULT_BLU;

    obj->num_children = 0;

    /* See if we've created one like this before... */
    rl = rubble_list;
    while (rl != NULL) {
	if (IS_NEAR(obj->color[0],rl->color[0])
		&& IS_NEAR(obj->color[1],rl->color[1])
		&& IS_NEAR(obj->color[2],rl->color[2])) {
	    break;
	}
	rl = rl->next;
    }
    if (rl != NULL) {
	/* Reuse it */
	obj->display_list = rl->dl_number;
    }
    else {
    	create_rubble_graphics(obj);
	if ((rl = (RUBBLE_LIST *) malloc(sizeof(RUBBLE_LIST))) != NULL) {
	    rl->color[0] = obj->color[0];
	    rl->color[1] = obj->color[1];
	    rl->color[2] = obj->color[2];
	    rl->dl_number = obj->display_list;
	    rl->next = rubble_list;
	    rubble_list = rl;
	}
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

    obj->surface_chars_xyz  = rubble_surface_chars_xyz;
    obj->surface_chars_bbox = rubble_surface_chars_bbox;

    /* Initial (mc) bounding box values */
    obj->bound_mc[0] = -0.5;
    obj->bound_mc[1] = 0.0;
    obj->bound_mc[2] = -0.5;
    obj->bound_mc[3] = 0.5;
    obj->bound_mc[4] = 1.0;
    obj->bound_mc[5] = 0.5;
    update_wc_bounds(obj);
}
