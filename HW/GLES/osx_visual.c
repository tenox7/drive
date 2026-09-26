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

static AGLContext
    origContext;

/* Some utility functions (see below) */
static void setP1P2( glDrawable *draw,
                hwInt32 x, hwInt32 y, hwInt32 w, hwInt32 h );
static hwInt32 InitGraphicsFildes( glDrawable *draw );

static pascal OSStatus __hwMac_eventHandler(EventHandlerCallRef nextHandler,
                             EventRef theEvent,
                             void *userData)
{
    FETCH_GL_CTX;
    UInt32 eventClass;
    UInt32 eventKind;
    static UniChar *text;
    UniChar *ptr;
    static UInt32 textAlloc;
    UInt32 actualSize;
    HICommand hiCommand;
    MenuID menuID;
    char charCode;
    HIPoint where;
    int ix, iy;
    EventMouseButton button;
    UInt32 modif;
    WindowRef window;
    void (*eventCallback)(hwDrawable, hwWinEvent *) = NULL;

    window = (WindowRef) userData;
    eventClass = GetEventClass(theEvent);
    eventKind = GetEventKind(theEvent);
    hwWinEvent event;
    int handled = 1;

    if( glctx ) {
        eventCallback = glctx->eventCB;
    }

    switch (eventClass) {
    case kEventClassWindow :
        switch (eventKind) {
        case kEventWindowBoundsChanged :
            GetWindowBounds( window, kWindowContentRgn, &glctx->contentRegion );
            GetWindowBounds( window, kWindowGrowRgn, &glctx->growRegion );
            aglUpdateContext( glctx->ctx );
            event.type = HW_INPUT_CONFIG;
            event.config.posx = glctx->contentRegion.left;
            event.config.posy = glctx->contentRegion.top;
            event.config.width = glctx->contentRegion.right
                                - glctx->contentRegion.left;
            event.config.height = glctx->contentRegion.bottom
                                - glctx->contentRegion.top;
            handled = 0;
            break;
        case kEventWindowClose :
            QuitApplicationEventLoop();
            exit(0);    // TBD
            break;
        default :
            return CallNextEventHandler(nextHandler, theEvent);
        }
        break;

    case kEventClassKeyboard :
        switch (eventKind) {
        case kEventRawKeyDown :
        case kEventRawKeyRepeat :
            GetEventParameter(theEvent, kEventParamKeyMacCharCodes,
                        typeChar, NULL, sizeof(char), NULL, &charCode);
            event.type = HW_INPUT_KEYBOARD;
            switch (charCode) {
            case 0x01 : event.keyboard.key = HW_KEY_HOME;       break;
            case 0x04 : event.keyboard.key = HW_KEY_END;        break;
            case 0x0B : event.keyboard.key = HW_KEY_PREV;       break;
            case 0x0C : event.keyboard.key = HW_KEY_NEXT;       break;
            case 0x1E : event.keyboard.key = HW_KEY_UP;         break;
            case 0x1F : event.keyboard.key = HW_KEY_DOWN;       break;
            case 0x1C : event.keyboard.key = HW_KEY_LEFT;       break;
            case 0x1D : event.keyboard.key = HW_KEY_RIGHT;      break;
            default :   event.keyboard.key = charCode;          break;
            }
            event.keyboard.mods = 0; // TBD
            handled = 0;
            break;
        default :
            return CallNextEventHandler(nextHandler, theEvent);
        }
        break;

    case kEventClassMouse :
        switch (eventKind) {
        case kEventMouseDown :
            GetEventParameter(theEvent, kEventParamMouseLocation,
                        typeHIPoint, NULL, sizeof(HIPoint), NULL, &where);
            ix = (int) where.x; iy = (int) where.y;
            if ((ix < glctx->contentRegion.left) ||
                (iy < glctx->contentRegion.top) ||
                (ix > glctx->contentRegion.right) ||
                (iy > glctx->contentRegion.bottom))
            {
                return CallNextEventHandler(nextHandler, theEvent);
            }
            if ((ix >= glctx->growRegion.left) &&
                (iy >= glctx->growRegion.top) &&
                (ix <= glctx->growRegion.right) &&
                (iy <= glctx->growRegion.bottom))
            {
                return CallNextEventHandler(nextHandler, theEvent);
            }
            ix -= glctx->contentRegion.left; iy -= glctx->contentRegion.top;

            GetEventParameter(theEvent, kEventParamMouseButton,
                        typeMouseButton, NULL, sizeof(EventMouseButton),
                        NULL, &button);
            button -= kEventMouseButtonPrimary;

            GetEventParameter(theEvent, kEventParamKeyModifiers,
                        typeUInt32, NULL, sizeof(UInt32),
                        NULL, &modif);
            /* cmdKey shiftKey alphaLock optionKey controlKey */
            event.type = HW_INPUT_BUTTON_PRESS;
            event.button.x = ix;
            event.button.y = iy;
            switch (button) {
            case 0 :    button = 0;     break;
            case 1 :    button = 2;     break;
            case 2 :    button = 1;     break;
            }
            event.button.button = button;
            event.button.kbdMods = 0;
            if( modif & cmdKey )     event.button.kbdMods |= HW_KBD_MOD_ALT;
            if( modif & shiftKey )   event.button.kbdMods |= HW_KBD_MOD_SHIFT;
            if( modif & optionKey )  event.button.kbdMods |= HW_KBD_MOD_ALT;
            if( modif & controlKey ) event.button.kbdMods |= HW_KBD_MOD_CTRL;
            handled = 0;
            break;

        case kEventMouseUp :
            GetEventParameter(theEvent, kEventParamMouseLocation,
                        typeHIPoint, NULL, sizeof(HIPoint), NULL, &where);
            ix = (int) where.x; iy = (int) where.y;
            ix -= glctx->contentRegion.left; iy -= glctx->contentRegion.top;

            GetEventParameter(theEvent, kEventParamMouseButton,
                        typeMouseButton, NULL, sizeof(EventMouseButton),
                        NULL, &button);
            button -= kEventMouseButtonPrimary;

            GetEventParameter(theEvent, kEventParamKeyModifiers,
                        typeUInt32, NULL, sizeof(UInt32),
                        NULL, &modif);
            event.type = HW_INPUT_BUTTON_RELEASE;
            event.button.x = ix;
            event.button.y = iy;
            event.button.button = button;
            event.button.kbdMods = 0;
            if( modif & cmdKey )     event.button.kbdMods |= HW_KBD_MOD_ALT;
            if( modif & shiftKey )   event.button.kbdMods |= HW_KBD_MOD_SHIFT;
            if( modif & optionKey )  event.button.kbdMods |= HW_KBD_MOD_ALT;
            if( modif & controlKey ) event.button.kbdMods |= HW_KBD_MOD_CTRL;
            handled = 0;
            break;

        case kEventMouseMoved :
            GetEventParameter(theEvent, kEventParamMouseLocation,
                        typeHIPoint, NULL, sizeof(HIPoint), NULL, &where);

            ix = (int) where.x; iy = (int) where.y;
            ix -= glctx->contentRegion.left; iy -= glctx->contentRegion.top;

            GetEventParameter(theEvent, kEventParamKeyModifiers,
                        typeUInt32, NULL, sizeof(UInt32),
                        NULL, &modif);

            event.type = HW_INPUT_POINTER;
            event.pointer.x = ix;
            event.pointer.y = iy;
            event.pointer.buttonState = 0; // TBD
            event.button.kbdMods = 0;
            if( modif & cmdKey )     event.button.kbdMods |= HW_KBD_MOD_ALT;
            if( modif & shiftKey )   event.button.kbdMods |= HW_KBD_MOD_SHIFT;
            if( modif & optionKey )  event.button.kbdMods |= HW_KBD_MOD_ALT;
            if( modif & controlKey ) event.button.kbdMods |= HW_KBD_MOD_CTRL;
            handled = 0;
            break;

        case kEventMouseDragged :
            GetEventParameter(theEvent, kEventParamMouseLocation,
                        typeHIPoint, NULL, sizeof(HIPoint), NULL, &where);

            GetEventParameter(theEvent, kEventParamKeyModifiers,
                        typeUInt32, NULL, sizeof(UInt32),
                        NULL, &modif);

            ix = (int) where.x; iy = (int) where.y;
            ix -= glctx->contentRegion.left; iy -= glctx->contentRegion.top;

            event.type = HW_INPUT_POINTER;
            event.pointer.x = ix;
            event.pointer.y = iy;
            event.pointer.buttonState = 1; // TBD
            event.button.kbdMods = 0;
            if( modif & cmdKey )     event.button.kbdMods |= HW_KBD_MOD_ALT;
            if( modif & shiftKey )   event.button.kbdMods |= HW_KBD_MOD_SHIFT;
            if( modif & optionKey )  event.button.kbdMods |= HW_KBD_MOD_ALT;
            if( modif & controlKey ) event.button.kbdMods |= HW_KBD_MOD_CTRL;
            handled = 0;
            break;

        default :
            return CallNextEventHandler(nextHandler, theEvent);
        }
        break;

    case kEventClassCommand :
        switch (eventKind) {
        case kEventProcessCommand :
            GetEventParameter(theEvent, kEventParamDirectObject, typeHICommand,
                                NULL, sizeof(HICommand), NULL, &hiCommand);
            if (hiCommand.commandID == kHICommandQuit) {
                QuitApplicationEventLoop();
                exit(0);
            }
            return CallNextEventHandler(nextHandler, theEvent);
        default :
            return CallNextEventHandler(nextHandler, theEvent);
        }
        break;
    default :
        return CallNextEventHandler(nextHandler, theEvent);
    }

    if( !handled ) {
        if (eventCallback) {
            (*eventCallback)((hwDrawable)glctx, &event);
        }
    }

    return noErr;
}

static void __hwMac_doIdle(void)
{
    QuitApplicationEventLoop();
}


static void __hwMac_osxAllowForeground(void)
{
    ProcessSerialNumber psn;
    GetCurrentProcess(&psn);
    TransformProcessType(&psn, kProcessTransformToForegroundApplication);
    SetFrontProcess(&psn);
}

/* hwInit - initialize the X display connection */
hwDisplay __hwGlCreate
(
    hwDisplay disp, hwDisplay shared,
    OS_DISPLAY_TYPE display
)
{
    glDisplay
        *result;
    CGRect
        rect;

    result = malloc( sizeof(glDisplay) );
    if( !result )       return 0;

    memset(result, 0, sizeof(glDisplay));

    result->disp.vtab = __hwGlInitFunc;
    __hwInternalInitDisplay( (hwDisplay)result, shared );

    __hwGlGetEnvVars( result );

    __hwMac_osxAllowForeground();

    rect = CGDisplayBounds(CGMainDisplayID());
    result->w = rect.size.width;
    result->h = rect.size.height;

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
    glDisplay *disp = (glDisplay *)display;
    GLint attribs[128], n;
    AGLPixelFormat pf;

    /* Combine required flags into desired flags */
    desired |= required;

    n = 0;
    attribs[n++] = AGL_ACCELERATED;
    attribs[n++] = AGL_RGBA;
    attribs[n++] = AGL_RED_SIZE;   attribs[n++] = 3;
    attribs[n++] = AGL_GREEN_SIZE; attribs[n++] = 3;
    attribs[n++] = AGL_BLUE_SIZE;  attribs[n++] = 2;

    if( desired & HW_VIS_DBUFF ) {
        attribs[n++] = AGL_DOUBLEBUFFER;
    }
    if( desired & HW_VIS_DEPTH ) {
        attribs[n++] = AGL_DEPTH_SIZE;   attribs[n++] = 20;
    }
    if( desired & HW_VIS_STENCIL ) {
        attribs[n++] = AGL_STENCIL_SIZE;   attribs[n++] = 1;
    }
    if( desired & HW_VIS_ALPHA ) {
        attribs[n++] = AGL_ALPHA_SIZE;   attribs[n++] = 1;
    }
    if( desired & (HW_VIS_AA|HW_VIS_AA_MED|HW_VIS_AA_HI) ) {
        attribs[n++] = AGL_MULTISAMPLE;
        attribs[n++] = AGL_SAMPLE_BUFFERS_ARB;
        attribs[n++] = 1;
        attribs[n++] = AGL_SAMPLES_ARB;
        if( desired & HW_VIS_AA_HI ) {
            attribs[n++] = 8;
        }
        else if( desired & HW_VIS_AA_MED ) {
            attribs[n++] = 4;
        }
        else {
            attribs[n++] = 2;
        }
    }
    if( desired & HW_VIS_STEREO ) {
        attribs[n++] = AGL_STEREO;
    }
    attribs[n++] = AGL_NONE;

    disp->pixelFormat = pf = aglChoosePixelFormat( NULL, 0, attribs );
    if( !pf ) return 0;

    /* Finally, see which features are actually present */
    if( aglDescribePixelFormat( pf,  AGL_BUFFER_SIZE, &n ) ) {
        if( n < 8 )     desired &= ~HW_VIS_OVERLAY;
    }
    if( aglDescribePixelFormat( pf,  AGL_LEVEL, &n ) ) {
        if( n < 1 )     desired &= ~HW_VIS_OVERLAY;
    }
    if( aglDescribePixelFormat( pf,  AGL_RGBA, &n ) ) {
        if( n )         desired &= ~HW_VIS_OVERLAY;
        else            /* TBD */;
    }
    if( aglDescribePixelFormat( pf,  AGL_DOUBLEBUFFER, &n ) ) {
        if( !n )        desired &= ~HW_VIS_DBUFF;
    }
    if( aglDescribePixelFormat( pf,  AGL_MULTISAMPLE, &n ) ) {
        if( n ) {
            if( aglDescribePixelFormat( pf,  AGL_SAMPLES_ARB, &n ) ) {
                if( n < 4 ) {
                    desired &= ~(HW_VIS_AA_MED | HW_VIS_AA_HI);
                }
                else if( n < 8 ) {
                    desired &= ~HW_VIS_AA_HI;
                }
            }
        }
        else {
            desired &= ~(HW_VIS_AA | HW_VIS_AA_MED | HW_VIS_AA_HI);
        }
    }
    if( aglDescribePixelFormat( pf,  AGL_DEPTH_SIZE, &n ) ) {
        if( n < 16 )    desired &= ~HW_VIS_DEPTH;
    }
    if( aglDescribePixelFormat( pf,  AGL_ALPHA_SIZE, &n ) ) {
        if( n < 4 )     desired &= ~HW_VIS_ALPHA;
    }
    if( aglDescribePixelFormat( pf,  AGL_STENCIL_SIZE, &n ) ) {
        if( n < 1 )     desired &= ~HW_VIS_STENCIL;
    }

    /* See if required criteria are met */
    if( (required & desired) != required ) {
        /* Oops - can't do it */
        return 0;
    }

    disp->dbuffer = (desired & HW_VIS_DBUFF) ? 1 : 0;
    return desired | HW_VIS_SUCCESS;
}

OS_VISUAL_TYPE __hwGlExtractVisual( hwDisplay display )
{
    glDisplay
        *disp = (glDisplay *)display;;

    return (OS_VISUAL_TYPE)disp->pixelFormat;
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
    WindowRef parent,   /* Name of the parent window, NULL if not a child */
    hwInt32 winX, hwInt32 winY, /* Placement */
    hwInt32 winW, hwInt32 winH, /* Size */
    hwInt32 flags
)
{
    Rect
        windowRect;
    WindowRef
        win;
    EventHandlerUPP
        handlerUPP;
    EventTypeSpec
        typeList[] = {
            { kEventClassKeyboard, kEventRawKeyDown },
            { kEventClassKeyboard, kEventRawKeyRepeat },
            { kEventClassWindow, kEventWindowClose },
            { kEventClassWindow, kEventWindowBoundsChanged },
            { kEventClassMouse, kEventMouseDown },
            { kEventClassMouse, kEventMouseUp },
            { kEventClassMouse, kEventMouseMoved },
            { kEventClassMouse, kEventMouseDragged },
            { kEventClassCommand, kEventProcessCommand },
        };

    int
        typeSize;
    glDrawable
        *Result;
    glDisplay
        *disp = (glDisplay *)display;

    Result = malloc( sizeof(glDrawable) );
    if( !Result ) {
        return 0;
    }

    memset(Result, 0, sizeof(glDrawable) );

    Result->disp = disp;
    Result->buffer = 0;

    if (flags & HW_WIN_FULLSCREEN) {
        windowRect.left = 0;
        windowRect.right = disp->w;
        windowRect.top = 0;
        windowRect.bottom = disp->h;
        CreateNewWindow(kOverlayWindowClass,
                        kWindowStandardHandlerAttribute |
                        kWindowOpaqueForEventsAttribute |
                        0,
                    &windowRect,
                    &win);
        Result->VDC_XMax = Result->winW = disp->w;
        Result->VDC_YMax = Result->winH = disp->h;
    }
    else {
        windowRect.left = winX;
        windowRect.right = winX + winW;
        windowRect.top = winY;
        windowRect.bottom = winY + winH;

        CreateNewWindow(kDocumentWindowClass,
                        kWindowStandardHandlerAttribute |
                        kWindowStandardDocumentAttributes |
                        kWindowLiveResizeAttribute |
                        0,
                    &windowRect,
                    &win);
        SetWindowTitleWithCFString( win,
                    CFStringCreateWithCString(NULL, winName,
                                              kCFStringEncodingMacRoman) );
    }

    handlerUPP = NewEventHandlerUPP(__hwMac_eventHandler);

    InstallEventLoopTimer( GetCurrentEventLoop(), kEventDurationNoWait,
                   0.01*kEventDurationSecond,
                   NewEventLoopTimerUPP((EventLoopTimerProcPtr)__hwMac_doIdle),
                   NULL, NULL );

    typeSize = (sizeof(typeList) / sizeof(typeList[0]));

    if (flags & HW_WIN_FULLSCREEN) {
        InstallApplicationEventHandler( handlerUPP,
                               typeSize, typeList,
                               NULL, NULL );
    }
    else {
        InstallWindowEventHandler( win, handlerUPP,
                               typeSize, typeList,
                               win, NULL );
    }

    Result->win = win;

    ShowWindow(win);
    GetWindowBounds( win, kWindowContentRgn, &Result->contentRegion );
    GetWindowBounds( win, kWindowGrowRgn, &Result->growRegion );

    if( !InitGraphicsFildes( Result ) ) {
        free( Result );
        return 0;
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
    return hwGlCreateWinUtility(display,winName, NULL,
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

void __hwGlGetMousePos( hwDisplay display, hwInt32 *x, hwInt32 *y )
{
    USE_GL_CTX(display);
    HIPoint point; 
    Rect contentRegion;
    HICoordinateSpace space = 2; 
    HIGetMousePosition(space, NULL, &point); 

    *x = ((hwInt32)point.x) - (hwInt32)glctx->contentRegion.left;
    *y = ((hwInt32)point.y) - (hwInt32)glctx->contentRegion.top;
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
}

static hwInt32 InitGraphicsFildes( glDrawable *draw )
{
    Rect
        contentRegion;
    glDisplay
        *disp = draw->disp;
    AGLContext
        ctx;
    WindowRef
        win = draw->win;

    ctx = aglCreateContext(disp->pixelFormat, NULL);

    if( !origContext ) {
        origContext = ctx;
    }

    draw->ctx = ctx;

    aglSetWindowRef( ctx, win );
    aglSetCurrentContext( ctx );
    aglUpdateContext( ctx );

    GetWindowBounds( draw->win, kWindowContentRgn, &contentRegion );
    setP1P2( draw, 0, 0,
             contentRegion.right - contentRegion.left,
             contentRegion.bottom - contentRegion.top );

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

    aglSetWindowRef( glctx->ctx, glctx->win );
    aglSetCurrentContext( glctx->ctx );
    aglUpdateContext( glctx->ctx );

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

void __hwGlCheckInput( hwDisplay disp )
{
    RunApplicationEventLoop();
}

void __hwGlSwapBuffers( glDrawable *glctx )
{
    aglSwapBuffers( glctx->ctx );
}

/*** EOF osx_visual.c ***/
