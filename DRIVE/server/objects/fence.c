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


/* Code Module for various fences */

#include <stdio.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "prims.h"
#include "obj_common.h"
#include "demo_physics.h"


#define Length		(obj->size[SIZE_LENGTH])
#define Height		(obj->size[SIZE_HEIGHT])
#define Spacing		(obj->spacing)

#define DEFAULT_HEIGHT	(4.0)
#define DEFAULT_LENGTH	(100.0)

#define TYPE_SPLIT_RAIL		0
#define TYPE_BARBED_WIRE	1
#define TYPE_CHAIN_LINK		2
#define TYPE_GUARDRAIL		3
#define TYPE_PICKET		4

#define SPLIT_RAIL_WIDTH		0.3
#define SPLIT_RAIL1_TOP			3.0
#define SPLIT_RAIL1_BOTTOM		(SPLIT_RAIL1_TOP-SPLIT_RAIL_WIDTH)
#define SPLIT_RAIL2_TOP			1.0
#define SPLIT_RAIL2_BOTTOM		(SPLIT_RAIL2_TOP-SPLIT_RAIL_WIDTH)
#define SPLIT_RAIL_POST_WIDTH		SPLIT_RAIL_WIDTH
#define SPLIT_RAIL_POST_HEIGHT		(SPLIT_RAIL1_TOP+0.5)
#define SPLIT_RAIL_POST_SPACING		10.0

#define BARBED_WIRE_POST_HEIGHT		3.5
#define BARBED_WIRE_Y1			(BARBED_WIRE_POST_HEIGHT-0.2)
#define BARBED_WIRE_Y2			1.5
#define BARBED_WIRE_POST_WIDTH		0.15
#define BARBED_WIRE_POST_SPACING	20.0

#define CHAIN_LINK_POST_RADIUS		0.15
#define CHAIN_LINK_MESH_SPACING		0.5
#define CHAIN_LINK_POST_SPACING		20.0
#define CHAIN_LINK_BUFFER		64

#define GWIDTH			0.5
#define GUARDRAIL_TOP			3.0
#define GUARDRAIL_BOTTOM		2.0
#define GUARDRAIL_MID1 \
    (GUARDRAIL_BOTTOM+(GUARDRAIL_TOP-GUARDRAIL_BOTTOM)/4.0)
#define GUARDRAIL_MID2 \
    (GUARDRAIL_BOTTOM+(GUARDRAIL_TOP-GUARDRAIL_BOTTOM)/2.0)
#define GUARDRAIL_MID3 \
    (GUARDRAIL_BOTTOM+(GUARDRAIL_TOP-GUARDRAIL_BOTTOM)*3.0/4.0)
#define GPOST_WIDTH		0.5
#define GPOST_HEIGHT		(GUARDRAIL_TOP+0.5)
#define GUARDRAIL_POST_SPACING		25.0


typedef struct _fence_list {
    float length;
    int type;
    float color[3];
    float spacing;
    unsigned int nameset_bits;
    int dl_number;
    struct _fence_list *next;
} FENCE_LIST;
static FENCE_LIST *fence_list = NULL;


static int fence_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = obj->size[SIZE_HEIGHT];
    get_box_mc_normal(obj,x,y,z,sc->mc_normal);
    return(TRUE);
}


static int fence_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = obj->size[SIZE_HEIGHT];
    return(TRUE);
}


static void create_split_rail_graphics(
    DRIVE_OBJECT *obj)
{
    static float rail1[2][4][3] = {
	{{ 0.0,				SPLIT_RAIL1_BOTTOM,	0.0 },
	 { -SPLIT_RAIL_WIDTH/2.0,	SPLIT_RAIL1_TOP,	0.0 },
	 { SPLIT_RAIL_WIDTH/2.0,	SPLIT_RAIL1_TOP,	0.0 },
	 { 0.0,				SPLIT_RAIL1_BOTTOM,	0.0 }},

	{{ 0.0,				SPLIT_RAIL1_BOTTOM,	1.0 },
	 { -SPLIT_RAIL_WIDTH/2.0,	SPLIT_RAIL1_TOP,	1.0 },
	 { SPLIT_RAIL_WIDTH/2.0,	SPLIT_RAIL1_TOP,	1.0 },
	 { 0.0,				SPLIT_RAIL1_BOTTOM,	1.0 }}
    };
    static float rail2[2][4][3] = {
	{{ 0.0,				SPLIT_RAIL2_BOTTOM,	0.0 },
	 { -SPLIT_RAIL_WIDTH/2.0,	SPLIT_RAIL2_TOP,	0.0 },
	 { SPLIT_RAIL_WIDTH/2.0,	SPLIT_RAIL2_TOP,	0.0 },
	 { 0.0,				SPLIT_RAIL2_BOTTOM,	0.0 }},

	{{ 0.0,				SPLIT_RAIL2_BOTTOM,	1.0 },
	 { -SPLIT_RAIL_WIDTH/2.0,	SPLIT_RAIL2_TOP,	1.0 },
	 { SPLIT_RAIL_WIDTH/2.0,	SPLIT_RAIL2_TOP,	1.0 },
	 { 0.0,				SPLIT_RAIL2_BOTTOM,	1.0 }}
    };
    int i;
    float z;
    hwObject
	curr, oList[100];
    int
	nObjs = 0;

    for (i=0; i<4; ++i) {
	rail1[1][i][2] = Length;
	rail2[1][i][2] = Length;
    }

    curr = hwMesh->create( hwMesh );
    HW_MODIFY_1I( curr, hwStrGraphN, 2 );
    HW_MODIFY_1I( curr, hwStrGraphM, 4 );
    curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,2*4*3), rail1 );
    oList[nObjs++] = curr;

    curr = hwMesh->create( hwMesh );
    HW_MODIFY_1I( curr, hwStrGraphN, 2 );
    HW_MODIFY_1I( curr, hwStrGraphM, 4 );
    curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,2*4*3), rail2 );
    oList[nObjs++] = curr;

    for (z=0.0; z<Length; z+=Spacing) {
	oList[nObjs++] = curr = post(0.0,0.0,z,SPLIT_RAIL_POST_HEIGHT,
	    SPLIT_RAIL_POST_WIDTH,TRUE,3);
	HW_MODIFY_3F( curr, hwStrColor,
		obj->color[0],obj->color[1],obj->color[2]);
    }
    if (!IS_NEAR(z-Spacing,Length)) {
	/* need a post at the end */
	oList[nObjs++] = curr = post(0.0,0.0,Length,SPLIT_RAIL_POST_HEIGHT,
	    SPLIT_RAIL_POST_WIDTH,TRUE,3);
	HW_MODIFY_3F( curr, hwStrColor,
		obj->color[0],obj->color[1],obj->color[2]);
    }

    curr = hwGroup->create( hwGroup );
    HW_OBJECT_NAMESET(curr,obj);
    HW_MODIFY_3F(curr,hwStrColor,obj->color[0],obj->color[1],obj->color[2]);
    curr->modify( curr, hwStrChildren,
		HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
    obj->display_list = createHwSegmentFromObj( &curr, 1 );
}


static void create_barbed_wire_graphics(
    DRIVE_OBJECT *obj)
{
    float z, poly[4*4];
    hwObject
	curr, oList[100];
    int
	nObjs = 0;

    poly[ 0]=0.0; poly[ 1]=BARBED_WIRE_Y1; poly[ 2]=0.0;    poly[ 3]=0.0;
    poly[ 4]=0.0; poly[ 5]=BARBED_WIRE_Y1; poly[ 6]=Length; poly[ 7]=1.0;
    poly[ 8]=0.0; poly[ 9]=BARBED_WIRE_Y2; poly[10]=0.0;    poly[11]=0.0;
    poly[12]=0.0; poly[13]=BARBED_WIRE_Y2; poly[14]=Length; poly[15]=1.0;
    curr = hwPolyline->create( hwPolyline );
    HW_MODIFY_1B( curr, hwStrHasFlags, HW_TRUE );
    HW_MODIFY_3F( curr, hwStrColor, STEEL_RED,STEEL_GREEN,STEEL_BLUE );
    curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,4*4), poly );
    oList[nObjs++] = curr;

    for (z=0.0; z<Length; z+=Spacing) {
	oList[nObjs++] = curr = post(0.0,0.0,z,BARBED_WIRE_POST_HEIGHT,
	    BARBED_WIRE_POST_WIDTH,FALSE,3);
	HW_MODIFY_3F( curr, hwStrColor,
		obj->color[0],obj->color[1],obj->color[2]);
    }
    if (!IS_NEAR(z-Spacing,Length)) {
	/* need a post at the end */
	oList[nObjs++] = curr = post(0.0,0.0,Length,BARBED_WIRE_POST_HEIGHT,
	    BARBED_WIRE_POST_WIDTH,FALSE,3);
	HW_MODIFY_3F( curr, hwStrColor,
		obj->color[0],obj->color[1],obj->color[2]);
    }

    curr = hwGroup->create( hwGroup );
    HW_OBJECT_NAMESET(curr,obj);
    curr->modify( curr, hwStrChildren,
		HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
    obj->display_list = createHwSegmentFromObj( &curr, 1 );
}


#define MOVE_TO_POINT(x,y,z) \
    *fptr++ = (x); *fptr++ = (y); *fptr++ = (z); *fptr++ = 0.0; ++count;
#define DRAW_TO_POINT(x,y,z) \
    *fptr++ = (x); *fptr++ = (y); *fptr++ = (z); *fptr++ = 1.0; ++count;
#define FLUSH_PLINE \
{ \
    if (count > 0) { \
	curr = hwPolyline->create( hwPolyline ); \
	HW_MODIFY_1B( curr, hwStrHasFlags, HW_TRUE ); \
	HW_MODIFY_3F( curr, hwStrColor, \
		obj->color[0],obj->color[1],obj->color[2]); \
	curr->modify( curr, hwStrData, \
			HW_MAKE_TYPE(HW_TYPE_FLOAT,4*count), pline ); \
	oList[nObjs++] = curr; \
    } \
    fptr = pline; count = 0; \
}

static void create_chain_link_graphics(
    DRIVE_OBJECT *obj)
{
    float y,z;
    float *pline,*fptr;
    int count,size;
    hwObject
	curr, oList[100];
    int
	nObjs = 0;

    size = (int) ((Height+CHAIN_LINK_MESH_SPACING-.001)/CHAIN_LINK_MESH_SPACING)*2;
    if (size < CHAIN_LINK_BUFFER) size = CHAIN_LINK_BUFFER;
    if ((pline = (float *) malloc(size*8*sizeof(float))) == NULL) {
	return;
    }

    /* handle the partial ones at the beginning */
    fptr = pline; count = 0;
    for (y=CHAIN_LINK_MESH_SPACING; y<Height; y+=CHAIN_LINK_MESH_SPACING) {
	MOVE_TO_POINT(0.0,y,0.0);
	DRAW_TO_POINT(0.0,Height,Height-y);
    }
    FLUSH_PLINE;
    
    /* Now for the main show */
    for (z=0.0; z<Length-Height; z+=CHAIN_LINK_MESH_SPACING) {
	MOVE_TO_POINT(0.0,0.0,z);
	DRAW_TO_POINT(0.0,Height,z+Height);
	if (count >= CHAIN_LINK_BUFFER-2) FLUSH_PLINE;
    }
    FLUSH_PLINE;

    /* Now finish up */
    for (; z<Length; z+=CHAIN_LINK_MESH_SPACING) {
	MOVE_TO_POINT(0.0,0.0,z);
	DRAW_TO_POINT(0.0,Length-z,Length);
    }
    FLUSH_PLINE;

    /* now the opposite way */
    /* handle the partial ones at the beginning */
    for (y=Height; y>0.0; y-=CHAIN_LINK_MESH_SPACING) {
	MOVE_TO_POINT(0.0,y,0.0);
	DRAW_TO_POINT(0.0,0.0,y);
    }
    FLUSH_PLINE;

    fptr = pline; count = 0;
    /* Now for the main show */
    for (z=0.0; z<Length-Height; z+=CHAIN_LINK_MESH_SPACING) {
	MOVE_TO_POINT(0.0,Height,z);
	DRAW_TO_POINT(0.0,0.0,z+Height);
	if (count >= CHAIN_LINK_BUFFER-2) FLUSH_PLINE;
    }
    FLUSH_PLINE;

    /* Now finish up */
    for (; z<Length; z+=CHAIN_LINK_MESH_SPACING) {
	MOVE_TO_POINT(0.0,Height,z);
	DRAW_TO_POINT(0.0,Height-(Length-z),Length);
    }
    FLUSH_PLINE;

    /* the posts */
    for (z=0.0; z<Length; z+=Spacing) {
	curr = hwCone->create( hwCone );
	HW_MODIFY_1F( curr, hwStrRadius, CHAIN_LINK_POST_RADIUS );
	HW_MODIFY_1F( curr, hwStrHeight, Height );
	HW_MODIFY_3F( curr, hwStrRotate, 90.0, 0.0, 0.0 );
	HW_MODIFY_3F( curr, hwStrPos, 0.0, 0.0, z );
	HW_MODIFY_1I( curr, hwStrGraphN, 2 );
	HW_MODIFY_1I( curr, hwStrGraphM, 4 );
	HW_MODIFY_3F( curr, hwStrColor,
			obj->color[0], obj->color[1], obj->color[2] );
	oList[nObjs++] = curr;
    }
    if (!IS_NEAR(z-Spacing,Length)) {
	/* need a post at the end */
	z = Length;
	curr = hwCone->create( hwCone );
	HW_MODIFY_1F( curr, hwStrRadius, CHAIN_LINK_POST_RADIUS );
	HW_MODIFY_1F( curr, hwStrHeight, Height );
	HW_MODIFY_3F( curr, hwStrRotate, 90.0, 0.0, 0.0 );
	HW_MODIFY_3F( curr, hwStrPos, 0.0, 0.0, z );
	HW_MODIFY_1I( curr, hwStrGraphN, 2 );
	HW_MODIFY_1I( curr, hwStrGraphM, 4 );
	HW_MODIFY_3F( curr, hwStrColor,
			obj->color[0], obj->color[1], obj->color[2] );
	oList[nObjs++] = curr;
    }
    curr = hwCone->create( hwCone );
    HW_MODIFY_1F( curr, hwStrRadius, CHAIN_LINK_POST_RADIUS );
    HW_MODIFY_1F( curr, hwStrHeight, Length );
    HW_MODIFY_3F( curr, hwStrPos, 0.0, Height, 0.0 );
    HW_MODIFY_1I( curr, hwStrGraphN, 2 );
    HW_MODIFY_1I( curr, hwStrGraphM, 4 );
    HW_MODIFY_3F( curr, hwStrColor,
			obj->color[0], obj->color[1], obj->color[2] );
    oList[nObjs++] = curr;

    free(pline);

    curr = hwGroup->create( hwGroup );
    HW_OBJECT_NAMESET(curr,obj);
    curr->modify( curr, hwStrChildren,
		HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
    obj->display_list = createHwSegmentFromObj( &curr, 1 );
}


static void create_guardrail_graphics(
    DRIVE_OBJECT *obj)
{
    static float rail1[2][5][3] = {
       {{ GPOST_WIDTH/2.0,		GUARDRAIL_BOTTOM,	1.0 },
	{ GPOST_WIDTH/2.0+GWIDTH,	GUARDRAIL_MID1,		1.0 },
	{ GPOST_WIDTH/2.0+GWIDTH/2.0,	GUARDRAIL_MID2,		1.0 },
	{ GPOST_WIDTH/2.0+GWIDTH,	GUARDRAIL_MID3,		1.0 },
	{ GPOST_WIDTH/2.0,		GUARDRAIL_TOP,		1.0 }},

       {{ GPOST_WIDTH/2.0,		GUARDRAIL_BOTTOM,	0.0 },
	{ GPOST_WIDTH/2.0+GWIDTH,	GUARDRAIL_MID1,		0.0 },
	{ GPOST_WIDTH/2.0+GWIDTH/2.0,	GUARDRAIL_MID2,		0.0 },
	{ GPOST_WIDTH/2.0+GWIDTH,	GUARDRAIL_MID3,		0.0 },
	{ GPOST_WIDTH/2.0,		GUARDRAIL_TOP,		0.0 }},
    };
    static float rail2[2][5][3] = {
       {{ -GPOST_WIDTH/2.0,		GUARDRAIL_BOTTOM,	0.0 },
	{ -GPOST_WIDTH/2.0-GWIDTH,	GUARDRAIL_MID1,		0.0 },
	{ -GPOST_WIDTH/2.0-GWIDTH/2.0,	GUARDRAIL_MID2,		0.0 },
	{ -GPOST_WIDTH/2.0-GWIDTH,	GUARDRAIL_MID3,		0.0 },
	{ -GPOST_WIDTH/2.0,		GUARDRAIL_TOP,		0.0 }},

       {{ -GPOST_WIDTH/2.0,		GUARDRAIL_BOTTOM,	1.0 },
	{ -GPOST_WIDTH/2.0-GWIDTH,	GUARDRAIL_MID1,		1.0 },
	{ -GPOST_WIDTH/2.0-GWIDTH/2.0,	GUARDRAIL_MID2,		1.0 },
	{ -GPOST_WIDTH/2.0-GWIDTH,	GUARDRAIL_MID3,		1.0 },
	{ -GPOST_WIDTH/2.0,		GUARDRAIL_TOP,		1.0 }}
    };
    int i;
    float z;
    hwObject
	curr, oList[100];
    int
	nObjs = 0;


    for (i=0; i<5; ++i) {
	rail1[0][i][2] = Length;
	rail2[1][i][2] = Length;
    }

    curr = hwMesh->create( hwMesh );
    HW_MODIFY_1I( curr, hwStrGraphN, 2 );
    HW_MODIFY_1I( curr, hwStrGraphM, 5 );
    HW_MODIFY_3F( curr, hwStrColor,
		    obj->color[0],obj->color[1],obj->color[2] );
    curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,2*5*3), rail1 );
    oList[nObjs++] = curr;

    curr = hwMesh->create( hwMesh );
	HW_MODIFY_1I( curr, hwStrGraphN, 2 );
    HW_MODIFY_1I( curr, hwStrGraphM, 5 );
    HW_MODIFY_3F( curr, hwStrColor,
		    obj->color[0],obj->color[1],obj->color[2] );
    curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,2*5*3), rail2 );
    oList[nObjs++] = curr;

    for (z=0.0; z<Length; z += Spacing) {
	oList[nObjs++] = curr = post(0.0,0.0,z,
	    GPOST_HEIGHT,GPOST_WIDTH,TRUE,4);
	WOOD_HW(curr);
    }
    if (!IS_NEAR(z-Spacing,Length)) {
	/* need a post at the end */
	oList[nObjs++] = curr = post(0.0,0.0,Length,SPLIT_RAIL_POST_HEIGHT,
	    SPLIT_RAIL_POST_WIDTH,TRUE,4);
	WOOD_HW(curr);
    }

    curr = hwGroup->create( hwGroup );
    HW_OBJECT_NAMESET(curr,obj);
    curr->modify( curr, hwStrChildren,
		HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
    obj->display_list = createHwSegmentFromObj( &curr, 1 );
}


void init_fence_object(
    DRIVE_OBJECT *obj)
{
    FENCE_LIST *gl;
    void (*graphics_routine)(DRIVE_OBJECT *obj);
    int type;
    float spacing;
    float def_color[3];


    if (strcmp(obj->subtype,"split rail") == 0) type = TYPE_SPLIT_RAIL;
    else if (strcmp(obj->subtype,"barbed wire") == 0) type = TYPE_BARBED_WIRE;
    else if (strcmp(obj->subtype,"chain link") == 0) type = TYPE_CHAIN_LINK;
    else if (strcmp(obj->subtype,"guardrail") == 0) type = TYPE_GUARDRAIL;
    else if (strcmp(obj->subtype,"picket") == 0) type = TYPE_PICKET;
    else type = TYPE_SPLIT_RAIL;

    switch (type) {
	case TYPE_SPLIT_RAIL:
	    Height = SPLIT_RAIL_POST_HEIGHT;
	    obj->bound_mc[0] = -SPLIT_RAIL_WIDTH/2.0;
	    obj->bound_mc[3] =  SPLIT_RAIL_WIDTH/2.0;
	    obj->bound_mc[4] =  SPLIT_RAIL_POST_HEIGHT;
	    def_color[0] = WOOD_RED;
	    def_color[1] = WOOD_GREEN;
	    def_color[2] = WOOD_BLUE;
	    spacing = SPLIT_RAIL_POST_SPACING;
	    graphics_routine = create_split_rail_graphics;
	    break;

	case TYPE_BARBED_WIRE:
	    Height = BARBED_WIRE_POST_HEIGHT;
	    obj->bound_mc[0] = -BARBED_WIRE_POST_WIDTH/2.0;
	    obj->bound_mc[3] =  BARBED_WIRE_POST_WIDTH/2.0;
	    obj->bound_mc[4] =  BARBED_WIRE_POST_HEIGHT;
	    def_color[0] = STEEL_RED;
	    def_color[1] = STEEL_GREEN;
	    def_color[2] = STEEL_BLUE;
	    spacing = BARBED_WIRE_POST_SPACING;
	    graphics_routine = create_barbed_wire_graphics;
	    break;

	case TYPE_CHAIN_LINK:
	    obj->bound_mc[0] = -CHAIN_LINK_POST_RADIUS/2.0;
	    obj->bound_mc[3] =  CHAIN_LINK_POST_RADIUS/2.0;
	    def_color[0] = STEEL_RED;
	    def_color[1] = STEEL_GREEN;
	    def_color[2] = STEEL_BLUE;
	    spacing = CHAIN_LINK_POST_SPACING;
	    graphics_routine = create_chain_link_graphics;
	    break;

	case TYPE_GUARDRAIL:
	default:
	    Height = GPOST_HEIGHT;
	    obj->bound_mc[0] = -GWIDTH/2.0;
	    obj->bound_mc[3] =  GWIDTH/2.0;
	    obj->bound_mc[4] =  GPOST_HEIGHT;
	    def_color[0] = STEEL_RED;
	    def_color[1] = STEEL_GREEN;
	    def_color[2] = STEEL_BLUE;
	    spacing = GUARDRAIL_POST_SPACING;
	    graphics_routine = create_guardrail_graphics;
	    break;

    }

    if (Height == DEFAULT_OBJECT_SIZE) Height = DEFAULT_HEIGHT;
    if (Length == DEFAULT_OBJECT_SIZE) Length = DEFAULT_LENGTH;
    if (Spacing == DEFAULT_OBJECT_SIZE) Spacing = spacing;
    if (obj->color[0] == DEFAULT_OBJECT_COLOR) obj->color[0] = def_color[0];
    if (obj->color[1] == DEFAULT_OBJECT_COLOR) obj->color[1] = def_color[1];
    if (obj->color[2] == DEFAULT_OBJECT_COLOR) obj->color[2] = def_color[2];

    /* Some things are constant for all fence types. */
    obj->bound_mc[1] =  0.0;
    obj->bound_mc[2] =  0.0;
    obj->bound_mc[4] =  Height;
    obj->bound_mc[5] =  Length;

    /* Look for one the same. */
    gl = fence_list;
    while (gl != NULL) {
	if (IS_NEAR(gl->length,Length)
		&& (gl->type == type)
		&& IS_NEAR(gl->color[0],obj->color[0])
		&& IS_NEAR(gl->color[1],obj->color[1])
		&& IS_NEAR(gl->color[2],obj->color[2])
		&& IS_NEAR(gl->spacing,Spacing)
		&& (gl->nameset_bits == obj->nameset_bits)) {
	    break;
	}
	gl = gl->next;
    }
    if (gl != NULL) {
	/* Reuse this one. */
	obj->display_list = gl->dl_number;
    }
    else {
	/* Gotta create a new one. */
	if ((gl = (FENCE_LIST *) malloc(sizeof(FENCE_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space.\n");
	    return;
	}
	gl->length = Length;
	gl->nameset_bits = obj->nameset_bits;
	gl->type = type;
	gl->color[0] = obj->color[0];
	gl->color[1] = obj->color[1];
	gl->color[2] = obj->color[2];
	gl->spacing = Spacing;
	(*graphics_routine)(obj);
	gl->dl_number = obj->display_list;
	gl->next = fence_list;
	fence_list = gl;
    }

    obj->surface_chars_xyz  = fence_surface_chars_xyz;
    obj->surface_chars_bbox = fence_surface_chars_bbox;

    /* Update WC bounding box */
    update_wc_bounds(obj);
}
