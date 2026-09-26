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


/* Code Module for various houses */

#include <stdio.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "prims.h"
#include "obj_common.h"
#include "demo_physics.h"

#define POLY_OFFSET	(0.05)

#define H1_LENGTH	7.0
#define H1_WIDTH	10.0
#define H1_HEIGHT	11.75
#define H1_ROOFPEAK	(H1_HEIGHT+H1_LENGTH/2.0)

#define H2_LENGTH		(8.0)
#define H2_WIDTH		(12.0)
#define H2_HEIGHT		(11.75)
#define L_H2_LENGTH		(4.5)
#define L_H2_WIDTH		(H2_WIDTH*0.8)
#define H2_ROOFPEAK		(H2_HEIGHT+H2_LENGTH/2.0)

#define H3_LENGTH	(6.0)
#define H3_WIDTH	(11.0)
#define H3_HEIGHT	(9.0)
#define H3_L_LENGTH	(4.0)
#define H3_L_WIDTH	(12.0)
#define H3_ROOFPEAK	(H3_HEIGHT+10.0)
#define H3_OVERHANG	(0.5)

#define H4_LENGTH	(10.0)
#define H4_WIDTH	(13.0)
#define H4_HEIGHT	(9.0)
#define H4_ROOFMID	(H4_HEIGHT+6.0)
#define H4_ROOFMID_Z	(6.0)
#define H4_ROOFPEAK	(H4_HEIGHT+9.0)
#define H4_OVERHANG	(1.0)
#define H4_PORCH_L	(5.0)
#define H4_PORCH_Y	(H4_HEIGHT-2.0)

typedef struct _house_list {
    int type,color,roofcolor;
    unsigned int nameset_bits;
    int dl_number;
    struct _house_list *next;
} HOUSE_LIST;
static HOUSE_LIST *house_list = NULL;

static int house_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = obj->size[SIZE_HEIGHT];
    get_box_mc_normal(obj,x,y,z,sc->mc_normal);
    return(TRUE);
}


static int house_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = obj->size[SIZE_HEIGHT];
    return(TRUE);
}

static void create_house_graphics(
    DRIVE_OBJECT *obj,
    char *filename,
    float r, float g, float b,
    float rr, float rg, float rb)
{
    int
	numObjs;
    hwObject
	*objs;

    numObjs = hwParseFile( filename, &objs );
    if( numObjs != 3 ) return;

    HW_MODIFY_3F( objs[0], hwStrColor, r, g, b );
    HW_MODIFY_3F( objs[1], hwStrColor, rr, rg, rb );

    obj->display_list = createHwSegmentFromObj( objs, numObjs );
}


void init_house_object(
    DRIVE_OBJECT *obj)
{
    HOUSE_LIST *hl;
    typedef struct {
	char *subtype;
	char *filename;
	float mc_extent[2][3];
    } HOUSE_CLASS;
    static HOUSE_CLASS house_class[] = {
	{ "garrison",
		"objects/Garrison.hw",
	    	{{ -H1_WIDTH,	0.0,		-H1_LENGTH },
	    	{  H1_WIDTH,	H1_ROOFPEAK,	H1_LENGTH }}
	},
	{ "antebellum farm",
		"objects/Antebellum.hw",
	    	{{ -H2_WIDTH,	0.0,		-H2_LENGTH-L_H2_LENGTH },
	    	{ H2_WIDTH,	H2_ROOFPEAK,	H2_LENGTH }}
	},
	{ "gothic cottage",
		"objects/Gothic.hw",
	    	{{ -H3_WIDTH,	0.0,		-H3_LENGTH-H3_L_LENGTH },
	    	{ H3_WIDTH,	H3_ROOFPEAK,	H3_LENGTH }}
	},
	{ "dutch colonial",
		"objects/Colonial.hw",
	    	{{ -H4_WIDTH,	0.0,		-H4_LENGTH-H4_PORCH_L },
	    	{ H4_WIDTH,	H4_ROOFPEAK,	H4_LENGTH }}
	},
    };
#   define NUMCLASSES (sizeof(house_class)/sizeof(HOUSE_CLASS))
    HOUSE_CLASS *hclass;
    typedef struct {
	float r;
	float g;
	float b;
    } HOUSE_COLOR;
    static HOUSE_COLOR house_color[] = {
	{ 0.9, 0.9, 0.9 },
	{ 0.7, 0.4, 0.4 },
	{ 0.2, 0.5, 0.7 },
	{ 0.4, 0.4, 0.4 },
    };
#   define NUMCOLORS (sizeof(house_color)/sizeof(HOUSE_COLOR))
    static HOUSE_COLOR roof_color[] = {
	{ 0.4, 0.2, 0.1 },
	{ 0.5, 0.1, 0.1 },
	{ 0.2, 0.2, 0.2 },
	{ 0.2, 0.4, 0.2 },
	{ 0.05, 0.05, 0.05 },
    };
#   define NUMROOFCOLORS (sizeof(roof_color)/sizeof(HOUSE_COLOR))
    int i,type,color,rcolor;


    /* Find house class */
    /* choose one at random */
    type = INTRAND(NUMCLASSES) - 1;
    if (obj->subtype[0] != '\0') {
	for (hclass=house_class,i=0; i < NUMCLASSES; ++hclass,++i) {
	    if (strcmp(obj->subtype,hclass->subtype) == 0) {
		type = i;
		break;
	    }
	}
    }

    /* Choose a random color */
    color = INTRAND(NUMCOLORS) - 1;

    /* Choose a random roof color */
    rcolor = INTRAND(NUMROOFCOLORS) - 1;

    if(debug) printf(" inside init_house_%d_%d_%d_object() routine \n",
	type,color,rcolor);


    /* See if we've created this type and color before... */
    hl = house_list;
    while (hl != NULL) {
	if ((hl->type == type)
		&& (hl->color == color)
		&& (hl->nameset_bits == obj->nameset_bits)
		&& (hl->roofcolor == rcolor)) {
	    break;
	}
	/* else */
	hl = hl->next;
    }
    if (hl != NULL) {
	/* good -- already got one. */
	obj->display_list = hl->dl_number;
    }
    else {
	/* Nope -- gotta create new one */
	if ((hl = (HOUSE_LIST *) malloc(sizeof(HOUSE_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}
	hl->type      = type;
	hl->color     = color;
	hl->roofcolor = rcolor;
	hl->nameset_bits = obj->nameset_bits;
	create_house_graphics(obj,
	    house_class[type].filename,
	    house_color[color].r, house_color[color].g, house_color[color].b,
	    roof_color[rcolor].r, roof_color[rcolor].g, roof_color[rcolor].b);
	hl->dl_number = obj->display_list;
	hl->next = house_list;
	house_list = hl;
    }

    obj->size[SIZE_LENGTH] = 
	house_class[type].mc_extent[1][0] - house_class[type].mc_extent[0][0];
    obj->size[SIZE_HEIGHT] = 
	house_class[type].mc_extent[1][1] - house_class[type].mc_extent[0][1];
    obj->size[SIZE_WIDTH] = 
	house_class[type].mc_extent[1][2] - house_class[type].mc_extent[0][2];

    obj->surface_chars_xyz  = house_surface_chars_xyz;
    obj->surface_chars_bbox = house_surface_chars_bbox;

    obj->bound_mc[0] = house_class[type].mc_extent[0][0];
    obj->bound_mc[1] = house_class[type].mc_extent[0][1];
    obj->bound_mc[2] = house_class[type].mc_extent[0][2];
    obj->bound_mc[3] = house_class[type].mc_extent[1][0];
    obj->bound_mc[4] = house_class[type].mc_extent[1][1];
    obj->bound_mc[5] = house_class[type].mc_extent[1][2];

    /* Update WC bounding box */
    update_wc_bounds(obj);

    elevate_object_to_terrain_height((SCENE *) obj->scene,obj,TRUE);
}
