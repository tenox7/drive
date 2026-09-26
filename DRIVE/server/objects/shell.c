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


/* Code Module for the shell object */


#include <stdio.h>
#include <math.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"
#include "scene.h"
#include "demo_physics.h"

#define DEFAULT_LENGTH	(1.0)
#define DEFAULT_RADIUS	(.15)

#define Length			(obj->size[SIZE_LENGTH])
#define Radius			(obj->radius)

typedef struct _shell_list {
    float radius,length;
    unsigned int nameset_bits;
    int dl_number;
    struct _shell_list *next;
} SHELL_LIST;
static SHELL_LIST *shell_list=NULL;



static int shell_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    return(TRUE);
}

static int shell_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    return(TRUE);
}


static int create_shell_graphics(
    DRIVE_OBJECT *obj)
{
    float outline[6][2];
    hwObject curr;

    outline[0][0] = 0.0;
    outline[0][1] = 0.0;

    outline[1][0] = Radius*.95;
    outline[1][1] = 0.0;

    outline[2][0] = Radius;
    outline[2][1] = Length*0.05;

    outline[3][0] = Radius;
    outline[3][1] = Length*0.5;

    outline[4][0] = Radius*(2.0/3.0);
    outline[4][1] = Length*(10.0/12.0);

    outline[5][0] = 0.0;
    outline[5][1] = Length;

    curr = hwSurfRev->create( hwSurfRev );
    STEEL_HW(curr);
    HW_MODIFY_1I( curr, hwStrGraphN, 7 );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,6*2), outline );
    HW_OBJECT_NAMESET(curr,obj);

    return createHwSegmentFromObj( &curr, 1 );
}


static VEHICLE_AUXDATA *init_vaux(
    DRIVE_OBJECT *obj)
{
    static VEHICLE_AUXDATA *vaux = NULL;

    if (vaux != NULL) {
	return(vaux);
    }

    /***** INIT AUXILIARY DATA *****/
    if ((vaux = (VEHICLE_AUXDATA *) malloc(sizeof(VEHICLE_AUXDATA))) == NULL) {
	return(NULL);
    }

    /***** INIT AUXILIARY DATA *****/
    vaux->bbox_mc[PT_UFL][0] = -Radius;
	    vaux->bbox_mc[PT_UFL][1] = -Radius;
	    vaux->bbox_mc[PT_UFL][2] = Length;
    vaux->bbox_mc[PT_UFR][0] =  Radius;
	    vaux->bbox_mc[PT_UFR][1] = -Radius;
	    vaux->bbox_mc[PT_UFR][2] = Length;
    vaux->bbox_mc[PT_UBR][0] =  Radius;
	    vaux->bbox_mc[PT_UBR][1] = -Radius;
	    vaux->bbox_mc[PT_UBR][2] = 0.0;
    vaux->bbox_mc[PT_UBL][0] = -Radius;
	    vaux->bbox_mc[PT_UBL][1] = -Radius;
	    vaux->bbox_mc[PT_UBL][2] = 0.0;
    vaux->bbox_mc[PT_TFL][0] = -Radius;
	    vaux->bbox_mc[PT_TFL][1] = Radius;
	    vaux->bbox_mc[PT_TFL][2] =  Length;
    vaux->bbox_mc[PT_TFR][0] =  Radius;
	    vaux->bbox_mc[PT_TFR][1] = Radius;
	    vaux->bbox_mc[PT_TFR][2] = Length;
    vaux->bbox_mc[PT_TBR][0] =  Radius;
	    vaux->bbox_mc[PT_TBR][1] = Radius;
	    vaux->bbox_mc[PT_TBR][2] = 0.0;
    vaux->bbox_mc[PT_TBL][0] = -Radius;
	    vaux->bbox_mc[PT_TBL][1] = Radius;
	    vaux->bbox_mc[PT_TBL][2] = 0.0;
    vaux->COG_mc[0] =
	    (vaux->bbox_mc[PT_UFR][0] + vaux->bbox_mc[PT_UFL][0]) / 2.0;
    vaux->COG_mc[1] =
	    (vaux->bbox_mc[PT_TFL][1] + vaux->bbox_mc[PT_UFL][1]) / 2.0;
    vaux->COG_mc[2] =
	    (vaux->bbox_mc[PT_UBR][2] + vaux->bbox_mc[PT_UFR][2]) / 2.0;


    vaux->coefficient_of_drag = 0.0;
    vaux->max_obstacle_height = 1e-6;
    vaux->thrust_mechanism    = THRUSTER;

    return(vaux);
}


void init_shell_object(
    DRIVE_OBJECT *obj)
{
    SHELL_LIST *rl;

    if (Length == 0.0) Length = DEFAULT_LENGTH;
    if (Radius == 0.0) Radius = DEFAULT_RADIUS;

    /* See if we've created one like this before... */
    rl = shell_list;
    while (rl != NULL) {
	if (IS_NEAR(rl->radius,Radius)
		&& IS_NEAR(rl->length,Length)
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
	if ((rl = (SHELL_LIST *) malloc(sizeof(SHELL_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}
	rl->radius = Radius;
	rl->length = Length;
	rl->nameset_bits = obj->nameset_bits;
	rl->next   = shell_list;
	rl->dl_number = obj->display_list = create_shell_graphics(obj);
	shell_list  = rl;
    }

    /***** INIT AUXILIARY DATA *****/
    if ((obj->vehicle_auxdata = init_vaux(obj)) == NULL) {
	return;
    }
    init_pobj(obj,20.0,70.0);

    obj->surface_chars_xyz  = shell_surface_chars_xyz;
    obj->surface_chars_bbox = shell_surface_chars_bbox;

    /* Initial (mc) bounding box values */
    obj->bound_mc[0] = -Radius;
    obj->bound_mc[1] = -Radius;
    obj->bound_mc[2] = 0.0;
    obj->bound_mc[3] = Radius;
    obj->bound_mc[4] = Radius;
    obj->bound_mc[5] = Length;

    obj->update_self = apply_ballistic_physics;

    /* Apply the object's xform matrix to the bounding box to put it in
     * world coordinates.
     */
    update_wc_bounds(obj);

    obj->num_children = 0;
}
