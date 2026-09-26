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


/* Code Module for the road segment object */


#include <stdio.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"

#define DEFAULT_ROAD_LENGTH	500.0
#define DEFAULT_ROAD_HEIGHT	0.0
#define DEFAULT_ROAD_COLOR	ASPHALT_INTENSITY

#define MAX_ROAD_SEGMENT_LENGTH	(ROAD_LINE_SPACING*10.0)
    /* MAX_ROAD_SEGMENT_LENGTH needs to be an even multiple
     * of ROAD_LINE_SPACING.
     */

#define Length				(obj->size[SIZE_LENGTH])
#define Width				(obj->size[SIZE_WIDTH])
#define Height				(obj->size[SIZE_HEIGHT])

typedef struct _road_list {
    float length,width,height;
    unsigned int nameset_bits;
    float color[3];
    int dl_number;
    struct _road_list *next;
} ROAD_LIST;
static ROAD_LIST *road_list=NULL;


static int road_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = 0.0;
    return(TRUE);
}

static int thick_road_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = 0.0;
    /*
    This makes it not work... leave it out 
    get_box_mc_normal(obj,x,y,z,sc->mc_normal);
    */
    return(TRUE);
}


static int thick_road_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = 0.0;
    return(TRUE);
}

static float 
    stripe_data[1000][4];	/* This should be big enough */

static hwObject create_road_segment(
    DRIVE_OBJECT *obj,
    float zstart, float len)
{
    float pgon[8][6],*ptr;
    float x,z,ztmp;
    int lanes,lane;
    int 
	i, 
	numChildren,
	num_stripes = 0;
    float
	roadLODs[8][2];
    hwObject 
	roadGroup,		/* Holds the road polgons and stripes */
	stripes,		/* hwQuads -- holds all the stripes */
	road[8];		/* for potentially raised roads */

    hwObject texture=NULL;

    if( do_textures ) {
	texture = hwFindObject( "RoadTexture" );
    }

    roadGroup = hwGroup->create(hwGroup);
    roadGroup->name = 0;

    i=0;
    road[i] = hwPolygon->create(hwPolygon);
    road[i]->name = 0;

    /* TBD: We've gotta do colored roads! */
    ASPHALT_HW(road[i]);

    ptr = (float *) pgon;
    *ptr++ = -Width/2.0;
	*ptr++ = ROAD_FLOAT;
	*ptr++ = zstart;
    *ptr++ =  Width/2.0;
	*ptr++ = ROAD_FLOAT;
	*ptr++ = zstart;
    *ptr++ = Width/2.0;
	*ptr++ = ROAD_FLOAT;
	*ptr++ = zstart+len;
    *ptr++ = -Width/2.0;
	*ptr++ = ROAD_FLOAT;
	*ptr++ = zstart+len;

    road[i]->modify(road[i], hwStrData, HW_MAKE_TYPE(HW_TYPE_FLOAT, 12),pgon);
    HW_MODIFY_1B( road[i], hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( road[i], hwStrFlipNormals, HW_TRUE );

    /* Always draw the road polygon */
    roadLODs[i][0] = 0.0;
    roadLODs[i][1] = 100.0;
    i++;

    if( Height > 0.0 )
    {

	road[i] = hwPolygon->create(hwPolygon);
	road[i]->name = 0;
	HW_MODIFY_3F(road[i], hwStrColor, 0.45,0.45,0.45);

	/* Draw the bottom of the road */
	ptr = (float *) pgon;
	*ptr++ = Width/2.0;
	    *ptr++ = -Height;
	    *ptr++ = zstart;
	*ptr++ = -Width/2.0;
	    *ptr++ = -Height;
	    *ptr++ = zstart;
	*ptr++ = -Width/2.0;
	    *ptr++ = -Height;
	    *ptr++ = zstart+len;
	*ptr++ = Width/2.0;
	    *ptr++ = -Height;
	    *ptr++ = zstart+len;
	road[i]->modify(road[i], hwStrData,HW_MAKE_TYPE(HW_TYPE_FLOAT,12),pgon);
	HW_MODIFY_1B( road[i], hwStrBackface, HW_TRUE );
	HW_MODIFY_1B( road[i], hwStrFlipNormals, HW_TRUE );


	roadLODs[i][0] = 0.0;
	roadLODs[i][1] = 100.0;
	i++;

	road[i] = hwPolygon->create(hwPolygon);
	road[i]->name = 0;
	CONCRETE_HW(road[i]);

	/* Draw the left side of the road */
	ptr = (float *) pgon;
	*ptr++ = -Width/2.0;
	    *ptr++ = ROAD_FLOAT;
	    *ptr++ = zstart;
	*ptr++ = -Width/2.0;
	    *ptr++ = ROAD_FLOAT;
	    *ptr++ = zstart+len;
	*ptr++ = -Width/2.0;
	    *ptr++ = -Height;
	    *ptr++ = zstart+len;
	*ptr++ = -Width/2.0;
	    *ptr++ = -Height;
	    *ptr++ = zstart;
	road[i]->modify(road[i], hwStrData,HW_MAKE_TYPE(HW_TYPE_FLOAT,12),pgon);
	HW_MODIFY_1B( road[i], hwStrBackface, HW_TRUE );
	HW_MODIFY_1B( road[i], hwStrFlipNormals, HW_TRUE );

	roadLODs[i][0] = 0.0;
	roadLODs[i][1] = 100.0;
	i++;

	road[i] = hwPolygon->create(hwPolygon);
	road[i]->name = 0;
	CONCRETE_HW(road[i]);
	/* Draw the right side of the road */
	ptr = (float *) pgon;
	*ptr++ = Width/2.0;
	    *ptr++ = -Height;
	    *ptr++ = zstart;
	*ptr++ = Width/2.0;
	    *ptr++ = -Height;
	    *ptr++ = zstart+len;
	*ptr++ = Width/2.0;
	    *ptr++ = ROAD_FLOAT;
	    *ptr++ = zstart+len;
	*ptr++ = Width/2.0;
	    *ptr++ = ROAD_FLOAT;
	    *ptr++ = zstart;
	road[i]->modify(road[i], hwStrData,HW_MAKE_TYPE(HW_TYPE_FLOAT,12),pgon);
	HW_MODIFY_1B( road[i], hwStrBackface, HW_TRUE );
	HW_MODIFY_1B( road[i], hwStrFlipNormals, HW_TRUE );
       
	roadLODs[i][0] = 0.0;
	roadLODs[i][1] = 100.0;
	i++;

	road[i] = hwPolygon->create(hwPolygon);
	road[i]->name = 0;
	CONCRETE_HW(road[i]);
	/* Cap the end of the road */
	ptr = (float *) pgon;
	*ptr++ = Width/2.0;
	    *ptr++ = -Height;
	    *ptr++ = zstart;
	*ptr++ = Width/2.0;
	    *ptr++ = ROAD_FLOAT;
	    *ptr++ = zstart;
	*ptr++ = -Width/2.0;
	    *ptr++ = ROAD_FLOAT;
	    *ptr++ = zstart;
	*ptr++ = -Width/2.0;
	    *ptr++ = -Height;
	    *ptr++ = zstart;
	road[i]->modify(road[i], hwStrData,HW_MAKE_TYPE(HW_TYPE_FLOAT,12),pgon);
	HW_MODIFY_1B( road[i], hwStrBackface, HW_TRUE );
	HW_MODIFY_1B( road[i], hwStrFlipNormals, HW_TRUE );

	roadLODs[i][0] = 0.0;
	roadLODs[i][1] = 100.0;
	i++;

	road[i] = hwPolygon->create(hwPolygon);
	road[i]->name = 0;
	CONCRETE_HW(road[i]);
	/* Cap the other end of the road */
	ptr = (float *) pgon;
	*ptr++ = -Width/2.0;
	    *ptr++ = -Height;
	    *ptr++ = zstart+len;
	*ptr++ = -Width/2.0;
	    *ptr++ = ROAD_FLOAT;
	    *ptr++ = zstart+len;
	*ptr++ = Width/2.0;
	    *ptr++ = ROAD_FLOAT;
	    *ptr++ = zstart+len;
	*ptr++ = Width/2.0;
	    *ptr++ = -Height;
	    *ptr++ = zstart+len;
	road[i]->modify(road[i], hwStrData,HW_MAKE_TYPE(HW_TYPE_FLOAT,12),pgon);
	HW_MODIFY_1B( road[i], hwStrBackface, HW_TRUE );
	HW_MODIFY_1B( road[i], hwStrFlipNormals, HW_TRUE );

	roadLODs[i][0] = 0.0;
	roadLODs[i][1] = 100.0;
	i++;
    }

    numChildren = i;

    /* Now, lets do the strips if we need to */

    if (Width > (LANE_WIDTH*2.0-0.5)) {

	ptr = (float *)stripe_data;
	stripes = hwQuads->create(hwQuads);
	stripes->name = 0;
	ROAD_LINE_YELLOW_HW(stripes);
	HW_MODIFY_1B(stripes, hwStrHasNormals, HW_TRUE);
	HW_MODIFY_1B(stripes, hwStrBackface, HW_TRUE);
	if( do_textures ) {
	    stripes->modify(stripes, hwStrTexture, HW_TYPE_OBJECT, texture);
	}

	if (Width < (LANE_WIDTH*4.0-0.5)) {
	    for (z=zstart; z<zstart+len; z+=ROAD_LINE_SPACING) {

		if ((ztmp = z + ROAD_LINE_LENGTH) > zstart+len)
		    ztmp = zstart+len;

		*ptr++ = ROAD_LINE_WIDTH/2.0;
		    *ptr++ = ROAD_LINE_FLOAT;
		    *ptr++ = z;
		    *ptr++ = 0.0;
		    *ptr++ = 0.707;
		    *ptr++ = -0.707;

		*ptr++ = ROAD_LINE_WIDTH/2.0;
		    *ptr++ = ROAD_LINE_FLOAT;
		    *ptr++ = ztmp;
		    *ptr++ = 0.0;
		    *ptr++ = 0.707;
		    *ptr++ = -0.707;

		*ptr++ = -ROAD_LINE_WIDTH/2.0;
		    *ptr++ = ROAD_LINE_FLOAT;
		    *ptr++ = ztmp;
		    *ptr++ = 0.0;
		    *ptr++ = 0.707;
		    *ptr++ = 0.707;

		*ptr++ = -ROAD_LINE_WIDTH/2.0;
		    *ptr++ = ROAD_LINE_FLOAT;
		    *ptr++ = z;
		    *ptr++ = 0.0;
		    *ptr++ = 0.707;
		    *ptr++ = 0.707;

		num_stripes++;
	    }
	}
	else {
	    /* wide road */
	    lanes = ((int) (Width/(LANE_WIDTH*2.0))) * 2;
	    for (lane = 0, x = -LANE_WIDTH*(lanes/2-1);
		    lane < lanes-1;
		    ++lane, x += LANE_WIDTH) {
		if (lane == (lanes/2-1)) {
		    /* center stripes */
		    *ptr++ =     x - (ROAD_LINE_WIDTH*0.5);
			*ptr++ = ROAD_LINE_FLOAT;
			*ptr++ = zstart;
			*ptr++ = 0.0;
			*ptr++ = 1.0;
			*ptr++ = 0.0;

		    *ptr++ =     x - (ROAD_LINE_WIDTH*0.5);
			*ptr++ = ROAD_LINE_FLOAT;
			*ptr++ = zstart+len;
			*ptr++ = 0.0;
			*ptr++ = 1.0;
			*ptr++ = 0.0;

		    *ptr++ =     x - (ROAD_LINE_WIDTH*1.5);
			*ptr++ = ROAD_LINE_FLOAT;
			*ptr++ = zstart+len;
			*ptr++ = 0.0;
			*ptr++ = 1.0;
			*ptr++ = 0.0;

		    *ptr++ =     x - (ROAD_LINE_WIDTH*1.5);
			*ptr++ = ROAD_LINE_FLOAT;
			*ptr++ = zstart;
			*ptr++ = 0.0;
			*ptr++ = 1.0;
			*ptr++ = 0.0;

		    num_stripes++;

		    *ptr++ =     x + (ROAD_LINE_WIDTH*1.5);
			*ptr++ = ROAD_LINE_FLOAT;
			*ptr++ = zstart;
			*ptr++ = 0.0;
			*ptr++ = 1.0;
			*ptr++ = 0.0;

		    *ptr++ =     x + (ROAD_LINE_WIDTH*1.5);
			*ptr++ = ROAD_LINE_FLOAT;
			*ptr++ = zstart+len;
			*ptr++ = 0.0;
			*ptr++ = 1.0;
			*ptr++ = 0.0;

		    *ptr++ =     x + (ROAD_LINE_WIDTH*0.5);
			*ptr++ = ROAD_LINE_FLOAT;
			*ptr++ = zstart+len;
			*ptr++ = 0.0;
			*ptr++ = 1.0;
			*ptr++ = 0.0;

		    *ptr++ =     x + (ROAD_LINE_WIDTH*0.5);
			*ptr++ = ROAD_LINE_FLOAT;
			*ptr++ = zstart;
			*ptr++ = 0.0;
			*ptr++ = 1.0;
			*ptr++ = 0.0;

		    num_stripes++;
		}
		else for (z=zstart; z<zstart+len; z+=ROAD_LINE_SPACING) {

		    if ((ztmp = z + ROAD_LINE_LENGTH) > zstart+len)
			ztmp = zstart+len;

		    *ptr++ =     x + ROAD_LINE_WIDTH/2.0;
			*ptr++ = ROAD_LINE_FLOAT;
			*ptr++ = z;
			*ptr++ = 0.0;
			*ptr++ = 0.707;
			*ptr++ = -0.707;

		    *ptr++ =     x + ROAD_LINE_WIDTH/2.0;
			*ptr++ = ROAD_LINE_FLOAT;
			*ptr++ = ztmp;
			*ptr++ = 0.0;
			*ptr++ = 0.707;
			*ptr++ = -0.707;

		    *ptr++ =     x - ROAD_LINE_WIDTH/2.0;
			*ptr++ = ROAD_LINE_FLOAT;
			*ptr++ = ztmp;
			*ptr++ = 0.0;
			*ptr++ = 0.707;
			*ptr++ = 0.707;

		    *ptr++ =     x - ROAD_LINE_WIDTH/2.0;
			*ptr++ = ROAD_LINE_FLOAT;
			*ptr++ = z;
			*ptr++ = 0.0;
			*ptr++ = 0.707;
			*ptr++ = 0.707;

		    num_stripes++;
		}
	    }
	}
		
	stripes->modify(stripes, hwStrData, 
	    HW_MAKE_TYPE(HW_TYPE_FLOAT, num_stripes * 6*4), stripe_data);
	road[numChildren] = stripes;

	/* We only want to draw stripes if the road is *big* */
	roadLODs[numChildren][0] = 40.0;
	roadLODs[numChildren][1] = 100.0;
	numChildren++;
    }

    roadGroup->modify(roadGroup, hwStrChildren, 
	HW_MAKE_TYPE(HW_TYPE_OBJECT,numChildren), road);
    roadGroup->modify(roadGroup, hwStrLOD, 
	HW_MAKE_TYPE(HW_TYPE_FLOAT,2*numChildren), roadLODs);
    return(roadGroup);

}


static void create_road_graphics(
    DRIVE_OBJECT *obj)
{
    float z,len;
    hwObject *segs;
    int segcount,size;
    hwObject group;

    /* Lets bust the road up into chunks.  */
    size = (Length/MAX_ROAD_SEGMENT_LENGTH + 1) * sizeof(hwObject);

    if ((segs = (hwObject *) malloc(size)) == NULL) {
	fprintf(stderr,"Out of malloc space!\n");
	exit(1);
    }

    for (z=0.0,segcount=0; z<Length; z += MAX_ROAD_SEGMENT_LENGTH,++segcount) {
	if (Length - z < MAX_ROAD_SEGMENT_LENGTH) len = Length - z;
	else len = MAX_ROAD_SEGMENT_LENGTH;
	segs[segcount] = create_road_segment(obj,z,len);
    }

    group = hwGroup->create(hwGroup);
    group->name = 0;
    group->modify(group, hwStrChildren, HW_MAKE_TYPE(HW_TYPE_OBJECT,segcount),
	segs);

    obj->display_list = createHwSegmentFromObj(&group,1);

    free((void *) segs);
}


void init_road_object(
    DRIVE_OBJECT *obj)
{
    ROAD_LIST *rl;
    if (Length <= 0.0) Length = DEFAULT_ROAD_LENGTH;
    if (Width <= 0.0)  Width  = DEFAULT_ROAD_WIDTH;
    if (Height <= 0.0)  Height  = DEFAULT_ROAD_HEIGHT;

    if (obj->color[0] == DEFAULT_OBJECT_COLOR)
	obj->color[0] = DEFAULT_ROAD_COLOR;
    if (obj->color[1] == DEFAULT_OBJECT_COLOR)
	obj->color[1] = DEFAULT_ROAD_COLOR;
    if (obj->color[2] == DEFAULT_OBJECT_COLOR)
	obj->color[2] = DEFAULT_ROAD_COLOR;

    /* Extend it a bit to avoid gaps */
    Length += (1.0/12.0);

    if(debug) printf(" inside init_road_%d_object() routine \n",(int) Length);

    obj->num_children = 0;

    /* See if we've created one like this before... */
    rl = road_list;
    while (rl != NULL) {
	if (IS_NEAR(rl->length,Length)
		&& IS_NEAR(rl->height,Height)
		&& IS_NEAR(rl->width,Width)
		&& IS_NEAR(rl->color[0],obj->color[0])
		&& IS_NEAR(rl->color[1],obj->color[1])
		&& IS_NEAR(rl->color[2],obj->color[2])
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
	if ((rl = (ROAD_LIST *) malloc(sizeof(ROAD_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}
	rl->length = Length;
	rl->width  = Width;
	rl->height  = Height;
	rl->nameset_bits = obj->nameset_bits;
	rl->color[0] = obj->color[0];
	rl->color[1] = obj->color[1];
	rl->color[2] = obj->color[2];
	rl->next   = road_list;
    	create_road_graphics(obj);
	rl->dl_number = obj->display_list;
	road_list  = rl;
    }

    if( Height == 0.0)
    {
	obj->surface_chars_xyz  = road_surface_chars_xyz;
    }
    else
    {
	obj->surface_chars_xyz  = thick_road_surface_chars_xyz;
	obj->surface_chars_bbox = thick_road_surface_chars_bbox;
    }


    /* Initialize bounding box values */
    obj->bound_mc[0] = -Width/2.0;
    obj->bound_mc[1] = -Height - BBOX_MARGIN;
    obj->bound_mc[2] = 0.0;
    obj->bound_mc[3] = Width/2.0;
    obj->bound_mc[4] = BBOX_MARGIN;
    obj->bound_mc[5] = Length;

    update_wc_bounds(obj);
}
