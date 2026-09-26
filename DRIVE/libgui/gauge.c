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



#include <math.h>
#include <string.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <Xm/Xm.h>

#include "global.h"
#include "drive.h"
#include "drive.h"
#include "gauge.h"
#include "windows.h"

gauge_type *gauge_list;
gear_type  *gear_list;

void initGaugeModule(
    void)
{
    gauge_list = NULL;
    gear_list = NULL;
}


void addGauge(
    gauge_type *g)
{
    static char *digstr[8] = {"8","88", "888", "8888",
	"88888", "888888", "8888888", "88888888"
    };
    int i;
    int dir,asc,des;
    XCharStruct ret;

#if defined(MOTIF_GUI)
    g->fontStruct[0] = getFont("gaugeFontLarge", GAUGEFONT_LARGE);
    g->fontStruct[1] = getFont("gaugeFontMedium",GAUGEFONT_MEDIUM);
    g->fontStruct[2] = getFont("gaugeFontSmall", GAUGEFONT_SMALL);
    g->fontStruct[3] = getFont("gaugeFontTiny",  GAUGEFONT_TINY);

    if (g->type == GAUGE_ANALOG) {
	g->radius = GAUGE_RADIUS*5/8;
    }
    else {
	/* Size and offset the rectangle based on the font. */
	if ((i = DIGITAL_DIGITS-1) > 7) i = 7;
	XTextExtents(g->fontStruct[g->font_index],digstr[i],i+1,
	    &dir,&asc,&des,&ret);
	g->width = ret.width + 4;
	g->height = ret.ascent + ret.descent + 4;
	g->x -= g->width/2;
	g->y -= g->height/2;
    }
#endif

    g->next = gauge_list;
    gauge_list = g;
}


void removeGauge(
    gauge_type *g)
{
    gauge_type *gl;

    if (g == gauge_list) {
	/* remove from front of list */
	gauge_list = gauge_list->next;
    }
    else {
	/* Find it in the list */
	gl = gauge_list;
	while (gl->next != g) {
	    if ((gl = gl->next) == NULL) return;
	}
	gl->next = (gl->next)->next;
    }
}


/*****************************************************************
 * gauge_angle
 *
 *	Calculate angle for value on circular gauge.  (in radians)
 *
 */
static float gauge_angle(
    gauge_type *g,
    float value,
    float min, float max)
{
    float angle_degrees,fraction;

    /* Find out where this value maps in the range of the guage. */
    fraction = (value - min) / (max - min);

    /* Calculate the angle in degrees for this value. */
    angle_degrees = (float)(g->begin)
	          + fraction * (float)(g->end - g->begin);

    /* Convert degrees to radians. */
    return (DEGREES_TO_RADIANS(angle_degrees));
}


/*****************************************************************
 * draw_gauge_pointer
 *
 *	Draw pointer for circular gauge.
 *
 */
static void draw_gauge_pointer(
    gauge_type *g,
    float angle,
    Boolean erase)
{
#if defined(MOTIF_GUI)
    float sin_angle = FSIN(angle);
    float cos_angle = -FCOS(angle);
    float large_radius = g->radius * 0.95;
    float small_radius = g->radius / 16.0;
    float xdelta = small_radius * sin_angle;
    float ydelta = small_radius * cos_angle;
    XPoint xp[4];

    if (erase) {
	xp[0].x = g->x - xdelta; xp[0].y = g->y - ydelta;
	xp[1].x = g->x - ydelta; xp[1].y = g->y + xdelta;
	xp[2].x = g->x + large_radius * sin_angle;
	    xp[2].y = g->y + large_radius * cos_angle;
	xp[3].x = g->x + ydelta; xp[3].y = g->y - xdelta;
	if (xs.useSmallDash) {
	    xp[0].x /= 2; xp[0].y /= 2;
	    xp[1].x /= 2; xp[1].y /= 2;
	    xp[2].x /= 2; xp[2].y /= 2;
	    xp[3].x /= 2; xp[3].y /= 2;
	}
	SET_FOREGROUND(xs.black);
	XFillPolygon(xs.display,xs.dashWindow,xs.dashGC,
	    xp,4,Convex,CoordModeOrigin);
    }
    else {
	xp[0].x = g->x - xdelta; xp[0].y = g->y - ydelta;
	xp[1].x = g->x - ydelta; xp[1].y = g->y + xdelta;
	xp[2].x = g->x + large_radius * sin_angle;
	    xp[2].y = g->y + large_radius * cos_angle;
	if (xs.useSmallDash) {
	    xp[0].x /= 2; xp[0].y /= 2;
	    xp[1].x /= 2; xp[1].y /= 2;
	    xp[2].x /= 2; xp[2].y /= 2;
	}
	SET_FOREGROUND_GREY(0.75 - sin_angle*0.25);
	XFillPolygon(xs.display,xs.dashWindow,xs.dashGC,
	    xp,3,Convex,CoordModeOrigin);

	xp[1].x = g->x + ydelta; xp[1].y = g->y - xdelta;
	if (xs.useSmallDash) {
	    xp[1].x /= 2; xp[1].y /= 2;
	}
	SET_FOREGROUND_GREY(0.75 + sin_angle*0.25);
	XFillPolygon(xs.display,xs.dashWindow,xs.dashGC,
	    xp,3,Convex,CoordModeOrigin);
    }
#endif
}


/*****************************************************************
 * gaugeUpdate
 *
 *	Updates the pointer on a gauge.
 *
 */
void gaugeUpdate(
    gauge_type *g,
    float value,
    boolean_type force)
{
#if defined(MOTIF_GUI)
    float angle;
    float tmp_angle;
    float range, delta, degrees;
    boolean_type redraw_wheel;
    int x,y,w,h;


    /* Check for -1 on digital gauges -- and 'remove' the gauge if needed */

    if( g->type == GAUGE_DIGITAL ) {
	if (xs.useSmallDash) {
	    x = g->x/2; y = g->y/2;
	    w = g->width + 10; h = g->height + 20;
	}
	else {
	    x = g->x; y = g->y;
	    w = g->width + 10; h = g->height + 20;
	}

	if( (g->value != -1) && (value == -1)) {
	    /* Need to expose the dashboard where the gauge is */
	    g->value = value;
	    redrawDash_xywh(x,y,w,h);
	    return;
	}
	else if( value == -1) {
	    /* Don't update the gauge -- it has been erased */
	    g->value = value;
	    return;
	}
	else if ( (g->value == -1) && (value != -1) ) {
	    /* Turn the gauge back on! */
	    g->value = value;
	    gaugeDraw(g);
	}
    }

     
    /* Check for values that are out of range and peg the gauge. */
    if (value < g->min_input) value = g->min_input;
    else if (value > g->max_input) value = g->max_input;

    /* Check to see that gauge has made some significant change. */
    delta = fabs(value - g->value);
    range = g->max_input - g->min_input;
    degrees = (float)(ABS(g->end - g->begin));
    if ((delta < (range/(degrees/GAUGE_DELTA))) && !force) return;

    /* Figure out whether I need to pick up and put down the wheel */
    if (xs.useSmallDash) {
	if (g->type == GAUGE_ANALOG) {
	    x = g->x/2 - g->radius/2;  y = g->y/2 - g->radius/2;
	    w = g->radius;  h = g->radius;
	}
	else { /* GAUGE_DIGITAL */
	    x = g->x/2; y = g->y/2;
	    w = g->width; h = g->height;
	}
    }
    else {
	if (g->type == GAUGE_ANALOG) {
	    x = g->x - g->radius;  y = g->y - g->radius;
	    w = g->radius*2;  h = g->radius*2;
	}
	else {
	    x = g->x; y = g->y;
	    w = g->width; h = g->height;
	}
    }
    redraw_wheel = rect_intersects_spoke(x,y,x+w,y+h,cstate.xpointer_value);

    if (redraw_wheel) undrawWheel();

    switch (g->type) {
      case GAUGE_ANALOG:
	/* Erase the old pointer. */
	tmp_angle = gauge_angle( g, g->value, g->min_input, g->max_input );
	draw_gauge_pointer(g,tmp_angle,True);

	/* Draw the new pointer. */
	angle = gauge_angle( g, value, g->min_input, g->max_input );
	draw_gauge_pointer(g,angle,False);
	break;

      case GAUGE_DIGITAL:
	{   XTextItem text;
	    char str[256];

	    SET_FOREGROUND(xs.black);
	    XFillRectangle(xs.display,xs.dashWindow,xs.dashGC, x,y, w,h);

	    sprintf(str,"%*d",DIGITAL_DIGITS,(int)(value + 0.5));
	    text.chars  = str;
	    text.nchars = DIGITAL_DIGITS;
	    text.delta  = 0;
	    text.font   = g->fontStruct[xs.useSmallDash+g->font_index]->fid;
	    SET_FOREGROUND(xs.red);
	    XDrawText(xs.display,xs.dashWindow,xs.dashGC, x+2,y+h-2, &text,1);
	}
	break;

      default:
	fprintf( stderr, "Illegal gauge type.\n" );
	break;
    }

    /* Update current value. */
    g->value = value;

    if (redraw_wheel) drawWheel(cstate.xpointer_value);
#endif
}


static void XDrawCenteredText(
    Display *display,
    Drawable d,
    GC gc,
    int x, int y,
    XTextItem *items,
    int nitems,
    XFontStruct *fontStr)
{
    int dir,asc,des;
    XCharStruct ret;

    XTextExtents(fontStr,items->chars,items->nchars,
	&dir,&asc,&des,&ret);
    x -= ret.width/2;
    y += (ret.ascent-ret.descent)/2;
    XDrawText(display,d,gc,x,y,items,nitems);
}


/*****************************************************************
 * gauge_draw
 *
 *	Draws the numbers on a gauge.
 *
 */
void gaugeDraw(
    gauge_type *g)
{
#if defined(MOTIF_GUI)
    float angle;
    char label[10];
    int value;
    float ticval;
    XTextItem text;
    XSegment xseg[256];
    int count;
    float ca,sa;
    int x,y,r,w,h;

    undrawWheel();

    if (xs.useSmallDash) {
	if( g->type == GAUGE_ANALOG ) {
	    x = g->x/2; y = g->y/2; r = g->radius/2;
	    w = g->width/2; h = g->height/2;
	}
	else {  /* GAUGE_DIGITAL */
	    x = g->x/2; y = g->y/2; r = g->radius/2;
	    w = g->width; h = g->height;
	}
    }
    else {
	x = g->x; y = g->y; r = g->radius;
	w = g->width; h = g->height;
    }

    switch ( g->type ) {
	case GAUGE_ANALOG:
	    /* Draw a background circle for the pointer. */
	    SET_FOREGROUND(xs.black);
	    XFillArc(xs.display,xs.dashWindow,xs.dashGC,
		x-r,y-r,r*2,r*2,0*64,360*64);

	    /* Draw tick marks around edge of circle. */
	    SET_FOREGROUND(xs.cyan);
	    count = 0;
	    for (ticval=(float)g->min; ticval<=(float)g->max; ticval+=g->minor){
		angle = gauge_angle(g,ticval,(float)g->min,(float)g->max);
		ca = FCOS(angle); sa = FSIN(angle);
	       
		xseg[count].x1 = x + r*sa;
		xseg[count].y1 = y - r*ca;
		xseg[count].x2 = x + (r*21/20)*sa;
		xseg[count].y2 = y - (r*21/20)*ca;
		if (++count >= 256) break;
	    }
	    XDrawSegments(xs.display,xs.dashWindow,xs.dashGC,xseg,count);

	    count = 0;
	    for (ticval=(float)g->min; ticval<=(float)g->max; ticval+=g->major){
		angle = gauge_angle(g,ticval,(float)g->min,(float)g->max);
		ca = FCOS(angle); sa = FSIN(angle);
	       
		xseg[count].x1 = x + r*sa;
		xseg[count].y1 = y - r*ca;
		xseg[count].x2 = x + (r*23/20)*sa;
		xseg[count].y2 = y - (r*23/20)*ca;
		if (++count >= 256) break;
	    }
	    XDrawSegments(xs.display,xs.dashWindow,xs.dashGC,xseg,count);

	    /* Draw values around edge of circle. */
	    SET_FOREGROUND(xs.white);
	    for (value=g->min; value<=g->max; value+=g->major) {
		sprintf(label,"%d",value);
		angle = gauge_angle(g,(float)value,(float)g->min,(float)g->max);
		ca = FCOS(angle); sa = FSIN(angle);
		text.chars  = label;
		text.nchars = strlen(label);
		text.delta  = 0;
		text.font   = g->fontStruct[xs.useSmallDash+2]->fid;
		XDrawCenteredText(xs.display,xs.dashWindow,xs.dashGC,
		   x + (r*13/10)*sa, y - (r*13/10)*ca, &text, 1,
		   g->fontStruct[xs.useSmallDash+2]);
	    }

	    if (g->label) {
		/* Draw label. */
		text.chars  = g->label;
		text.nchars = strlen(text.chars);
		text.delta  = 0;
		text.font   = g->fontStruct[xs.useSmallDash+2]->fid;
		XDrawCenteredText(xs.display,xs.dashWindow,xs.dashGC,
		    x, y+r/2, &text,1,g->fontStruct[xs.useSmallDash+2]);
	    }
	    break;

	case GAUGE_DIGITAL:
	    if (g->value == -1) return;
	    if (g->label) {
		/* Draw label. */
		SET_FOREGROUND(xs.white);
		text.chars  = g->label;
		text.nchars = strlen(text.chars);
		text.delta  = 0;
		text.font   = g->fontStruct[xs.useSmallDash+g->font_index]->fid;
		XDrawCenteredText(xs.display,xs.dashWindow,xs.dashGC,
		    x+w/2, y+h*1.5, &text,1,
		    g->fontStruct[xs.useSmallDash+g->font_index]);
	    }
	    break;

	default:
	    fprintf( stderr, "Illegal gauge type.\n" );
	    break;
    }

    gaugeUpdate( g, 0.0, TRUE );
#endif
}



void addGear(
    gear_type *g)
{
#if defined(MOTIF_GUI)
    int dir,asc,des;
    XCharStruct ret;

    g->x = GEAR_XC;
    g->y = GEAR_YC;

    g->fontStruct[0] = getFont("gaugeFontMedium",GAUGEFONT_MEDIUM);
    g->fontStruct[1] = getFont("gaugeFontSmall",GAUGEFONT_SMALL);

    XTextExtents(g->fontStruct[0],"N",1,
	&dir,&asc,&des,&ret);
    g->width  = ret.width*2 + 4;
    g->charheight = (ret.ascent + ret.descent) * 3/2;
    g->height = g->charheight * g->num_gears;
    g->x -= g->width/2;
    g->y -= g->height/2;

    g->uColor = RGB_TO_PIXEL(g->color[0],g->color[1],g->color[2]);
    g->sColor = RGB_TO_PIXEL(g->scolor[0],g->scolor[1],g->scolor[2]);
#endif
    g->next = gear_list;
    gear_list = g;
}


void removeGear(
    gear_type *g)
{
    gear_type *gl;

    if (g == gear_list) {
	/* remove from front of list */
	gear_list = gear_list->next;
    }
    else {
	/* Find it in the list */
	gl = gear_list;
	while (gl->next != g) {
	    if ((gl = gl->next) == NULL) return;
	}
	gl->next = (gl->next)->next;
    }
}



/*****************************************************************
 * gearDraw
 *
 *	Draws the numbers on a gear.
 *
 */
void gearDraw(
    gear_type *g)
{
#if defined(MOTIF_GUI)
    int i;
    XTextItem text;
    int x,y;
    unsigned int w,h,ch;
    boolean_type redraw_wheel;

    if (xs.useSmallDash) {
	x = g->x/2; y = g->y/2; w = g->width/2; h = g->height/2;
	ch = g->charheight/2;
    }
    else {
	x = g->x; y = g->y; w = g->width; h = g->height;
	ch = g->charheight;
    }

    redraw_wheel = rect_intersects_spoke(x,y,x+w,y+h,cstate.xpointer_value);
    if (redraw_wheel) undrawWheel();

    /* First draw the black rectangle, and highlights */
    SET_FOREGROUND(xs.black);
    XFillRectangle(xs.display,xs.dashWindow,xs.dashGC,
	x,y,w,h);

    /* Now, draw each of the non-selected gears */
    text.delta = 0; text.font = g->fontStruct[(unsigned char) xs.useSmallDash]->fid;
    SET_FOREGROUND(g->uColor);
    for (i=0; i<g->num_gears; i++) {
	text.chars  = g->labels[i];
	text.nchars = strlen(text.chars);
	XDrawCenteredText(xs.display,xs.dashWindow,xs.dashGC,
	    x + w/2, y + (g->num_gears-i)*ch - ch/2,
	    &text, 1, g->fontStruct[(unsigned char) xs.useSmallDash]);
    }

    /* Now, draw selected gear */
    SET_FOREGROUND(g->sColor);
    text.chars  = g->labels[g->actual];
    text.nchars = strlen(text.chars);
    XDrawCenteredText(xs.display,xs.dashWindow,xs.dashGC,
	x + w/2, y + (g->num_gears-g->actual)*ch - ch/2,
	&text, 1, g->fontStruct[(unsigned char) xs.useSmallDash]);

    if (xs.useSmallDash) {
	drawGearshift(g->actual,SMALLDASH_WIDTH-SMALLGEAR_WIDTH,
	    SMALLDASH_HEIGHT-SMALLGEAR_HEIGHT,
	    SMALLGEAR_WIDTH,SMALLGEAR_HEIGHT);
    }
    else {
	drawGearshift(g->actual,LARGEDASH_WIDTH-LARGEGEAR_WIDTH,
	    LARGEDASH_HEIGHT-LARGEGEAR_HEIGHT,
	    LARGEGEAR_WIDTH,LARGEGEAR_HEIGHT);
    }

    if (redraw_wheel) drawWheel(cstate.xpointer_value);
#endif
}


void gearUpdate(
    gear_type *g,
    boolean_type force)
{
#if defined(MOTIF_GUI)
    static int last_gear = INVALID;


    if ((g->actual == last_gear) && !force) return;

    gearDraw(g);

    /* Redraw the gear shift */
    if (xs.useSmallDash) {
	redrawDash_xywh(SMALLDASH_WIDTH-SMALLGEAR_WIDTH,
	    SMALLDASH_HEIGHT-SMALLGEAR_HEIGHT,
	    SMALLGEAR_WIDTH,SMALLGEAR_HEIGHT);
    }
    else {
	redrawDash_xywh(LARGEDASH_WIDTH-LARGEGEAR_WIDTH,
	    LARGEDASH_HEIGHT-LARGEGEAR_HEIGHT,
	    LARGEGEAR_WIDTH,LARGEGEAR_HEIGHT);
    }

    last_gear = g->actual;
#endif
}


/*****************************************************************
 * compass_update
 *
 *	Updates the pointer on a compass.  Normally only updates
 *	if the direction has changed somewhat.  However, an 
 *	update can be forced by setting "force" to TRUE.
 *
 */
void compassUpdate(
    compass_type *c,
    float travel_angle, float home_angle,
    boolean_type force)		/* Force an update. */
{
#if defined(MOTIF_GUI)
    float delta;
    boolean_type redraw_wheel;

    delta = fabs( travel_angle - c->travel_angle );
    if ((delta < COMPASS_CHANGE) && !force) return;

    if (xs.useSmallDash) {
	redraw_wheel = rect_intersects_spoke(SMALLCOMPASS_X1,SMALLCOMPASS_Y1,
	    SMALLCOMPASS_X2,SMALLCOMPASS_Y2,cstate.xpointer_value);
    }
    else {
	redraw_wheel = rect_intersects_spoke(LARGECOMPASS_X1,LARGECOMPASS_Y1,
	    LARGECOMPASS_X2,LARGECOMPASS_Y2,cstate.xpointer_value);
    }

    if (redraw_wheel) undrawWheel();
    cstate.compass_angle = travel_angle;
    compass_draw();
    if (redraw_wheel) drawWheel(cstate.xpointer_value);

    /* Update stored angles. */
    c->travel_angle = travel_angle;
#endif
}


/*****************************************************************
 * bargraph_update
 *
 *	Updates the pointer on a bargraph.  Normally only updates
 *	if the value has changed somewhat.  However, an 
 *	update can be forced by setting "force" to TRUE.
 *
 */
void bargraphUpdate(
    float value,
    boolean_type force)		/* Force an update. */
{
#if defined(MOTIF_GUI)
    int full,half,position;
    static int last_position = -1;

    full = xs.grHeight - ACCBRKBAR_LABELHEIGHT*2;
    half = full/2;
    position = (1.0 - value)/2.0 * full;
    if (last_position == -1) last_position = half;
    if (last_position == position) return;

    if (position < half) {
	/* Did the position cross zero?  Draw some, erase some. */
	if (last_position > half) {
	    XSetForeground(xs.display,xs.abGC,xs.black);
	    XFillRectangle(xs.display,xs.abWindow,xs.abGC,
		0,half,ACCBRKBAR_WIDTH,last_position-half);

	    XSetForeground(xs.display, xs.abGC, xs.green);
	    XFillRectangle(xs.display,xs.abWindow,xs.abGC,
		0,position,ACCBRKBAR_WIDTH,half-position);
	}
	/* Is the graph increasing?  Draw in the positive color. */
	else if (position < last_position) {
	    XSetForeground(xs.display,xs.abGC,xs.green);
	    XFillRectangle(xs.display,xs.abWindow,xs.abGC,
	        0,position, ACCBRKBAR_WIDTH,last_position-position);
	}
	/* The graph is decreasing.  Erase. */
	else {
	    XSetForeground(xs.display,xs.abGC,xs.black);
	    XFillRectangle(xs.display,xs.abWindow,xs.abGC,
		0,last_position, ACCBRKBAR_WIDTH,position-last_position);
	}
    }
    else {
	/* Did the position cross zero?  Draw some, erase some. */
	if (last_position < half) {
	    XSetForeground(xs.display,xs.abGC,xs.black);
	    XFillRectangle(xs.display,xs.abWindow,xs.abGC,
		0,last_position, ACCBRKBAR_WIDTH,half-last_position);

	    XSetForeground(xs.display,xs.abGC,xs.red);
	    XFillRectangle(xs.display,xs.abWindow,xs.abGC,
		0,half, ACCBRKBAR_WIDTH,position-half);
	}
	/* Is the graph decreasing? Draw in the negative color. */
	else if (position > last_position) {
	    XSetForeground(xs.display,xs.abGC,xs.red);
	    XFillRectangle(xs.display,xs.abWindow,xs.abGC,
		0,last_position, ACCBRKBAR_WIDTH,position-last_position);
	}
	/* The graph is increasing.  Erase. */
	else {
	    XSetForeground(xs.display,xs.abGC,xs.black);
	    XFillRectangle(xs.display,xs.abWindow,xs.abGC,
		0,position, ACCBRKBAR_WIDTH,last_position-position);
	}
    }

    /* Update value. */
    last_position = position;
    cstate.accBrk_value = value;
#endif
}


/***********************************************************************
 * These next two functions, segment_receive_indicator_on() and 
 * segment_receive_indicator_off(), while not technically gauges, serve
 * to indicate when data is being received from the drive_server 
 ***********************************************************************/

void segment_receive_indicator_on(void)
{
#if defined(MOTIF_GUI)
    XSetForeground(xs.display,xs.textGC,xs.red);
    XFillRectangle(xs.display,xs.textWindow,xs.textGC,
	0,0, ACCBRKBAR_WIDTH/2,ACCBRKBAR_WIDTH);
#endif
}


void segment_receive_indicator_off(void)
{
#if defined(MOTIF_GUI)
    XSetForeground(xs.display,xs.textGC,xs.black);
    XFillRectangle(xs.display,xs.textWindow,xs.textGC,
	0,0, ACCBRKBAR_WIDTH/2,ACCBRKBAR_WIDTH);
#endif
}
