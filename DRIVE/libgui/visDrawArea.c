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
#include <Xm/Xm.h>
#ifdef _IBM_SOURCE
#include <X11/StringDefs.h>
#endif
#include <Mrm/MrmPublic.h>
#include "visDrawAreaP.h"

#ifndef _HPUX_SOURCE
static int XHPGetServerMode(Display *d,int s) { return(0); }
# define XHPCOMBINED_MODE 1
#else /* is HP */
# include <X11/XHPproto.h>
#endif /* !HP else */

static char ID_string[] = "@(#) Visual Drawing Area Widget, DRIVE Version";
#define Overlay(w) XmField(w,offsets,XgVisualDrawingArea,overlay,Boolean)
#define Transparent(w) XmField(w,offsets,XgVisualDrawingArea,transparent,int)
#define vdaVisual(w) XmField(w,offsets,XgVisualDrawingArea,visual,Visual *)
#define WmCmap(w) XmField(w,offsets,XgVisualDrawingArea,wm_cmap,int)

#define ExposeCallback(w) XmField(w,offsets,XmDrawingArea,expose_callback, \
    caddr_t)
#define ResizeCallback(w) XmField(w,offsets,XmDrawingArea,resize_callback, \
    caddr_t)

#define BackgroundPixel(w) XmField(w,offsets,Core,background_pixel,Pixel)
#define Depth(w) XmField(w,offsets,Core,depth,int)
#define Colormap(w) XmField(w,offsets,Core,colormap,int)
#define Width(w) XmField(w,offsets,Core,width,Dimension)
#define Height(w) XmField(w,offsets,Core,height,Dimension)

#ifndef _HPUX_SOURCE
#define XmInheritTranslations XtInheritTranslations
#endif

static void ClassInitialize(void);
static void Realize(
    Widget w,
    Mask *mask,
    XSetWindowAttributes *winattr);
static void Redisplay(
    XgVisualDrawingAreaWidget w,
    XEvent *event,
    Region region);
static void Resize(XgVisualDrawingAreaWidget w);
static void Destroy(XgVisualDrawingAreaWidget w);
static Boolean SetValues(
    XgVisualDrawingAreaWidget current,
    XgVisualDrawingAreaWidget request,
    XgVisualDrawingAreaWidget new);

static XmPartResource resources[] = {
    {XgNoverlay, XgCOverlay, XmRBoolean, sizeof(Boolean),
	XmPartOffset(XgVisualDrawingArea,overlay), XmRImmediate, False},
    {XgNtransparent, XgCTransparent, XmRInt, sizeof(int),
	XmPartOffset(XgVisualDrawingArea,transparent), XmRImmediate, (caddr_t)-1},
    {XgNvisual, XgCVisual, XtRVisual, sizeof(Visual *),
	XmPartOffset(XgVisualDrawingArea,visual), XmRImmediate, NULL},
    {XgNwmCmap, XgCWmCmap, XgRWmCmap, sizeof(int),
	XmPartOffset(XgVisualDrawingArea,wm_cmap), XmRImmediate,
	(caddr_t)XgWM_CMAP_HIGH_PRIORITY},
};

XgVisualDrawingAreaClassRec xgVisualDrawingAreaClassRec = {
    {                                   /* core_class fields    */
    (WidgetClass) &xmDrawingAreaClassRec,/* superclass           */
    "XgVisualDrawingArea",                       /* class_name           */
    sizeof(XgVisualDrawingAreaRec),              /* widget_size          */
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

externaldef(xgstarbasewidgetclass) WidgetClass xgVisualDrawingAreaWidgetClass =
    (WidgetClass) &xgVisualDrawingAreaClassRec;

static XmOffsetPtr offsets; /* Part Offset table for XmResolvePartOffsets */

/**********************************************************************
 *
 * XgCreateVisualDrawingArea - Convenience routine, used by Uil/Mrm.
 *
 *********************************************************************/

Widget XgCreateVisualDrawingArea(
    Widget parent,
    String name,
    ArgList arglist,
    Cardinal nargs)
{
    return(XtCreateWidget(name, xgVisualDrawingAreaWidgetClass, parent,
	arglist, nargs));
}

/**********************************************************************
 *
 * VisualDrawingAreaMrmInitialize - register VisualDrawingArea widget class with Mrm
 *
 *********************************************************************/

int XgVisualDrawingAreaMrmInitialize(void)
{
    return(MrmRegisterClass (MrmwcUnknown, "VisualDrawingArea",
	"XgCreateVisualDrawingArea",
	XgCreateVisualDrawingArea, (WidgetClass) &xgVisualDrawingAreaClassRec));
}

/**********************************************************************
 *
 * Class methods
 *
 *********************************************************************/

static void ConversionWarning(String string, String type)
{
    String params[2];
    Cardinal num_params = 2;
    extern XtAppContext _XtDefaultAppContext(void);

    params[0] = string;
    params[1] = type;
    XtAppWarningMsg((XtAppContext) _XtDefaultAppContext(),
	       "conversionError", "string", "XtToolkitError",
	       "Cannot convert string \"%s\" to type %s",
		params, &num_params);
}


static Boolean CvtStringToWMCmap(
    XrmValuePtr args,
    Cardinal *num_args,
    XrmValuePtr fromVal,
    XrmValuePtr toVal)
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


static void ClassInitialize(void)
{
    XmResolvePartOffsets(xgVisualDrawingAreaWidgetClass, &offsets);
    XtAddConverter(XtRString, XgRWmCmap,
	(XtConverter) CvtStringToWMCmap, NULL, 0);
}


static void UpdateWmCmap(Widget w)
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

static void SetServerOverlayVisualsProperty(Widget w)
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
 * This routine takes a VisualDrawingArea widget and sets the best visual and depth
 * for the widget based on its screen, its overlay resource, and its
 * shade_mode resource.
 *
 ******************************************************************************/

static void ChooseVisual(
    Widget w,
    int *class,
    const Visual *visual)
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

	if ((visual != NULL)
	        && (visual->visualid == pVis->visual->visualid)) {
	    /* We have a perfect match for a requested visual. */
	    Depth(w) = pVis->depth;
	    vdaVisual(w) = pVis->visual;
	    Transparent(w) = transparentPixel;
	    *class = pVis->class;
	    break;  /* this is the one! */
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

	score = pVis->depth;

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
	    vdaVisual(w) = pVis->visual;
	    Transparent(w) = transparentPixel;
	    *class = pVis->class;
	}
    }

    /* Free any allocated memory */
    XFree((char *) pVisuals);
    if (pOverlayVisuals)
	XFree((char *) pOverlayVisuals);
} /* ChooseVisual() */


static void Realize(
    Widget w,
    Mask *mask,
    XSetWindowAttributes *winattr)
{
    int class;

    /* Choose the best visual and get its visual class. */
    ChooseVisual(w,&class,vdaVisual(w));

    /* If there wasn't a colormap explicitly specified... */
    if (Colormap(w) == DefaultColormapOfScreen(XtScreen(w))) {
	/* If this isn't the default visual, create a colormap. */
	if (vdaVisual(w) == DefaultVisualOfScreen(XtScreen(w))) {
	    winattr->colormap = DefaultColormapOfScreen(XtScreen(w));
	} else {
	    winattr->colormap = XCreateColormap(XtDisplay(w),
		XtWindow(XtParent(w)), vdaVisual(w),
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
    XtCreateWindow(w, InputOutput, vdaVisual(w),
	(*mask | CWBackPixel | CWColormap | CWBorderPixel), winattr);

    UpdateWmCmap(w);

    XResizeWindow(XtDisplay(w), XtWindow(w), Width(w), Height(w));
    XSync(XtDisplay(w), 0);

    Resize((XgVisualDrawingAreaWidget) w);
}


static void Destroy(XgVisualDrawingAreaWidget w)
{
}

static void Resize(XgVisualDrawingAreaWidget w)
{
    XmDrawingAreaCallbackStruct cb;

    cb.reason = XmCR_RESIZE;
    cb.event = NULL;
    cb.window = XtWindow(w);

    XtCallCallbackList ((Widget) w, (XtCallbackList) ResizeCallback(w), &cb);
}

static void Redisplay(
    XgVisualDrawingAreaWidget w,
    XEvent *event,
    Region region)
{
    XmDrawingAreaCallbackStruct cb;

    cb.reason = XmCR_EXPOSE;
    cb.event = event;
    cb.window = XtWindow(w);

    _XmRedisplayGadgets( (Widget) w, event, region);

    XtCallCallbackList ((Widget) w, (XtCallbackList) ExposeCallback(w), &cb);
}

static Boolean SetValues(
    XgVisualDrawingAreaWidget current,
    XgVisualDrawingAreaWidget request,
    XgVisualDrawingAreaWidget new)
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

    if (XtIsRealized(current)) {
	/* Refuse any changes to these resources.  They are read only. */
	Overlay(new) = Overlay(current);
	Transparent(new) = Transparent(current);
	vdaVisual(new) = vdaVisual(current);
    }

    if (WmCmap(new) != WmCmap(current)) {
	UpdateWmCmap((Widget) new);
    }

    return (redraw);
}
#endif
