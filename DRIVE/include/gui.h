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


#ifndef _GUI_H_INCLUDED
#define _GUI_H_INCLUDED

#include <X11/X.h>
#include <X11/Xlib.h>
#include <Xm/Xm.h>

#include "message.h"

/********************** FUNCTION PROTOTYPES ***********************************/
/*** From interface.c ***/
extern void mapVehicleSelectionWindow(
    void);
extern void grabCursor(
    void);
extern void processXEvents(
    void);

/*** From menubar.c ***/
extern void smallWindowCallback(
    void);
extern void updateGUIVehicles(
    Driveable *driveables,
    int num_cars);

/*** From placewin.c ***/
extern void updateRacePosition(
    char *text,
    int position,
    boolean_type highlight,
    float r, float b, float g);
extern void updateRacePositionType(
    boolean_type is_final,
    int players);


/*** From radar.c ***/
extern void radarUpdate(
    radar_type *r);


/*** From textwin.c ***/
extern void redrawCheckpoint(
    void);
extern void updateStateWindow(
    int state);
extern void showPlace(
    int position);


/*** From title.c ***/
extern void mapTitleWindow(
    void);
extern void unmapTitleWindow(
    void);


/*** From wheel.c ***/
extern void drawWheel(
    float xval);
extern void undrawWheel(
    void);


/*** From windows.c ***/
/* Returns fildes of graphics window */
extern int createGUI(
    int argc,
    char *argv[],
    Boolean forceSmall,
    void (*resize_routine)
	(int width, int height),
    Boolean (*select_visuals_routine)
	(XVisualInfo *vdash, XVisualInfo *vgraphics));
extern void redrawDash(
    void);


/*** From xsight.c ***/
extern void xDrawSight(
    int width, int height,
    boolean_type do_crosshair);


/*** From pexwin.c ***/
extern void set_pex_table_entries(
    void *color_lut);  /* actually PEXLookupTable* */
extern Boolean PEXGetVisuals(
    XVisualInfo *vdash,
    XVisualInfo *vgraphics);

/*** From starwin.c ***/
extern Boolean StarbaseGetVisuals(
    XVisualInfo *vdash,
    XVisualInfo *vgraphics);


/*** From findsvr.c ***/
/* Call when you want a new server location selected */
extern char *findServer(
    void);
/* Call for bad server */
extern void badServerMessage(
    char *bad_server_name);


/*** From cmap.c ***/
extern void colornameToRGB(
    char *colorname,
    float *r, float *g, float *b);


#endif /* _GUI_H_INCLUDED */
