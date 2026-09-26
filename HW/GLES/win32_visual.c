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

static HGLRC
    origContext;

/* Some utility functions (see below) */
static void setP1P2( glDrawable *draw,
                hwInt32 x, hwInt32 y, hwInt32 w, hwInt32 h );
static hwInt32 InitGraphicsFildes( glDrawable *draw );

void
    (*__hwglEventHandler)( HWND hwnd, UINT msg, LPARAM wParam, LPARAM lParam );

static char
    *__hwWindowClass;

static LONG WINAPI __hwWindowProc
(
    HWND hwnd, UINT msg, LPARAM wParam, LPARAM lParam
)
{
    FETCH_GL_CTX;
    PAINTSTRUCT
        ps;
    hwWinEvent
        event;
    hwInt32
        clipRect[4];
    static int
        shiftState,
        buttonState;
    int
        handled = 1;
    void
        (*eventCallback)(hwDrawable, hwWinEvent *) = NULL;

    if( glctx ) {
        eventCallback = glctx->eventCB;
    }

    if( __hwglEventHandler ) {
        (*__hwglEventHandler)( hwnd, msg, wParam, lParam );
    }
    switch( msg ) {
    case WM_CREATE :
        handled = 0;
        break;
    case WM_CLOSE :
        PostQuitMessage( 0 );
        handled = 0;
        break;
    case WM_DESTROY :
        event.type = HW_INPUT_DESTROY;
        break;
    case WM_PAINT :
        BeginPaint( hwnd, &ps );
        event.type = HW_INPUT_EXPOSE;
        event.expose.numClipRects = 1;
        event.expose.clipRectList = clipRect;
        clipRect[0] = ps.rcPaint.left;
        clipRect[1] = ps.rcPaint.top;
        clipRect[2] = ps.rcPaint.right - ps.rcPaint.left + 1;
        clipRect[3] = ps.rcPaint.bottom - ps.rcPaint.top + 1;
        EndPaint( hwnd, &ps );
        break;
    case WM_KEYUP :
        handled = 0;
        switch (wParam) {
        case VK_SHIFT :
        case VK_LSHIFT :
        case VK_RSHIFT :
            shiftState &= ~HW_KBD_MOD_SHIFT;
            break;
        case VK_CONTROL :
        case VK_LCONTROL :
        case VK_RCONTROL :
            shiftState &= ~(HW_KBD_MOD_CTRL | HW_KBD_MOD_ALT);
            break;
        }
        break;
    case WM_KEYDOWN :
        event.type = HW_INPUT_KEYBOARD;
        event.keyboard.mods = shiftState;
        switch (wParam) {
        case VK_SHIFT :
        case VK_LSHIFT :
        case VK_RSHIFT :
            shiftState |= HW_KBD_MOD_SHIFT;
            handled = 0;
            break;
        case VK_CONTROL :
        case VK_LCONTROL :
        case VK_RCONTROL :
            shiftState |= (HW_KBD_MOD_CTRL | HW_KBD_MOD_ALT);
            handled = 0;
            break;
        case VK_PRIOR :
            event.keyboard.key = HW_KEY_PREV;
            break;
        case VK_NEXT :
            event.keyboard.key = HW_KEY_NEXT;
            break;
        case VK_HOME :
            event.keyboard.key = HW_KEY_HOME;
            break;
        case VK_END :
            event.keyboard.key = HW_KEY_END;
            break;
        case VK_LEFT :
            event.keyboard.key = HW_KEY_LEFT;
            break;
        case VK_RIGHT :
            event.keyboard.key = HW_KEY_RIGHT;
            break;
        case VK_UP :
            event.keyboard.key = HW_KEY_UP;
            break;
        case VK_DOWN :
            event.keyboard.key = HW_KEY_DOWN;
            break;
        default :
            if ((wParam >= VK_F1) && (wParam <= VK_F12)) {
                event.keyboard.key = HW_KEY_F1 + wParam - VK_F1;
            } else {
                handled = 0;
            }
            break;
        }
        break;
    case WM_CHAR :
        event.type = HW_INPUT_KEYBOARD;
        event.keyboard.key = wParam;
        event.keyboard.mods = 0;
        if (!lParam & 0x80000000) {
            handled = 0;
        }
        break;
    case WM_SIZE :
        event.type = HW_INPUT_CONFIG;
        event.config.width = LOWORD(lParam);
        event.config.height = HIWORD(lParam);
        event.config.posx = 0;
        event.config.posy = 0;
        break;
    case WM_LBUTTONDOWN : case WM_LBUTTONUP :
    case WM_MBUTTONDOWN : case WM_MBUTTONUP :
    case WM_RBUTTONDOWN : case WM_RBUTTONUP :
        switch (msg) {
        case WM_LBUTTONDOWN :
            event.type = HW_INPUT_BUTTON_PRESS;
            event.button.button = 0;
            buttonState |= 1 << 0;
            break;
        case WM_LBUTTONUP :
            event.type = HW_INPUT_BUTTON_RELEASE;
            event.button.button = 0;
            buttonState &= ~(1 << 0);
            break;
        case WM_MBUTTONDOWN :
            event.type = HW_INPUT_BUTTON_PRESS;
            event.button.button = 1;
            buttonState |= 1 << 1;
            break;
        case WM_MBUTTONUP :
            event.type = HW_INPUT_BUTTON_RELEASE;
            event.button.button = 1;
            buttonState &= ~(1 << 1);
            break;
        case WM_RBUTTONDOWN :
            event.type = HW_INPUT_BUTTON_PRESS;
            event.button.button = 2;
            buttonState |= 1 << 2;
            break;
        case WM_RBUTTONUP :
            event.type = HW_INPUT_BUTTON_RELEASE;
            event.button.button = 2;
            buttonState &= ~(1 << 2);
            break;
        }
        event.button.x = LOWORD(lParam);
        event.button.y = HIWORD(lParam);
        event.button.kbdMods = 0;
        if (wParam & MK_CONTROL) {
            event.button.kbdMods |= HW_KBD_MOD_CTRL;
        }
        if (wParam & MK_SHIFT) {
            event.button.kbdMods |= HW_KBD_MOD_SHIFT;
        }
        if (GetKeyState(VK_MENU) < 0) {
            event.button.kbdMods |= HW_KBD_MOD_ALT;
        }
        break;
    case WM_MOUSEMOVE :
        event.type = HW_INPUT_POINTER;
        event.pointer.x = LOWORD(lParam);
        event.pointer.y = HIWORD(lParam);
        event.pointer.buttonState = buttonState;
        event.pointer.kbdMods = 0;
        if (wParam & MK_CONTROL) {
            event.button.kbdMods |= HW_KBD_MOD_CTRL;
        }
        if (wParam & MK_SHIFT) {
            event.button.kbdMods |= HW_KBD_MOD_SHIFT;
        }
        if (GetKeyState(VK_MENU) < 0) {
            event.button.kbdMods |= HW_KBD_MOD_ALT;
        }
        break;
    default :
        handled = 0;
        break;
    }

    if (handled) {
        if (eventCallback) {
            (*eventCallback)((hwDrawable)glctx, &event);
        }
        return 0;
    }
    else {
        return DefWindowProc( hwnd, msg, wParam, lParam );
    }
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

    result = malloc( sizeof(glDisplay) );
    if( !result )       return 0;

    memset(result, 0, sizeof(glDisplay));

    result->disp.vtab = __hwGlInitFunc;
    __hwInternalInitDisplay( (hwDisplay)result, shared );

    __hwGlGetEnvVars( result );

    if( !__hwWindowClass ) {
        HANDLE
            hInstance;
        WNDCLASS
            wc;

        __hwWindowClass = "HoverWare";

        /* Get current instance */
        hInstance = GetModuleHandle( NULL );
        
        /* fill in the window class structure */
        wc.style         = 0;                   /* no special styles */
        wc.lpfnWndProc   = (WNDPROC)__hwWindowProc;     /* event handler */
        wc.cbClsExtra    = 0;                   /* no extra class data */
        wc.cbWndExtra    = 0;                   /* no extra window data */
        wc.hInstance     = hInstance;           /* instance */
        wc.hIcon         = LoadIcon(hInstance, "HW_ICON");      /* load icon */
        wc.hCursor       = LoadCursor(hInstance, IDC_ARROW);
        wc.hbrBackground = NULL;                /* redraw our own bg */
        wc.lpszMenuName  = NULL;                /* no menu */
        wc.lpszClassName = __hwWindowClass;     /* use a special class */
        if(!wc.hIcon) {
            wc.hIcon       = LoadIcon(NULL, IDI_WINLOGO);  /* default icon */
        }
  
        /* register the window class */
        if(!RegisterClass(&wc)) {
            free( result );
            return 0;
        }
    }
    result->w = GetSystemMetrics( SM_CXSCREEN );
    result->h = GetSystemMetrics( SM_CYSCREEN );

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
    glDisplay
        *disp = (glDisplay *)display;
    PIXELFORMATDESCRIPTOR
        pfd,
        bestPfd;
    HWND
        hwnd;
    HDC
        hdc;
    int
        i, done, iPixelFormat,
        score, bestScore;
    hwInt32
        actual;

    hwnd = GetDesktopWindow();
    hdc = GetDC( hwnd );
    if( !hdc ) {
        printf( "GetDC failed in chooseVisual!\n" );
        return 0;
    }

    iPixelFormat = bestScore = done = 0;
    for( i = 1; !done; i++ ) {
        score = 0;

        if( !DescribePixelFormat( hdc, i, sizeof(pfd), &pfd ) ) {
            done = 1;
            continue;
        }

        if( !iPixelFormat ) { iPixelFormat = i; bestPfd = pfd; }

        /* Musts for all pixel formats */
        if( pfd.dwFlags & PFD_GENERIC_FORMAT ) {
            if( !(pfd.dwFlags & PFD_GENERIC_ACCELERATED) ) {
                continue;
            }
        }
        if( pfd.iPixelType != PFD_TYPE_RGBA ) {
            continue;
        }
        if( !(pfd.dwFlags & (PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL)) ) {
            continue;
        }

        if( !(pfd.dwFlags & PFD_DOUBLEBUFFER) ) {
            if( required & HW_VIS_DBUFF ) {
                continue;
            }
        }
        else {
            if( (desired | required) & HW_VIS_DBUFF ) {
                score++;
            }
        }

        if( !pfd.cDepthBits ) {
            if( required & HW_VIS_DEPTH ) {
                continue;
            }
        }
        else if( pfd.cDepthBits >= bestPfd.cDepthBits ) {
            if( (desired | required) & HW_VIS_DEPTH ) {
                score += pfd.cDepthBits;
            }
        }

        if( !pfd.cAlphaBits ) {
            if( required & HW_VIS_ALPHA ) {
                continue;
            }
        }
        else if( pfd.cAlphaBits >= bestPfd.cAlphaBits ) {
            if( (desired | required) & HW_VIS_ALPHA ) {
                score += pfd.cAlphaBits;
            }
        }

        if( !pfd.cStencilBits ) {
            if( required & HW_VIS_STENCIL ) {
                continue;
            }
        }
        else if( pfd.cStencilBits >= bestPfd.cStencilBits ) {
            if( (desired | required) & HW_VIS_STENCIL ) {
                score += pfd.cAlphaBits;
            }
        }

        if( !(pfd.dwFlags & PFD_STEREO) ) {
            if( required & HW_VIS_STEREO ) {
                continue;
            }
        }
        else {
            if( (desired | required) & HW_VIS_DBUFF ) {
                score += pfd.cColorBits;
            }
        }

        if( pfd.cColorBits >= bestPfd.cColorBits ) {
            score += pfd.cColorBits;
        }

        if( score > bestScore ) {
            bestScore = score;
            iPixelFormat = i;
            bestPfd = pfd;
        }
    }

    ReleaseDC( hwnd, hdc );

    actual = HW_VIS_TEXTURE;
    if( bestPfd.dwFlags & PFD_DOUBLEBUFFER )    actual |= HW_VIS_DBUFF;
    if( bestPfd.dwFlags & PFD_STEREO )          actual |= HW_VIS_STEREO;
    if( bestPfd.cDepthBits > 0 )                actual |= HW_VIS_DEPTH;
    if( bestPfd.cAlphaBits > 0 )                actual |= HW_VIS_ALPHA;
    if( bestPfd.cStencilBits > 0 )              actual |= HW_VIS_STENCIL;
    if( (actual & required) != required ) {
        printf( "Requirement mismatch; actual %08x, required %08x!\n",
                        actual, required );
        return 0;
    }

    disp->iPixelFormat = iPixelFormat;
    disp->dbuffer = (actual & HW_VIS_DBUFF) ? 1 : 0;
    return actual | HW_VIS_SUCCESS;
}

OS_VISUAL_TYPE __hwGlExtractVisual( hwDisplay display )
{
    glDisplay
        *disp = (glDisplay *)display;;

    return (OS_VISUAL_TYPE)disp->iPixelFormat;
}

OS_DRAWABLE_TYPE __hwGlExtractWin( hwDisplay disp, hwDrawable draw )
{
    glDrawable
        *win = (glDrawable *)draw;

    return win->hDC;
}

/* Create a stoopid window of the given characteristics */
hwDrawable hwGlCreateWinUtility
(
    hwDisplay display,  /* Display to use */
    const char *winName,      /* Name of the window to create */
    HWND parent,        /* Name of the parent window, NULL if not a child */
    hwInt32 winX, hwInt32 winY, /* Placement */
    hwInt32 winW, hwInt32 winH, /* Size */
    hwInt32 flags
)
{
    int
        mode;
    glDisplay
        *disp = (glDisplay *)display;
    glDrawable
        *result;
    PIXELFORMATDESCRIPTOR
        pfd;
    DWORD
        style;
    RECT
        rect;

    result = malloc( sizeof( glDrawable ) );
    if( !result )       return 0;

    memset(result, 0, sizeof(glDrawable) );

    result->disp = disp;

    if (flags & HW_WIN_FULLSCREEN) {
        style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP | WS_MAXIMIZE;
        winX = 0;
        winY = 0;
        winW = disp->w;
        winH = disp->h;
    }
    else {
        if( parent != NULL ) {
            style = WS_CHILD | WS_VISIBLE;
        }
        else {
            style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW;
        }
    }
    rect.left = winX;
    rect.top = winY;
    rect.right = winX + winW;
    rect.bottom = winY + winH;
    if( AdjustWindowRectEx( &rect, style, 0, 0  ) ) {
        winX = rect.left;
        winY = rect.top;
        winW = rect.right - rect.left;
        winH = rect.bottom - rect.top;
    }

    result->win = CreateWindow( __hwWindowClass,        /* class */
                        winName,                        /* name */
                        style,                          /* style */
                        winX, winY, winW, winH,         /* size */
                        parent,                         /* parent */
                        NULL,                           /* menu */
                        GetModuleHandle( NULL ),        /* instance */
                        (LPVOID)disp );                 /* window info */
    if( !result->win ) {
        free( result );
        return 0;
    }
    result->hDC = GetDC( result->win );

    if( !InitGraphicsFildes( result ) ) {
        free( result );
        return 0;
    }

    /*SendMessage( result->win, WM_INIT, 0, 0L );*/
    ShowWindow( result->win, TRUE );
    if (flags & HW_WIN_FULLSCREEN) {
        BringWindowToTop(result->win);
        SetForegroundWindow(result->win);
    }
    if (flags & HW_WIN_HIDE_CURSOR) {
        ShowCursor(FALSE);
    }
    UpdateWindow( result->win );
    __hwGlCheckInput( display );

    return (hwDrawable)result;
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
    OS_WINDOW_TYPE parent,        /* handle of the the parent window */
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
    POINT
        point;

    GetCursorPos( &point );
    ScreenToClient( glctx->win, &point );
    *x = point.x;
    *y = point.y;
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
    Result->win = draw;
    Result->hDC = GetDC( draw );

    if( !InitGraphicsFildes( Result ) ) {
        free( Result );
        return 0;
    }

    return (hwDrawable)Result;
}

static void setP1P2( glDrawable *draw,
                hwInt32 x, hwInt32 y, hwInt32 w, hwInt32 h )
{
    RECT
        rect;

    if( !draw )
        return;

    GetClientRect( draw->win, &rect );
    draw->VDC_XMax = draw->winW = w;
    draw->VDC_YMax = draw->winH = h;
    glViewport( x, rect.bottom-(y+h), w, h );
}

static hwInt32 InitGraphicsFildes( glDrawable *draw )
{
    PIXELFORMATDESCRIPTOR
        pfd;
    RECT
        rect;

    DescribePixelFormat( draw->hDC, draw->disp->iPixelFormat,
                        sizeof(pfd), &pfd );
    SetPixelFormat( draw->hDC, draw->disp->iPixelFormat, &pfd );

    draw->ctx = wglCreateContext( draw->hDC );
    if( !draw->ctx ) {
        return 0;
    }
    if( origContext ) {
        wglShareLists( origContext, draw->ctx );
    }
    else {
        origContext = draw->ctx;
    }
    wglMakeCurrent( draw->hDC, draw->ctx );

    rect.right = 100; rect.bottom = 100;
    GetClientRect( draw->win, &rect );
    setP1P2( draw, 0, 0, rect.right, rect.bottom );

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

    wglMakeCurrent( glctx->hDC, glctx->ctx );
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
    MSG
        msg;

    while( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) ) {
        if( !GetMessage( &msg, NULL, 0, 0 ) ) {
            exit( 0 );
        }
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }
}


void __hwGlSwapBuffers( glDrawable *glctx )
{
    SwapBuffers( glctx->hDC );
}


/*** EOF win32_visual.c ***/
