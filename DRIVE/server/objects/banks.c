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


/* Code Module for the banked curve object */


#include <math.h>
#include <stdio.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"

#define BANK_FLOAT (ROAD_FLOAT + 0.005)

#define DEFAULT_ANGLE		DEGREES_TO_RADIANS(180.0)
#define DEFAULT_RADIUS		100.0
#define DEFAULT_HEIGHT		5.0

#define MAX_VERTS		(100)
#define TANANGLE(length,radius) \
	(2.0 * (float) atan((double)((length)/(2.0*(radius)))))

#define Width			(obj->size[SIZE_WIDTH])
#define Height			(obj->size[SIZE_HEIGHT])
#define Angle			(obj->angle)
#define Radius			(obj->radius)

typedef struct _bank_list {
    float width,angle,radius,height;
    int dl_number;
    unsigned int nameset_bits;
    struct _bank_list *next;
} BANK_LIST;
static BANK_LIST *bank_list=NULL;


static int bank_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    float inner_radius;
    float distance;
    float angle;
    float bank_height;

    /* Check to see if we're really on the bank. */
    distance = x * x + z * z;
    if ( distance > Radius * Radius )
	return FALSE;
    inner_radius = Radius - Width;
    if ( distance < inner_radius * inner_radius )
	return FALSE;

    /* Now check angle. */
    angle = FATAN2( z, x );
    if (Angle > 0.0)
    {
	if ((Angle > M_PI) && (angle < 0.0))
	    angle += (2 * M_PI);
	if ((angle > Angle) || (angle < 0.0))
	    return FALSE;
    }
    else
    {
	if ((Angle < -M_PI) && (angle > 0.0))
	    angle -= (2 * M_PI);
	if ((angle < Angle) || (angle > 0.0))
	    return FALSE;
    }

    /* If we're here, must be on the bank. */
    distance = FSQRT( distance );
    bank_height = FSIN( (angle/Angle) * M_PI ) * Height;
    sc->mc_y = ((distance - inner_radius) / Width) * bank_height;

    return TRUE;
}


static void create_bank_graphics(
    DRIVE_OBJECT *obj)
{
#if defined(HOVERWARE_MODEL) /* [ */
    float wall[MAX_VERTS*2*3], *wallptr;
    float bank[MAX_VERTS*2*3], *bankptr;
    float bottom[MAX_VERTS*2*3], *bottomptr;
    float bank_height;		/* Height of bank along curve. */
    float bank_fraction;	/* Fraction of distance along curve. */
    int bank_verts;		/* Number of vertices in bank. */
    float bank_angle, sinangle, cosangle;
    int vertex;
    hwObject hwObjs[10], group;

    /* Calculate number of vertices along the bank.  This is the same
     * as the curve object, so changes here should be reflected there.
     */
    bank_verts = (int)((Angle/(2.0 * M_PI)) * (float)MAX_VERTS);
    if (bank_verts > MAX_VERTS)
	bank_verts = MAX_VERTS;

    bankptr = (float *)bank;
    wallptr = (float *)wall;
    bottomptr = (float *)bottom;

    for (vertex=0; vertex<bank_verts; vertex++)
    {
	bank_fraction = (float)vertex / (float)(bank_verts-1);
	bank_height = FSIN( bank_fraction * M_PI ) * Height;
	bank_angle = bank_fraction * Angle;
	sinangle = FSIN( bank_angle );
	cosangle = FCOS( bank_angle );

	/* Banked road. */

	*bankptr++ = Radius * cosangle;		/* Upper X */
	*bankptr++ = BANK_FLOAT + bank_height;	/* Upper Y */
	*bankptr++ = Radius * sinangle;		/* Upper Z */

	*bankptr++ = (Radius-Width) * cosangle;	/* Lower X */
	*bankptr++ = BANK_FLOAT;		/* Lower Y */
	*bankptr++ = (Radius-Width) * sinangle;	/* Lower Z */

	/* Back wall. */

	*wallptr++ = Radius * cosangle;		/* Lower X */
	*wallptr++ = BANK_FLOAT;		/* Lower Y */
	*wallptr++ = Radius * sinangle;		/* Lower Z */

	*wallptr++ = Radius * cosangle;		/* Upper X */
	*wallptr++ = BANK_FLOAT + bank_height;	/* Upper Y */
	*wallptr++ = Radius * sinangle;		/* Upper Z */

	/* Bottom. */

	*bottomptr++ = (Radius-Width) * cosangle;	/* Inner X */
	*bottomptr++ = BANK_FLOAT;			/* Inner Y */
	*bottomptr++ = (Radius-Width) * sinangle;	/* Inner Z */

	*bottomptr++ = Radius * cosangle;		/* Outer X */
	*bottomptr++ = BANK_FLOAT;			/* Outer Y */
	*bottomptr++ = Radius * sinangle;		/* Outer Z */
    }


    hwObjs[0] = hwStrip->create( hwStrip );
    hwObjs[0]->name = 0;
    ASPHALT_HW( hwObjs[0] );
    HW_MODIFY_1I( hwObjs[0], hwStrGraphN, bank_verts*2 );
    HW_MODIFY_1B( hwObjs[0], hwStrBackface, HW_FALSE );
    hwObjs[0]->modify( hwObjs[0], hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,3*bank_verts*2), bank );

    hwObjs[1] = hwStrip->create( hwStrip );
    hwObjs[1]->name = 0;
    CONCRETE_HW( hwObjs[1] );
    HW_MODIFY_1I( hwObjs[1], hwStrGraphN, bank_verts*2 );
    HW_MODIFY_1B( hwObjs[1], hwStrBackface, HW_FALSE );
    hwObjs[1]->modify( hwObjs[1], hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,3*bank_verts*2), wall );

    hwObjs[2] = hwStrip->create( hwStrip );
    hwObjs[2]->name = 0;
    CONCRETE_HW( hwObjs[2] );
    HW_MODIFY_1I( hwObjs[2], hwStrGraphN, bank_verts*2 );
    HW_MODIFY_1B( hwObjs[2], hwStrBackface, HW_FALSE );
    hwObjs[2]->modify( hwObjs[2], hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,3*bank_verts*2), bottom );

#if 0 /* [ */
    open_segment( img_fildes, stripe_seg, FALSE, FALSE );
	HANDLE_OBJECT_NAMESET(img_fildes,obj);
	vertex_format( img_fildes, 3, 3, 0, 0, CLOCKWISE );
	ROAD_LINE_YELLOW( img_fildes );
	d_phi    = TANANGLE(ROAD_LINE_LENGTH,Radius-Width/2.0);
	jump_phi = TANANGLE(ROAD_LINE_SPACING,Radius-Width/2.0);
	r1 = Radius - Width/2.0 - ROAD_LINE_WIDTH/2.0;
	r2 = r1 + ROAD_LINE_WIDTH;
	inner_radius = Radius - Width;

	for (phi=0.0; phi<Angle; phi+=jump_phi) {
	    stripeptr = (float *)stripe;

	    bank_fraction = phi / Angle;
	    bank_height = FSIN( bank_fraction * M_PI ) * Height;
	    stripe_inner_height = ((r1 - inner_radius) / Width) * bank_height;
	    stripe_outer_height = ((r2 - inner_radius) / Width) * bank_height;

	    *stripeptr++ = r2 * FCOS(phi);
	    *stripeptr++ = stripe_outer_height + BANK_FLOAT + ROAD_LINE_FLOAT;
	    *stripeptr++ = r2 * FSIN(phi);
	    *stripeptr++ = FSIN(phi) * 0.707;
	    *stripeptr++ = 0.707;
	    *stripeptr++ = FCOS(phi) * 0.707;

	    *stripeptr++ = r1 * FCOS(phi);
	    *stripeptr++ = stripe_inner_height + BANK_FLOAT + ROAD_LINE_FLOAT;
	    *stripeptr++ = r1 * FSIN(phi);
	    *stripeptr++ = -FSIN(phi) * 0.707;
	    *stripeptr++ = 0.707;
	    *stripeptr++ = -FCOS(phi) * 0.707;

	    if ((phi2 = phi + d_phi) > Angle)
		phi2 = Angle;

	    bank_fraction = phi2 / Angle;
	    bank_height = FSIN( bank_fraction * M_PI ) * Height;
	    stripe_inner_height = ((r1 - inner_radius) / Width) * bank_height;
	    stripe_outer_height = ((r2 - inner_radius) / Width) * bank_height;

	    *stripeptr++ = r2 * FCOS(phi2);
	    *stripeptr++ = stripe_outer_height + BANK_FLOAT + ROAD_LINE_FLOAT;
	    *stripeptr++ = r2 * FSIN(phi2);
	    *stripeptr++ = -FSIN(phi) * 0.707;
	    *stripeptr++ = 0.707;
	    *stripeptr++ = -FCOS(phi) * 0.707;

	    *stripeptr++ = r1 * FCOS(phi2);
	    *stripeptr++ = stripe_inner_height + BANK_FLOAT + ROAD_LINE_FLOAT;
	    *stripeptr++ = r1 * FSIN(phi2);
	    *stripeptr++ = FSIN(phi) * 0.707;
	    *stripeptr++ = 0.707;
	    *stripeptr++ = FCOS(phi) * 0.707;

	    triangular_strip( img_fildes, stripe, 4, NULL );
	}
	RESTORE_DEFAULT_VERTEX_FORMAT( img_fildes );
	DEFAULT_OBJECT_NAMESET(img_fildes,obj);
    close_segment( img_fildes );
#endif /* ] */

    group = hwGroup->create( hwGroup );
    group->modify( group, hwStrChildren,
		HW_MAKE_TYPE(HW_TYPE_OBJECT,3), hwObjs );

    obj->display_list = createHwSegmentFromObj( &group, 1 );
#else /* ] [ */
    float wall[MAX_VERTS*2*3], *wallptr;
    float bank[MAX_VERTS*2*3], *bankptr;
    float bottom[MAX_VERTS*2*3], *bottomptr;
    float stripe[4][6], *stripeptr;
    float bank_height;		/* Height of bank along curve. */
    float bank_fraction;	/* Fraction of distance along curve. */
    int bank_verts;		/* Number of vertices in bank. */
    float bank_angle, sinangle, cosangle;
    float stripe_inner_height, stripe_outer_height;
    float inner_radius;
    int vertex;
    float phi,phi2,d_phi,jump_phi,r1,r2;
    int   bank_seg,asphalt_seg,stripe_seg;
    float mc_extent[2][3];

    mc_extent[0][0] = -Radius;
	mc_extent[0][1] = 0.0;
	mc_extent[0][2] = -Radius;
    mc_extent[1][0] =  Radius;
	mc_extent[1][1] = BANK_FLOAT + Height;
	mc_extent[1][2] =  Radius;

    bank_seg  = get_dl_segment();
    asphalt_seg  = get_dl_segment();
    stripe_seg = get_dl_segment();
    obj->display_list = bank_seg;

    /* Calculate number of vertices along the bank.  This is the same
     * as the curve object, so changes here should be reflected there.
     */
    bank_verts = (int)((Angle/(2.0 * M_PI)) * (float)MAX_VERTS);
    if (bank_verts > MAX_VERTS)
	bank_verts = MAX_VERTS;

    bankptr = (float *)bank;
    wallptr = (float *)wall;
    bottomptr = (float *)bottom;

    for (vertex=0; vertex<bank_verts; vertex++)
    {
	bank_fraction = (float)vertex / (float)(bank_verts-1);
	bank_height = FSIN( bank_fraction * M_PI ) * Height;
	bank_angle = bank_fraction * Angle;
	sinangle = FSIN( bank_angle );
	cosangle = FCOS( bank_angle );

	/* Banked road. */

	*bankptr++ = Radius * cosangle;		/* Upper X */
	*bankptr++ = BANK_FLOAT + bank_height;	/* Upper Y */
	*bankptr++ = Radius * sinangle;		/* Upper Z */

	*bankptr++ = (Radius-Width) * cosangle;	/* Lower X */
	*bankptr++ = BANK_FLOAT;		/* Lower Y */
	*bankptr++ = (Radius-Width) * sinangle;	/* Lower Z */

	/* Back wall. */

	*wallptr++ = Radius * cosangle;		/* Lower X */
	*wallptr++ = BANK_FLOAT;		/* Lower Y */
	*wallptr++ = Radius * sinangle;		/* Lower Z */

	*wallptr++ = Radius * cosangle;		/* Upper X */
	*wallptr++ = BANK_FLOAT + bank_height;	/* Upper Y */
	*wallptr++ = Radius * sinangle;		/* Upper Z */

	/* Bottom. */

	*bottomptr++ = (Radius-Width) * cosangle;	/* Inner X */
	*bottomptr++ = BANK_FLOAT;			/* Inner Y */
	*bottomptr++ = (Radius-Width) * sinangle;	/* Inner Z */

	*bottomptr++ = Radius * cosangle;		/* Outer X */
	*bottomptr++ = BANK_FLOAT;			/* Outer Y */
	*bottomptr++ = Radius * sinangle;		/* Outer Z */
    }


    open_segment( img_fildes, asphalt_seg, FALSE, FALSE );
	HANDLE_OBJECT_NAMESET(img_fildes,obj);
	vertex_format( img_fildes, 0, 0, 0, 0, CLOCKWISE );

	ASPHALT( img_fildes );
	triangular_strip( img_fildes, bank, bank_verts*2, NULL );

	CONCRETE(img_fildes);
	triangular_strip( img_fildes, wall, bank_verts*2, NULL );
	triangular_strip( img_fildes, bottom, bank_verts*2, NULL );
	RESTORE_DEFAULT_VERTEX_FORMAT( img_fildes );
	DEFAULT_OBJECT_NAMESET(img_fildes,obj);
    close_segment( img_fildes );

    open_segment( img_fildes, bank_seg, FALSE, FALSE );
	HANDLE_OBJECT_NAMESET(img_fildes,obj);
	call_segment(img_fildes,asphalt_seg);
	set_extent( img_fildes, mc_extent );
	set_cull_size( img_fildes, Radius );
	cond_call_segment( img_fildes, CI_CULL, FALSE, stripe_seg );
	DEFAULT_OBJECT_NAMESET(img_fildes,obj);
    close_segment( img_fildes );

    open_segment( img_fildes, stripe_seg, FALSE, FALSE );
	HANDLE_OBJECT_NAMESET(img_fildes,obj);
	vertex_format( img_fildes, 3, 3, 0, 0, CLOCKWISE );
	ROAD_LINE_YELLOW( img_fildes );
	d_phi    = TANANGLE(ROAD_LINE_LENGTH,Radius-Width/2.0);
	jump_phi = TANANGLE(ROAD_LINE_SPACING,Radius-Width/2.0);
	r1 = Radius - Width/2.0 - ROAD_LINE_WIDTH/2.0;
	r2 = r1 + ROAD_LINE_WIDTH;
	inner_radius = Radius - Width;

	for (phi=0.0; phi<Angle; phi+=jump_phi) {
	    stripeptr = (float *)stripe;

	    bank_fraction = phi / Angle;
	    bank_height = FSIN( bank_fraction * M_PI ) * Height;
	    stripe_inner_height = ((r1 - inner_radius) / Width) * bank_height;
	    stripe_outer_height = ((r2 - inner_radius) / Width) * bank_height;

	    *stripeptr++ = r2 * FCOS(phi);
	    *stripeptr++ = stripe_outer_height + BANK_FLOAT + ROAD_LINE_FLOAT;
	    *stripeptr++ = r2 * FSIN(phi);
	    *stripeptr++ = FSIN(phi) * 0.707;
	    *stripeptr++ = 0.707;
	    *stripeptr++ = FCOS(phi) * 0.707;

	    *stripeptr++ = r1 * FCOS(phi);
	    *stripeptr++ = stripe_inner_height + BANK_FLOAT + ROAD_LINE_FLOAT;
	    *stripeptr++ = r1 * FSIN(phi);
	    *stripeptr++ = -FSIN(phi) * 0.707;
	    *stripeptr++ = 0.707;
	    *stripeptr++ = -FCOS(phi) * 0.707;

	    if ((phi2 = phi + d_phi) > Angle)
		phi2 = Angle;

	    bank_fraction = phi2 / Angle;
	    bank_height = FSIN( bank_fraction * M_PI ) * Height;
	    stripe_inner_height = ((r1 - inner_radius) / Width) * bank_height;
	    stripe_outer_height = ((r2 - inner_radius) / Width) * bank_height;

	    *stripeptr++ = r2 * FCOS(phi2);
	    *stripeptr++ = stripe_outer_height + BANK_FLOAT + ROAD_LINE_FLOAT;
	    *stripeptr++ = r2 * FSIN(phi2);
	    *stripeptr++ = -FSIN(phi) * 0.707;
	    *stripeptr++ = 0.707;
	    *stripeptr++ = -FCOS(phi) * 0.707;

	    *stripeptr++ = r1 * FCOS(phi2);
	    *stripeptr++ = stripe_inner_height + BANK_FLOAT + ROAD_LINE_FLOAT;
	    *stripeptr++ = r1 * FSIN(phi2);
	    *stripeptr++ = FSIN(phi) * 0.707;
	    *stripeptr++ = 0.707;
	    *stripeptr++ = FCOS(phi) * 0.707;

	    triangular_strip( img_fildes, stripe, 4, NULL );
	}
	RESTORE_DEFAULT_VERTEX_FORMAT( img_fildes );
	DEFAULT_OBJECT_NAMESET(img_fildes,obj);
    close_segment( img_fildes );
#endif /* ] */
}


void init_bank_object(
    DRIVE_OBJECT *obj)
{
    BANK_LIST *rl;

    if (Width <= 0.0)
	Width = DEFAULT_ROAD_WIDTH;
    if (Angle <= 0.0)
	Angle = DEFAULT_ANGLE;
    if (Radius <= 0.0)
	Radius = DEFAULT_RADIUS;
    if (Height <= 0.0)
	Height = DEFAULT_HEIGHT;

    /* If the Width is > than the radius, we have a degenerate condition which
     * causes some serious problems.  So, don't allow it to happen.
     */

    if( Width > Radius) Width = Radius;

    if(debug) printf(" inside init_bank_%d_object() routine \n",(int)Radius*2);

    obj->num_children = 0;

    /* See if we've created one like this before... */
    rl = bank_list;
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
	if ((rl = (BANK_LIST *) malloc(sizeof(BANK_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}
	rl->width  = Width;
	rl->height = Height;
	rl->angle  = Angle;
	rl->radius = Radius;
	rl->nameset_bits = obj->nameset_bits;
	rl->next   = bank_list;
    	create_bank_graphics(obj);
	rl->dl_number = obj->display_list;
	bank_list  = rl;
    }

    /* xform, ixform initialized elsewhere */

    obj->surface_chars_xyz = bank_surface_chars_xyz;

    /* Initialize bounding box values */
    if ((Angle > M_PI)
	    || (Angle < -M_PI)) {
	obj->bound_mc[0] = -Radius;
    }
    else {
	obj->bound_mc[0] = FCOS(Angle)*Radius;
    }
    obj->bound_mc[1] = 0.0;
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
    obj->bound_mc[4] = Height;
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
