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
#include "scene.h"

#define DEFAULT_HEIGHT	(50.0)
#define Y_VARIATION	(Height)
#ifdef TEST_CODE
# define HILL_CHANCE	(1)	/* 1-in-N chance when not constrained */
#else
# define HILL_CHANCE	(3)	/* 1-in-N chance when not constrained */
#endif

#define Height		(obj->size[SIZE_HEIGHT])
#define SQUARESIZE	((2.0*GROUND_EXTENT)/(GROUND_MESH_SIZE-1))
#define RAND_Y		(BOUNDED_FLOATRAND(-Y_VARIATION,Y_VARIATION))

#define EPSILON		(1.0e-6)

typedef struct _hill_list {
    int type;
    float user_height;
    int display_list;
    float maxht;
    float height[GROUND_MESH_SIZE][GROUND_MESH_SIZE];
    struct _hill_list *next;
} HILL_LIST;
static HILL_LIST *hill_list = NULL;


static float bi_interp(
    float offx, float offz,
    float h1, float h2, float h3)
{
    float f,ha,hb;

    if (offz > (1.0-EPSILON)) return(h2);

    ha = h1*(1.0-offz) + h2*offz;
    hb = h3*(1.0-offz) + h2*offz;
    f  = offx/(1.0-offz);
    return(ha*(1.0-f) + hb*f);
}


static int hill_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    int whichx,whichz;
    float offsetx,offsetz;
    HILL_LIST *hl = (HILL_LIST *) (obj->additional_data);

    sc->mc_x = x;
    sc->mc_z = z;

    /* Use bi-linear interpolation. */
    x += GROUND_EXTENT;
    z += GROUND_EXTENT;

    if ((whichx = x / SQUARESIZE) > (GROUND_MESH_SIZE-2))
	whichx = (GROUND_MESH_SIZE-2);
    else if (whichx < 0) whichx = 0;
    if ((whichz = z / SQUARESIZE) > (GROUND_MESH_SIZE-2))
	whichz = (GROUND_MESH_SIZE-2);
    else if (whichz < 0) whichz = 0;

    offsetx = (x - whichx * SQUARESIZE) / SQUARESIZE;
    offsetz = (z - whichz * SQUARESIZE) / SQUARESIZE;

#ifdef LEFT_HAND_TRIANGULARIZATION
    if (offsetx + offsetz > 1.0) {
	/* in the far triangle */
	sc->mc_y = bi_interp(1.0-offsetx,1.0-offsetz,
	    hl->height[whichx+1][whichz+1],
	    hl->height[whichx+1][whichz],
	    hl->height[whichx][whichz+1]);
    }
    else {
	sc->mc_y = bi_interp(offsetx,offsetz,
	    hl->height[whichx][whichz],
	    hl->height[whichx][whichz+1],
	    hl->height[whichx+1][whichz]);
    }
#else
    if (offsetx > offsetz) {
	sc->mc_y = bi_interp(1.0-offsetx,offsetz,
	    hl->height[whichx+1][whichz],
	    hl->height[whichx+1][whichz+1],
	    hl->height[whichx][whichz]);
    }
    else {
	sc->mc_y = bi_interp(offsetx,1.0-offsetz,
	    hl->height[whichx][whichz+1],
	    hl->height[whichx][whichz],
	    hl->height[whichx+1][whichz+1]);
    }
#endif /* LEFT_HAND_TRIANGULARIZATION else */


    sc->mc_normal[0] = 0.0; sc->mc_normal[1] = 1.0; sc->mc_normal[2] = 0.0;
    sc->friction = 1.0;
    sc->roughness = 0.1;
    sc->roughness_frequency = 5.0;

    return(TRUE);
}


#define DELTA	   (SCENE_SIZE/(GROUND_MESH_SIZE-1))

static float high_res_model(
    DRIVE_OBJECT *obj,
    int type,
    HILL_LIST *hptr,
    DRIVE_OBJECT *hu,
    DRIVE_OBJECT *hd,
    DRIVE_OBJECT *hl,
    DRIVE_OBJECT *hr)
{
    float mesh[GROUND_MESH_SIZE][GROUND_MESH_SIZE][6];
    float x,z,maxht_found;
    int i,j,offset;
    hwObject 
	meshObj;
    hwObject
	texture = NULL;

    if( do_textures ) {
	texture = hwFindObject( "GrassTexture" );
    }

    /* X's and Z's are fixed! */
    for (z=-GROUND_EXTENT,j=0; j<GROUND_MESH_SIZE; ++j,z += SQUARESIZE) {
	for (x=-GROUND_EXTENT,i=0; i<GROUND_MESH_SIZE; ++i,x += SQUARESIZE) {
	    mesh[i][j][0] = x;
	    mesh[i][j][2] = z;
	}
    }
    /* Now just worry about the Y's */

    /* Bottom edge */
    if (type & SCENE_DOWN_FLAT) {
	for (i=0; i<GROUND_MESH_SIZE; ++i) {
	    mesh[i][0][1] = Y_GROUND;
	}
    }
    else {
	if (hd != NULL) {
	    for (i=0,x=(-SCENE_SIZE/2.0); i<GROUND_MESH_SIZE; ++i,x+=DELTA) {
		mesh[i][0][1] = hill_y_mc(obj,hd,x,(-SCENE_SIZE/2.0)-1.0);
	    }
	}
	else {
	    if ((mesh[0][0][1] = RAND_Y) < Y_GROUND) mesh[0][0][1] = Y_GROUND;
	    for (i=1; i<GROUND_MESH_SIZE; ++i) {
		if ((mesh[i][0][1] = mesh[i-1][0][1] + RAND_Y) < Y_GROUND) {
		    mesh[i][0][1] = Y_GROUND;
		}
	    }
	}
    }

    /* Left edge */
    if (type & SCENE_LEFT_FLAT) {
	for (i=0; i<GROUND_MESH_SIZE; ++i) {
	    mesh[0][i][1] = Y_GROUND;
	}
    }
    else {
	if (hl != NULL) {
	    for (i=0,z=(-SCENE_SIZE/2.0); i<GROUND_MESH_SIZE; ++i,z+=DELTA) {
		mesh[0][i][1] = hill_y_mc(obj,hl,(-SCENE_SIZE/2.0)-1.0,z);
	    }
	}
	else {
	    /* mesh[0][0][1] already set, but get a random number anyway. */
	    i = RAND_Y;
	    for (i=1; i<GROUND_MESH_SIZE; ++i) {
		if ((mesh[0][i][1] = mesh[0][i-1][1] + RAND_Y) < Y_GROUND) {
		    mesh[0][i][1] = Y_GROUND;
		}
	    }
	}
    }
		

    /* Top edge */
    if (type & SCENE_UP_FLAT) {
	for (i=0; i<GROUND_MESH_SIZE; ++i) {
	    mesh[i][GROUND_MESH_SIZE-1][1] = Y_GROUND;
	}
    }
    else {
	if (hu != NULL) {
	    for (i=0,x=(-SCENE_SIZE/2.0); i<GROUND_MESH_SIZE; ++i,x+=DELTA) {
		mesh[i][GROUND_MESH_SIZE-1][1] =
		    hill_y_mc(obj,hu,x,(SCENE_SIZE/2.0)+1.0);
	    }
	}
	else {
	    /* mesh[0][GROUND_MESH_SIZE-1][1] already set,
	     * but get random number anyway.
	     */
	    i = RAND_Y;
	    for (i=1; i<GROUND_MESH_SIZE; ++i) {
		if ((mesh[i][GROUND_MESH_SIZE-1][1]
			    = mesh[i-1][GROUND_MESH_SIZE-1][1] + RAND_Y)
			< Y_GROUND) {
		    mesh[i][GROUND_MESH_SIZE-1][1] = Y_GROUND;
		}
	    }
	}
    }
		

    /* Right edge */
    if (type & SCENE_RIGHT_FLAT) {
	for (i=0; i<GROUND_MESH_SIZE; ++i) {
	    mesh[GROUND_MESH_SIZE-1][i][1] = Y_GROUND;
	}
    }
    else {
	if (hr != NULL) {
	    for (i=0,z=(-SCENE_SIZE/2.0); i<GROUND_MESH_SIZE; ++i,z+=DELTA) {
		mesh[GROUND_MESH_SIZE-1][i][1] =
		    hill_y_mc(obj,hr,(SCENE_SIZE/2.0)+1.0,z);
	    }
	}
	else {
	    /* mesh[GROUND_MESH_SIZE-1][0][1] already set,
	     * but get random number anyway.
	     */
	    i = RAND_Y;
	    for (i=1; i<GROUND_MESH_SIZE; ++i) {
		if ((mesh[GROUND_MESH_SIZE-1][i][1]
			    = mesh[GROUND_MESH_SIZE-1][i-1][1] + RAND_Y)
			< Y_GROUND) {
		    mesh[GROUND_MESH_SIZE-1][i][1] = Y_GROUND;
		}
	    }
	}
    }

    /* Now the interior */
    offset = 1;
    while (offset < (GROUND_MESH_SIZE/2.0)) {
	j = (GROUND_MESH_SIZE-1)-offset;
	for (i=offset; i<=j; ++i) {
	    /* lower */
	    if ((mesh[i][offset][1] = (mesh[i-1][offset][1] +
		    mesh[i][offset-1][1]) / 2.0 + RAND_Y) < Y_GROUND) 
		mesh[i][offset][1] = Y_GROUND;
	    /* left */
	    if ((mesh[offset][i][1] = (mesh[offset][i-1][1] +
		    mesh[offset-1][i][1]) / 2.0 + RAND_Y) < Y_GROUND) 
		mesh[offset][i][1] = Y_GROUND;
	    /* top */
	    if ((mesh[i][j][1] = (mesh[i-1][j][1] + mesh[i][j+1][1]) / 2.0
		    + RAND_Y) < Y_GROUND) 
		mesh[i][j][1] = Y_GROUND;
	    /* right */
	    if ((mesh[j][i][1] = (mesh[j][i-1][1] + mesh[j+1][i][1]) / 2.0
		    + RAND_Y) < Y_GROUND) 
		mesh[j][i][1] = Y_GROUND;
	}
	++offset;
    }

    /* Now the colors */
    do_mesh_colors(mesh);

    meshObj = hwMesh->create(hwMesh);
    HW_MODIFY_1B(meshObj, hwStrHasRGB, HW_TRUE);
    HW_MODIFY_1B(meshObj, hwStrBackface, HW_TRUE);
    /*HW_MODIFY_1B(meshObj, hwStrFlipNormals, HW_TRUE);*/
    HW_MODIFY_1I(meshObj, hwStrGraphN, GROUND_MESH_SIZE);
    HW_MODIFY_1I(meshObj, hwStrGraphM, GROUND_MESH_SIZE);
    if( do_textures ) {
	meshObj->modify( meshObj, hwStrTexture, HW_TYPE_OBJECT, texture );
    }
    meshObj->modify(meshObj, hwStrData, 
	HW_MAKE_TYPE(HW_TYPE_FLOAT,GROUND_MESH_SIZE*GROUND_MESH_SIZE*6),mesh);

    /* Copy the heights to the structure */
    maxht_found = Y_GROUND;
    for (j=0; j<GROUND_MESH_SIZE; ++j) {
	for (i=0; i<GROUND_MESH_SIZE; ++i) {
	    if ((hptr->height[i][j] = mesh[i][j][1]) > maxht_found) {
		maxht_found = mesh[i][j][1];
	    }
	}
    }

    obj->display_list = createHwSegmentFromObj(&meshObj,1);

    return(maxht_found);
}


static void create_hill_graphics(
    DRIVE_OBJECT *obj,
    int type,
    HILL_LIST *hptr,
    DRIVE_OBJECT *hu,
    DRIVE_OBJECT *hd,
    DRIVE_OBJECT *hl,
    DRIVE_OBJECT *hr)
{
    float maxht_found;

    maxht_found = high_res_model(obj,type,hptr,hu,hd,hl,hr);

    hptr->maxht = maxht_found;
}



void init_hill_object(
    DRIVE_OBJECT *obj)
{
    SCENE *scene = (SCENE *) (obj->scene);
    HILL_LIST *hptr;
    DRIVE_OBJECT *hu,*hd,*hl,*hr;

    if (Height == DEFAULT_OBJECT_SIZE) Height = DEFAULT_HEIGHT;

    obj->num_children = 0;

    /* Figure out how to match the edges. */
    scene->flags &= (~SCENE_FLAT_FLAGS);
    if (scene->up == NULL) {
	if (ZINTRAND(HILL_CHANCE) != 0) scene->flags |= SCENE_UP_FLAT;
    }
    else {
	if (scene->up->flags & (SCENE_ALL_FLAT|SCENE_DOWN_FLAT))
	    scene->flags |= SCENE_UP_FLAT;
    }

    if (scene->down == NULL) {
	if (ZINTRAND(HILL_CHANCE) != 0) scene->flags |= SCENE_DOWN_FLAT;
    }
    else {
	if (scene->down->flags & (SCENE_ALL_FLAT|SCENE_UP_FLAT))
	    scene->flags |= SCENE_DOWN_FLAT;
    }

    if (scene->left == NULL) {
	if (ZINTRAND(HILL_CHANCE) != 0) scene->flags |= SCENE_LEFT_FLAT;
    }
    else {
	if (scene->left->flags & (SCENE_ALL_FLAT|SCENE_RIGHT_FLAT))
	    scene->flags |= SCENE_LEFT_FLAT;
    }

    if (scene->right == NULL) {
	if (ZINTRAND(HILL_CHANCE) != 0) scene->flags |= SCENE_RIGHT_FLAT;
    }
    else {
	if (scene->right->flags & (SCENE_ALL_FLAT|SCENE_LEFT_FLAT))
	    scene->flags |= SCENE_RIGHT_FLAT;
    }


    hd = find_hill_in_scene(scene->down);
    hu = find_hill_in_scene(scene->up);
    hl = find_hill_in_scene(scene->left);
    hr = find_hill_in_scene(scene->right);

    hptr = NULL;
    if ((scene->flags & SCENE_FLAT_EDGE_FLAGS) == SCENE_FLAT_EDGE_FLAGS) {
	for (hptr = hill_list; hptr != NULL; hptr = hptr->next) {
	    if (IS_NEAR(hptr->user_height,Height)
		    && (hptr->type == scene->flags)) {
		break;
	    }
	}
    }

    if (hptr != NULL) {
	obj->display_list = hptr->display_list;
    }
    else {
	if ((hptr = (HILL_LIST *) malloc(sizeof(HILL_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    exit(1);
	}
	hptr->type = scene->flags & SCENE_FLAT_EDGE_FLAGS;
	hptr->user_height = Height;
	create_hill_graphics(obj,hptr->type,hptr,hu,hd,hl,hr);
	hptr->display_list = obj->display_list;
	hptr->next = hill_list;
	hill_list = hptr;
    }

    obj->additional_data = (void *) hptr;

    obj->surface_chars_xyz = hill_surface_chars_xyz;
    obj->bound_mc[0] = -GROUND_EXTENT;
    obj->bound_mc[1] = Y_GROUND;
    obj->bound_mc[2] = -GROUND_EXTENT;
    obj->bound_mc[3] = GROUND_EXTENT;
    obj->bound_mc[4] = hptr->maxht;
    obj->bound_mc[5] = GROUND_EXTENT;
    update_wc_bounds(obj);
}
