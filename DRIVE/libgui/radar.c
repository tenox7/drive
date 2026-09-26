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

static GC radarGC;
static unsigned int radarWidth,radarHeight;
static Boolean isMapped;

#define RADAR_BLIP_SIZE		3
#define EDGE_SCALE		50	/* sqrt(max distance displayed) */
#define EPSILON			(1e-3)

void createRadarWindow(
    Position x, Position y)
{
#if defined(MOTIF_GUI)
    XSetWindowAttributes attrs;
    XSizeHints hints;
    XWMHints wmhints;

    attrs.colormap = xs.dashInfo.stdCmap.colormap;
    attrs.background_pixel = xs.black;
    attrs.border_pixel = xs.black;

    if( cstate.mode & CLIENT_DUAL_RADAR_MODE ) {
	xs.radarWindow = XCreateWindow(xs.display,RootWindowOfScreen(xs.screen),
	    x,y,DEFAULT_RADAR_SIZE,DEFAULT_RADAR_SIZE * 2,0,
	    xs.dashInfo.visualInfo.depth, InputOutput,
	    xs.dashInfo.visualInfo.visual,
	    CWBackPixel|CWBorderPixel|CWColormap,&attrs);
	hints.height = DEFAULT_RADAR_SIZE * 2;
    }
    else {
	xs.radarWindow = XCreateWindow(xs.display,RootWindowOfScreen(xs.screen),
	    x,y,DEFAULT_RADAR_SIZE,DEFAULT_RADAR_SIZE,0,
	    xs.dashInfo.visualInfo.depth, 
	    InputOutput,xs.dashInfo.visualInfo.visual,
	    CWBackPixel|CWBorderPixel|CWColormap,&attrs);
	hints.height = DEFAULT_RADAR_SIZE;
    }

    if (xs.radarWindow == 0) {
	fprintf(stderr,"Cannot create radar window!\n");
	exit(1);
    }
    hints.x = x; hints.y = y;
    hints.width =   DEFAULT_RADAR_SIZE;
    hints.flags = USPosition|USSize;
    XSetNormalHints(xs.display,xs.radarWindow,&hints);
    XStoreName(xs.display,xs.radarWindow,"Radar");
    wmhints.window_group = XtWindow(xs.app_shellW);
    wmhints.flags = WindowGroupHint;
    XSetWMHints(xs.display,xs.radarWindow,&wmhints);

    XSelectInput(xs.display,xs.radarWindow,
	StructureNotifyMask|SubstructureNotifyMask);
    XRaiseWindow(xs.display,xs.radarWindow);
    radarGC = XCreateGC(xs.display,xs.radarWindow,0,NULL);
    radarWidth = DEFAULT_RADAR_SIZE;
    if( cstate.mode & CLIENT_DUAL_RADAR_MODE)
	radarHeight = DEFAULT_RADAR_SIZE * 2;
    else
	radarHeight = DEFAULT_RADAR_SIZE;
    isMapped = False;
#endif
}


void mapRadarWindow(
    void)
{
#if defined(MOTIF_GUI)
    if (isMapped || (xs.radarWindow == 0)) return;
    isMapped = True;

    XMapWindow(xs.display,xs.radarWindow);
    XClearWindow(xs.display,xs.radarWindow);
    XFlush(xs.display);
#endif
}


void unmapRadarWindow(
    void)
{
#if defined(MOTIF_GUI)
    if (!isMapped) return;
    isMapped = False;

    XUnmapWindow(xs.display,xs.radarWindow);
    XFlush(xs.display);
#endif
}


void radarEventHandler(
    XEvent *event)
{
#if defined(MOTIF_GUI)
    XConfigureEvent *configEvent;

    switch (event->type) {
	case ConfigureNotify:
	    configEvent = (XConfigureEvent *) event;
	    if ((configEvent->width != radarWidth)
		    || (configEvent->height != radarHeight)) {
		radarWidth  = configEvent->width;
		radarHeight = configEvent->height;
		XClearWindow(xs.display,xs.radarWindow);
		XFlush(xs.display);
	    }
	    break;
	case UnmapNotify:
	    unmapRadarWindow();
	    break;
	case DestroyNotify:
	    unmapRadarWindow();
	    xs.radarWindow = 0;
	    break;
    }
#endif
}


static void world_to_radar(
    radar_type *r,
    float fx, float fy, float fz,
    int *ix, int *iy, int *iz )
{
    float
	w = radarWidth/2,
	h = radarHeight/2,
	dist, scaling,
	x1, y1, z1;
    int
	x, y, z;

    h = radarHeight / 4;
    if (w < h) {
	scaling = w/EDGE_SCALE;
    }
    else {
	scaling = h/EDGE_SCALE;
    }

    x1 = fx;	y1 = fy;	z1 = fz;

    /* Model-relative coordinate [xyz] */
    point_xform( &x1, &y1, &z1, r->ixform );

    dist = FHYPOT3(x1,y1,z1);

    /* Now scale the distance */
    if (dist < 0.01) {
	dist = 0.0;
    }
    else {
	x1 /= dist;
	y1 /= dist;
	z1 /= dist;
	dist = FSQRT(dist) * scaling;
    }

    x = (int) ( x1*dist + 0.5);
    y = (int) (-y1*dist + 0.5);
    z = (int) (-z1*dist + 0.5);

    /* Clip to window bounds */
    if (x < -w) {	
	y *= -(w/x); z *= -(w/x); x = -w;
    }
    else if (x > w) {
	y *= (w/x); z *= (w/x); x = w;
    }

    if (z < -h) {	
	x *= -(h/z); y *= -(h/z); z = -h;
    }
    else if (z > h) {
	x *= (h/z); y *= (h/z); z = h;
    }

    if (y < -h) {	
	x *= -(h/y); z *= -(h/y); y = -h;
    }
    else if (y > h) {
	x *= (h/y); z *= (h/y); y = h;
    }

    /* Offset to center */
    x += w - (RADAR_BLIP_SIZE/2);
    y += radarHeight/2 + h - (RADAR_BLIP_SIZE/2);
    z += h - (RADAR_BLIP_SIZE/2);

    *ix = x;
    *iy = y;
    *iz = z;
}


void radarUpdate(
    radar_type *r)
{
#if defined(MOTIF_GUI)
    int i;
    int x,y,z,xx,yy,zz,hx,hy,hz;
    float x1,y1,z1;
    float ca,sa;
    float w = radarWidth/2;
    float h = radarHeight/2;
    float scaling;
    float dist;


    if (!isMapped || (xs.radarWindow == 0)) return;

    XClearWindow(xs.display,xs.radarWindow);

    if( cstate.mode & CLIENT_DUAL_RADAR_MODE) {
	/* Draw the separating line... */
	XSetForeground(xs.display,radarGC, xs.white);
	XDrawLine(xs.display,xs.radarWindow,radarGC,
	    0,(radarHeight/2),radarWidth,(radarHeight/2));

	x1 = 0.0; y1 = 0.0; z1 = 1.0;
	point_xform( &x1, &y1, &z1, r->xform );
	y1 -= r->xform[3][1];
	if( y1 < 0.0 ) {
	    XSetForeground(xs.display,radarGC,xs.red);
	}
	else {
	    XSetForeground(xs.display,radarGC,xs.green);
	}

	/* Draw the scene bounds... */
	x1 = r->xpos - 3000.0; y1 = 0.0; z1 = r->zpos + 3000.0;
	world_to_radar( r, x1, y1, z1, &x, &y, &z );

	x1 = r->xpos + 3000.0; y1 = 0.0; z1 = r->zpos + 3000.0;
	world_to_radar( r, x1, y1, z1, &xx, &yy, &zz );
	x1 = r->xpos + 3000.0; y1 = 1000.0; z1 = r->zpos + 3000.0;
	world_to_radar( r, x1, y1, z1, &hx, &hy, &hz );
	XDrawLine(xs.display,xs.radarWindow,radarGC, xx,yy,hx,hy);
	XDrawLine(xs.display,xs.radarWindow,radarGC, x,y,xx,yy);

	x1 = r->xpos + 3000.0; y1 = 0.0; z1 = r->zpos - 3000.0;
	world_to_radar( r, x1, y1, z1, &x, &y, &z );
	x1 = r->xpos + 3000.0; y1 = 1000.0; z1 = r->zpos - 3000.0;
	world_to_radar( r, x1, y1, z1, &hx, &hy, &hz );
	XDrawLine(xs.display,xs.radarWindow,radarGC, x,y,hx,hy);
	XDrawLine(xs.display,xs.radarWindow,radarGC, x,y,xx,yy);

	x1 = r->xpos - 3000.0; y1 = 0.0; z1 = r->zpos - 3000.0;
	world_to_radar( r, x1, y1, z1, &xx, &yy, &zz );
	x1 = r->xpos - 3000.0; y1 = 1000.0; z1 = r->zpos - 3000.0;
	world_to_radar( r, x1, y1, z1, &hx, &hy, &hz );
	XDrawLine(xs.display,xs.radarWindow,radarGC, xx,yy,hx,hy);
	XDrawLine(xs.display,xs.radarWindow,radarGC, x,y,xx,yy);

	x1 = r->xpos - 3000.0; y1 = 0.0; z1 = r->zpos + 3000.0;
	world_to_radar( r, x1, y1, z1, &x, &y, &z );
	x1 = r->xpos - 3000.0; y1 = 1000.0; z1 = r->zpos + 3000.0;
	world_to_radar( r, x1, y1, z1, &hx, &hy, &hz );
	XDrawLine(xs.display,xs.radarWindow,radarGC, x,y,hx,hy);
	XDrawLine(xs.display,xs.radarWindow,radarGC, x,y,xx,yy);

	for (i=0; i<r->num_cars; ++i) {
	    /* World coordinate [xyz] */
	    x1 = r->car_list[i].x;
	    y1 = r->car_list[i].y;
	    z1 = r->car_list[i].z;

	    world_to_radar( r, x1, y1, z1, &x, &y, &z );

	    XSetForeground(xs.display,radarGC,
		RGB_TO_PIXEL(r->car_list[i].r,r->car_list[i].g,r->car_list[i].b));
	    XFillRectangle(xs.display,xs.radarWindow,radarGC,
		x,z,RADAR_BLIP_SIZE,RADAR_BLIP_SIZE);
	    XFillRectangle(xs.display,xs.radarWindow,radarGC,
		x,y,RADAR_BLIP_SIZE,RADAR_BLIP_SIZE);
	}
    }
    else {
	ca = FCOS(-(r->angle));
	sa = FSIN(-(r->angle));
	if (w < h) {
	    scaling = w/EDGE_SCALE;
	}
	else {
	    scaling = h/EDGE_SCALE;
	}

	for (i=0; i<r->num_cars; ++i) {
	    x = r->car_list[i].x - r->xpos;
	    z = r->car_list[i].z - r->zpos;

	    /* Rotate relative to my car's facing */
	    x1 =  x*ca + z*sa;
	    z1 = -x*sa + z*ca;

	    /* Normalize */
	    dist  = FHYPOT2(x1,z1);

	    /* Now scale the distance */
	    if (dist < 0.01) {
		dist = 0.0;
	    }
	    else {
		x1 /= dist;
		z1 /= dist;
		dist = FSQRT(dist) * scaling;
	    }

	    x = (int) ( x1*dist + 0.5);
	    z = (int) (-z1*dist + 0.5);

	    /* Clip to edges */
	    if (x < -w) {	
		z *= -(w/x);
		x = -w;
	    }
	    else if (x > w) {
		z *= (w/x);
		x = w;
	    }

	    if (z < -h) {	
		x *= -(h/z);
		z = -h;
	    }
	    else if (z > h) {
		x *= (h/z);
		z = h;
	    }

	    /* Offset to center */
	    x += w - (RADAR_BLIP_SIZE/2);
	    z += h - (RADAR_BLIP_SIZE/2);

	    XSetForeground(xs.display,radarGC,
		RGB_TO_PIXEL(r->car_list[i].r,r->car_list[i].g,r->car_list[i].b));
	    XFillRectangle(xs.display,xs.radarWindow,radarGC,
		x,z,RADAR_BLIP_SIZE,RADAR_BLIP_SIZE);
	}
    }

    /* XFlush(xs.display); */
#endif
}
