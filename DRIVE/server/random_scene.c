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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <sys/param.h>
#include <unistd.h>
#endif
#include <math.h>
#include <time.h>
#include "global.h"
#include "object.h"
#include "scene.h"
#include "connection.h"
#include "message.h"
#include "drive_server.h"
#include "filenames.h"

#define TEST_CODE 1

#define BRIDGE_LENGTH		500.0
#define BRIDGE_HEIGHT		17.0
#define TURN_RADIUS		400.0
#define TURN_OFFSET		(TURN_RADIUS-SCENE_ROAD_WIDTH/2.0)
#define OFFSET_MIN		50.0
#define OFFSET_MAX		100.0
#define HILL_XROAD_LEN		(SCENE_SIZE/(GROUND_MESH_SIZE-1))
#define HILL_XROAD_ROAD_LEN	((SCENE_SIZE-HILL_XROAD_LEN)/2.0)

#define QUADRANT_ROAD_LEFT	(1<<0)
#define QUADRANT_ROAD_RIGHT	(1<<1)
#define QUADRANT_ROAD_BOTTOM	(1<<2)
#define QUADRANT_ROAD_TOP	(1<<3)

#define QUADRANT_SIZE	(SCENE_SIZE/2.0)
#define AREA_MARGIN	10.0
#define QUADRANT_EXTENT	(QUADRANT_SIZE/2.0)
#define DRIVEWAY_OFFSET	30.0

typedef struct _object_block {
    char desc[256];
    int weight;
    boolean_type needs_road;
    float road_offset;
    float block_width;
    struct _object_block *next;
} OBJECT_BLOCK;
static OBJECT_BLOCK *object_block_list = NULL;


SCENE_TYPE scene_type[] = {
    { SCENE_NONRANDOM,
	"nonrandom",
	SCENE_ALL_FLAT|SCENE_ROAD_ALL,
	100 },
    { SCENE_WHATEVER_FITS,
	"whateverfits",
	SCENE_ALL_FLAT|SCENE_ROAD_ALL,
	100 },
    { SCENE_FLAT_NO_ROADS,
	"flatnoroads",
	SCENE_ALL_FLAT,
	400 },
    { SCENE_FLAT_XROAD,
	"flatxroad",
	SCENE_ALL_FLAT|SCENE_ROAD_LEFT|SCENE_ROAD_RIGHT,
	200 },
    { SCENE_FLAT_ZROAD,
	"flatzroad",
	SCENE_ALL_FLAT|SCENE_ROAD_UP|SCENE_ROAD_DOWN,
	200 },
    { SCENE_FLAT_LEFT_T,
	"flatleftt",
	SCENE_ALL_FLAT|SCENE_ROAD_UP|SCENE_ROAD_DOWN|SCENE_ROAD_LEFT,
	100 },
    { SCENE_FLAT_RIGHT_T,
	"flatrightt",
	SCENE_ALL_FLAT|SCENE_ROAD_UP|SCENE_ROAD_DOWN|SCENE_ROAD_RIGHT,
	100 },
    { SCENE_FLAT_DOWN_T,
	"flatdownt",
	SCENE_ALL_FLAT|SCENE_ROAD_LEFT|SCENE_ROAD_RIGHT|SCENE_ROAD_DOWN,
	100 },
    { SCENE_FLAT_UP_T,
	"flatupt",
	SCENE_ALL_FLAT|SCENE_ROAD_LEFT|SCENE_ROAD_RIGHT|SCENE_ROAD_UP,
	100 },
    { SCENE_FLAT_CROSSROADS,
	"flatcrossroads",
	SCENE_ALL_FLAT|SCENE_ROAD_ALL,
	200 },
    { SCENE_FLAT_OVERPASS,
	"flatoverpass",
	SCENE_ALL_FLAT|SCENE_ROAD_ALL,
	200 },
    { SCENE_FLAT_TURN_RIGHT_DOWN,
	"flatturnrightdown",
	SCENE_ALL_FLAT|SCENE_ROAD_RIGHT|SCENE_ROAD_DOWN,
	100 },
    { SCENE_FLAT_TURN_RIGHT_UP,
	"flatturnrightup",
	SCENE_ALL_FLAT|SCENE_ROAD_RIGHT|SCENE_ROAD_UP,
	100 },
    { SCENE_FLAT_TURN_LEFT_DOWN,
	"flatturnleftdown",
	SCENE_ALL_FLAT|SCENE_ROAD_LEFT|SCENE_ROAD_DOWN,
	100 },
    { SCENE_FLAT_TURN_LEFT_UP,
	"flatturnleftup",
	SCENE_ALL_FLAT|SCENE_ROAD_LEFT|SCENE_ROAD_UP,
	100 },
    { SCENE_FLAT_XHIGHWAY,
	"flatxhighway",
	SCENE_ALL_FLAT|SCENE_ROAD_LEFT|SCENE_ROAD_RIGHT,
	0 },
    { SCENE_FLAT_ZHIGHWAY,
	"flatzhighway",
	SCENE_ALL_FLAT|SCENE_ROAD_UP|SCENE_ROAD_DOWN,
	0 },
    { SCENE_FLAT_XHIGHWAY_OVERPASS,
	"flatxhighwayoverpass",
	SCENE_ALL_FLAT|SCENE_ROAD_UP|SCENE_ROAD_DOWN,
	0 },
    { SCENE_FLAT_ZHIGHWAY_OVERPASS,
	"flatzhighwayoverpass",
	SCENE_ALL_FLAT|SCENE_ROAD_UP|SCENE_ROAD_DOWN,
	0 },
    { SCENE_FLAT_RANDOM_TOWN,
	"",
	SCENE_ALL_FLAT|SCENE_ROAD_ALL,
	200 },
    { SCENE_FLAT_VILLAGE,
	"village",
	SCENE_ALL_FLAT|SCENE_ROAD_ALL,
	0 },
    { SCENE_FLAT_TOWN,
	"town",
	SCENE_ALL_FLAT|SCENE_ROAD_ALL,
	0 },
    { SCENE_FLAT_CITY,
	"city",
	SCENE_ALL_FLAT|SCENE_ROAD_ALL,
	0 },
    { SCENE_HILL_NO_ROADS,
	"hillnoroads",
	0,
	400 },
    { SCENE_HILL_XROAD,
	"hillxroad",
	SCENE_ROAD_LEFT|SCENE_ROAD_RIGHT,
	200 },
    { SCENE_HILL_ZROAD,
	"hillzroad",
	SCENE_ROAD_UP|SCENE_ROAD_DOWN,
	200 },
    { SCENE_HILL_CROSSROADS,
	"hillcrossroads",
	SCENE_ROAD_ALL,
	50 },

    /* This one must be last */
    { INVALID,
	"",
	INVALID,
	0 }
};


DRIVE_OBJECT *add_object_to_scene(
    SCENE *scene,
    char *obj_name, char *subtype, char *label,
    float yrot,
    float x, float y, float z,
    float length, float width, float height,
    float radius, float angle, float spacing,
    int count)
{
    DRIVE_OBJECT *obj;

    if ((obj = (DRIVE_OBJECT *) malloc(sizeof(DRIVE_OBJECT))) == NULL) {
	fprintf(stderr,"Out of malloc space!\n");
	return(NULL);
    }
    obj->idptr = find_object(INVALID,obj_name);
    if (obj->idptr->number == INVALID) {
	free(obj);
	return(NULL);
    }
    strcpy(obj->subtype,subtype);
    strcpy(obj->label,label);
    obj->xform[0][0] = obj->xform[2][2] = FCOS(yrot);
    obj->xform[0][2] = FSIN(yrot); obj->xform[2][0] = -obj->xform[0][2];
    obj->xform[1][1] = obj->xform[3][3] = 1.0;
    obj->xform[3][0] = x + scene->xscene * SCENE_SIZE;
    obj->xform[3][1] = y;
    obj->xform[3][2] = z + scene->zscene * SCENE_SIZE;
    obj->xform[0][1] = obj->xform[0][3] = 
	obj->xform[1][0] = obj->xform[1][2] = obj->xform[1][3] = 
	obj->xform[2][1] = obj->xform[2][3] = 0.0;
    obj->size[SIZE_LENGTH] = length;
    obj->size[SIZE_HEIGHT] = height;
    obj->size[SIZE_WIDTH]  = width;
    obj->radius            = radius;
    obj->nameset_bits      = ALL_TIMED_NAMESETS;
    obj->angle             = angle;
    obj->scene             = (void *) scene;
    obj->spacing	   = spacing;
    obj->count             = count;
    obj->connection        = 0;
    obj->color[0] = obj->color[1] = obj->color[2] = DEFAULT_OBJECT_COLOR;
    complete_object(&(scene->object_head),obj);

    return(obj);
}


void add_random_tree(
    SCENE *scene,
    float x, float z)
{
    switch (INTRAND(5)) {
	case 1:
	    add_object_to_scene(scene,"conifer","","",FLOATRAND(2.0*M_PI),
		x,0.0,z,
		0.0,0.0,RANDOM_TREE_HEIGHT(8.0,40.0),
		0.0,0.0,0.0,0);
	    break;
	case 2:
	    add_object_to_scene(scene,"tree","","",FLOATRAND(2.0*M_PI),
		x,0.0,z,
		0.0,0.0,RANDOM_TREE_HEIGHT(8.0,40.0),
		0.0,0.0,0.0,0);
	    break;
	case 3:
	    add_object_to_scene(scene,"bush","","", FLOATRAND(2.0*M_PI),
		x,0.0,z,
		0.0,0.0,RANDOM_BUSH_HEIGHT(3.0,15.0),
		0.0,0.0,0.0,0);
	    break;
	default:
	    add_object_to_scene(scene,"deciduous","","",FLOATRAND(2.0*M_PI),
		x,0.0,z,
		0.0,0.0,RANDOM_TREE_HEIGHT(8.0,40.0),
		0.0,0.0,0.0,0);
	    break;
    }
}


static void add_construct_to_scene(
    SCENE *scene,
    char *obj_name, char *subtype, char *label,
    float yrot,
    float x, float y, float z,
    float length, float width, float height,
    float radius, float angle, float spacing,
    int count)
{
    static float identity[4][4] = {
	{ 1.0, 0.0, 0.0, 0.0 },
	{ 0.0, 1.0, 0.0, 0.0 },
	{ 0.0, 0.0, 1.0, 0.0 },
	{ 0.0, 0.0, 0.0, 1.0 }
    };
    float mat[4][4];
    OBJECT_SPECS specs;

    reset_object_specs(&specs);

    strcpy(specs.subtype,subtype);
    strcpy(specs.label,label);
    specs.xrot = 0.0; specs.yrot = RADIANS_TO_DEGREES(yrot); specs.zrot = 0.0;
    specs.size[SIZE_LENGTH] = length;
    specs.size[SIZE_WIDTH]  = width;
    specs.size[SIZE_HEIGHT] = height;
    specs.angle = angle;
    specs.radius = radius;
    specs.spacing = spacing;
    specs.count = count;
    memcpy(specs.mat,identity,sizeof(float)*16);

    memcpy(mat,identity,sizeof(float)*16);
    mat[3][0] = x + scene->xscene * SCENE_SIZE;
    mat[3][1] = y;
    mat[3][2] = z + scene->zscene * SCENE_SIZE;

    get_construct(scene,obj_name,&specs,mat);
}


static void adjust_coords(
    float x, float z,
    float orientation,
    float xmin, float zmin,
    float xsize, float zsize,
    float *rx, float *rz)
{
    if (orientation == 0.0) {
	*rx =  x;  *rz =  z;
    }
    else if (IS_NEAR(orientation,(M_PI/2.0))) {
	*rx = -z;  *rz =  x;
    }
    else if (IS_NEAR(orientation,M_PI)) {
	*rx = -x;  *rz = -z;
    }
    else {
	*rx =  z;  *rz = -x;
    }

    *rx += (xsize/2.0) + xmin;
    *rz += (zsize/2.0) + zmin;
}


static void add_house(
    SCENE *scene,
    float orientation,
    float xhouse, float zhouse,
    float xmin, float zmin,
    float xsize, float zsize,
    boolean_type is_farm)
{
    float lawnwidth,lawnlength;
    float drivelength;
    float x,z, xr,zr;
    int i,n;
    boolean_type good;

    /* house */
    adjust_coords(xhouse,zhouse, orientation, xmin,zmin, xsize,zsize, &xr,&zr);
    add_object_to_scene(scene,"house","","",orientation,
	xr,0.0,zr,
	0.0,0.0,0.0,
	0.0,0.0,0.0,0);

    /* lawn */
    lawnwidth  = ROUND_TO_INCREMENT2(100.0,300.0,50.0);
    lawnlength = ROUND_TO_INCREMENT2(100.0,300.0,50.0);
    /* make sure it doesn't spill over the edges */
    if (xr - lawnwidth/2.0 < xmin)
	lawnwidth = (xr - xmin) * 2.0;
    if (xr + lawnwidth/2.0 > xmin+xsize)
	lawnwidth = ((xmin+xsize) - xr) * 2.0;
    if (zr - lawnlength/2.0 < zmin)
	lawnlength = (zr - zmin) * 2.0;
    if (zr + lawnlength/2.0 > zmin+zsize)
	lawnlength = ((zmin+zsize) - zr) * 2.0;
    if (scene->flags & SCENE_ALL_FLAT) {
	add_object_to_scene(scene,"lawn","","",0.0,
	    xr,0.0,zr,
	    lawnlength,lawnwidth,0.0,
	    0.0,0.0,0.0,0);
    }

    /* Adjust lawnsizes for later use when placing trees and such */
    if (IS_NEAR(ABS(orientation),(M_PI/2.0))) FLOATSWAP(lawnlength,lawnwidth);

    /* driveway */
    /* Use the xr and zr of the house */
    if (orientation == 0.0) {
	drivelength = zr - zmin;
    }
    else if (IS_NEAR(orientation,(M_PI/2.0))) {
	drivelength = (xmin+xsize) - xr;
    }
    else if (IS_NEAR(orientation,M_PI)) {
	drivelength = (zmin+xsize) - zr;
    }
    else {
	drivelength = xr - xmin;
    }
    drivelength += 0.1;
    adjust_coords(xhouse+DRIVEWAY_OFFSET,zhouse-drivelength,
	orientation, xmin,zmin, xsize,zsize, &xr,&zr);
    if (scene->flags & SCENE_ALL_FLAT) {
	add_object_to_scene(scene,"road","","",orientation,
	    xr,0.1,zr,
	    drivelength,SCENE_ROAD_WIDTH/2.0,0.0,
	    0.0,0.0,0.0,0);
    }
    else {
	add_object_to_scene(scene,"hillroad","","",orientation,
	    xr,0.1,zr,
	    drivelength,SCENE_ROAD_WIDTH/2.0,0.0,
	    0.0,0.0,0.0,0);
    }

    /* mailbox */	
    adjust_coords(xhouse+DRIVEWAY_OFFSET+SCENE_ROAD_WIDTH/3.0,
	zhouse-drivelength+3.0, orientation, xmin,zmin, xsize,zsize, &xr,&zr);
    add_object_to_scene(scene,"mailbox","","",orientation,
	xr,0.0,zr,
	0.0,0.0,0.0,
	0.0,0.0,0.0,0);

    if (is_farm) {
	float xsign;
	/* barn and silo */
	if (xhouse < 0.0) xsign = 1.0;
	else xsign = -1.0;

	adjust_coords(xhouse+xsign*(lawnwidth/2.0+30.0),zhouse+40.0,
	    orientation, xmin,zmin, xsize,zsize, &xr,&zr);
	add_object_to_scene(scene,"barn","","",orientation,
	    xr,0.0,zr,
	    0.0,0.0,0.0,
	    0.0,0.0,0.0,0);

	adjust_coords(xhouse+xsign*(lawnwidth/2.0), zhouse+80.0,
	    orientation, xmin,zmin, xsize,zsize, &xr,&zr);
	add_object_to_scene(scene,"silo","","",orientation,
	    xr,0.0,zr,
	    0.0,0.0,0.0,
	    0.0,0.0,0.0,0);
    }

    /* a few random trees */
    n = ZINTRAND(4) + ZINTRAND(4);
    for (i=0; i<n; ++i) {
	do {
	    x = BOUNDED_FLOATRAND(-lawnwidth/2.0+5.0,lawnwidth/2.0-5.0);
	    z = BOUNDED_FLOATRAND(-lawnlength/2.0+5.0,lawnlength/2.0-5.0);
	    /* check the location */
	    good = TRUE;
	    if ((ABS(x) < 20.0) && (ABS(z) < 20.0)) {
		/* Too close to house */
		good = FALSE;
	    }
	    else if (ABS(x - DRIVEWAY_OFFSET) < SCENE_ROAD_WIDTH) {
		/* Too close to driveway */
		good = FALSE;
	    }
	} while (!good);

	adjust_coords(x+xhouse,z+zhouse,
	    orientation, xmin,zmin, xsize,zsize, &xr,&zr);
	add_random_tree(scene,xr,zr);
    }
}


static void add_business(
    SCENE *scene,
    float orientation,
    float xhouse, float zhouse,
    float xmin, float zmin,
    float xsize, float zsize)
{
    float bwidth,blength;
    float lotw,lotl,lotx,lotz;
    float drivelength;
    float x,z, xr,zr, w, l;
    int i,n;
    boolean_type good;

    /* building */
    adjust_coords(xhouse,zhouse, orientation, xmin,zmin, xsize,zsize, &xr,&zr);
    bwidth  = ROUND_SKYSCRAPER_SIZE(BOUNDED_FLOATRAND(50.0,150.0));
    blength = ROUND_SKYSCRAPER_SIZE(BOUNDED_FLOATRAND(50.0,150.0));
    if (xr - bwidth/2.0 < xmin)
	bwidth = (xr - xmin) * 2.0;
    if (xr + bwidth/2.0 > xmin+xsize)
	bwidth = ((xmin+xsize) - xr) * 2.0;
    if (zr - blength/2.0 < zmin)
	blength = (zr - zmin) * 2.0;
    if (zr + blength/2.0 > zmin+zsize)
	blength = ((zmin+zsize) - zr) * 2.0;
    add_object_to_scene(scene,"skyscraper","","",0.0,
	xr-bwidth/2.0,0.0,zr-blength/2.0,
	bwidth,blength,RANDOM_SKYSCRAPER_HEIGHT(10.0,30.0),
	0.0,0.0,0.0,0);

    /* driveway */
    if (orientation == 0.0) {
	drivelength = zr - zmin;
    }
    else if (IS_NEAR(orientation,(M_PI/2.0))) {
	drivelength = (xmin+xsize) - xr;
    }
    else if (IS_NEAR(orientation,M_PI)) {
	drivelength = (zmin+xsize) - zr;
    }
    else {
	drivelength = xr - xmin;
    }
    drivelength += 0.1;
    adjust_coords(xhouse, zhouse-drivelength,
	orientation, xmin,zmin, xsize,zsize, &xr,&zr);
    if (scene->flags & SCENE_ALL_FLAT) {
	add_object_to_scene(scene,"road","","",orientation,
	    xr,0.1,zr,
	    drivelength,SCENE_ROAD_WIDTH/2.0,0.0,
	    0.0,0.0,0.0,0);
    }
    else {
	add_object_to_scene(scene,"hillroad","","",orientation,
	    xr,0.1,zr,
	    drivelength,SCENE_ROAD_WIDTH/2.0,0.0,
	    0.0,0.0,0.0,0);
    }

    /* parking lot */
    lotl = (zhouse - blength/2.0) - (-zsize/2.0) - 10.0;
    lotw = 100.0 + FLOATRAND(50.0);
    if (lotl > 150.0) lotl = 150.0;
    if (lotl > 30.0) {
	lotl = ROUND_TO_INCREMENT(lotl,20.0);
	lotw = ROUND_TO_INCREMENT(lotw,20.0);
	lotx = xhouse;
	lotz = (zhouse - blength/2.0) - 5.0 - lotl/2.0;
	adjust_coords(lotx,lotz, orientation, xmin,zmin, xsize,zsize, &xr,&zr);
	w = lotw; l = lotl;
	if (IS_NEAR(ABS(orientation),(M_PI/2.0))) FLOATSWAP(w,l);
	if (scene->flags & SCENE_ALL_FLAT) {
	    add_object_to_scene(scene,"parking lot","","",orientation,
		xr,0.0,zr,
		l,w,0.0,
		0.0,0.0,0.0,0);
	}
    }

    /* A few random trees */
    adjust_coords(xhouse,zhouse, orientation, xmin,zmin, xsize,zsize, &xr,&zr);
    n = INTRAND(3) + INTRAND(3);
    for (i=0; i<n; ++i) {
	int count;
	count = 0;
	do {
	    good = TRUE;
	    x = BOUNDED_FLOATRAND(xr - 100.0, xr+100.0);
	    z = BOUNDED_FLOATRAND(zr - 100.0, zr+100.0);
	    if (ABS(x-xr) < SCENE_ROAD_WIDTH) good = FALSE;
	    else if (ABS(z-zr) < SCENE_ROAD_WIDTH) good = FALSE;
	    else if (x < xmin) good = FALSE;
	    else if (x > (xmin+xsize)) good = FALSE;
	    else if (z < zmin) good = FALSE;
	    else if (z > (zmin+zsize)) good = FALSE;
	    else if ((x > (xr - bwidth/2.0))
		    && (x < (xr + bwidth/2.0))
		    && (z > (zr - blength/2.0))
		    && (z < (zr + blength/2.0)))
		good = FALSE;
	    if (++count > 10) break;
	} while (!good);
	if (good) add_random_tree(scene,x,z);
    }
}



static void add_oriented_construct(
    SCENE *scene,
    float orientation,
    char *construct,
    float offset,
    float construct_width,
    float xmin, float zmin,
    float xsize, float zsize)
{
    float x,z;

    if (!(scene->flags & SCENE_ALL_FLAT)) return;

    if (orientation == 0.0) {
	/* bottom */
	if (xsize < construct_width) x = xmin+xsize/2.0;
	else x = xmin + FLOATRAND(xsize - construct_width)
	    + construct_width/2.0;
	add_construct_to_scene(scene,construct,"","",orientation,
	    x, 0.0, zmin+offset,
	    0.0,0.0,0.0,
	    0.0,0.0,0.0,0);
    }
    else if (IS_NEAR(orientation,(M_PI/2.0))) {
	/* right */
	if (zsize < construct_width) z = zmin+zsize/2.0;
	else z = zmin + FLOATRAND(zsize - construct_width)
	    + construct_width/2.0;
	add_construct_to_scene(scene,construct,"","",orientation,
	    xmin+xsize-offset, 0.0, z,
	    0.0,0.0,0.0,
	    0.0,0.0,0.0,0);
    }
    else if (IS_NEAR(orientation,M_PI)) {
	/* top */
	if (xsize < construct_width) x = xmin+xsize/2.0;
	else x = xmin + FLOATRAND(xsize - construct_width)
	    + construct_width/2.0;
	add_construct_to_scene(scene,construct,"","",orientation,
	    x, 0.0, zmin+zsize-offset,
	    0.0,0.0,0.0,
	    0.0,0.0,0.0,0);
    }
    else {
	/* left */
	if (zsize < construct_width) z = zmin+zsize/2.0;
	else z = zmin + FLOATRAND(zsize - construct_width)
	    + construct_width/2.0;
	add_construct_to_scene(scene,construct,"","",orientation,
	    xmin+offset, 0.0, z,
	    0.0,0.0,0.0,
	    0.0,0.0,0.0,0);
    }
}



static void read_object_blocks(
    void)
{
    FILE *file;
    char input[4096],desc[256],road[256];
    int weight,count;
    float offset,width;
    OBJECT_BLOCK *block;

    if ((file = fopen(object_block_filename,"r")) == NULL) {
	fprintf(stderr,"Cannot find file %s!\n",object_block_filename);
	return;
    }

    count = 0;
    while (fgets(input,sizeof(input)-1,file) != NULL) {
	if (input[0] == '#') continue;
	if (sscanf(input,"%s %d %s %f %f",
		desc,&weight,road,&offset,&width) == 5) {
	    if ((block = (OBJECT_BLOCK *) malloc(sizeof(OBJECT_BLOCK)))
		    == NULL) {
		fprintf(stderr,"Out of malloc space!\n");
		fclose(file);
		return;
	    }

	    strcpy(block->desc,desc);
	    block->weight = weight;
	    if ((road[0] == 'T') || (road[0] == 't')
		    || (road[0] == 'Y') || (road[0] == 'y')
		    || (road[0] == '1')) {
		block->needs_road = TRUE;
	    }
	    else {
		block->needs_road = FALSE;
	    }
	    block->road_offset = offset;
	    block->block_width = width;
	    block->next = object_block_list;
	    object_block_list = block;
	    ++count;
	}
    }

    fclose(file);
    if (debug) printf("%d Object Blocks read.\n",count);
}


static void populate_area(
    SCENE *scene,
    unsigned int road_flags,
    float xmin, float zmin,
    float xsize, float zsize)
{
    int sum,choice,i,num;
    float orientation = 0.0;
    float x,z;
    float xhouse,zhouse;
    OBJECT_BLOCK *block;


    if (object_block_list == NULL) read_object_blocks();

    sum = 0;
    block = object_block_list;
    while (block != NULL) {
	if ((road_flags) || (!(block->needs_road))) {
	    sum += block->weight;
	}
	block = block->next;
    }
    choice = ZINTRAND(sum);

    block = object_block_list;
    while (block != NULL) {
	if ((road_flags) || (!(block->needs_road))) {
	    if ((choice -= block->weight) < 0) break;
	}
	block = block->next;
    }
    if (block == NULL) return;

#ifdef TEST_CODE
    /* Testing mechanism */
    if (getenv("DRIVE_QUADRANT_TYPE")) {
	block = object_block_list;
	while (block != NULL) {
	    if (strcmp(block->desc,getenv("DRIVE_QUADRANT_TYPE")) == 0) break;
	    block = block->next;
	}
	if (block == NULL) return;
    }
#endif /* TEST_CODE */

    if (block->needs_road) {
	zhouse = BOUNDED_FLOATRAND(-(MIN(xsize,zsize)/2.0-AREA_MARGIN),0.0);
	xhouse = BOUNDED_FLOATRAND(zhouse,-zhouse);

	orientation = 0.0;
	sum = 0;
	if (road_flags & QUADRANT_ROAD_LEFT)   ++sum;
	if (road_flags & QUADRANT_ROAD_RIGHT)  ++sum;
	if (road_flags & QUADRANT_ROAD_TOP)    ++sum;
	if (road_flags & QUADRANT_ROAD_BOTTOM) ++sum;
	if (sum <= 1) choice = 1;
	else choice = INTRAND(sum);
	if (road_flags & QUADRANT_ROAD_LEFT) {
	    if (--choice == 0) orientation = (-M_PI/2.0);
	}
	if (road_flags & QUADRANT_ROAD_RIGHT) {
	    if (--choice == 0) orientation = (M_PI/2.0);
	}
	if (road_flags & QUADRANT_ROAD_BOTTOM) {
	    if (--choice == 0) orientation = 0.0;
	}
	if (road_flags & QUADRANT_ROAD_TOP) {
	    if (--choice == 0) orientation = M_PI;
	}
    }

    if (strcmp(block->desc,"empty") == 0) {
	/* do nothing */
    }
    else if (strcmp(block->desc,"grass") == 0) {
	if (scene->flags & SCENE_ALL_FLAT) {
	    add_object_to_scene(scene,"grass","","",0.0,
		xmin+xsize/2.0,0.0,zmin,
		zsize,xsize,2.0,
		0.0, 0.0, 0.0, 50 + INTRAND(300));
	}
	num = INTRAND(10);
	for (i=0; i<num; ++i) {
	    x = BOUNDED_FLOATRAND(xmin+5.0,xmin+xsize-5.0);
	    z = BOUNDED_FLOATRAND(zmin+5.0,zmin+zsize-5.0);
	    add_object_to_scene(scene,"bush","","",FLOATRAND(2.0*M_PI),
		x,0.0,z,
		0.0,0.0,RANDOM_BUSH_HEIGHT(2.0,6.0),
		0.0,0.0,0.0,0);
	}
    }
    else if (strcmp(block->desc,"farm") == 0) {
	add_house(scene,orientation,xhouse,zhouse,
	    xmin,zmin,xsize,zsize,TRUE);
    }
    else if (strcmp(block->desc,"house") == 0) {
	add_house(scene,orientation,xhouse,zhouse,
	    xmin,zmin,xsize,zsize,FALSE);
    }
    else if (strcmp(block->desc,"woods") == 0) {
	float txmin,txmax,tzmin,tzmax;

	txmin = BOUNDED_FLOATRAND(xmin+10.0,  xmin+xsize-60.0);
	txmax = BOUNDED_FLOATRAND(txmin+50.0, xmin+xsize-10.0);
	if (txmax - txmin > 400.0) txmax = txmin + 400.0;

	tzmin = BOUNDED_FLOATRAND(zmin+10.0,  zmin+zsize-60.0);
	tzmax = BOUNDED_FLOATRAND(tzmin+50.0, zmin+zsize-10.0);
	if (tzmax - tzmin > 400.0) tzmax = tzmin + 400.0;

	num = INTRAND(6) + INTRAND(6) + 2;
	for (i=0; i<num; ++i) {
	    x = BOUNDED_FLOATRAND(txmin,txmax);
	    z = BOUNDED_FLOATRAND(tzmin,tzmax);
	    add_random_tree(scene,x,z);
	}
    }
    else if (strcmp(block->desc,"mounds") == 0) {
	if (scene->flags & SCENE_ALL_FLAT) {
	    float maxhillsize = MIN(xsize,zsize)/2.0 - 50.0;
	    num = INTRAND(6);
	    for (i=0; i<num; ++i) {
		x = xmin + BOUNDED_FLOATRAND(xsize/4,xsize*3/4);
		z = zmin + BOUNDED_FLOATRAND(zsize/4,zsize*3/4);
		add_object_to_scene(scene,"mound","","",FLOATRAND(2.0*M_PI),
		    x,0.0,z,
		    BOUNDED_FLOATRAND(50.0,maxhillsize),
			BOUNDED_FLOATRAND(50.0,maxhillsize),
			BOUNDED_FLOATRAND(5.0,30.0),
		    0.0,0.0,0.0,0);
	    }
	}
    }
    else if (strcmp(block->desc,"business") == 0) {
	add_business(scene,orientation,xhouse,zhouse,
	    xmin,zmin,xsize,zsize);
    }
    else if (strcmp(block->desc,"teleline") == 0) {
	if (scene->flags & SCENE_ALL_FLAT) {
	    if (orientation == 0.0) {
		/* bottom */
		add_object_to_scene(scene,"telephone line","","",-M_PI/2.0,
		    xmin+5.0,0.0,zmin+5.0,
		    xsize-10.0,0.0,0.0,
		    0.0,0.0,0.0,0);
	    }
	    else if (IS_NEAR(orientation,(M_PI/2.0))) {
		/* right */
		add_object_to_scene(scene,"telephone line","","",M_PI,
		    xmin+xsize-5.0,0.0,zmin+zsize-5.0,
		    zsize-10.0,0.0,0.0,
		    0.0,0.0,0.0,0);
	    }
	    else if (IS_NEAR(orientation,M_PI)) {
		/* top */
		add_object_to_scene(scene,"telephone line","","",M_PI/2.0,
		    xmin+xsize-5.0,0.0,zmin+zsize-5.0,
		    xsize-10.0,0.0,0.0,
		    0.0,0.0,0.0,0);
	    }
	    else {
		/* left */
		add_object_to_scene(scene,"telephone line","","",0.0,
		    xmin+5.0,0.0,zmin+5.0,
		    xsize-10.0,0.0,0.0,
		    0.0,0.0,0.0,0);
	    }
	}
    }
    else {
	/* construct */
	add_oriented_construct(scene,orientation,block->desc,
	    block->road_offset,block->block_width,
	    xmin,zmin,xsize,zsize);
    }
}



#define ELIMINATE_SCENES_WITH(flag)	 				\
{   int _j;								\
    for (_j=0; _j<NUM_SCENES; ++_j) {					\
	if (scene_type[_j].flags & (flag)) scene_ok[_j] = FALSE;	\
    }									\
}

#define ELIMINATE_SCENES_WITHOUT(flag) 					\
{   int _j;								\
    for (_j=0; _j<NUM_SCENES; ++_j) {					\
	if (!(scene_type[_j].flags & (flag))) scene_ok[_j] = FALSE;	\
    }									\
}



static void generate_random_objects(
    SCENE *scene,
    int type)
{
    int scene_ok[NUM_SCENES];
    int weightsum, weight;
    boolean_type add_detail = TRUE;
    int i;


    if (type == SCENE_WHATEVER_FITS) {
	/* Need to look at my neighbors and decide on an alternative. */
	for (i=0; i<NUM_SCENES; ++i) {
	    scene_ok[i] = TRUE;
	}
	scene_ok[SCENE_NONRANDOM] = FALSE;
	scene_ok[SCENE_WHATEVER_FITS] = FALSE;

	if (scene->left != NULL) {
	    if (scene->left->flags & SCENE_ROAD_RIGHT) {
		ELIMINATE_SCENES_WITHOUT(SCENE_ROAD_LEFT);
	    }
	    else {
		ELIMINATE_SCENES_WITH(SCENE_ROAD_LEFT);
	    }

	    if (!(scene->left->flags & (SCENE_RIGHT_FLAT|SCENE_ALL_FLAT))) {
		ELIMINATE_SCENES_WITH(SCENE_ALL_FLAT);
	    }
	}
	if (scene->right != NULL) {
	    if (scene->right->flags & SCENE_ROAD_LEFT) {
		ELIMINATE_SCENES_WITHOUT(SCENE_ROAD_RIGHT);
	    }
	    else {
		ELIMINATE_SCENES_WITH(SCENE_ROAD_RIGHT);
	    }

	    if (!(scene->right->flags & (SCENE_LEFT_FLAT|SCENE_ALL_FLAT))) {
		ELIMINATE_SCENES_WITH(SCENE_ALL_FLAT);
	    }
	}
	if (scene->up != NULL) {
	    if (scene->up->flags & SCENE_ROAD_DOWN) {
		ELIMINATE_SCENES_WITHOUT(SCENE_ROAD_UP);
	    }
	    else {
		ELIMINATE_SCENES_WITH(SCENE_ROAD_UP);
	    }

	    if (!(scene->up->flags & (SCENE_DOWN_FLAT|SCENE_ALL_FLAT))) {
		ELIMINATE_SCENES_WITH(SCENE_ALL_FLAT);
	    }
	}
	if (scene->down != NULL) {
	    if (scene->down->flags & SCENE_ROAD_UP) {
		ELIMINATE_SCENES_WITHOUT(SCENE_ROAD_DOWN);
	    }
	    else {
		ELIMINATE_SCENES_WITH(SCENE_ROAD_DOWN);
	    }

	    if (!(scene->down->flags & (SCENE_UP_FLAT|SCENE_ALL_FLAT))) {
		ELIMINATE_SCENES_WITH(SCENE_ALL_FLAT);
	    }
	}

	/* Add up the weights of the ones left */
	weightsum = 0;
	for (i=0; i<NUM_SCENES; ++i) {
	    if (scene_ok[i]) weightsum += scene_type[i].weight;
	}

	if (weightsum <= 0) {
	    i = SCENE_FLAT_CROSSROADS;
	}
	else {
	    /* Choose one at random */
	    weight = INTRAND(weightsum);
	    for (i=0; i<NUM_SCENES; ++i) {
		if (scene_ok[i]) {
		    if ((weight -= scene_type[i].weight) <= 0) break;
		}
	    }
	}

#ifdef TEST_CODE
	if (getenv("DRIVE_SCENE_TYPE") != NULL) {
	    i = atoi(getenv("DRIVE_SCENE_TYPE"));
	}
#endif /* TEST_CODE */

	/* Now that we've decided, call this routine with the real value. */
	generate_random_objects(scene,i);
	return;
    }
    /* else */


    scene->type  = type;
    scene->flags = scene_type[type].flags;

    /* The hill has to be added first so other things can be laid on top. */
    if (!(scene->flags & SCENE_ALL_FLAT)) {
	add_object_to_scene(scene,"hill","","",0.0,
	    0.0,0.0,0.0,
	    SCENE_SIZE,SCENE_SIZE,0.0,
	    0.0,0.0,0.0,0);
    }


    switch (type) {
	case SCENE_NONRANDOM:
	case SCENE_WHATEVER_FITS:
	    add_detail = FALSE;
	    /* shouldn't hit this case. */
	    break;

	case SCENE_FLAT_NO_ROADS:
	    /* Will do ground later */
	    add_object_to_scene(scene,"grass","","",0.0,
		-SCENE_SIZE/2.0, 0.0, -SCENE_SIZE/2.0,
		SCENE_SIZE,SCENE_SIZE,2.0,
		0.0, 0.0, 0.0, 500 );
	    break;

	case SCENE_HILL_NO_ROADS:
	    /* Already added the hill. */
	    break;
	
	case SCENE_FLAT_RANDOM_TOWN:
	    add_detail = FALSE;
	    generate_town(scene,0);
	    break;

	case SCENE_FLAT_VILLAGE:
	    add_detail = FALSE;
	    generate_town(scene,CITY_TYPE_VILLAGE);
	    break;

	case SCENE_FLAT_TOWN:
	    add_detail = FALSE;
	    generate_town(scene,CITY_TYPE_TOWN);
	    break;

	case SCENE_FLAT_CITY:
	    add_detail = FALSE;
	    generate_town(scene,CITY_TYPE_CITY);
	    break;

	case SCENE_FLAT_XROAD:
	    add_object_to_scene(scene,"road","","",M_PI/2.0,
		SCENE_SIZE/2.0,0.0,0.0, 
		SCENE_SIZE,SCENE_ROAD_WIDTH,0.0,
		0.0,0.0,0.0,0);
	    break;

	case SCENE_HILL_XROAD:
	    add_object_to_scene(scene,"hillroad","","",M_PI/2.0,
		SCENE_SIZE/2.0,0.0,0.0, 
		SCENE_SIZE,SCENE_ROAD_WIDTH,0.0,
		0.0,0.0,0.0,0);
	    break;

	case SCENE_FLAT_ZROAD:
	    add_object_to_scene(scene,"road","", "",0.0,
		0.0,0.0,-SCENE_SIZE/2.0, 
		SCENE_SIZE,SCENE_ROAD_WIDTH,0.0,
		0.0,0.0,0.0,0);
	    break;

	case SCENE_HILL_ZROAD:
	    add_object_to_scene(scene,"hillroad","", "",0.0,
		0.0,0.0,-SCENE_SIZE/2.0, 
		SCENE_SIZE,SCENE_ROAD_WIDTH,0.0,
		0.0,0.0,0.0,0);
	    break;

	case SCENE_FLAT_XHIGHWAY:
	    add_object_to_scene(scene,"road","","", M_PI/2.0,
		SCENE_SIZE/2.0,0.0,0.0, 
		SCENE_SIZE,SCENE_ROAD_WIDTH*2.0,0.0,
		0.0,0.0,0.0,0);
	    add_detail = FALSE;
	    switch (INTRAND(4)) {
		case 1:
		    add_object_to_scene(scene,"telephone line","","", M_PI/2.0,
			SCENE_SIZE/2.0,0.0,SCENE_ROAD_WIDTH*2, 
			SCENE_SIZE,SCENE_ROAD_WIDTH*2.0,0.0,
			0.0,0.0,0.0,0);
		    break;
		case 2:
		    add_object_to_scene(scene,"telephone line","","", M_PI/2.0,
			SCENE_SIZE/2.0,0.0,-SCENE_ROAD_WIDTH*2, 
			SCENE_SIZE,SCENE_ROAD_WIDTH*2.0,0.0,
			0.0,0.0,0.0,0);
		    break;
	    }
	    break;

	case SCENE_FLAT_ZHIGHWAY:
	    add_object_to_scene(scene,"road","","", 0.0,
		0.0,0.0,-SCENE_SIZE/2.0, 
		SCENE_SIZE,SCENE_ROAD_WIDTH*2.0,0.0,
		0.0,0.0,0.0,0);
	    add_detail = FALSE;
	    switch (INTRAND(4)) {
		case 1:
		    add_object_to_scene(scene,"telephone line","","", 0.0,
			SCENE_ROAD_WIDTH*2,0.0,-SCENE_SIZE/2.0, 
			SCENE_SIZE,SCENE_ROAD_WIDTH*2.0,0.0,
			0.0,0.0,0.0,0);
		    break;
		case 2:
		    add_object_to_scene(scene,"telephone line","","", 0.0,
			-SCENE_ROAD_WIDTH*2,0.0,-SCENE_SIZE/2.0, 
			SCENE_SIZE,SCENE_ROAD_WIDTH*2.0,0.0,
			0.0,0.0,0.0,0);
		    break;
	    }
	    break;

	case SCENE_FLAT_LEFT_T:
	    add_object_to_scene(scene,"road","","", 0.0,
		0.0,0.0,-SCENE_SIZE/2.0, 
		SCENE_SIZE,SCENE_ROAD_WIDTH,0.0,
		0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"road","","", M_PI/2.0,
		-SCENE_ROAD_WIDTH/2.0+1,0.0,0.0, 
		SCENE_SIZE/2.0-SCENE_ROAD_WIDTH/2.0+1,SCENE_ROAD_WIDTH,0.0,
		0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"stop sign","","",-M_PI/2.0,
		-SCENE_ROAD_WIDTH * 0.7,0.0,-SCENE_ROAD_WIDTH * 0.7,
		0.0,0.0,4.0,
		3.0,0.0,0.0,0);
	    add_object_to_scene(scene,"left t sign","","",0.0,
		SCENE_ROAD_WIDTH,0.0,-SCENE_SIZE/2.0 * 0.50,
		0.0,6.0,4.0,
		0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"right t sign","","",M_PI,
		-SCENE_ROAD_WIDTH,0.0,SCENE_SIZE/2.0 * 0.50,
		0.0,6.0,4.0,
		0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"top t sign","","",-M_PI/2.0,
		-SCENE_SIZE/4.0,0.0,-SCENE_ROAD_WIDTH * 0.70,
		0.0,6.0,4.0,
		0.0,0.0,0.0,0);
	    break;

	case SCENE_FLAT_RIGHT_T:
	    add_object_to_scene(scene,"road","","", 0.0,
		0.0,0.0,-SCENE_SIZE/2.0, 
		SCENE_SIZE,SCENE_ROAD_WIDTH,0.0,
		0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"road","","", -M_PI/2.0,
		SCENE_ROAD_WIDTH/2.0-1,0.0,0.0, 
		SCENE_SIZE/2.0-SCENE_ROAD_WIDTH/2.0+1,SCENE_ROAD_WIDTH,0.0,
		0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"stop sign","","",M_PI/2.0,
		SCENE_ROAD_WIDTH * 0.7,0.0,SCENE_ROAD_WIDTH * 0.7,
		0.0,0.0,4.0,
		3.0,0.0,0.0,0);
	    add_object_to_scene(scene,"top t sign","","",M_PI/2.0,
		SCENE_SIZE/4.0,0.0,SCENE_ROAD_WIDTH * 0.70,
		0.0,6.0,4.0,
		0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"left t sign","","",M_PI,
		-SCENE_ROAD_WIDTH,0.0,SCENE_SIZE/2.0 * 0.50,
		0.0,6.0,4.0,
		0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"right t sign","","",0.0,
		SCENE_ROAD_WIDTH,0.0,-SCENE_SIZE/2.0 * 0.50,
		0.0,6.0,4.0,
		0.0,0.0,0.0,0);
	    break;

	case SCENE_FLAT_DOWN_T:
	    add_object_to_scene(scene,"road","","", M_PI/2.0,
		SCENE_SIZE/2.0,0.0,0.0, 
		SCENE_SIZE,SCENE_ROAD_WIDTH,0.0,
		0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"road","","", M_PI,
		0.0,0.0,-SCENE_ROAD_WIDTH/2.0+1, 
		SCENE_SIZE/2.0-SCENE_ROAD_WIDTH/2.0+1,SCENE_ROAD_WIDTH,0.0,
		0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"stop sign","","",0.0,
		SCENE_ROAD_WIDTH * 0.7,0.0, -SCENE_ROAD_WIDTH * 0.7,
		0.0,0.0,4.0,
		3.0,0.0,0.0,0);
	    add_object_to_scene(scene,"top t sign","","",0.0,
		SCENE_ROAD_WIDTH * 0.7,0.0,-SCENE_SIZE/2.0 * 0.50,
		0.0,6.0,4.0,
		0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"left t sign","","",M_PI/2.0,
		SCENE_SIZE/2.0 * 0.50,0.0,SCENE_ROAD_WIDTH,
		0.0,6.0,4.0,
		0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"right t sign","","",-M_PI/2.0,
		-SCENE_SIZE/2.0 * 0.50,0.0,-SCENE_ROAD_WIDTH,
		0.0,6.0,4.0,
		0.0,0.0,0.0,0);
	    break;

	case SCENE_FLAT_UP_T:
	    add_object_to_scene(scene,"road","","", M_PI/2.0,
		SCENE_SIZE/2.0,0.0,0.0, 
		SCENE_SIZE,SCENE_ROAD_WIDTH,0.0,
		0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"road","","", 0.0,
		0.0,0.0,SCENE_ROAD_WIDTH/2.0-1, 
		SCENE_SIZE/2.0-SCENE_ROAD_WIDTH/2.0+1,SCENE_ROAD_WIDTH,0.0,
		0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"stop sign","","",M_PI,
		-SCENE_ROAD_WIDTH * 0.7,0.0, SCENE_ROAD_WIDTH * 0.7,
		0.0,0.0,4.0,
		3.0,0.0,0.0,0);
	    add_object_to_scene(scene,"top t sign","","",M_PI,
		-SCENE_ROAD_WIDTH * 0.7,0.0, SCENE_SIZE/2.0 * 0.50,
		0.0,6.0,4.0,
		0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"left t sign","","",-M_PI/2.0,
		-SCENE_SIZE/2.0 * 0.50,0.0,-SCENE_ROAD_WIDTH,
		0.0,6.0,4.0,
		0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"right t sign","","",M_PI/2.0,
		SCENE_SIZE/2.0 * 0.50,0.0,SCENE_ROAD_WIDTH,
		0.0,6.0,4.0,
		0.0,0.0,0.0,0);
	    break;

	case SCENE_FLAT_CROSSROADS:
	    /* just flat crossroads */
	    switch (i = INTRAND(5)) {
		case 1:
		case 2:
		case 3:
		    add_object_to_scene(scene,"stop sign","","",0.0,
			SCENE_ROAD_WIDTH * 0.7,0.0, -SCENE_ROAD_WIDTH * 0.7,
			0.0,0.0,4.0,
			3.0,0.0,0.0,0);
		    add_object_to_scene(scene,"stop sign","","",M_PI,
			-SCENE_ROAD_WIDTH * 0.7,0.0, SCENE_ROAD_WIDTH * 0.7,
			0.0,0.0,4.0,
			3.0,0.0,0.0,0);
		    if (i < 3) break;
		    /* else fall through */
		case 4:
		case 5:
		    add_object_to_scene(scene,"stop sign","","",M_PI/2.0,
			SCENE_ROAD_WIDTH * 0.7,0.0, SCENE_ROAD_WIDTH * 0.7,
			0.0,0.0,4.0,
			3.0,0.0,0.0,0);
		    add_object_to_scene(scene,"stop sign","","",-M_PI/2.0,
			-SCENE_ROAD_WIDTH * 0.7,0.0, -SCENE_ROAD_WIDTH * 0.7,
			0.0,0.0,4.0,
			3.0,0.0,0.0,0);
		    break;
	    }

	    add_object_to_scene(scene,"road","","", M_PI/2.0,
		SCENE_SIZE/2.0,0.0,0.0, 
		SCENE_SIZE,SCENE_ROAD_WIDTH,0.0,
		0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"road","","", 0.0,
		0.0,0.0,-SCENE_SIZE/2.0, 
		SCENE_SIZE/2.0 - SCENE_ROAD_WIDTH/2.0+1,SCENE_ROAD_WIDTH,0.0,
		0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"road","","", 0.0,
		0.0,0.0,SCENE_ROAD_WIDTH/2.0-1, 
		SCENE_SIZE/2.0 - SCENE_ROAD_WIDTH/2.0+1,SCENE_ROAD_WIDTH,0.0,
		0.0,0.0,0.0,0);
	    break;

	case SCENE_HILL_CROSSROADS:
	    add_object_to_scene(scene,"hillcrossroad","","", 0.0,
		0.0,0.0,0.0, 
		HILL_XROAD_LEN,SCENE_ROAD_WIDTH,0.0,
		0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"hillroad","","", 0.0,
		0.0,0.0,(-SCENE_SIZE/2.0),
		HILL_XROAD_ROAD_LEN,SCENE_ROAD_WIDTH,0.0,
		0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"hillroad","","", 0.0,
		0.0,0.0,HILL_XROAD_LEN/2.0,
		HILL_XROAD_ROAD_LEN,SCENE_ROAD_WIDTH,0.0,
		0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"hillroad","","", M_PI/2.0,
		(SCENE_SIZE/2.0),0.0,0.0, 
		HILL_XROAD_ROAD_LEN,SCENE_ROAD_WIDTH,0.0,
		0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"hillroad","","", M_PI/2.0,
		-HILL_XROAD_LEN/2.0,0.0,0.0, 
		HILL_XROAD_ROAD_LEN,SCENE_ROAD_WIDTH,0.0,
		0.0,0.0,0.0,0);
	    break;

	case SCENE_FLAT_OVERPASS:
	    switch (INTRAND(2)) {
		case 1:
		    /* Z overpass */
		    add_object_to_scene(scene,"road","","", M_PI/2.0,
			SCENE_SIZE/2.0,0.0,0.0, 
			SCENE_SIZE,SCENE_ROAD_WIDTH,0.0,
			0.0,0.0,0.0,0);
		    add_object_to_scene(scene,"road","","", 0.0,
			0.0,0.0,BRIDGE_LENGTH/2.0-1,
			(SCENE_SIZE-BRIDGE_LENGTH)/2.0+1,SCENE_ROAD_WIDTH,0.0,
			0.0,0.0,0.0,0);
		    add_object_to_scene(scene,"road","","", 0.0,
			0.0,0.0,-SCENE_SIZE/2.0,
			(SCENE_SIZE-BRIDGE_LENGTH)/2.0+1,SCENE_ROAD_WIDTH,0.0,
			0.0,0.0,0.0,0);
		    add_object_to_scene(scene,"bridge","","", 0.0,
			0.0,0.0,0.0,
			BRIDGE_LENGTH,SCENE_ROAD_WIDTH,BRIDGE_HEIGHT,
			0.0,0.0,0.0,0);
		    break;

		case 2:
		    /* X overpass */
		    add_object_to_scene(scene,"road","","", 0.0,
			0.0,0.0,-SCENE_SIZE/2.0, 
			SCENE_SIZE,SCENE_ROAD_WIDTH,0.0,
			0.0,0.0,0.0,0);
		    add_object_to_scene(scene,"road","","", -M_PI/2.0,
			BRIDGE_LENGTH/2.0-1,0.0,0.0,
			(SCENE_SIZE-BRIDGE_LENGTH)/2.0+1,SCENE_ROAD_WIDTH,0.0,
			0.0,0.0,0.0,0);
		    add_object_to_scene(scene,"road","","", M_PI/2.0,
			-BRIDGE_LENGTH/2.0+1,0.0,0.0,
			(SCENE_SIZE-BRIDGE_LENGTH)/2.0+1,SCENE_ROAD_WIDTH,0.0,
			0.0,0.0,0.0,0);
		    add_object_to_scene(scene,"bridge","","", M_PI/2.0,
			0.0,0.0,0.0,
			BRIDGE_LENGTH,SCENE_ROAD_WIDTH,BRIDGE_HEIGHT,
			0.0,0.0,0.0,0);
		    break;
		}
		break;

	case SCENE_FLAT_XHIGHWAY_OVERPASS:
		/* X overpass */
		add_object_to_scene(scene,"road","","", 0.0,
		    0.0,0.0,-SCENE_SIZE/2.0, 
		    SCENE_SIZE,SCENE_ROAD_WIDTH,0.0,
		    0.0,0.0,0.0,0);
		add_object_to_scene(scene,"road","","", -M_PI/2.0,
		    BRIDGE_LENGTH/2.0-1,0.0,0.0,
		    (SCENE_SIZE-BRIDGE_LENGTH)/2.0+1,SCENE_ROAD_WIDTH*2.0,0.0,
		    0.0,0.0,0.0,0);
		add_object_to_scene(scene,"road","","", M_PI/2.0,
		    -BRIDGE_LENGTH/2.0+1,0.0,0.0,
		    (SCENE_SIZE-BRIDGE_LENGTH)/2.0+1,SCENE_ROAD_WIDTH*2.0,0.0,
		    0.0,0.0,0.0,0);
		add_object_to_scene(scene,"bridge","","", M_PI/2.0,
		    0.0,0.0,0.0,
		    BRIDGE_LENGTH,SCENE_ROAD_WIDTH*2.0,BRIDGE_HEIGHT,
		    0.0,0.0,0.0,0);
		break;

	case SCENE_FLAT_ZHIGHWAY_OVERPASS:
		/* Z overpass */
		add_object_to_scene(scene,"road","","", M_PI/2.0,
		    SCENE_SIZE/2.0,0.0,0.0, 
		    SCENE_SIZE,SCENE_ROAD_WIDTH,0.0,
		    0.0,0.0,0.0,0);
		add_object_to_scene(scene,"road","","", 0.0,
		    0.0,0.0,BRIDGE_LENGTH/2.0-1,
		    (SCENE_SIZE-BRIDGE_LENGTH)/2.0+1,SCENE_ROAD_WIDTH*2.0,0.0,
		    0.0,0.0,0.0,0);
		add_object_to_scene(scene,"road","","", 0.0,
		    0.0,0.0,-SCENE_SIZE/2.0,
		    (SCENE_SIZE-BRIDGE_LENGTH)/2.0+1,SCENE_ROAD_WIDTH*2.0,0.0,
		    0.0,0.0,0.0,0);
		add_object_to_scene(scene,"bridge","","", 0.0,
		    0.0,0.0,0.0,
		    BRIDGE_LENGTH,SCENE_ROAD_WIDTH*2.0,BRIDGE_HEIGHT,
		    0.0,0.0,0.0,0);
		break;

	case SCENE_FLAT_TURN_RIGHT_DOWN:
	    add_object_to_scene(scene,"road","","", -M_PI/2.0,
		TURN_OFFSET-1,0.0,0.0, 
		SCENE_SIZE/2.0-TURN_OFFSET+1,SCENE_ROAD_WIDTH,0.0,
		0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"road","","", M_PI,
		0.0,0.0,-TURN_OFFSET+1, 
		SCENE_SIZE/2.0-TURN_OFFSET+1,SCENE_ROAD_WIDTH,0.0,
		0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"curve","","", M_PI/2.0,
		TURN_OFFSET,0.0,-TURN_OFFSET, 
		0.0,SCENE_ROAD_WIDTH,0.0,
		TURN_RADIUS,M_PI/2.0,0.0,0);
	    add_object_to_scene(scene,"right curve sign","","",0.0,
		SCENE_ROAD_WIDTH * 0.7,0.0, -SCENE_SIZE/2.0 * 0.50,
		0.0,6.0,4.0,0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"left curve sign","","",M_PI/2.0,
		SCENE_SIZE/2.0 * 0.50,0.0, SCENE_ROAD_WIDTH * 0.7,
		0.0,6.0,4.0,0.0,0.0,0.0,0);
	    break;

	case SCENE_FLAT_TURN_RIGHT_UP:
	    add_object_to_scene(scene,"road","","", -M_PI/2.0,
		TURN_OFFSET-1,0.0,0.0, 
		SCENE_SIZE/2.0-TURN_OFFSET+1,SCENE_ROAD_WIDTH,0.0,
		0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"road","","", 0.0,
		0.0,0.0,TURN_OFFSET-1, 
		SCENE_SIZE/2.0-TURN_OFFSET+1,SCENE_ROAD_WIDTH,0.0,
		0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"curve","","", M_PI,
		TURN_OFFSET,0.0,TURN_OFFSET, 
		0.0,SCENE_ROAD_WIDTH,0.0,
		TURN_RADIUS,M_PI/2.0,0.0,0);
	    add_object_to_scene(scene,"left curve sign","","",M_PI,
		-SCENE_ROAD_WIDTH * 0.7,0.0, SCENE_SIZE/2.0 * 0.50,
		0.0,6.0,4.0,
		0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"right curve sign","","",M_PI/2.0,
		SCENE_SIZE/2.0 * 0.50,0.0, SCENE_ROAD_WIDTH * 0.7,
		0.0,6.0,4.0,
		0.0,0.0,0.0,0);
	    break;

	case SCENE_FLAT_TURN_LEFT_DOWN:
	    add_object_to_scene(scene,"road","","", M_PI/2.0,
		-TURN_OFFSET+1,0.0,0.0, 
		SCENE_SIZE/2.0-TURN_OFFSET+1,SCENE_ROAD_WIDTH,0.0,
		0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"road","","", M_PI,
		0.0,0.0,-TURN_OFFSET+1, 
		SCENE_SIZE/2.0-TURN_OFFSET+1,SCENE_ROAD_WIDTH,0.0,
		0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"curve","","", 0.0,
		-TURN_OFFSET,0.0,-TURN_OFFSET, 
		0.0,SCENE_ROAD_WIDTH,0.0,
		TURN_RADIUS,M_PI/2.0,0.0,0);
	    add_object_to_scene(scene,"left curve sign","","",0.0,
		SCENE_ROAD_WIDTH * 0.7,0.0, -SCENE_SIZE/2.0 * 0.50,
		0.0,6.0,4.0,
		0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"right curve sign","","",-M_PI/2.0,
		-SCENE_SIZE/2.0 * 0.50,0.0, -SCENE_ROAD_WIDTH * 0.7,
		0.0,6.0,4.0,
		0.0,0.0,0.0,0);
	    break;

	case SCENE_FLAT_TURN_LEFT_UP:
	    add_object_to_scene(scene,"road","","", M_PI/2.0,
		-TURN_OFFSET+1,0.0,0.0, 
		SCENE_SIZE/2.0-TURN_OFFSET,SCENE_ROAD_WIDTH,0.0,
		0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"road","","", 0.0,
		0.0,0.0,TURN_OFFSET-1, 
		SCENE_SIZE/2.0-TURN_OFFSET+1,SCENE_ROAD_WIDTH,0.0,
		0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"curve","","", -M_PI/2.0,
		-TURN_OFFSET,0.0,TURN_OFFSET, 
		0.0,SCENE_ROAD_WIDTH,0.0,
		TURN_RADIUS,M_PI/2.0,0.0,0);
	    add_object_to_scene(scene,"right curve sign","","",M_PI,
		-SCENE_ROAD_WIDTH * 0.7,0.0, SCENE_SIZE/2.0 * 0.50,
		0.0,6.0,4.0,
		0.0,0.0,0.0,0);
	    add_object_to_scene(scene,"left curve sign","","",-M_PI/2.0,
		-SCENE_SIZE/2.0 * 0.50,0.0, -SCENE_ROAD_WIDTH * 0.7,
		0.0,6.0,4.0,
		0.0,0.0,0.0,0);
	    break;
    }

    if (scene->flags & SCENE_ALL_FLAT) {
	add_object_to_scene(scene,"ground","","",0.0,
	    0.0,0.0,0.0,
	    SCENE_SIZE,SCENE_SIZE,0.0,
	    0.0,0.0,0.0,0);
    }

    /* Now that the roads are laid out, plug in some random type objects */
    if (add_detail) {
	unsigned int road_flags;

	if (scene->type == SCENE_FLAT_TURN_RIGHT_UP) {
	    /* make sure we don't intersect the curve */
	    populate_area(scene,0,
		TURN_RADIUS,TURN_RADIUS,
		QUADRANT_SIZE-TURN_RADIUS,QUADRANT_SIZE-TURN_RADIUS);
	}
	else {
	    road_flags = 0;
	    if (scene->flags & SCENE_ROAD_RIGHT)
		road_flags |= QUADRANT_ROAD_BOTTOM;
	    if (scene->flags & SCENE_ROAD_UP)
		road_flags |= QUADRANT_ROAD_LEFT;
	    populate_area(scene,road_flags,
		SCENE_ROAD_WIDTH/2.0, SCENE_ROAD_WIDTH/2.0,
		QUADRANT_SIZE - SCENE_ROAD_WIDTH/2.0,
		QUADRANT_SIZE - SCENE_ROAD_WIDTH/2.0);
	}

	if (scene->type == SCENE_FLAT_TURN_LEFT_UP) {
	    /* make sure we don't intersect the curve */
	    populate_area(scene,0,
		-QUADRANT_SIZE,TURN_RADIUS,
		QUADRANT_SIZE-TURN_RADIUS,QUADRANT_SIZE-TURN_RADIUS);
	}
	else {
	    road_flags = 0;
	    if (scene->flags & SCENE_ROAD_LEFT)
		road_flags |= QUADRANT_ROAD_BOTTOM;
	    if (scene->flags & SCENE_ROAD_UP)
		road_flags |= QUADRANT_ROAD_RIGHT;
	    populate_area(scene,road_flags,
		-QUADRANT_SIZE,SCENE_ROAD_WIDTH/2.0,
		QUADRANT_SIZE - SCENE_ROAD_WIDTH/2.0,
		QUADRANT_SIZE - SCENE_ROAD_WIDTH/2.0);
	}

	if (scene->type == SCENE_FLAT_TURN_LEFT_DOWN) {
	    /* make sure we don't intersect the curve */
	    populate_area(scene,0,
		-QUADRANT_SIZE,-QUADRANT_SIZE,
		QUADRANT_SIZE-TURN_RADIUS,QUADRANT_SIZE-TURN_RADIUS);
	}
	else {
	    road_flags = 0;
	    if (scene->flags & SCENE_ROAD_LEFT)
		road_flags |= QUADRANT_ROAD_TOP;
	    if (scene->flags & SCENE_ROAD_DOWN)
		road_flags |= QUADRANT_ROAD_RIGHT;
	    populate_area(scene,road_flags,
		-QUADRANT_SIZE,-QUADRANT_SIZE,
		QUADRANT_SIZE - SCENE_ROAD_WIDTH/2.0,
		QUADRANT_SIZE - SCENE_ROAD_WIDTH/2.0);
	}

	if (scene->type == SCENE_FLAT_TURN_RIGHT_DOWN) {
	    /* make sure we don't intersect the curve */
	    populate_area(scene,0,
		TURN_RADIUS,-QUADRANT_SIZE,
		QUADRANT_SIZE-TURN_RADIUS,QUADRANT_SIZE-TURN_RADIUS);
	}
	else {
	    road_flags = 0;
	    if (scene->flags & SCENE_ROAD_RIGHT)
		road_flags |= QUADRANT_ROAD_TOP;
	    if (scene->flags & SCENE_ROAD_DOWN)
		road_flags |= QUADRANT_ROAD_LEFT;
	    populate_area(scene,road_flags,
		SCENE_ROAD_WIDTH/2.0,-QUADRANT_SIZE,
		QUADRANT_SIZE - SCENE_ROAD_WIDTH/2.0,
		QUADRANT_SIZE - SCENE_ROAD_WIDTH/2.0);
	}
    }
}


SCENE *generate_random_scene(
    SCENE *newscene,
    int xscene,
    int zscene)
{
    int num_segs;

    if (newscene == NULL) {
	if ((newscene = (SCENE *) malloc(sizeof(SCENE))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return(NULL);
	}

	newscene->xscene = xscene;
	newscene->zscene = zscene;
	newscene->type   = SCENE_WHATEVER_FITS;
	newscene->flags  = 0;
	newscene->static_seg = INVALID;
	newscene->count = 0;
	newscene->random_seed = 0;
	newscene->object_head = NULL;
	newscene->defined = FALSE;
	newscene->read_in = FALSE;
	newscene->scenefilename = NULL;
	newscene->file_mtime = 0;
	newscene->comments  = NULL;

	if (!add_scene_to_list(newscene)) {
	    free(newscene);
	    return(NULL);
	}
    }

    num_segs = cur_seg;
    newscene->random_seed = getpid() ^ time(0) ^ ZINTRAND(1<<30);
    newscene->static_seg = INVALID;
    newscene->count = 0;

    /* Generate objects */
    seedrand(newscene->random_seed);
    newscene->object_head = NULL;
    generate_random_objects(newscene,newscene->type);

#if 0
    update_segment_lists(num_segs);
#endif

    return(newscene);
}
