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


/* Code Module for the ltube segment object */


#include <stdio.h>
#include <math.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"

#define DEFAULT_RADIUS (50.0)
#define DEFAULT_LENGTH (200.0)
#define DELTA 10.0

#define Length			(obj->size[SIZE_LENGTH])
#define Radius			(obj->radius)
#define HEIGHT 			(obj->radius - obj->radius * SIN45)
#define WIDTH 			(obj->radius * COS45)

typedef struct _ltube_list {
    float length,radius;
    unsigned int nameset_bits;
    int dl_number;
    struct _ltube_list *next;
} LTUBE_LIST;
static LTUBE_LIST *ltube_list=NULL;



static int ltube_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{

    float yn,xn;

    if( x < WIDTH && x > -WIDTH )
    {
	sc->mc_y = Radius - FSQRT( Radius * Radius - x * x );
	if (y > sc->mc_y - BBOX_MARGIN) {
	    xn = -x;
	    yn = Radius - sc->mc_y;
	    NORMALIZE2(xn,yn);
	    sc->mc_normal[0] = xn;
	    sc->mc_normal[1] = yn;
	    sc->mc_normal[2] = 0.0;
	}
	else {
	    get_box_mc_normal(obj,x,y,z,sc->mc_normal);
	}
    }
    else
    {
	sc->mc_y = HEIGHT;
	if (y > sc->mc_y - BBOX_MARGIN) {
	    sc->mc_normal[0] = 0.0;
	    sc->mc_normal[1] = 1.0;
	    sc->mc_normal[2] = 0.0;
	}
	else {
	    get_box_mc_normal(obj,x,y,z,sc->mc_normal);
	}
    }
    return 1;
}

static int high_res_model(
    DRIVE_OBJECT *obj,
    hwObject *oList )
{
    float r = Radius;
    float len = Length/2.0;
    float height = HEIGHT;
    float width =  WIDTH;
    float pgon[100 * 4];
    float theta;
    float *ptr;
    int steps,i;
    hwObject curr;
    int nObjs = 0;

    ptr = pgon;
    steps = 90.0 / DELTA + 1;
    for(i=0;i<steps;i++)    /* First row of quad mesh */
    {
	theta = -135 + i * DELTA;
	*ptr++ = r * (float)cos( (double)(theta deg) );
	*ptr++ = r + r * (float)sin( (double)(theta deg) );
	*ptr++ = -len;
    }

    for(i=0;i<steps;i++)    /* Second row of quad mesh */
    {
	theta = -135 + i * DELTA;
	*ptr++ = r * (float)cos( (double)(theta deg) );
	*ptr++ = r + r * (float)sin( (double)(theta deg) );
	*ptr++ = len;
    }

    curr = hwMesh->create( hwMesh );
    HW_MODIFY_1I( curr, hwStrGraphN, 2 );
    HW_MODIFY_1I( curr, hwStrGraphM, steps );
    HW_MODIFY_3F( curr, hwStrColor, 0.5, 0.5, 0.5 );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,2*steps*3), pgon );
    oList[nObjs++] = curr;

    ptr = pgon;
    *ptr++ = -width *1.2; *ptr++ = height; *ptr++ = -len;
    for(i=0;i<steps;i++)    
    {
	theta = -135 + i * DELTA;
	*ptr++ = r * (float)cos( (double)(theta deg) );
	*ptr++ = r + r * (float)sin( (double)(theta deg) );
	*ptr++ = -len;
    }
    *ptr++ = width *1.2; *ptr++ = height; *ptr++ = -len;

    *ptr++ = -width*1.2; *ptr++ = 0.0; *ptr++ = -len;
    for(i=0;i<steps;i++)    
    {
	theta = i / (float)(steps - 1);
	*ptr++ = (theta*2.0 - 1.0)*width;
	*ptr++ = 0.0;
	*ptr++ = -len;
    }
    *ptr++ = width*1.2; *ptr++ = 0.0; *ptr++ = -len;

    curr = hwMesh->create( hwMesh );
    HW_MODIFY_1I( curr, hwStrGraphN, 2 );
    HW_MODIFY_1I( curr, hwStrGraphM, (steps+2) );
    HW_MODIFY_3F( curr, hwStrColor, 0.5, 0.5, 0.5 );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,2*(steps+2)*3), pgon );
    oList[nObjs++] = curr;

    ptr = pgon;
    for(i=0;i<2*(steps+2);i++) {
	ptr[2] = len; ptr += 3;
    }

    curr = hwMesh->create( hwMesh );
    HW_MODIFY_1I( curr, hwStrGraphN, 2 );
    HW_MODIFY_1I( curr, hwStrGraphM, (steps+2) );
    HW_MODIFY_3F( curr, hwStrColor, 0.5, 0.5, 0.5 );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,2*(steps+2)*3), pgon );
    oList[nObjs++] = curr;


    /* Polygon for the right side */
    ptr = pgon;
    *ptr++ = width * 1.2;  *ptr++ = height; *ptr++ = -len;
    *ptr++ = width * 1.2;  *ptr++ = height; *ptr++ = len;
    *ptr++ = width * 1.2;  *ptr++ = 0.0; *ptr++ = len;
    *ptr++ = width * 1.2;  *ptr++ = 0.0; *ptr++ = -len;

    /* Polygon for the left side */
    *ptr++ = width * -1.2;  *ptr++ = height; *ptr++ = len;
    *ptr++ = width * -1.2;  *ptr++ = height; *ptr++ = -len;
    *ptr++ = width * -1.2;  *ptr++ = 0.0; *ptr++ = -len;
    *ptr++ = width * -1.2;  *ptr++ = 0.0; *ptr++ = len;

    /* Polygon for top right */
    *ptr++ = width;        *ptr++ = height; *ptr++ = -len;
    *ptr++ = width;        *ptr++ = height; *ptr++ = len;
    *ptr++ = width * 1.2;  *ptr++ = height; *ptr++ = len;
    *ptr++ = width * 1.2;  *ptr++ = height; *ptr++ = -len;

    /* Polygon for top left */
    *ptr++ = -width;        *ptr++ = height; *ptr++ = len;
    *ptr++ = -width;        *ptr++ = height; *ptr++ = -len;
    *ptr++ = -width * 1.2;  *ptr++ = height; *ptr++ = -len;
    *ptr++ = -width * 1.2;  *ptr++ = height; *ptr++ = len;
    curr = hwQuads->create( hwQuads );
    HW_MODIFY_3F( curr, hwStrColor, 0.5, 0.5, 0.5 );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,4*4*3), pgon );
    oList[nObjs++] = curr;

    return nObjs;
}


static int ltube_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = Radius;  /**** IS THIS RIGHT??? ****/
    return(TRUE);
}


static void create_ltube_graphics(
    DRIVE_OBJECT *obj)
{
    hwObject oList[40];
    int nObjs;

    nObjs = high_res_model( obj, oList );

    /* TBD: LOD */
    obj->display_list = createHwSegmentFromObj( oList, nObjs );
}


void init_ltube_object(
    DRIVE_OBJECT *obj)
{
    LTUBE_LIST *rl;

    if( Length == 0) Length = DEFAULT_LENGTH;
    if( Radius == 0) Radius = DEFAULT_RADIUS;

    if(debug) printf(" inside init_ltube_object() routine \n");

    obj->num_children = 0;

    /* See if we've created one like this before... */
    rl = ltube_list;
    while (rl != NULL) {
	if (IS_NEAR(rl->length,Length)
		&& (rl->nameset_bits == obj->nameset_bits)
		&& IS_NEAR(rl->radius,Radius)) {
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
	if ((rl = (LTUBE_LIST *) malloc(sizeof(LTUBE_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}
	rl->length = Length;
	rl->radius  = Radius;
	rl->nameset_bits = obj->nameset_bits;
	rl->next   = ltube_list;
    	create_ltube_graphics(obj);
	rl->dl_number = obj->display_list;
	ltube_list  = rl;
    }

    obj->surface_chars_xyz  = ltube_surface_chars_xyz;
    obj->surface_chars_bbox = ltube_surface_chars_bbox;

    /* Initial (mc) bounding box values */
    obj->bound_mc[0] = -WIDTH*1.2; ;
    obj->bound_mc[1] = 0.0;
    obj->bound_mc[2] = -Length/2.0;
    obj->bound_mc[3] = WIDTH*1.2; 
    obj->bound_mc[4] = HEIGHT + 2.0;
    obj->bound_mc[5] = Length/2.0;

    /* apply the object's xform matrix to the bounding box to put it in
     * world coordinates */
    update_wc_bounds(obj);
}
