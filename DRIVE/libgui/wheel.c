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

#if defined(MOTIF_GUI)
#include <X11/X.h>
#include <X11/Xlib.h>
#include <Xm/Xm.h>
#endif

#include "global.h"
#include "drive.h"
#include "gui.h"
#include "windows.h"

#define SPOKES		3
#define SPOKE_HOLES	2
/* Assumes large dash.  Divide by two for small */
#define SPOKE_WIDTH1	60	/* At hub */
#define SPOKE_WIDTH2	30	/* At wheel */

#if defined(MOTIF_GUI)
#define XFORMPT(xin,yin,xout,yout) \
{   float _xt,_yt; \
    _xt = (float) (xin); \
    _yt = (float) (yin); \
    (xout) = (int) (ca*_xt - sa*_yt + (HUB_CENTER_X + 0.5)); \
    (yout) = (int) (sa*_xt + ca*_yt + (HUB_CENTER_Y + 0.5)); \
    if (xs.useSmallDash) { \
	(xout) /= 2; (yout) /= 2; \
    } \
}
#else
#define XFORMPT(xin,yin,xout,yout)
#endif



boolean_type rect_intersects_spoke(
    int xmin, int ymin, int xmax, int ymax,
    float xval)
{
    float angle,dangle;
    float ca,sa;
    int xl1,yl1,xl2,yl2;
    int xr1,yr1,xr2,yr2;
    int xint;
    float dxl,dyl,dxr,dyr,t;
    boolean_type lo;
    int i;

    dangle = ((2.0*M_PI)/SPOKES);
    angle  = (M_PI/2.0) + xval*M_PI;
    for (i=0; i<SPOKES; ++i,angle+=dangle) {
	ca = FCOS(angle); sa = FSIN(angle);
	/* Ignore spokes out of the picture */
	/* Rotate the spoke around the hub */
	XFORMPT(HUB_RADIUS,(-SPOKE_WIDTH1/2),xl1,yl1);
	XFORMPT(WHEEL_RADIUS,(-SPOKE_WIDTH2/2),xl2,yl2);
	XFORMPT(HUB_RADIUS,(SPOKE_WIDTH1/2),xr1,yr1);
	XFORMPT(WHEEL_RADIUS,(SPOKE_WIDTH2/2),xr2,yr2);

	if ((yl1 < ymin) && (yl2 < ymin)
		&& (yr1 < ymin) && (yr2 < ymin)) continue;
	if ((yl1 > ymax) && (yl2 > ymax)
		&& (yr1 > ymax) && (yr2 > ymax)) continue;

	/* Check to see whether the intersection of the spoke edge
	 * and the top and bottom of the rectangle falls within the rectangle.
	 */
	dxl = xl2-xl1; dyl = yl2-yl1;
	lo = FALSE;
	if (ABS(dyl) > 0.001) {
	    t = (ymax-yl1)/dyl;
	    xint = xl1 + t*dxl;
	    if ((xint >= xmin) && (xint <= xmax)) return(TRUE);
	    else if (xint < xmin) lo = TRUE;
	    t = (ymin-yl1)/dyl;
	    xint = xl1 + t*dxl;
	    if ((xint >= xmin) && (xint <= xmax)) return(TRUE);
	    else if (xint < xmin) lo = TRUE;
	}
	/* Try the other edge */
	dxr = xr2-xr1; dyr = yr2-yr1;
	if (ABS(dyr) > 0.001) {
	    t = (ymax-yr1)/dyr;
	    xint = xr1 + t*dxr;
	    if ((xint >= xmin) && (xint <= xmax)) return(TRUE);
	    /* check for spoke spanning rectangle */
	    else if (lo && (xint > xmax)) return(TRUE);
	    t = (ymin-yr1)/dyr;
	    xint = xr1 + t*dxr;
	    if ((xint >= xmin) && (xint <= xmax)) return(TRUE);
	    /* check for spoke spanning rectangle */
	    else if (lo && (xint > xmax)) return(TRUE);
	}
    }

    return(FALSE);
}


static void draw_spoke(
    float angle) /* radians */
{
#if defined(MOTIF_GUI)
    float ca = FCOS(angle);
    float sa = FSIN(angle);
    int x1,y1,x2,y2;
    int i,x,dx,diam,ddiam;


    XFORMPT(HUB_RADIUS,(SPOKE_WIDTH1/2),x1,y1);
    XFORMPT(WHEEL_RADIUS,(SPOKE_WIDTH2/2),x2,y2);
    XDrawLine(xs.display,xs.dashWindow,xs.dashGC,
	x1,y1,x2,y2);

    XFORMPT(HUB_RADIUS,(-SPOKE_WIDTH1/2),x1,y1);
    XFORMPT(WHEEL_RADIUS,(-SPOKE_WIDTH2/2),x2,y2);
    XDrawLine(xs.display,xs.dashWindow,xs.dashGC,
	x1,y1,x2,y2);

    /***** WORKAROUND *****/
    /* Oki doesn't do arcs with drawing mode */
    if (cstate.vendor & VENDOR_OKI) return;

    dx = ((WHEEL_RADIUS-HUB_RADIUS)/SPOKE_HOLES);
    x = HUB_RADIUS + dx/2;
    ddiam = ((SPOKE_WIDTH2-SPOKE_WIDTH1)/SPOKE_HOLES*3/4);
    diam = (SPOKE_WIDTH1*3/4) + ddiam/2;
    if (xs.useSmallDash) {
	diam /= 2; ddiam /= 2;
    }
    for (i=0; i<SPOKE_HOLES; ++i,x+=dx,diam+=ddiam) {
	XFORMPT(x,0,x1,y1);
	XDrawArc(xs.display,xs.dashWindow,xs.dashGC,
		x1-diam/2,y1-diam/2, diam,diam, 0,360*64);
    }
#endif
}


static void drawit(
    float xval)
{
    float angle,dangle;
    int i;

    dangle = ((2.0*M_PI)/SPOKES);
    angle  = (M_PI/2.0) + xval*M_PI;
    for (i=0; i<SPOKES; ++i,angle+=dangle) {
	draw_spoke(angle);
    }

}


static float last_xval = 0.0;

void drawWheel(
    float xval)
{
#if defined(MOTIF_GUI)
    if (!xs.wheel_drawn) {
	/* Draw hub */
	SET_FOREGROUND(xs.black);
	if (xs.useSmallDash) {
	    XFillArc(xs.display,xs.dashWindow,xs.dashGC,
		(HUB_CENTER_X - HUB_RADIUS)/2, (HUB_CENTER_Y - HUB_RADIUS)/2,
		HUB_RADIUS, HUB_RADIUS,
		0,360*64);
	}
	else {
	    XFillArc(xs.display,xs.dashWindow,xs.dashGC,
		HUB_CENTER_X - HUB_RADIUS, HUB_CENTER_Y - HUB_RADIUS,
		HUB_RADIUS*2, HUB_RADIUS*2,
		0,360*64);
	}

	/* Draw spokes */
	XSetFunction(xs.display,xs.dashGC,GXxor);
	SET_FOREGROUND(xs.white);
	drawit(xval);
	XSetFunction(xs.display,xs.dashGC,GXcopy);

	last_xval = xval;
	xs.wheel_drawn = True;
    }
#endif
}


void undrawWheel(
    void)
{
#if defined(MOTIF_GUI)
    if (xs.wheel_drawn) {
	XSetFunction(xs.display,xs.dashGC,GXxor);
	SET_FOREGROUND(xs.white);
	drawit(last_xval);
	XSetFunction(xs.display,xs.dashGC,GXcopy);
	xs.wheel_drawn = False;
    }
#endif
}
