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


/* Code Module for the mound object */


#include <stdio.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"

static float default_mound_dl = -1;

/* #define LEFT_HAND_TRIANGULARIZATION 1 */

typedef struct _mound_list {
    float r,g,b;
    unsigned int nameset_bits;
    int dl_number;
    struct _mound_list *next;
} MOUND_LIST;
static MOUND_LIST *mound_list=NULL;

#define Length			(obj->size[SIZE_LENGTH])
#define Width			(obj->size[SIZE_WIDTH])
#define Height			(obj->size[SIZE_HEIGHT])
#define MOUND_MESH_SIZE		5
#define DEFAULT_LENGTH		100.0
#define DEFAULT_WIDTH		100.0
#define DEFAULT_HEIGHT		20.0
#define SQUARESIZE		(1.0/(MOUND_MESH_SIZE-1))

#ifndef EPSILON
# define EPSILON (1.0e-6)
#endif /* !EPSILON */

#define MOUNDHEIGHT(x,z) \
    ((FCOS((x)*(M_PI/2.0))*FCOS((z)*(M_PI/2.0))-0.05)/0.95)

#define MOUNDHEIGHT_QUANTIZED(xi,zi) \
    MOUNDHEIGHT(-1.0 + (float) (xi)*(SQUARESIZE*2.0), \
                -1.0 + (float) (zi)*(SQUARESIZE*2.0))

#define IS_DEFAULT_COLOR(obj) \
    (  (obj->color[0] == DEFAULT_OBJECT_COLOR) \
    && (obj->color[1] == DEFAULT_OBJECT_COLOR) \
    && (obj->color[2] == DEFAULT_OBJECT_COLOR))

#define MOUNDNORMAL(x,z,nx,ny,nz) \
{ \
    nx = FSIN((x)*(M_PI/2.0)); ny = 1.0; nz = FSIN((z)*(M_PI/2.0)); \
}


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


static int mound_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    int whichx,whichz;
    float offsetx,offsetz;
    float new_x,new_y,new_z;

    new_x = (x + 1.0)/2.0;
    new_z = (z + 1.0)/2.0;

    whichx = new_x/SQUARESIZE;
    whichz = new_z/SQUARESIZE;

    offsetx = (new_x - whichx * SQUARESIZE) / SQUARESIZE;
    offsetz = (new_z - whichz * SQUARESIZE) / SQUARESIZE;

#ifdef LEFT_HAND_TRIANGULARIZATION
    if (offsetx + offsetz > 1.0) {
	/* in the far triangle */
	new_y = bi_interp(1.0-offsetx,1.0-offsetz,
	    MOUNDHEIGHT_QUANTIZED(whichx+1,whichz+1),
	    MOUNDHEIGHT_QUANTIZED(whichx+1,whichz),
	    MOUNDHEIGHT_QUANTIZED(whichx,whichz+1));
    }
    else {
	new_y = bi_interp(offsetx,offsetz,
	    MOUNDHEIGHT_QUANTIZED(whichx,whichz),
	    MOUNDHEIGHT_QUANTIZED(whichx,whichz+1),
	    MOUNDHEIGHT_QUANTIZED(whichx+1,whichz));
    }
#else
    if (offsetx > offsetz) {
	new_y = bi_interp(1.0-offsetx,offsetz,
	    MOUNDHEIGHT_QUANTIZED(whichx+1,whichz),
	    MOUNDHEIGHT_QUANTIZED(whichx+1,whichz+1),
	    MOUNDHEIGHT_QUANTIZED(whichx,whichz));
    }
    else {
	new_y = bi_interp(offsetx,1.0-offsetz,
	    MOUNDHEIGHT_QUANTIZED(whichx,whichz+1),
	    MOUNDHEIGHT_QUANTIZED(whichx,whichz),
	    MOUNDHEIGHT_QUANTIZED(whichx+1,whichz+1));
    }
#endif /* LEFT_HAND_TRIANGULARIZATION else */

    /* Ignore the underground parts */
    if (new_y < 0.0) return(FALSE);

    sc->mc_x = x;
    sc->mc_y = new_y;
    sc->mc_z = z;

    MOUNDNORMAL(x,z,sc->mc_normal[0],sc->mc_normal[1],sc->mc_normal[2]);
    sc->mc_normal[0] /= Width;
    sc->mc_normal[1] /= Height;
    sc->mc_normal[2] /= Length;
    NORMALIZE3(sc->mc_normal[0],sc->mc_normal[1],sc->mc_normal[2]);

    return(TRUE);
}


static int mound_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    return(checkbounds_bbox(obj,bbox_mc,sc,mound_surface_chars_xyz));
}


static hwObject high_res_model_default(
    DRIVE_OBJECT *obj)
{
    float mesh[MOUND_MESH_SIZE][MOUND_MESH_SIZE][6];
    float x,z,inc;
    int i,j;
    hwObject curr;

    inc = 2.0/(MOUND_MESH_SIZE-1);

    for (j=0, z= -1.0; j<MOUND_MESH_SIZE; z+=inc,++j) {
	for (i=0, x = -1.0; i<MOUND_MESH_SIZE; x+=inc,++i) {
	    mesh[j][i][0] = x;
	    mesh[j][i][1] = MOUNDHEIGHT(x,z);
	    mesh[j][i][2] = z;
	    random_ground_rgb(&(mesh[j][i][3]),
		&(mesh[j][i][4]), &(mesh[j][i][5]));
	}
    }

    curr = hwMesh->create( hwMesh );
    HW_MODIFY_1I( curr, hwStrGraphN, MOUND_MESH_SIZE );
    HW_MODIFY_1I( curr, hwStrGraphM, MOUND_MESH_SIZE );
    HW_MODIFY_1B( curr, hwStrHasRGB, HW_TRUE );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,MOUND_MESH_SIZE*MOUND_MESH_SIZE*6),
		mesh );
    return curr;
}


static hwObject high_res_model_solidcolor(
    DRIVE_OBJECT *obj)
{
    float mesh[MOUND_MESH_SIZE][MOUND_MESH_SIZE][6];
    float x,z,inc;
    int i,j;
    hwObject curr;

    inc = 2.0/(MOUND_MESH_SIZE-1);

    for (j=0, z= -1.0; j<MOUND_MESH_SIZE; z+=inc,++j) {
	for (i=0, x = -1.0; i<MOUND_MESH_SIZE; x+=inc,++i) {
	    mesh[j][i][0] = x;
	    mesh[j][i][1] = MOUNDHEIGHT(x,z);
	    mesh[j][i][2] = z;
	    MOUNDNORMAL(x,z,mesh[j][i][3],mesh[j][i][4],mesh[j][i][5]);
	    NORMALIZE3(mesh[j][i][3],mesh[j][i][4],mesh[j][i][5]);
	}
    }

    curr = hwMesh->create( hwMesh );
    HW_MODIFY_1I( curr, hwStrGraphN, MOUND_MESH_SIZE );
    HW_MODIFY_1I( curr, hwStrGraphM, MOUND_MESH_SIZE );
    HW_MODIFY_1B( curr, hwStrHasNormals, HW_TRUE );
    HW_MODIFY_3F( curr, hwStrColor, obj->color[0],obj->color[1],obj->color[2]);
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,MOUND_MESH_SIZE*MOUND_MESH_SIZE*6),
		mesh );
    return curr;
}


static void create_mound_graphics(
    DRIVE_OBJECT *obj)
{
    hwObject curr;

    if (IS_DEFAULT_COLOR(obj)) {
	curr = high_res_model_default(obj);
    }
    else {
	curr = high_res_model_solidcolor(obj);
    }
    HW_OBJECT_NAMESET(curr,obj);
    obj->display_list = createHwSegmentFromObj( &curr, 1 );
}


void init_mound_object(
    DRIVE_OBJECT *obj)
{
    static float scalemat[4][4] = IDENTITY4x4;
    MOUND_LIST *ml;

    if (Length == DEFAULT_OBJECT_SIZE) Length = DEFAULT_LENGTH;
    else if (ABS(Length) < EPSILON) Length = EPSILON;
    if (Width  == DEFAULT_OBJECT_SIZE) Width  = DEFAULT_WIDTH;
    else if (ABS(Width) < EPSILON) Width = EPSILON;
    if (Height == DEFAULT_OBJECT_SIZE) Height = DEFAULT_HEIGHT;
    else if (ABS(Height) < EPSILON) Height = EPSILON;

    /* Halve length and width since they run from -1.0 to 1.0 */
    Length /= 2.0;
    Width  /= 2.0;

    obj->num_children = 0;

    if (IS_DEFAULT_COLOR(obj)) {
	if (default_mound_dl == -1) {
	    create_mound_graphics(obj);
	    default_mound_dl = obj->display_list;
	}
	else {
	    obj->display_list = default_mound_dl;
	}
    }
    else {
	ml = mound_list;
	while (ml != NULL) {
	    if (IS_NEAR(ml->r,obj->color[0])
		    && IS_NEAR(ml->g,obj->color[1])
		    && IS_NEAR(ml->b,obj->color[2])
		    && (ml->nameset_bits == obj->nameset_bits)) {
		break;
	    }
	    /* else */
	    ml = ml->next;
	}
	if (ml != NULL) {
	    /* got one already */
	    obj->display_list = ml->dl_number;
	}
	else {
	    /* create one */
	    if ((ml = (MOUND_LIST *) malloc(sizeof(MOUND_LIST))) == NULL) {
		fprintf(stderr,"Out of malloc space!\n");
		return;
	    }
	    ml->r = obj->color[0];
	    ml->g = obj->color[1];
	    ml->b = obj->color[2];
	    ml->nameset_bits = obj->nameset_bits;
	    ml->next = mound_list;
	    create_mound_graphics(obj);
	    ml->dl_number = obj->display_list;
	    mound_list = ml;
	}
    }

    /* Scale to the proper size */
    scalemat[0][0] = Width;
    scalemat[1][1] = Height;
    scalemat[2][2] = Length;
    concat_matrix(scalemat,obj->xform,obj->xform);

    scalemat[0][0] = 1.0/Width;
    scalemat[1][1] = 1.0/Height;
    scalemat[2][2] = 1.0/Length;
    concat_matrix(obj->ixform,scalemat,obj->ixform);

    obj->surface_chars_xyz = mound_surface_chars_xyz;
    obj->surface_chars_bbox = mound_surface_chars_bbox;

    obj->bound_mc[0] = -1.0;
    obj->bound_mc[1] =  0.0;
    obj->bound_mc[2] = -1.0;
    obj->bound_mc[3] =  1.0;
    obj->bound_mc[4] =  1.0;
    obj->bound_mc[5] =  1.0;
    update_wc_bounds(obj);
}
