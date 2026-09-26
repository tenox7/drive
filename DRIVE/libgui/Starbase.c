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
#ifdef __linux__
typedef int _dummy;
#else

#include <stdio.h>
#include <Xm/Xm.h>
#include <X11/XHPproto.h>
#include <Mrm/MrmPublic.h>
#include "StarbaseP.h"
#include "starbase.c.h"

/* Modify this string when modifications are made to this source */
static char ID_string[] = "@(#) Starbase Widget, DRIVE Version";

#define OpenMode(w) XmField(w,offsets,XgStarbase,open_mode,int)
#define ShadeMode(w) XmField(w,offsets,XgStarbase,shade_mode,int)
#define Overlay(w) XmField(w,offsets,XgStarbase,overlay,Boolean)
#define Retained(w) XmField(w,offsets,XgStarbase,retained,Boolean)
#define Transparent(w) XmField(w,offsets,XgStarbase,transparent,int)
#define sbVisual(w) XmField(w,offsets,XgStarbase,visual,Visual *)
#define Driver(w) XmField(w,offsets,XgStarbase,driver,String)
#define Fildes(w) XmField(w,offsets,XgStarbase,fildes,int)
#define WmCmap(w) XmField(w,offsets,XgStarbase,wm_cmap,int)
#define RescalePolicy(w) XmField(w,offsets,XgStarbase,rescale_policy,int)
#define MaxWidth(w) XmField(w,offsets,XgStarbase,max_width,Dimension)
#define MaxHeight(w) XmField(w,offsets,XgStarbase,max_height,Dimension)

#define ExposeCallback(w) XmField(w,offsets,XmDrawingArea,expose_callback, \
    caddr_t)
#define ResizeCallback(w) XmField(w,offsets,XmDrawingArea,resize_callback, \
    caddr_t)

#define BackgroundPixel(w) XmField(w,offsets,Core,background_pixel,Pixel)
#define Depth(w) XmField(w,offsets,Core,depth,int)
#define Colormap(w) XmField(w,offsets,Core,colormap,int)
#define Width(w) XmField(w,offsets,Core,width,Dimension)
#define Height(w) XmField(w,offsets,Core,height,Dimension)

#ifdef __STDC__ /* FN PROTO [ */
static void ClassInitialize(void);
static void Realize(
    Widget w,
    Mask *mask,
    XSetWindowAttributes *winattr);
static void Redisplay(
    XgStarbaseWidget w,
    XEvent *event,
    Region region);
static void Resize(XgStarbaseWidget w);
static void Destroy(XgStarbaseWidget w);
static Boolean SetValues(
    XgStarbaseWidget current,
    XgStarbaseWidget request,
    XgStarbaseWidget new);
#else  /* __OLD_K&R_C ] [ */
static void ClassInitialize();
static void Realize();
static void Redisplay();
static void Resize();
static void Destroy();
static Boolean SetValues();
#endif /* __OLD_K&R_C ] */

static XmPartResource resources[] = {
    {XgNopenMode, XgCOpenMode, XgROpenMode, sizeof(int),
	XmPartOffset(XgStarbase,open_mode), XmRImmediate,
	(caddr_t)(INIT|THREE_D|MODEL_XFORM)},
    {XgNshadeMode, XgCShadeMode, XgRShadeMode, sizeof(int),
	XmPartOffset(XgStarbase,shade_mode), XmRImmediate,
	(caddr_t)CMAP_NORMAL},
    {XgNoverlay, XgCOverlay, XmRBoolean, sizeof(Boolean),
	XmPartOffset(XgStarbase,overlay), XmRImmediate, False},
    {XgNretained, XgCRetained, XmRBoolean, sizeof(Boolean),
	XmPartOffset(XgStarbase,retained), XmRImmediate, False},
    {XgNtransparent, XgCTransparent, XmRInt, sizeof(int),
	XmPartOffset(XgStarbase,transparent), XmRImmediate, (caddr_t)-1},
    {XgNvisual, XgCVisual, XtRVisual, sizeof(Visual *),
	XmPartOffset(XgStarbase,visual), XmRImmediate, NULL},
    {XgNdriver, XgCDriver, XmRString, sizeof(String),
	XmPartOffset(XgStarbase,driver), XmRString, NULL},
    {XgNfildes, XgCFildes, XmRInt, sizeof(int),
	XmPartOffset(XgStarbase,fildes), XmRImmediate, (caddr_t)-2},
    {XgNwmCmap, XgCWmCmap, XgRWmCmap, sizeof(int),
	XmPartOffset(XgStarbase,wm_cmap), XmRImmediate,
	(caddr_t)XgWM_CMAP_HIGH_PRIORITY},
    {XgNrescalePolicy, XgCRescalePolicy, XgRRescalePolicy, sizeof(int),
	XmPartOffset(XgStarbase,rescale_policy), XmRImmediate,
	(caddr_t)XgRESCALE_MINOR},
    {XgNmaxWidth, XgCMaxWidth, XmRDimension, sizeof(Dimension),
	XmPartOffset(XgStarbase,max_width), XmRImmediate, (caddr_t)0},
    {XgNmaxHeight, XgCMaxHeight, XmRDimension, sizeof(Dimension),
	XmPartOffset(XgStarbase,max_height), XmRImmediate, (caddr_t)0}
};

XgStarbaseClassRec xgStarbaseClassRec = {
    {                                   /* core_class fields    */
    (WidgetClass) &xmDrawingAreaClassRec,/* superclass           */
    "XgStarbase",                       /* class_name           */
    sizeof(XgStarbaseRec),              /* widget_size          */
    ClassInitialize,                    /* class_initialize     */
    NULL,                               /* class_part_initialize*/
    False,                              /* class_inited         */
    NULL,                               /* initialize           */
    NULL,                               /* initialize_notify    */
    Realize,                            /* realize              */
    NULL,                               /* actions              */
    0,                                  /* num_actions          */
    (XtResourceList)resources,          /* resources            */
    XtNumber(resources),                /* num_resources        */
    NULLQUARK,                          /* xrm_class            */
    True,                               /* compress_motion      */
    True,                               /* compress_exposure    */
    True,                               /* compress_enterleave  */
    False,                              /* visible_interest     */
    (XtWidgetProc) Destroy,             /* destroy              */
    (XtWidgetProc) Resize,              /* resize               */
    (XtExposeProc) Redisplay,           /* expose               */
    (XtSetValuesFunc) SetValues,        /* set_values           */
    NULL,                               /* set_values_hook      */
    XtInheritSetValuesAlmost,           /* set_values_almost    */
    NULL,                               /* get_values_hook      */
    NULL,                               /* accept_focus         */
    XtVersionDontCheck,                 /* version              */
    NULL,                               /* callback_private     */
    XmInheritTranslations,              /* tm_table             */
    XtInheritQueryGeometry,             /* query_geometry       */
    NULL,                               /* disp accelerator     */
    NULL                                /* extension            */
    },
    {                                   /* composite_class fields */
    XtInheritGeometryManager,           /* geometry_manager   */
    XtInheritChangeManaged,             /* change_managed     */
    XtInheritInsertChild,               /* insert_child       */
    XtInheritDeleteChild,               /* delete_child       */
    NULL,                               /* extension          */
    },
    {                                   /* constraint_class fields */
    NULL,                               /* resource list        */
    0,                                  /* num resources        */
    0,                                  /* constraint size      */
    NULL,                               /* init proc            */
    NULL,                               /* destroy proc         */
    NULL,                               /* set values proc      */
    NULL,                               /* extension            */
    },
    {                                   /* manager_class fields */
    XtInheritTranslations,              /* translations           */
    NULL,                               /* syn_resources          */
    0,                                  /* num_get_resources      */
    NULL,                               /* syn_cont_resources     */
    0,                                  /* num_get_cont_resources */
    XmInheritParentProcess,             /* parent_process         */
    NULL,                               /* extension           */
    },
    {                                   /* drawing_area_class record */
    NULL,                               /* extension, aka. mumble */
    },
    {                                   /* starbase_class record */
    NULL,                               /* extension            */
    }
};

externaldef(xgstarbasewidgetclass) WidgetClass xgStarbaseWidgetClass =
    (WidgetClass) &xgStarbaseClassRec;

static XmOffsetPtr offsets; /* Part Offset table for XmResolvePartOffsets */

/**********************************************************************
 *
 * XgCreateStarbase - Convenience routine, used by Uil/Mrm.
 *
 *********************************************************************/

#ifdef __STDC__ /* FN PROTO [ */
Widget XgCreateStarbase(
    Widget parent,
    String name,
    ArgList arglist,
    Cardinal nargs)
#else  /* __OLD_K&R_C ] [ */
Widget XgCreateStarbase(parent, name, arglist, nargs)
    Widget parent;
    String name;
    ArgList arglist;
    Cardinal nargs;
#endif /* __OLD_K&R_C ] */
{
    return(XtCreateWidget(name, xgStarbaseWidgetClass, parent, arglist, nargs));
}

/**********************************************************************
 *
 * StarbaseMrmInitialize - register Starbase widget class with Mrm
 *
 *********************************************************************/

#ifdef __STDC__ /* FN PROTO [ */
int XgStarbaseMrmInitialize(void)
#else  /* __OLD_K&R_C ] [ */
int XgStarbaseMrmInitialize()
#endif /* __OLD_K&R_C ] */
{
    return(MrmRegisterClass (MrmwcUnknown, "Starbase", "XgCreateStarbase",
	XgCreateStarbase, (WidgetClass) &xgStarbaseClassRec));
}

/**********************************************************************
 *
 * Class methods
 *
 *********************************************************************/

#ifdef __STDC__ /* FN PROTO [ */
static void ConversionWarning(String string, String type)
#else  /* __OLD_K&R_C ] [ */
static void ConversionWarning(string, type)
    String string, type;
#endif /* __OLD_K&R_C ] */
{
    String params[2];
    Cardinal num_params = 2;

    params[0] = string;
    params[1] = type;
    XtAppWarningMsg((XtAppContext) _XtDefaultAppContext(),
	       "conversionError", "string", "XtToolkitError",
	       "Cannot convert string \"%s\" to type %s",
		params, &num_params);
}


#ifdef __STDC__ /* FN PROTO [ */
static Boolean CvtStringToOpenMode(
    XrmValuePtr args,
    Cardinal *num_args,
    XrmValuePtr fromVal,
    XrmValuePtr toVal)
#else  /* __OLD_K&R_C ] [ */
static Boolean CvtStringToOpenMode(args, num_args, fromVal, toVal)
    XrmValuePtr args;
    Cardinal *num_args;
    XrmValuePtr fromVal;
    XrmValuePtr toVal;
#endif /* __OLD_K&R_C ] */
{
    static int value = 0;
    char *ptr = (char *) fromVal->addr;
    char *tail;
    char save;

    while (*ptr) {
	tail = ptr;
	while (*tail && *tail != ' ' && *tail != '|')
	    tail++;

	save = *tail;
	*tail = NULL;

	if (!strcasecmp(ptr, "INIT")) {
	    value |= INIT;
	} else if (!strcasecmp(ptr, "THREE_D")) {
	    value |= THREE_D;
	} else if (!strcasecmp(ptr, "MODEL_XFORM")) {
	    value |= MODEL_XFORM;
	} else if (!strcasecmp(ptr, "FLOAT_XFORM")) {
	    value |= FLOAT_XFORM;
	} else if (!strcasecmp(ptr, "INT_XFORM")) {
	    value |= INT_XFORM;
	} else if (!strcasecmp(ptr, "INT_XFORM32")) {
	    value |= INT_XFORM32;
	} else if (!strcasecmp(ptr, "UNACCELERATED")) {
	    value |= UNACCELERATED;
	} else if (!strcasecmp(ptr, "ACCELERATED")) {
	    value |= ACCELERATED;
	} else {
	    *tail = save;
	    ConversionWarning((String) fromVal->addr, "XgOpenMode");
	    return False;
	}
	*tail = save;
	ptr = tail;
	while (*ptr == ' ' || *ptr == '|')
	    ptr++;
    }
    if (toVal->addr) {
	if (toVal->size < sizeof(int)) {
	    return False;
	} else {
	    *((int *) toVal->addr) = value;
	}
    } else {
	toVal->addr = (caddr_t) &value;
    }
    toVal->size = sizeof(int);
    return True;
}


#ifdef __STDC__ /* FN PROTO [ */
static Boolean CvtStringToShadeMode(
    XrmValuePtr args,
    Cardinal *num_args,
    XrmValuePtr fromVal,
    XrmValuePtr toVal)
#else  /* __OLD_K&R_C ] [ */
static Boolean CvtStringToShadeMode(args, num_args, fromVal, toVal)
    XrmValuePtr args;
    Cardinal *num_args;
    XrmValuePtr fromVal;
    XrmValuePtr toVal;
#endif /* __OLD_K&R_C ] */
{
    static int value;

    if (!strcasecmp((char *) fromVal->addr, "CMAP_NORMAL")) {
	value = CMAP_NORMAL;
    } else if (!strcasecmp((char *) fromVal->addr, "CMAP_MONOTONIC")) {
	value = CMAP_MONOTONIC;
    } else if (!strcasecmp((char *) fromVal->addr, "CMAP_FULL")) {
	value = CMAP_FULL;
    } else {
	ConversionWarning((String) fromVal->addr, "XgShadeMode");
	return False;
    }
    if (toVal->addr) {
	if (toVal->size < sizeof(int)) {
	    return False;
	} else {
	    *((int *) toVal->addr) = value;
	}
    } else {
	toVal->addr = (caddr_t) &value;
    }
    toVal->size = sizeof(int);
    return True;
}


#ifdef __STDC__ /* FN PROTO [ */
static Boolean CvtStringToWMCmap(
    XrmValuePtr args,
    Cardinal *num_args,
    XrmValuePtr fromVal,
    XrmValuePtr toVal)
#else  /* __OLD_K&R_C ] [ */
static Boolean CvtStringToWMCmap(args, num_args, fromVal, toVal)
    XrmValuePtr args;
    Cardinal *num_args;
    XrmValuePtr fromVal;
    XrmValuePtr toVal;
#endif /* __OLD_K&R_C ] */
{
    static int value;

    if (!strcasecmp((char *) fromVal->addr, "WM_CMAP_NONE")) {
	value = XgWM_CMAP_NONE;
    } else if (!strcasecmp((char *) fromVal->addr, "WM_CMAP_LOW_PRIORITY")) {
	value = XgWM_CMAP_LOW_PRIORITY;
    } else if (!strcasecmp((char *) fromVal->addr, "WM_CMAP_HIGH_PRIORITY")) {
	value = XgWM_CMAP_HIGH_PRIORITY;
    } else {
	ConversionWarning((String) fromVal->addr, "XgWmCmap");
	return False;
    }
    if (toVal->addr) {
	if (toVal->size < sizeof(int)) {
	    return False;
	} else {
	    *((int *) toVal->addr) = value;
	}
    } else {
	toVal->addr = (caddr_t) &value;
    }
    toVal->size = sizeof(int);
    return True;
}


#ifdef __STDC__ /* FN PROTO [ */
static Boolean CvtStringToRescalePolicy(
    XrmValuePtr args,
    Cardinal *num_args,
    XrmValuePtr fromVal,
    XrmValuePtr toVal)
#else  /* __OLD_K&R_C ] [ */
static Boolean CvtStringToRescalePolicy(args, num_args, fromVal, toVal)
    XrmValuePtr args;
    Cardinal *num_args;
    XrmValuePtr fromVal;
    XrmValuePtr toVal;
#endif /* __OLD_K&R_C ] */
{
    static int value;

    if (!strcasecmp((char *) fromVal->addr, "RESCALE_NONE")) {
	value = XgRESCALE_NONE;
    } else if (!strcasecmp((char *) fromVal->addr, "RESCALE_DISTORT")) {
	value = XgRESCALE_DISTORT;
    } else if (!strcasecmp((char *) fromVal->addr, "RESCALE_MAJOR")) {
	value = XgRESCALE_MAJOR;
    } else if (!strcasecmp((char *) fromVal->addr, "RESCALE_MINOR")) {
	value = XgRESCALE_MINOR;
    } else {
	ConversionWarning((String) fromVal->addr, "XgRescalePolicy");
	return False;
    }
    if (toVal->addr) {
	if (toVal->size < sizeof(int)) {
	    return False;
	} else {
	    *((int *) toVal->addr) = value;
	}
    } else {
	toVal->addr = (caddr_t) &value;
    }
    toVal->size = sizeof(int);
    return True;
}


#ifdef __STDC__ /* FN PROTO [ */
static void ClassInitialize(void)
#else  /* __OLD_K&R_C ] [ */
static void ClassInitialize()
#endif /* __OLD_K&R_C ] */
{
    XmResolvePartOffsets(xgStarbaseWidgetClass, &offsets);
    XtAddConverter(XtRString, XgROpenMode,
	(XtConverter) CvtStringToOpenMode, NULL, 0);
    XtAddConverter(XtRString, XgRShadeMode,
	(XtConverter) CvtStringToShadeMode, NULL, 0);
    XtAddConverter(XtRString, XgRWmCmap,
	(XtConverter) CvtStringToWMCmap, NULL, 0);
    XtAddConverter(XtRString, XgRRescalePolicy,
	(XtConverter) CvtStringToRescalePolicy,
	NULL, 0);
}


#ifdef __STDC__ /* FN PROTO [ */
static void UpdateWmCmap(Widget w)
#else  /* __OLD_K&R_C ] [ */
static void UpdateWmCmap(w)
    Widget w;
#endif /* __OLD_K&R_C ] */
{
    Window *colormap_windows, *colormap_windows_return;
    Window two_colormap_windows[2], topwindow;
    int i, count, count_return, top_listed;
    Widget top;

    /* Update the WM_COLORMAP_WINDOWS property. */
    if (!XtIsRealized(w))
	return;
    switch(WmCmap(w)) {
    case XgWM_CMAP_NONE: /* Make no change to WM_COLORMAP_WINDOWS */
	break;
    case XgWM_CMAP_LOW_PRIORITY: /* Add to bottom of WM_COLORMAP_WINDOWS */
	/* Find the top level widget. */
	top = w;
	while (!XtIsShell(top)) {
	    top = XtParent(top);
	}
	topwindow = XtWindow(top);

	/* Check for an existing property. */
	if (XGetWMColormapWindows(XtDisplay(w), topwindow,
	    &colormap_windows_return, &count_return)) {

	    if (NULL == (colormap_windows =
		(Window *) XtCalloc(count_return+1, sizeof(Window)))) {
		XFree((char *)colormap_windows_return);
		return;
	    }

	    count = 0;
	    for (i=0; i<count_return; i++) {
		if (colormap_windows_return[i] != XtWindow(w)) {
		    colormap_windows[count++] = colormap_windows_return[i];
		}
	    }
	    colormap_windows[count++] = XtWindow(w);

	    XSetWMColormapWindows(XtDisplay(w), topwindow,
		colormap_windows, count);
	    XtFree((char *)colormap_windows);
	    XFree((char *)colormap_windows_return);
	} else {
	    two_colormap_windows[0] = XtWindow(w);
	    XSetWMColormapWindows(XtDisplay(w), topwindow,
		two_colormap_windows, 1);
	}
	break;
    case XgWM_CMAP_HIGH_PRIORITY: /* Add to top of WM_COLORMAP_WINDOWS */
	/* Find the top level widget. */
	top = w;
	while (!XtIsShell(top)) {
	    top = XtParent(top);
	}
	topwindow = XtWindow(top);

	/* Check for an existing property. */
	if (XGetWMColormapWindows(XtDisplay(w), topwindow,
	    &colormap_windows_return, &count_return)) {

	    if (NULL == (colormap_windows =
		(Window *) XtCalloc(count_return+1, sizeof(Window)))) {
		XFree((char *)colormap_windows_return);
		return;
	    }

	    top_listed = 0;
	    for (i=0; i<count_return; i++) {
		if (colormap_windows_return[i] == topwindow) {
		    top_listed = 1;
		}
	    }

	    colormap_windows[0] = XtWindow(w);
	    count = 1;
	    if (!top_listed) {
		/* The top level window was implicitly of higher
		 * priority than the windows listed in the property.
		 * Place it just below this widget's priority in the
		 * new list.
		 */
		colormap_windows[count++] = topwindow;
	    }
	    for (i=0; i<count_return; i++) {
		if (colormap_windows_return[i] != XtWindow(w)) {
		    colormap_windows[count++] = colormap_windows_return[i];
		}
	    }

	    XSetWMColormapWindows(XtDisplay(w), topwindow,
		colormap_windows, count);
	    XtFree((char *)colormap_windows);
	    XFree((char *)colormap_windows_return);
	} else {
	    two_colormap_windows[0] = XtWindow(w);
	    two_colormap_windows[1] = topwindow;
	    XSetWMColormapWindows(XtDisplay(w), topwindow,
		two_colormap_windows, 2);
	}
	break;
    }
}


/*
 * These tables give preference weights to each visual class based on how
 * well they suit specific Starbase colormap modes.  The classes with
 * higher numbers work better than those with lower numbers.  This is used
 * to choose a visual among those available given a requirement for image or
 * overlay visuals.  Greatest depth will be used to choose between visuals
 * of the same class. (PseudoColor over depth 8 is avoided.)
 *
 *                                    StaticGray   0
 *                                    |    GrayScale    1
 *                                    |    |    StaticColor  2
 *                                    |    |    |    PseudoColor  3
 *                                    |    |    |    |    TrueColor    4
 *                                    |    |    |    |    |    DirectColor  5
 *                                    |    |    |    |    |    |    */
int CMAP_NORMAL_preferences[6] =    { 300, 400, 000, 500, 100, 200 };
int CMAP_MONOTONIC_preferences[6] = { 300, 400, 000, 500, 200, 100 };
int CMAP_FULL_preferences[6] =      { 100, 200, 000, 300, 500, 400 };

/* This is the actual structure returned by the X server describing the
 * SERVER_OVERLAY_VISUAL property.
 */
typedef struct
{
    VisualID visualID;   /* The VisualID of the overlay visual */
    int transparentType; /* Can be None, TransparentPixel or TransparentMask */
    int value;           /* Pixel value */
    int layer;           /* Overlay planes will always be in layer 1 */
} OverlayVisualPropertyRec;

/* This is one of the possible values of the "transparentType" above. */
#ifndef TransparentPixel
#define TransparentPixel 1
#endif

/******************************************************************************
 *
 * SetServerOverlayVisualsProperty()
 *
 * This routine sets the SERVER_OVERLAY_VISUALS property for the screen
 * of widget w.
 *
 * NOTE: This code won't be necessary in the future once all servers that
 * support overlays set this property.
 *
 ******************************************************************************/

#ifdef __STDC__ /* FN PROTO [ */
static void SetServerOverlayVisualsProperty(Widget w)
#else  /* __OLD_K&R_C ] [ */
static void SetServerOverlayVisualsProperty(w)
    Widget w;
#endif /* __OLD_K&R_C ] */
{
    OverlayVisualPropertyRec *pOVis;
    Atom overlayVisualsAtom; /* Parameters for XGetWindowProperty */

    /* On "old" HP "combined mode" devices, the default visual is the one
     * and only overlay visual.  These devices really support a transparent
     * pixel "value" (which is what the overlay planes are painted with above
     * image planes windows), but they don't yet advertise that pixel value
     * (be careful, with what you look at down there).  The transparent pixel
     * "value" is the same number as the number of colormap entries.  For
     * example, if the device returns a 4-plane overlay visual, the
     * "map_entries" value below is 15 (the colormap has 15 allocatable
     * colors: 0-14), and the transparent pixel value is 15.  If the device
     * returns a 3-plane visual, the "map_entries" value below is 7, and the
     * transparent pixel value is 7.
     */
    pOVis = ((OverlayVisualPropertyRec *)
	XtMalloc(sizeof(OverlayVisualPropertyRec)));
    if (pOVis == NULL) return;
    pOVis->visualID = DefaultVisualOfScreen(XtScreen(w))->visualid;
    pOVis->transparentType = TransparentPixel;
    pOVis->value = DefaultVisualOfScreen(XtScreen(w))->map_entries;
    pOVis->layer = 1;

    overlayVisualsAtom =
	XInternAtom(XtDisplay(w), "SERVER_OVERLAY_VISUALS", False);
    XChangeProperty(XtDisplay(w), RootWindowOfScreen(XtScreen(w)),
	overlayVisualsAtom, overlayVisualsAtom, 32, PropModeReplace,
	(unsigned char *) pOVis, 4);
    XSync(XtDisplay(w), 0);
    XtFree((char *) pOVis);
} /* SetServerOverlayVisualsProperty() */


/******************************************************************************
 *
 * ChooseVisual()
 *
 * This routine takes a Starbase widget and sets the best visual and depth
 * for the widget based on its screen, its overlay resource, and its
 * shade_mode resource.  The class of the choosen visual is returned in
 * *class.  The visual can be explictly requested by a non-NULL visual
 * parameter.
 *
 ******************************************************************************/

#ifdef __STDC__ /* FN PROTO [ */
static void ChooseVisual(Widget w, int *class, Visual *visual)
#else  /* __OLD_K&R_C ] [ */
static void ChooseVisual(w, class, visual)
    Widget w;
    int *class;
    Visual *visual;
#endif /* __OLD_K&R_C ] */
{
    XVisualInfo template; /* Requirements for XGetVisualInfo */
    XVisualInfo *pVisuals;
    XVisualInfo *pVis;
    OverlayVisualPropertyRec *pOverlayVisuals;
    OverlayVisualPropertyRec *pOVis;
    int nVisuals, numOverlayVisuals, nOVisuals;
    Atom overlayVisualsAtom;     /* Parameters for XGetWindowProperty */
    Atom actualType;
    int numItems, actualFormat, bytesAfter;
    int overlayVisual;
    int transparentPixel;
    int bestScore = -1;
    int score;
    int screenNumber;

    screenNumber = ScreenCount(XtDisplay(w));
    while (--screenNumber > 0) {
	if (ScreenOfDisplay(XtDisplay(w), screenNumber) == XtScreen(w))
	    break;
    }

    /* First, get the list of visuals for this screen. */
    template.screen = screenNumber;
    pVisuals =
	XGetVisualInfo(XtDisplay(w), VisualScreenMask, &template, &nVisuals);

    /* Now, get the overlay visual information for this screen.  To obtain
     * this information, get the SERVER_OVERLAY_VISUALS property.
     */
    overlayVisualsAtom =
	XInternAtom(XtDisplay(w), "SERVER_OVERLAY_VISUALS", True);
    if (overlayVisualsAtom == None &&
	strcmp(ServerVendor(XtDisplay(w)), "Hewlett-Packard Company") == 0 &&
	XHPGetServerMode(XtDisplay(w), screenNumber) == XHPCOMBINED_MODE) {
	/* This is an old X server that supports overlay plane windows, but
	 * doesn't set the SERVER_OVERLAY_VISUALS propery.  Call a routine
	 * to set the property, then retry.
	 * NOTE: This code won't be necessary in the future once all servers
	 * that support overlays set this property.
	 */
	SetServerOverlayVisualsProperty(w);
	overlayVisualsAtom =
	    XInternAtom(XtDisplay(w), "SERVER_OVERLAY_VISUALS", True);
    }
    if (overlayVisualsAtom != None) {
	/* Since the Atom exists, we can request the property's contents.
	 * Accept information on up to 200 overlay visuals.
	 */
	XGetWindowProperty(XtDisplay(w),
	    RootWindowOfScreen(XtScreen(w)), /* window for property */
	    overlayVisualsAtom, /* Property name */
	    0, /* offset in longs */
	    200 * sizeof(OverlayVisualPropertyRec) / 4, /* max size in longs */
	    False, /* don't delete property */
	    overlayVisualsAtom, /* type required */
	    &actualType, /* returned actual type */
	    &actualFormat, /* returned actual format */
	    (unsigned long*) &numItems, /* number of items returned */
	    (unsigned long*) &bytesAfter, /* number of unread bytes */
	    (unsigned char**) &pOverlayVisuals);

	/* Calculate the number of overlay visuals in the list. */
	numOverlayVisuals = (numItems*4) / sizeof(OverlayVisualPropertyRec);
    } else {
	/* This screen doesn't have overlay planes. */
	numOverlayVisuals = 0;
	pOverlayVisuals = NULL;
    }

    /* Process the pVisuals array. */
    for (pVis = pVisuals; --nVisuals >= 0; pVis++) {
	nOVisuals = numOverlayVisuals;
	overlayVisual = False;
	transparentPixel = -1; /* no transparent pixel by default. */
	for (pOVis = pOverlayVisuals; --nOVisuals >= 0; pOVis++) {
	    if (pVis->visualid == pOVis->visualID)
	    {
		overlayVisual = True;
		if (pOVis->transparentType == TransparentPixel) {
		    transparentPixel = pOVis->value;
		}
		break;
	    }
	}

	if (visual == pVis->visual) {
	    /* We have a perfect match for a requested visual. */
	    Depth(w) = pVis->depth;
	    sbVisual(w) = pVis->visual;
	    Transparent(w) = transparentPixel;
	    *class = pVis->class;
	    break; /* Break out of visuals loop.  This is the one. */
	}

	if (pVis->class == PseudoColor && pVis->depth > 8) {
	    /* Pseudocolor with over 8 planes is usually not as good as
	     * it sounds.
	     */
	    continue;
	}

	/* Compute a score for this visual.  The score is weighted most
	 * heavily for matching overlay or image plane requirements, next
	 * for having a transparent pixel when overlay planes are requested,
	 * next by the preference for a visual class, and least for depth.
	 */

	switch (ShadeMode(w)) {
	case CMAP_NORMAL:
	    score = CMAP_NORMAL_preferences[pVis->class];
	    break;
	case CMAP_MONOTONIC:
	    score = CMAP_MONOTONIC_preferences[pVis->class];
	    break;
	case CMAP_FULL:
	    score = CMAP_FULL_preferences[pVis->class];
	    break;
	}

	score += pVis->depth;

	if (Overlay(w)) {
	    if (overlayVisual) {
		/* This visual is in overlay planes as requested. */
		score += 10000;
		if (transparentPixel != -1) {
		    /* This overlay visual has a transparent pixel. */
		    score += 1000;
		}
	    }
	} else {
	    if (!overlayVisual) {
		/* This visual is in image planes as requested. */
		score += 10000;
	    }
	}

	if (score > bestScore) {
	    bestScore = score;
	    Depth(w) = pVis->depth;
	    sbVisual(w) = pVis->visual;
	    Transparent(w) = transparentPixel;
	    *class = pVis->class;
	}
    }

    /* Free any allocated memory */
    XFree((char *) pVisuals);
    if (pOverlayVisuals)
	XFree((char *) pOverlayVisuals);
} /* ChooseVisual() */


#ifdef __STDC__ /* FN PROTO [ */
static void Realize(
    Widget w,
    Mask *mask,
    XSetWindowAttributes *winattr)
#else  /* __OLD_K&R_C ] [ */
static void Realize(w, mask, winattr)
    Widget w;
    Mask *mask;
    XSetWindowAttributes *winattr;
#endif /* __OLD_K&R_C ] */
{
    char *device;
    int class;

    /* Choose the best visual and get its visual class. */
    ChooseVisual(w, &class, sbVisual(w));

    /* If there wasn't a colormap explicitly specified... */
    if (Colormap(w) == DefaultColormapOfScreen(XtScreen(w))) {
	/* If this isn't the default visual, create a colormap. */
	if (sbVisual(w) == DefaultVisualOfScreen(XtScreen(w))) {
	    winattr->colormap = DefaultColormapOfScreen(XtScreen(w));
	} else {
	    winattr->colormap = XCreateColormap(XtDisplay(w),
		XtWindow(XtParent(w)), sbVisual(w),
		/* Can allocate colors if a dynamic-color visual */
		(class & 1) ? AllocAll : AllocNone);
	}
	Colormap(w) = winattr->colormap;
    }
    else {
	winattr->colormap = Colormap(w);
    }

    /* Create window. */
    winattr->border_pixel = 0;
    winattr->background_pixel = BackgroundPixel(w);
    /* Set backingstore attribute */
    if (Retained(w)) {
	winattr->backing_store = Always;
    } else {
	winattr->backing_store = NotUseful;
    }
    XtCreateWindow(w, InputOutput, sbVisual(w),
	(*mask | CWBackPixel | CWColormap | CWBorderPixel | CWBackingStore),
	winattr);

    UpdateWmCmap(w);

    device = make_X11_gopen_string(XtDisplay(w), XtWindow(w));

    /* Gopen the window.
     * Resize the window to the full screen size while gopen is
     * called so the Starbase library will be willing to resize
     * the p1-p2 area to any window size up to full screen.
     */

    if (RescalePolicy(w) == XgRESCALE_NONE) {
	MaxWidth(w) = Width(w);
	MaxHeight(w) = Height(w);
    } else {
	if (MaxWidth(w) == 0) MaxWidth(w) = WidthOfScreen(XtScreen(w));
	if (MaxHeight(w) == 0) MaxHeight(w) = HeightOfScreen(XtScreen(w));
	XResizeWindow(XtDisplay(w), XtWindow(w), MaxWidth(w), MaxHeight(w));
	XSync(XtDisplay(w), 0);
    }

    Fildes(w) = gopen(device, OUTDEV, Driver(w), OpenMode(w));

    if (RescalePolicy(w) != XgRESCALE_NONE) {
	XResizeWindow(XtDisplay(w), XtWindow(w), Width(w), Height(w));
	XSync(XtDisplay(w), 0);
    }

    free(device);

    if (Fildes(w) >= 0) {
	int cmap_mode, dbuffer_mode, planes, current_buffer;

	inquire_display_mode(Fildes(w), &cmap_mode, &dbuffer_mode,
	    &planes, &current_buffer);
	if (cmap_mode != ShadeMode(w)) {
	    shade_mode(Fildes(w), ShadeMode(w)|INIT, FALSE);
	}
	inquire_display_mode(Fildes(w), &cmap_mode, &dbuffer_mode,
	    &planes, &current_buffer);
	ShadeMode(w) = cmap_mode;
    }

    Resize((XgStarbaseWidget) w);
}


#ifdef __STDC__ /* FN PROTO [ */
static void Destroy(XgStarbaseWidget w)
#else  /* __OLD_K&R_C ] [ */
static void Destroy(w)
    XgStarbaseWidget w;
#endif /* __OLD_K&R_C ] */
{
    if (Fildes(w) >= 0) {
	gclose(Fildes(w));
    }
}

#ifdef __STDC__ /* FN PROTO [ */
static void Resize(XgStarbaseWidget w)
#else  /* __OLD_K&R_C ] [ */
static void Resize(w)
    XgStarbaseWidget w;
#endif /* __OLD_K&R_C ] */
{
    XmDrawingAreaCallbackStruct cb;

    if (Fildes(w) >= 0) { /* Resize the Starbase p1-p2 area. */
	double maxx, miny;

	if (Width(w) < MaxWidth(w)) {
	    maxx = (Width(w)-1)/((double) (MaxWidth(w)-1));
	} else {
	    maxx = 1.0;
	}
	if (Height(w) < MaxHeight(w)) {
	    miny = 1.0 - (Height(w)-1)/((double) (MaxHeight(w)-1));
	} else {
	    miny = 0.0;
	}

	switch (RescalePolicy(w)) {
	case XgRESCALE_NONE:
	    break;
	case XgRESCALE_DISTORT:
	    if ((OpenMode(w) & (INT_XFORM|INT_XFORM32)) == 0)
		hidden_surface(Fildes(w), FALSE, FALSE);
	    set_p1_p2(Fildes(w), FRACTIONAL, 0.0, miny, 0.0, maxx, 1.0, 1.0);
	    mapping_mode(Fildes(w), TRUE); /* distort */
	    break;
	case XgRESCALE_MAJOR:
	    if ((OpenMode(w) & (INT_XFORM|INT_XFORM32)) == 0)
		hidden_surface(Fildes(w), FALSE, FALSE);
	    if ((1.0 - miny) > maxx) {
		maxx = (1.0 - miny); /* scale x up to larger y scale */
	    } else {
		miny = (1.0 - maxx); /* scale y up to larger x scale */
	    }
	    set_p1_p2(Fildes(w), FRACTIONAL, 0.0, miny, 0.0, maxx, 1.0, 1.0);
	    mapping_mode(Fildes(w), FALSE); /* don't distort */
	    break;
	case XgRESCALE_MINOR:
	    if ((OpenMode(w) & (INT_XFORM|INT_XFORM32)) == 0)
		hidden_surface(Fildes(w), FALSE, FALSE);
	    set_p1_p2(Fildes(w), FRACTIONAL, 0.0, miny, 0.0, maxx, 1.0, 1.0);
	    mapping_mode(Fildes(w), FALSE); /* don't distort */
	    break;
	}
    }

    cb.reason = XmCR_RESIZE;
    cb.event = NULL;
    cb.window = XtWindow(w);

    XtCallCallbackList ((Widget) w, (XtCallbackList) ResizeCallback(w), &cb);
}

#ifdef __STDC__ /* FN PROTO [ */
static void Redisplay(
    XgStarbaseWidget w,
    XEvent *event,
    Region region)
#else  /* __OLD_K&R_C ] [ */
static void Redisplay(w, event, region)
    XgStarbaseWidget w;
    XEvent *event;
    Region region;
#endif /* __OLD_K&R_C ] */
{
    XmDrawingAreaCallbackStruct cb;

    cb.reason = XmCR_EXPOSE;
    cb.event = event;
    cb.window = XtWindow(w);

    _XmRedisplayGadgets( (Widget) w, event, region);

    XtCallCallbackList ((Widget) w, (XtCallbackList) ExposeCallback(w), &cb);
}

#ifdef __STDC__ /* FN PROTO [ */
static Boolean SetValues(
    XgStarbaseWidget current,
    XgStarbaseWidget request,
    XgStarbaseWidget new)
#else  /* __OLD_K&R_C ] [ */
static Boolean SetValues(current, request, new)
    XgStarbaseWidget current;
    XgStarbaseWidget request;
    XgStarbaseWidget new;
#endif /* __OLD_K&R_C ] */
{
    Boolean redraw = False;

    /* Range check settable resources. */
    switch(WmCmap(request)) {
    case XgWM_CMAP_NONE: /* Make no change to WM_COLORMAP_WINDOWS */
    case XgWM_CMAP_LOW_PRIORITY: /* Add to bottom of WM_COLORMAP_WINDOWS */
    case XgWM_CMAP_HIGH_PRIORITY: /* Add to top of WM_COLORMAP_WINDOWS */
	break;
    default:
	WmCmap(new) = XgWM_CMAP_LOW_PRIORITY;
	break;
    }

    switch (RescalePolicy(request)) {
    case XgRESCALE_NONE:
    case XgRESCALE_DISTORT:
    case XgRESCALE_MAJOR:
    case XgRESCALE_MINOR:
	break;
    default:
	RescalePolicy(new) = XgRESCALE_MINOR;
	break;
    }

    if (XtIsRealized(current)) {
	/* Refuse any changes to these resources.  They are read only. */
	OpenMode(new) = OpenMode(current);
	ShadeMode(new) = ShadeMode(current);
	Overlay(new) = Overlay(current);
	Transparent(new) = Transparent(current);
	sbVisual(new) = sbVisual(current);
	Driver(new) = Driver(current);
	Fildes(new) = Fildes(current);
	MaxWidth(new) = MaxWidth(current);
	MaxHeight(new) = MaxHeight(current);

	/* Update backing_store for realized widgets. */
	if (Retained(new) != Retained(current)) {
	    XSetWindowAttributes attr;
	    if (Retained(new)) {
		attr.backing_store = Always;
	    } else {
		attr.backing_store = NotUseful;
	    }
	    XChangeWindowAttributes(XtDisplay(new), XtWindow(new),
		CWBackingStore, &attr);
	}
    }

    if (RescalePolicy(new) != RescalePolicy(current)) {
	Resize(new);
	redraw = True;
    }

    if (WmCmap(new) != WmCmap(current)) {
	UpdateWmCmap((Widget) new);
    }

    return (redraw);
}
#endif /* !__linux__ */

#endif
