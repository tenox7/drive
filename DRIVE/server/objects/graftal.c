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


/* Code Module for various growing things. */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "global.h"
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "prims.h"
#include "obj_common.h"
#include "demo_physics.h"

#define Height	(obj->size[SIZE_HEIGHT])
#define Length	(obj->size[SIZE_LENGTH])
#define Width	(obj->size[SIZE_WIDTH])
#define Radius	(obj->radius)
#define Count	(obj->count)

/*****************************************************************/

typedef struct _tree_list {
    float height;
    unsigned int nameset_bits;
    int dl_number;
    struct _tree_list *next;
} TREE_LIST;

static TREE_LIST *tree_list = NULL;


static int tree_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = obj->size[SIZE_HEIGHT];
    get_pole_mc_normal(obj,x,y,z,sc->mc_normal);
    return(TRUE);
}


static int tree_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = obj->size[SIZE_HEIGHT];
    return(TRUE);
}


/*****************************************************************/

#define DEFAULT_FOREST_COUNT	25
#define DEFAULT_FOREST_HEIGHT	30.0
#define DEFAULT_FOREST_LENGTH	50.0
#define DEFAULT_FOREST_WIDTH	50.0

typedef struct _forest_list {
    float height, length, width;
    unsigned int nameset_bits;
    int count;
    int dl_number;
    struct _forest_list *next;
} FOREST_LIST;

static FOREST_LIST *forest_list = NULL;


static int forest_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    if ( x*x + z*z < 1.0 ) 
    {
	sc->mc_y = obj->size[SIZE_HEIGHT];
	get_pole_mc_normal( obj, x, y, z, sc->mc_normal );
	return TRUE;
    }
    /* else */
    return FALSE;
}


static int forest_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = obj->size[SIZE_HEIGHT];
    return TRUE;
}


/*****************************************************************/

#define DEFAULT_WEEDPATCH_COUNT		25
#define DEFAULT_WEEDPATCH_HEIGHT	3.0
#define DEFAULT_WEEDPATCH_LENGTH	50.0
#define DEFAULT_WEEDPATCH_WIDTH		50.0

typedef struct _weedpatch_list {
    float height, length, width;
    unsigned int nameset_bits;
    int count;
    int dl_number;
    struct _weedpatch_list *next;
} WEEDPATCH_LIST;

static WEEDPATCH_LIST *weedpatch_list = NULL;

/*****************************************************************/

#define DEFAULT_GRASS_COUNT	25
#define DEFAULT_GRASS_HEIGHT	3.0
#define DEFAULT_GRASS_LENGTH	50.0
#define DEFAULT_GRASS_WIDTH	50.0
#define DEFAULT_GRASS_RADIUS	0.0

typedef struct _GRASS_list {
    float height, length, width, radius;
    unsigned int nameset_bits;
    int count;
    int dl_number;
    struct _GRASS_list *next;
} GRASS_LIST;

static GRASS_LIST *grass_list = NULL;

/*****************************************************************/

#define TRUNK_SIZE	(Height/50.0)
#define	X	0
#define	Y	1
#define	Z	2
#define EPS .001

#define	DEFAULT_TREE_HEIGHT	15.0

typedef unsigned char segment;

typedef struct gs
{
    float position[3];		/* Position. */
    float direction[3];		/* Direction. */
    float length;		/* Length of each segment. */
    int level;			/* Current branch level. */
    int quadrant;		/* Current branch quadrant. */

    struct gs *next;		/* Next pointer for stack. */
} graftal_state_type;

/* State stack. */
graftal_state_type *graftal_state_stack = (graftal_state_type *)NULL;

/*****************************************************************
 * graftal_push -
 *  
 */
static void graftal_push(
    graftal_state_type *state)
{
    graftal_state_type *temp;

    temp = (graftal_state_type *)malloc( sizeof(graftal_state_type) );
    memcpy( temp, state, sizeof( graftal_state_type ) );
    temp->next = graftal_state_stack;
    graftal_state_stack = temp;
}


/*****************************************************************
 * graftal_pop -
 *  
 */
static void graftal_pop(
    graftal_state_type *state)
{
    graftal_state_type *temp = graftal_state_stack;

    if ( temp )
    {
	memcpy( state, temp, sizeof( graftal_state_type ) );
	graftal_state_stack = temp->next;
	free( temp );
    }
    else
    {
	fprintf( stderr, "Empty graftal stack.\n" );
	exit( -1 );
    }
}


/*****************************************************************
 * graftal_rule - Identify which growth rule to use in replacing 
 *               the segment at "index" in "string".
 */
static int graftal_rule(
    segment *string,
    int index)
{
    int left_index = index;
    int right_index = index;
    int rule = 0;
    int length = strlen((char *) string );
    int level;
    int done;

    /* Center. */
    if ( string[index] == '1' )
	rule |= 0x2;

    /* Left.  Search backward until we find the "left neighbor" */
    if ( index == 0 )
	/* Unattached ends of branches have implicit '1'. */
	rule |= 0x4;
    else
    {
	level = 0;
	done = FALSE;
	while ( left_index-- && !done )
	{
	    switch ( string[left_index] )
	    {
	      case '[':
		if ( level > 0 )
		    level--;
		break;

	      case ']':
		level++;
		break;
		
	      case '1':
		if ( level == 0 )
		    rule |= 0x4;
		/* Fall through. */

	      case '0':
		if ( level == 0 )
		    done = TRUE;
		break;
	    }

	    if ( (!done) && (left_index == 0) )
		/* Unattached ends of branches have implicit '1'. */
		rule |= 4;
	}
    }

    /* Right.  Search forward until we find the "right neighbor" */
    if ( index == length-1 )
	rule |= 0x1;
    else 
    {
	level = 0;
	done = FALSE;
	while ( (++right_index < length) && !done )
	{
	    switch ( string[right_index] )
	    {
	      case '[':
		level++;
		break;
		
	      case ']':
		if ( level == 0 )
		{
		    /* Unattached ends of branches have implicit '1'. */
		    rule |= 0x1;
		    done = TRUE;
		}
		else
		    level--;
		break;
		
	      case '1':
		if ( level == 0 )
		    rule |= 0x1;
		/* Fall through. */
		
	      case '0':
		if ( level == 0 )
		    done = TRUE;
		break;
	    }
	    
	    if ( (!done) && (right_index == length-1) )
		/* Unattached ends of branches have implicit '1'. */
		rule |= 0x1;
	}
    }

    return rule;
}


/*****************************************************************
 * graftal_size - Computes the length of the word produced from the
 *               word "string" after 1 generation.
 */
static int graftal_size(
    segment *string,
    segment *rules[8])
{
    int i;
    unsigned int length = 0;

    for ( i=0; i<strlen((char *) string); i++ )
    {
	if ( (string[i] == '0') || (string[i] == '1') )
	    length += strlen((char *) rules[graftal_rule( string, i )] );
	else
	    length++;
    }

    return length;
}

/*****************************************************************
 * graftal_word - Computes the word produced at "generation".
 */
static segment *graftal_word(
    segment *axiom,
    segment *rules[8],
    int generation)
{
    int i, j, k;
    segment *word, *wordptr, *newword;
    segment *newwordptr, *ruleptr;
    int rulelen, rule;
    int wordlen, newwordlen;

    /* Initialize word to the axiom word. */
    word = (segment *)strdup((char *) axiom );

    for ( i = 1; i < generation; i++ )
    {
	newwordlen = graftal_size( word, rules );
	newword = (segment *)calloc( newwordlen + 1, 1 );
	newwordptr = newword;
	wordlen = strlen((char *) word);
	wordptr = word;

	for ( j = 0; j < wordlen; j++ )
	{
	    if ( (*wordptr == '0') || (*wordptr == '1') )
	    {
		rule = graftal_rule( word, j );
		ruleptr = rules[ rule ];
		rulelen = strlen((char *) ruleptr);
		
		/* Copy expansion into new word space. */
		for ( k = 0; k < rulelen; k++ )
		    *newwordptr++ = *ruleptr++;
	    }		    
	    else
		*newwordptr++ = *wordptr;
	    wordptr++;
	}

	free( word );
	word = newword;
    }

    return word;
}


#if 0
/*****************************************************************
 * graftal_leaf -
 */
static int graftal_leaf(
    segment *string,
    int index)
{
    if ( index == (strlen((char *) string) - 1) )
	return TRUE;

    if ( string[index+1] == ']' )
	return TRUE;

    return FALSE;
}
#endif


/*****************************************************************
 * graftal_angle - Computes the around angle for a branch.  Tries
 *                 to keep a balance between quadrants.
 */
static float graftal_angle(
    graftal_state_type *state)
{
    float angle;

    switch ( state->quadrant )
    {
      case 0:
	/* Compute a random angle in quadrant 0. */
	angle = FLOATRAND(M_PI_2);
	state->quadrant = 2;
	break;

      case 1:
	/* Compute a random angle in quadrant 1. */
	angle = BOUNDED_FLOATRAND(M_PI_2,M_PI);
	state->quadrant = 3;
	break;

      case 2:
	/* Compute a random angle in quadrant 2. */
	angle = BOUNDED_FLOATRAND(M_PI,M_PI*3/2);
	state->quadrant = 1;
	break;

      case 3:
      default:
	/* Compute a random angle in quadrant 3. */
	angle = BOUNDED_FLOATRAND(M_PI*3/2,M_PI*2);
	state->quadrant = 0;
	break;
    }
    return angle;
}


/*****************************************************************
 * graftal_length - Computes the number of coordinates needed for
 *                  the polyline corresponding to "word."
 */
static int graftal_length(
    segment *word,
    int *trunklen, int *leaflen)
{
    int i;
    segment *wordptr = word;
    int total = 0;
    int trunk = 0;
    int leaf = 0;
    int wordlen;

    /* Move to inital position. */
    total++;
    trunk++;

    wordlen = strlen((char *) word );
    for ( i = 0; i < wordlen; i++ )
    {
	switch ( *wordptr )
	{
	  case ']':		/* Move. */
	    trunk++;
	    total++;
	    break;

	  case '0':		/* Draw. */
	  case '1':		/* Draw. */
	    if ((i==wordlen-1) || ((i<wordlen-1) && (*(wordptr+1) == ']')))
		leaf += 2;
	    else
		trunk++;

	    total++;
	    break;

	  case '[':
	  default:
	    break;
	}
	wordptr++;
    }

    if ( trunklen )
	*trunklen = trunk;
    if ( leaflen )
	*leaflen = leaf;

    return total;
}


/*****************************************************************
 * graftal_direction - Computes a random vector based on the
 *		       direction of the current one and replaces
 *                     the one in state->direction.
 */
static void graftal_direction(
    graftal_state_type *state,
    float alpha, float beta)
{
    float d, rotvec[3], ppvec[3];

    /* Generate a random vector in the XZ plane from the rotation
     * angle.
     */
    rotvec[X] = cos( beta );
    rotvec[Y] = 0.0;
    rotvec[Z] = sin( beta );

    /* Use the rotation vector to get a vector in the plane 
     * perpendicular to the current direction vector.
     */
    if ( APX_EQ( state->direction[Y], 0.0 ) )
    {
	ppvec[X] = 0.0;
	ppvec[Y] = 1.0;
	ppvec[Z] = 0.0;
    }
    else
    {
	ppvec[X] = state->direction[Y] * rotvec[Z]
	    - state->direction[Z] * rotvec[Y];
	ppvec[Y] = state->direction[X] * rotvec[Z]
	    - state->direction[Z] * rotvec[X];
	ppvec[Z] = state->direction[X] * rotvec[Y]
	    - state->direction[Y] * rotvec[X];
	NORMALIZE3( ppvec[X], ppvec[Y], ppvec[Z] );
    }
	    
    /* Make the perpendicular direction vector the correct
     * length.
     */  
    d = tan( alpha );
    ppvec[X] *= d;
    ppvec[Y] *= d;
    ppvec[Z] *= d;

    /* Add perpendicular direction vector to direction vector to
     * get new direction.
     */
    state->direction[X] += ppvec[X];
    state->direction[Y] += ppvec[Y];
    state->direction[Z] += ppvec[Z];
    NORMALIZE3( state->direction[X],
	       state->direction[Y],
	       state->direction[Z] );
}


/*****************************************************************
 * graftal_polyline - Computes the polyline corresponding to "word."
 *                    If the "leaf" parameter is NULL, the whole
 *                    graftal will be put in "trunk".
 */
static void graftal_polyline(
    segment *word,
    float *trunk, float *leaf,
    float root_x, float root_y, float root_z,
    float seglen)
{
    int i, wordlen;
    graftal_state_type state;
    segment *wordptr = word;
    float *trunkptr = trunk;
    float *leafptr = leaf;
    float alpha, beta;
    
    /* Set initial state. */
    state.position[X] = root_x;
    state.position[Y] = root_y;
    state.position[Z] = root_z;
    state.direction[X] = 0.0;
    state.direction[Y] = 1.0;
    state.direction[Z] = 0.0;
    state.length = seglen;
    state.level = 0;
    state.quadrant = ZINTRAND(4);

    /* Move to inital position. */
    *trunkptr++ = root_x;
    *trunkptr++ = root_y;
    *trunkptr++ = root_z;
    *trunkptr++ = 0.0;

    wordlen = strlen((char *) word );
    for ( i = 0; i < wordlen; i++ )
    {
	switch ( *wordptr )
	{
	  case '[':
	    /* Generate random angles.  Alpha is the branch angle.  Beta
	     * is the rotation angle.
	     */
	    alpha = FLOATRAND(M_PI_4 - 0.2) + 0.2;
	    beta = graftal_angle( &state );

	    /* Save state and change growth direction. */
	    graftal_push( &state );
	    graftal_direction( &state, alpha, beta );
	    state.level++;
	    state.quadrant = ZINTRAND(4);
	    break;

	  case ']':
	    /* Restore state and move back to restored position. */
	    graftal_pop( &state );

	    *trunkptr++ = state.position[X];
	    *trunkptr++ = state.position[Y];
	    *trunkptr++ = state.position[Z];
	    *trunkptr++ = 0.0;
	    break;
		
	  case '0':
	  case '1':
	    if ( (leaf) &&
		 ((i==wordlen-1) || ((i<wordlen-1) && (*(wordptr+1) == ']'))) )
	    {
		/* Move to base of leaf. */
		*leafptr++ = state.position[X];
		*leafptr++ = state.position[Y];
		*leafptr++ = state.position[Z];
		*leafptr++ = 0.0;

		/* Grow in current direction and draw leaf. */
		state.position[X] += (state.direction[X] * state.length/2.0);
		state.position[Y] += (state.direction[Y] * state.length/2.0);
		state.position[Z] += (state.direction[Z] * state.length/2.0);

		*leafptr++ = state.position[X];
		*leafptr++ = state.position[Y];
		*leafptr++ = state.position[Z];
		*leafptr++ = 1.0;
	    }
	    else
	    {
		/* Grow in current direction and draw segment. */
		state.position[X] += (state.direction[X] * state.length);
		state.position[Y] += (state.direction[Y] * state.length);
		state.position[Z] += (state.direction[Z] * state.length);

		*trunkptr++ = state.position[X];
		*trunkptr++ = state.position[Y];
		*trunkptr++ = state.position[Z];
		*trunkptr++ = 1.0;
	    }
	    break;
	}
	wordptr++;
    }
}

/****
static segment *tree_rules[8] =
{
    (segment *)"1[0]1[0]0",
    (segment *)"1[0]1[0]0",
    (segment *)"11",
    (segment *)"11",
    (segment *)"1[0]1[0]0",
    (segment *)"1[0]1[0]0",
    (segment *)"11",
    (segment *)"11"
};
****/

static segment *grass_rules[8] =
{
    (segment *)"1[1]",
    (segment *)"1[1]",
    (segment *)"1[1]",
    (segment *)"1[1]",
    (segment *)"1[1]",
    (segment *)"1[1]",
    (segment *)"1[1]",
    (segment *)"1[1]"
};

#if defined HOVERWARE_MODEL
/*****************************************************************
 * grass_graphics -
 */
static hwObject grass_graphics(
    float height)
{
    segment *word;
    int length;
    float *clist;
    hwObject curr;
    
    word = graftal_word((segment *) "[1][1][1][1][1]", grass_rules, 3 );
    length = graftal_length( word, NULL, NULL );
    
    /* Malloc space for polyline. */
    clist = (float *)malloc( length * 4 * sizeof(float) );
    graftal_polyline( word, clist, NULL, 0.0, 0.0, 0.0, height/2.0 );
    
    curr = hwPolyline->create( hwPolyline );
    HW_MODIFY_3F( curr, hwStrColor, 0.4, 0.7, 0.3 );
    HW_MODIFY_1B( curr, hwStrHasFlags, HW_TRUE );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,4*length), clist );

    free( clist );

    return curr;
}
#endif

static segment *weedpatch_rules[8] =
{
    (segment *)"0",
    (segment *)"1[1]",
    (segment *)"1",
    (segment *)"1",
    (segment *)"0",
    (segment *)"11",
    (segment *)"1",
    (segment *)"0"
};

#if defined(HOVERWARE_MODEL)
/*****************************************************************
 * weedpatch_graphics -
 */
static hwObject weedpatch_graphics(
    float height)
{
    segment *word;
    int length;
    float *clist;
    float dist;
    hwObject curr, oList[10];
    int nObjs = 0;

    word = graftal_word((segment *) "1", weedpatch_rules, 15 );
    length = graftal_length( word, NULL, NULL );
    clist = (float *)malloc( length * 4 * sizeof(float) );

    graftal_polyline( word, clist, NULL, 0.0, 0.0, 0.0, height/10.0 );

    curr = hwPolyline->create( hwPolyline );
    HW_MODIFY_3F( curr, hwStrColor, 0.7, 0.6, 0.3 );
    HW_MODIFY_1B( curr, hwStrHasFlags, HW_TRUE );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,4*length), clist );
    oList[nObjs++] = curr;

    word = graftal_word((segment *) "1", weedpatch_rules, 15 );
    dist = FLOATRAND(height/2.0);
    graftal_polyline( word, clist, NULL, dist, 0.0, dist, height/10.0 );

    word = graftal_word((segment *) "1", weedpatch_rules, 15 );
    dist = FLOATRAND(height/2.0);
    graftal_polyline( word, clist, NULL, dist, 0.0, -dist, height/10.0 );
    curr = hwPolyline->create( hwPolyline );
    HW_MODIFY_3F( curr, hwStrColor, 0.7, 0.6, 0.3 );
    HW_MODIFY_1B( curr, hwStrHasFlags, HW_TRUE );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,4*length), clist );
    oList[nObjs++] = curr;

    word = graftal_word((segment *) "1", weedpatch_rules, 15 );
    dist = FLOATRAND(height/2.0);
    graftal_polyline( word, clist, NULL, -dist, 0.0, dist, height/10.0 );
    curr = hwPolyline->create( hwPolyline );
    HW_MODIFY_3F( curr, hwStrColor, 0.7, 0.6, 0.3 );
    HW_MODIFY_1B( curr, hwStrHasFlags, HW_TRUE );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,4*length), clist );
    oList[nObjs++] = curr;

    word = graftal_word((segment *) "1", weedpatch_rules, 15 );
    dist = FLOATRAND(height/2.0);
    graftal_polyline( word, clist, NULL, -dist, 0.0, -dist, height/10.0 );
    curr = hwPolyline->create( hwPolyline );
    HW_MODIFY_3F( curr, hwStrColor, 0.7, 0.6, 0.3 );
    HW_MODIFY_1B( curr, hwStrHasFlags, HW_TRUE );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,4*length), clist );
    oList[nObjs++] = curr;

    free( clist );

    curr = hwGroup->create( hwGroup );
    curr->modify( curr, hwStrChildren,
		HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
    return curr;
}
#endif


/*****************************************************************
 * tree_graphics -
 */
static segment *mytree_rules[8] =
{
    (segment *)"1[0]1[0]0",
    (segment *)"1[0]1[0]0",
    (segment *)"1",
    (segment *)"1",
    (segment *)"1[0]1[0]0",
    (segment *)"1[0]1[0]0",
    (segment *)"1",
    (segment *)"1"
};


static void create_tree_graphics(
    DRIVE_OBJECT *obj)
{
    segment *word;
    int trunklen, leaveslen;
    float *trunk, *leaves;
    hwObject curr, oList[10];
    int nObjs = 0;
    
    /* Generate grammar word. */
    word = graftal_word((segment *) "1[0][0][0]0", mytree_rules, 4 );
    graftal_length( word, &trunklen, &leaveslen );
    
    /* Malloc space for polylines. */
    trunk = (float *)malloc( trunklen * 4 * sizeof(float) );
    leaves = (float *)malloc( leaveslen * 4 * sizeof(float) );
    graftal_polyline( word, trunk, leaves, 0.0, Height/3.0, 0.0, Height/10.0 );
    
    curr = hwPolyline->create( hwPolyline );
    HW_MODIFY_3F( curr, hwStrColor, 0.5, 0.4, 0.1 );
    HW_MODIFY_1B( curr, hwStrHasFlags, HW_TRUE );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,4*trunklen), trunk );
    oList[nObjs++] = curr;

    curr = hwPolyline->create( hwPolyline );
    HW_MODIFY_3F( curr, hwStrColor, 0.13, 0.56, 0.13 );
    HW_MODIFY_1B( curr, hwStrHasFlags, HW_TRUE );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,4*leaveslen), leaves );
    oList[nObjs++] = curr;

    curr = hwCone->create( hwCone );
    HW_MODIFY_2F( curr, hwStrRadius, (Height/50.0), 0.05 );
    HW_MODIFY_1F( curr, hwStrHeight, Height/2.0 );
    HW_MODIFY_3F( curr, hwStrRotate, 90.0, 0.0, 0.0 );
    HW_MODIFY_1I( curr, hwStrGraphN, 2 );
    HW_MODIFY_1I( curr, hwStrGraphM, 5 );
    WOOD_HW( curr );
    oList[nObjs++] = curr;

    /* TBD: LOD */
    HW_OBJECT_NAMESET(curr,obj);
    curr = hwGroup->create( hwGroup );
    curr->modify( curr, hwStrChildren,
		HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );

    obj->display_list = createHwSegmentFromObj( &curr, 1 );
}


void init_tree_object(
    DRIVE_OBJECT *obj)
{
    TREE_LIST *tl;

    if (Height <= 0.0)  Height  = DEFAULT_TREE_HEIGHT;

    if ( debug )
	printf(" inside init_tree_%d_object() routine \n", (int) Height );

    /* See if we've created a tree this tall before... */
    tl = tree_list;
    while (tl != NULL) {
	if (IS_NEAR(tl->height,Height)
		&& (tl->nameset_bits == obj->nameset_bits))
	    break;
	/* else */
	tl = tl->next;
    }

    if (tl != NULL) {
	/* good -- already got one. */
	obj->display_list = tl->dl_number;
    }
    else {
	/* Nope -- gotta create new one */
	if ((tl = (TREE_LIST *) malloc(sizeof(TREE_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}
	tl->height = Height;
	tl->nameset_bits = obj->nameset_bits;

	create_tree_graphics( obj );

	tl->dl_number = obj->display_list;
	tl->next = tree_list;
	tree_list = tl;
    }

    obj->size[SIZE_LENGTH] = TRUNK_SIZE*2.0;
    obj->size[SIZE_WIDTH]  = TRUNK_SIZE*2.0;

    obj->surface_chars_xyz  = tree_surface_chars_xyz;
    obj->surface_chars_bbox = tree_surface_chars_bbox;

    obj->bound_mc[0] = -TRUNK_SIZE;
    obj->bound_mc[1] = 0.0;
    obj->bound_mc[2] = -TRUNK_SIZE;
    obj->bound_mc[3] = TRUNK_SIZE;
    obj->bound_mc[4] = Height;
    obj->bound_mc[5] = TRUNK_SIZE;

    /* Update WC bounding box */
    update_wc_bounds(obj);

    elevate_object_to_terrain_height((SCENE *) obj->scene,obj,FALSE);
}

/*****************************************************************/

static void create_forest_graphics(
    DRIVE_OBJECT *obj)
{
    int forestseg, *seglist;
    segment *word;
    int trunklen, leaveslen;
    float *trunk, *leaves;
    float (*mats)[4][4];
    float x, z;
    int i, nObjs = 0;
    hwObject curr, oList[10];
    
    /* Generate grammar word. */
    word = graftal_word((segment *) "1[0][0][0]0", mytree_rules, 4 );
    graftal_length( word, &trunklen, &leaveslen );
    
    /* Malloc space for polylines. */
    trunk = (float *)malloc( trunklen * 4 * sizeof(float) );
    leaves = (float *)malloc( leaveslen * 4 * sizeof(float) );
    graftal_polyline( word, trunk, leaves, 0.0, Height/3.0, 0.0, Height/10.0 );
    
    curr = hwPolyline->create( hwPolyline );
    HW_MODIFY_1B( curr, hwStrHasFlags, HW_TRUE );
    HW_MODIFY_3F( curr, hwStrColor, 0.5, 0.4, 0.1 );
    curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,trunklen*4), trunk );
    oList[nObjs++] = curr;

    curr = hwPolyline->create( hwPolyline );
    HW_MODIFY_1B( curr, hwStrHasFlags, HW_TRUE );
    HW_MODIFY_3F( curr, hwStrColor, 0.13, 0.56, 0.13 );
    curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,leaveslen*4), leaves );
    oList[nObjs++] = curr;

    curr = hwCone->create( hwCone );
    HW_MODIFY_2F( curr, hwStrRadius, Height/50.0, 0.05 );
    HW_MODIFY_1F( curr, hwStrHeight, Height/2.0 );
    HW_MODIFY_1I( curr, hwStrGraphN, 2 );
    HW_MODIFY_1I( curr, hwStrGraphM, 4 );
    WOOD_HW( curr );
    HW_MODIFY_3F( curr, hwStrRotate, 90.0, 0.0, 0.0 );
    oList[nObjs++] = curr;

    /* TBD: LOD */
    curr = hwGroup->create( hwGroup );
    curr->modify( curr, hwStrChildren,
		HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );

    forestseg = createHwSegmentFromObj( &curr, 1 );

    /* Create forest. */
    seglist = malloc( Count * sizeof(int) );
    mats = malloc( Count * 4 * 4 * sizeof(float) );

    for ( i = 0; i < Count; i++ ) {
	seglist[i] = forestseg;

	x = FLOATRAND(Width);
	z = FLOATRAND(Length);

	hwIdentity( mats[i] );
	mats[i][3][0] = x;
	mats[i][3][2] = z;
    }

    obj->display_list = createHwSegmentList( Count, seglist, mats );
}


void init_forest_object(
    DRIVE_OBJECT *obj)
{
    FOREST_LIST *tl;
    int i;
    DRIVE_OBJECT *child;
    static float mat[4][4] = IDENTITY4x4;


    if (Height <= 0.0)  Height  = DEFAULT_FOREST_HEIGHT;
    if (Length <= 0.0)  Length  = DEFAULT_FOREST_LENGTH;
    if (Width <= 0.0)  	Width   = DEFAULT_FOREST_WIDTH;
    if (Count <= 0)	Count	= DEFAULT_FOREST_COUNT;

    if ( debug )
	printf(" inside init_forest_%d_object() routine \n", (int) Height );

    obj->num_children = Count;

    /* See if we've created a forest this tall before... */
    tl = forest_list;
    while (tl != NULL) {
	if (IS_NEAR(tl->height,Height)
		&& IS_NEAR(tl->width,Width)
		&& IS_NEAR(tl->length,Length)
		&& (tl->nameset_bits == obj->nameset_bits)
		&& (tl->count == Count)) {
	    break;
	}
	/* else */
	tl = tl->next;
    }

    if (tl != NULL) {
	/* good -- already got one. */
	obj->display_list = tl->dl_number;
    }
    else {
	/* Nope -- gotta create new one */
	if ((tl = (FOREST_LIST *) malloc(sizeof(FOREST_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}

	tl->height = Height;
	tl->width = Width;
	tl->length = Length;
	tl->count = Count;
	tl->nameset_bits = obj->nameset_bits;

	create_forest_graphics( obj );

	tl->dl_number = obj->display_list;
	tl->next = forest_list;
	forest_list = tl;
    }

    obj->surface_chars_xyz  = forest_surface_chars_xyz;
    obj->surface_chars_bbox = forest_surface_chars_bbox;

    obj->bound_mc[0] = -TRUNK_SIZE;
    obj->bound_mc[1] = 0.0;
    obj->bound_mc[2] = -TRUNK_SIZE;
    obj->bound_mc[3] = Width;
    obj->bound_mc[4] = Height;
    obj->bound_mc[5] = Length;

    /* Update WC bounding box */
    update_wc_bounds(obj);

    /* Now create subobjects for each line */
    for ( i = 0; i < obj->num_children; i++ )
    {
	if ((child = (DRIVE_OBJECT *) malloc(sizeof(DRIVE_OBJECT))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    break;
	}

	/* as a good starting point, clone myself */
	memcpy( child, obj, sizeof(DRIVE_OBJECT) );

	/* now change relevant parts */
	child->num_children = 0;
	child->child_list = NULL;
	child->display_list = 0;
/* 	mat[3][2] = z; */

	concat_matrix( mat, obj->xform, child->xform );

	_hp_invert( child->xform, child->ixform, 0 );

	child->bound_mc[0] = -TRUNK_SIZE;
	child->bound_mc[1] = 0.0;
	child->bound_mc[2] = -TRUNK_SIZE;
	child->bound_mc[3] = TRUNK_SIZE;
	child->bound_mc[4] = Height;
	child->bound_mc[5] = TRUNK_SIZE;

	update_wc_bounds( child );

	add_object_to_list( &(obj->child_list), child );
    }
}

/*****************************************************************/

static segment *weed_rules[8] =
{
    (segment *)"0",
    (segment *)"0",
    (segment *)"0",
    (segment *)"11",
    (segment *)"1",
    (segment *)"1[1]",
    (segment *)"1",
    (segment *)"0"
};


static void create_weedpatch_graphics(
    DRIVE_OBJECT *obj)
{
    int length;
    float *clist;
    segment *word;
    int levels;
    float x, y, z;
    int i;
    hwObject curr, oList[1000];
    int nObjs = 0;

    /* Generate grammar word. */
    levels = (int)(Height * 5.0);
    word = graftal_word((segment *) "1", weed_rules, levels );
    length = graftal_length( word, NULL, NULL );
    
    /* Malloc space for polyline. */
    clist = (float *)malloc( length * 4 * sizeof(float) );
    
    /* Create weedpatch. */
    for ( i = 0; i < Count; i++ )
    {
	x = FLOATRAND(Width);
	y = 0.0;
	z = FLOATRAND(Length);

	graftal_polyline( word, clist, NULL, x, y, z,
			 Height/(float)levels );

	curr = hwPolyline->create( hwPolyline );
	HW_MODIFY_3F( curr, hwStrColor, 0.2, 0.8, 0.1 );
	HW_MODIFY_1B( curr, hwStrHasFlags, HW_TRUE );
	curr->modify( curr, hwStrData,
		    HW_MAKE_TYPE(HW_TYPE_FLOAT,length*4), clist );
	oList[nObjs++] = curr;
    }

    curr = hwGroup->create( hwGroup );
    HW_OBJECT_NAMESET(curr,obj);
    curr->modify( curr, hwStrChildren,
		HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
    obj->display_list = createHwSegmentFromObj( &curr, 1 );
}


void init_weedpatch_object(
    DRIVE_OBJECT *obj)
{
    WEEDPATCH_LIST *tl;

    if (Height <= 0.0)  Height  = DEFAULT_WEEDPATCH_HEIGHT;
    if (Length <= 0.0)  Length  = DEFAULT_WEEDPATCH_LENGTH;
    if (Width <= 0.0)  	Width   = DEFAULT_WEEDPATCH_WIDTH;
    if (Count <= 0)	Count	= DEFAULT_WEEDPATCH_COUNT;

    if ( debug )
	printf(" inside init_weedpatch_%d_object() routine \n", (int) Height);

    /* See if we've created a weedpatch this tall before... */
    tl = weedpatch_list;
    while (tl != NULL) {
	if (IS_NEAR(tl->height,Height)
	        && IS_NEAR(tl->width,Width)
	        && IS_NEAR(tl->length,Length)
	        && (tl->nameset_bits == obj->nameset_bits)
	        && (tl->count == Count))
	    break;
	/* else */
	tl = tl->next;
    }

    if (tl != NULL) {
	/* good -- already got one. */
	obj->display_list = tl->dl_number;
    }
    else {
	/* Nope -- gotta create new one */
	if ((tl = (WEEDPATCH_LIST *) malloc(sizeof(WEEDPATCH_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}

	tl->height = Height;
	tl->width = Width;
	tl->length = Length;
	tl->count = Count;
	tl->nameset_bits = obj->nameset_bits;

	create_weedpatch_graphics( obj );

	tl->dl_number = obj->display_list;
	tl->next = weedpatch_list;
	weedpatch_list = tl;
    }

    /* Don't want to do any collision detection here. */
    obj->surface_chars_xyz  = NULL;
    obj->surface_chars_bbox = NULL;
}



/*****************************************************************
 * grass_graphics -
 */
#define BLADES	3

static void create_grass_graphics(
    DRIVE_OBJECT *obj)
{
    int grass_seg;
    float grass_clist[BLADES * 8], *grass_ptr;
    float x, y, z;
    int i, blade;
    hwObject curr, oList[1000];
    int nObjs = 0;

    /* Create grass segment. */
    grass_seg = get_dl_segment();
    
    /* Create grass. */
    for ( i = 0; i < Count; i++ )
    {
	/* Circular patch. */
	if ( Radius > 0.0 )
	{
	    x = BOUNDED_FLOATRAND(-Radius,Radius);
	    y = 0.0;
	    z = sqrt( Radius * Radius - x * x );
	    z = BOUNDED_FLOATRAND(-z,z);
	}
	/* Rectangular patch. */
	else
	{
	    x = FLOATRAND(Width);
	    y = 0.0;
	    z = FLOATRAND(Length);
	}

	grass_ptr = grass_clist;
	for ( blade=0; blade<BLADES; blade++ )
	{
	    *grass_ptr++ = x;
	    *grass_ptr++ = y;
	    *grass_ptr++ = z;
	    *grass_ptr++ = 0.0; /* Move. */

	    *grass_ptr++ = x + floatrand() - 0.5;
	    *grass_ptr++ = y + FLOATRAND(Height);
	    *grass_ptr++ = z + floatrand() - 0.5;
	    *grass_ptr++ = 1.0; /* Draw. */
	}

	curr = hwPolyline->create( hwPolyline );
	HW_MODIFY_3F( curr, hwStrColor, 0.2, 0.6, 0.1 );
	HW_MODIFY_1B( curr, hwStrHasFlags, HW_TRUE );
	curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,BLADES*2), grass_clist );
	oList[nObjs++] = curr;
    }

    curr = hwGroup->create( hwGroup );
    HW_OBJECT_NAMESET(curr,obj);
    curr->modify( curr, hwStrChildren,
		HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );

    obj->display_list = createHwSegmentFromObj( &curr, 1 );
}


void init_grass_object(
    DRIVE_OBJECT *obj)
{
    GRASS_LIST *tl;

    if (Height <= 0.0)  Height  = DEFAULT_GRASS_HEIGHT;
    if (Length <= 0.0)  Length  = DEFAULT_GRASS_LENGTH;
    if (Width <= 0.0)  	Width   = DEFAULT_GRASS_WIDTH;
    if (Radius <= 0.0) 	Radius  = DEFAULT_GRASS_RADIUS;
    if (Count <= 0)	Count	= DEFAULT_GRASS_COUNT;

    if ( debug )
	printf(" inside init_grass_%d_object() routine \n", (int) Height );

    /* See if we've created a grass this tall before... */
    tl = grass_list;
    while (tl != NULL) {
	if (IS_NEAR(tl->height,Height)
	        && IS_NEAR(tl->width,Width)
	        && IS_NEAR(tl->length,Length)
	        && IS_NEAR(tl->radius,Radius)
	        && (tl->nameset_bits == obj->nameset_bits)
	        && (tl->count == Count))
	    break;
	/* else */
	tl = tl->next;
    }

    if (tl != NULL) {
	/* good -- already got one. */
	obj->display_list = tl->dl_number;
    }
    else {
	/* Nope -- gotta create new one */
	if ((tl = (GRASS_LIST *) malloc(sizeof(GRASS_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}

	tl->height = Height;
	tl->width = Width;
	tl->radius = Radius;
	tl->length = Length;
	tl->count = Count;
	tl->nameset_bits = obj->nameset_bits;

	create_grass_graphics( obj );

	tl->dl_number = obj->display_list;
	tl->next = grass_list;
	grass_list = tl;
    }

    /* Don't want to do any collision detection here. */
    obj->surface_chars_xyz  = NULL;
    obj->surface_chars_bbox = NULL;
}

/*****************************************************************
 *****************************************************************
 * Bushes.
 *****************************************************************
 *****************************************************************
 */

#define DEFAULT_BUSH_HEIGHT	3.0

typedef struct _BUSH_list {
    float height;
    unsigned int nameset_bits;
    int dl_number;
    struct _BUSH_list *next;
} BUSH_LIST;

static BUSH_LIST *bush_list = NULL;

/*****************************************************************
 * create_bush_graphics -
 */
static void create_bush_graphics(
    DRIVE_OBJECT *obj)
{
    segment *word;
    int length;
    float *clist;
    hwObject curr;
    
    /* Create bush. */
    word = graftal_word((segment *) "[0][0][0][0][0][0][0]1[0]0", mytree_rules, 3 );
    length = graftal_length( word, NULL, NULL );
    
    /* Malloc space for polyline. */
    clist = (float *)malloc( length * 4 * sizeof(float) );
    graftal_polyline( word, clist, NULL, 0.0, -Height/5.0, 0.0, Height/5.0 );
    
    /* Draw bush. */
    curr = hwPolyline->create( hwPolyline );
    HW_MODIFY_1B( curr, hwStrHasFlags, HW_TRUE );
    HW_MODIFY_3F( curr, hwStrColor, 0.5, 0.4, 0.1 );
    HW_OBJECT_NAMESET(curr,obj);
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,length*4), clist );

    free( clist );
    obj->display_list = createHwSegmentFromObj( &curr, 1 );
}


/*****************************************************************
 * init_bush_object -
 */
void init_bush_object(
    DRIVE_OBJECT *obj)
{
    BUSH_LIST *bl;

    if (Height <= 0.0)
	Height  = DEFAULT_BUSH_HEIGHT;

    if ( debug )
	printf("Inside init_bush_%d_object() routine.\n", (int) Height );

    /* See if we've created a bush this tall before. */
    bl = bush_list;
    while (bl != NULL)
    {
	if (IS_NEAR(bl->height,Height)
		&& (bl->nameset_bits == obj->nameset_bits))
	    break;
	/* else */
	bl = bl->next;
    }

    if (bl != NULL)
    {
	/* Good -- already got one. */
	obj->display_list = bl->dl_number;
    }
    else
    {
	/* Nope -- gotta create new one */
	if ((bl = (BUSH_LIST *) malloc(sizeof(BUSH_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}

	bl->height = Height;
	bl->nameset_bits = obj->nameset_bits;

	create_bush_graphics( obj );

	bl->dl_number = obj->display_list;
	bl->next = bush_list;
	bush_list = bl;
    }

    /* Don't want to do any collision detection here. */
    obj->surface_chars_xyz  = NULL;
    obj->surface_chars_bbox = NULL;

    elevate_object_to_terrain_height((SCENE *) obj->scene,obj,FALSE);
}
