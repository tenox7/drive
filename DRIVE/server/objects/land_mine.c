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


/* Code Module for the landmine object */


#include <stdio.h>
#include <math.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"
#include "scene.h"

#define DEFAULT_HEIGHT	(0.25)
#define DEFAULT_LENGTH	(5.0)
#define DEFAULT_WIDTH	(5.0)
#define DEFAULT_SPACING (10.0)

#define Length			(obj->size[SIZE_LENGTH])
#define Width			(obj->size[SIZE_WIDTH])
#define Height			(obj->size[SIZE_HEIGHT])
#define Spacing			(obj->spacing)
#define Radius			(obj->radius)
#define Type			(obj->type)
#define TIME_LEFT_TO_ARM(obj)	((obj)->angle)

#define MINE_TYPE_PERMANENT 1
#define MINE_TYPE_TEMPORARY 2

typedef struct {
    int mine_type;
} mine_data;

typedef struct _shell_list {
    float radius,height;
    unsigned int nameset_bits;
    int dl_number;
    struct _shell_list *next;
} SHELL_LIST;
static SHELL_LIST *shell_list=NULL;


static int land_mine_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = Height + (MAX_OBSTACLE_CLEARANCE+1.0); /* ensure a collision */
    sc->mc_normal[0] = 0.0;
    sc->mc_normal[1] = 1.0;
    sc->mc_normal[2] = 0.0;
    return(TRUE);
}


static int land_mine_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = Height + (MAX_OBSTACLE_CLEARANCE+1.0); /* ensure a collision */
    return(TRUE);
}


static RETURN_CONDITION rearm_countdown(
    DRIVE_OBJECT *obj,
    float t_interval)
{
    mine_data *md = (mine_data *) (obj->additional_data);
    RETURN_CONDITION ret = RETURN_OK;

    TIME_LEFT_TO_ARM(obj) -= t_interval;
    if (md->mine_type == MINE_TYPE_TEMPORARY) {
	if (TIME_LEFT_TO_ARM(obj) <= 0.0) ret = RETURN_DELETE_ME;
    }

    return(ret);
}


static void possible_explosion(
    DRIVE_OBJECT *hitting_obj, DRIVE_OBJECT *hit_obj,
    float t_interval)
{
    mine_data *md = (mine_data *) (hit_obj->additional_data);

    if (md->mine_type == MINE_TYPE_TEMPORARY) {
	cause_explosion(hit_obj->scene,
	    hit_obj->xform[3][0], hit_obj->xform[3][1], hit_obj->xform[3][2],
	    hit_obj->radius+8.0,EXPLOSION_BASE_FORCE,EXPLOSION_BASE_TORQUE);

	if (md != NULL) free(md);
	delete_object_from_scene(hit_obj->scene,hit_obj);
	/* delete_object(hit_obj); */
    }
    else if ( TIME_LEFT_TO_ARM(hit_obj) <= 0.0) {
	cause_explosion(hit_obj->scene,
	    hit_obj->xform[3][0], hit_obj->xform[3][1], hit_obj->xform[3][2],
	    hit_obj->radius+8.0,EXPLOSION_BASE_FORCE,EXPLOSION_BASE_TORQUE);
	TIME_LEFT_TO_ARM(hit_obj) = hit_obj->spacing;
    }
}

static int create_mine_graphics(
    DRIVE_OBJECT *obj)
{
    float radius;
    hwObject curr, oList[10];
    int nObjs = 0;

    if( Width > Length) radius = Width/2.0; else radius = Length/2.0;

    curr = hwCone->create( hwCone );
    HW_MODIFY_1F( curr, hwStrRadius, radius );
    HW_MODIFY_1F( curr, hwStrHeight, Height );
    HW_MODIFY_1I( curr, hwStrVisibility, HALF_SECOND_NAMESET_EVEN );
    HW_MODIFY_3F( curr, hwStrColor, 1.0, 0.0, 0.0 );
    HW_MODIFY_1B( curr, hwStrBright, HW_TRUE );
    HW_MODIFY_3F( curr, hwStrRotate, 90.0, 0.0, 0.0 );
    HW_MODIFY_1I( curr, hwStrGraphN, 2 );
    HW_MODIFY_1I( curr, hwStrGraphM, 11 );
    oList[nObjs++] = curr;

    curr = hwDisc->create( hwDisc );
    HW_MODIFY_1F( curr, hwStrRadius, radius );
    HW_MODIFY_1I( curr, hwStrVisibility, HALF_SECOND_NAMESET_EVEN );
    HW_MODIFY_3F( curr, hwStrColor, 1.0, 0.0, 0.0 );
    HW_MODIFY_1B( curr, hwStrBright, HW_TRUE );
    HW_MODIFY_3F( curr, hwStrRotate, 90.0, 0.0, 0.0 );
    HW_MODIFY_1I( curr, hwStrGraphN, 11 );
    oList[nObjs++] = curr;

    curr = hwDisc->create( hwDisc );
    HW_MODIFY_1F( curr, hwStrRadius, radius );
    HW_MODIFY_1I( curr, hwStrVisibility, HALF_SECOND_NAMESET_EVEN );
    HW_MODIFY_3F( curr, hwStrColor, 1.0, 0.0, 0.0 );
    HW_MODIFY_1B( curr, hwStrBright, HW_TRUE );
    HW_MODIFY_3F( curr, hwStrRotate, 90.0, 0.0, 0.0 );
    HW_MODIFY_1I( curr, hwStrGraphN, 11 );
    HW_MODIFY_3F( curr, hwStrPos, 0.0, Height, 0.0 );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    oList[nObjs++] = curr;

    curr = hwCone->create( hwCone );
    HW_MODIFY_1F( curr, hwStrRadius, radius );
    HW_MODIFY_1F( curr, hwStrHeight, Height );
    HW_MODIFY_1I( curr, hwStrVisibility, HALF_SECOND_NAMESET_ODD );
    HW_MODIFY_3F( curr, hwStrColor, 0.5, 0.0, 0.0 );
    HW_MODIFY_3F( curr, hwStrRotate, 90.0, 0.0, 0.0 );
    HW_MODIFY_1I( curr, hwStrGraphN, 2 );
    HW_MODIFY_1I( curr, hwStrGraphM, 11 );
    oList[nObjs++] = curr;

    curr = hwDisc->create( hwDisc );
    HW_MODIFY_1F( curr, hwStrRadius, radius );
    HW_MODIFY_1I( curr, hwStrVisibility, HALF_SECOND_NAMESET_ODD );
    HW_MODIFY_3F( curr, hwStrColor, 0.5, 0.0, 0.0 );
    HW_MODIFY_3F( curr, hwStrRotate, 90.0, 0.0, 0.0 );
    HW_MODIFY_1I( curr, hwStrGraphN, 11 );
    oList[nObjs++] = curr;

    curr = hwDisc->create( hwDisc );
    HW_MODIFY_1F( curr, hwStrRadius, radius );
    HW_MODIFY_1I( curr, hwStrVisibility, HALF_SECOND_NAMESET_ODD );
    HW_MODIFY_3F( curr, hwStrColor, 0.5, 0.0, 0.0 );
    HW_MODIFY_3F( curr, hwStrRotate, 90.0, 0.0, 0.0 );
    HW_MODIFY_1I( curr, hwStrGraphN, 11 );
    HW_MODIFY_3F( curr, hwStrPos, 0.0, Height, 0.0 );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    oList[nObjs++] = curr;

    return createHwSegmentFromObj( oList, nObjs );
}

void init_land_mine_object(
    DRIVE_OBJECT *obj)
{
    SHELL_LIST *rl;
    mine_data *md;

    if (Height  == DEFAULT_OBJECT_SIZE) Height  = DEFAULT_HEIGHT;
    if (Length  == DEFAULT_OBJECT_SIZE) Length  = DEFAULT_LENGTH;
    if (Width   == DEFAULT_OBJECT_SIZE) Width   = DEFAULT_WIDTH;
    if (Spacing == DEFAULT_OBJECT_SIZE) Spacing = DEFAULT_SPACING;

    obj->global_display_list = obj->display_list = INVALID;

    obj->additional_data = (mine_data *)malloc( sizeof(mine_data));
    md = (mine_data *)(obj->additional_data);
    if( strcmp(obj->subtype,"One Shot") == 0)
    {
	md->mine_type=MINE_TYPE_TEMPORARY;
	obj->idptr->flags |= OBJECTCLASS_DYNAMIC;
    }
    else
	md->mine_type=MINE_TYPE_PERMANENT;

    obj->radius = HYPOT2(Width/2.0,Length/2.0) / SIN45;


    if(md->mine_type == MINE_TYPE_TEMPORARY)
    {
	/* See if we've created one like this before... */
	rl = shell_list;
	while (rl != NULL) {
	    if (IS_NEAR(rl->radius,Radius)
		    && IS_NEAR(rl->height,Height) ) {
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
	    rl->height = Height;
	    rl->next   = shell_list;
	    rl->dl_number = obj->display_list = create_mine_graphics(obj);
	    shell_list  = rl;
	}
    }


    /* Initial (mc) bounding box values */
    obj->bound_mc[0] = -Width/2.0;
    obj->bound_mc[1] = 0.0;
    obj->bound_mc[2] = -Length/2.0;
    obj->bound_mc[3] = Width/2.0;
    obj->bound_mc[4] = Height;
    obj->bound_mc[5] = Length/2.0;
    update_wc_bounds(obj);


    obj->surface_chars_xyz  = land_mine_surface_chars_xyz;
    obj->surface_chars_bbox = land_mine_surface_chars_bbox;
    obj->collision_routine  = possible_explosion;
    obj->update_self        = rearm_countdown;
    if(md->mine_type == MINE_TYPE_TEMPORARY)
	TIME_LEFT_TO_ARM(obj)   = Spacing;
    else
	TIME_LEFT_TO_ARM(obj)   = 0.0;
}
