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


#ifndef _XgStarbase_h
#define _XgStarbase_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif

/* New resources */
#define XgNopenMode "openMode"
#define XgNshadeMode "shadeMode"
#define XgNoverlay "overlay"
#define XgNretained "retained"
#define XgNtransparent "transparent"
#define XgNvisual "visual"
#define XgNdriver "driver"
#define XgNfildes "fildes"
#define XgNwmCmap "wmCmap"
#define XgNrescalePolicy "rescalePolicy"
#define XgNmaxWidth "maxWidth"
#define XgNmaxHeight "maxHeight"

#define XgCOpenMode "OpenMode"
#define XgCShadeMode "ShadeMode"
#define XgCOverlay "Overlay"
#define XgCRetained "Retained"
#define XgCTransparent "Transparent"
#define XgCVisual "Visual"
#define XgCDriver "Driver"
#define XgCFildes "Fildes"
#define XgCWmCmap "WmCmap"
#define XgCRescalePolicy "RescalePolicy"
#define XgCMaxWidth "MaxWidth"
#define XgCMaxHeight "MaxHeight"

#define XgROpenMode "openMode"
#define XgRShadeMode "shadeMode"
#define XgRWmCmap "wmCmap"
#define XgRRescalePolicy "rescalePolicy"

/* New resource settings */
#define XgINIT INIT
#define XgTHREE_D THREE_D
#define XgMODEL_XFORM MODEL_XFORM
#define XgFLOAT_XFORM FLOAT_XFORM
#define XgINT_XFORM INT_XFORM
#define XgINT_XFORM32 INT_XFORM32
#define XgUNACCELERATED UNACCELERATED
#define XgACCELERATED ACCELERATED
#define XgCMAP_NORMAL CMAP_NORMAL
#define XgCMAP_FULL CMAP_FULL
#define XgCMAP_MONOTONIC CMAP_MONOTONIC
#define XgWM_CMAP_NONE 0          /* Make no change to WM_COLORMAP_WINDOWS */
#define XgWM_CMAP_LOW_PRIORITY 1  /* Add to bottom of WM_COLORMAP_WINDOWS */
#define XgWM_CMAP_HIGH_PRIORITY 2 /* Add to top of WM_COLORMAP_WINDOWS */
#define XgRESCALE_NONE 0    /* Make no change to scale. */
#define XgRESCALE_DISTORT 1 /* Set p1-p2 to scale with distortion. */
#define XgRESCALE_MAJOR 2 /* Set p1-p2 to scale to max axis. */
#define XgRESCALE_MINOR 3 /* Set p1-p2 to scale to min axis. */

/* Class record pointer */
extern WidgetClass xgStarbaseWidgetClass;

/* C Widget type definition */
typedef struct _XgStarbaseClassRec *XgStarbaseWidgetClass;
typedef struct _XgStarbaseRec *XgStarbaseWidget;

#define XgIsStarbase(w) XtIsSubclass((w), xgStarbaseWidgetClass)

/*  Creation entry point: */
#ifdef __STDC__ /* FN PROTO [ */
extern Widget XgCreateStarbase(
    Widget p,
    String name,
    ArgList args,
    Cardinal n);
#else  /* __OLD_K&R_C ] [ */
extern Widget XgCreateStarbase();
#endif /* __OLD_K&R_C ] */

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XgStarbase_h */
