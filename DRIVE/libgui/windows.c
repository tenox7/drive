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


#include "hw.h"

#if defined(MOTIF_GUI)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/Xos.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>

#include <X11/Xutil.h>

#ifdef _UX807
#include <X11/Xatom.h>
#endif

#include <Xm/Xm.h>
#include <Xm/DrawingA.h>
#include <Xm/LabelG.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/Scale.h>
#include "Starbase.h"
#include "visDrawArea.h"
#endif

#include "global.h"
#include "drive.h"
#include "windows.h"
#include "filenames.h"
#include "gauge.h"


#if defined(MOTIF_GUI)
/*** Program globals ***/
X_STATE xs;

/*** Module globals ***/
static char myPath[MAXPATHLEN];
static Widget menuBarW,graphicsW,accBrkW,messageW,dashW;
static int fildes;
static void (*graphics_resize_routine)(int width, int height);

#ifdef __linux__
#define PRINT_VISUAL(vinfo) \
    printf("  Visual 0x%08x\n",(unsigned int) (vinfo)->visual); \
    printf("  Visual ID %ld\n",(vinfo)->visualid); \
    printf("  Screen: 0x%08x\n",(vinfo)->screen); \
    printf("  Depth:  %d\n",(vinfo)->depth); \
    printf("  Class:  %d\n",(vinfo)->class); \
    printf("  Red Mask: 0x%08lx\n",(vinfo)->red_mask); \
    printf("  Green Mask: 0x%08lx\n",(vinfo)->green_mask); \
    printf("  Blue Mask: 0x%08lx\n",(vinfo)->blue_mask); \
    printf("  Colormap Size: %d\n",(vinfo)->colormap_size); \
    printf("  Bits per RGB: %d\n",(vinfo)->bits_per_rgb);
#define PRINT_STDCMAP(cmap) \
    printf("  Colormap:  0x%08x\n",(unsigned int) (cmap)->colormap); \
    printf("  RGB Maxes:  0x%08lx, 0x%08lx, 0x%08lx\n", \
	    (cmap)->red_max, (cmap)->green_max, (cmap)->blue_max); \
    printf("  RGB Mults:  0x%08lx, 0x%08lx, 0x%08lx\n", \
	    (cmap)->red_mult, (cmap)->green_mult, (cmap)->blue_mult); \
    printf("  Base Pixel:  %ld\n",(cmap)->base_pixel); \
    printf("  Visual ID: %ld\n",(cmap)->visualid); \
    printf("  KillID:  %ld\n",(cmap)->killid);
#else
#define PRINT_VISUAL(vinfo) \
    printf("  Visual 0x%08x\n",(vinfo)->visual); \
    printf("  Visual ID %d\n",(vinfo)->visualid); \
    printf("  Screen: 0x%08x\n",(vinfo)->screen); \
    printf("  Depth:  %d\n",(vinfo)->depth); \
    printf("  Class:  %d\n",(vinfo)->class); \
    printf("  Red Mask: 0x%08x\n",(vinfo)->red_mask); \
    printf("  Green Mask: 0x%08x\n",(vinfo)->green_mask); \
    printf("  Blue Mask: 0x%08x\n",(vinfo)->blue_mask); \
    printf("  Colormap Size: %d\n",(vinfo)->colormap_size); \
    printf("  Bits per RGB: %d\n",(vinfo)->bits_per_rgb);
#define PRINT_STDCMAP(cmap) \
    printf("  Colormap:  0x%08x\n",(cmap)->colormap); \
    printf("  RGB Maxes:  0x%08x, 0x%08x, 0x%08x\n", \
	    (cmap)->red_max, (cmap)->green_max, (cmap)->blue_max); \
    printf("  RGB Mults:  0x%08x, 0x%08x, 0x%08x\n", \
	    (cmap)->red_mult, (cmap)->green_mult, (cmap)->blue_mult); \
    printf("  Base Pixel:  %d\n",(cmap)->base_pixel); \
    printf("  Visual ID: %d\n",(cmap)->visualid); \
    printf("  KillID:  %d\n",(cmap)->killid);
#endif
#endif

hwDisplay
    disp;
hwDrawable
    draw;


void redrawAccBrk(
    void)
{
#if defined(MOTIF_GUI)
    int accBrk_y,half,full;

    full = xs.grHeight - ACCBRKBAR_LABELHEIGHT*2;
    half = full/2;
    accBrk_y = (1.0 - cstate.accBrk_value)/2.0 * full;
    if (accBrk_y < half) {
	/* black + green, black */
	XSetForeground(xs.display,xs.abGC,xs.black);
	XFillRectangle(xs.display,xs.abWindow,xs.abGC,0,0,
	    ACCBRKBAR_WIDTH,accBrk_y);
	XSetForeground(xs.display,xs.abGC,xs.green);
	XFillRectangle(xs.display,xs.abWindow,xs.abGC,0,accBrk_y,
	    ACCBRKBAR_WIDTH,half-accBrk_y);
	XSetForeground(xs.display,xs.abGC,xs.black);
	XFillRectangle(xs.display,xs.abWindow,xs.abGC,0,half,
	    ACCBRKBAR_WIDTH,half);
    }
    else {
	/* black, red + black */
	XSetForeground(xs.display,xs.abGC,xs.black);
	XFillRectangle(xs.display,xs.abWindow,xs.abGC,0,0,
	    ACCBRKBAR_WIDTH,half);
	XSetForeground(xs.display,xs.abGC,xs.red);
	XFillRectangle(xs.display,xs.abWindow,xs.abGC,0,half,
	    ACCBRKBAR_WIDTH,accBrk_y-half);
	XSetForeground(xs.display,xs.abGC,xs.black);
	XFillRectangle(xs.display,xs.abWindow,xs.abGC,0,accBrk_y,
	    ACCBRKBAR_WIDTH,full-accBrk_y);
    }
#endif
}


void compass_draw(
    void)
{
#if defined(MOTIF_GUI)
    static XImage xi = {
	0,0,			/* width,height -- to be filled in */
	0,			/* xoffset */
	ZPixmap,		/* format */
	NULL,			/* data */
	MSBFirst,		/* byte_order */
	8,			/* bitmap_unit */
	MSBFirst,		/* bitmap_bit_order */
	0,			/* bitmap_pad */
	8,			/* depth */
	0,			/* bytes_per_line -- to be filled in */
	8,			/* bits_per_pixel */
	0xe0,			/* red_mask */
	0x1c,			/* green_mask */
	0x03,			/* blue_mask */
	NULL,			/* obdata */
	{   NULL,		/* f.create_image */
	    NULL,		/* f.destroy_image */
	    NULL,		/* f.get_pixel */
	    NULL,		/* f.put_pixel */
	    NULL,		/* f.sub_image */
	    NULL,		/* f.add_pixel */
	},
    };
    static unsigned char *ldata = NULL;
    static unsigned char *sdata;
    int x1,x2,y1,y2,xsrc;
    int wx1,wy1,wx2,wy2;


    if (xs.useSmallDash) {
	x1 = SMALLCOMPASS_X1; y1 = SMALLCOMPASS_Y1;
	x2 = SMALLCOMPASS_X2; y2 = SMALLCOMPASS_Y2;
    }
    else {
	x1 = LARGECOMPASS_X1; y1 = LARGECOMPASS_Y1;
	x2 = LARGECOMPASS_X2; y2 = LARGECOMPASS_Y2;
    }

    if (ldata == NULL) {
	getPixmap(compass_filename,LARGECOMPASS_WIDTH,LARGECOMPASS_HEIGHT,
	    &ldata,&sdata,True);
	INITIALIZE_XIMAGE_PARAMETERS(xi);
    }

    /* Now fill in the size-specific part of the XImage */
    if (xs.useSmallDash) {
	xi.width  = SMALLCOMPASS_WIDTH;
	xi.height = SMALLCOMPASS_HEIGHT;
	xi.data = (char *) sdata;
	wx1 = SMALLCOMPASS_X1;
	wy1 = SMALLCOMPASS_Y1;
	wx2 = SMALLCOMPASS_X2;
	wy2 = SMALLCOMPASS_Y2;

	xsrc = -cstate.compass_angle * (8.0/(18.0*M_PI)*SMALLCOMPASS_WIDTH)
	    - ((SMALLCOMPASS_X2 - SMALLCOMPASS_X1)/2) + (x1 - wx1);
	if (xsrc < 0) xsrc += (SMALLCOMPASS_WIDTH*8/9);
    }
    else {
	xi.width  = LARGECOMPASS_WIDTH;
	xi.height = LARGECOMPASS_HEIGHT;
	xi.data = (char *) ldata;
	wx1 = LARGECOMPASS_X1;
	wy1 = LARGECOMPASS_Y1;
	wx2 = LARGECOMPASS_X2;
	wy2 = LARGECOMPASS_Y2;

	xsrc = -cstate.compass_angle * (8.0/(18.0*M_PI)*LARGECOMPASS_WIDTH)
	    - ((LARGECOMPASS_X2 - LARGECOMPASS_X1)/2) + (x1 - wx1);
	if (xsrc < 0) xsrc += (LARGECOMPASS_WIDTH*8/9);
    }
    xi.bytes_per_line = xi.width * xi.bits_per_pixel/8;

    XPutImage(xs.display,xs.dashWindow,xs.dashGC,&xi,
	xsrc,y1-wy1, x1,y1,x2-x1+1,y2-y1+1);

    /* Draw the border */
    SET_FOREGROUND_GREY(0.6);
    XFillRectangle(xs.display,xs.dashWindow,xs.dashGC,
	wx1 - COMPASS_MARGIN, wy1 - COMPASS_MARGIN,
	wx2 - wx1 + 2*COMPASS_MARGIN, COMPASS_MARGIN);
    XFillRectangle(xs.display,xs.dashWindow,xs.dashGC,
	wx1 - COMPASS_MARGIN, wy2,
	wx2 - wx1 + 2*COMPASS_MARGIN, COMPASS_MARGIN);
    XFillRectangle(xs.display,xs.dashWindow,xs.dashGC,
	wx1 - COMPASS_MARGIN, wy1,
	COMPASS_MARGIN, wy2 - wy1);
    XFillRectangle(xs.display,xs.dashWindow,xs.dashGC,
	wx2, wy1,
	COMPASS_MARGIN, wy2 - wy1);
    XFillRectangle(xs.display,xs.dashWindow,xs.dashGC,
	(wx2+wx1)/2, wy1,
	1,(wy2 - wy1));
#endif
}


void drawGearshift(
    int which,
    int x, int y,
    int width, int height)
{
#if defined(MOTIF_GUI)
    static XImage xgi = {
	0,0,			/* width,height -- to be filled in */
	0,			/* xoffset */
	ZPixmap,		/* format */
	NULL,			/* data */
	MSBFirst,		/* byte_order */
	8,			/* bitmap_unit */
	MSBFirst,		/* bitmap_bit_order */
	0,			/* bitmap_pad */
	8,			/* depth */
	0,			/* bytes_per_line -- to be filled in */
	8,			/* bits_per_pixel */
	0xe0,			/* red_mask */
	0x1c,			/* green_mask */
	0x03,			/* blue_mask */
	NULL,			/* obdata */
	{   NULL,		/* f.create_image */
	    NULL,		/* f.destroy_image */
	    NULL,		/* f.get_pixel */
	    NULL,		/* f.put_pixel */
	    NULL,		/* f.sub_image */
	    NULL,		/* f.add_pixel */
	},
    };
    static unsigned char *lgdata[MAX_GEARS] = {NULL},*sgdata[MAX_GEARS];
    int i,xg,yg,x2,y2,xt,yt;


    if (lgdata[0] == NULL) {
	for (i=0; i<MAX_GEARS; ++i) {
	    getPixmap(gear_filename[i],LARGEGEAR_WIDTH,LARGEGEAR_HEIGHT,
		&lgdata[i],&sgdata[i],True);
	}
	INITIALIZE_XIMAGE_PARAMETERS(xgi);
    }

    /* Now fill in the size-specific part of the XImage */
    if (xs.useSmallDash) {
	xg = SMALLDASH_WIDTH  - SMALLGEAR_WIDTH;
	yg = SMALLDASH_HEIGHT - SMALLGEAR_HEIGHT;
	xgi.width  = SMALLGEAR_WIDTH;
	xgi.height = SMALLGEAR_HEIGHT;
	xgi.data = (char *) sgdata[which];
    }
    else {
	xg = LARGEDASH_WIDTH  - LARGEGEAR_WIDTH;
	yg = LARGEDASH_HEIGHT - LARGEGEAR_HEIGHT;
	xgi.width  = LARGEGEAR_WIDTH;
	xgi.height = LARGEGEAR_HEIGHT;
	xgi.data = (char *) lgdata[which];
    }
    xgi.bytes_per_line = xgi.width * xgi.bits_per_pixel/8;

    x2 = x+width;
    y2 = y+height;
    if ((x2 > xg) && (y2 > yg)) {
	xt = MAX(x,xg);
	yt = MAX(y,yg);
	XPutImage(xs.display,xs.dashWindow,xs.dashGC, &xgi,
	    xt-xg,yt-yg, xt,yt, x2-xt,y2-yt);
    }
#endif
}


void redrawDash_xywh(
    int x, int y,
    int width, int height)
{
#if defined(MOTIF_GUI)
    static XImage xi = {
	0,0,			/* width,height -- to be filled in */
	0,			/* xoffset */
	ZPixmap,		/* format */
	NULL,			/* data */
	MSBFirst,		/* byte_order */
	8,			/* bitmap_unit */
	MSBFirst,		/* bitmap_bit_order */
	0,			/* bitmap_pad */
	8,			/* depth */
	0,			/* bytes_per_line -- to be filled in */
	8,			/* bits_per_pixel */
	0xe0,			/* red_mask */
	0x1c,			/* green_mask */
	0x03,			/* blue_mask */
	NULL,			/* obdata */
	{   NULL,		/* f.create_image */
	    NULL,		/* f.destroy_image */
	    NULL,		/* f.get_pixel */
	    NULL,		/* f.put_pixel */
	    NULL,		/* f.sub_image */
	    NULL,		/* f.add_pixel */
	},
    };
    static unsigned char *ldata = NULL,*sdata;
    int x1,y1,x2,y2,xt,yt,xx,yx,xg,yg;


    if (ldata == NULL) {
	getPixmap(dash_filename,LARGEDASH_WIDTH,LARGEDASH_HEIGHT,
	    &ldata,&sdata,True);
	INITIALIZE_XIMAGE_PARAMETERS(xi);
    }

    undrawWheel();

    /* Now fill in the size-specific part of the XImage */
    if (xs.useSmallDash) {
	xi.width  = SMALLDASH_WIDTH;
	xi.height = SMALLDASH_HEIGHT;
	xi.data = (char *) sdata;
	xg = SMALLDASH_WIDTH  - SMALLGEAR_WIDTH;
	yg = SMALLDASH_HEIGHT - SMALLGEAR_HEIGHT;
    }
    else {
	xi.width  = LARGEDASH_WIDTH;
	xi.height = LARGEDASH_HEIGHT;
	xi.data = (char *) ldata;
	xg = LARGEDASH_WIDTH  - LARGEGEAR_WIDTH;
	yg = LARGEDASH_HEIGHT - LARGEGEAR_HEIGHT;
    }
    xi.bytes_per_line = xi.width * xi.bits_per_pixel/8;

    /* Update the static dashboard display */
    if ((x < xg) || (y < yg)) {
	XPutImage(xs.display,xs.dashWindow,xs.dashGC, &xi,
	    x,y,x,y,width,height);
    }

    /* Update the moveable gear shift */
    drawGearshift(gear.actual,x,y,width,height);

    /* Redraw the compass */
    if (intersectRect(
            x,y, x+width,y+height,
            LARGECOMPASS_X1, LARGECOMPASS_Y1, LARGECOMPASS_X2, LARGECOMPASS_Y2,
            &x1,&y1,&x2,&y2)) {
	compass_draw();
    }


    /* If the redraw area includes the gauges and wheel, redraw those too. */
    if (intersectRect(x,y,x+width,y+width,
	    HUB_CENTER_X-WHEEL_RADIUS,HUB_CENTER_Y-WHEEL_RADIUS,
	    HUB_CENTER_X+WHEEL_RADIUS,HUB_CENTER_Y+WHEEL_RADIUS,
	    &xt,&yt,&xx,&yx)) {
	gauge_type *gauge;
	gear_type *gearptr;

	gauge = gauge_list;	
	while (gauge != NULL) {
	    gaugeDraw(gauge);
	    gauge = gauge->next;
	}

	gearptr = gear_list;
	while (gearptr != NULL) {
	    gearDraw(gearptr);
	    gearptr = gearptr->next;
	}
    }

    drawWheel(cstate.xpointer_value);
#endif
}


static void exposeDash(
    Widget whichW,
    void *client_data,
    XmDrawingAreaCallbackStruct *cb)
{
#if defined(MOTIF_GUI)
    XExposeEvent *event = (XExposeEvent *) (cb->event);

    redrawDash_xywh(event->x,event->y, event->width,event->height);
#endif
}


void redrawDash(
    void)
{
#if defined(MOTIF_GUI)
    Window root;
    unsigned int width,height;
    int junk;
    unsigned int ijunk;

    XGetGeometry(xs.display,xs.dashWindow,&root,&junk,&junk,
	    &width,&height,&ijunk,&ijunk);
    redrawDash_xywh(0,0,width,height);
#endif
}


#if defined(MOTIF_GUI)
static void resizeCallback(
    void)
{
    Window root;
    int junk;
    unsigned int width,height;
    int dash_width,dash_height;
    unsigned int ijunk;
    Arg arg[MAX_ARG];
    Cardinal ac;
    int x;
    Boolean newDashSize;

    XGetGeometry(xs.display,XtWindow(xs.app_shellW),&root,&junk,&junk,
	&width,&height,&ijunk,&ijunk);

    /* Make sure it's of at least a minimum size */
    if (width < WIN_MINWIDTH) {
	if (height < WIN_MINHEIGHT) {
	    XResizeWindow(xs.display,XtWindow(xs.app_shellW),
		WIN_MINWIDTH,WIN_MINHEIGHT);
	    return;
	}
	else {
	    XResizeWindow(xs.display,XtWindow(xs.app_shellW),
		WIN_MINWIDTH,height);
	    return;
	}
    }
    else if (height < WIN_MINHEIGHT) {
	XResizeWindow(xs.display,XtWindow(xs.app_shellW),
	    width,WIN_MINHEIGHT);
	return;
    }
	

    /* Should I use the small dash or the large? */
    newDashSize = False;
    if ((width >= LARGEDASH_MINTOTWIDTH)
	    && (height >= LARGEDASH_MINTOTHEIGHT)) {
	if (xs.useSmallDash) newDashSize = True;
	xs.useSmallDash = False;
	dash_width  = LARGEDASH_WIDTH;
	dash_height = LARGEDASH_HEIGHT;
    }
    else {
	if (!xs.useSmallDash) newDashSize = True;
	xs.useSmallDash = True;
	dash_width  = SMALLDASH_WIDTH;
	dash_height = SMALLDASH_HEIGHT;
    }

    /*** Resize main graphics window ***/
    ac = 0;
    XtSetArg(arg[ac],XmNwidth,  DRAW_WIDTH(width)); ++ac;
    XtSetArg(arg[ac],XmNheight, DRAW_HEIGHT(height,dash_height)); ++ac;
    xs.grHeight = DRAW_HEIGHT(height,dash_height);
    XtSetValues(graphicsW,arg,ac);
    cstate.gWinWidth  = DRAW_WIDTH(width);
    cstate.gWinHeight = DRAW_HEIGHT(height,dash_height);

    /*** Resize and reposition the dashboard window ***/
    ac = 0;
    XtSetArg(arg[ac],XmNwidth,dash_width); ++ac;
    XtSetArg(arg[ac],XmNheight,dash_height); ++ac;
    if ((x = (DRAW_WIDTH(width) - dash_width)/2) < 0) x = 0;
    XtSetArg(arg[ac],XmNleftOffset,x); ++ac;
    XtSetValues(dashW,arg,ac);

    /*** Resize the message window ***/
    ac = 0;
    XtSetArg(arg[ac],XmNwidth,width); ++ac;
    XtSetValues(messageW,arg,ac);

    /*** Now the Accel/Decel bar ***/
    ac = 0;
    XtSetArg(arg[ac],XmNheight,
	DRAW_HEIGHT(height,dash_height) - 2*ACCBRKBAR_LABELHEIGHT); ++ac;
    XtSetValues(accBrkW,arg,ac);

    /*** Redraw the dash window if it changed size ***/
    if (newDashSize) {
	redrawDash_xywh(0,0, dash_width,dash_height);
    }

    /*** Let the graphics module do whatever is necessary ***/
    (*graphics_resize_routine)(cstate.gWinWidth,cstate.gWinHeight);
}


static void starbaseWindowMapped(
    Widget whichW,
    XtPointer client_data,
    XEvent *event,
    Boolean *continue_to_dispatch)
{
    Arg arg[MAX_ARG];
    Cardinal ac;
    XWindowAttributes wattr;

    if (event->type != MapNotify) return;

    /* Check and see if the standard colormap has been overwritten by the
     * graphics core (like on Gecko, when a Color Recovery cmap is 
     * installed.  If so, change the values of the stdCmap.colormaps for
     * the graphics and dashboard window, so they both look the same */
    XGetWindowAttributes(xs.display,xs.graphicsWindow,&wattr);
    if(wattr.colormap != xs.graphicsInfo.stdCmap.colormap)
    {
	xs.graphicsInfo.stdCmap.colormap = wattr.colormap;
	xs.dashInfo.stdCmap.colormap = wattr.colormap;
    }

    if (fildes == INVALID) {
	ac = 0;
	XtSetArg(arg[ac],XgNfildes,&fildes); ++ac;
	XtGetValues(graphicsW,arg,ac);
	if (fildes != INVALID) {
	    XSetWindowColormap(xs.display,xs.graphicsWindow,
		xs.graphicsInfo.stdCmap.colormap);
	    XSetWindowColormap(xs.display,xs.dashWindow,
		xs.graphicsInfo.stdCmap.colormap);
	    /* Don't do this again -- it's unnecessary */
	    XtRemoveEventHandler(whichW,StructureNotifyMask,
		False,starbaseWindowMapped,NULL);
	}
    }
}

static void exposeAccBrk(
    Widget whichW,
    void *client_data,
    XmDrawingAreaCallbackStruct *cb)
{
    redrawAccBrk();
}


static void enter(
    Widget widget,
    XtPointer client_data,
    XEvent *event,
    Boolean *continue_to_dispatch)
{
    Arg arg[MAX_ARG];
    int i,n;
    Colormap *installed;


    installed = XListInstalledColormaps(xs.display,XtWindow(widget),&n);
    for (i=0; i<n; ++i) {
	if (XtWindow(widget) == xs.graphicsWindow) {
	    if (installed[i] == xs.graphicsInfo.stdCmap.colormap) {
		/* already installed -- no need for further action */
		XFree((char *) installed);
		return;
	    }
	}
	else {
	    if (installed[i] == xs.dashInfo.stdCmap.colormap) {
		/* already installed -- no need for further action */
		XFree((char *) installed);
		return;
	    }
	}
    }
    XFree((char *) installed);

    n = 0;
    XtSetArg(arg[n], XgNwmCmap, XgWM_CMAP_HIGH_PRIORITY); n++;
    XtSetValues(widget, arg, n);
}


static void leave(
    Widget widget,
    XtPointer client_data,
    XEvent *event,
    Boolean *continue_to_dispatch)
{
#ifdef OLD_WAY
    Arg arg[MAX_ARG];
    int n;

    n = 0;
    XtSetArg(arg[n], XgNwmCmap, XgWM_CMAP_LOW_PRIORITY); n++;
    XtSetValues(widget, arg, n);

#endif
    Arg arg[MAX_ARG];
    int i,n;
    Colormap *installed;


    installed = XListInstalledColormaps(xs.display,XtWindow(widget),&n);
    for (i=0; i<n; ++i) {
	if (XtWindow(widget) == xs.graphicsWindow) {
	    if (installed[i] == xs.graphicsInfo.stdCmap.colormap) {
		/* already installed -- no need for further action */
		XFree((char *) installed);
		return;
	    }
	}
	else {
	    if (installed[i] == xs.dashInfo.stdCmap.colormap) {
		/* already installed -- no need for further action */
		XFree((char *) installed);
		return;
	    }
	}
    }
    XFree((char *) installed);

    n = 0;
    XtSetArg(arg[n], XgNwmCmap, XgWM_CMAP_LOW_PRIORITY); n++;
    XtSetValues(widget, arg, n);
}
#endif

void handleStandardInput(
    XEvent *event)
{
#if defined(MOTIF_GUI)
    KeySym ks;

    if (event->type == KeyPress) {
	ks = XKeycodeToKeysym(xs.display,event->xkey.keycode,0);
	process_keypress(ks,event->xkey.state);
    }
    else if (event->type == ButtonPress) {
	process_button(event->xbutton.button);
    }
#endif
}


#if defined(MOTIF_GUI)
/* Called on any input to graphics windows */
static void standardInputCallback(
    Widget whichW,
    caddr_t data,
    XmDrawingAreaCallbackStruct *cb)
{
    handleStandardInput(cb->event);
}


static void create_all_windows(
    int argc, char *argv[],
    int total_width, int total_height,
    Boolean (*select_visuals_routine)
	    (XVisualInfo *vdash, XVisualInfo *vgraphics))
{
    XmString xmstr;
    XFontStruct *xfs;
    Arg arg[MAX_ARG];
    Cardinal ac;
    int dash_width,dash_height;
    Widget formW,drawFrameW,topLabelW,bottomLabelW;
    int x;


    if (argv[0][0] == '/') strcpy(myPath,argv[0]);
    else sprintf(myPath,"./%s",argv[0]);
    xs._current_fg = 0xffffffff;

    initGaugeModule();

#define USE_APP 1
#ifdef USE_APP
#if XtSpecificationRelease == 4
    xs.app_shellW = XtAppInitialize(&xs.app_context,CLASS,
	NULL,0,
	(Cardinal *) &argc,argv,
	NULL,NULL,0);
#else
    xs.app_shellW = XtAppInitialize(&xs.app_context,CLASS,
	NULL,0,
	&argc,argv,
	NULL,NULL,0);
#endif /* X11R4 */
#else
    xs.app_shellW = XtInitialize(CLASS,CLASS,NULL,0,&argc,argv);
#endif

    if (xs.app_shellW == NULL) {
	fprintf(stderr,"Could not open Motif1.2 X11R5 Windows on display!!\n");
	exit(1);
    }

    xs.display   = XtDisplay(xs.app_shellW);
    xs.screen    = XtScreen(xs.app_shellW);
    xs.screenNum = screenNumOfScreen(xs.display,xs.screen);
    set_vendor_flag(ServerVendor(xs.display));
printf("xs.display = %08x\n", xs.display); fflush(stdout);

    if( getenv("DO_SYNC") != NULL)
	    XSynchronize(xs.display,True);

    if (debug) {
	printf("Main shell widget opened.\n");
	printf("%s Server, Release X11R%d\n",ServerVendor(xs.display),VendorRelease(xs.display));
	printf("Screen %d, size %dx%dx%d\n",XScreenNumberOfScreen(xs.screen),WidthOfScreen(xs.screen),
		HeightOfScreen(xs.screen),PlanesOfScreen(xs.screen));
	printf("Colormaps: %d-%d\n",MinCmapsOfScreen(xs.screen),MaxCmapsOfScreen(xs.screen));
	printf("\n");
    }

    /* If small display, use smaller window */
    if ((WidthOfScreen(xs.screen) < 1150)
	    || (HeightOfScreen(xs.screen) < 1024)) {
	total_width  = WIN_SMALLWIDTH;
	total_height = WIN_SMALLHEIGHT;
    }

#if 0
    if (!(*select_visuals_routine)(&(xs.dashInfo.visualInfo),
		&(xs.graphicsInfo.visualInfo))) {
	fprintf(stderr, "Cannot find a good double-buffered visual.\n" );
	fprintf(stderr, "Aborting.\n");
	exit(1);
    }
#endif

    if (debug)  {
	printf("Using the following visual:\n");
	PRINT_VISUAL(&(xs.dashInfo.visualInfo));
    }

    get_standard_colormap(&(xs.dashInfo.visualInfo),&(xs.dashInfo.stdCmap),
	&(xs.dashInfo.cmap_size), xs.dashInfo.map332ToPixel);
    if (xs.dashInfo.visualInfo.visualid == xs.graphicsInfo.visualInfo.visualid) {
	memcpy(&(xs.graphicsInfo),&(xs.dashInfo),sizeof(VISUAL_AND_CMAP_INFO));
    }
    else {
	get_standard_colormap(&(xs.graphicsInfo.visualInfo),&(xs.graphicsInfo.stdCmap),
	    &(xs.graphicsInfo.cmap_size), xs.graphicsInfo.map332ToPixel);
    }
    if (debug) {
	printf("Standard colormap: \n");
	PRINT_STDCMAP(&(xs.dashInfo.stdCmap));
    }

    /* Get some colors */
    xs.black	= xs.dashInfo.map332ToPixel[BLACK332];
    xs.white	= xs.dashInfo.map332ToPixel[WHITE332];
    xs.red	= xs.dashInfo.map332ToPixel[RED332];
    xs.green	= xs.dashInfo.map332ToPixel[GREEN332];
    xs.blue	= xs.dashInfo.map332ToPixel[BLUE332];
    xs.cyan	= xs.dashInfo.map332ToPixel[CYAN332];
    xs.gray	= xs.dashInfo.map332ToPixel[GRAY332];
    xs.background = xs.dashInfo.map332ToPixel[BACKGROUND332];

    /*** This widget just for resize detection -- it's never mapped ***/
    ac = 0;
    XtSetArg(arg[ac],XmNwidth,  total_width); ++ac;
    XtSetArg(arg[ac],XmNheight, total_height); ++ac;
    XtSetArg(arg[ac],XmNmappedWhenManaged, False); ++ac;
    XtSetArg(arg[ac],XmNvisual,xs.dashInfo.visualInfo.visual); ++ac;
#if !defined(AVOID_PASSING_COLORMAP)
    XtSetArg(arg[ac],XmNcolormap,xs.dashInfo.stdCmap.colormap); ++ac;
#endif
    XtSetArg(arg[ac],XmNforeground,xs.white); ++ac;
    XtSetArg(arg[ac],XmNbackground,xs.black); ++ac;
    drawFrameW = XgCreateVisualDrawingArea(xs.app_shellW,"main",arg,ac);
    XtAddCallback(drawFrameW,XmNresizeCallback,
	(XtCallbackProc) resizeCallback,NULL);
    XtManageChild(drawFrameW);

    /*** The title window ***/
    createTitleWindow(xs.app_shellW,total_width,total_height);

    /*** The visual controls help window ***/
    createVisCtrlsWindow(xs.app_shellW);

    /*** The main form ***/
    ac = 0;
    XtSetArg(arg[ac],XmNwidth,  total_width); ++ac;
    XtSetArg(arg[ac],XmNfractionBase, total_width); ++ac;
    XtSetArg(arg[ac],XmNheight, total_height); ++ac;
    XtSetArg(arg[ac],XmNallowOverlap, False); ++ac;
    XtSetArg(arg[ac],XmNresizable, False); ++ac;
    XtSetArg(arg[ac],XmNrubberPositioning, True); ++ac;
    formW = XmCreateForm(xs.app_shellW,"main",arg,ac);
    XtManageChild(formW);

    /* Should I use the small dash or the large? */
    if ((total_width >= LARGEDASH_MINTOTWIDTH)
	    && (total_height >= LARGEDASH_MINTOTHEIGHT)) {
	xs.useSmallDash = False;
	dash_width   = LARGEDASH_WIDTH;
	dash_height  = LARGEDASH_HEIGHT;
    }
    else {
	xs.useSmallDash = True;
	dash_width   = SMALLDASH_WIDTH;
	dash_height  = SMALLDASH_HEIGHT;
    }

    /*** Create main graphics window ***/
    ac = 0;
    XtSetArg(arg[ac],XmNwidth,  DRAW_WIDTH(total_width)); ++ac;
    XtSetArg(arg[ac],XmNheight, DRAW_HEIGHT(total_height,dash_height)); ++ac;
    xs.grHeight = DRAW_HEIGHT(total_height,dash_height);
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_FORM); ++ac;
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_FORM); ++ac;
    XtSetArg(arg[ac],XmNvisual,xs.graphicsInfo.visualInfo.visual); ++ac;
#if !defined(AVOID_PASSING_COLORMAP)
    XtSetArg(arg[ac],XmNcolormap,xs.graphicsInfo.stdCmap.colormap); ++ac;
    XtSetArg(arg[ac],XgNwmCmap,XgWM_CMAP_HIGH_PRIORITY); ++ac;
#endif
    XtSetArg(arg[ac],XmNforeground,xs.graphicsInfo.map332ToPixel[WHITE332]); ++ac;
    XtSetArg(arg[ac],XmNbackground,xs.graphicsInfo.map332ToPixel[BLACK332]); ++ac;
    XtSetArg(arg[ac],XgNrescalePolicy,XgRESCALE_DISTORT); ++ac;
    graphicsW = XgCreateVisualDrawingArea(formW,"main",arg,ac);
    XtAddEventHandler(graphicsW, StructureNotifyMask, False,
	starbaseWindowMapped,NULL);


    /* Add the following event handlers to kick vuewm in the head and 
     * make it install our colormaps!
     */
    XtAddEventHandler(graphicsW, EnterWindowMask, False, enter, NULL);
    XtAddEventHandler(graphicsW, LeaveWindowMask, False, leave, NULL);
    XtManageChild(graphicsW);
    XtAddCallback(graphicsW,XmNinputCallback,
	(XtCallbackProc) standardInputCallback,NULL);

    /*** Create the dashboard window ***/
    ac = 0;
    XtSetArg(arg[ac],XmNwidth,dash_width); ++ac;
    XtSetArg(arg[ac],XmNheight,dash_height); ++ac;
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNtopWidget,graphicsW); ++ac;
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_FORM); ++ac;
    if ((x = (DRAW_WIDTH(total_width) - dash_width)/2) < 0) x = 0;
    XtSetArg(arg[ac],XmNleftOffset,x); ++ac;
#if !defined(AVOID_PASSING_VISUAL)
    XtSetArg(arg[ac],XgNvisual,xs.dashInfo.visualInfo.visual); ++ac;
#endif
#if !defined(AVOID_PASSING_COLORMAP)
    XtSetArg(arg[ac],XmNcolormap,xs.dashInfo.stdCmap.colormap); ++ac;
#endif
    XtSetArg(arg[ac],XmNforeground,xs.white); ++ac;
    XtSetArg(arg[ac],XmNbackground,xs.black); ++ac;
    dashW = XgCreateVisualDrawingArea(formW,"dash",arg,ac);
    XtAddCallback(dashW,XmNexposeCallback,
	(XtCallbackProc) exposeDash,NULL);
    /* Add the following event handlers to kick vuewm in the head and 
     * make it install our colormaps!
     */
    XtAddEventHandler(dashW, EnterWindowMask, False, enter, NULL);
    XtAddEventHandler(dashW, LeaveWindowMask, False, leave, NULL);
    XtAddCallback(dashW,XmNinputCallback,
	(XtCallbackProc) standardInputCallback,NULL);
    XtManageChild(dashW);

    /*** And the message window ***/
    ac = 0;
    XtSetArg(arg[ac],XmNwidth,total_width); ++ac;
    XtSetArg(arg[ac],XmNheight,MESSAGEWIN_HEIGHT); ++ac;
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNtopWidget,dashW); ++ac;
    XtSetArg(arg[ac],XmNtopOffset,GUTTER); ++ac;
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_FORM); ++ac;
    XtSetArg(arg[ac],XmNresizePolicy,XmRESIZE_ANY); ++ac;
#if !defined(AVOID_PASSING_VISUAL)
    XtSetArg(arg[ac],XgNvisual,xs.dashInfo.visualInfo.visual); ++ac;
#endif
#if !defined(AVOID_PASSING_COLORMAP)
    XtSetArg(arg[ac],XmNcolormap,xs.dashInfo.stdCmap.colormap); ++ac;
#endif
    XtSetArg(arg[ac],XmNforeground,xs.white); ++ac;
    XtSetArg(arg[ac],XmNbackground,xs.black); ++ac;
    messageW = XgCreateVisualDrawingArea(formW,"message",arg,ac);
    XtAddCallback(messageW,XmNexposeCallback,
	(XtCallbackProc) exposeTextWindow,NULL);
    XtAddCallback(messageW,XmNinputCallback,
	(XtCallbackProc) standardInputCallback,NULL);
    XtManageChild(messageW);

    /*** The menu bar -- at the bottom to prevent careening off the road ***/
    ac = 0;
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNtopWidget,messageW); ++ac;
    XtSetArg(arg[ac],XmNtopOffset,GUTTER); ++ac;
    menuBarW = create_menu_bar(formW,arg,ac);

    /*** Now the Accel/Decel bar ***/
    ac = 0;
    XtSetArg(arg[ac],XmNwidth,ACCBRKBAR_WIDTH); ++ac;
    XtSetArg(arg[ac],XmNheight,ACCBRKBAR_LABELHEIGHT); ++ac;
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_OPPOSITE_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNtopWidget,graphicsW); ++ac;
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNleftWidget,graphicsW); ++ac;
    XtSetArg(arg[ac],XmNleftOffset,GUTTER); ++ac;
    XtSetArg(arg[ac],XmNrightAttachment,XmATTACH_FORM); ++ac;
    XtSetArg(arg[ac],XmNrightOffset,GUTTER); ++ac;
    xmstr = XmStringCreateLtoR("ACC",XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNlabelString,xmstr); ++ac;
    topLabelW = XmCreateLabelGadget(formW,"label",arg,ac);
    XtManageChild(topLabelW);
    XmStringFree(xmstr);

    ac = 0;
    XtSetArg(arg[ac],XmNwidth,ACCBRKBAR_WIDTH); ++ac;
    XtSetArg(arg[ac],XmNheight,ACCBRKBAR_LABELHEIGHT); ++ac;
    XtSetArg(arg[ac],XmNbottomAttachment,XmATTACH_OPPOSITE_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNbottomWidget,graphicsW); ++ac;
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNleftWidget,graphicsW); ++ac;
    XtSetArg(arg[ac],XmNleftOffset,GUTTER); ++ac;
    XtSetArg(arg[ac],XmNrightAttachment,XmATTACH_FORM); ++ac;
    XtSetArg(arg[ac],XmNrightOffset,GUTTER); ++ac;
    xmstr = XmStringCreateLtoR("BRK",XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNlabelString,xmstr); ++ac;
    bottomLabelW = XmCreateLabelGadget(formW,"label",arg,ac);
    XtManageChild(bottomLabelW);
    XmStringFree(xmstr);

    ac = 0;
    XtSetArg(arg[ac],XmNwidth,ACCBRKBAR_WIDTH); ++ac;
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNtopWidget,topLabelW); ++ac;
    XtSetArg(arg[ac],XmNbottomAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNbottomWidget,bottomLabelW); ++ac;
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNleftWidget,graphicsW); ++ac;
    XtSetArg(arg[ac],XmNleftOffset,GUTTER); ++ac;
    XtSetArg(arg[ac],XmNrightAttachment,XmATTACH_FORM); ++ac;
    XtSetArg(arg[ac],XmNrightOffset,GUTTER); ++ac;
#if !defined(AVOID_PASSING_VISUAL)
    XtSetArg(arg[ac],XgNvisual,xs.dashInfo.visualInfo.visual); ++ac;
#endif
#if !defined(AVOID_PASSING_COLORMAP)
    XtSetArg(arg[ac],XmNcolormap,xs.dashInfo.stdCmap.colormap); ++ac;
#endif
    XtSetArg(arg[ac],XmNforeground,xs.white); ++ac;
    XtSetArg(arg[ac],XmNbackground,xs.black); ++ac;
    accBrkW = XgCreateVisualDrawingArea(formW,"misc",arg,ac);
    XtAddCallback(accBrkW,XmNexposeCallback,
	(XtCallbackProc) exposeAccBrk,NULL);
    XtAddCallback(accBrkW,XmNinputCallback,
	(XtCallbackProc) standardInputCallback,NULL);
    XtManageChild(accBrkW);

    /************************************************************************/

    XtRealizeWidget(xs.app_shellW);
    XSync(xs.display,False);

    /*** Get stuff for redrawing various windows ***/

    /* Accelerate/brake */
    xs.abWindow = XtWindow(accBrkW);
    xs.abGC = XCreateGC(xs.display,xs.abWindow,0,NULL);

    /* Dash */
    xs.dashWindow = XtWindow(dashW);
    xs.dashGC = XCreateGC(xs.display,xs.dashWindow,0,NULL);
    xs.gearFontStruct = getFont("gearFont","fixed");
    xs.gearFont = xs.gearFontStruct->fid;

    /* The text window */
    xs.textWindow = XtWindow(messageW);
    xs.textGC = XCreateGC(xs.display,xs.textWindow,0,NULL);
    xfs = getFont("font","8x13");
    xs.textDfont = xfs->fid;
    xfs = getFont("bigFont","vgl-40");
    xs.textBfont = xfs->fid;

    /* Main window */
    xs.mainWindow = XtWindow(formW);
    xs.crosshair =  XCreateFontCursor(xs.display, XC_crosshair);

    /* Copy some stuff to cstate. */
    cstate.display = xs.display;
    cstate.gWinWidth  = DRAW_WIDTH(total_width);
    cstate.gWinHeight = DRAW_HEIGHT(total_height,dash_height);
    cstate.graphicsWindow = xs.graphicsWindow = XtWindow(graphicsW);
    xs.grGC = XCreateGC(xs.display,xs.graphicsWindow,0,NULL);

    {
    XWMHints wmhints;
    wmhints.window_group = XtWindow(xs.app_shellW);
    wmhints.flags = WindowGroupHint;
    XSetWMHints(xs.display,XtWindow(xs.app_shellW),&wmhints);
    }

    /* Create auxiliary windows */
    createRadarWindow((Position) (total_width + GUTTER*4), GUTTER);
    if (cstate.mode & CLIENT_INITIAL_RADAR) toggleRadarCallback();

    if( cstate.mode & CLIENT_DUAL_RADAR_MODE) {
	createRaceWindow((Position) (total_width + GUTTER*4),
	    (Position) (DEFAULT_RADAR_SIZE *2 +100+GUTTER));
    }
    else {
	createRaceWindow((Position) (total_width + GUTTER*4),
	    (Position) (DEFAULT_RADAR_SIZE+100+GUTTER));
    }
    if (cstate.mode & CLIENT_INITIAL_STANDINGS) toggleLeaderCallback();
}
#endif


/* Returns fildes of graphics window */
int createGUI(
    int argc,
    char *argv[],
    Boolean forceSmall,
    void (*resize_routine)
	(int width, int height),
    Boolean (*select_visuals_routine)
	    (XVisualInfo *vdash, XVisualInfo *vgraphics))
{
#if defined(MOTIF_GUI)
    XVisualInfo
	*hwVisual;

    fildes = INVALID;
    graphics_resize_routine = resize_routine;

    /* HACK! */
    if (!hwInit(argc, argv)) exit(1);
    disp = hwDefaultDisplay->create( hwDefaultDisplay, NULL, xs.display );
    if( !disp ) exit( 1 );

    /* TBD: Visual selection */
    (void)disp->chooseVisual( disp, HW_VIS_DEPTH | HW_VIS_DBUFF, 0 );

    if ((hwVisual = (XVisualInfo *)disp->extractVisual(disp)) == NULL) {
        fprintf(stderr,"Hoverware visual extraction failed.\n");
	exit(1);
    }
    memcpy(&xs.dashInfo.visualInfo, hwVisual, sizeof(XVisualInfo));
    memcpy(&xs.graphicsInfo.visualInfo, hwVisual, sizeof(XVisualInfo));


    if (forceSmall) {
	xs.useSmallDash = True;
	create_all_windows(argc,argv,WIN_SMALLWIDTH,WIN_SMALLHEIGHT,
	    select_visuals_routine);
    }
    else {
	xs.useSmallDash = False;
	create_all_windows(argc,argv,WIN_LARGEWIDTH,WIN_LARGEHEIGHT,
	    select_visuals_routine);
    }

    /* Wait for the image fildes to get assigned */
    XFlush(xs.display);
    fildes = 100;
    processXEvents();

#if 0
    while (fildes == INVALID) {
	processXEvents();
    }
#endif

    draw = disp->initDrawable( disp,
			(OS_DRAWABLE_TYPE)xs.graphicsWindow );
    if( !draw ) exit( 1 );
    disp->makeCurrent( disp, draw );
    disp->renderMode(disp, HW_RENDER_DEFAULT | HW_RENDER_CULL_FACE);

    return(fildes);
#else
    /* HACK! */
    cstate.display = XOpenDisplay(NULL);

    hwInit( argc, argv );
    disp = hwDefaultDisplay->create(hwDefaultDisplay, NULL, cstate.display);
    if( !disp ) exit( 1 );

    if (!disp->chooseVisual( disp, HW_VIS_DEPTH | HW_VIS_DBUFF, 0 )) {
	exit(1);
    }
    draw = disp->createWindow( disp, "Drive", 100, 100,
    		WIN_SMALLWIDTH, WIN_SMALLHEIGHT,
		HW_WIN_INPUT);
    if( !draw ) exit( 1 );
    disp->makeCurrent( disp, draw );
    disp->renderMode(disp, HW_RENDER_DEFAULT | HW_RENDER_CULL_FACE);
    cstate.graphicsWindow = disp->extractWindow( disp, draw );
    cstate.gWinWidth = WIN_SMALLWIDTH;
    cstate.gWinHeight = WIN_SMALLHEIGHT;
    return 1;
#endif
}
