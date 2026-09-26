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


#ifndef _SCENE_H_INCLUDED
#define _SCENE_H_INCLUDED

#include "libnum.h"
#include "physics.h"
#include "controls.h"
#include "global.h"
#include "object.h"
#include "sys/types.h"

/* Constants used in several scene-generating modules. */
#define SCENE_ROAD_WIDTH	30.0

/* The following constants are used so display lists can be reused in
 * different scenes.
 */
#define SCENE_GRID_SIZE			10.0
#define SCENE_TREE_HEIGHT_INCREMENT	8.0
#define SCENE_BUSH_HEIGHT_INCREMENT	1.0
#define SCENE_SKYSCRAPER_INCREMENT	12.0
#define ROUND_TO_INCREMENT2(min,max,inc) \
    ((float) ((inc) * ((((min) < (inc)) ? 1 : (int) ((min)/(inc))) \
	+ ZINTRAND(((max)-(min))/(inc) + 1))))
#define ROUND_TO_INCREMENT(val,inc) \
    ((float) ((inc) * ((int) ((val)/(inc)))))
#define RANDOM_TREE_HEIGHT(minheight,maxheight) \
    ROUND_TO_INCREMENT2(minheight,maxheight,SCENE_TREE_HEIGHT_INCREMENT)
#define RANDOM_BUSH_HEIGHT(minheight,maxheight) \
    ROUND_TO_INCREMENT2(minheight,maxheight,SCENE_BUSH_HEIGHT_INCREMENT)
#define ROUND_SKYSCRAPER_SIZE(size) \
    ROUND_TO_INCREMENT(size,SCENE_SKYSCRAPER_INCREMENT)
#define RANDOM_SKYSCRAPER_HEIGHT(minheight,maxheight) \
    ROUND_TO_INCREMENT2(minheight,maxheight,SCENE_SKYSCRAPER_INCREMENT)
#define ROUND_TO_GRID(loc) \
    ROUND_TO_INCREMENT(loc,SCENE_GRID_SIZE)


/* A linked list of scenes, which act as XxZ tiles of the world, is generated
 * by the scene parser and random scene generator.
 * The X axis runs right, the Z axis up.  The connectivity of the
 * scenes is maintained by the left,right,up,down pointers.  If one of
 * these pointers is NULL, it means the relevant scene hasn't been generated
 * yet.  The random seed used to generate the scene is stored in randseed.
 * If scene_type is equal to NONRANDOM_SCENE, the scene was read in from a file
 * instead of being randomly generated.  Each scene is a square on a 
 * world tesselated in X and Z:  xscene and yscene give the coordinates of
 * the tile relative to an arbitrary origin.
 */

typedef struct _scene {
    int xscene,zscene;				/* scene coordinates */
    int type;					/* see selections below */
    int flags;					/* bits below */
    int static_seg;				/* dl segment for static objs*/
    int count;					/* # of updates since this 
						   scene has been sent */
    long random_seed;				/* for random-generated scenes*/
    DRIVE_OBJECT *object_head;			/* ptr to 1st object in scene */
    struct _scene *left,*right,*up,*down;	/* connected scenes */
    struct _scene *previous,*next;		/* for scene list */
    boolean_type defined;			/* is scene defined in a file */
    boolean_type read_in;			/* has it been read in yet? */
    char *scenefilename;			/* file scene is defined in */
    time_t file_mtime;				/* last time scene file modif */
    char *comments;				/* comments before Scene Loc  */
} SCENE;
extern SCENE *scene_head;			/* ptr to first scene in list */

#define SCENE_LAST_SEEN 20	/* How many updates before ignoring scene */

#define SCENE_ROAD_RIGHT	0x00000001	/* road out on right side */
#define SCENE_ROAD_LEFT		0x00000002	/* road out on left side */
#define SCENE_ROAD_UP		0x00000004	/* road out on top side */
#define SCENE_ROAD_DOWN		0x00000008	/* road out on bottom side */
#define SCENE_ROAD_ALL \
    (SCENE_ROAD_RIGHT|SCENE_ROAD_LEFT|SCENE_ROAD_UP|SCENE_ROAD_DOWN)
#define SCENE_ALL_FLAT		0x00000010	/* all based at y == 0 */
#define SCENE_RIGHT_FLAT	0x00000020	/* right edge flat */
#define SCENE_LEFT_FLAT		0x00000040	/* left edge flat */
#define SCENE_UP_FLAT		0x00000080	/* top edge flat */
#define SCENE_DOWN_FLAT		0x00000100	/* bottom edge flat */
#define DEFAULT_SCENE_FLAGS	(SCENE_ALL_FLAT)
#define SCENE_FLAT_FLAGS \
 (SCENE_ALL_FLAT|SCENE_RIGHT_FLAT|SCENE_LEFT_FLAT|SCENE_UP_FLAT|SCENE_DOWN_FLAT)
#define SCENE_FLAT_EDGE_FLAGS \
    (SCENE_RIGHT_FLAT|SCENE_LEFT_FLAT|SCENE_UP_FLAT|SCENE_DOWN_FLAT)

typedef struct {
    int type;
    char name[256];	/* all lower case, no spaces */
    int flags;
    int weight;		/* Used when picking random one.  Higher = more often */
} SCENE_TYPE;
extern SCENE_TYPE scene_type[];

/* scene types */
#define SCENE_NONRANDOM			0
#define SCENE_WHATEVER_FITS		1
#define SCENE_FLAT_NO_ROADS		2
#define SCENE_FLAT_XROAD		3
#define SCENE_FLAT_ZROAD		4
#define SCENE_FLAT_LEFT_T		5
#define SCENE_FLAT_RIGHT_T		6
#define SCENE_FLAT_DOWN_T		7
#define SCENE_FLAT_UP_T			8
#define SCENE_FLAT_CROSSROADS		9
#define SCENE_FLAT_OVERPASS		10
#define SCENE_FLAT_TURN_RIGHT_DOWN	11
#define SCENE_FLAT_TURN_RIGHT_UP	12
#define SCENE_FLAT_TURN_LEFT_DOWN	13
#define SCENE_FLAT_TURN_LEFT_UP		14	
#define SCENE_FLAT_XHIGHWAY		15
#define SCENE_FLAT_ZHIGHWAY		16
#define SCENE_FLAT_XHIGHWAY_OVERPASS	17
#define SCENE_FLAT_ZHIGHWAY_OVERPASS	18
#define SCENE_FLAT_RANDOM_TOWN		19
#define SCENE_FLAT_VILLAGE		20
#define SCENE_FLAT_TOWN			21
#define SCENE_FLAT_CITY			22
#define SCENE_HILL_NO_ROADS		23
#define SCENE_HILL_XROAD		24
#define SCENE_HILL_ZROAD		25
#define SCENE_HILL_CROSSROADS		26
#define NUM_SCENES			(SCENE_HILL_CROSSROADS+1)


#define SCENE_LEFT			(1 << 0)
#define SCENE_UP			(1 << 1)
#define SCENE_UP_LEFT			(1 << 2)
#define SCENE_UP_RIGHT			(1 << 3)
#define SCENE_RIGHT			(1 << 4)
#define SCENE_DOWN			(1 << 5)
#define SCENE_DOWN_LEFT			(1 << 6)
#define SCENE_DOWN_RIGHT		(1 << 7)
#define SCENE_CENTER			(1 << 8)
#define SCENE_ALL			0x1ff



typedef struct _player {
    int player_num;
    SCENE *current_scene;			/* which scene am I in? */
    DRIVE_OBJECT *my_vehicle;			/* which vehicle? */
    int watcher;				/* am I a watcher? */
    struct _player *next;			/* for player list */
} PLAYER;
extern PLAYER *player_head;


typedef struct {
    char  subtype[NAME_STRLEN];
    char  label[LABEL_STRLEN];
    float xrot,yrot,zrot;
    float size[3];
    float angle;
    float radius;
    float spacing;
    float mat[4][4];
    float color[3];
    int count;
    WORMHOLE_DATA wormhole_data;
    int dimension;
    float *data;
    unsigned int nameset_bits;
    unsigned int object_bits;
} OBJECT_SPECS;


/*************************** FUNCTION PROTOTYPES ******************************/
/*** From scene.c ***/
extern SCENE *find_scene(
    int x, int z);
extern void delete_object_from_scene(
    SCENE *scene,
    DRIVE_OBJECT *object);
extern int add_scene_to_list(
    SCENE *scene);
extern SCENE *check_scenes(
    DRIVE_OBJECT *obj,
    float xvehicle, float zvehicle);
extern void scene_update(
    float interval);
extern void create_scene_mongo_dl(
    SCENE *scene,
    int fildes);
extern void object_initialization(
    void);
extern void check_all_defined_scenefiles(
    void);
extern time_t scenefile_mtime(
    char *scenefilename);


/*** From random_scene.c ***/
extern SCENE *generate_random_scene(
    SCENE *newscene,
    int xscene,
    int zscene);
extern DRIVE_OBJECT *add_object_to_scene(
    SCENE *scene,
    char *obj_name, char *subtype, char *label,
    float yrot,
    float x, float y, float z,	/* relative to scene origin */
    float length, float width, float height,
    float radius, float angle, float spacing,
    int count);
extern void update_segment_lists(
    int last_segment_sent);
extern void add_random_tree(
    SCENE *scene,
    float x, float z);


/*** From town.c ***/
#define CITY_TYPE_VILLAGE               1
#define CITY_TYPE_TOWN                  2
#define CITY_TYPE_CITY                  3
extern void generate_town(
    SCENE *scene,
    int city_type);


/*** From parse_scene.c ***/
extern void get_construct(
    SCENE *scene,
    char *construct_name,
    OBJECT_SPECS *specs,
    float mat[4][4]);
extern void reset_object_specs(
    OBJECT_SPECS *specs);


/* #defines for player starting positions */
#define START_POSITION_X 	  88.0
#define START_POSITION_Z	-110.0
#define START_POSITION_DELTA	  15.0

#endif /*  _SCENE_H_INCLUDED */
