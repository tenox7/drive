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


/* Code module for the pond object */


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"

#define DEFAULT_POND_LENGTH	(10.0)
#define DEFAULT_POND_WIDTH	(15.0)
#define DEFAULT_POND_HEIGHT	(3.0)

#define POND_OFFSET		(0.1)

#define Length	(obj->size[SIZE_LENGTH])
#define Width	(obj->size[SIZE_WIDTH])
#define Height	(obj->size[SIZE_HEIGHT])

#define FUDGE_VALUE	(BOUNDED_FLOATRAND(0.8,1.0))

/*****************************************************************
 *
 */
static double octagon1[8][2] = 
{
    { 0.9238795325112,  0.3826834323650},
    { 0.3826834323650,  0.9238795325112},
    {-0.3826834323650,  0.9238795325112},
    {-0.9238795325112,  0.3826834323650},
    {-0.9238795325112, -0.3826834323650},
    {-0.3826834323650, -0.9238795325112},
    { 0.3826834323650, -0.9238795325112},
    { 0.9238795325112, -0.3826834323650}
};

static double octagon2[8][2] = 
{
    { 1.0000000,  0.0000000},
    { M_SQRT1_2,  M_SQRT1_2},
    { 0.0000000,  1.0000000},
    {-M_SQRT1_2,  M_SQRT1_2},
    {-1.0000000,  0.0000000},
    {-M_SQRT1_2, -M_SQRT1_2},
    {-0.0000000, -1.0000000},
    { M_SQRT1_2, -M_SQRT1_2}
};

/*****************************************************************
 *
 */
typedef struct _pond_list
{
    float length, width, height;
    unsigned int nameset_bits;
    int dl_number;
    struct _pond_list *next;

} POND_LIST;

static POND_LIST *pond_list=NULL;

/*****************************************************************
 *
 */
static int pond_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    float xsq, ysq, zsq;
    float a, b, asq, bsq, hsq;

    a = Width / 2.0;
    b = Length / 2.0;
    asq = a * a;
    bsq = b * b;
    hsq = Height * Height;
    xsq = x * x;
    zsq = z * z;
    ysq = hsq * ( 1.0 - (xsq/asq) - (zsq/bsq) );

    if ( ysq <= 0.0 )
    {
	sc->friction = 1.0;
	return FALSE;
    }
    else
    {
	/* Set coefficient of friction to be low. */
	sc->friction = 0.8;

	sc->mc_normal[0] = 0.0;
	sc->mc_normal[1] = 1.0;
	sc->mc_normal[2] = 0.0;

	/* Find sqrt( Ax^2 + Bz^2 + C ). */
	sc->mc_y = -FSQRT( ysq );

	return TRUE;
    }

}


/*****************************************************************
 *
 */
static hwObject do_grass(
    float x, float y, float z)
{
    float grass_clist[3 * 3 * 8], *grass_ptr;
    int i, blade;
    hwObject pline;
    
    grass_ptr = grass_clist;
    for ( i = 0; i < 3; i++ )
    {
	x += BOUNDED_FLOATRAND(-2.0,2.0);
	z += BOUNDED_FLOATRAND(-2.0,2.0);

	for ( blade=0; blade<3; blade++ )
	{
	    *grass_ptr++ = x;
	    *grass_ptr++ = y;
	    *grass_ptr++ = z;
	    *grass_ptr++ = 0.0; /* Move. */
	    
	    *grass_ptr++ = x + floatrand() - 0.5;
	    *grass_ptr++ = y + floatrand() * 2.0;
	    *grass_ptr++ = z + floatrand() - 0.5;
	    *grass_ptr++ = 1.0; /* Draw. */
	}
    }
    pline = hwPolyline->create( hwPolyline );
    pline->name = 0;
    HW_MODIFY_1B( pline, hwStrHasFlags, HW_TRUE );
    HW_MODIFY_3F( pline, hwStrColor, 0.2, 0.7, 0.1 );
    pline->modify( pline, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT, 3*3*2*4), grass_clist );
    return pline;
}


/*****************************************************************
 *
 */
static hwObject draw_pond(
    float l, float w)
{
    hwObject group, curr, objs[18];
    int i, nObjs = 0;
    float x, z;
    float *floatptr, *pondptr;
    float radius_x = w / 2.0;
    float radius_z = l / 2.0;
    float inner_radius_x = radius_x * 0.75;
    float inner_radius_z = radius_z * 0.75;
    float grass_radius_x = radius_x * 0.95;
    float grass_radius_z = radius_z * 0.95;
    float outer_radius_x = radius_x * 1.25;
    float outer_radius_z = radius_z * 1.25;
    float pond_center[48];
    float pond_border[108];

    /* Compute vertices of octagonal pond center polygon. */
    floatptr = pond_center;
    for ( i=0; i<8; i++ )
    {
	*floatptr++ = inner_radius_x * octagon1[i][0]; /* X */
	*floatptr++ = Y_GROUND + POND_OFFSET;	       /* Y */
	*floatptr++ = inner_radius_z * octagon1[i][1]; /* Z */
	*floatptr++ = 0.2 * FUDGE_VALUE; 	       /* R */
	*floatptr++ = 0.5 * FUDGE_VALUE;	       /* G */
	*floatptr++ = 0.8 * FUDGE_VALUE; 	       /* B */
    }

    curr = hwPolygon->create( hwPolygon );
    curr->name = 0;
    HW_MODIFY_1B( curr, hwStrHasRGB, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
    curr->modify( curr, hwStrData, HW_MAKE_TYPE(HW_TYPE_FLOAT,8*6),
				pond_center );
    objs[nObjs++] = curr;

    /* Compute vertices of octagonal pond border tri-strip. */
    pondptr = pond_center;
    floatptr = pond_border;
    for ( i=0; i<8; i++ )
    {
	*floatptr++ = outer_radius_x * octagon2[i][0]; /* X */
	*floatptr++ = Y_GROUND - POND_OFFSET;	       /* Y */
	*floatptr++ = outer_radius_z * octagon2[i][1]; /* Z */
	*floatptr++ = 0.2 * FUDGE_VALUE; 	       /* R */
	*floatptr++ = 0.5 * FUDGE_VALUE;	       /* G */
	*floatptr++ = 0.1 * FUDGE_VALUE; 	       /* B */

	*floatptr++ = *pondptr++;		       /* X */
	*floatptr++ = *pondptr++;		       /* Y */
	*floatptr++ = *pondptr++;		       /* Z */
	*floatptr++ = *pondptr++;		       /* R */
	*floatptr++ = *pondptr++;		       /* G */
	*floatptr++ = *pondptr++;		       /* B */
    }
    *floatptr++ = pond_border[0];
    *floatptr++ = pond_border[1];
    *floatptr++ = pond_border[2];
    *floatptr++ = pond_border[3];
    *floatptr++ = pond_border[4];
    *floatptr++ = pond_border[5];

    *floatptr++ = pond_center[0];
    *floatptr++ = pond_center[1];
    *floatptr++ = pond_center[2];
    *floatptr++ = pond_center[3];
    *floatptr++ = pond_center[4];
    *floatptr++ = pond_center[5];

    curr = hwStrip->create( hwStrip );
    curr->name = 0;
    HW_MODIFY_1B( curr, hwStrHasRGB, HW_TRUE );
    HW_MODIFY_1I( curr, hwStrGraphN, 18 );
    curr->modify( curr, hwStrData, HW_MAKE_TYPE(HW_TYPE_FLOAT,18*6),
			pond_border );
    objs[nObjs++] = curr;

#if 1
    for ( i=0; i<8; i++ )
    {
	x = grass_radius_x * octagon1[i][0];
	z = grass_radius_z * octagon1[i][1];

	if ( fifty_fifty() )
	    objs[nObjs++] = do_grass( x, Y_GROUND, z );

	x = grass_radius_x * octagon2[i][0];
	z = grass_radius_z * octagon2[i][1];

	if ( fifty_fifty() )
	    objs[nObjs++] = do_grass( x, 0.0, z );
    }
#endif

    group = hwGroup->create( hwGroup );
    group->name = 0;
    group->modify( group, hwStrChildren, HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs),
				objs );
    return group;
}


/*****************************************************************
 *
 */
static void create_pond_graphics(
    DRIVE_OBJECT *obj,
    float l, float w)
{
    hwObject curr;

    curr = draw_pond( l, w );
    obj->display_list = createHwSegmentFromObj( &curr, 1 );
}


/*****************************************************************
 * init_pond_object
 * 
 *
 */
void init_pond_object(
    DRIVE_OBJECT *obj)
{
    POND_LIST *pl;

    if (Length <= 0.0)
	Length = DEFAULT_POND_LENGTH;
    if (Width <= 0.0)
	Width  = DEFAULT_POND_WIDTH;
    if (Height <= 0.0)
	Height = DEFAULT_POND_HEIGHT;

    if (debug)
	printf("Inside init_pond_%d_%d_%d_object() routine.\n",
	       (int)Length, (int)Width, (int)Height );

    obj->num_children = 0;

    /* See if we've created one like this before... */
    pl = pond_list;
    while (pl != NULL)
    {
	if (IS_NEAR(pl->length,Length)
		&& IS_NEAR(pl->width,Width)
		&& IS_NEAR(pl->height,Height)
		&& (pl->nameset_bits == obj->nameset_bits)) {
	    break;
	}
	pl = pl->next;
    }

    if (pl != NULL) 
    {
	/* Good -- I have one like this already. */
	obj->display_list = pl->dl_number;
    }
    else
    {
	/* Nope -- gotta create a new one. */
	if ((pl = (POND_LIST *)malloc(sizeof(POND_LIST))) == NULL)
	{
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}

	pl->length = Length;
	pl->width  = Width;
	pl->height  = Height;
	pl->nameset_bits = obj->nameset_bits;
	pl->next   = pond_list;

    	create_pond_graphics( obj, Length, Width );

	pl->dl_number = obj->display_list;
	pond_list = pl;
    }

    /* Initial (mc) bounding box values */
    obj->bound_mc[0] = -(Width/2.0);
    obj->bound_mc[1] = Y_GROUND - Height - BBOX_MARGIN;
    obj->bound_mc[2] = -(Length/2.0);
    obj->bound_mc[3] = Width/2.0;
    obj->bound_mc[4] = Y_GROUND + POND_OFFSET + BBOX_MARGIN;
    obj->bound_mc[5] = Length/2.0;

    /* Apply the object's xform matrix to the bounding box to put it in
     * world coordinates. */
    update_wc_bounds(obj);

    obj->surface_chars_xyz  = pond_surface_chars_xyz;
}
