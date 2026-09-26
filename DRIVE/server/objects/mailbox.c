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


/* Code Module for the mailbox segment object */


#include <stdio.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"


#define HEIGHT		(4.0)
#define POST_RADIUS	(0.2)
#define BOX_WIDTH	(1.0)
#define BOX_HEIGHT	(1.0)
#define BOX_LENGTH	(2.0)

static int mailbox_dl_number = -1;
static int last_nameset_bits = ALL_TIMED_NAMESETS;


static int mailbox_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    if (x*x + z*z < POST_RADIUS*POST_RADIUS) {
	sc->mc_y = HEIGHT;
	get_pole_mc_normal(obj,x,y,z,sc->mc_normal);
	return(TRUE);
    }
    /* else */
    return(FALSE);
}



static int mailbox_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = HEIGHT;
    return(TRUE);
}


static void create_mailbox_graphics(
    DRIVE_OBJECT *obj)
{
    static float mesh[] = {
	BOX_WIDTH/2.0,HEIGHT-BOX_HEIGHT,-BOX_LENGTH/2.0,
	    1.0,0.0,0.0,
	BOX_WIDTH/2.0,HEIGHT-BOX_HEIGHT,BOX_LENGTH/2.0,
	    1.0,0.0,0.0,

	BOX_WIDTH/2.0,HEIGHT-BOX_HEIGHT/3,-BOX_LENGTH/2.0,
	    COS45,SIN45,0.0,
	BOX_WIDTH/2.0,HEIGHT-BOX_HEIGHT/3,BOX_LENGTH/2.0,
	    COS45,SIN45,0.0,

	0.0,HEIGHT,-BOX_LENGTH/2.0,
	    0.0,1.0,0.0,
	0.0,HEIGHT,BOX_LENGTH/2.0,
	    0.0,1.0,0.0,

	-BOX_WIDTH/2.0,HEIGHT-BOX_HEIGHT/3,-BOX_LENGTH/2.0,
	    -COS45,SIN45,0.0,
	-BOX_WIDTH/2.0,HEIGHT-BOX_HEIGHT/3,BOX_LENGTH/2.0,
	    -COS45,SIN45,0.0,

	-BOX_WIDTH/2.0,HEIGHT-BOX_HEIGHT,-BOX_LENGTH/2.0,
	    -1.0,0.0,0.0,
	-BOX_WIDTH/2.0,HEIGHT-BOX_HEIGHT,BOX_LENGTH/2.0,
	    -1.0,0.0,0.0,
    };
    static float front[] = {
	BOX_WIDTH/2.0,HEIGHT-BOX_HEIGHT,-BOX_LENGTH/2.0,
	BOX_WIDTH/2.0,HEIGHT-BOX_HEIGHT/3,-BOX_LENGTH/2.0,
	0.0,HEIGHT,-BOX_LENGTH/2.0,
	-BOX_WIDTH/2.0,HEIGHT-BOX_HEIGHT/3,-BOX_LENGTH/2.0,
	-BOX_WIDTH/2.0,HEIGHT-BOX_HEIGHT,-BOX_LENGTH/2.0,
    };
    static float back[] = {
	-BOX_WIDTH/2.0,HEIGHT-BOX_HEIGHT,BOX_LENGTH/2.0,
	-BOX_WIDTH/2.0,HEIGHT-BOX_HEIGHT/3,BOX_LENGTH/2.0,
	0.0,HEIGHT,BOX_LENGTH/2.0,
	BOX_WIDTH/2.0,HEIGHT-BOX_HEIGHT/3,BOX_LENGTH/2.0,
	BOX_WIDTH/2.0,HEIGHT-BOX_HEIGHT,BOX_LENGTH/2.0,
    };
    static float bottom[] = {
	BOX_WIDTH/2.0,HEIGHT-BOX_HEIGHT,-BOX_LENGTH/2.0,
	-BOX_WIDTH/2.0,HEIGHT-BOX_HEIGHT,-BOX_LENGTH/2.0,
	-BOX_WIDTH/2.0,HEIGHT-BOX_HEIGHT,BOX_LENGTH/2.0,
	BOX_WIDTH/2.0,HEIGHT-BOX_HEIGHT,BOX_LENGTH/2.0,
    };
    hwObject
	curr, oList[16];
    int
	nObjs = 0;


    curr = post(0.0,0.0,0.0,HEIGHT-BOX_HEIGHT,POST_RADIUS,FALSE,4);
    WOOD_HW(curr);
    oList[nObjs++] = curr;

    curr = hwPolygon->create( hwPolygon );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
    ALUMINUM_HW(curr);
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,NUMPTS(front)*3), front );
    oList[nObjs++] = curr;

    curr = hwPolygon->create( hwPolygon );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
    ALUMINUM_HW(curr);
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,NUMPTS(back)*3), back );
    oList[nObjs++] = curr;

    curr = hwPolygon->create( hwPolygon );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
    ALUMINUM_HW(curr);
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,NUMPTS(bottom)*3), bottom );
    oList[nObjs++] = curr;

    curr = hwMesh->create( hwMesh );
    HW_MODIFY_1I( curr, hwStrGraphN, 5 );
    HW_MODIFY_1I( curr, hwStrGraphM, 2 );
    HW_MODIFY_1B( curr, hwStrHasNormals, HW_TRUE );
    ALUMINUM_HW(curr);
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,5*2*6), mesh );
    oList[nObjs++] = curr;

    obj->display_list = createHwSegmentFromObj( oList, nObjs );
}


void init_mailbox_object(
    DRIVE_OBJECT *obj)
{
    obj->size[SIZE_HEIGHT] = HEIGHT;	
    if (debug)
	printf(" inside init_mailbox_%d_object() routine \n",(int) HEIGHT);

    obj->num_children = 0;

    if ((mailbox_dl_number == -1) || (last_nameset_bits != obj->nameset_bits)) {
    	create_mailbox_graphics(obj);
	mailbox_dl_number = obj->display_list;
	last_nameset_bits = obj->nameset_bits;
    }
    else {
	/* Good -- I have one like this already. */
	obj->display_list = mailbox_dl_number;
    }

    /* Initial (mc) bounding box values */
    obj->bound_mc[0] = -POST_RADIUS;
    obj->bound_mc[1] = 0.0;
    obj->bound_mc[2] = -POST_RADIUS;
    obj->bound_mc[3] = POST_RADIUS;
    obj->bound_mc[4] = HEIGHT;
    obj->bound_mc[5] = POST_RADIUS;

    /* Apply the object's xform matrix to the bounding box to put it in
     * world coordinates.
     */
    update_wc_bounds(obj);

    obj->surface_chars_xyz  = mailbox_surface_chars_xyz;
    obj->surface_chars_bbox = mailbox_surface_chars_bbox;

    elevate_object_to_terrain_height((SCENE *) obj->scene,obj,FALSE);
}
