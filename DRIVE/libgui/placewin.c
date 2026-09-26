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


#if defined(MOTIF_GUI)
#include <X11/X.h>
#include <X11/Xlib.h>
#endif

#include "global.h"
#include "drive.h"
#include "gui.h"
#include "message.h"
#include "windows.h"

#define DEFAULT_BIGFONT	"vrb-25"
#define DEFAULT_FONT	"vr-20"
#define WINDOW_LINES	10
#define WINCHARS	26


static Boolean isMapped;
static boolean_type displayFinal = FALSE;

static int headerHeight,lineHeight,raceWidth,raceHeight,currentLines;

#if defined(MOTIF_GUI)
static GC raceGC;
static XFontStruct *xfs_medium, *xfs_large;
#endif

#define POSITION_STRLEN 64	
static char position_str[MAX_PLAYERS][POSITION_STRLEN];
static boolean_type highlight_position[MAX_PLAYERS];

#define TEXT_YMIN(i)	(headerHeight+(i-1)*lineHeight)
#define TEXT_YMAX(i)	(headerHeight+(i)*lineHeight)
#define TEXT_YTEXT(i)	(headerHeight+(i-1)*lineHeight+xfs_medium->ascent)

/* Buffer font settings to avoid X calls */
static Font _currentFont = -1;
#if defined(MOTIF_GUI)
#define SETFONT(fid) \
{ \
    if (_currentFont != (fid)) { \
	_currentFont = (fid); \
	XSetFont(xs.display,raceGC,(fid)); \
    } \
}
#else
#define SETFONT(fid)
#endif


void createRaceWindow(
    Position x, Position y)
{
#if defined(MOTIF_GUI)
    int i;
    XSetWindowAttributes attrs;
    XSizeHints hints;
    XWMHints wmhints;

    /* get fonts */
    xfs_large  = getFont("resultsFontBig",DEFAULT_BIGFONT);
    xfs_medium = getFont("resultsFont",DEFAULT_FONT);
    lineHeight = xfs_medium->ascent + xfs_medium->descent;
    raceWidth = (xfs_medium->max_bounds.width+xfs_medium->min_bounds.width)/2
	* WINCHARS;
    headerHeight = xfs_large->ascent + xfs_large->descent + (2*GUTTER);
    currentLines = WINDOW_LINES;
    raceHeight = headerHeight+lineHeight*WINDOW_LINES;

    attrs.colormap = xs.dashInfo.stdCmap.colormap;
    attrs.background_pixel = xs.black;
    attrs.border_pixel = xs.black;
    xs.raceWindow = XCreateWindow(xs.display,RootWindowOfScreen(xs.screen),
        x,y,raceWidth,raceHeight,
        0,xs.dashInfo.visualInfo.depth,InputOutput,xs.dashInfo.visualInfo.visual,
        CWBackPixel|CWBorderPixel|CWColormap,&attrs);
    if (xs.raceWindow == 0) return;
    hints.x = x; hints.y = y;
    hints.width = raceWidth; hints.height = raceHeight;
    hints.flags = USPosition|USSize;
    XSetNormalHints(xs.display,xs.raceWindow,&hints);
    XStoreName(xs.display,xs.raceWindow,"Race Leaders");
    wmhints.window_group = XtWindow(xs.app_shellW);
    wmhints.flags = WindowGroupHint;
    XSetWMHints(xs.display,xs.radarWindow,&wmhints);

    XSelectInput(xs.display,xs.raceWindow,
	ExposureMask|StructureNotifyMask|SubstructureNotifyMask);
    XRaiseWindow(xs.display,xs.raceWindow);
    raceGC = XCreateGC(xs.display,xs.raceWindow,0,NULL);
    XSetForeground(xs.display,raceGC,xs.white);

    for (i=0; i<MAX_PLAYERS; ++i) {
	position_str[i][0] = '\0';
	highlight_position[i] = FALSE;
    }
    isMapped = False;
#endif
}


static void handleExposeEvent(
    int expx, int expy,
    int expwidth, int expheight)
{
#if defined(MOTIF_GUI)
    static XTextItem text = {NULL, 0, 0, None};
    int i,x;

    if (!isMapped) return;

    XClearArea(xs.display,xs.raceWindow,
	expx,expy,expwidth,expheight,False);

    if (expy < headerHeight) {
	/* need to redraw Title */
	XClearArea(xs.display,xs.raceWindow,0,0,raceWidth,headerHeight,False);
	SETFONT(xfs_large->fid);
	if (displayFinal) {
	    text.chars = "RACE RESULTS";
	    text.nchars = 12;
	}
	else {
	    text.chars = "CURRENT LEADERS";
	    text.nchars = 15;
	}
	x = (raceWidth - XTextWidth(xfs_large,text.chars,text.nchars))/2;
	XDrawText(xs.display,xs.raceWindow,raceGC,
	    x,headerHeight-GUTTER-xfs_large->descent,&text,1);
    }

    /* redraw current contents */
    for (i=0; i<currentLines; ++i) {
	if (position_str[i][0] == '\0') continue;
	if ((expy > TEXT_YMIN(i)) || (expy+expheight < TEXT_YMAX(i))) continue;
	/* else */
	SETFONT(xfs_medium->fid);
	XClearArea(xs.display,xs.raceWindow,0,TEXT_YMIN(i),
	    raceWidth,lineHeight,False);
	text.chars = &(position_str[i][0]);
	text.nchars = strlen(text.chars);
	if (highlight_position[i]) {
	    XSetForeground(xs.display,raceGC,xs.black);
	    XSetBackground(xs.display,raceGC,xs.white);
	    XDrawImageString(xs.display,xs.raceWindow,raceGC,
		0,TEXT_YTEXT(i),
		&(position_str[i][0]),strlen(&(position_str[i][0])));
	    XSetForeground(xs.display,raceGC,xs.white);
	    XSetBackground(xs.display,raceGC,xs.black);
	}
	else {
	    XDrawText(xs.display,xs.raceWindow,raceGC,
		0,TEXT_YTEXT(i),&text,1);
	}
    }
    XFlush(xs.display);
#endif
}


void mapRaceWindow(
    void)
{
#if defined(MOTIF_GUI)
    if (isMapped || (xs.raceWindow == 0)) return;
    isMapped = True;

    XMapWindow(xs.display,xs.raceWindow);
    XFlush(xs.display);
    handleExposeEvent(0,0,raceWidth,raceHeight);
#endif
}


void unmapRaceWindow(
    void)
{
#if defined(MOTIF_GUI)
    if (!isMapped) return;

    XUnmapWindow(xs.display,xs.raceWindow);
    isMapped = False;
#endif
}


void raceEventHandler(
    XEvent *event)
{
#if defined(MOTIF_GUI)
    XExposeEvent *exposeEvent;
    XConfigureEvent *configEvent;

    switch (event->type) {
	case Expose:
	    exposeEvent = (XExposeEvent *) event;
	    handleExposeEvent(exposeEvent->x,exposeEvent->y,
		exposeEvent->width,exposeEvent->height);
	    break;
	case ConfigureNotify:
	    configEvent = (XConfigureEvent *) event;
	    /* check for resize */
	    if ((configEvent->width != raceWidth) 
		    || (configEvent->height != raceHeight)) {
		raceWidth  = configEvent->width;
		raceHeight = configEvent->height;
		/* handleExposeEvent(0,0,raceWidth,raceHeight); */
	    }
	    break;
	case UnmapNotify:
	    unmapRaceWindow();
	    break;
	case DestroyNotify:
	    unmapRaceWindow();
	    xs.raceWindow = 0;
	    break;
    }
#endif
}


void updateRacePosition(
    char *text,
    int position,
    boolean_type highlight,
    float r, float g, float b)
{
    char str[1024];


#if defined(MOTIF_GUI)
    if (xs.raceWindow == 0) return;

    if (position > currentLines) {
	/* resize the window */
	currentLines = position;
	raceHeight = headerHeight + lineHeight*currentLines;
	XResizeWindow(xs.display,xs.raceWindow, raceWidth,raceHeight);
    }
#endif

    sprintf(str,"#%2d: %s (%s)", position,text,lookup_rgb(r,g,b));
    strncpy(&(position_str[position][0]),str,POSITION_STRLEN);
    highlight_position[position] = highlight;

    handleExposeEvent(0,TEXT_YMIN(position),raceWidth,lineHeight);
}


void updateRacePositionType(
    boolean_type is_final,
    int players)
{
#if defined(MOTIF_GUI)
    if (xs.raceWindow == 0) return;

    /* Clear any blank spots in the list */
    if (players < currentLines) {
	XClearArea(xs.display,xs.raceWindow,
	    0,headerHeight + players*lineHeight,
	    raceWidth,(currentLines-players)*lineHeight,False);
    }
#endif

    if (is_final == displayFinal) return;
    displayFinal = is_final;

    handleExposeEvent(0,0,raceWidth,headerHeight);
}
