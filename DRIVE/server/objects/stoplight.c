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


/* Code Module for the stoplight object */


#include <stdio.h>
#include <math.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"
#include "scene.h"

#define DEFAULT_HEIGHT	(25.0)
#define DEFAULT_LENGTH	SCENE_ROAD_WIDTH
#define DEFAULT_WIDTH	SCENE_ROAD_WIDTH

#define BOX_WIDTH	2.0
#define BOX_HEIGHT	5.0
#define HBW		(BOX_WIDTH/2.0 + 0.1)
#define H1		(Height - (BOX_HEIGHT*3.0/4.0))
#define H2		(Height - (BOX_HEIGHT/2.0))
#define H3		(Height - (BOX_HEIGHT/4.0))
#define LIGHT_RADIUS	(BOX_HEIGHT/10.0)
#define LIGHT_FACETS	8

#define POLE_MARGIN	5.0
#define POLE_SAG	5.0
#define POLE_HEIGHT	(Height+POLE_SAG)
#define POLE_RADIUS	0.5

#define GREEN_IN_X	0x01
#define YELLOW_IN_X	0x02
#define GREEN_IN_Z	0x04
#define YELLOW_IN_Z	0x08

#define ROTATION_RATE 	(45.0*M_PI/180.0)

#define Length			(obj->size[SIZE_LENGTH])
#define Width			(obj->size[SIZE_WIDTH])
#define Height			(obj->size[SIZE_HEIGHT])

typedef struct _stoplight_list {
    float length,width,height;
    int dl_number;
    struct _stoplight_list *next;
} STOPLIGHT_LIST;
static STOPLIGHT_LIST *stoplight_list=NULL;



static int pole_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    if (x*x + z*z < (POLE_RADIUS*POLE_RADIUS)) {
        sc->mc_y = POLE_HEIGHT;
        get_pole_mc_normal(obj,x,y,z,sc->mc_normal);
        return(TRUE);
    }
    /* else */
    return(FALSE);
}


static int pole_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = POLE_HEIGHT;
    return(TRUE);
}


static RETURN_CONDITION change_yourself(
    DRIVE_OBJECT *obj,
    float t_interval)
{
    typedef struct {
	int name_filter;
	float time_in_state;
    } TIMING;
    static TIMING timing[] = {
	{ GREEN_IN_Z,	20.0 },
	{ YELLOW_IN_Z,	3.0  },
	{ GREEN_IN_X,	20.0 },
	{ YELLOW_IN_X,	3.0  },
    };
    int which;

    if ((obj->spacing -= t_interval) > 0.0) return(RETURN_OK);

    /* Time has run out -- switch to the next state. */
    switch (obj->upd->invis[0]) {
	case GREEN_IN_Z:
	    which = 0;
	    break;
	case YELLOW_IN_Z:
	    which = 1;
	    break;
	case GREEN_IN_X:
	    which = 2;
	    break;
	case YELLOW_IN_X:
	default:
	    which = 3;
	    break;
    }

    which = (which + 1) % 4;
    obj->spacing = timing[which].time_in_state;
    obj->upd->invis[0] = timing[which].name_filter;

    return(RETURN_OK);
}


static void create_stoplight_graphics(
    DRIVE_OBJECT *obj)
{
    static float box[] = {
	-BOX_WIDTH/2.0, 0.0, -BOX_WIDTH/2.0,
	 BOX_WIDTH/2.0, 1.0,  BOX_WIDTH/2.0,
    };
    float lightPos[12][3] = {
	{ HBW,   0,  0.0,},
	{-HBW,   0,  0.0,},
	{ 0.0,   0,  HBW,},
	{ 0.0,   0, -HBW,},
	{ HBW,   0,  0.0,},
	{-HBW,   0,  0.0,},
	{ 0.0,   0,  HBW,},
	{ 0.0,   0, -HBW,},
	{ HBW,   0,  0.0,},
	{-HBW,   0,  0.0,},
	{ 0.0,   0,  HBW,},
	{ 0.0,   0, -HBW,},
    };
    static float lightColor[12][3] = {
	{ 0.0, 1.0, 0.0 },
	{ 0.0, 1.0, 0.0 },
	{ 0.0, 1.0, 0.0 },
	{ 0.0, 1.0, 0.0 },
	{ 1.0, 1.0, 0.0 },
	{ 1.0, 1.0, 0.0 },
	{ 1.0, 1.0, 0.0 },
	{ 1.0, 1.0, 0.0 },
	{ 1.0, 0.0, 0.0 },
	{ 1.0, 0.0, 0.0 },
	{ 1.0, 0.0, 0.0 },
	{ 1.0, 0.0, 0.0 },
    };
    static float lightRot[12][3] = {
	{   0.0,  90.0,   0.0 },
	{   0.0, 270.0,   0.0 },
	{   0.0, 180.0,   0.0 },
	{   0.0,   0.0,   0.0 },
	{   0.0,  90.0,   0.0 },
	{   0.0, 270.0,   0.0 },
	{   0.0, 180.0,   0.0 },
	{   0.0,   0.0,   0.0 },
	{   0.0,  90.0,   0.0 },
	{   0.0, 270.0,   0.0 },
	{   0.0, 180.0,   0.0 },
	{   0.0,   0.0,   0.0 },
    };
    static hwInt32 lightVisibility[12][5] = {
	{ 1, GREEN_IN_X, GREEN_IN_Z, YELLOW_IN_Z, YELLOW_IN_X },
	{ 1, GREEN_IN_X, GREEN_IN_Z, YELLOW_IN_Z, YELLOW_IN_X },
	{ 1, GREEN_IN_Z, GREEN_IN_X, YELLOW_IN_Z, YELLOW_IN_X },
	{ 1, GREEN_IN_Z, GREEN_IN_X, YELLOW_IN_Z, YELLOW_IN_X },
	{ 1, YELLOW_IN_X, GREEN_IN_Z, YELLOW_IN_Z, GREEN_IN_X },
	{ 1, YELLOW_IN_X, GREEN_IN_Z, YELLOW_IN_Z, GREEN_IN_X },
	{ 1, YELLOW_IN_Z, GREEN_IN_Z, YELLOW_IN_X, GREEN_IN_X },
	{ 1, YELLOW_IN_Z, GREEN_IN_Z, YELLOW_IN_X, GREEN_IN_X },
	{ 2, GREEN_IN_Z, YELLOW_IN_Z, GREEN_IN_X, YELLOW_IN_X },
	{ 2, GREEN_IN_Z, YELLOW_IN_Z, GREEN_IN_X, YELLOW_IN_X },
	{ 2, GREEN_IN_X, YELLOW_IN_X, GREEN_IN_Z, YELLOW_IN_Z },
	{ 2, GREEN_IN_X, YELLOW_IN_X, GREEN_IN_Z, YELLOW_IN_Z },
    };
    int i, j;
    float lines[3][3];
    hwObject curr, oList[64];
    int nObjs = 0;

    lightPos[ 0][1] = H1; lightPos[ 1][1] = H1;
    lightPos[ 2][1] = H1; lightPos[ 3][1] = H1;
    lightPos[ 4][1] = H2; lightPos[ 5][1] = H2;
    lightPos[ 6][1] = H2; lightPos[ 7][1] = H2;
    lightPos[ 8][1] = H3; lightPos[ 9][1] = H3;
    lightPos[10][1] = H3; lightPos[11][1] = H3;

    box[1] = Height - BOX_HEIGHT;
    box[4] = Height;

	/* The poles */
    curr = hwCone->create( hwCone );
    STEEL_HW( curr );
    HW_MODIFY_1F( curr, hwStrHeight, POLE_HEIGHT );
    HW_MODIFY_1F( curr, hwStrRadius, POLE_RADIUS );
    HW_MODIFY_3F( curr, hwStrRotate, 90.0, 0.0, 0.0 );
    HW_MODIFY_3F( curr, hwStrPos,
		-Width/2.0-POLE_MARGIN,0.0,-Width/2.0-POLE_MARGIN );
    HW_MODIFY_1I( curr, hwStrGraphN, 2 );
    HW_MODIFY_1I( curr, hwStrGraphM, 5 );
    oList[nObjs++] = curr;

    curr = hwCone->create( hwCone );
    STEEL_HW( curr );
    HW_MODIFY_1F( curr, hwStrHeight, POLE_HEIGHT );
    HW_MODIFY_1F( curr, hwStrRadius, POLE_RADIUS );
    HW_MODIFY_3F( curr, hwStrRotate, 90.0, 0.0, 0.0 );
    HW_MODIFY_3F( curr, hwStrPos,
		Width/2.0+POLE_MARGIN,0.0,Width/2.0+POLE_MARGIN );
    HW_MODIFY_1I( curr, hwStrGraphN, 2 );
    HW_MODIFY_1I( curr, hwStrGraphM, 5 );
    oList[nObjs++] = curr;

    /* The lines */
    lines[0][0] = -Width/2.0-POLE_MARGIN;
    lines[0][1] = POLE_HEIGHT;
    lines[0][2] = -Width/2.0-POLE_MARGIN;
    lines[1][0] = 0.0;
    lines[1][1] = Height;
    lines[1][2] = 0.0;
    lines[2][0] = Width/2.0+POLE_MARGIN;
    lines[2][1] = POLE_HEIGHT;
    lines[2][2] = Width/2.0+POLE_MARGIN;

    curr = hwPolyline->create( hwPolyline );
    HW_MODIFY_3F( curr, hwStrColor, 0.5, 0.5, 0.5 );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,3*3), lines );
    oList[nObjs++] = curr;

    /* The lights */
    for( i = 0; i < 12; i++ ) {
	for( j = 0; j < 4; j++ ) {
	    curr = hwDisc->create( hwDisc );
	    HW_MODIFY_1I( curr, hwStrVisibility, lightVisibility[i][j+1] );
	    HW_MODIFY_3F( curr, hwStrRotate,
			lightRot[i][0],
			lightRot[i][1],
			lightRot[i][2] );
	    HW_MODIFY_3F( curr, hwStrPos,
			lightPos[i][0],
			lightPos[i][1],
			lightPos[i][2] );
	    HW_MODIFY_1I( curr, hwStrGraphN, LIGHT_FACETS );
	    HW_MODIFY_1F( curr, hwStrRadius, LIGHT_RADIUS );
	    if( j < lightVisibility[i][0] ) {
		HW_MODIFY_1B( curr, hwStrBright, HW_TRUE );
		HW_MODIFY_3F( curr, hwStrColor,
			lightColor[i][0],
			lightColor[i][1],
			lightColor[i][2] );
	    }
	    else {
		HW_MODIFY_3F( curr, hwStrColor, 0.15, 0.15, 0.15 );
	    }
	    oList[nObjs++] = curr;
	}
    }

    curr = hwBox->create( hwBox );
    HW_MODIFY_3F( curr, hwStrColor, 0.7, 0.7, 0.0 );
    curr->modify( curr, hwStrData, HW_MAKE_TYPE(HW_TYPE_FLOAT,6), box );
    oList[nObjs++] = curr;

    /* TBD: LOD */
    curr = hwGroup->create( hwGroup );
    curr->modify( curr, hwStrChildren,
		HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );

    obj->display_list = createHwSegmentFromObj( &curr, 1 );
}


void init_stoplight_object(
    DRIVE_OBJECT *obj)
{
    STOPLIGHT_LIST *rl;
    static float mat[4][4] = IDENTITY4x4;
    DRIVE_OBJECT *child;


    if (Height == DEFAULT_OBJECT_SIZE) Height = DEFAULT_HEIGHT;
    if (Length == DEFAULT_OBJECT_SIZE) Length = DEFAULT_LENGTH;
    if (Width  == DEFAULT_OBJECT_SIZE) Width  = DEFAULT_WIDTH;

    if(debug) printf(" inside init_stoplight_object() routine \n");

    /* Two poles */
    obj->num_children = 2;

    /* See if we've created one like this before... */
    rl = stoplight_list;
    while (rl != NULL) {
	if (IS_NEAR(rl->height,Height)
		&& IS_NEAR(rl->length,Length)
		&& IS_NEAR(rl->width,Width)) {
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
	if ((rl = (STOPLIGHT_LIST *) malloc(sizeof(STOPLIGHT_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}
	rl->length = Length;
	rl->width  = Width;
	rl->height = Height;
	rl->next   = stoplight_list;
    	create_stoplight_graphics(obj);
	rl->dl_number = obj->display_list;
	stoplight_list  = rl;
    }

    if ((obj->upd = (UpdateDisp_ptr) malloc(sizeof(UpdateDisp))) == NULL) {
	fprintf(stderr,"Out of malloc space!\n");
	return;
    }
    obj->upd->invis_words = 1;
    obj->upd->invis[0] = GREEN_IN_X;
    obj->upd->color[0] = obj->upd->color[1] = obj->upd->color[2] = 0.0;
    obj->spacing = 0.0;

    /* Initial (mc) bounding box values */
    obj->bound_mc[0] = -Width/2.0 - (POLE_MARGIN + POLE_RADIUS);
    obj->bound_mc[1] = 0.0;
    obj->bound_mc[2] = -Length/2.0 - (POLE_MARGIN + POLE_RADIUS);
    obj->bound_mc[3] = Width/2.0 + (POLE_MARGIN + POLE_RADIUS);
    obj->bound_mc[4] = Height + POLE_SAG;
    obj->bound_mc[5] = Length/2.0 + (POLE_MARGIN + POLE_RADIUS);
    update_wc_bounds(obj);

    obj->update_self = change_yourself;

    /* Now create subobjects for each pole */
    if ((child = (DRIVE_OBJECT *) malloc(sizeof(DRIVE_OBJECT))) == NULL) {
	fprintf(stderr,"Out of malloc space!\n");
	return;
    }
    /* First clone myself */
    memcpy(child,obj,sizeof(DRIVE_OBJECT));
    /* Change relevant parts */
    child->idptr = &(object_id[12]);
    child->num_children = 0;
    child->child_list = NULL;
    child->display_list = INVALID;
    child->upd = NULL;
    mat[3][0] = -Width/2.0 - POLE_MARGIN;
    mat[3][2] = -Length/2.0 - POLE_MARGIN;
    concat_matrix(mat,obj->xform,child->xform);
    _hp_invert(child->xform,child->ixform,0);
    child->bound_mc[0] = -POLE_RADIUS;
    child->bound_mc[1] = 0.0;
    child->bound_mc[2] = -POLE_RADIUS;
    child->bound_mc[3] = POLE_RADIUS;
    child->bound_mc[4] = Height + POLE_SAG;
    child->bound_mc[5] = POLE_RADIUS;
    child->surface_chars_xyz  = pole_surface_chars_xyz;
    child->surface_chars_bbox = pole_surface_chars_bbox;
    update_wc_bounds(child);
    add_object_to_list(&(obj->child_list),child);

    if ((child = (DRIVE_OBJECT *) malloc(sizeof(DRIVE_OBJECT))) == NULL) {
	fprintf(stderr,"Out of malloc space!\n");
	return;
    }
    /* First clone myself */
    memcpy(child,obj,sizeof(DRIVE_OBJECT));
    /* Change relevant parts */
    child->idptr = &(object_id[12]);
    child->num_children = 0;
    child->child_list = NULL;
    child->display_list = INVALID;
    child->upd = NULL;
    mat[3][0] = Width/2.0 + POLE_MARGIN;
    mat[3][2] = Length/2.0 + POLE_MARGIN;
    concat_matrix(mat,obj->xform,child->xform);
    _hp_invert(child->xform,child->ixform,0);
    child->bound_mc[0] = -POLE_RADIUS;
    child->bound_mc[1] = 0.0;
    child->bound_mc[2] = -POLE_RADIUS;
    child->bound_mc[3] = POLE_RADIUS;
    child->bound_mc[4] = Height + POLE_SAG;
    child->bound_mc[5] = POLE_RADIUS;
    child->surface_chars_xyz  = pole_surface_chars_xyz;
    child->surface_chars_bbox = pole_surface_chars_bbox;
    update_wc_bounds(child);
    add_object_to_list(&(obj->child_list),child);
}
