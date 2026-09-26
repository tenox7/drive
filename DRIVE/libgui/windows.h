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


#ifndef _WINDOWS_H_INCLUDED
#define _WINDOWS_H_INCLUDED

#include "gui.h"

/********************************* MACROS *************************************/
#define CLASS			"Drive"
#define PROGRAM			"drive"
#define MAX_ARG			32
#define MENUBAR_HEIGHT		30

#define MESSAGEWIN_HEIGHT	100
#define GUTTER			5
#define ACCBRKBAR_WIDTH		25
#define ACCBRKBAR_LABELHEIGHT	15
#define MAINWIN_MINHEIGHT	150
#define MAINWIN_DEFLGHEIGHT	500
#define MAINWIN_DEFSMHEIGHT	(MAINWIN_DEFLGHEIGHT/2)
#define LARGEDASH_WIDTH		800
#define LARGEDASH_HEIGHT	300
#define SMALLDASH_WIDTH		(LARGEDASH_WIDTH/2)
#define SMALLDASH_HEIGHT	(LARGEDASH_HEIGHT/2)
#define LARGEGEAR_WIDTH		160
#define LARGEGEAR_HEIGHT	210
#define SMALLGEAR_WIDTH		(LARGEGEAR_WIDTH/2)
#define SMALLGEAR_HEIGHT	(LARGEGEAR_HEIGHT/2)
#define LARGECOMPASS_WWIDTH	60
#define LARGECOMPASS_WHEIGHT	30
#define LARGECOMPASS_X1		270
#define LARGECOMPASS_Y1		60
#define LARGECOMPASS_X2		(LARGECOMPASS_X1+LARGECOMPASS_WWIDTH)
#define LARGECOMPASS_Y2		(LARGECOMPASS_Y1+LARGECOMPASS_WHEIGHT)
#define SMALLCOMPASS_X1		(LARGECOMPASS_X1/2)
#define SMALLCOMPASS_Y1		(LARGECOMPASS_Y1/2)
#define SMALLCOMPASS_X2		(LARGECOMPASS_X2/2)
#define SMALLCOMPASS_Y2		(LARGECOMPASS_Y2/2)
#define LARGECOMPASS_WIDTH	540
#define LARGECOMPASS_HEIGHT	30
#define SMALLCOMPASS_WIDTH	(LARGECOMPASS_WIDTH/2)
#define SMALLCOMPASS_HEIGHT	(LARGECOMPASS_HEIGHT/2)
#define COMPASS_MARGIN		2
/* Assumes large dash.  Divide by two for small */
#define HUB_RADIUS		80
#define WHEEL_RADIUS		310
#define HUB_CENTER_X		295
#define HUB_CENTER_Y		350

#define GAUGE_RADIUS		90
#define GEAR_XC			(LARGECOMPASS_X1+LARGECOMPASS_WWIDTH/2)
#define GEAR_YC			((LARGECOMPASS_Y2+(HUB_CENTER_Y-HUB_RADIUS))/2)

#define DEFAULT_RADAR_SIZE	400

#define FIXED_WIDTH	(GUTTER+ACCBRKBAR_WIDTH+GUTTER)
#define FIXED_HEIGHT	(MENUBAR_HEIGHT+GUTTER+GUTTER+MESSAGEWIN_HEIGHT)
#define WIN_MINWIDTH	(SMALLDASH_WIDTH+FIXED_WIDTH)
#define WIN_MINHEIGHT	(MAINWIN_MINHEIGHT+SMALLDASH_HEIGHT+FIXED_HEIGHT)
#define WIN_SMALLWIDTH	WIN_MINWIDTH
#define WIN_SMALLHEIGHT	(MAINWIN_DEFSMHEIGHT+SMALLDASH_HEIGHT+FIXED_HEIGHT)
#define WIN_LARGEWIDTH	(LARGEDASH_WIDTH+FIXED_WIDTH)
#define WIN_LARGEHEIGHT	(MAINWIN_DEFLGHEIGHT+LARGEDASH_HEIGHT+FIXED_HEIGHT)
#define LARGEDASH_MINTOTWIDTH  (LARGEDASH_WIDTH)
#define LARGEDASH_MINTOTHEIGHT (MAINWIN_MINHEIGHT+LARGEDASH_HEIGHT+FIXED_HEIGHT)

#define DRAW_WIDTH(total_width) \
    ((total_width) - FIXED_WIDTH)
#define DRAW_HEIGHT(total_height,dash_height) \
    ((total_height) - (dash_height) - FIXED_HEIGHT)



#define INITIALIZE_XIMAGE_PARAMETERS(ximage) \
{ \
    (ximage).depth      = xs.dashInfo.visualInfo.depth; \
    (ximage).red_mask   = xs.dashInfo.stdCmap.red_max   * xs.dashInfo.stdCmap.red_mult;   \
    (ximage).green_mask = xs.dashInfo.stdCmap.green_max * xs.dashInfo.stdCmap.green_mult; \
    (ximage).blue_mask  = xs.dashInfo.stdCmap.blue_max  * xs.dashInfo.stdCmap.blue_mult;  \
    if (xs.dashInfo.cmap_size > 256) { \
	(ximage).bits_per_pixel = 32; \
    } \
    else { \
	(ximage).bits_per_pixel = 8; \
    } \
    (ximage).byte_order = ImageByteOrder(xs.display); \
}


/*** COLOR ***/
#define COLORMAX		65535
#define GRAY_RED_WEIGHT		299
#define GRAY_GREEN_WEIGHT	587
#define GRAY_BLUE_WEIGHT	114
#define GRAY_WEIGHT_BASE	\
    (GRAY_RED_WEIGHT+GRAY_GREEN_WEIGHT+GRAY_BLUE_WEIGHT)

/* Some 332 colors */
#define RED332			0xe0
#define GREEN332		0x1c
#define BLUE332			0x03
#define BLACK332		0x00
#define WHITE332		0xff
#define CYAN332			(GREEN332|BLUE332)
#define GRAY332			0x49
#define BACKGROUND332		0x73

/* And translations from other formats to 332 */
#define RGB_TO_332(r,g,b) \
    ( ((int) ((r)*7.0+0.5) << 5) \
    | ((int) ((g)*7.0+0.5) << 2) \
    | ((int) ((b)*3.0+0.5)))
#define RGB_TO_PIXEL(r,g,b) \
    (xs.dashInfo.map332ToPixel[RGB_TO_332((r),(g),(b))])
#define PIXELVAL_TO_332(r,g,b) \
    ( ((((r)*7+(COLORMAX/2))/COLORMAX) << 5) \
    | ((((g)*7+(COLORMAX/2))/COLORMAX) << 2) \
    | ((((b)*3+(COLORMAX/2))/COLORMAX)))
#define PIXELVAL_TO_PIXEL(r,g,b) \
    (xs.dashInfo.map332ToPixel[PIXELVAL_TO_332((r),(g),(b))])
	
/* macros to set dash colors quickly and easily */
#define SET_FOREGROUND(fg) \
{ \
    if (xs._current_fg != (fg)) { \
	XSetForeground(xs.display,xs.dashGC,(fg)); \
	xs._current_fg = (fg); \
    } \
}

#define SET_FOREGROUND_RGB(red,green,blue) \
{   unsigned long _pix; \
    _pix = RGB_TO_PIXEL((red),(green),(blue)); \
    SET_FOREGROUND(_pix); \
}

#define SET_FOREGROUND_GREY(grey) \
{   unsigned long _pix; \
    _pix = RGB_TO_PIXEL((grey),(grey),(grey)); \
    SET_FOREGROUND(_pix); \
}

#define SET_FOREGROUND_NAMED(cname) \
{   XColor _xc,_junk_xc; unsigned long _pix; \
    XLookupColor(display,xs.stdCmap.colormap,(cname),&_junk_xc,&_xc); \
    _pix = PIXELVAL_TO_PIXEL(_xc.red,_xc.green,_xc.blue); \
    SET_FOREGROUND(_pix); \
}


typedef struct {
    XVisualInfo visualInfo;		/* includes depth, visual */
    XStandardColormap stdCmap;		/* includes xs.stdCmap.colormap */
    int cmap_size;			/* stdCmap.colormap size */
    Pixel map332ToPixel[256];		/* 332->Pixel translation table */
} VISUAL_AND_CMAP_INFO;

/***************************** GLOBAL VARIABLES *******************************/
typedef struct {
    XtAppContext
	app_context;
    Display
	*display;
    Screen
	*screen;
    int 
	screenNum;
    Widget
	app_shellW;
    Boolean
	useSmallDash;
    Window
	mainWindow;
    Cursor
	crosshair;
    unsigned long
	_current_fg;		/* for redundancy checks */
    Boolean
	wheel_drawn;

    /*** Visual and colormap information ***/
    VISUAL_AND_CMAP_INFO	dashInfo;
    VISUAL_AND_CMAP_INFO	graphicsInfo;

    /*** for main graphics window ***/
    Window
	graphicsWindow;
    int
	grHeight;		/* Height of SB/PEXlib window */
    GC 
	grGC;

    /*** for the accelerate/brake window ***/
    Window
	abWindow;		/* AB=Accelerate/Brake */
    GC 
	abGC;

    /*** for the dashboard window ***/
    Window
	dashWindow;
    GC 
	dashGC;
    XFontStruct
	*gearFontStruct;
    Font
	gearFont;

    /*** for the text window ***/
    Window
	textWindow;
    GC
	textGC;
    Font
	textBfont,		/* big font */
	textDfont;		/* default font */

    /*** Colors ***/
    Pixel
	red,
	green,
	blue,
	black,
	white,
	cyan,
	gray,
        background;

    /*** The radar window ***/
    Window
	radarWindow;

    /*** The standings window ***/
    Window
	raceWindow;
} X_STATE;

extern X_STATE xs;


/***************************** FUNCTION PROTOTYPES ****************************/

/*** From windows.c ***/
extern void redrawDash_xywh(
    int x, int y,
    int width, int height);
extern void compass_draw(
    void);
extern void drawGearshift(
    int which,
    int x, int y,
    int width, int height);
extern void handleStandardInput(
    XEvent *event);


/*** From xutils.c ***/
extern void set_vendor_flag(
    char *vendor_string);
extern int screenNumOfScreen(
    Display *display,
    Screen *screen);
extern Widget createButton(
    Widget parent,
    char *label,
    char mnemonic,
    XtCallbackProc activateCallback, caddr_t callbackData);
extern Boolean intersectRect(
    int x1min, int y1min, int x1max, int y1max,	/* from server */
    int x2min, int y2min, int x2max, int y2max,	/* constants, LARGE assumed */
    int *x3min, int *y3min, int *x3max, int *y3max);	/* result */
extern XFontStruct *getFont(
    char *option,
    char *default_font);
extern void XmString_to_string(
    XmString xmstr,
    char *str);
extern void destroyWidgetCallback(
    Widget parentW,
    Widget whichW,
    XmAnyCallbackStruct *cb);


/*** From cmap.c ***/
extern Boolean getVisuals(
    XVisualInfo *vdash,
    XVisualInfo *vgraphics);
extern void get_standard_colormap(
    XVisualInfo *vinfo,
    XStandardColormap *stdCmap,
    int *cmap_size,
    Pixel map332ToPixel[256]);
extern void getPixmap(
    char *filename,
    int width, int height,
    unsigned char **ldata,
    unsigned char **sdata,
    Boolean downsample);


/*** From menubar.c ***/
extern Widget create_menu_bar(
    Widget parent,
    Arg arg[],
    Cardinal ac);
extern void toggleRadarCallback(
    void);
extern void toggleLeaderCallback(
    void);
extern void updateVehicleSensitivity(
    int state);


/*** From wheel.c ***/
extern boolean_type rect_intersects_spoke(
    int xa, int ya, int xb, int yb,
    float xval);


/*** From textwin.c ***/
extern void exposeTextWindow(
    Widget whichW,
    void *client_data,
    XmDrawingAreaCallbackStruct *cb);


/*** From help.c ***/
extern void parseHelpFile(
    char *subject[],
    int *num_subjects,
    char *subject_text[]);
extern void createVisCtrlsWindow(
    Widget parent);
extern void mapVisCtrlsWindow(
    void);
extern void unmapVisCtrlsWindow(
    void);
extern void genericHelpCallback(
    Widget whichW,
    char *helpmsg,
    XmAnyCallbackStruct *cb);


/*** From radar.c ***/
extern void createRadarWindow(
    Position x, Position y);
extern void mapRadarWindow(
    void);
extern void unmapRadarWindow(
    void);
extern void radarEventHandler(
    XEvent *event);


/*** From placewin.c ***/
extern void createRaceWindow(
    Position x, Position y);
extern void mapRaceWindow(
    void);
extern void unmapRaceWindow(
    void);
extern void raceEventHandler(
    XEvent *event);


/*** From title.c ***/
void createTitleWindow(
    Widget parent,
    int width,int height);


#endif /* _WINDOWS_H_INCLUDED */
