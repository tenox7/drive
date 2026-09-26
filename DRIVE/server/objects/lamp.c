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


/* Code Module for the lamp segment object */


#include <stdio.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"

#define INCH(x)	((x)/12.0)

/* Pentagonal angles. */

static float z_factor[] =
{
    0.0,		/*  SIN0   */
    0.9510565,		/*  SIN72  */
    0.5877852,		/*  SIN144 */
    -0.5877852,		/*  SIN216 */
    -0.9510565,		/*  SIN288 */
    0.0,		/*  SIN0   */
};

static float x_factor[] =
{
    1.0,		/*  COS0	 */
    0.3090169,		/*  COS72 */
    -0.8090169,		/*  COS144 */
    -0.8090169,		/*  COS216 */
    0.3090169,		/*  COS288 */
    1.0,		/*  COS0	 */
};

#define DEFAULT_LAMP_RADIUS	INCH(6.0);
#define DEFAULT_LAMP_HEIGHT	INCH(240.0);
#define DEFAULT_LAMP_TYPE	0
#define DEFAULT_LAMP_LENGTH	INCH(60.0);

#define Length	(obj->size[SIZE_LENGTH])
#define Height	(obj->size[SIZE_HEIGHT])
#define Radius	(obj->radius)


typedef struct _lamp_list
{
    float height, radius, length;
    int type;
    unsigned int nameset_bits;
    int dl_number;
    struct _lamp_list *next;

} LAMP_LIST;

static LAMP_LIST *lamp_list=NULL;

static int lamp_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    if (x*x + z*z < Radius*Radius) {
	sc->mc_y = obj->size[SIZE_HEIGHT];
	get_pole_mc_normal(obj,x,y,z,sc->mc_normal);
	return(TRUE);
    }
    /* else */
    return(FALSE);
}



static int lamp_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = obj->size[SIZE_HEIGHT];
    return(TRUE);
}


/*****************************************************************
 * do_lamp_type_0
 * 
 *
 */
static void do_lamp_type_0(
    DRIVE_OBJECT *obj)
{
    int index;
    float *floatptr;

    float r_pole_top, h_pole_top;
    float r_base, h_base;
    float r_light_bot, h_light_bot;
    float r_light_top;
    float h_pole_bot;

    float pole[100];
    float base[100];
    float light[100];

    hwObject curr, oList[10];
    int nObjs = 0;

    /* Calculate major radii */
    r_pole_top 	= Radius * 0.5;
    r_base	= Radius * 1.5;
    r_light_bot	= Radius * 1.2;
    r_light_top	= Radius * 0.8;

    /* Calculate heights. */
    h_pole_top  = Height * 0.925;
    h_pole_bot  = Height * 0.06;
    h_base      = Height * 0.05;
    h_light_bot = Height * 0.95;


     /* Pole. */
    floatptr = pole;
    for ( index = 0; index < 6; index++ )
    {
	*floatptr++ = x_factor[index] * r_base;
	*floatptr++ = h_base;
	*floatptr++ = z_factor[index] * r_base;

	*floatptr++ = x_factor[index] * Radius;
	*floatptr++ = h_pole_bot;
	*floatptr++ = z_factor[index] * Radius;

	*floatptr++ = x_factor[index] * r_pole_top;
	*floatptr++ = h_pole_top;
	*floatptr++ = z_factor[index] * r_pole_top;

	*floatptr++ = x_factor[index] * r_light_bot;
	*floatptr++ = h_light_bot;
	*floatptr++ = z_factor[index] * r_light_bot;
    }

     /* Base. */
    floatptr = base;
    for ( index = 0; index < 6; index++ )
    {
	*floatptr++ = x_factor[index] * r_base;
	*floatptr++ = 0.0;
	*floatptr++ = z_factor[index] * r_base;

	*floatptr++ = x_factor[index] * r_base;
	*floatptr++ = h_base;
	*floatptr++ = z_factor[index] * r_base;
    }

     /* Light. */
    floatptr = light;
    for ( index = 0; index < 6; index++ )
    {
	*floatptr++ = x_factor[index] * r_light_bot;
	*floatptr++ = h_light_bot;
	*floatptr++ = z_factor[index] * r_light_bot;

	*floatptr++ = x_factor[index] * r_light_top;
	*floatptr++ = Height;
	*floatptr++ = z_factor[index] * r_light_top;

	*floatptr++ = x_factor[index] * INCH(0.5);
	*floatptr++ = Height;
	*floatptr++ = z_factor[index] * INCH(0.5);
    }


    curr = hwMesh->create( hwMesh );
    HW_MODIFY_1I( curr, hwStrGraphN, 6 );
    HW_MODIFY_1I( curr, hwStrGraphM, 4 );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
    BLACK_PLASTIC_HW(curr);
    curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,6*4*3), pole );
    oList[nObjs++] = curr;

    curr = hwMesh->create( hwMesh );
    HW_MODIFY_1I( curr, hwStrGraphN, 6 );
    HW_MODIFY_1I( curr, hwStrGraphM, 2 );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
    CHROME_HW(curr);
    curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,6*2*3), base );
    oList[nObjs++] = curr;

    curr = hwMesh->create( hwMesh );
    HW_MODIFY_1I( curr, hwStrGraphN, 6 );
    HW_MODIFY_1I( curr, hwStrGraphM, 3 );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
    WHITE_PLASTIC_HW(curr);
    curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,6*3*3), light );
    oList[nObjs++] = curr;

    /* TBD: LOD */
    curr = hwGroup->create( hwGroup );
    curr->modify( curr, hwStrChildren,
		HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
    HW_OBJECT_NAMESET(curr,obj);

    obj->display_list = createHwSegmentFromObj( &curr, 1 );
}

/*****************************************************************
 * do_lamp_type_1
 * 
 *
 */
static void do_lamp_type_1(
    DRIVE_OBJECT *obj)
{
    int index;
    float *floatptr;

    float r_pole_top, h_pole_top;
    float r_base, h_base, r_arm;
    float h_should_bot;
    float r_shoulder, h_should_top;
    float h_pole_bot, h_light, h_arm;

    float pole[200];
    float base[100];
    float arm[100], arm1[100];
    float light_box[100];
    float light[20];

    hwObject curr, oList[10];
    int nObjs = 0;

    /* Calculate major radii */
    r_pole_top 	= Radius * 0.5;
    r_base	= Radius * 1.5;
    r_shoulder	= Radius * 0.8;
    /* r_light_bot	= Radius * 1.2; */
    r_arm       = Length * 0.4;

    /* Calculate heights. */
    h_pole_top   = Height * 0.9;
    h_pole_bot   = Height * 0.06;
    h_base       = Height * 0.05;
    h_should_bot = Height * 0.95;
    h_should_top = Height * 0.975;
    h_light      = Height;
    h_arm        = Height * 0.95;

     /* Pole. */
    floatptr = pole;
    for ( index = 0; index < 6; index++ )
    {
	*floatptr++ = x_factor[index] * r_base;
	*floatptr++ = h_base;
	*floatptr++ = z_factor[index] * r_base;

	*floatptr++ = x_factor[index] * Radius;
	*floatptr++ = h_pole_bot;
	*floatptr++ = z_factor[index] * Radius;

	*floatptr++ = x_factor[index] * r_pole_top;
	*floatptr++ = h_pole_top;
	*floatptr++ = z_factor[index] * r_pole_top;

	*floatptr++ = x_factor[index] * r_shoulder;
	*floatptr++ = h_should_bot;
	*floatptr++ = z_factor[index] * r_shoulder;

	*floatptr++ = x_factor[index] * r_shoulder;
	*floatptr++ = h_should_top;
	*floatptr++ = z_factor[index] * r_shoulder;

	*floatptr++ = x_factor[index] * INCH(1.0);
	*floatptr++ = h_should_top;
	*floatptr++ = z_factor[index] * INCH(1.0);
    }

    /* Arm. */
    floatptr = arm;
    for ( index = 0; index < 6; index++ )
    {
	*floatptr++ = 0.0;
	*floatptr++ = h_arm + z_factor[index] * r_pole_top;
	*floatptr++ = x_factor[index] * r_pole_top;

	*floatptr++ = r_arm;
	*floatptr++ = (Height - r_pole_top) + z_factor[index] * r_pole_top;
	*floatptr++ = x_factor[index] * r_pole_top;
    }

    /* Arm. */
    floatptr = arm1;
    for ( index = 0; index < 6; index++ )
    {
	*floatptr++ = r_arm;
	*floatptr++ = (Height - r_pole_top) + z_factor[index] * r_pole_top;
	*floatptr++ = x_factor[index] * r_pole_top;

	*floatptr++ = Length;
	*floatptr++ = (Height - r_pole_top) + z_factor[index] * r_pole_top;
	*floatptr++ = x_factor[index] * r_pole_top;
    }

     /* Base. */
    floatptr = base;
    for ( index = 0; index < 6; index++ )
    {
	*floatptr++ = x_factor[index] * r_base;
	*floatptr++ = 0.0;
	*floatptr++ = z_factor[index] * r_base;

	*floatptr++ = x_factor[index] * r_base;
	*floatptr++ = h_base;
	*floatptr++ = z_factor[index] * r_base;
    }

     /* Light Box. */
    floatptr = light_box;

    *floatptr++ = (Length * 0.8);
    *floatptr++ = h_light;
    *floatptr++ = -Length * 0.1;

    *floatptr++ = (Length * 0.8);
    *floatptr++ = (h_light * 0.975);
    *floatptr++ = -Length * 0.1;

    /*...*/

    *floatptr++ = (Length * 0.8);
    *floatptr++ = h_light;
    *floatptr++ = Length * 0.1;

    *floatptr++ = (Length * 0.8);
    *floatptr++ = (h_light * 0.975);
    *floatptr++ = Length * 0.1;

    /*...*/

    *floatptr++ = (Length * 1.050);
    *floatptr++ = h_light;
    *floatptr++ = Length * 0.1;

    *floatptr++ = (Length * 1.050);
    *floatptr++ = (h_light * 0.975);
    *floatptr++ = Length * 0.1;

    /*...*/

    *floatptr++ = (Length * 1.050);
    *floatptr++ = h_light;
    *floatptr++ = -Length * 0.1;

    *floatptr++ = (Length * 1.050);
    *floatptr++ = (h_light * 0.975);
    *floatptr++ = -Length * 0.1;

    /*...*/

    *floatptr++ = (Length * 0.8);
    *floatptr++ = h_light;
    *floatptr++ = -Length * 0.1;

    *floatptr++ = (Length * 0.8);
    *floatptr++ = (h_light * 0.975);
    *floatptr++ = -Length * 0.1;

     /* Light. */
    floatptr = light;

    *floatptr++ = (Length * 1.050);
    *floatptr++ = (h_light * 0.975);
    *floatptr++ = -Length * 0.1;

    *floatptr++ = (Length * 1.050);
    *floatptr++ = (h_light * 0.975);
    *floatptr++ = Length * 0.1;

    *floatptr++ = (Length * 0.8);
    *floatptr++ = (h_light * 0.975);
    *floatptr++ = Length * 0.1;

    *floatptr++ = (Length * 0.8);
    *floatptr++ = (h_light * 0.975);
    *floatptr++ = -Length * 0.1;

    curr = hwMesh->create( hwMesh );
    HW_MODIFY_1I( curr, hwStrGraphN, 5 );
    HW_MODIFY_1I( curr, hwStrGraphM, 2 );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
    BLACK_PLASTIC_HW(curr);
    curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,5*2*3), light_box );
    oList[nObjs++] = curr;

    curr = hwMesh->create( hwMesh );
    HW_MODIFY_1I( curr, hwStrGraphN, 6 );
    HW_MODIFY_1I( curr, hwStrGraphM, 6 );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
    CHROME_HW(curr);
    curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,6*6*3), pole );
    oList[nObjs++] = curr;

    curr = hwMesh->create( hwMesh );
    HW_MODIFY_1I( curr, hwStrGraphN, 6 );
    HW_MODIFY_1I( curr, hwStrGraphM, 2 );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
    CHROME_HW(curr);
    curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,6*2*3), arm );
    oList[nObjs++] = curr;

    curr = hwMesh->create( hwMesh );
    HW_MODIFY_1I( curr, hwStrGraphN, 6 );
    HW_MODIFY_1I( curr, hwStrGraphM, 2 );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
    CHROME_HW(curr);
    curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,6*2*3), arm1 );
    oList[nObjs++] = curr;

    curr = hwMesh->create( hwMesh );
    HW_MODIFY_1I( curr, hwStrGraphN, 6 );
    HW_MODIFY_1I( curr, hwStrGraphM, 2 );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
    CHROME_HW(curr);
    curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,6*2*3), base );
    oList[nObjs++] = curr;

    curr = hwPolygon->create( hwPolygon );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
    WHITE_PLASTIC_HW(curr);
    curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,4*3), light );

    /* TBD: LOD */
    curr = hwGroup->create( hwGroup );
    curr->modify( curr, hwStrChildren,
			HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );

    obj->display_list = createHwSegmentFromObj( &curr, 1 );
}

/*****************************************************************
 * create_lamp_graphics
 * 
 *
 */
static void create_lamp_graphics(
    DRIVE_OBJECT *obj,
    int type)
{
    switch (type)
    {
      case 1:
	do_lamp_type_1( obj );
	break;

      default:
	do_lamp_type_0( obj );
	break;
    }
}


/*****************************************************************
 * init_lamp_object
 * 
 *
 */
void init_lamp_object(
    DRIVE_OBJECT *obj)
{
    LAMP_LIST *ll;
    int type;


    if (Length <= 0.0) Length = DEFAULT_LAMP_LENGTH;
    if (Height <= 0.0) Height = DEFAULT_LAMP_HEIGHT;
    if (Radius <= 0.0) Radius = DEFAULT_LAMP_RADIUS;

    type = DEFAULT_LAMP_TYPE;
    if (strcmp(obj->subtype,"overhanging") == 0) type = 1;

    if(debug) printf(" inside init_lamp_%d_object() routine \n",(int) Height);

    obj->num_children = 0;

    /* See if we've created one like this before... */
    ll = lamp_list;
    while (ll != NULL)
    {
	if (IS_NEAR(ll->height,Height)
		&& IS_NEAR(ll->radius,Radius)
		&& (ll->nameset_bits == obj->nameset_bits)
		&& (ll->type == type))
	    break;
	ll = ll->next;
    }

    if (ll != NULL) 
    {
	/* Good -- I have one like this already. */
	obj->display_list = ll->dl_number;
    }
    else
    {
	/* Nope -- gotta create a new one. */
	if ((ll = (LAMP_LIST *) malloc(sizeof(LAMP_LIST))) == NULL)
	{
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}

	ll->height = Height;
	ll->radius = Radius;
	ll->type   = type;
	ll->nameset_bits = obj->nameset_bits;
	ll->next   = lamp_list;

    	create_lamp_graphics(obj,type);

	ll->dl_number = obj->display_list;
	lamp_list  = ll;
    }


    /* Initial (mc) bounding box values */
    obj->bound_mc[0] = -Radius;
    obj->bound_mc[1] = 0.0;
    obj->bound_mc[2] = -Radius;
    obj->bound_mc[3] = Radius;
    obj->bound_mc[4] = Height;
    obj->bound_mc[5] = Radius;

    /* apply the object's xform matrix to the bounding box to put it in
     * world coordinates */
    update_wc_bounds(obj);

    obj->surface_chars_xyz  = lamp_surface_chars_xyz;
    obj->surface_chars_bbox = lamp_surface_chars_bbox;

    elevate_object_to_terrain_height((SCENE *) obj->scene,obj,FALSE);
}
