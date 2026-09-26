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


/* Code Module for the deciduous object */

#include <stdio.h>
#include <math.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"

#ifndef EPSILON
# define EPSILON	(1.0e-6)
#endif /* EPSILON */

#define MIN_RADIUS	(0.01+EPSILON)	/* If smaller, ignore the trunk. */
#define DEFAULT_HEIGHT	(20.0)
#define DEFAULT_WIDTH	(Height*0.75)
#define DEFAULT_RADIUS	(Width*.03)
#define MAX_LATS	8
#define MAX_LONGS	12
#define HI_GRANULARITY	(3.0)
#define LO_GRANULARITY	(10.0)
#define NORMAL_NOISE	0.5

#define Radius			(obj->radius)
#define Height			(obj->size[SIZE_HEIGHT])
#define Length			(obj->size[SIZE_LENGTH])
#define Width			(obj->size[SIZE_WIDTH])
#define Type			(obj->count)
#define Trunkheight		(Height*.25)
#define Leafheight		(Height*.75)

/* types */
#define TREE_ROUND		0
#define TREE_OVAL		1
#define TREE_BULGE_UP		2
#define TREE_BULGE_DOWN		3
#define TREE_CYLINDER		4
#define TREE_CONE		5
#define TREE_ONION		6

#define MESHVAL(lng,lt,which) (*(mesh+((((lng)*lats+(lt))*6)+(which))))


typedef struct _deciduous_list {
    float radius;
    float length,width,height;
    int type;
    float r,g,b;
    unsigned int nameset_bits;
    int dl_number;
    struct _deciduous_list *next;
} DECIDUOUS_LIST;
static DECIDUOUS_LIST *deciduous_list=NULL;


static int deciduous_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = obj->size[SIZE_HEIGHT];
    get_pole_mc_normal(obj,x,y,z,sc->mc_normal);
    return(TRUE);
}


static int deciduous_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = obj->size[SIZE_HEIGHT];
    return(TRUE);
}

/******************************************************************************/

static float outline_oval(
    float y,
    float size1, float size2,
    float *nx, float *ny)
{
    float x;

    x = size1 * FSQRT(.25 - (y-.5)*(y-.5));
    *nx = x;
    *ny = (y-.5)*2.0*size1;
    return(x);
}

static float outline_onion(
    float y,
    float size1, float size2,
    float *nx, float *ny)
{
    *nx = 1.0;
    *ny = -FCOS(M_PI*y);
    return(size1*FSIN(M_PI*y));
}

static float outline_bulge_up(
    float y,
    float size1, float size2,
    float *nx, float *ny)
{
    float k = M_PI*y*y;

    *nx = 1.0;
    *ny = -FCOS(k);
    return(size1*FSIN(k));
}

static float outline_bulge_down(
    float y,
    float size1, float size2,
    float *nx, float *ny)
{
    float k = M_PI*(1.0-y)*(1.0-y);

    *nx = 1.0;
    *ny = FCOS(k);
    return(size1*FSIN(k));
}

static float outline_cone(
    float y,
    float size1, float size2,
    float *nx, float *ny)
{
    *nx = 1.0;
    *ny = size1-size2;
    return(size1 + y*(size2-size1));
}


static void draw_deciduous_mesh(
    DRIVE_OBJECT *obj,
    float granularity)
{
    int longs,lats;
    float *mesh,*fptr,*fptr2;
    float (*outline_routine)(float y,float size1, float size2,
	float *nx, float *ny);
    float size1=0.0,size2=0.0;
    float outline[MAX_LATS][2],pgon[MAX_LONGS+2][6];
    float normal[MAX_LATS][2];
    float x,y,z,yt,dy,dyt;
    float sina,cosa;
    double angle,da;
    float noisexz,noisey;
    int i,j,first,last,facets;
    float nx,ny,nz;
    hwObject curr, oList[10];
    int nObjs = 0;


    if ((lats = Height/granularity+2) < 4) lats = 4;
    else if (lats > MAX_LATS) lats = MAX_LATS;

    if ((longs = MAX(Width,Length)/granularity+2) < 4) longs = 4;
    else if (longs > MAX_LONGS) longs = MAX_LONGS;

    if ((mesh = (float *) malloc(sizeof(float)*lats*longs*6)) == NULL) {
	fprintf(stderr,"Out of malloc space!\n");
	return;
    }

    switch (Type) {
	case TREE_ROUND:
	    size1 = 1.0;
	    outline_routine = outline_oval;
	    break;
	case TREE_OVAL:
	    size1 = Width/Leafheight;
	    outline_routine = outline_oval;
	    break;
	case TREE_BULGE_UP:
	    size1 = Width/Leafheight/2.0;
	    outline_routine = outline_bulge_up;
	    break;
	case TREE_BULGE_DOWN:
	    size1 = Width/Leafheight/2.0;
	    outline_routine = outline_bulge_down;
	    break;
	case TREE_CYLINDER:
	    size1 = Width/Leafheight/2.0;
	    size2 = size1;
	    outline_routine = outline_cone;
	    break;
	case TREE_CONE:
	    size1 = Width/Leafheight/2.0;
	    size2 = Length/Leafheight/2.0;
	    outline_routine = outline_cone;
	    break;
	default:
	case TREE_ONION:
	    size1 = 0.5;
	    outline_routine = outline_onion;
	    break;
    }


    /* create an outline */
    dyt = 1.0/(lats-1);
    dy  = Leafheight/(lats-1);
    i = 0;
    for (y=Trunkheight,yt=0.0; yt<=(1.0+EPSILON); yt+=dyt,y+=dy) {
	outline[i][0] = (*outline_routine)(yt,size1,size2,
	    &(normal[i][0]),&(normal[i][1])) * Width;
	outline[i][1] = y;
	++i;
    }

    /* Now create the mesh */
    da = (2.0*M_PI)/(longs-1);
    fptr = mesh;
    noisexz = MIN(Width,Length)/20.0;
    noisey = Leafheight/20.0;
    for (angle=0.0,i=0; i<longs-1; angle+=da,++i) {
	sina = FSIN(angle);
	cosa = FCOS(angle);
	if (IS_NEAR(outline[0][0],0.0)) {
	    first = 1;
	    *fptr++ =  outline[0][0]*cosa;
	    *fptr++ =  outline[0][1];
	    *fptr++ = -outline[0][0]*sina;
	    nx =  normal[0][0]*cosa;
	    ny =  normal[0][1];
	    nz = -normal[0][0]*sina;
	    NORMALIZE3(nx,ny,nz);
	    *fptr++ = nx; *fptr++ = ny; *fptr++ = nz;
	}
	else first=0;
	if (IS_NEAR(outline[lats-1][0],0.0)) last = lats-1;
	else last = lats;
	for (j=first; j<last; ++j) {
	    *fptr++ =  outline[j][0]*cosa + BOUNDED_FLOATRAND(-noisexz,noisexz);
	    *fptr++ =  outline[j][1]      + BOUNDED_FLOATRAND(-noisey,noisey);
	    *fptr++ = -outline[j][0]*sina + BOUNDED_FLOATRAND(-noisexz,noisexz);
	    nx =  normal[j][0]*cosa
		+ BOUNDED_FLOATRAND(-NORMAL_NOISE,NORMAL_NOISE);
	    ny =  normal[j][1]
		+ BOUNDED_FLOATRAND(-NORMAL_NOISE,NORMAL_NOISE);
	    nz = -normal[j][0]*sina
		+ BOUNDED_FLOATRAND(-NORMAL_NOISE,NORMAL_NOISE);
	    NORMALIZE3(nx,ny,nz);
	    *fptr++ = nx; *fptr++ = ny; *fptr++ = nz;
	}
	if (last != lats) {
	    *fptr++ =  outline[lats-1][0]*cosa;
	    *fptr++ =  outline[lats-1][1];
	    *fptr++ = -outline[lats-1][0]*sina;
	    nx =  normal[lats-1][0]*cosa;
	    ny =  normal[lats-1][1];
	    nz = -normal[lats-1][0]*sina;
	    NORMALIZE3(nx,ny,nz);
	    *fptr++ = nx; *fptr++ = ny; *fptr++ = nz;
	}
    }

    /* copy that last column */
    fptr2 = mesh;
    for (j=0; j<lats; ++j) {
	*fptr++ = *fptr2++;
	*fptr++ = *fptr2++;
	*fptr++ = *fptr2++;
	*fptr++ = *fptr2++;
	*fptr++ = *fptr2++;
	*fptr++ = *fptr2++;
    }

    curr = hwMesh->create( hwMesh );
    HW_MODIFY_1I( curr, hwStrGraphN, longs );
    HW_MODIFY_1I( curr, hwStrGraphM, lats );
    HW_MODIFY_1B( curr, hwStrHasNormals, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_3F(curr, hwStrColor, obj->color[0],obj->color[1],obj->color[2]);
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,longs*lats*6), mesh );
    oList[nObjs++] = curr;

    /* Now do top and bottom. */
    if (!IS_NEAR(outline[lats-1][0],0.0)) {
	/* top */
	fptr = (float *) pgon;
	x = MESHVAL(0,lats-1,0);
	y = MESHVAL(0,lats-1,1);
	z = MESHVAL(0,lats-1,2);
	/* establish normal */
	*fptr++ = x;    *fptr++ = y; *fptr++ = z;
	*fptr++ = x;    *fptr++ = y; *fptr++ = z-.1;
	*fptr++ = x+.1; *fptr++ = y; *fptr++ = z-.1;
	for (i=1; i<longs-1; ++i) {
	    *fptr++ = MESHVAL(i,lats-1,0);
	    *fptr++ = MESHVAL(i,lats-1,1);
	    *fptr++ = MESHVAL(i,lats-1,2);
	}
	curr = hwPolygon->create( hwPolygon );
	HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
	HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
	HW_MODIFY_3F(curr, hwStrColor,
			obj->color[0],obj->color[1],obj->color[2]);
	curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,(longs-1+2)), pgon );
	oList[nObjs++] = curr;
    }

    if (!IS_NEAR(outline[0][0],0.0)) {
	/* bottom */
	fptr = (float *) pgon;
	x = MESHVAL(0,0,0);
	y = MESHVAL(0,0,1);
	z = MESHVAL(0,0,2);
	/* establish normal */
	*fptr++ = x;    *fptr++ = y; *fptr++ = z;
	*fptr++ = x;    *fptr++ = y; *fptr++ = z+.1;
	*fptr++ = x+.1; *fptr++ = y; *fptr++ = z+.1;
	for (i=1; i<longs-1; ++i) {
	    *fptr++ = MESHVAL(i,0,0);
	    *fptr++ = MESHVAL(i,0,1);
	    *fptr++ = MESHVAL(i,0,2);
	}
	curr = hwPolygon->create( hwPolygon );
	HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
	HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
	HW_MODIFY_3F(curr, hwStrColor,
			obj->color[0],obj->color[1],obj->color[2]);
	curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,(longs-1+2)), pgon );
	oList[nObjs++] = curr;
    }

    /* TBD: LOD */
    if (Radius > MIN_RADIUS) {
	if ((facets = Radius+2) < 3) facets = 3;
	else if (facets > 6) facets = 6;

	curr = hwCone->create( hwCone );
	WOOD_HW( curr );
	HW_MODIFY_1I( curr, hwStrGraphN, 2 );
	HW_MODIFY_1I( curr, hwStrGraphM, facets+1 );
	HW_MODIFY_1F( curr, hwStrRadius, Radius );
	HW_MODIFY_1F( curr, hwStrHeight, Trunkheight+1.0 );
	HW_MODIFY_3F( curr, hwStrRotate, 90.0, 0.0, 0.0 );
	oList[nObjs++] = curr;
    }

    curr = hwGroup->create( hwGroup );
    curr->modify( curr, hwStrChildren,
			HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );

    obj->display_list = createHwSegmentFromObj( &curr, 1 );
    free(mesh);
}



static void create_deciduous_graphics(
    DRIVE_OBJECT *obj)
{
    draw_deciduous_mesh( obj, HI_GRANULARITY );
}


void init_deciduous_object(
    DRIVE_OBJECT *obj)
{
    DECIDUOUS_LIST *rl;

    /* Height must be first */
    if (Height == DEFAULT_OBJECT_SIZE)   Height = DEFAULT_HEIGHT;

    Type = INVALID;
    if (obj->subtype[0] != DEFAULT_OBJECT_SUBTYPE0) {
	char subtype[256];
	normalize_string(obj->subtype,subtype);
	if (strcmp(subtype,"round") == 0)		Type = TREE_ROUND;
	else if (strcmp(subtype,"onion") == 0)		Type = TREE_ONION;
	else if (strcmp(subtype,"oval") == 0)		Type = TREE_OVAL;
	else if (strcmp(subtype,"pear") == 0)		Type = TREE_BULGE_UP;
	else if (strcmp(subtype,"top") == 0)		Type = TREE_BULGE_DOWN;
	else if (strcmp(subtype,"cylinder") == 0)	Type = TREE_CYLINDER;
	else if (strcmp(subtype,"cone") == 0)		Type = TREE_CONE;
    }
    if (Type == INVALID) {
	switch (INTRAND(8)) {
	    case 1:
	    case 2:
		Type = TREE_ROUND;
		break;
	    case 3:
	    case 4:
		Type = TREE_OVAL;
		break;
	    case 5:
		Type = TREE_CONE;
		break;
	    case 6:
		Type = TREE_BULGE_UP;
		break;
	    case 7:
		Type = TREE_BULGE_DOWN;
		break;
	    case 8:
		Type = TREE_ONION;
		break;
	}
    }
    switch (Type) {
	case TREE_OVAL:
	    if (Width == DEFAULT_OBJECT_SIZE)  Width  = DEFAULT_WIDTH*0.75;
	    if (Length == DEFAULT_OBJECT_SIZE) Length = Width;
	    break;
	case TREE_CYLINDER:
	    if (Width  == DEFAULT_OBJECT_SIZE) Width = DEFAULT_WIDTH*0.75;
	    Length = Width;
	    break;
	case TREE_CONE:
	    if (Width  == DEFAULT_OBJECT_SIZE) Width = DEFAULT_WIDTH*0.75;
	    if (Length == DEFAULT_OBJECT_SIZE)
		Length = Width*BOUNDED_FLOATRAND(.5,1.5);
	    break;
	default:
	    if (Width  == DEFAULT_OBJECT_SIZE)   Width  = DEFAULT_WIDTH;
	    if (Length == DEFAULT_OBJECT_SIZE)   Length = Width;
	    break;
    }
    if (Radius == DEFAULT_OBJECT_RADIUS) Radius = DEFAULT_RADIUS;
    if ((obj->color[0] == DEFAULT_OBJECT_COLOR)
	    || (obj->color[1] == DEFAULT_OBJECT_COLOR)
	    || (obj->color[2] == DEFAULT_OBJECT_COLOR)) {
	obj->color[0] = BOUNDED_FLOATRAND(0.0,0.5);
	obj->color[1] = BOUNDED_FLOATRAND(0.5,0.9);
	obj->color[2] = BOUNDED_FLOATRAND(0.0,0.4);
    }

    obj->num_children = 0;

    /* See if we've created one like this before... */
    rl = deciduous_list;
    while (rl != NULL) {
	if (IS_NEAR(rl->radius,Radius)
		&& IS_NEAR(rl->width,Width)
		&& IS_NEAR(rl->height,Height)
		&& IS_NEAR(rl->length,Length)
		&& (rl->nameset_bits == obj->nameset_bits)
		&& (rl->type == Type)
		&& (ABS(rl->r - obj->color[0]) < 0.1)
		&& (ABS(rl->g - obj->color[1]) < 0.1)
		&& (ABS(rl->b - obj->color[2]) < 0.1)) {
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
	if ((rl = (DECIDUOUS_LIST *) malloc(sizeof(DECIDUOUS_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}
	rl->radius = Radius;
	rl->width  = Width;
	rl->height = Height;
	rl->length = Length;
	rl->type   = Type;
	rl->r      = obj->color[0];
	rl->g      = obj->color[1];
	rl->b      = obj->color[2];
	rl->nameset_bits = obj->nameset_bits;
    	create_deciduous_graphics(obj);
	rl->dl_number = obj->display_list;
	rl->next   = deciduous_list;
	deciduous_list  = rl;
    }

    if (Radius > MIN_RADIUS) {
	obj->surface_chars_xyz  = deciduous_surface_chars_xyz;
	obj->surface_chars_bbox = deciduous_surface_chars_bbox;
    }

    /* Initial (mc) bounding box values */
    obj->bound_mc[0] = -Radius;
    obj->bound_mc[1] = -Radius;
    obj->bound_mc[2] = -Radius;
    obj->bound_mc[3] = Radius;
    obj->bound_mc[4] = Radius;
    obj->bound_mc[5] = Radius;

    /* Apply the object's xform matrix to the bounding box to put it in
     * world coordinates.
     */
    update_wc_bounds(obj);

    elevate_object_to_terrain_height((SCENE *) obj->scene,obj,FALSE);
}
