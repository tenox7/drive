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


/* Code Module for the ground object */


#include <stdio.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"

static int ground_seg = -1;

static int ground_surface_chars(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = 0.0;
    return(TRUE);
}


static hwObject high_res_model( void )
{
    float mesh[GROUND_MESH_SIZE][GROUND_MESH_SIZE][6];
    float x,y,z,inc,offset,range;
    int i,j;
    hwObject obj;
    static hwObject texture;

    if( do_textures ) {
	texture = hwFindObject( "GrassTexture" );
    }

    y = Y_GROUND;
    inc = GROUND_EXTENT*2.0/(GROUND_MESH_SIZE-1);

    /* The corners must match */
    mesh[0][0][0] =
	mesh[0][GROUND_MESH_SIZE-1][0] = 
	-GROUND_EXTENT;
    mesh[GROUND_MESH_SIZE-1][0][0] =
	mesh[GROUND_MESH_SIZE-1][GROUND_MESH_SIZE-1][0] = 
	GROUND_EXTENT;
    mesh[0][0][1] =
	mesh[GROUND_MESH_SIZE-1][0][1] =
	mesh[0][GROUND_MESH_SIZE-1][1] =
	mesh[GROUND_MESH_SIZE-1][GROUND_MESH_SIZE-1][1] =
	y;
    mesh[0][0][2] =
	mesh[GROUND_MESH_SIZE-1][0][2] = 
	-GROUND_EXTENT;
    mesh[0][GROUND_MESH_SIZE-1][2] =
	mesh[GROUND_MESH_SIZE-1][GROUND_MESH_SIZE-1][2] = 
	GROUND_EXTENT;

    /* And the edges must match */
    for (z=-GROUND_EXTENT+inc,j=1; j<(GROUND_MESH_SIZE-1); ++j,z+=inc) {
	mesh[0][j][0] = -GROUND_EXTENT;
	mesh[GROUND_MESH_SIZE-1][j][0] = GROUND_EXTENT;
	mesh[j][0][0] = 
	    mesh[j][GROUND_MESH_SIZE-1][0] =
	    z;
	mesh[0][j][1] =
	    mesh[GROUND_MESH_SIZE-1][j][1] =
	    mesh[j][0][1] = 
	    mesh[j][GROUND_MESH_SIZE-1][1] =
	    y;
	mesh[j][0][2] = -GROUND_EXTENT;
	mesh[j][GROUND_MESH_SIZE-1][2] = GROUND_EXTENT;
	mesh[0][j][2] = 
	    mesh[GROUND_MESH_SIZE-1][j][2] =
	    z;
    }

    /* Now the interior */
    range = inc/3.0;
    offset = -range/2.0;
    for (x = -GROUND_EXTENT+inc+offset, i=1;
	    i < (GROUND_MESH_SIZE-1);
	    ++i, x += inc) {
	for (z = -GROUND_EXTENT+inc+offset, j=1;
		j < (GROUND_MESH_SIZE-1);
		++j, z += inc) {
	    mesh[i][j][0] = x + FLOATRAND(range);
	    mesh[i][j][1] = y;
	    mesh[i][j][2] = z + FLOATRAND(range);
	}
    }

    /* Now the colors */
    do_mesh_colors(mesh);

    obj = hwMesh->create( hwMesh );
    HW_MODIFY_1B( obj, hwStrHasRGB, HW_TRUE );
    HW_MODIFY_1F( obj, hwStrShininess, 0.0 );
    HW_MODIFY_1B( obj, hwStrBackface, HW_TRUE );
    HW_MODIFY_1I( obj, hwStrGraphN, GROUND_MESH_SIZE );
    HW_MODIFY_1I( obj, hwStrGraphM, GROUND_MESH_SIZE );
    obj->modify( obj, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,GROUND_MESH_SIZE*GROUND_MESH_SIZE*6),
		mesh );
    if( do_textures ) {
	obj->modify( obj, hwStrTexture, HW_TYPE_OBJECT, texture );
    }
    return obj;
}

#if _LOW_RES_MODEL_USED
static hwObject low_res_model( void )
{
    static float pgon[] = {
	-GROUND_EXTENT,Y_GROUND,-GROUND_EXTENT,
	 GROUND_EXTENT,Y_GROUND,-GROUND_EXTENT,
	 GROUND_EXTENT,Y_GROUND, GROUND_EXTENT,
	-GROUND_EXTENT,Y_GROUND, GROUND_EXTENT,
    };
    hwObject obj;

    obj = hwPolygon->create( hwPolygon );
    HW_MODIFY_3F( obj, hwStrColor, GROUND_R_AVE, GROUND_G_AVE, GROUND_B_AVE );
    obj->modify( obj, hwStrData, HW_MAKE_TYPE(HW_TYPE_FLOAT,12), pgon );
    return obj;
}
#endif


static void create_ground_graphics( DRIVE_OBJECT *obj )
{
    hwObject
	hwObj;

    /* TBD: LOD */
    hwObj = high_res_model();
    hwObj->name = 0;
    obj->display_list = createHwSegmentFromObj( &hwObj, 1 );
}


void init_ground_object(
    DRIVE_OBJECT *obj)
{

    if(debug) printf(" inside init_ground_object() routine \n");

    obj->num_children = 0;

    if (ground_seg == -1) {
    	create_ground_graphics(obj);
	ground_seg = obj->display_list;
    }
    else {
	obj->display_list = ground_seg;
    }

    obj->surface_chars_xyz = ground_surface_chars;
}
