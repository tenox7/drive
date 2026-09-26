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
 * gauge.h - Definitions for gauge objects.
 */

#ifndef _GAUGE_INCLUDED
#define _GAUGE_INCLUDED

#include "global.h"
#include <math.h>
#if !(defined(WIN32) || defined(MAC))
#include <X11/X.h>
#include <X11/Xlib.h>
#include <Xm/Xm.h>
#endif

/************************ MACROS AND CONSTANTS ********************************/
/* Octant defs. */
#define PI_1_8		(M_PI*1.0/8.0)
#define PI_2_8		(M_PI*2.0/8.0)
#define PI_3_8		(M_PI*3.0/8.0)
#define PI_4_8		(M_PI*4.0/8.0)
#define PI_5_8		(M_PI*5.0/8.0)
#define PI_6_8		(M_PI*6.0/8.0)
#define PI_7_8		(M_PI*7.0/8.0)
#define PI_8_8		(M_PI)
#define PI_9_8		(M_PI*9.0/8.0)
#define PI_10_8		(M_PI*10.0/8.0)
#define PI_11_8		(M_PI*11.0/8.0)
#define PI_12_8		(M_PI*12.0/8.0)
#define PI_13_8		(M_PI*13.0/8.0)
#define PI_14_8		(M_PI*14.0/8.0)
#define PI_15_8		(M_PI*15.0/8.0)
#define PI_16_8		(M_PI*2.0)

#define GAUGEFONT_LARGE		"12x24"
#define GAUGEFONT_MEDIUM	"8x16"
#define GAUGEFONT_SMALL		"6x10"
#define GAUGEFONT_TINY		"5x7"

#define MAINGAUGE_X_LEFT	170
#define MAINGAUGE_X_RIGHT	420
#define MAINGAUGE_Y		160

/******************************* STANDARD GAUGES ******************************/
typedef enum {
    GAUGE_ANALOG	= 0,
    GAUGE_DIGITAL	= 1
} GAUGE_READOUT_TYPE;

#define DIGITAL_DIGITS	5

/* Gauge structure definition. */
struct gauge_struct {
    GAUGE_CLASS class;		/* what it's reading */
    GAUGE_READOUT_TYPE type;	/* Analog or digital */

    float min_input;		/* Minimum gauge value. */
    float max_input;		/* Maximum gauge value. */

    int begin;			/* Where to start (in degrees). */
    int end;			/* Where to finish (in degrees). */

    float value;		/* Current value. */

    char *label;		/* Gauge label. */
    int min;			/* Minimum value to label gauge with. */
    int max;			/* Maximum value to label gauge with. */
    int major;			/* Major increment for labels. */
    float minor;		/* Minor increment for labels. */
    int font_index;		/* 0-2, with 0 being the largest */

    /*** The following stuff will be filled in by add_gauge ***/
    int x, y;			/* Position of gauge. */
    unsigned int width,height;	/* Size of digital gauge */
    int radius;			/* Gauge radius. */
#if !(defined(WIN32) || defined(MAC))
    XFontStruct *fontStruct[4];	/* Large, medium, and small */
#endif
    struct gauge_struct *next;
};
typedef struct gauge_struct gauge_type;
extern gauge_type *gauge_list;


/*********************** GEAR SELECTOR DISPLAY ********************************/
#define MAX_GEARS 7
#define GEAR_HORIZONTAL 1
#define GEAR_VERTICAL 0
#define GEAR_AUTOMATIC 1
#define GEAR_STANDARD  0

struct gear_struct
{
    int type;		/* manual or automatic */
    int updates;	/* for automatic switching */

    int max_gears;  	/* maximum number of foreward gears */
    int num_gears; 	/* how many total gears (including N and R ) */
    int desired;    	/* which gear the driver wishes to be in */
    int actual;     	/* the gear the server thinks the car is in */
    float color[3];	/* color for unselected gear labels */
    float scolor[3];	/* color for selected gear label */
    float rev_speed; 	/* the maximum speed the car can be going and still
			 * shift into reverse */
    char *labels[MAX_GEARS];/* labels for gears, incl Neutral and Reverse */


    /*** The following stuff will be filled in by add_gear ***/
    int x,y;
    unsigned int width,height;
    unsigned int charheight;
#if !(defined(WIN32) || defined(MAC))
    XFontStruct *fontStruct[2];	/* large and medium */
    Pixel uColor,sColor;
#endif
    struct gear_struct *next;
};
typedef struct gear_struct gear_type;
extern gear_type *gear_list;


/**************************** COMPASS *****************************************/
#define COMPASS_CHANGE	.0872664626
#define GAUGE_DELTA	(0.5)

/* Compass structure definition. */
struct compass_struct
{
    float travel_angle;		/* Current direction of travel. */
    float home_angle;		/* Direction back to center. */
};
typedef struct compass_struct compass_type;

/********************************** RADAR *************************************/
#define	RADAR_CHANGE		(3)
struct radar_car
{
    float x, y, z;
    float r, g, b;
};

struct radar_struct
{
    float xpos, ypos, zpos;		/* X,Z position of car. */
    float angle;
    int num_cars;		/* Number of cars on radar. */
    struct radar_car car_list[MAX_PLAYERS];
    float xform[4][4];
    float ixform[4][4];
};
typedef struct radar_struct radar_type;


/*********************** FUNCTION PROTOTYPES **********************************/
extern void initGaugeModule(
    void);

extern void addGauge(
    gauge_type *g);
extern void removeGauge(
    gauge_type *g);
extern void gaugeDraw(
    gauge_type *g);
extern void gaugeUpdate(
    gauge_type *g,
    float value,
    boolean_type force);

extern void addGear(
    gear_type *g);
extern void removeGear(
    gear_type *g);
extern void gearDraw(
    gear_type *g);

extern void compassDraw(
    void);
extern void compassUpdate(
    compass_type *c,
    float travel_angle, float home_angle,
    boolean_type force);

extern void bargraphUpdate(
    float value,
    boolean_type force);
extern void segment_receive_indicator_on(void);
extern void segment_receive_indicator_off(void);

#endif /* _GAUGE_INCLUDED */
