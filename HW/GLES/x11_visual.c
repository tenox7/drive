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

#include <stdlib.h>
#include <string.h>

#include "hw.h"
#include "hw_internal.h"
#include "hw_gl.h"

static Atom WM_DELETE_WINDOW;
static Atom _MOTIF_WM_HINTS;

static GLXContext
    origContext;

/* Some utility functions (see below) */
static void setP1P2( glDrawable *draw,
                hwInt32 x, hwInt32 y, hwInt32 w, hwInt32 h );
static hwInt32 InitGraphicsFildes( glDrawable *draw );

/* hwInit - initialize the X display connection */
hwDisplay __hwGlCreate
(
    hwDisplay disp, hwDisplay shared,
    OS_DISPLAY_TYPE display
)
{
    glDisplay
        *result;

    result = malloc( sizeof(glDisplay) );
    if( !result )       return 0;

    memset(result, 0, sizeof(glDisplay));

    result->disp.vtab = __hwGlInitFunc;
    __hwInternalInitDisplay( (hwDisplay)result, shared );

    __hwGlGetEnvVars( result );

    result->xdisp = display;
    if( !result->xdisp ) {
       result->xdisp = XOpenDisplay( (char *)0 );
    }
    if( !result->xdisp ) {
        free( result );
        return 0;
    }
    result->screen = DefaultScreen( result->xdisp );
    result->w = XDisplayWidth( result->xdisp, result->screen );
    result->h = XDisplayHeight( result->xdisp, result->screen );

    /* Set up the current time-of-day clock */
    result->glob.startTime = result->glob.currTime = hwGetSysTime();

    return (hwDisplay)result;
}

/* hwChooseVisual - choose an X visual which matches the request */
hwInt32 __hwGlChooseVisual
(
    hwDisplay display, hwInt32 required, hwInt32 desired
)
{
    int
        attrs[128], i;
    XVisualInfo
        *vi;
    glDisplay
        *disp = (glDisplay *)display;;

    /* Combine required flags into desired flags */
    desired |= required;

    /* Initialize gl attribute array */
    i = 0;
    attrs[i++] = GLX_USE_GL;

    if( desired & HW_VIS_OVERLAY ) {
        /* Ask for at *least* 8-bit indexed overlay planes */
        attrs[i++] = GLX_BUFFER_SIZE;
        attrs[i++] = 8;
        attrs[i++] = GLX_LEVEL;
        attrs[i++] = 1;
    }
    else {
        /* Ask for at *least* 332 RGB visual */
        attrs[i++] = GLX_RGBA;
        attrs[i++] = GLX_RED_SIZE;
        attrs[i++] = 3;
        attrs[i++] = GLX_GREEN_SIZE;
        attrs[i++] = 3;
        attrs[i++] = GLX_BLUE_SIZE;
        attrs[i++] = 2;
    }
    if( desired & HW_VIS_DBUFF ) {
        attrs[i++] = GLX_DOUBLEBUFFER;
    }
    if( desired & HW_VIS_DEPTH ) {
        attrs[i++] = GLX_DEPTH_SIZE;
        attrs[i++] = 16;
    }
    if( desired & HW_VIS_ALPHA ) {
        attrs[i++] = GLX_ALPHA_SIZE;
        attrs[i++] = 4;
    }
    if( desired & HW_VIS_STENCIL ) {
        attrs[i++] = GLX_STENCIL_SIZE;
        attrs[i++] = 1;
    }
    if( desired & HW_VIS_STEREO ) {
        attrs[i++] = GLX_STEREO;
    }

    attrs[i++] = None;

    vi = glXChooseVisual( disp->xdisp, disp->screen, attrs );
    if( !vi )   return 0;

    /* Finally, see which features are actually present */
    if( glXGetConfig( disp->xdisp, vi, GLX_BUFFER_SIZE, &i ) == 0 ) {
        if( i < 8 )     desired &= ~HW_VIS_OVERLAY;
    }
    if( glXGetConfig( disp->xdisp, vi, GLX_LEVEL, &i ) == 0 ) {
        if( i < 1 )     desired &= ~HW_VIS_OVERLAY;
    }
    if( glXGetConfig( disp->xdisp, vi, GLX_RGBA, &i ) == 0 ) {
        if( i )         desired &= ~HW_VIS_OVERLAY;
        else            /* TBD */;
    }
    if( glXGetConfig( disp->xdisp, vi, GLX_DOUBLEBUFFER, &i ) == 0 ) {
        if( !i )        desired &= ~HW_VIS_DBUFF;
    }
    if( glXGetConfig( disp->xdisp, vi, GLX_DEPTH_SIZE, &i ) == 0 ) {
        if( i < 16 )    desired &= ~HW_VIS_DEPTH;
    }
    if( glXGetConfig( disp->xdisp, vi, GLX_ALPHA_SIZE, &i ) == 0 ) {
        if( i < 4 )     desired &= ~HW_VIS_ALPHA;
    }
    if( glXGetConfig( disp->xdisp, vi, GLX_STENCIL_SIZE, &i ) == 0 ) {
        if( i < 1 )     desired &= ~HW_VIS_STENCIL;
    }

    /* See if required criteria are met */
    if( (required & desired) != required ) {
        /* Oops - can't do it */
        return 0;
    }

    /* Stash away the result */
    disp->vis = *vi;
    disp->dbuffer = (desired & HW_VIS_DBUFF) ? 1 : 0;

    /* Return what we actually got */
    return desired | HW_VIS_SUCCESS;
}

OS_VISUAL_TYPE __hwGlExtractVisual( hwDisplay display )
{
    glDisplay
        *disp = (glDisplay *)display;;

    return &disp->vis;
}

OS_DRAWABLE_TYPE __hwGlExtractWin( hwDisplay disp, hwDrawable draw )
{
    glDrawable
        *win = (glDrawable *)draw;

    return win->win;
}

/* Create a stoopid window of the given characteristics */
hwDrawable hwGlCreateWinUtility
(
    hwDisplay display,  /* Display to use */
    const char *winName,      /* Name of the window to create */
    Window  parent,     /* Name of the parent window, NULL if not a child */
    hwInt32 winX, hwInt32 winY, /* Placement */
    hwInt32 winW, hwInt32 winH, /* Size */
    hwInt32 flags
)
{
    XSetWindowAttributes
        Attr;
    XSizeHints
        SizeHints;
    Colormap
        XCMap;
    XVisualInfo
        *VI;
    XEvent
        Report;
    unsigned long
        valuemask;
    glDrawable
        *Result;
    glDisplay
        *disp = (glDisplay *)display;

    Result = malloc( sizeof(glDrawable) );
    if( !Result ) {
        return NULL;
    }

    memset(Result, 0, sizeof(glDrawable) );

    Result->disp = disp;
    Result->buffer = 0;

    XCMap = XCreateColormap( disp->xdisp,
                        RootWindow( disp->xdisp, disp->screen ),
                        disp->vis.visual, AllocNone );

    valuemask = CWColormap | CWBackPixel | CWBorderPixel;

    if (flags & HW_WIN_FULLSCREEN) {
        winX = 0;
        winY = 0;
        winW = disp->w;
        winH = disp->h;
        Attr.override_redirect = True;
        valuemask |= CWOverrideRedirect;
    }

    Attr.colormap = XCMap;
    Attr.background_pixel = 0;
    Attr.border_pixel = 0;

    if( parent == (Window)0 ) {
        Result->win = XCreateWindow( disp->xdisp,
                        RootWindow( disp->xdisp, disp->screen ),
                        winX, winY, winW, winH, 2,
                        disp->vis.depth, CopyFromParent,
                        disp->vis.visual,
                        valuemask, &Attr );
    }
    else {
        Result->win = XCreateWindow( disp->xdisp,
                        parent,
                        winX, winY, winW, winH, 2,
                        disp->vis.depth, CopyFromParent,
                        disp->vis.visual,
                        valuemask, &Attr );
    }
    if( !Result->win ) {
        free( Result );
        return 0;
    }

    SizeHints.flags = USPosition | USSize | PMinSize;
    SizeHints.x = winX; SizeHints.y = winY;
    SizeHints.min_width =  winW/2;
    SizeHints.width = winW;
    SizeHints.min_height =  winH/2;
    SizeHints.height = winH;
    XSetStandardProperties( disp->xdisp, Result->win,
                winName, winName,
                None, 0, 0, &SizeHints );

    Result->gc = XCreateGC( disp->xdisp, Result->win, 0, NULL );


    if( !InitGraphicsFildes( Result ) ) {
            XDestroyWindow( disp->xdisp, Result->win );
            free( Result );
            return 0;
    }

    /* Map the window */
    XMapWindow( disp->xdisp, Result->win );

    if (flags & HW_WIN_FULLSCREEN) {
        XFlush( disp->xdisp );
        XSetInputFocus( disp->xdisp, Result->win, RevertToParent, CurrentTime );
    }

    if (flags & HW_WIN_INPUT) {
        XSelectInput( disp->xdisp, Result->win,
                        KeyPressMask |
                        ButtonPressMask |
                        ButtonReleaseMask |
                        PointerMotionMask |
                        ExposureMask |
                        StructureNotifyMask |
                        FocusChangeMask );
        if (!WM_DELETE_WINDOW) {
            WM_DELETE_WINDOW = XInternAtom( disp->xdisp,
                                "WM_DELETE_WINDOW", False );
        }
        (void)XSetWMProtocols( disp->xdisp, Result->win, &WM_DELETE_WINDOW, 1 );
    } else {
        XSelectInput( disp->xdisp, Result->win, ExposureMask );
    }

    if (!(flags & HW_WIN_FULLSCREEN)) {
        do {
            XPeekEvent( disp->xdisp, &Report );
            if( Report.type != Expose ) {
                XNextEvent( disp->xdisp, &Report );
            }
        } while( Report.type != Expose );
    }

    if (!(flags & HW_WIN_INPUT)) {
        XSelectInput( disp->xdisp, Result->win, NoEventMask );
    }

    return (hwDrawable)Result;
}

hwDrawable __hwGlCreateWin
(
    hwDisplay display,  /* Display to use */
    const char *winName,      /* Name of the window to create */
    hwInt32 winX, hwInt32 winY, /* Placement */
    hwInt32 winW, hwInt32 winH, /* Size */
    hwInt32 flags
)
{
    return hwGlCreateWinUtility(display, winName, (Window)0,
                winX, winY, winW, winH, flags);
}

hwDrawable __hwGlCreateChildWin
(
    hwDisplay display,  /* Display to use */
    const char *winName,      /* Name of the window to create */
    OS_WINDOW_TYPE parent,     /* handle of the the parent window */
    hwInt32 winX, hwInt32 winY, /* Placement */
    hwInt32 winW, hwInt32 winH, /* Size */
    hwInt32 flags
)
{
    return hwGlCreateWinUtility(display,winName, parent,
                                winX, winY, winW, winH,flags);
}

void __hwGlInputHandler
(
    hwDisplay display,
    void (*callback)( hwDrawable, hwWinEvent * )
)
{
    USE_GL_CTX(display);

    if( !glctx ) return;

    glctx->eventCB = callback;
}

void __hwGlCheckInput( hwDisplay display )
{
    USE_GL_CTX(display);
    int
        i, n, numEvents;
    XEvent
        report;
    XWindowAttributes
        attr;
    KeySym
        KS;
    char
        buff[16];
    hwWinEvent
        event;
    glDisplay
        *disp = (glDisplay *)display;
    static hwInt32
        clipListAlloc = 0,
        *clipList = NULL;
    void
        (*callback)( hwDrawable, hwWinEvent * ) = NULL;

    if( glctx ) {
        callback = glctx->eventCB;
    }

    if( !callback ) return;

    /* TBD: We need a way to find the hwDrawable from the X window handle */

    numEvents = XPending( disp->xdisp );
    while (numEvents > 0) {
        XNextEvent( disp->xdisp, &report );
        switch (report.type) {
        case ClientMessage :
            if (report.xclient.data.l[0] == WM_DELETE_WINDOW) {
                event.type = HW_INPUT_DESTROY;
                (*callback)( (hwDrawable)glctx, &event );
            }
            break;

        case ButtonPress :
        case ButtonRelease :
            event.button.type = (report.type == ButtonPress)
                        ? HW_INPUT_BUTTON_PRESS
                        : HW_INPUT_BUTTON_RELEASE;
            event.button.x = report.xbutton.x;
            event.button.y = report.xbutton.y;
            event.button.button = report.xbutton.button;
            event.button.kbdMods = 0;
            if (report.xbutton.state & (ShiftMask|LockMask)) {
                event.button.kbdMods |= HW_KBD_MOD_SHIFT;
            }
            if (report.xbutton.state & ControlMask) {
                event.button.kbdMods |= HW_KBD_MOD_CTRL;
            }
            if (report.xbutton.state &
                (Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
            {
                event.button.kbdMods |= HW_KBD_MOD_ALT;
            }
            (*callback)( (hwDrawable)glctx, &event );
            break;
        
        case MotionNotify :
            event.pointer.type = HW_INPUT_POINTER;
            event.pointer.x = report.xmotion.x;
            event.pointer.y = report.xmotion.y;
            event.pointer.buttonState = 0;
            event.pointer.kbdMods = 0;
            if( report.xmotion.state & Button1Mask ) {
                event.pointer.buttonState |= 1;
            }
            if( report.xmotion.state & Button2Mask ) {
                event.pointer.buttonState |= 2;
            }
            if( report.xmotion.state & Button3Mask ) {
                event.pointer.buttonState |= 4;
            }
            if (report.xmotion.state & (ShiftMask|LockMask)) {
                event.pointer.kbdMods |= HW_KBD_MOD_SHIFT;
            }
            if (report.xmotion.state & ControlMask) {
                event.pointer.kbdMods |= HW_KBD_MOD_CTRL;
            }
            if (report.xmotion.state &
                (Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
            {
                event.pointer.kbdMods |= HW_KBD_MOD_ALT;
            }
            (*callback)( (hwDrawable)glctx, &event );
            break;
        case KeyPress :
            n = 0;
            i = XLookupString( (void *)&report, buff, 16, &KS, 0L );
            switch( KS ) {
            case XK_Left :              n = HW_KEY_LEFT;        break;
            case XK_Up :                n = HW_KEY_UP;          break;
            case XK_Right :             n = HW_KEY_RIGHT;       break;
            case XK_Down :              n = HW_KEY_DOWN;        break;
            case XK_Prior :             n = HW_KEY_PREV;        break;
            case XK_Next :              n = HW_KEY_NEXT;        break;
            case XK_Home :              n = HW_KEY_HOME;        break;
            case XK_End :               n = HW_KEY_END;         break;
            case XK_F1 : case XK_F2 : case XK_F3 :
            case XK_F4 : case XK_F5 : case XK_F6 :
            case XK_F7 : case XK_F8 : case XK_F9 :
            case XK_F10 : case XK_F11 : case XK_F12 :
                n = HW_KEY_F1 + (report.xkey.keycode - XK_F1);
                break;
            default :
                if( i == 1 ) {
                    n = buff[0];
                }
                break;
            }
            if( n ) {
                event.keyboard.type = HW_INPUT_KEYBOARD;
                event.keyboard.key = n;
                (*callback)( (hwDrawable)glctx, &event );
            }
            break;
        case Expose :
            event.expose.type = HW_INPUT_EXPOSE;
            n = 1 + report.xexpose.count;
            if( n > clipListAlloc ) {
                clipListAlloc = n;
                clipList = realloc( clipList, n * 4 * sizeof(hwInt32) );
            }
            i = 0;
            do {
                if( clipList ) {
                    clipList[4*i  ] = report.xexpose.x;
                    clipList[4*i+1] = report.xexpose.y;
                    clipList[4*i+2] = report.xexpose.width;
                    clipList[4*i+3] = report.xexpose.height;
                    i++;
                }
                n--;
                if( n > 0 ) {
                    XNextEvent( disp->xdisp, &report );
                    numEvents--;
                }
            } while( n > 0 );
            event.expose.numClipRects = i;
            event.expose.clipRectList = clipList;
            (*callback)( (hwDrawable)glctx, &event );
            break;
        case ConfigureNotify :
            event.config.type = HW_INPUT_CONFIG;
            event.config.posx = report.xconfigure.x;
            event.config.posy = report.xconfigure.y;
            event.config.width = report.xconfigure.width;
            event.config.height = report.xconfigure.height;
            (*callback)( (hwDrawable)glctx, &event );
            break;
        }
        numEvents--;
    }
}

void __hwGlGetMousePos( hwDisplay display, hwInt32 *x, hwInt32 *y )
{
    USE_GL_CTX(display);
    glDisplay
        *disp = (glDisplay *)display;
    Window
        root, child;
    int
        same_screen, root_x, root_y, win_x, win_y;
    unsigned int
        keys_buttons;

    /* Get the mouse position from cstate.viewScreenWindow */
    same_screen = XQueryPointer( disp->xdisp, glctx->win,
                                &root, &child, &root_x, &root_y,
                                &win_x, &win_y, &keys_buttons );
    *x = win_x;
    *y = win_y;
}

hwDrawable __hwGlInitDrawable( hwDisplay display, OS_DRAWABLE_TYPE draw )
{
    glDisplay
        *disp = (glDisplay *)display;
    glDrawable
        *Result;
    int
        nitems_return;
    XVisualInfo
        vinfo_template,
        *visReturn;
    XWindowAttributes 
        wattrs;

    Result = malloc( sizeof(glDrawable) );
    if( !Result ) {
        return 0;
    }
    memset(Result, 0, sizeof(glDrawable) );

    Result->disp = disp;
    Result->buffer = 0;

    Result->win = draw;
    Result->gc = XCreateGC( disp->xdisp, Result->win, 0, NULL );
    if(disp->vis.visualid == 0 )  {
        /* Hey! we don't have a valid XVisualInfo structure yet!
        ** We need one before we can call InitGraphicsFildes()
        */
        XGetWindowAttributes(disp->xdisp, Result->win, &wattrs);
        vinfo_template.visualid = XVisualIDFromVisual(wattrs.visual);
        visReturn = XGetVisualInfo(disp->xdisp, VisualIDMask, 
            &vinfo_template, &nitems_return);
        if( nitems_return <= 0 ) {
            printf("Can't find XVisualInfo structures for this window!\n");
        }
        else {
            /* What to do if there are more than one? */
            memcpy(&(disp->vis), visReturn, sizeof(XVisualInfo));
        }
    }

    if( !InitGraphicsFildes( Result ) ) {
        free( Result );
        return 0;
    }

    return (hwDrawable)Result;
}

static void setP1P2( glDrawable *draw,
                hwInt32 x, hwInt32 y, hwInt32 w, hwInt32 h )
{
    XWindowAttributes
        Attrs;

    if( !draw ) 
        return;

    XGetWindowAttributes( draw->disp->xdisp, draw->win, &Attrs );
    draw->VDC_XMax = draw->winW = w;
    draw->VDC_YMax = draw->winH = h;
    glViewport( x, Attrs.height-(y+h), w, h );
}

static hwInt32 InitGraphicsFildes( glDrawable *draw )
{
    Display
        *disp = draw->disp->xdisp;
    Window
        win = draw->win;
    GLXContext
        ctx;
    XWindowAttributes
        Attrs;

    ctx = glXCreateContext( disp, &draw->disp->vis, origContext, 1 );
    if( !ctx )  return 0;

    if( !origContext ) {
        origContext = ctx;
    }

    draw->ctx = ctx;

    /* Remember the original size of the window... */
    XGetWindowAttributes( disp, win, &Attrs );

    glXMakeCurrent( disp, win, ctx );

    setP1P2( draw, 0, 0, Attrs.width, Attrs.height );

    __hwGlInitDrawableVars( draw );

    return 1;
}

void __hwGlMakeCurrent( hwDisplay disp, hwDrawable draw )
{
    glDisplay *gldisp;
    glDrawable *glctx;

    __hwInternalMakeCurrent( disp );

    gldisp = (glDisplay *)disp;
    gldisp->draw = glctx = (glDrawable *)draw;

    glXMakeCurrent( gldisp->xdisp, glctx->win, glctx->ctx );
    __hwGlInitState( glctx );
}

void __hwGlViewport
(
    hwDisplay disp, hwInt32 x, hwInt32 y, hwInt32 width, hwInt32 height
)
{
    USE_GL_CTX(disp);
    setP1P2( glctx, x, y, width, height );
}

void __hwGlSwapBuffers( glDrawable *glctx )
{
    glXSwapBuffers( glctx->disp->xdisp,
                    glctx->win );

    /* See if X rendering gets better in full scene AA */
    glXWaitGL();
}

/*** EOF x11_visual.c ***/
