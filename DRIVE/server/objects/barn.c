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


/* Code Module for the barn segment object */

#include <stdio.h>
#include <math.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"

#define DEFAULT_HEIGHT (40.0)
#define DEFAULT_LENGTH (80.0)
#define DEFAULT_WIDTH  (40.0)

#define Length			(obj->size[SIZE_LENGTH])
#define Width			(obj->size[SIZE_WIDTH])
#define Height			(obj->size[SIZE_HEIGHT])
#define BankWidth		(Height/2.0)

typedef struct _barn_list {
    float length,width,height;
    unsigned int nameset_bits;
    int dl_number;
    struct _barn_list *next;
} BARN_LIST;
static BARN_LIST *barn_list=NULL;



static int barn_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = Height;
    get_box_mc_normal(obj,x,y,z,sc->mc_normal);
    return(TRUE);
}


static int barn_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = Height;
    return(TRUE);
}


static int trimmings( hwObject *oList )
{
    static float 
	Roof_under1[] = {
	     20, 20, -42,
	     15, 35, -42,
	      0, 40, -42,
	    -15, 35, -42,
	    -20, 20, -42,
	     20, 20, -39,
	     15, 35, -39,
	      0, 40, -39,
	    -15, 35, -39,
	    -20, 20, -39,

	},
	Roof_under2[] = {

	     20, 20, 39,
	     15, 35, 39,
	      0, 40, 39,
	    -15, 35, 39,
	    -20, 20, 39,

	     20, 20, 42,
	     15, 35, 42,
	      0, 40, 42,
	    -15, 35, 42,
	    -20, 20, 42,

	},

	/* Barn loft */
	BarnLoft[] = {
	      5, 22, -40.5,
	      5, 32, -40.5,
	     -5, 32, -40.5,
	     -5, 22, -40.5,
	},

	/* Barn door - outlines */
	BarnDoor0[] = {
	    7.5,   0, -40.5,
	    7.5,14.5, -40.5,
	    .25,14.5, -40.5,
	    .25,   0, -40.5,
	},

	BarnDoor1[] = {
	   -.25,   0, -40.5,
	   -.25,14.5, -40.5,
	   -7.5,14.5, -40.5,
	   -7.5,   0, -40.5,
	},

	BarnDoor2[] = {
	     -8,  0, 40.5,
	     -8, 15, 40.5,
	      8, 15, 40.5,
	      8,  0, 40.5,
	},

	/* Barn window */
	BarnWin[] = {
	      0, 10,  -2,
	      0, 10,   2,
	      0, 14,   2,
	      0, 14,  -2,
	},
	ActualBarnWin[4 * 3],
	data[12 * 4 * 3];
    int
	i, j, q,
	nObjs = 0;
    hwObject
	curr;

    /* Right windows */
    (void)memcpy( data + 0*4*3, BarnDoor0, 4*3*sizeof(float) );
    (void)memcpy( data + 1*4*3, BarnDoor1, 4*3*sizeof(float) );
    (void)memcpy( data + 2*4*3, BarnDoor2, 4*3*sizeof(float) );
    (void)memcpy( data + 3*4*3, BarnLoft,  4*3*sizeof(float) );
    q = 4;
    for( i = 1; i < 4; i++ ) {
	for( j = 0; j < 4; j++ ) {
	    ActualBarnWin[3*j  ] = BarnWin[3*j  ] + 20.5;
	    ActualBarnWin[3*j+1] = BarnWin[3*j+1];
	    ActualBarnWin[3*j+2] = BarnWin[3*j+2] + 20*i - 40;
	}
	(void)memcpy( data + q*4*3, ActualBarnWin, 4*3*sizeof(float) );
	q++;
    }
    /* Left windows */
    for( i = 1; i < 4; i++ ) {
	for( j = 0; j < 4; j++ ) {
	    /* Reverse the polygons... */
	    ActualBarnWin[3*j  ] = BarnWin[3*(3-j)  ] - 20.5;
	    ActualBarnWin[3*j+1] = BarnWin[3*(3-j)+1];
	    ActualBarnWin[3*j+2] = BarnWin[3*(3-j)+2] + 20*i - 40;
	}
	(void)memcpy( data + q*4*3, ActualBarnWin, 4*3*sizeof(float) );
	q++;
    }
    curr = hwQuads->create( hwQuads );
    HW_MODIFY_3F( curr, hwStrColor, 0.0, 0.0, 0.0 );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,q*4*3), data );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrTwoSided, HW_TRUE );
    oList[nObjs++] = curr;

    curr = hwMesh->create( hwMesh );
    HW_MODIFY_1I( curr, hwStrGraphN, 2 );
    HW_MODIFY_1I( curr, hwStrGraphM, 5 );
    HW_MODIFY_3F( curr, hwStrColor, 0.5, 0.5, 0.5 );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,2*5*3), Roof_under1 );
    oList[nObjs++] = curr;

    curr = hwMesh->create( hwMesh );
    HW_MODIFY_1I( curr, hwStrGraphN, 2 );
    HW_MODIFY_1I( curr, hwStrGraphM, 5 );
    HW_MODIFY_3F( curr, hwStrColor, 0.5, 0.5, 0.5 );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,2*5*3), Roof_under2 );
    oList[nObjs++] = curr;

    return nObjs;
}


static int barn( hwObject *oList )
{
    static float
	/* Outline of front of barn */
	BarnFront[] = {
	     20,  0, -40,
	     20, 20, -40,
	     15, 35, -40,
	      0, 40, -40,
	    -15, 35, -40,
	    -20, 20, -40,
	    -20,  0, -40,
	},
	/* Back of barn */
	BarnBack[] = {
	    -20,  0,  40,
	    -20, 20,  40,
	    -15, 35,  40,
	      0, 40,  40,
	     15, 35,  40,
	     20, 20,  40,
	     20,  0,  40,
	},
	/* Left side of barn */
	BarnLeft[] = {
	    -20, 20, -40,
	    -20, 20,  40,
	    -20,  0,  40,
	    -20,  0, -40,
	},
	/* Right side of barn */
	BarnRight[] = {
	     20,  0, -40,
	     20,  0,  40,
	     20, 20,  40,
	     20, 20, -40,
	},
	Roof[] = {
	    -20, 20, -42,
	    -15, 35, -42,
	      0, 40, -42,
	     15, 35, -42,
	     20, 20, -42,
	    -20, 20,  42,
	    -15, 35,  42,
	      0, 40,  42,
	     15, 35,  42,
	     20, 20,  42,
	};
    hwObject
	curr;
    int
	nObjs = 0;

    /* Red barn */
    curr = hwPolygon->create( hwPolygon );
    HW_MODIFY_3F( curr, hwStrColor, 1.0, 0.0, 0.0 );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,sizeof(BarnFront)/sizeof(float)),
		BarnFront );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrTwoSided, HW_TRUE );
    oList[nObjs++] = curr;

    curr = hwPolygon->create( hwPolygon );
    HW_MODIFY_3F( curr, hwStrColor, 1.0, 0.0, 0.0 );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,sizeof(BarnBack)/sizeof(float)),
		BarnBack );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrTwoSided, HW_TRUE );
    oList[nObjs++] = curr;

    curr = hwPolygon->create( hwPolygon );
    HW_MODIFY_3F( curr, hwStrColor, 1.0, 0.0, 0.0 );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,sizeof(BarnLeft)/sizeof(float)),
		BarnLeft );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrTwoSided, HW_TRUE );
    oList[nObjs++] = curr;

    curr = hwPolygon->create( hwPolygon );
    HW_MODIFY_3F( curr, hwStrColor, 1.0, 0.0, 0.0 );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,sizeof(BarnRight)/sizeof(float)),
		BarnRight );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrTwoSided, HW_TRUE );
    oList[nObjs++] = curr;


    /* Grey roof */
    curr = hwMesh->create( hwMesh );
    HW_MODIFY_1I( curr, hwStrGraphN, 2 );
    HW_MODIFY_1I( curr, hwStrGraphM, 5 );
    HW_MODIFY_3F( curr, hwStrColor, 0.5, 0.5, 0.5 );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,2*5*3), Roof );
    oList[nObjs++] = curr;

    return nObjs;
}

static void create_barn_graphics(
    DRIVE_OBJECT *obj)
{
    hwObject
	oList[100];
    int
	nObjs;

    nObjs = trimmings( oList );
    nObjs += barn( oList + nObjs );
    /* TBD: LOD */
    obj->display_list = createHwSegmentFromObj( oList, nObjs );
}


void init_barn_object(
    DRIVE_OBJECT *obj)
{
    BARN_LIST *rl;

    Length = DEFAULT_LENGTH;
    Width = DEFAULT_WIDTH;
    Height = DEFAULT_HEIGHT;

    if(debug) printf(" inside init_barn_object() routine \n");

    obj->num_children = 0;

    /* See if we've created one like this before... */
    rl = barn_list;
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
	if ((rl = (BARN_LIST *) malloc(sizeof(BARN_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}
	rl->length = Length;
	rl->width  = Width;
	rl->height = Height;
	rl->nameset_bits = obj->nameset_bits;
	rl->next   = barn_list;
    	create_barn_graphics(obj);
	rl->dl_number = obj->display_list;
	barn_list  = rl;
    }

    obj->surface_chars_xyz  = barn_surface_chars_xyz;
    obj->surface_chars_bbox = barn_surface_chars_bbox;

    /* Initial (mc) bounding box values */
    obj->bound_mc[0] = -Width/2.0 - 1.0;
    obj->bound_mc[1] = 0.0;
    obj->bound_mc[2] = -Length/2.0 - 1.0;
    obj->bound_mc[3] = Width/2.0 + 1.0;
    obj->bound_mc[4] = Height;
    obj->bound_mc[5] = Length/2.0 + 1.0;

    /* apply the object's xform matrix to the bounding box to put it in
     * world coordinates */
    update_wc_bounds(obj);

    elevate_object_to_terrain_height((SCENE *) obj->scene,obj,TRUE);
}
