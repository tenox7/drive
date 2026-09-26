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
#endif

#include "global.h"
#include "drive.h"
#include "gui.h"
#include "message.h"
#include "windows.h"


void xDrawSight(
    int width, int height,
    boolean_type do_crosshair)
{
#if defined(MOTIF_GUI)
    int radius,xc,yc,dy,y,x2,i;
    XSegment seg[2+SIGHT_ELEVATION_MARKS];

    radius = MIN(width,height)/2 - SIGHT_MARGIN;
    xc = width/2;
    yc = height/2;

    XSetForeground(xs.display,xs.grGC,xs.white);
    XDrawArc(xs.display,xs.graphicsWindow,xs.grGC,
	xc-radius,yc-radius,radius*2,radius*2,0,360*64);
		
    if (do_crosshair) {
	seg[0].x1 = xc-radius; seg[0].y1 = yc;
	    seg[0].x2 = xc+radius; seg[0].y2 = yc;
	seg[1].x1 = xc; seg[1].y1 = yc-radius;
	    seg[1].x2 = xc; seg[1].y2 = yc+radius;

	dy = radius/SIGHT_ELEVATION_MARKS;
	x2 = xc + 20;
	/* elevation marks */
	for (i=2,y = yc-dy; y > yc-radius+dy/2; y -= dy,++i) {
	    seg[i].x1 = xc; seg[i].y1 = y;
		seg[i].x2 = x2; seg[i].y2 = y;
	}
	XDrawSegments(xs.display,xs.graphicsWindow,xs.grGC,
	    seg,2+SIGHT_ELEVATION_MARKS);
    }
#endif
}
