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


/* Common macros for object definitions */


/****
 ****  !!!!!!!!!! IMPORTANT OBJECT NOTES !!!!!!!!!!
 ****
 *
 *  The default surface model has specular OFF.  If your object needs
 *  specular on, it must call surface_model explicitly and use the
 *  macro RESTORE_DEFAULT_SURFACE_MODEL(img_fildes) when done.
 *
 *  The default vertex format is "0, 0, 0, 0, COUNTER_CLOCKWISE".  If your
 *  object needs to change it, it must do so explicitly and call
 *  RESTORE_DEFAULT_VERTEX_FORMAT(img_fildes) when done.
 *
 *
 ****
 ****
 ****/

#ifndef _OBJ_COMMON_INCLUDED
#define _OBJ_COMMON_INCLUDED 1

#include "prims.h"
#include "demo_physics.h"
#include "object.h"
#include "scene.h"
#include "drive_server.h"

extern int img_fildes, do_textures;


/* Position of shadow in ID list */
#define SHADOW_ID_NUMBER	0

/* Each object type has a unique number */
#define GROUND_OBJECT 			1
#define ROAD_OBJECT			2
#define CURVE_OBJECT			3
#define RAMP_OBJECT			4
#define BANK_OBJECT			5
#define RAILROAD_OBJECT			6
#define HILLROAD_OBJECT			7
#define BRIDGE_OBJECT			8
#define SKYSCRAPER_OBJECT		9
#define BARN_OBJECT			10
#define SILO_OBJECT			11
#define LAMP_OBJECT			12
#define PILLAR_OBJECT			13
#define TELELINE_OBJECT			14
#define CONIFER_OBJECT			15
#define HILLXROAD_OBJECT		16
#define BUMP_OBJECT			17
#define STOP_SIGN_OBJECT 		18
#define SPEED_LIMIT_SIGN_OBJECT 	19
#define LEFT_T_SIGN_OBJECT 		20
#define RIGHT_T_SIGN_OBJECT 		21
#define TOP_T_SIGN_OBJECT 		22
#define LEFT_CURVE_SIGN_OBJECT 		23
#define RIGHT_CURVE_SIGN_OBJECT 	24
#define GENERIC_SIGN_OBJECT 		25
#define HILL_OBJECT			26
#define STOPLIGHT_OBJECT		27
#define TWOPOLE_SIGN_OBJECT 		28
#define MOUND_OBJECT			29
#define POWERSHIFT_SIGN_OBJECT		30
#define WALL_OBJECT			31
#define CURVEWALL_OBJECT		32
#define POLYLINE_OBJECT			33
#define CURVEGUARDRAIL_OBJECT		34
#define FLAT_OBJECT			35
#define LAWN_OBJECT			36
#define SIDEWALK_OBJECT			37
#define SHADOW_OBJECT			38
#define ICE_OBJECT			39
#define HOUSE_OBJECT			40
#define PARKING_LOT_OBJECT		41
#define TREE_OBJECT			42
#define MAILBOX_OBJECT			43
#define LOWER_TUBE_OBJECT		44
#define FOREST_OBJECT			45
#define WEEDPATCH_OBJECT		46
#define GRASS_OBJECT			47
#define POND_OBJECT			48
#define BUSH_OBJECT			49
#define LIGHT_OBJECT			50
#define	SPIRAL_OBJECT			51
#define COUNTACH_OBJECT			52
#define MINIVAN_OBJECT			53
#define T34_OBJECT			54
#define SEDAN_OBJECT			55
#define FOKD7_OBJECT			56
#define UFO_OBJECT			57
#define POLICE_OBJECT			58
#define TWISTRAMP_OBJECT		59
#define MCYCLE_OBJECT			60
#define CONE_OBJECT			61
#define SPHERE_OBJECT			62
#define DECIDUOUS_OBJECT		63
#define OVAL_OBJECT			64
#define SPLROAD_OBJECT			65
#define XFIGHTER_OBJECT			66
#define FLAMES_OBJECT			67
#define RUBBLE_OBJECT			68
#define FENCE_OBJECT			69
#define SHELL_OBJECT			70
#define LAND_MINE_OBJECT		71
#define ROCKET_CAR_OBJECT		72

#define START_OBJECT			100
#define FINISH_OBJECT			101
#define CHECKPOINT_OBJECT		102

#define WORMHOLE_OBJECT			200
#define BKWORMHOLE_OBJECT		201
#define INVISWORMHOLE_OBJECT		202


#define	 deg		*M_PI/180	/* convert radians to degrees */

#define SIN0	(0.0)
#define COS0	(1.0)
#define SIN5	(0.08155743)
#define COS5	(0.99619470)
#define SIN10	(0.17364818)
#define COS10	(0.98480775)
#define SIN15	(0.25881905)
#define COS15	(0.96592583)
#define SIN20	(0.34202014)
#define COS20	(0.93969262)
#define SIN25	(0.42261826)
#define COS25	(0.90630779)
#define SIN30	(0.50000000)
#define COS30	(0.86602540)
#define SIN35	(0.57357644)
#define COS35	(0.81915204)
#define SIN40	(0.64278761)
#define COS40	(0.76604444)
#define SIN45	(0.70710678)
#define COS45	SIN45
#define SIN50	COS40
#define COS50	SIN40
#define SIN55	COS35
#define COS55	SIN35
#define SIN60	COS30
#define COS60	SIN30
#define SIN65	COS25
#define COS65	SIN25
#define SIN70	COS20
#define COS70	SIN20
#define SIN75	COS15
#define COS75	SIN15
#define SIN80	COS10
#define COS80	SIN10
#define SIN85	COS5
#define COS85	SIN5
#define SIN90	COS0
#define COS90	SIN0

#define FAR_WHEEL_ANGLE		(30 deg)
#define NEAR_WHEEL_ANGLE	(15 deg)

extern double drand48();

#define NUMPTS(array) (sizeof(array)/sizeof(float)/3)
#define NUMPTSNORMAL(array) (sizeof(array)/sizeof(float)/6)

/* After a routine changes the surface model, it should call this. */
#define RESTORE_DEFAULT_SURFACE_MODEL(fildes)		\
	surface_model((fildes),FALSE,12,0.5,0.5,0.5);

#define RESTORE_DEFAULT_VERTEX_FORMAT(fildes)		\
	vertex_format((fildes), 0, 0, 0, 0, COUNTER_CLOCKWISE);

# define AMBIENT_WITH_DIFFUSE	0.5,0.5,0.5
# define AMBIENT_ALONE		1.0,1.0,1.0

#ifdef USE_AMBIENT_ONLY_WHEN_POSSIBLE
# define DIFFUSE_LIGHTING_ON(fildes)			\
    light_ambient((fildes),AMBIENT_WITH_DIFFUSE);	\
    light_switch((fildes),0x3);

# define DIFFUSE_LIGHTING_OFF(fildes)			\
    light_ambient((fildes),AMBIENT_ALONE);		\
    light_switch((fildes),0x01);
#else /* don't change lighting */
# define DIFFUSE_LIGHTING_ON(fildes)
# define DIFFUSE_LIGHTING_OFF(fildes)
#endif

#define SELF_LIT_ON(fildes)				\
    self_lit_on(fildes)

#define SELF_LIT_OFF(fildes)				\
    self_lit_off(fildes)

#define TRANSPARENT_ON(fildes,level) 

#define TRANSPARENT_OFF(fildes) 


#define	C_HW(o,r,g,b) { \
    float __tmp[3]; \
    __tmp[0] = (r); __tmp[1] = (g); __tmp[2] = (b); \
    (o)->modify( (o), hwStrColor, HW_TYPE_3F, __tmp ); \
}

/**** Surface types ****/
#define PAINT(fildes,r,g,b) \
	fill_color((fildes),(r),(g),(b)); 

#define ALUMINUM(fildes) \
	fill_color((fildes),0.5,0.5,0.5); 
#define	ALUMINUM_HW(o)	C_HW(o, 0.5, 0.5, 0.5)

#define STEEL_INTENSITY	0.3
#define STEEL_RED	STEEL_INTENSITY
#define STEEL_GREEN	STEEL_INTENSITY
#define STEEL_BLUE	STEEL_INTENSITY
#define STEEL(fildes) \
	fill_color((fildes),STEEL_RED,STEEL_GREEN,STEEL_BLUE);
#define STEEL_HW(o)	C_HW(o, STEEL_RED, STEEL_GREEN, STEEL_BLUE)

#define RUSTY_STEEL(fildes) \
	fill_color((fildes),0.3,0.2,0.2); 
#define RUSTY_STEEL_HW(o)	C_HW(o, 0.3, 0.2, 0.2)

#define BLACK(fildes) \
	fill_color((fildes),0.0,0.0,0.0); 
#define	BLACK_HW(o)	C_HW(o, 0.0, 0.0, 0.0)

#define WOOD_RED	0.5
#define WOOD_GREEN	0.4
#define WOOD_BLUE	0.1
#define WOOD(fildes) \
	fill_color((fildes),WOOD_RED,WOOD_GREEN,WOOD_BLUE); 
#define	WOOD_HW(o)		C_HW(o, WOOD_RED, WOOD_GREEN, WOOD_BLUE)

#define GREY_GLASS(fildes) \
	fill_color((fildes),0.1,0.1,0.1); 
#define GRAY_GLASS(fildes) GREY_GLASS(fildes)
#define	GREY_GLASS_HW(o)		C_HW(o, 0.1, 0.1, 0.1)
#define	GRAY_GLASS_HW(o)		GREY_GLASS_HW(o)

#define CHROME(fildes) \
	fill_color((fildes),0.4,0.4,0.4); 
#define	CHROME_HW(o)		C_HW(o, 0.4, 0.4, 0.4)

#define RUBBER(fildes) \
	fill_color((fildes),0.03,0.03,0.03); 
#define	RUBBER_HW(o)		C_HW(o, 0.03, 0.03, 0.03)

#define WHITE_PLASTIC(fildes) \
	fill_color((fildes),0.60,0.60,0.60); 
#define	WHITE_PLASTIC_HW(o)	C_HW(o, 0.60, 0.60, 0.60)

#define YELLOW_PLASTIC(fildes) \
	fill_color((fildes),0.7,0.6,0.0); 
#define	YELLOW_PLASTIC_HW(o)	C_HW(o, 0.70, 0.60, 0.00)

#define RED_PLASTIC(fildes) \
	fill_color((fildes),0.5,0.0,0.0); 
#define	RED_PLASTIC_HW(o)	C_HW(o, 0.50, 0.00, 0.00)

#define BLACK_PLASTIC(fildes) \
	fill_color((fildes),0.03,0.03,0.03); 
#define	BLACK_PLASTIC_HW(o)	C_HW(o, 0.03, 0.03, 0.03)

#define ASPHALT_INTENSITY 0.15
#define	ASPHALT_TM_INTENS 0.30
#define ASPHALT(fildes) \
	fill_color((fildes), \
	    ASPHALT_INTENSITY,ASPHALT_INTENSITY,ASPHALT_INTENSITY); 
#define	ASPHALT_HW(o) {\
    if( do_textures ) { \
	C_HW(o, ASPHALT_TM_INTENS, ASPHALT_TM_INTENS, ASPHALT_TM_INTENS); \
	o->modify( o, hwStrTexture, \
			HW_TYPE_OBJECT, hwFindObject( "RoadTexture" ) ); \
    } \
    else { \
	C_HW(o, ASPHALT_INTENSITY, ASPHALT_INTENSITY, ASPHALT_INTENSITY); \
    } \
}

#define	ROAD_COLOR_HW(o,r,g,b) { \
    C_HW(o, r, g, b); \
    if( do_textures ) { \
	o->modify( o, hwStrTexture, \
			HW_TYPE_OBJECT, hwFindObject( "RoadTexture" ) ); \
    } \
}

#define	ASPHALT_TM_HW(o) C_HW(o, 0.3, 0.3, 0.3 )

#define CONCRETE_INTENSITY 0.6
#define CONCRETE(fildes) \
	fill_color((fildes), \
	    CONCRETE_INTENSITY,CONCRETE_INTENSITY,CONCRETE_INTENSITY); 
#define	CONCRETE_HW(o) \
	if( do_textures ) { \
	    C_HW(o,1,1,1); \
	    o->modify(o,hwStrTexture, \
			HW_TYPE_OBJECT, hwFindObject( "ConcreteTexture" ) ); \
	} \
	else { \
	    C_HW(o,CONCRETE_INTENSITY,CONCRETE_INTENSITY,CONCRETE_INTENSITY); \
	}

#define DARK_CONCRETE(fildes) \
	fill_color((fildes), \
	    CONCRETE_INTENSITY-0.2,CONCRETE_INTENSITY-0.2,CONCRETE_INTENSITY-0.2); 
#define	DARK_CONCRETE_HW(o) \
	if( do_textures ) { \
	    C_HW(o,.8,.8,.8); \
	    o->modify(o,hwStrTexture, \
			HW_TYPE_OBJECT, hwFindObject( "ConcreteTexture" ) ); \
	} \
	else { \
	    C_HW(o,CONCRETE_INTENSITY-.2,CONCRETE_INTENSITY-.2,CONCRETE_INTENSITY-.2); \
	}

#define BRICK(fildes) \
	fill_color((fildes),0.4,0.1,0.1);
#define	BRICK_HW(o)		C_HW(o, 0.4, 0.1, 0.1)

#define ROAD_LINE_YELLOW(fildes) \
	if (server_mode & SERVER_MODE_NIGHT_DRIVING) \
	    fill_color((fildes),0.8,0.8,0.1); \
	else			\
	    fill_color((fildes),0.6,0.6,0.1); 
#define	ROAD_LINE_YELLOW_HW(o) \
	if (server_mode & SERVER_MODE_NIGHT_DRIVING) { \
	    C_HW(o,0.8,0.8,0.1); \
	} \
	else if( do_textures ) { 			\
	    C_HW(o,0.8,0.8,0.1); \
	    o->modify( o, hwStrTexture, \
			HW_TYPE_OBJECT, hwFindObject( "RoadTexture" ) ); \
	} \
	else { \
	    C_HW(o,0.6,0.6,0.1); \
	}

#define GRASS(fildes) \
	fill_color((fildes),0.2,0.5,0.1); 
#define	GRASS_HW(o) \
    if( do_textures ) { \
	C_HW(o, 0.4, 1.0, 0.2); \
	o->modify( o, hwStrTexture, \
			HW_TYPE_OBJECT, hwFindObject( "GrassTexture" ) ); \
    } \
    else { \
	C_HW(o, 0.2, 0.5, 0.1); \
    }

#define PORCELAIN(fildes) \
	fill_color((fildes),0.8,0.8,0.8); 
#define	PORCELAIN_HW(o)	C_HW(o, 0.8, 0.8, 0.8)

#define STUCCO(fildes) \
	fill_color((fildes),0.7,0.7,0.7); 
#define	STUCCO_HW(o)	C_HW(o, 0.7, 0.7, 0.7)

#define SHADOW_INTENSITY	0.05
#define EMPTY_SPACE(fildes) \
	fill_color((fildes),SHADOW_INTENSITY,SHADOW_INTENSITY,SHADOW_INTENSITY);
#define	EMPTY_SPACE_HW(o)	C_HW(o,SHADOW_INTENSITY, SHADOW_INTENSITY, SHADOW_INTENSITY)

#define SHADOW(fildes) \
	EMPTY_SPACE(fildes)
#define	SHADOW_HW(o)	EMPTY_SPACE_HW(o)

#define LANE_WIDTH		(15.0)
#define DEFAULT_ROAD_WIDTH	(30.0)
#define ROAD_FLOAT		(0.1)
#define ROAD_LINE_WIDTH		(0.4)
#define ROAD_LINE_LENGTH	(10.0)
#define ROAD_LINE_SPACING	(25.0)
#define ROAD_LINE_FLOAT		(ROAD_FLOAT+0.1)

/* This defines how much curved objects are broken up graphically. */
#define MAX_CURVE_CHORD		(50.0)
#define MAX_CURVE_SUBANGLE	(22.5*M_PI/180.0)

/* Ground segments use the same random seed to match colors at edges. */
#define GROUND_RANDSEED		(1021825)
#define GROUND_EXTENT		(SCENE_SIZE/2.0)
#define Y_GROUND		(-0.1)
#define GROUND_R_MIN		0.3
#define GROUND_R_MAX		0.7
#define GROUND_R_AVE		((GROUND_R_MIN+GROUND_R_MAX)/2.0)
#define GROUND_G_MIN		0.4
#define GROUND_G_MAX		0.8
#define GROUND_G_AVE		((GROUND_G_MIN+GROUND_G_MAX)/2.0)
#define GROUND_B_MIN		0.1
#define GROUND_B_MAX		0.5
#define GROUND_B_AVE		((GROUND_B_MIN+GROUND_B_MAX)/2.0)
#define GROUND_MAX_LUMINOSITY	50
#define GROUND_MIN_LUMINOSITY	20

#define BBOX_MARGIN		(3.0)
    /* This is the distance into a complex object's top something can
     * penetrate and still be considered on top of the object.
     */


/*** MACROS TO HANDLE NAMESETS ***/
/* Objects which use namesets explicitly should *not* use these macros.
 * Since the nameset flag is "visible if any flag is true", explicit
 * namesets will interfere with the timed namesets here.
 */

#define HANDLE_OBJECT_NAMESET(fildes,obj) \
{   unsigned int _names; \
    if ((_names = (obj)->nameset_bits) != ALL_TIMED_NAMESETS) { \
	add_names_to_set((fildes),1,(int *) &_names); \
    } \
}

#define	HW_OBJECT_NAMESET(hwObj,obj) \
{   unsigned int _names; \
    if ((_names = (obj)->nameset_bits) != ALL_TIMED_NAMESETS) { \
	HW_MODIFY_1I(hwObj,hwStrVisibility,_names); \
    } \
}

#define DEFAULT_OBJECT_NAMESET(fildes,obj) \
{ \
    if ((obj)->nameset_bits != ALL_TIMED_NAMESETS) \
	remove_all_names_from_set(fildes); \
}


/* Negate X coordinate and reverse order to preserve clockwise/ccw */
#define NEGATE_X(object,count)				\
{   int _i,_j;						\
    float _x,_y,_z;					\
    for (_i=0,_j=(count)-1; _i<=_j; ++_i,--_j) {	\
	_x = object[_i][0];				\
	_y = object[_i][1];				\
	_z = object[_i][2];				\
	object[_i][0] = -object[_j][0];			\
	object[_i][1] = object[_j][1];			\
	object[_i][2] = object[_j][2];			\
	object[_j][0] = -_x;				\
	object[_j][1] = _y;				\
	object[_j][2] = _z;				\
    }							\
}

/* Negate Y coordinate and reverse order to preserve clockwise/ccw */
#define NEGATE_Y(object,count)				\
{   int _i,_j;						\
    float _x,_y,_z;					\
    for (_i=0,_j=(count)-1; _i<=_j; ++_i,--_j) {	\
	_x = object[_i][0];				\
	_y = object[_i][1];				\
	_z = object[_i][2];				\
	object[_i][0] = object[_j][0];			\
	object[_i][1] = -object[_j][1];			\
	object[_i][2] = object[_j][2];			\
	object[_j][0] = _x;				\
	object[_j][1] = -_y;				\
	object[_j][2] = _z;				\
    }							\
}

/* Negate Z coordinate and reverse order to preserve clockwise/ccw */
#define NEGATE_Z(object,count)				\
{   int _i,_j;						\
    float _x,_y,_z;					\
    for (_i=0,_j=(count)-1; _i<=_j; ++_i,--_j) {	\
	_x = object[_i][0];				\
	_y = object[_i][1];				\
	_z = object[_i][2];				\
	object[_i][0] = object[_j][0];			\
	object[_i][1] = object[_j][1];			\
	object[_i][2] = -object[_j][2];			\
	object[_j][0] = _x;				\
	object[_j][1] = _y;				\
	object[_j][2] = -_z;				\
    }							\
}

/* Negate X & Z, no need to swap */
#define NEGATE_XZ(object,count)				\
{   int _i;						\
    for (_i=0; _i<(count); ++_i) {			\
	object[_i][0] = -object[_i][0];			\
	object[_i][2] = -object[_i][2];			\
    }							\
}

#define IDENTITY4x4 \
  {{ 1.0, 0.0, 0.0, 0.0 }, \
   { 0.0, 1.0, 0.0, 0.0 }, \
   { 0.0, 0.0, 1.0, 0.0 }, \
   { 0.0, 0.0, 0.0, 1.0 }}

/***** Routines provided by the obj_utils module. *****/
extern hwObject post(
    float x, float y, float z,
    float h, float r,
    int do_top,
    int num_sides /* 3 or 4 */);
extern int checkbounds_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc,
    int (*xyz_routine)(
	DRIVE_OBJECT *obj1,
	float x, float y, float z,
	MC_SURFACE_CHARACTERISTICS *sc1)
    );
extern void do_mesh_colors(
    float mesh[GROUND_MESH_SIZE][GROUND_MESH_SIZE][6]);
extern void self_lit_on(
    int fildes);
extern void self_lit_off(
    int fildes);
extern float hill_y_wc(  /* returns WC y value */
    DRIVE_OBJECT *hill,
    float xwc, float zwc);
extern float hill_y_mc(  /* returns MC y value */
    DRIVE_OBJECT *obj,
    DRIVE_OBJECT *hill,
    float xmc, float zmc); /* MCs of "obj" */
extern DRIVE_OBJECT *find_hill_in_scene(
    SCENE *scene);
extern void random_ground_rgb(
    float *r, float *g, float *b);
extern void elevate_object_to_terrain_height(
    SCENE *scene,
    DRIVE_OBJECT *obj,
    boolean_type do_xz_bounds);
extern void init_pobj(
    DRIVE_OBJECT *obj,
    float pounds,
    float torquemult);

/***** From sign.c *****/
extern hwObject draw_string(
    char *string,
    float x, float y, float z,
    float width, float height,
    int align, int path,
    float exp_factor);

/*** From SBDL ***/
extern void c_set_cull_size();

#ifdef __linux__
# include "stub_starbase.c.h"
#endif

#endif /* _OBJ_COMMON_INCLUDED */
