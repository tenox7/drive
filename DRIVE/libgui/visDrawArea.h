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



#ifndef _XgVisualDrawingArea_h
#define _XgVisualDrawingArea_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _XgStarbase_h
/* New resources */
# define XgNoverlay "overlay"
# define XgNtransparent "transparent"
# define XgNvisual "visual"
# define XgNwmCmap "wmCmap"
# define XgCOverlay "Overlay"
# define XgCTransparent "Transparent"
# define XgCVisual "Visual"
# define XgCWmCmap "WmCmap"
# define XgRWmCmap "wmCmap"
/* New resource settings */
# define XgWM_CMAP_NONE 0          /* Make no change to WM_COLORMAP_WINDOWS */
# define XgWM_CMAP_LOW_PRIORITY 1  /* Add to bottom of WM_COLORMAP_WINDOWS */
# define XgWM_CMAP_HIGH_PRIORITY 2 /* Add to top of WM_COLORMAP_WINDOWS */
#endif /* ndef _XgStarbase_h */

/* Class record pointer */
extern WidgetClass xgVisualDrawingAreaWidgetClass;

/* C Widget type definition */
typedef struct _XgVisualDrawingAreaClassRec *XgVisualDrawingAreaWidgetClass;
typedef struct _XgVisualDrawingAreaRec *XgVisualDrawingAreaWidget;

#define XgIsVisualDrawingArea(w) XtIsSubclass((w), xgVisualDrawingAreaWidgetClass)

/*  Creation entry point: */
#ifdef __STDC__ /* FN PROTO [ */
extern Widget XgCreateVisualDrawingArea(
    Widget p,
    String name,
    ArgList args,
    Cardinal n);
#else  /* __OLD_K&R_C ] [ */
extern Widget XgCreateVisualDrawingArea();
#endif /* __OLD_K&R_C ] */

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XgVisualDrawingArea_h */
