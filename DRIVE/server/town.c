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
#include <math.h>
#include <string.h>
#include "global.h"
#include "object.h"
#include "scene.h"

#define SIDE_ROAD_WIDTH		25.0
#define MAX_CITY_ROADS		32
#define MAX_CITY_AREAS		64
#define MIN_ROAD_SPACING	100.0
#define MIN_ROAD_LENGTH		150.0
#define MAX_AREA_SIZE		(SCENE_SIZE/5.0)
#define MAX_LAWN_SIZE		200.0
#define MAX_LOT_SIZE		400.0
#define MAX_SKYSCRAPER_SIZE	200.0
#define SIGN_OFFSET		((SCENE_ROAD_WIDTH/2.0)+3.0)
#define SIGN_HEIGHT		6.0
#define STREET_OFFSET		((SCENE_ROAD_WIDTH/2.0)+10.0)
#define TREE_MIN_HEIGHT		10.0
#define TREE_MAX_HEIGHT		50.0
#define DRIVEWAY_OFFSET		20.0
#define DRIVEWAY_WIDTH		12.0
#define ENTER_SIGN_LENGTH	5.0
#define ENTER_SIGN_WIDTH	10.0
#define ENTER_SIGN_HEIGHT	2.0

#define RANDOM_TOWN_LOC(radius)						\
    (BOUNDED_FLOATRAND(-radius,radius))
#define GRID_RANDOM_TOWN_LOC(radius_increments) 			\
    ((float) BOUNDED_INTRAND(-(radius_increments),(radius_increments))	\
	    * SCENE_GRID_SIZE)

#define AREA_TYPE_EMPTY_LOT		1
#define AREA_TYPE_PARK			2
#define AREA_TYPE_WOODS			3
#define AREA_TYPE_MAX_NOATTACH		AREA_TYPE_WOODS
#define AREA_TYPE_HOUSE			4
#define AREA_TYPE_LOW_SKYSCRAPER	5
#define AREA_TYPE_SKYSCRAPER		6
#define AREA_TYPE_PARKING_LOT		7
#define AREA_TYPE_POND			8
#define AREA_TYPE_MAX			AREA_TYPE_POND


typedef struct {
    float xvalue;
    float zmin,zmax;
} XROAD;

typedef struct {
    float xmin,xmax;
    float zvalue;
} ZROAD;

typedef struct {
    float left,right,top,bottom;
    XROAD *leftroad,*rightroad;
    ZROAD *downroad,*uproad;
    int type,attach,bestattach,maxsize;
} AREA;

static int corner_checks[4] = {
    SCENE_ROAD_RIGHT|SCENE_ROAD_UP,
    SCENE_ROAD_LEFT|SCENE_ROAD_UP,
    SCENE_ROAD_LEFT|SCENE_ROAD_DOWN,
    SCENE_ROAD_RIGHT|SCENE_ROAD_DOWN
};
static int side_checks[4] = {
    SCENE_ROAD_RIGHT,
    SCENE_ROAD_LEFT,
    SCENE_ROAD_DOWN,
    SCENE_ROAD_UP
};
/* Random arrangement of four choices. */
static int check_order[24][4] = {
    { 0, 1, 2, 3 },
    { 0, 1, 3, 2 },
    { 0, 2, 1, 3 },
    { 0, 2, 3, 1 },
    { 0, 3, 1, 2 },
    { 0, 3, 2, 1 },
    { 1, 0, 2, 3 },
    { 1, 0, 3, 2 },
    { 1, 2, 0, 3 },
    { 1, 2, 3, 0 },
    { 1, 3, 0, 2 },
    { 1, 3, 2, 0 },
    { 2, 0, 1, 3 },
    { 2, 0, 3, 1 },
    { 2, 1, 0, 3 },
    { 2, 1, 3, 0 },
    { 2, 3, 0, 1 },
    { 2, 3, 1, 0 },
    { 3, 0, 1, 2 },
    { 3, 0, 2, 1 },
    { 3, 1, 0, 2 },
    { 3, 1, 2, 0 },
    { 3, 2, 0, 1 },
    { 3, 2, 1, 0 }
};



static void construct_name(
    char *out,
    int maxlen)
{
    static char *con_start[] = {
	"B", "B", "B", "B", "B", "B", "BL", "BR", "BR", "C", "C", "C",
	"C", "CH", "CH", "CH", "CHR", "CHR", "CL", "CR", "CR", "D", "D",
	"D", "D", "D", "D", "DR", "DR", "DW", "F", "F", "F", "F", "F",
	"F", "FL", "FL", "FR", "FR", "G", "G", "G", "G", "GL", "GN",
	"GR", "GR", "GR", "H", "H", "H", "J", "K", "KH", "KL", "KN",
	"L", "L", "L", "L", "L", "L", "M", "M", "M", "M", "M", "M", "M",
	"N", "N", "N", "N", "N", "N", "N", "P", "P", "P", "P", "P",
	"PH", "PHR", "PL", "PL", "PR", "PR", "QU", "QU", "R", "R", "R",
	"R", "R", "RH", "S", "S", "S", "S", "S", "S", "SH", "SH", "SH",
	"SH", "SHR", "SK", "SL", "SL", "SN", "SN", "SP", "SP", "SQU",
	"ST", "ST", "ST", "STR", "STR", "SW", "T", "T", "T", "T", "T",
	"T", "T", "T", "T", "T", "TH", "TH", "TH", "TH", "THR", "TR",
	"TR", "V", "V", "W", "W", "WH", "WR", "Z"

    };
#   define NUM_CS (sizeof(con_start)/sizeof(char *))
    static char *vowel_start[] = {
	"A", "A", "A", "A", "A", "AI", "E", "E", "E", "EA", "EI", "EU",
	"I", "I", "I", "I", "IO", "O", "O", "O", "O", "OA", "OI", "OU",
	"U", "U", "U"

    };
#   define NUM_VS (sizeof(vowel_start)/sizeof(char *))


    static char *con_mid[] = {
	"B", "B", "B", "B", "B", "B", "BB", "BL", "BR", "C", "C", "CH",
	"CHR", "CK", "CL", "CR", "CT", "CT", "D", "D", "D", "D", "DD",
	"DR", "DT", "DW", "F", "F", "F", "F", "FF", "FL", "FR", "G",
	"G", "G", "G", "GG", "GH", "GHT", "GL", "GN", "GR", "GHT", "H",
	"H", "J", "K", "K", "KK", "KL", "KR", "L", "L", "L", "L", "L",
	"LD", "LL", "LN", "LT", "M", "M", "M", "M", "M", "M", "MD",
	"ML", "MM", "MN", "MP", "MT", "N", "N", "N", "N", "N", "N", "N",
	"N", "ND", "ND", "ND", "NN", "NQU", "NT", "P", "P", "P", "P",
	"PH", "PL", "PP", "PR", "QU", "R", "R", "R", "R", "R", "R",
	"RB", "RD", "RG", "RL", "RM", "RN", "RP", "RR", "RT", "S", "S",
	"S", "S", "S", "S", "SC", "SH", "SH", "SH", "SK", "SL", "SM",
	"SN", "SP", "SQU", "SS", "ST", "ST", "ST", "STR", "SW", "T",
	"T", "T", "T", "T", "T", "TH", "TH", "TH", "THR", "TL", "TR",
	"TR", "TT", "TW", "V", "V", "V", "VR", "VV", "W", "W", "W",
	"WH", "WN", "WR", "WW", "Z",

    };
#   define NUM_CM (sizeof(con_mid)/sizeof(char *))
    static char *vowel_mid[] = {
	"A", "A", "A", "A", "A", "AI", "AU", "AU", "AY", "E", "E", "E",
	"E", "E", "E", "E", "E", "E", "EA", "EE", "EE", "EI", "I", "I",
	"I", "I", "I", "I", "IA", "IE", "IO", "O", "O", "O", "O", "O",
	"O", "OA", "OI", "OO", "OU", "OY", "U", "U", "U"

    };
#   define NUM_VM (sizeof(vowel_mid)/sizeof(char *))

    static char *con_end[] = {
	"B", "BB", "C", "CH", "CH", "CK", "CH", "CT", "CT", "D", "DD",
	"F", "FF", "G", "G", "GG", "GH", "GHT", "K", "KK", "L", "L",
	"L", "LL", "LL", "LT", "M", "M", "M", "M", "M", "M", "MM", "MM",
	"MP", "N", "N", "N", "N", "N", "N", "ND", "ND", "ND", "ND",
	"ND", "NN", "NN", "NT", "NT", "P", "R", "R", "R", "R", "RB",
	"RD", "RG", "RGH", "RL", "RM", "RN", "RP", "RR", "RR", "RT",
	"RT", "S", "S", "S", "S", "SH", "SH", "SK", "SP", "SS", "SS",
	"ST", "ST", "T", "T", "T", "T", "T", "T", "TH", "TT", "TT",
	"WN", "ZZ"

    };
#   define NUM_CE (sizeof(con_end)/sizeof(char *))
    static char *vowel_end[] = {
	"A", "A", "A", "A", "A", "AU", "AY", "AY", "E", "E", "E", "E",
	"E", "E", "E", "E", "E", "EA", "EA", "EY", "EY", "I", "I", "I",
	"IA", "IE", "O", "O", "O", "O", "OA", "OI", "OU", "OY", "OY",
	"U", 
    };
#   define NUM_VE (sizeof(vowel_end)/sizeof(char *))

    char name[256],tmp[256];
    int is_vowel, count, got_complex, len;
    float f;


    do {
	is_vowel = (floatrand() > 0.9);

	if (is_vowel) strcpy(name,vowel_start[ZINTRAND(NUM_VS)]);
	else strcpy(name,con_start[ZINTRAND(NUM_CS)]);
	got_complex = (strlen(name) > 1);

	if ((f = floatrand()) < .1) count = 3;
	else if (f < .3)  count = 4;
	else if (f < .8)  count = 5;
	else if (f < .85) count = 6;
	else if (f < .97) count = 7;
	else              count = 8;

	while ((--count) > 1) {
	    is_vowel = !is_vowel;
	    do {
		if (is_vowel)
		    strcpy(tmp,vowel_mid[ZINTRAND(NUM_VM)]);
		else
		    strcpy(tmp,con_mid[ZINTRAND(NUM_CM)]);
		len = strlen(tmp);
	    } while (got_complex && (len > 1));
	    strcat(name,tmp);
	    if (len > 1) got_complex = TRUE;
	}

	is_vowel = !is_vowel;
	do {
	    if (is_vowel)
		strcpy(tmp,vowel_end[ZINTRAND(NUM_VE)]);
	    else
		strcpy(tmp,con_end[ZINTRAND(NUM_CE)]);
	    len = strlen(tmp);
	} while (got_complex && (len > 1));
	strcat(name,tmp);
    } while (strlen(name) > maxlen);

/*    name[0] += ('A' - 'a'); */

    strcpy(out,name);
}


static void random_town_name(
	char *name)
{
    static char *suffix[] = {
	" BEND",
	" BLUFF",
	" BROOK",
	" CENTER",
	" CITY",
	" CITY",
	" CREEK",
	" CROSSING",
	" GROVE",
	" HILL",
	" JUNCTION",
	" LAKE",
	" MILLS",
	" PARK",
	" RIVER",
	" ROCK",
	" SPRINGS",
	" TOWN",
	" VALLEY",
	" WOODS",
	"BURG",
	"BURG",
	"BURY",
	"BURY",
	"DALE",
	"FIELD",
	"FORD",
	"HAM",
	"LAND",
	"MONT",
	"PORT",
	"TON",
	"TON",
	"TOWN",
	"VILLE",
	"VILLE",
	"VILLE",
	"WOOD"
    };
#   define NUM_SUFFIXES (sizeof(suffix)/sizeof(char *))

    construct_name(name,8);
    if (fifty_fifty()) {
	strcat(name,suffix[ZINTRAND(NUM_SUFFIXES)]);
    }
}



static void position_area(
	AREA *aptr,
	float maxsize)
{
    float width  = aptr->right - aptr->left;
    float length = aptr->top - aptr->bottom;
    float w,l;
    int order,flags,i;


    if (!(aptr->attach)) return;

    if (width > maxsize) w = maxsize;
    else w = width;
    if (length > maxsize) l = maxsize;
    else l = length;

    order = ZINTRAND(24);
    for (i=0; i<4; ++i) {
	flags = corner_checks[check_order[order][i]];
	if ((aptr->attach & flags) == flags) {
	    switch (flags) {
		case SCENE_ROAD_RIGHT|SCENE_ROAD_UP:
		    /* upper right unchanged */
		    aptr->left   = aptr->right - w;
		    aptr->bottom = aptr->top - l;
		    if ((w < l) || ((w == l) && fifty_fifty()))
			aptr->bestattach = SCENE_ROAD_RIGHT;
		    else aptr->bestattach = SCENE_ROAD_UP;
		    aptr->leftroad = NULL;
		    aptr->downroad = NULL;
		    break;
		case SCENE_ROAD_LEFT|SCENE_ROAD_DOWN:
		    /* lower left unchanged */
		    aptr->right  = aptr->left + w;
		    aptr->top    = aptr->bottom + l;
		    if ((w < l) || ((w == l) && fifty_fifty()))
			aptr->bestattach = SCENE_ROAD_LEFT;
		    else aptr->bestattach = SCENE_ROAD_DOWN;
		    aptr->rightroad = NULL;
		    aptr->uproad    = NULL;
		    break;
		case SCENE_ROAD_LEFT|SCENE_ROAD_UP:
		    /* upper left unchanged */
		    aptr->right  = aptr->left + w;
		    aptr->bottom = aptr->top - l;
		    if ((w < l) || ((w == l) && fifty_fifty()))
			aptr->bestattach = SCENE_ROAD_LEFT;
		    else aptr->bestattach = SCENE_ROAD_UP;
		    aptr->rightroad = NULL;
		    aptr->downroad  = NULL;
		    break;
		case SCENE_ROAD_RIGHT|SCENE_ROAD_DOWN:
		    /* lower right unchanged */
		    aptr->left   = aptr->right - w;
		    aptr->top    = aptr->bottom + l;
		    if ((w < l) || ((w == l) && fifty_fifty()))
			aptr->bestattach = SCENE_ROAD_RIGHT;
		    else aptr->bestattach = SCENE_ROAD_DOWN;
		    aptr->leftroad = NULL;
		    aptr->uproad   = NULL;
		    break;
	    }
	    /* Done. */
	    return;
	}
    }
    /* else no corner match... */

    order = ZINTRAND(24);
    aptr->bestattach = aptr->attach;
    for (i=0; i<4; ++i) {
	flags = side_checks[check_order[order][i]];
	if ((aptr->attach & flags) == flags) {
	    switch (flags) {
    		case SCENE_ROAD_LEFT:
		    /* Slide it up and down the left side. */
		    aptr->right   = aptr->left + w;
		    aptr->bottom += ROUND_TO_GRID(FLOATRAND(length - l));
		    aptr->top     = aptr->bottom + l;
		    aptr->uproad    = NULL;
		    aptr->rightroad = NULL;
		    aptr->downroad  = NULL;
		    break;
    		case SCENE_ROAD_DOWN:
		    /* Slide it across the bottom side. */
		    aptr->left   += ROUND_TO_GRID(FLOATRAND(width - w));
		    aptr->right   = aptr->left + w;
		    aptr->top     = aptr->bottom + l;
		    aptr->uproad    = NULL;
		    aptr->rightroad = NULL;
		    aptr->leftroad  = NULL;
		    break;
    		case SCENE_ROAD_RIGHT:
		    /* Slide it up and down the right side. */
		    aptr->left    = aptr->right - w;
		    aptr->bottom += ROUND_TO_GRID(FLOATRAND(length - l));
		    aptr->top     = aptr->bottom + l;
		    aptr->uproad    = NULL;
		    aptr->downroad  = NULL;
		    aptr->leftroad  = NULL;
		    break;
    		case SCENE_ROAD_UP:
		    /* Slide it across the top side. */
		    aptr->bottom  = aptr->top - l;
		    aptr->left   += ROUND_TO_GRID(FLOATRAND(width - w));
		    aptr->right   = aptr->left + w;
		    aptr->rightroad = NULL;
		    aptr->downroad  = NULL;
		    aptr->leftroad  = NULL;
		    break;
	    }
	    /* Done. */
	    return;
	}
    }
}



static void add_area_objects(
    SCENE *scene,
    AREA  *aptr)
{
    int i,j;
    float orig_left, orig_right, orig_bottom, orig_top;
    float width,length,squarefeet;
    float x,z,w,l,yrot;

    orig_left   = aptr->left;
    orig_right  = aptr->right;
    orig_bottom = aptr->bottom;
    orig_top    = aptr->top;

    /* Move it slightly away from the street. */
    aptr->left   += STREET_OFFSET;
    aptr->right  -= STREET_OFFSET;
    aptr->bottom += STREET_OFFSET;
    aptr->top    -= STREET_OFFSET;

    width      = aptr->right - aptr->left;
    length     = aptr->top - aptr->bottom;
    squarefeet = width * length;


    switch (aptr->type) {
	case AREA_TYPE_PARK:
	    /* lawn */
	    x = (aptr->right + aptr->left) / 2.0;
	    z = (aptr->top + aptr->bottom) / 2.0;
	    add_object_to_scene(scene,"lawn","","",0.0,
		x,0.0,z,
		length,width,0.0,
		0.0,0.0,0.0,0);
	    /* trees */
	    j = 4 + ZINTRAND(squarefeet/20000.0);
	    if (j > 12) j = 12;
	    for (i=0; i<j; ++i) {
		x = BOUNDED_FLOATRAND(aptr->left,aptr->right);
		z = BOUNDED_FLOATRAND(aptr->bottom,aptr->top);
		add_random_tree(scene,x,z);
	    }
	    break;

	case AREA_TYPE_WOODS:
	    /* trees */
	    j = 5 + ZINTRAND(squarefeet/12000.0);
	    for (i=0; i<j; ++i) {
		x = BOUNDED_FLOATRAND(aptr->left,aptr->right);
		z = BOUNDED_FLOATRAND(aptr->bottom,aptr->top);
		add_random_tree(scene,x,z);
	    }
	    break;

	case AREA_TYPE_HOUSE:
	    position_area(aptr, MAX_LAWN_SIZE);
	    x = (aptr->left + aptr->right)/2.0;
	    z = (aptr->top + aptr->bottom)/2.0;
	    w = aptr->right - aptr->left;
	    l = aptr->top - aptr->bottom;

	    /* lawn */
	    add_object_to_scene(scene,"lawn","","",0.0,
		x,0.0,z,
		l,w,0.0,
		0.0,0.0,0.0,0);

	    /* driveway & mailbox */
	    switch (aptr->bestattach) {
		case SCENE_ROAD_RIGHT:
		    yrot = M_PI/2.0;
		    if (orig_right == 0.0) {
			add_object_to_scene(scene,"road","","",-M_PI/2.0,
			    x,0.1,z + DRIVEWAY_OFFSET,
			    orig_right - x - SCENE_ROAD_WIDTH/2.0 + 1.0,
			    DRIVEWAY_WIDTH,0.0,
			    0.0,0.0,0.0,0);
			add_object_to_scene(scene,"mailbox","","",M_PI/2.0,
			    orig_right - SCENE_ROAD_WIDTH/2.0 - 2.0,0.0,
			    z + DRIVEWAY_WIDTH/2.0 + 2.0,
			    0.0,0.0,0.0,
			    0.0,0.0,0.0,0);
		    }
		    else {
			add_object_to_scene(scene,"road","","",-M_PI/2.0,
			    x,0.1,z + DRIVEWAY_OFFSET,
			    orig_right - x - SIDE_ROAD_WIDTH/2.0 + 1.0,
			    DRIVEWAY_WIDTH,0.0,
			    0.0,0.0,0.0,0);
			add_object_to_scene(scene,"mailbox","","",M_PI/2.0,
			    orig_right - SIDE_ROAD_WIDTH/2.0 - 2.0,0.0,
			    z + DRIVEWAY_WIDTH/2.0 + 2.0,
			    0.0,0.0,0.0,
			    0.0,0.0,0.0,0);
		    }
		    break;
		case SCENE_ROAD_LEFT:
		    yrot = -M_PI/2.0;
		    if (orig_left == 0.0) {
			add_object_to_scene(scene,"road","","",M_PI/2.0,
			    x,0.1,z - DRIVEWAY_OFFSET,
			    x - orig_left - SCENE_ROAD_WIDTH/2.0 + 1.0,
			    DRIVEWAY_WIDTH,0.0,
			    0.0,0.0,0.0,0);
			add_object_to_scene(scene,"mailbox","","",-M_PI/2.0,
			    orig_left + SCENE_ROAD_WIDTH/2.0 + 2.0,0.0,
			    z - DRIVEWAY_WIDTH/2.0 - 2.0,
			    0.0,0.0,0.0,
			    0.0,0.0,0.0,0);
		    }
		    else {
			add_object_to_scene(scene,"road","","",M_PI/2.0,
			    x,0.1,z - DRIVEWAY_OFFSET,
			    x - orig_left - SIDE_ROAD_WIDTH/2.0 + 1.0,
			    DRIVEWAY_WIDTH,0.0,
			    0.0,0.0,0.0,0);
			add_object_to_scene(scene,"mailbox","","",-M_PI/2.0,
			    orig_left + SIDE_ROAD_WIDTH/2.0 + 2.0,0.0,
			    z - DRIVEWAY_WIDTH/2.0 - 2.0,
			    0.0,0.0,0.0,
			    0.0,0.0,0.0,0);
		    }
		    break;
		case SCENE_ROAD_UP:
		    yrot = -M_PI;
		    if (orig_top == 0.0) {
			add_object_to_scene(scene,"road","","",0.0,
			    x-DRIVEWAY_OFFSET,0.1,z,
			    orig_top - z - SCENE_ROAD_WIDTH/2.0 + 1.0,
			    DRIVEWAY_WIDTH,0.0,
			    0.0,0.0,0.0,0);
			add_object_to_scene(scene,"mailbox","","",M_PI,
			    x + DRIVEWAY_WIDTH/2.0 + 2.0,0.0,
			    orig_top - SCENE_ROAD_WIDTH/2.0 - 2.0,
			    0.0,0.0,0.0,
			    0.0,0.0,0.0,0);
		    }
		    else {
			add_object_to_scene(scene,"road","","",0.0,
			    x-DRIVEWAY_OFFSET,0.1,z,
			    orig_top - z - SIDE_ROAD_WIDTH/2.0 + 1.0,
			    DRIVEWAY_WIDTH,0.0,
			    0.0,0.0,0.0,0);
			add_object_to_scene(scene,"mailbox","","",M_PI,
			    x + DRIVEWAY_WIDTH/2.0 + 2.0,0.0,
			    orig_top - SIDE_ROAD_WIDTH/2.0 - 2.0,
			    0.0,0.0,0.0,
			    0.0,0.0,0.0,0);
		    }
		    break;
		case SCENE_ROAD_DOWN:
		    yrot = 0.0;
		    if (orig_bottom == 0.0) {
			add_object_to_scene(scene,"road","","",M_PI,
			    x+DRIVEWAY_OFFSET,0.1,z,
			    z - orig_bottom - SCENE_ROAD_WIDTH/2.0 + 1.0,
			    DRIVEWAY_WIDTH,0.0,
			    0.0,0.0,0.0,0);
			add_object_to_scene(scene,"mailbox","","",0.0,
			    x - DRIVEWAY_WIDTH/2.0 - 2.0,0.0,
			    orig_bottom + SCENE_ROAD_WIDTH/2.0 + 2.0,
			    0.0,0.0,0.0,
			    0.0,0.0,0.0,0);
		    }
		    else {
			add_object_to_scene(scene,"road","","",M_PI,
			    x+DRIVEWAY_OFFSET,0.1,z,
			    z - orig_bottom - SIDE_ROAD_WIDTH/2.0 + 1.0,
			    DRIVEWAY_WIDTH,0.0,
			    0.0,0.0,0.0,0);
			add_object_to_scene(scene,"mailbox","","",0.0,
			    x - DRIVEWAY_WIDTH/2.0 - 2.0,0.0,
			    orig_bottom + SIDE_ROAD_WIDTH/2.0 + 2.0,
			    0.0,0.0,0.0,
			    0.0,0.0,0.0,0);
		    }
		    break;
	    }

	    /* house */
	    add_object_to_scene(scene,"house","","",yrot,
		x,0.0,z,
		0.0,0.0,0.0,
		0.0,0.0,0.0,0);

	    /* trees */
	    j = 1 + ZINTRAND(length*width/12000.0);
	    if (j > 12) j = 12;
	    for (i=0; i<j; ++i) {
		float  xt,zt;
		do {
		    xt = BOUNDED_FLOATRAND(orig_left+STREET_OFFSET,
			orig_right-STREET_OFFSET);
		    zt = BOUNDED_FLOATRAND(orig_bottom+STREET_OFFSET,
			orig_top-STREET_OFFSET);
		} while ((ABS(xt - x) < TREE_MAX_HEIGHT/2.0)
			|| (ABS(zt - z) < TREE_MAX_HEIGHT/2.0));
		add_random_tree(scene,xt,zt);
	    }
	    break;

	case AREA_TYPE_SKYSCRAPER:
	    position_area(aptr, MAX_SKYSCRAPER_SIZE);
	    x = (aptr->left + aptr->right)/2.0;
	    z = (aptr->top + aptr->bottom)/2.0;
	    w = aptr->right - aptr->left;
	    l = aptr->top - aptr->bottom;
	    add_object_to_scene(scene,"skyscraper","","",0.0,
		x-w/2.0,0.0,z-l/2.0,
		ROUND_SKYSCRAPER_SIZE(w),ROUND_SKYSCRAPER_SIZE(l),
		    RANDOM_SKYSCRAPER_HEIGHT(12.0,120.0),
		0.0,0.0,0.0,0);
	    break;
	case AREA_TYPE_LOW_SKYSCRAPER:
	    position_area(aptr, MAX_SKYSCRAPER_SIZE);
	    x = (aptr->left + aptr->right)/2.0;
	    z = (aptr->top + aptr->bottom)/2.0;
	    w = aptr->right - aptr->left;
	    l = aptr->top - aptr->bottom;
	    add_object_to_scene(scene,"skyscraper","","",0.0,
		x-w/2.0,0.0,z-l/2.0,
		ROUND_SKYSCRAPER_SIZE(w),ROUND_SKYSCRAPER_SIZE(l),
		    RANDOM_SKYSCRAPER_HEIGHT(12.0,48.0),
		0.0,0.0,0.0,0);
	    break;

	case AREA_TYPE_PARKING_LOT:
	    position_area(aptr, MAX_LOT_SIZE);
	    x = (aptr->left + aptr->right)/2.0;
	    z = (aptr->top + aptr->bottom)/2.0;
	    w = aptr->right - aptr->left;
	    l = aptr->top - aptr->bottom;
	    add_object_to_scene(scene,"parking lot","","",0.0,
		x,0.0,z,
		l,w,0.0,
		0.0,0.0,0.0,0);
	    break;

	case AREA_TYPE_POND:
	    x = (aptr->right + aptr->left) / 2.0;
	    z = (aptr->top + aptr->bottom) / 2.0;
	    add_object_to_scene(scene,"pond","","",0.0,
		x,0.0,z,
		length,width,0.0,
		0.0,0.0,0.0,0);
	    break;

	case AREA_TYPE_EMPTY_LOT:
	default:
	    break;	
    }
}


static int town_area_type(
  int attach,
  int city_type)
{
    int areatype;
    static int village_prob[] = {
	AREA_TYPE_EMPTY_LOT,
	AREA_TYPE_PARK,
	AREA_TYPE_WOODS,
	AREA_TYPE_WOODS,
	AREA_TYPE_HOUSE,
	AREA_TYPE_HOUSE,
	AREA_TYPE_HOUSE,
	AREA_TYPE_HOUSE,
	AREA_TYPE_LOW_SKYSCRAPER,
	AREA_TYPE_POND,
	AREA_TYPE_POND,
    };
#   define NUM_VILLAGE_PROB	(sizeof(village_prob)/sizeof(int))
    static int town_prob[] = {
	AREA_TYPE_EMPTY_LOT,
	AREA_TYPE_PARK,
	AREA_TYPE_PARK,
	AREA_TYPE_WOODS,
	AREA_TYPE_HOUSE,
	AREA_TYPE_HOUSE,
	AREA_TYPE_HOUSE,
	AREA_TYPE_HOUSE,
	AREA_TYPE_LOW_SKYSCRAPER,
	AREA_TYPE_SKYSCRAPER,
	AREA_TYPE_PARKING_LOT,
	AREA_TYPE_POND,
    };
#   define NUM_TOWN_PROB	(sizeof(town_prob)/sizeof(int))
    static int city_prob[] = {
	AREA_TYPE_PARK,
	AREA_TYPE_PARK,
	AREA_TYPE_HOUSE,
	AREA_TYPE_LOW_SKYSCRAPER,
	AREA_TYPE_LOW_SKYSCRAPER,
	AREA_TYPE_LOW_SKYSCRAPER,
	AREA_TYPE_SKYSCRAPER,
	AREA_TYPE_SKYSCRAPER,
	AREA_TYPE_PARKING_LOT,
	AREA_TYPE_PARKING_LOT,
    };
#   define NUM_CITY_PROB	(sizeof(city_prob)/sizeof(int))


    do {
	if (city_type == CITY_TYPE_VILLAGE)
	    areatype = village_prob[ZINTRAND(NUM_VILLAGE_PROB)];
	else if (city_type == CITY_TYPE_TOWN)
	    areatype = town_prob[ZINTRAND(NUM_TOWN_PROB)];
	else
	    areatype = city_prob[ZINTRAND(NUM_CITY_PROB)];
    } while (!attach && (areatype > AREA_TYPE_MAX_NOATTACH));

    return(areatype);
}


static void set_area_attachment(
	AREA *aptr)
{
    /* Check that the roads fill the whole side. */
    aptr->attach = 0;
    if (aptr->leftroad) {
	if (((aptr->leftroad)->zmin <= aptr->bottom)
		&& ((aptr->leftroad)->zmax >= aptr->top)) {
	    aptr->attach |= SCENE_ROAD_LEFT;
	}
    }
    if (aptr->rightroad) {
	if (((aptr->rightroad)->zmin <= aptr->bottom)
		&& ((aptr->rightroad)->zmax >= aptr->top)) {
	    aptr->attach |= SCENE_ROAD_RIGHT;
	}
    }
    if (aptr->downroad) {
	if (((aptr->downroad)->xmin <= aptr->left)
		&& ((aptr->downroad)->xmax >= aptr->right)) {
	    aptr->attach |= SCENE_ROAD_DOWN;
	}
    }
    if (aptr->uproad) {
	if (((aptr->uproad)->xmin <= aptr->left)
		&& ((aptr->uproad)->xmax >= aptr->right)) {
	    aptr->attach |= SCENE_ROAD_UP;
	}
    }
}


void generate_town(
    SCENE *scene,
    int city_type)
{
    XROAD xroad[MAX_CITY_ROADS];
    ZROAD zroad[MAX_CITY_ROADS];
    int num_xroads,num_zroads,num_roads,num_areas;
    int i,j,k,city_radius,city_radius_increments;
    int twoended,good_road,maxend;
    int possible_end[MAX_CITY_ROADS],ends;
    typedef struct {
	float x,z,rot;
    } SIGNSTRUCT;
    SIGNSTRUCT stopsign[MAX_CITY_ROADS], *ss;
    int new_stopsigns;
    AREA area[MAX_CITY_AREAS],*aptr,*aptr2;
    char name[256],welcome[256];
    int good_areas,area_count;
	    

    num_xroads = 1;
    xroad[0].xvalue = 0.0;
    xroad[0].zmin   = -SCENE_SIZE/2.0;
    xroad[0].zmax   =  SCENE_SIZE/2.0;
    num_zroads = 1;
    zroad[0].zvalue = 0.0;
    zroad[0].xmin   = -SCENE_SIZE/2.0;
    zroad[0].xmax   =  SCENE_SIZE/2.0;

    /* Do main crossroads, "enter" signs, and stop signs there. */
    random_town_name(name);

    if ((city_type < CITY_TYPE_VILLAGE) || (city_type < CITY_TYPE_CITY)) {
	switch (INTRAND(6)) {
	    case 1:
		city_type = CITY_TYPE_CITY;
		break;
	    case 2:
	    case 3:
		city_type = CITY_TYPE_TOWN;
		break;
	    default:
		city_type = CITY_TYPE_VILLAGE;
		break;
	}
    }

    switch (city_type) {
	case CITY_TYPE_VILLAGE:
	    city_radius = SCENE_SIZE/4.0;
	    num_roads = BOUNDED_INTRAND(4,8);
	    num_areas = num_roads + INTRAND(4);
	    break;
	case CITY_TYPE_TOWN:
	    city_radius = SCENE_SIZE/3.0;
	    num_roads = BOUNDED_INTRAND(8,12);
	    num_areas = num_roads + INTRAND(8);
	    break;
	case CITY_TYPE_CITY:
	    city_radius = SCENE_SIZE/2.1;
	    num_roads = BOUNDED_INTRAND(12,16);
	    num_areas = num_roads + INTRAND(12);
	    break;
    }
    city_radius_increments = city_radius / SCENE_GRID_SIZE;
    if (num_roads > MAX_CITY_ROADS) num_roads = MAX_CITY_ROADS;
    if (num_areas > MAX_CITY_AREAS) num_areas = MAX_CITY_AREAS;

    /* Main roads */
    add_object_to_scene(scene,"road","","",0.0,
	0.0,0.0,-(SCENE_SIZE/2.0),
	SCENE_SIZE,SCENE_ROAD_WIDTH,0.0,
	0.0,0.0,0.0,0);
    add_object_to_scene(scene,"road","","",M_PI/2.0,
	SCENE_SIZE/2.0,0.0,0.0,
	SCENE_SIZE,SCENE_ROAD_WIDTH,0.0,
	0.0,0.0,0.0,0);

    /* Stop signs */
    if (fifty_fifty()) {
	add_object_to_scene(scene,"stop sign","stop","",0.0,
	    SIGN_OFFSET,0.0,-SIGN_OFFSET,
	    0.0,0.0,SIGN_HEIGHT,
	    0.0,0.0,0.0,0);
	add_object_to_scene(scene,"stop sign","stop","",M_PI,
	    -SIGN_OFFSET,0.0,SIGN_OFFSET,
	    0.0,0.0,SIGN_HEIGHT,
	    0.0,0.0,0.0,0);
    }
    else {
	add_object_to_scene(scene,"stop sign","stop","",M_PI/2.0,
	    SIGN_OFFSET,0.0,SIGN_OFFSET,
	    0.0,0.0,SIGN_HEIGHT,
	    0.0,0.0,0.0,0);
	add_object_to_scene(scene,"stop sign","stop","",-M_PI/2.0,
	    -SIGN_OFFSET,0.0,-SIGN_OFFSET,
	    0.0,0.0,SIGN_HEIGHT,
	    0.0,0.0,0.0,0);
    }

    /* enter signs */
    sprintf(welcome,"NOW ENTERING\\%s",name); 

    add_object_to_scene(scene,"generic sign","generic",welcome,0.0,
	SIGN_OFFSET,0.0,-(city_radius + 20.0) + SIGN_OFFSET*2.0,
	ENTER_SIGN_LENGTH,ENTER_SIGN_WIDTH,ENTER_SIGN_HEIGHT,
	0.0,0.0,0.0,0);
    add_object_to_scene(scene,"generic sign","generic",welcome,M_PI/2.0,
	(city_radius + 20.0) - SIGN_OFFSET*2.0,0.0,SIGN_OFFSET,
	ENTER_SIGN_LENGTH,ENTER_SIGN_WIDTH,ENTER_SIGN_HEIGHT,
	0.0,0.0,0.0,0);
    add_object_to_scene(scene,"generic sign","generic",welcome,M_PI,
	-SIGN_OFFSET,0.0,(city_radius + 20.0) - SIGN_OFFSET*2.0,
	ENTER_SIGN_LENGTH,ENTER_SIGN_WIDTH,ENTER_SIGN_HEIGHT,
	0.0,0.0,0.0,0);
    add_object_to_scene(scene,"generic sign","generic",welcome,-M_PI/2.0,
	-(city_radius + 20.0) + SIGN_OFFSET*2.0,0.0,-SIGN_OFFSET,
	ENTER_SIGN_LENGTH,ENTER_SIGN_WIDTH,ENTER_SIGN_HEIGHT,
	0.0,0.0,0.0,0);

    /* Generate roads */
    for (i=2; i<num_roads; ++i) {
	do {
	    good_road = TRUE;
	    ss  = stopsign;
	    new_stopsigns = 0;
	    if (fifty_fifty()) {
		/*** Xroad ***/
		XROAD *xr = xroad + num_xroads;

		/* Pick an X. */
		xr->xvalue = GRID_RANDOM_TOWN_LOC(city_radius_increments);

		/* See if we can run in line with an existing road. */
		for (j=0; j<num_xroads; ++j) {
		    if (ABS(xr->xvalue - xroad[j].xvalue)
			    < MIN_ROAD_SPACING/2.0) {
			xr->xvalue = xroad[j].xvalue;
		    }
		}

		/* Connect it to a Zroad on one end. */
		ends = 0;
		for (j=0; j<num_zroads; ++j) {
		    /* Ignore ones that don't connect. */
		    if ((zroad[j].xmin > xr->xvalue)
			    || (zroad[j].xmax < xr->xvalue))
			continue;
		    /* Add to the list of possibilties. */
		    possible_end[ends] = j;
		    ++ends;
		}
		/* Connect it up. */
		twoended = FALSE;
		j = ZINTRAND(ends);
		if (ends > 1) {
		    xr->zmin = zroad[possible_end[j]].zvalue;
		    /* Connect both ends */
		    do {
			k = ZINTRAND(ends);
		    } while (j == k);
		    xr->zmax = zroad[possible_end[k]].zvalue;
		    /* Swap if reversed. */
		    if (xr->zmin > xr->zmax) FLOATSWAP(xr->zmin,xr->zmax);
		    if ((twoended =
		    		((xr->zmax - xr->zmin) > MIN_ROAD_LENGTH))) {
			/* Add stop signs */
			ss->x = xr->xvalue + SIGN_OFFSET;
			ss->z = xr->zmax   - SIGN_OFFSET;
			ss->rot = 0.0;
			++ss;
			ss->x = xr->xvalue - SIGN_OFFSET;
			ss->z = xr->zmin   + SIGN_OFFSET;
			ss->rot = M_PI;
			++ss;
			new_stopsigns += 2;
		    }
		}

		if (!twoended) {
		    xr->zmin = zroad[possible_end[j]].zvalue;
		    /* Connect only one end. */
		    do {
			xr->zmax = GRID_RANDOM_TOWN_LOC(city_radius_increments);
		    } while (ABS(xr->zmax - xr->zmin) < MIN_ROAD_LENGTH);
		    /* Swap if reversed. */
		    if (xr->zmin > xr->zmax) FLOATSWAP(xr->zmin,xr->zmax);
		    /* Add stop sign */
		    if (IS_NEAR(xr->zmax,zroad[possible_end[j]].zvalue)) {
			maxend = TRUE;
			ss->x = xr->xvalue + SIGN_OFFSET;
			ss->z = xr->zmax   - SIGN_OFFSET;
			ss->rot = 0.0;
		    }
		    else {
			maxend = FALSE;
			ss->x = xr->xvalue - SIGN_OFFSET;
			ss->z = xr->zmin   + SIGN_OFFSET;
			ss->rot = M_PI;
		    }
		    ++ss; ++new_stopsigns;
		}

		/* Now make sure it's not too close to a parallel road */
		for (j=0; j<num_xroads; ++j) {
		    if ((ABS(xroad[j].xvalue - xr->xvalue) < MIN_ROAD_SPACING) 
			    && (xroad[j].zmin < xr->zmax)
			    && (xroad[j].zmax > xr->zmin)) {
			good_road = FALSE;
			break;
		    }
		}
		if (!good_road) continue;

		/* Mark any crossings with stopsigns */
		for (k=0; k<ends; ++k) {
		    j = possible_end[k];
		    if ((xr->zmin < zroad[j].zvalue)
			    && (xr->zmax > zroad[j].zvalue)
			    && (zroad[j].xmin	< xr->xvalue)
			    && (zroad[j].xmax	> xr->xvalue)) {
			/* Add stopsigns */
			ss->x   = xr->xvalue + SIGN_OFFSET;
			ss->z   = zroad[j].zvalue - SIGN_OFFSET;
			ss->rot = 0.0;
			++ss;
			ss->x   = xr->xvalue - SIGN_OFFSET;
			ss->z   = zroad[j].zvalue + SIGN_OFFSET;
			ss->rot = M_PI;
			++ss;
			new_stopsigns += 2;
		    }
		}

		/* It's good -- add it! */
		++num_xroads;
		{
		    float zmin = xr->zmin;
		    float zmax = xr->zmax;

		    if (twoended) {
			if (zmin == 0.0) zmin += (SCENE_ROAD_WIDTH/2.0 - 0.1);
			else zmin += (SIDE_ROAD_WIDTH/2.0 - 0.1);
			if (zmax == 0.0) zmax -= (SCENE_ROAD_WIDTH/2.0 - 0.1);
			else zmax -= (SIDE_ROAD_WIDTH/2.0 - 0.1);
		    }
		    else if (maxend) {
			if (zmax == 0.0) zmax -= (SCENE_ROAD_WIDTH/2.0 - 0.1);
			else zmax -= (SIDE_ROAD_WIDTH/2.0 - 0.1);
		    }
		    else {
			if (zmin == 0.0) zmin += (SCENE_ROAD_WIDTH/2.0 - 0.1);
			else zmin += (SIDE_ROAD_WIDTH/2.0 - 0.1);
		    }
		    add_object_to_scene(scene,"road","","",0.0,
			xr->xvalue,0.0,zmin,
			zmax-zmin,SIDE_ROAD_WIDTH,0.0,
			0.0,0.0,0.0,0);
		}
	    }
	    else {
		/*** Zroad ***/
		ZROAD *zr = zroad + num_zroads;

		/* Pick a Z. */
		zr->zvalue = GRID_RANDOM_TOWN_LOC(city_radius_increments);

		/* See if we can run in line with an existing road. */
		for (j=0; j<num_zroads; ++j) {
		    if (ABS(zr->zvalue - zroad[j].zvalue)
			    < MIN_ROAD_SPACING/2.0) {
			zr->zvalue = zroad[j].zvalue;
		    }
		}

		/* Connect it to an Xroad on one end. */
		ends = 0;
		for (j=0; j<num_xroads; ++j) {
		    /* Ignore ones that don't connect. */
		    if ((xroad[j].zmin > zr->zvalue)
			    || (xroad[j].zmax < zr->zvalue))
			continue;
		    /* Add to the list of possibilties. */
		    possible_end[ends] = j;
		    ++ends;
		}
		/* Connect it up. */
		twoended = FALSE;
		j = ZINTRAND(ends);
		if (ends > 1) {
		    zr->xmin = xroad[possible_end[j]].xvalue;
		    /* Connect both ends */
		    do {
			k = ZINTRAND(ends);
		    } while (j == k);
		    zr->xmax = xroad[possible_end[k]].xvalue;
		    /* Swap if reversed. */
		    if (zr->xmin > zr->xmax) FLOATSWAP(zr->xmin,zr->xmax);
		    if ((twoended =
		    		((zr->xmax - zr->xmin) > MIN_ROAD_LENGTH))) {
			/* add stop signs */
			ss->x  = zr->xmin   + SIGN_OFFSET;
			ss->z  = zr->zvalue + SIGN_OFFSET;
			ss->rot = M_PI/2.0;
			++ss;
			ss->x = zr->xmax   - SIGN_OFFSET;
			ss->z = zr->zvalue - SIGN_OFFSET;
			ss->rot = -M_PI/2.0;
			++ss;
			new_stopsigns += 2;
		    }
		}

		if (!twoended) {
		    zr->xmin = xroad[possible_end[j]].xvalue;
		    /* Connect only one end. */
		    do {
			zr->xmax = GRID_RANDOM_TOWN_LOC(city_radius_increments);
		    } while (ABS(zr->xmax - zr->xmin) < MIN_ROAD_LENGTH);
		    /* Swap if reversed. */
		    if (zr->xmin > zr->xmax) FLOATSWAP(zr->xmin,zr->xmax);
		    /* Add stop signs. */
		    if (IS_NEAR(zr->xmax,xroad[possible_end[j]].xvalue)) {
			maxend = TRUE;
			ss->x = zr->xmax   - SIGN_OFFSET;
			ss->z = zr->zvalue - SIGN_OFFSET;
			ss->rot = -M_PI/2.0;
		    }
		    else {
			maxend = FALSE;
			ss->x = zr->xmin   + SIGN_OFFSET;
			ss->z = zr->zvalue + SIGN_OFFSET;
			ss->rot = M_PI/2.0;
		    }
		    ++ss; ++new_stopsigns;
		}

		/* Now make sure it's not too close to a parallel road */
		for (j=0; j<num_zroads; ++j) {
		    if ((ABS(zroad[j].zvalue - zr->zvalue) < MIN_ROAD_SPACING) 
			    && (zroad[j].xmin < zr->xmax)
			    && (zroad[j].xmax > zr->xmin)) {
			good_road = FALSE;
			break;
		    }
		}
		if (!good_road) continue;

		/* Mark any crossings with stopsigns */
		for (k=0; k<ends; ++k) {
		    j = possible_end[k];
		    if ((zr->xmin < xroad[j].xvalue)
			    && (zr->xmax > xroad[j].xvalue)
			    && (xroad[j].zmin	< zr->zvalue)
			    && (xroad[j].zmax	> zr->zvalue)) {
			/* Add stopsigns */
			ss->x   = xroad[j].xvalue - SIGN_OFFSET;
			ss->z   = zr->zvalue - SIGN_OFFSET;
			ss->rot = -M_PI/2.0;
			++ss;
			ss->x   = xroad[j].xvalue + SIGN_OFFSET;
			ss->z   = zr->zvalue + SIGN_OFFSET;
			ss->rot = M_PI/2.0;
			++ss;
			new_stopsigns += 2;
		    }
		}

		/* It's good -- add it. */
		++num_zroads;
		{
		    float xmin = zr->xmin;
		    float xmax = zr->xmax;

		    if (twoended) {
			if (xmin == 0.0) xmin += (SCENE_ROAD_WIDTH/2.0 - 0.1);
			else xmin += (SIDE_ROAD_WIDTH/2.0 - 0.1);
			if (xmax == 0.0) xmax -= (SCENE_ROAD_WIDTH/2.0 - 0.1);
			else xmax -= (SIDE_ROAD_WIDTH/2.0 - 0.1);
		    }
		    else if (maxend) {
			if (xmax == 0.0) xmax -= (SCENE_ROAD_WIDTH/2.0 - 0.1);
			else xmax -= (SIDE_ROAD_WIDTH/2.0 - 0.1);
		    }
		    else {
			if (xmin == 0.0) xmin += (SCENE_ROAD_WIDTH/2.0 - 0.1);
			else xmin += (SIDE_ROAD_WIDTH/2.0 - 0.1);
		    }
		    add_object_to_scene(scene,"road","","",-M_PI/2.0,
			xmin,0.0,zr->zvalue,
			xmax-xmin,SIDE_ROAD_WIDTH,0.0,
			0.0,0.0,0.0,0);
		}
	    }
	    for (ss = stopsign; ss < stopsign + new_stopsigns; ++ss) {
		add_object_to_scene(scene,"stop sign","stop","",ss->rot,
		    ss->x,0.0,ss->z,
		    0.0,0.0,SIGN_HEIGHT,
		    0.0,0.0,0.0,0);
	    }
	} while (!good_road);
    }

    /* Drop in some areas */
    do {
	int type, type_count[AREA_TYPE_MAX+1];

	area_count = 0;
	for (type=0; type<= AREA_TYPE_MAX; ++type) {
	    type_count[type] = 0;
	}
	for (i=0,aptr=area; i<num_areas; ++i) {
	    float x,z;
	    int checks,good_spot,spot_checks;

	    checks = 0;
	    do {
		/* Pick a random spot. */
		spot_checks = 0;
		do {
		    x = RANDOM_TOWN_LOC(city_radius);
		    z = RANDOM_TOWN_LOC(city_radius);
		    /* See if we're already inside a maximum-sized area.
		     * If so, there's no point in continuing, because we're
		     * just going to expand to the same area and have to
		     * start over with another spot.
		     */
		    good_spot = TRUE;
		    for (aptr2=area; aptr2 < aptr; ++aptr2) {
			if ((aptr2->maxsize)
				&& (aptr2->left <= x)
				&& (aptr2->right >= x)
				&& (aptr2->bottom <= z)
				&& (aptr2->top >= z)) {
			    good_spot = FALSE;
			    break;
			}
		    }
		    if (++spot_checks > 100) break;
		} while (!good_spot);
		if (spot_checks > 100) break;

		aptr->left = aptr->bottom = -city_radius;
		aptr->right = aptr->top = city_radius;
		aptr->leftroad = aptr->rightroad = NULL;
		aptr->downroad = aptr->uproad = NULL;
		/* Go left and right until we hit a road. */
		for (j=0; j<num_xroads; ++j) {
		    if ((xroad[j].zmin > z) || (xroad[j].zmax < z)) continue;
		    if (xroad[j].xvalue < x) {
			if (xroad[j].xvalue > aptr->left) {
			    aptr->left = xroad[j].xvalue;
			    aptr->leftroad = xroad + j;
			}
		    }
		    else {
			if (xroad[j].xvalue < aptr->right) {
			    aptr->right = xroad[j].xvalue;
			    aptr->rightroad = xroad + j;
			}
		    }
		}
		/* Now top and bottom over the whole range. */
		for (j=0; j<num_zroads; ++j) {
		    if ((zroad[j].xmin >= aptr->right)
			    || (zroad[j].xmax <= aptr->left))
			continue;
		    if (zroad[j].zvalue < z) {
			if (zroad[j].zvalue > aptr->bottom) {
			    aptr->bottom = zroad[j].zvalue;
			    aptr->downroad = zroad + j;
			}
		    }
		    else {
			if (zroad[j].zvalue < aptr->top) {
			    aptr->top = zroad[j].zvalue;
			    aptr->uproad = zroad + j;
			}
		    }
		}

		/* Make sure there are no xroads poking down into the area. */
		for (j=0; j<num_xroads; ++j) {
		    if ((xroad[j].xvalue <= aptr->left)
			    || (xroad[j].xvalue >= aptr->right)
			    || (xroad[j].zmin >= aptr->top)
			    || (xroad[j].zmax <= aptr->bottom)) 
			continue;
		    if ((xroad[j].zmin < aptr->top)
			    && (xroad[j].zmin > aptr->bottom)) {
			aptr->top   = xroad[j].zmin;
			aptr->uproad = NULL;
		    }
		    else if ((xroad[j].zmax < aptr->top)
			    && (xroad[j].zmax > aptr->bottom)) {
			aptr->bottom = xroad[j].zmax;
			aptr->downroad = NULL;
		    }
		}

		/* Check for overlap/redundant. */
		for (aptr2=area; aptr2 < aptr; ++aptr2) {
		    if ((aptr2->right > aptr->left)
			    && (aptr2->left < aptr->right)
			    && (aptr2->top > aptr->bottom)
			    && (aptr2->bottom < aptr->top)) {
			/* They overlap. */
			if (aptr->left < aptr2->left) {
			    /* Throw away my right side. */
			    aptr->right = aptr2->left;
			    aptr->rightroad = NULL;
			}
			else if (aptr->right > aptr2->right) {
			    /* Throw away my left side. */
			    aptr->left = aptr2->right;
			    aptr->leftroad = NULL;
			}
			else if (aptr->bottom < aptr2->bottom) {
			    /* Throw away my top. */
			    aptr->top = aptr2->bottom;
			    aptr->uproad = NULL;
			}
			else if (aptr->top > aptr2->top) {
			    /* Throw away my botttom. */
			    aptr->bottom = aptr2->top;
			    aptr->downroad = NULL;
			}
			else {
			    /* Hosed */
			    aptr->left = aptr->right;
			    /* No need to check against further areas. */
			    break;
			}
		    }
		}

		if (++checks > num_areas*2) break;  /* No place to put it. */
	    } while ((aptr->right <= aptr->left)
		    || (aptr->top <= aptr->bottom));

	    if (((aptr->right - aptr->left) > MIN_ROAD_SPACING)
		    && ((aptr->top - aptr->bottom) > MIN_ROAD_SPACING)) {
		set_area_attachment(aptr);

		/* If too big, cut down in size. */
		if (((aptr->right - aptr->left) > MAX_AREA_SIZE)
			|| ((aptr->top - aptr->bottom) > MAX_AREA_SIZE)) {
		    position_area(aptr, MAX_AREA_SIZE);
		    /* Now refigure attachment */
		    set_area_attachment(aptr);
		    aptr->maxsize = FALSE;
		}
		else {
		    aptr->maxsize = TRUE;
		}

		aptr->type = town_area_type(aptr->attach,city_type);
		++type_count[aptr->type];
		++area_count;
		++aptr;
	    }
	}

	/* Now some simple checks. */
	good_areas = TRUE;
	if (area_count < num_areas/2)
	    good_areas = FALSE;
	else if ((type_count[AREA_TYPE_PARKING_LOT] > 0)
		&& (type_count[AREA_TYPE_LOW_SKYSCRAPER] == 0)
		&& (type_count[AREA_TYPE_SKYSCRAPER] == 0))
	    good_areas = FALSE;
	else if ((type_count[AREA_TYPE_HOUSE] == 0)
		&& (type_count[AREA_TYPE_LOW_SKYSCRAPER] == 0)
		&& (type_count[AREA_TYPE_SKYSCRAPER] == 0))
	    good_areas = FALSE;
	else if ((city_type != CITY_TYPE_CITY)
		&& (type_count[AREA_TYPE_HOUSE] == 0)) 
	    good_areas = FALSE;
	else if ((city_type == CITY_TYPE_CITY)
		&& (type_count[AREA_TYPE_LOW_SKYSCRAPER] == 0)
		&& (type_count[AREA_TYPE_SKYSCRAPER] == 0))
	    good_areas = FALSE;
    } while (!good_areas);

    /* Add areas to list */
    for (aptr=area; aptr < area + area_count; ++aptr) {
	add_area_objects(scene,aptr);
    }
}

