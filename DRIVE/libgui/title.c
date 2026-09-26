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
#include <Xm/Xm.h>
#include "visDrawArea.h"
#endif

#include "filenames.h"
#include "global.h"
#include "drive.h"
#include "gui.h"
#include "message.h"
#include "windows.h"

#define TITLEFONT_LARGE		"vrb-30"
#define TITLEFONT_SMALL		"fgb-13"
#define TITLE_WIDTH		800
#define TITLE_HEIGHT		640
#define TITLE_WINWIDTH		TITLE_WIDTH
#define TITLE_WINHEIGHT		780

#if defined(MOTIF_GUI)
static Widget titleW = NULL;
static int offsetX,offsetY;
static unsigned char *ldata = NULL, *sdata;
static XFontStruct *titleFontStruct;


static void redrawTitleText(
    Window win,
    GC gc,
    int xcenter, int y)
{
    char str[256];
    int dir,asc,desc,fullwidth,namestart;
    XCharStruct overall;
    XTextItem xt;


    XSetForeground(xs.display,gc,xs.white);

    xt.delta = 0;
    xt.font = titleFontStruct->fid;

    /* overlaps into text -- redraw that */
    sprintf(str,"Developed by:  ");
    XQueryTextExtents(xs.display,xt.font,str,strlen(str),
	&dir,&asc,&desc,&overall);
    namestart = overall.width;

    sprintf(str,"Developed by:  Norman Gee");
    XQueryTextExtents(xs.display,xt.font,str,strlen(str),
	&dir,&asc,&desc,&overall);
    fullwidth = overall.width;

    y += (asc+desc);

    xt.chars  = "Developed by:  Mike Banks";
    xt.nchars = strlen(xt.chars);
    XDrawText(xs.display,win,gc,xcenter-fullwidth/2,y,&xt,1);
    y += (asc+desc+4);

    xt.chars  = "Norman Gee";
    xt.nchars = strlen(xt.chars);
    XDrawText(xs.display,win,gc,xcenter-fullwidth/2+namestart,y,&xt,1);
    y += (asc+desc+4);

    xt.chars  = "Daryl Poe";
    xt.nchars = strlen(xt.chars);
    XDrawText(xs.display,win,gc,xcenter-fullwidth/2+namestart,y,&xt,1);
    y += (asc+desc+4);

    xt.chars  = "Ross Cunniff";
    xt.nchars = strlen(xt.chars);
    XDrawText(xs.display,win,gc,xcenter-fullwidth/2+namestart,y,&xt,1);
    y += (asc+desc)*2;

    xt.chars = "HP Graphics Software Lab";
    xt.nchars = strlen(xt.chars);
    XQueryTextExtents(xs.display,xt.font,xt.chars,xt.nchars,
	&dir,&asc,&desc,&overall);
    XDrawText(xs.display,win,gc,xcenter-overall.width/2,y,&xt,1);
}



static void redrawTitle(
    int x, int y,
    int width, int height)
{
    static XImage xi = {
	TITLE_WIDTH,TITLE_HEIGHT,	/* width,height -- TO BE FILLED IN */
	0,			/* xoffset */
	ZPixmap,		/* format */
	NULL,			/* data -- TO BE FILLED IN */
	MSBFirst,		/* byte_order */
	8,			/* bitmap_unit */
	MSBFirst,		/* bitmap_bit_order */
	0,			/* bitmap_pad */
	8,			/* depth */
	TITLE_WIDTH,		/* bytes_per_line -- TO BE FILLED IN */
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
    static Window titleWindow = 0;
    static GC titleGC;

    if (titleWindow == 0) {
	titleWindow = XtWindow(titleW);
	/* install_standard_colormap(titleWindow); */
	titleGC = XCreateGC(xs.display,titleWindow,0,NULL);
    }

    if (ldata == NULL) {
	/* Don't downsample if unnecessary. */
	if (xs.useSmallDash) {
	    getPixmap(title_filename,TITLE_WIDTH,TITLE_HEIGHT,
		&ldata,&sdata,True);
	}
	else {
	    getPixmap(title_filename,TITLE_WIDTH,TITLE_HEIGHT,
		&ldata,NULL,False);
	}
	INITIALIZE_XIMAGE_PARAMETERS(xi);
    }

    /* clear it */
    XSetForeground(xs.display,titleGC,xs.gray);
    XFillRectangle(xs.display,titleWindow,titleGC,x,y,width,height);

    /* Offset to center */
    if (x < offsetX) {
	width -= (offsetX - x);
	x = offsetX;
    }

    if (y < offsetY) {
	height -= (offsetY - y);
	y = offsetY;
    }

    /* Use small or large version? */
    if (xs.useSmallDash) {
	xi.data   = (char *) sdata;
	xi.width  = TITLE_WIDTH/2;
	xi.height = TITLE_HEIGHT/2;
    }
    else {
	xi.data   = (char *) ldata;
	xi.width  = TITLE_WIDTH;
	xi.height = TITLE_HEIGHT;
    }
    xi.bytes_per_line = xi.width * xi.bits_per_pixel/8;

    /* Update the static title display */
    XPutImage(xs.display,titleWindow,titleGC, &xi,
	x-offsetX,y-offsetY,x,y,width,height);

    if (y + height > xi.height+offsetY) {
	redrawTitleText(titleWindow,titleGC,xi.width/2+offsetX,
	    xi.height+offsetY+10);
    }
}


static void exposeTitle(
    Widget whichW,
    void *client_data,
    XmDrawingAreaCallbackStruct *cb)
{
    XExposeEvent *event = (XExposeEvent *) (cb->event);

    redrawTitle(event->x,event->y, event->width,event->height);
}


void createTitleWindow(
    Widget parent,
    int width,int height)
{
    Arg arg[MAX_ARG];
    Cardinal ac;
    int twidth;

    ac = 0;
    XtSetArg(arg[ac],XmNwidth,width); ++ac;
    XtSetArg(arg[ac],XmNheight,height); ++ac;
    XtSetArg(arg[ac],XgNvisual,xs.dashInfo.visualInfo.visual); ++ac;
    XtSetArg(arg[ac],XmNcolormap,xs.dashInfo.stdCmap.colormap); ++ac;
    XtSetArg(arg[ac],XmNforeground,xs.white); ++ac;
    XtSetArg(arg[ac],XmNbackground,xs.gray); ++ac;
    titleW = XgCreateVisualDrawingArea(parent,"title",arg,ac);
    XtAddCallback(titleW,XmNexposeCallback,
	(XtCallbackProc) exposeTitle,NULL);
    XtManageChild(titleW);

    if (xs.useSmallDash) {
	twidth = TITLE_WIDTH/2;
	titleFontStruct = getFont("titleFont", TITLEFONT_SMALL);
    }
    else {
	twidth = TITLE_WIDTH;
	titleFontStruct = getFont("titleFont", TITLEFONT_LARGE);
    }

    if (width > twidth) {
	offsetX = (width - twidth)/2;
    }
    else {
	offsetX = 0;
    }

    offsetY = 0;
}
#endif


void mapTitleWindow(
    void)
{
#if defined(MOTIF_GUI)
    /* Map the window */
    if (titleW != NULL) {
	XRaiseWindow(xs.display,XtWindow(titleW));
	XmUpdateDisplay(titleW);
    }
#endif
}


void unmapTitleWindow(
    void)
{
#if defined(MOTIF_GUI)
    /* Unmap and destroy the window */
    if (titleW != NULL) {
	XtDestroyWidget(titleW);
	titleW = NULL;
	if (ldata != NULL) {
	    free(ldata); ldata = NULL;
	    if (sdata) free(sdata);
	}
    }
#endif
}
