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

//#  define ANDROID_LOG(a) __android_log_print a
#  define ANDROID_LOG(a)

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

    /* HACK!  Need to get the EGL dims somehow */
    result->w = 320;
    result->h = 480;

    /* Set up the current time-of-day clock */
    result->glob.startTime = result->glob.currTime = hwGetSysTime();

    ANDROID_LOG((ANDROID_LOG_INFO, "HB_HW",
                "__hwGlInit: Return result: %x",result));

    return (hwDisplay)result;
}

/* hwChooseVisual - choose an X visual which matches the request */
hwInt32 __hwGlChooseVisual
(
    hwDisplay display, hwInt32 required, hwInt32 desired
)
{
    // Assume Android always works :-)
    return required;
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

    return (void *)win->win;
}

/* Create a stoopid window of the given characteristics */
hwDrawable hwGlCreateWinUtility
(
    hwDisplay display,  /* Display to use */
    const char *winName,      /* Name of the window to create */
    void  *parent,        /* Name of the parent window, NULL if not a child */
    hwInt32 winX, hwInt32 winY, /* Placement */
    hwInt32 winW, hwInt32 winH, /* Size */
    hwInt32 flags
)
{
    glDrawable
        *Result;
    glDisplay
        *disp = (glDisplay *)display;

    ANDROID_LOG((ANDROID_LOG_INFO, "HB_HW", "CreateWin"));

    Result = malloc( sizeof(glDrawable) );
    if( !Result ) {
        __android_log_print(ANDROID_LOG_INFO, "HB_HW",
                            "CreateWin failed malloc");
        return 0;
    }

    memset(Result, 0, sizeof(glDrawable) );

    Result->disp = disp;
    Result->buffer = 0;

    if( !InitGraphicsFildes( Result ) ) {
        __android_log_print(ANDROID_LOG_INFO, "HB_HW",
                            "CreateWin InitGraphicsFildes failed");
        free( Result );
        return 0;
    }

    ANDROID_LOG((ANDROID_LOG_INFO, "HB_HW", "CreateWin success"));
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
    return hwGlCreateWinUtility(display,winName, NULL,
                winX, winY, winW, winH, flags);
}

hwDrawable __hwGlCreateChildWin
(
    hwDisplay display,  /* Display to use */
    const char *winName,      /* Name of the window to create */
    void  *parent,        /* handle of the the parent window */
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

void __hwGlGetMousePos( hwDisplay display, hwInt32 *x, hwInt32 *y )
{
    *x = *y = 0;
}

hwDrawable __hwGlInitDrawable( hwDisplay display, OS_DRAWABLE_TYPE draw )
{
    glDisplay
        *disp = (glDisplay *)display;
    glDrawable
        *Result;

    Result = malloc( sizeof(glDrawable) );
    if( !Result ) {
        return 0;
    }
    memset(Result, 0, sizeof(glDrawable) );

    Result->disp = disp;
    Result->buffer = 0;

    if( !InitGraphicsFildes( Result ) ) {
        free( Result );
        return 0;
    }

    return (hwDrawable)Result;
}

static void setP1P2( glDrawable *draw,
                hwInt32 x, hwInt32 y, hwInt32 w, hwInt32 h )
{
    draw->VDC_XMax = draw->winW = w;
    draw->VDC_YMax = draw->winH = h;
    glViewport( x, y, w, h );
    ANDROID_LOG((ANDROID_LOG_INFO, "HB_HW", "setP1P2:%d %d %d %d ",x,y,w,h));
}

static hwInt32 InitGraphicsFildes( glDrawable *draw )
{
    hwInt32
        i;

    draw->ctx = 0;

    setP1P2( draw, 0, 0, 320, 480 );

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

    // What else to do here?

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

void __hwGlCheckInput( hwDisplay display )
{
    // TBD - nothing to do here?
}

void __hwGlSwapBuffers( glDrawable *glctx )
{
    // What to do here?
}

/*** EOF droid_visual.c ***/
