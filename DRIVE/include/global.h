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


/* 
 * global.h - Global definitions.
 */

#ifndef _GLOBAL_INCLUDED
#define _GLOBAL_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <sys/types.h>
#include <math.h>
#ifndef WIN32
#include <sys/param.h>
#endif

/**************************** STRUCTURES AND TYPES ****************************/
typedef float time_value;	/* only 0.0 to 23.9999... valid */


/**************************** FUNCTION PROTOTYPES *****************************/
/*** From random.c ***/
extern float floatrand(
    void);
extern unsigned long intrand(
    void);
extern unsigned int fifty_fifty(
    void);
extern void seedrand(
    unsigned int seed);


/*** From fastmalloc.c ***/
extern unsigned char *fastmalloc(
    unsigned int size);
extern void fastfree(
    unsigned char *addr,
    unsigned int size);


/*** From read_scene.c ***/
extern int get_dl_segment(
    void);


/*** From utils.c ***/
extern void normalize_string(
    char *in, char *out);
extern void check_graphics_configuration(
    void);


/*** Unofficial stuff in starbase ***/
extern void _hp_identity(
    float mat[4][4]);
extern void _hp_high_res_sleep(
    double sleeptime);
extern void _hp_invert(
    float mat[4][4],
    float imat[4][4],
    int twod_flag);
	


/************************************* MACROS *********************************/
#ifdef PROFILE
# define STATIC
#else
# define STATIC static
#endif /* PROFILING else */


/* Constants */
#define NAME_STRLEN	32
#define LABEL_STRLEN	256
#define IDLE_RPM	(500.0)		/* Vehicle idle speed */
#define SCENE_SIZE	(2000.0)	/* feet per side of scene square */
#define GROUND_MESH_SIZE	8	/* how much to break it up */
#define HORIZON		(SCENE_SIZE/2.0-150.0)	/* Distance you can see (ft) */
#define BUFSIZE		8192	/* Size for socket buffers. */
#define SEMKEY		0x70420822
#define INVALID		(-1)

/* Default values for object attributes */
#define DEFAULT_OBJECT_SIZE	0.0
#define DEFAULT_OBJECT_COLOR	(-1.0)
#define DEFAULT_OBJECT_RADIUS	0.0
#define DEFAULT_OBJECT_ANGLE	0.0
#define DEFAULT_OBJECT_SUBTYPE0	'\0'
#define DEFAULT_OBJECT_LABEL0	'\0'
#define DEFAULT_OBJECT_SPACING	0.0
#define DEFAULT_OBJECT_COUNT	0

/* Conversions */
#define MPH_TO_FPS(k)		((k)*(5280.0/3600.0))
#define FPS_TO_MPH(k)		((k)*(3600.0/5280.0))
#define POUNDS_TO_SLUGS(k)	((k)/GRAVITY_CONSTANT)
#define DEGREES_TO_RADIANS(k)	((k)*(M_PI/180.0))
#define RADIANS_TO_DEGREES(k)	((k)*(180.0/M_PI))
#define HORSEPOWER_TO_POUNDS(k)	((k)*550.0)

/* Pseudo-functions */
#ifndef MAX
# define MAX(i,j)		((i) > (j) ? (i) : (j))
#endif
#ifndef MIN
# define MIN(i,j)		((i) < (j) ? (i) : (j))
#endif
#define BOUND(lo,x,hi)		MAX((lo), MIN((x),(hi)))
#define APX_EQ(x,y)		(fabs(x-y) < EPS) 
#define REL_EQ(x,y)		(fabs(x-y) < (MAX(fabs(x),fabs(y))*EPS))
#define IABS(x)			(((x) < 0) ? (-(x)) : (x))
#define ABS(x)			IABS(x)
#define FSQRT(k)		((float) sqrt((double)(k)))
#define FSIN(k)			((float) sin((double)(k)))
#define FCOS(k)			((float) cos((double)(k)))
#define FTAN(k)			((float) tan((double)(k)))
#define FPOW(x,y)		((float) pow((double)(x),(double)(y)))
#define FATAN2(y,x) \
    (((ABS(x) < 1e-9) && (ABS(y) < 1e-9)) ? \
	0.0 : \
	((float) atan2((double)(y),(double)(x))))
#define ASIN(x)			((float) asin((double)(x)))
#define ACOS(x)			((float) acos((double)(x)))
#define FLN(x)			((float) log((double)(x)))
#define LN_10			(2.3025851)
#define FLOG10(x)		(FLN(x) * (1.0/LN_10))
#define FEXP(x)			((float) exp((double)(x)))
#define FHYPOT2(x,y)		FSQRT((x)*(x) + (y)*(y))
#define FHYPOT3(x,y,z)		FSQRT((x)*(x) + (y)*(y) + (z)*(z))
#define HYPOT2(x,y)		FHYPOT2(x,y)
#define HYPOT3(x,y,z)		FHYPOT3(x,y,z)
#define FLOAT_NEAR_EPSILON	(1.0e-5)
#define IS_NEAR(a,b)		(ABS((a) - (b)) < FLOAT_NEAR_EPSILON)
#define INTERPOLATE( t, a, b )	((1.0 - (t)) * (a) + (t) * (b))
#define INTSWAP(a,b)		{int _tmp;   _tmp = (a); (a) = (b); (b) = _tmp;}
#define FLOATSWAP(a,b)		{float _tmp; _tmp = (a); (a) = (b); (b) = _tmp;}

#define FLOATRAND(k)		(floatrand() * (k))
#define ZINTRAND(k)		((int) FLOATRAND(k))
#define INTRAND(k)		(1 + ZINTRAND(k))
#define BOUNDED_FLOATRAND(min,max)					\
	((min) + FLOATRAND((max)-(min)))
#define BOUNDED_INTRAND(min,max)					\
	((min) + ZINTRAND((max)-(min)+1))
#define EITHERSIGN_FLOATRAND(min,max)					\
	(fifty_fifty() ?						\
	    BOUNDED_FLOATRAND((min),(max))				\
	    : -BOUNDED_FLOATRAND((min),(max)))				\


#define INDENT( x )			\
	{int _i; for (_i=0; _i < ((x)<<1); _i++) fprintf(stderr," ");}
#define DISTSQ(x1,y1,z1,x2,y2,z2)	\
	(((x1)-(x2))*((x1)-(x2))	\
	    + ((y1)-(y2))*((y1)-(y2))	\
	    + ((z1)-(z2))*((z1)-(z2)))
#define NORMALIZE2(x,y)			\
{   float _len;				\
    _len = FHYPOT2((x),(y));		\
    if (_len != 0.0) {			\
	(x) /= _len;			\
	(y) /= _len;			\
    }					\
}
#define NORMALIZE3(x,y,z)		\
{   float _len;				\
    _len = FHYPOT3((x),(y),(z));	\
    if (_len != 0.0) {			\
	(x) /= _len;			\
	(y) /= _len;			\
	(z) /= _len;			\
    }					\
}

#define POINT_IN_BBOX(bbox,x,y,z)		\
    (   ((x) >= bbox[0]) && ((x) <= bbox[3])	\
     && ((y) >= bbox[1]) && ((y) <= bbox[4])	\
     && ((z) >= bbox[2]) && ((z) <= bbox[5]))

#define POINT_IN_BBOX_XZ(bbox,x,z)		\
    (   ((x) >= bbox[0]) && ((x) <= bbox[3])	\
     && ((z) >= bbox[2]) && ((z) <= bbox[5]))


/****************************** NAMESET BITS **********************************/
/* #defines for display list name sets */
#define LIGHTS_OFF_BRAKES_OFF	(1<<0)
#define LIGHTS_OFF_BRAKES_ON	(1<<1)
#define LIGHTS_ON_BRAKES_OFF	(1<<2)
#define LIGHTS_ON_BRAKES_ON	(1<<3)
#define LIGHTS_MASK		(LIGHTS_OFF_BRAKES_OFF | \
    LIGHTS_OFF_BRAKES_ON | LIGHTS_ON_BRAKES_OFF | LIGHTS_ON_BRAKES_ON)

#define BACKUP_LIGHTS_OFF	(1<<4)
#define BACKUP_LIGHTS_ON	(1<<5)
#define BACKUP_MASK		(BACKUP_LIGHTS_OFF | BACKUP_LIGHTS_ON)

#define WHEELS_FAR_LEFT		(1<<6)
#define WHEELS_LEFT		(1<<7)
#define WHEELS_CENTER		(1<<8)
#define WHEELS_RIGHT		(1<<9)
#define WHEELS_FAR_RIGHT	(1<<10)
#define WHEELS_MASK		(WHEELS_FAR_LEFT | WHEELS_LEFT | WHEELS_CENTER \
    | WHEELS_RIGHT | WHEELS_FAR_RIGHT)


#define CAR_GUN_MASK		(1<<15)

/* The upper 16 bits of the word are automagically cycled one per .25 seconds
 * for a four-second mini-animation loop.
 */
#define TIMED_NAMESET(k)		(1<<(k+16))
#define ALL_TIMED_NAMESETS		(0xffff0000)
#define QUARTER_SECOND_NAMESET_ODD	(0x55550000)
#define QUARTER_SECOND_NAMESET_EVEN \
	(ALL_TIMED_NAMESETS^QUARTER_SECOND_NAMESET_ODD)
#define HALF_SECOND_NAMESET_ODD		(0x33330000)
#define HALF_SECOND_NAMESET_EVEN \
	(ALL_TIMED_NAMESETS^HALF_SECOND_NAMESET_ODD)
#define ONE_SECOND_NAMESET_ODD		(0x0f0f0000)
#define ONE_SECOND_NAMESET_EVEN \
	(ALL_TIMED_NAMESETS^ONE_SECOND_NAMESET_ODD)
#define TWO_SECOND_NAMESET_ODD		(0x00ff0000)
#define TWO_SECOND_NAMESET_EVEN \
	(ALL_TIMED_NAMESETS^TWO_SECOND_NAMESET_ODD)


#define ALL_INVISIBILITY_BITS	\
    (LIGHTS_MASK|BACKUP_MASK|WHEELS_MASK|CAR_GUN_MASK \
	|ALL_TIMED_NAMESETS)
#define DEFAULT_INVISIBILITY_BITS \
    (LIGHTS_OFF_BRAKES_OFF|BACKUP_LIGHTS_OFF|WHEELS_CENTER \
	|TIMED_NAMESET(0))


#define SUN_DIRECTION_X	(0.24055810)	/* SIN23*COS52 */
#define SUN_DIRECTION_Y	(0.92050485)	/* COS23 */
#define SUN_DIRECTION_Z	(-0.30790033)	/* SIN23*SIN52 */
#define PEX_AMBIENT_INTENSITY		0.4
#define PEX_DIRECT_INTENSITY		0.9

/* Default sky/background color */
#define SKY_RED				0.43
#define SKY_GREEN			0.61
#define SKY_BLUE			1.00


#if 0
typedef enum { FALSE, TRUE } boolean_type;
#endif

typedef int boolean_type;

#ifndef TRUE
# define TRUE	1
#endif

#ifndef FALSE
# define FALSE	0 
#endif

#ifndef M_PI
# define M_PI	3.14159265358979323846
#endif

/* Default directory and path names */
#define	DRIVE_DIRNAME		"/usr/demos/graphics/drive"
    /* Override with env DRIVE_DIRECTORY */
#define SCENE_DIRNAME		"scenes"
#define CONSTRUCT_DIRNAME	"constructs"

/* definitions for states in demo mode */
#define PRACTICE_STATE		0
#define PRE_RACE_STATE		1
#define RACE_STATE		2
#define POST_RACE_STATE		3
#define WELCOME_STATE		4	/* Only used until server updates us */

#define MAX_PLAYERS 		32
#define MAX_OBJECTS_PER_SCENE	300
#define MAX_DRIVEABLES		32

#define WHEEL_SEG 100
#define HUB_SEG   101

/* Defines for invalid position */
#define POSITION_INVALID 999999

#define EXPLOSION_RADIUS	20.0

/* Typedefs to achieve 16/32/64-bit portability */
typedef char		SP_I8;
typedef char		SP_CHAR;
typedef unsigned char	SP_U8;
typedef short		SP_I16;
typedef unsigned short	SP_U16;
typedef int		SP_I32;
typedef unsigned int	SP_U32;
typedef float		SP_FLOAT;

/*************************** GLOBAL STRUCTURES ********************************/
typedef float matrix3d[4][4];

typedef enum {
    GAUGE_ANALOG_MPH		= 0,
    GAUGE_DIGITAL_MPH		= 1,
    GAUGE_ANALOG_RPM		= 2,
    GAUGE_DIGITAL_RPM		= 3,
    GAUGE_ANALOG_ALTITUDE	= 4,
    GAUGE_DIGITAL_ALTITUDE	= 5,
    GAUGE_DIGITAL_TURBO		= 6,
    GAUGE_DIGITAL_SHELLS	= 7
} GAUGE_CLASS;

typedef struct driveable_struct {
    int   display_list;
    int   object_number;
    char  name[NAME_STRLEN];
    float horsepower;

    /* Information to get the gauges set up correctly */
    GAUGE_CLASS	left_gauge_class;
    GAUGE_CLASS	right_gauge_class;
    float max_speed;
    float max_rpm;
    float max_altitude;

    /* and other stuff as we need to add it */
} Driveable, *Driveable_ptr;


typedef struct update_display {
    float color[3];
    int   invis_words;
    unsigned int invis[2];
    char  name[20];
}UpdateDisp, *UpdateDisp_ptr;

/* Client Starting position structure  -- this will be part of the 
 * connection structure, but it is used by the client and the server. */

typedef struct {
    int scene_x, scene_z;
    float pos[3];
    float angle;
} drive_start_type;


/****************************** GLOBAL VARIABLES ******************************/
extern boolean_type debug;
extern char *version;

/****************************** Old Starbase stuff  ***************************/

#define	CLOCKWISE		0
#define COUNTER_CLOCKWISE	1


/* text alignment enumerated types */
#define TA_LEFT 0
#define TA_CENTER_SB 1
#define TA_RIGHT 2
#define TA_CONTINUOUS_HORIZONTAL 3
#define TA_NORMAL_HORIZONTAL 4

/* character path and line path enumerated types */
#define PATH_RIGHT 0
#define PATH_LEFT 1
#define PATH_UP 2
#define PATH_DOWN 3

#define CI_PRUNE		1
#define CI_CULL			2

#define	PRE			0
#define PUSH			1

#define UNIT_NORMALS		0x0200

/* spline orders and rationalities */
#define SPLINE_CACHED 0x80000000
#define NONRATIONAL 0
#define RATIONAL 1
#define LINEAR 2
#define QUADRATIC 3
#define CUBIC 4
#define QUARTIC 5
#define QUINTIC 6
#define DC_VALUES 0
#define VDC_VALUES 2
#define STEP_SIZE 3
#define STEP_SIZE_CACHED (STEP_SIZE | SPLINE_CACHED)

typedef struct {
 float refx,refy,refz;
 float camx,camy,camz;
 float upx,upy,upz;
 float field_of_view;
 float front,back;
 float projection;
} camera_arg;

#endif /* _GLOBAL_INCLUDED */
