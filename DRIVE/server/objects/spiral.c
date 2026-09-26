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


/* Code Module for the spiral road object */


#include <math.h>
#include <stdio.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"

#define SPIRAL_FLOAT (ROAD_FLOAT + 0.005)

/* Define thickness of spiral */
#define	SPIRAL_THICK	5.0

#define DEFAULT_ANGLE		DEGREES_TO_RADIANS(180.0)
#define DEFAULT_RADIUS		100.0
#define DEFAULT_HEIGHT		20.0

#define MAX_GRANULARITY		100	
#define TANANGLE(length,radius) \
	(2.0 * (float) atan((double)((length)/(2.0*(radius)))))

#define Width			(obj->size[SIZE_WIDTH])
#define Height			(obj->size[SIZE_HEIGHT])
#define Angle			(obj->angle)
#define Radius			(obj->radius)

typedef struct _spiral_list {
    float width,angle,radius,height;
    unsigned int nameset_bits;
    int dl_number;
    struct _spiral_list *next;
} SPIRAL_LIST;
static SPIRAL_LIST *spiral_list=NULL;


static int spiral_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    float r2,rmin,angle;
    float h;

    /* check to see if we're really on the spiral. */
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

    h = Height * (angle / Angle);

    if( y + SPIRAL_THICK/2.0 < h ) 
	return(FALSE);

    /* Set the height */
    sc->mc_y = h;

    return(TRUE);
}


static void create_spiral_graphics(
    DRIVE_OBJECT *obj)
{
    float spiral[MAX_GRANULARITY*2][3],pgon[4096*3],*ptr;
    float wall[MAX_GRANULARITY*2][3];
    float iwall[MAX_GRANULARITY*2][3];
    float phi,phi2,d_phi,jump_phi,r1,r2, h, h2;
    float *wptr1, *wptr2;
    int i, GRANULARITY;
    hwObject oList[100], curr;
    int nObjs = 0, nPts = 0;

    /* Angle is in Radians -- calculate the granularity by multiplying the
     * angle by a constant.  The largest usual angle will be 360 degrees 
     * (which is 2 PI radians), so multiply the Angle by max_granularity
     * divided by the max angle */

    GRANULARITY = (int) ( (Angle) * (MAX_GRANULARITY / (2 * M_PI)));
    if (GRANULARITY > MAX_GRANULARITY ) GRANULARITY = MAX_GRANULARITY;

    ptr = (float *) spiral;
    wptr1 = (float *) wall;
    wptr2 = (float *) ((float *)wall + (GRANULARITY *3) );

    /* outside edge of mesh */
    for (i=0,phi = Angle; i<GRANULARITY; ++i,phi -= (Angle/(GRANULARITY-1))) {
	h = Height * (phi / Angle);
	*wptr1++ = *wptr2++ = *ptr++ = Radius * FCOS(phi);
	*wptr2++ = *ptr++ = h + SPIRAL_FLOAT;
	*wptr1++ =  h - SPIRAL_THICK;
	*wptr1++ = *wptr2++ = *ptr++ = Radius * FSIN(phi);
    }
    /* inside edge of mesh */

    wptr1 = (float *) iwall;
    wptr2 = (float *) ((float *)iwall + (GRANULARITY *3) );

    for (i=0,phi = Angle; i<GRANULARITY; ++i,phi -= (Angle/(GRANULARITY-1))) {
	h = Height * (phi / Angle);
	*wptr1++ = *wptr2++ = *ptr++ = (Radius-Width) * FCOS(phi);
	*wptr1++ = *ptr++ = h + SPIRAL_FLOAT;
	*wptr2++ = h - SPIRAL_THICK;
	*wptr1++ = *wptr2++ = *ptr++ = (Radius-Width) * FSIN(phi);
    }

    curr = hwMesh->create( hwMesh );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
    HW_MODIFY_1I( curr, hwStrGraphN, 2 );
    HW_MODIFY_1I( curr, hwStrGraphM, GRANULARITY );
    curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,2*GRANULARITY*3), spiral );
    ASPHALT_HW(curr);
    oList[nObjs++] = curr;

    if( SPIRAL_THICK > 0.0 )
    {
	curr = hwMesh->create( hwMesh );
	HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
	HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
	HW_MODIFY_1I( curr, hwStrGraphN, 2 );
	HW_MODIFY_1I( curr, hwStrGraphM, GRANULARITY );
	curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,2*GRANULARITY*3), wall );
	CONCRETE_HW(curr);
	oList[nObjs++] = curr;

	curr = hwMesh->create( hwMesh );
	HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
	HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
	HW_MODIFY_1I( curr, hwStrGraphN, 2 );
	HW_MODIFY_1I( curr, hwStrGraphM, GRANULARITY );
	curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,2*GRANULARITY*3), iwall );
	CONCRETE_HW(curr);
	oList[nObjs++] = curr;

	/* Draw the bottom */
	ptr = (float *) spiral;
	for (i=GRANULARITY-1; i>=0;i--) 
	{
	    h = Height * (i / (float)(GRANULARITY-1));
	    ptr++ ;  /* skip X */
	    *ptr++ = h - SPIRAL_THICK;  /* Bottom is SPIRAL_THICK below... */
	    ptr++;   /* skip Z */
	}
	for (i=GRANULARITY-1; i>=0;i--) 
	{
	    h = Height * (i / (float)(GRANULARITY-1));
	    ptr++ ;  /* skip X */
	    *ptr++ = h - SPIRAL_THICK;  /* Bottom is SPIRAL_THICK below... */
	    ptr++;   /* skip Z */
	}

	curr = hwMesh->create( hwMesh );
	HW_MODIFY_1I( curr, hwStrGraphN, 2 );
	HW_MODIFY_1I( curr, hwStrGraphM, GRANULARITY );
	curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,2*GRANULARITY*3), spiral );
	HW_MODIFY_3F( curr, hwStrColor, 0.45, 0.45, 0.45 );
	oList[nObjs++] = curr;
    }

    d_phi    = TANANGLE(ROAD_LINE_LENGTH,Radius-Width/2.0);
    jump_phi = TANANGLE(ROAD_LINE_SPACING,Radius-Width/2.0);
    r1 = Radius - Width/2.0 - ROAD_LINE_WIDTH/2.0;
    r2 = r1 + ROAD_LINE_WIDTH;
    ptr = (float *) pgon;
    for (phi=0.0; phi<Angle; phi+=jump_phi) {
	h = Height * (phi / Angle);

	if ((phi2 = phi + d_phi) > Angle) phi2 = Angle;
	h2 = Height * (phi2 / Angle);

	*ptr++ = r2 * FCOS(phi2);
	*ptr++ = h2 + SPIRAL_FLOAT + ROAD_LINE_FLOAT;
	*ptr++ = r2 * FSIN(phi2);

	*ptr++ = r1 * FCOS(phi2);
	*ptr++ = h2 + SPIRAL_FLOAT + ROAD_LINE_FLOAT;
	*ptr++ = r1 * FSIN(phi2);

	*ptr++ = r1 * FCOS(phi);
	*ptr++ = h + SPIRAL_FLOAT + ROAD_LINE_FLOAT;
	*ptr++ = r1 * FSIN(phi);

	*ptr++ = r2 * FCOS(phi);
	*ptr++ = h + SPIRAL_FLOAT + ROAD_LINE_FLOAT;
	*ptr++ = r2 * FSIN(phi);

	nPts += 4;
    }
    curr = hwQuads->create( hwQuads );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
    curr->modify( curr, hwStrData, HW_MAKE_TYPE(HW_TYPE_FLOAT,nPts*3), pgon );
    ROAD_LINE_YELLOW_HW(curr);
    oList[nObjs++] = curr;
    /* TBD: LOD */

    curr = hwGroup->create( hwGroup );
    curr->modify( curr, hwStrChildren,
		HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
    HW_OBJECT_NAMESET(curr,obj);

    obj->display_list = createHwSegmentFromObj( &curr, 1 );
}


void init_spiral_object(
    DRIVE_OBJECT *obj)
{
    SPIRAL_LIST *rl;

    if (Width <= 0.0)
	Width = DEFAULT_ROAD_WIDTH;
    if (Angle <= 0.0)
	Angle = DEFAULT_ANGLE;
    if (Radius <= 0.0)
	Radius = DEFAULT_RADIUS;
    if (fabs(Height) < 1.0)
	Height = DEFAULT_HEIGHT;

    if(debug) printf(" inside init_spiral_%d_object() routine \n",(int)Radius*2);

    obj->num_children = 0;

    /* See if we've created one like this before... */
    rl = spiral_list;
    while (rl != NULL) {
	if (IS_NEAR(rl->width,Width)
		&& IS_NEAR(rl->angle,Angle)
		&& IS_NEAR(rl->height,Height)
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
	if ((rl = (SPIRAL_LIST *) malloc(sizeof(SPIRAL_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}
	rl->width  = Width;
	rl->height = Height;
	rl->angle  = Angle;
	rl->radius = Radius;
	rl->nameset_bits = obj->nameset_bits;
	rl->next   = spiral_list;
    	create_spiral_graphics(obj);
	rl->dl_number = obj->display_list;
	spiral_list  = rl;
    }

    /* xform, ixform initialized elsewhere */

    obj->surface_chars_xyz = spiral_surface_chars_xyz;

    /* Initialize bounding box values */
    if ((Angle > M_PI)
	    || (Angle < -M_PI)) {
	obj->bound_mc[0] = -Radius;
    }
    else {
	obj->bound_mc[0] = FCOS(Angle)*Radius;
    }
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

    if( Height < 0.0 ) {
	obj->bound_mc[1] = Height-SPIRAL_THICK;
	obj->bound_mc[4] = 0.0;
    }
    else {
	obj->bound_mc[1] = -SPIRAL_THICK;
	obj->bound_mc[4] = Height;
    }

    update_wc_bounds(obj);
}

