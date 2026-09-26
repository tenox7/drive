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


/* Code Module for the curved road object */

#include <math.h>
#include <stdio.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"

#define CURVE_FLOAT (ROAD_FLOAT + 0.005)

#define DEFAULT_ANGLE		DEGREES_TO_RADIANS(180.0)
#define DEFAULT_RADIUS		100.0
#define DEFAULT_HEIGHT		0.0
#define DEFAULT_ROAD_COLOR	ASPHALT_INTENSITY

#define TANANGLE(length,radius) \
	(2.0 * (float) atan((double)((length)/(2.0*(radius)))))

#define Width			(obj->size[SIZE_WIDTH])
#define Height			(obj->size[SIZE_HEIGHT])
#define Angle			(obj->angle)
#define Radius			(obj->radius)

typedef struct _curve_list {
    float width,angle,radius,height;
    unsigned int nameset_bits;
    float color[3];
    int dl_number;
    struct _curve_list *next;
} CURVE_LIST;
static CURVE_LIST *curve_list=NULL;


static int curve_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    float r2,rmin,angle;

    /* check to see if we're really on the curve. */
    r2 = x*x + z*z;
    if (r2 > Radius*Radius) return(FALSE);
    rmin = Radius-Width;
    if (r2 < rmin*rmin) return(FALSE);
    /* now check angle */
    angle = FATAN2(z,x);
    if (Angle > 0.0) {
	if( (Angle > M_PI) && (angle < 0.0)  ) angle += (2 * M_PI);
	if ((angle > Angle) || (angle < 0.0)) return(FALSE);
    }
    else {
	if( (Angle < -M_PI) && (angle > 0.0) ) angle -= (2 * M_PI);
	if ((angle < Angle) || (angle > 0.0)) return(FALSE);
    }

    sc->mc_y = 0.0;
    return(TRUE);
}


static void create_curve_graphics(
    DRIVE_OBJECT *obj)
{
    float *curve,*wall,*iwall;
    float *ptr;
    float phi,phi2,d_phi,jump_phi,r1,r2,theta;
    float *wptr1, *wptr2;
    int   i;
    float *stripe_data;
    int   
	numStripes=0,
	this,
	numHwObjs=1,
	chunks;
    hwObject
	curveGroup,
	objList[5];


    /* Break the curve up into reasonable-sized chunks, based on the
     * chord of the chunk.
     */
    if ((d_phi = MAX_CURVE_CHORD/Radius) > MAX_CURVE_SUBANGLE)
	d_phi = MAX_CURVE_SUBANGLE;
    chunks = (int) ((Angle+d_phi*0.99)/d_phi) + 1;

    /* malloc space for the graphics objects */
    if ((curve = (float *) malloc(chunks*2*3*sizeof(float))) == NULL) {
	return;
    }
    if ((wall = (float *) malloc(chunks*2*3*sizeof(float))) == NULL) {
	free(curve);
	return;
    }
    if ((iwall = (float *) malloc(chunks*2*3*sizeof(float))) == NULL) {
	free(curve);
	free(wall);
	return;
    }

    /* outside edge of mesh */
    ptr   = curve;
    wptr1 = wall;
    wptr2 = wall + (chunks*3);
    for (i=0,phi=Angle; i<chunks; ++i,phi-=d_phi) {
	theta = MAX(0.0,phi);
	*wptr1++ = *wptr2++ = *ptr++ = Radius * FCOS(theta);
	*wptr2++ = *ptr++ = CURVE_FLOAT;
	*wptr1++ =  -Height;
	*wptr1++ = *wptr2++ = *ptr++ = Radius * FSIN(theta);
    }

    /* inside edge of mesh */
    wptr1 = iwall;
    wptr2 = iwall + (chunks*3);
    for (i=0,phi=Angle; i<chunks; ++i,phi-=d_phi) {
	theta = MAX(0.0,phi);
	*wptr1++ = *wptr2++ = *ptr++ = (Radius-Width) * FCOS(theta);
	*wptr1++ = *ptr++ = CURVE_FLOAT;
	*wptr2++ = -Height;
	*wptr1++ = *wptr2++ = *ptr++ = (Radius-Width) * FSIN(theta);
    }

    objList[0] = hwMesh->create(hwMesh);
    objList[0]->name = 0;
    if(		(obj->color[0] == DEFAULT_ROAD_COLOR)
	&&	(obj->color[1] == DEFAULT_ROAD_COLOR)
	&&	(obj->color[2] == DEFAULT_ROAD_COLOR) )
    {
	ASPHALT_HW( objList[0] );
    }
    else {
	ROAD_COLOR_HW(objList[0], obj->color[0], obj->color[1], obj->color[2]);
    }
    HW_MODIFY_1I(objList[0], hwStrGraphN, 2);
    HW_MODIFY_1I(objList[0], hwStrGraphM, chunks);
    HW_MODIFY_1B(objList[0], hwStrBackface, HW_TRUE);
    objList[0]->modify(objList[0], hwStrData,
	HW_MAKE_TYPE(HW_TYPE_FLOAT, chunks*2*3), curve);
    
    if (Height > 0.0) {
	for(i=1; i< 4; i++ ) {
	    objList[i] = hwMesh->create(hwMesh);
	    objList[i]->name = 0;
	    CONCRETE_HW(objList[i]);
	    HW_MODIFY_1I(objList[i], hwStrGraphN, 2);
	    HW_MODIFY_1I(objList[i], hwStrGraphM, chunks);
	    HW_MODIFY_1B(objList[i], hwStrBackface, HW_TRUE);
	}
	objList[1]->modify(objList[1], hwStrData,
	    HW_MAKE_TYPE(HW_TYPE_FLOAT, chunks*2*3), wall);

	objList[2]->modify(objList[2], hwStrData,
	    HW_MAKE_TYPE(HW_TYPE_FLOAT, chunks*2*3), iwall);

	/* Draw the bottom */
	ptr = curve;
	for (i=0; i<2*chunks; ++i) {
	    ptr++ ;  /* skip X */
	    *ptr++ = -Height;  /* Y is -Height */
	    ptr++;   /* skip Z */
	}
	HW_MODIFY_3F(objList[3],hwStrColor,0.45, 0.45, 0.45);
	HW_MODIFY_1B(objList[3], hwStrBackface, HW_FALSE);
	objList[3]->modify(objList[3], hwStrData,
	    HW_MAKE_TYPE(HW_TYPE_FLOAT, chunks*2*3), curve);

	numHwObjs = 4;
    }

    this = numHwObjs;
    numHwObjs++;
    objList[this] = hwQuads->create(hwQuads);
    objList[this]->name = 0;
    ROAD_LINE_YELLOW_HW(objList[this]);

    d_phi    = TANANGLE(ROAD_LINE_LENGTH,Radius-Width/2.0);
    jump_phi = TANANGLE(ROAD_LINE_SPACING,Radius-Width/2.0);
    r1 = Radius - Width/2.0 - ROAD_LINE_WIDTH/2.0;
    r2 = r1 + ROAD_LINE_WIDTH;

    /* allocate enough data to hold all the strips quads */
    stripe_data = (float *)malloc( (Angle/jump_phi+1)*4*6*sizeof(float));
    if(! stripe_data) exit(1);
    ptr = stripe_data;

    for (phi=0.0; phi<Angle; phi+=jump_phi) {

	if ((phi2 = phi + d_phi) > Angle) phi2 = Angle;
	*ptr++ = r2 * FCOS(phi2);
	*ptr++ = CURVE_FLOAT + ROAD_LINE_FLOAT;
	*ptr++ = r2 * FSIN(phi2);
	*ptr++ = FSIN(phi) * 0.707;
	*ptr++ = 0.707;
	*ptr++ = FCOS(phi) * 0.707;

	*ptr++ = r1 * FCOS(phi2);
	*ptr++ = CURVE_FLOAT + ROAD_LINE_FLOAT;
	*ptr++ = r1 * FSIN(phi2);
	*ptr++ = -FSIN(phi) * 0.707;
	*ptr++ =  0.707;
	*ptr++ = -FCOS(phi) * 0.707;

	*ptr++ = r1 * FCOS(phi);
	*ptr++ = CURVE_FLOAT + ROAD_LINE_FLOAT;
	*ptr++ = r1 * FSIN(phi);
	*ptr++ = -FSIN(phi) * 0.707;
	*ptr++ =  0.707;
	*ptr++ = -FCOS(phi) * 0.707;

	*ptr++ = r2 * FCOS(phi);
	*ptr++ = CURVE_FLOAT + ROAD_LINE_FLOAT;
	*ptr++ = r2 * FSIN(phi);
	*ptr++ = FSIN(phi) * 0.707;
	*ptr++ =  0.707;
	*ptr++ = FCOS(phi) * 0.707;

	numStripes++;
    }

    HW_MODIFY_1B(objList[this], hwStrBackface, HW_TRUE);
    HW_MODIFY_1B(objList[this], hwStrHasNormals, HW_TRUE);
    objList[this]->modify(objList[this],  hwStrData,
      HW_MAKE_TYPE(HW_TYPE_FLOAT, numStripes * 6 *4), stripe_data);


    free(curve);
    free(wall);
    free(iwall);
    free(stripe_data);

    curveGroup = hwGroup->create(hwGroup);
    curveGroup->name = 0; 

    curveGroup->modify(curveGroup, hwStrChildren, 
	HW_MAKE_TYPE(HW_TYPE_OBJECT,numHwObjs), objList);

    obj->display_list = createHwSegmentFromObj( &curveGroup, 1);
}


void init_curve_object(
    DRIVE_OBJECT *obj)
{
    CURVE_LIST *rl;

    if (Width <= 0.0)
	Width = DEFAULT_ROAD_WIDTH;
    if (Angle <= 0.0)
	Angle = DEFAULT_ANGLE;
    if (Radius <= 0.0)
	Radius = DEFAULT_RADIUS;
    if (Height <= 0.0)
	Height = DEFAULT_HEIGHT;
    if (obj->color[0] == DEFAULT_OBJECT_COLOR)
	obj->color[0] = DEFAULT_ROAD_COLOR;
    if (obj->color[1] == DEFAULT_OBJECT_COLOR)
	obj->color[1] = DEFAULT_ROAD_COLOR;
    if (obj->color[2] == DEFAULT_OBJECT_COLOR)
	obj->color[2] = DEFAULT_ROAD_COLOR;

    /* If the Width is > than the radius, we have a degenerate condition which
     * causes some serious problems.  So, don't allow it to happen.
     */

    if( Width > Radius) Width = Radius;

    if(debug) printf(" inside init_curve_%d_object() routine \n",(int)Radius*2);

    obj->num_children = 0;

    /* See if we've created one like this before... */
    rl = curve_list;
    while (rl != NULL) {
	if (IS_NEAR(rl->width,Width)
		&& IS_NEAR(rl->angle,Angle)
		&& (rl->nameset_bits == obj->nameset_bits)
		&& IS_NEAR(rl->height,Height)
		&& IS_NEAR(rl->color[0],obj->color[0])
		&& IS_NEAR(rl->color[1],obj->color[1])
		&& IS_NEAR(rl->color[2],obj->color[2])
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
	if ((rl = (CURVE_LIST *) malloc(sizeof(CURVE_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}
	rl->width  = Width;
	rl->height = Height;
	rl->angle  = Angle;
	rl->radius = Radius;
	rl->nameset_bits = obj->nameset_bits;
	rl->color[0] = obj->color[0];
	rl->color[1] = obj->color[1];
	rl->color[2] = obj->color[2];
	rl->next   = curve_list;
    	create_curve_graphics(obj);
	rl->dl_number = obj->display_list;
	curve_list  = rl;
    }

    /* xform, ixform initialized elsewhere */

    obj->surface_chars_xyz = curve_surface_chars_xyz;

    /* Initialize bounding box values */
    if ((Angle > M_PI)
	    || (Angle < -M_PI)) {
	obj->bound_mc[0] = -Radius;
    }
    else {
	obj->bound_mc[0] = FCOS(Angle)*Radius;
    }
    obj->bound_mc[1] = -Height;
    if ((Angle >= (M_PI*3.0/2.0)) 
	    || (Angle < -M_PI)) {
	obj->bound_mc[2] = -Radius;
    }
    else if ((Angle > M_PI)
	    || (Angle < 0.0)) {
	obj->bound_mc[2] = FSIN(Angle)*Radius;
    }
    else {
	obj->bound_mc[2] = 0.0;
    }

    obj->bound_mc[3] = Radius;
    obj->bound_mc[4] = BBOX_MARGIN;
    if ((Angle >= M_PI/2.0)
	    || (Angle < (-M_PI*3.0/2.0))) {
	obj->bound_mc[5] = Radius;
    }
    else if ((Angle > 0.0)
	    || (Angle < -M_PI)) {
	obj->bound_mc[5] = FSIN(Angle)*Radius;
    }
    else {
	obj->bound_mc[5] = 0.0;
    }

    update_wc_bounds(obj);
}
