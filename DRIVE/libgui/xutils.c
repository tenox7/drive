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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/Xos.h>
#include <X11/keysym.h>

#include <Xm/Xm.h>
#include <Xm/PushBG.h>

#include "global.h"
#include "drive.h"
#include "windows.h"
#include "gui.h"


#define READ_ACCESS	04
#define ACCESS_ERROR	(-1)


void set_vendor_flag(
    char *vendor_string)
{
    typedef struct {
	char vendorname[64];
	unsigned int flag;
    } VENDOR;
    static VENDOR vendor[] = {
	{ "Hewlett-Packard",				VENDOR_HP },
	{ "International Business Machines",		VENDOR_IBM },
	{ "Oki Electric",				VENDOR_OKI },
	{ "X11/NeWs - Sun",				VENDOR_SUN },
	{ "Network Computing Devices",			VENDOR_NCD },
	{ "Tektronix",					VENDOR_TEKTRONIX },
	{ "Digital",					VENDOR_DEC },
	{ "Sho",					VENDOR_SHOGRAPHICS },
    };
#   define NUM_VENDORS (sizeof(vendor)/sizeof(VENDOR))
    int i;

    cstate.vendor = VENDOR_UNKNOWN;
    for (i=0; i<NUM_VENDORS; ++i) {
	if (strncmp(vendor_string,vendor[i].vendorname,
		strlen(vendor[i].vendorname)) == 0) {
	    cstate.vendor = vendor[i].flag;
	    break;
	}
    }
}


/* Given a display and screen pointer, give me a screen number */
int screenNumOfScreen(
    Display *display,
    Screen *screen)
{
    int i,maxscreen;

    maxscreen = ScreenCount(display);
    for (i=0; i<maxscreen; ++i) {
	if (RootWindowOfScreen(ScreenOfDisplay(display,i))
		== RootWindowOfScreen(screen)) {
	    return(i);
	}
    }

    /* else didn't find it */
    return(DefaultScreen(display));
}



Widget createButton(
    Widget parent,
    char *label,
    char mnemonic,
    XtCallbackProc activateCallback, caddr_t callbackData)
{
    Cardinal ac;
    Arg arg[MAX_ARG];
    XmString xmstr;
    Widget btnW;

    ac = 0;
    xmstr = XmStringCreateLtoR(label,XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNlabelString,xmstr); ++ac;
    XtSetArg(arg[ac],XmNalignment,XmALIGNMENT_CENTER); ++ac;
    if ((mnemonic != ' ') && (mnemonic != '\0')) {
	    XtSetArg(arg[ac],XmNmnemonic,mnemonic); ++ac;
    }
    btnW = XmCreatePushButtonGadget(parent,"btn",arg,ac);
    if (activateCallback != NULL) {
	XtAddCallback(btnW,XmNactivateCallback,
	    activateCallback,callbackData);
    }
    XmStringFree(xmstr);
    XtManageChild(btnW);

    return(btnW);
}


Boolean intersectRect(
    int x1min, int y1min, int x1max, int y1max,	/* from server */
    int x2min, int y2min, int x2max, int y2max,	/* constants, LARGE assumed */
    int *x3min, int *y3min, int *x3max, int *y3max)	/* result */
{
    if (xs.useSmallDash) {
	x2min /= 2.0;
	y2min /= 2.0;
	x2max /= 2.0;
	y2max /= 2.0;
    }

    *x3min = MAX(x1min,x2min); *x3max = MIN(x1max,x2max);
    if (*x3max <= *x3min) return(False);

    *y3min = MAX(y1min,y2min); *y3max = MIN(y1max,y2max);
    if (*y3max <= *y3min) return(False);

    return(True);
}


XFontStruct *getFont(
    char *option,
    char *default_font)
{
    char *fontname;
    XFontStruct *xfs;

    if ((option == NULL) || (*option == '\0')) {
	fontname = default_font;
    }
    else if ((fontname = XGetDefault(xs.display,PROGRAM,option)) == NULL) {
	fontname = default_font;
    }

    if ((xfs = XLoadQueryFont(xs.display,fontname)) == NULL) {
	/* Load a default */
	if ((xfs = XLoadQueryFont(xs.display,"fixed")) == NULL) {
	    fprintf(stderr,"Cannot load font %s!!!\n",fontname);
	    exit(1);
	}
    }

    return(xfs);
}


void XmString_to_string(
    XmString xmstr,
    char *str)
{
    XmStringContext scontext;
    char *text;
    XmStringCharSet charset;
    XmStringDirection direction;
    Boolean seperator;

    XmStringInitContext(&scontext,xmstr);
    XmStringGetNextSegment(scontext,&text,&charset,&direction,&seperator);
    strcpy(str,text);
    XtFree(text);
    XtFree((char *) charset);
    XmStringFreeContext(scontext);
}


void destroyWidgetCallback(
    Widget parentW,
    Widget whichW,
    XmAnyCallbackStruct *cb)
{
    XtDestroyWidget(whichW);
}

#endif
