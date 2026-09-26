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

#define INITGUID

#include "hw.h"
#include "hw_internal.h"
#include "hw_dx.h"

#include        <sys/timeb.h>

double
    __hwIntStartTime,
    __hwIntCurrTime,
    __hwIntElapsedTime;

#define SHORTCUT_TEXTURE_SWITCH
#undef  SHORTCUT_TEXTURE_SWITCH

static const struct __hwDisplayStruct
    __hwDxInitFunc = {
        "DX9",
        __hwDxCreate,
        __hwDxChooseVisual,
        __hwDxExtractVisual,
        __hwDxExtractWin,
        __hwDxCreateWin,
        __hwDxCreateChildWin,
        __hwDxInitDrawable,
        __hwDxCheckInput,
        __hwDxGetMousePos,

        __hwDxMakeCurrent,
        __hwDxGetInfo,
        __hwDxViewport,
        __hwDxCamera,
        __hwDxPushMat,
        __hwDxPopMat,
        __hwDxXformPoint,
        __hwDxUpdate,
        __hwDxSetDrawBuffer,
        __hwDxAccumOp,

        __hwDxRenderMode,
        __hwDxSelectionInfo,
        __hwDxWasSelected,
        __hwDxCurrObject,
        __hwDxCurrChild,

        __hwDxPosLight,
        __hwDxDirLight,
        __hwDxFog,
        __hwDxFogParams,
        __hwDxAmbient,
        __hwDxLighting,
        __hwDxBackground,

        __hwDxCreateTexture,
        __hwDxDestroyTexture,
        __hwDxCurrentTexture,

        __hwDxSurfAttrs,
        __hwDxSetVisibility,
        __hwDxSetInvisibility,
        __hwDxGetVisibility,
        __hwDxDrawBBox,
        __hwDxBoundsVisible,
        __hwDxBoundsSize,

        __hwDxDrawMesh,
        __hwDxDrawStrip,
        __hwDxDrawPolygon,
        __hwDxDrawPolyline,
        __hwDxDrawQuads,
        __hwDxIndexedTris,
        __hwDxDrawMarkers,

        __hwDxGuiText,
        __hwDxGuiRaster,
        __hwDxGuiRectangle,
        __hwDxGuiPolyline,
        __hwDxGuiLines,
        __hwDxGuiPolygon,
        __hwDxOpenGuiList,
        __hwDxCloseGuiList,
        __hwDxCallGuiList,
        __hwDxDestroyGuiList,

        __hwDxOpenList,
        __hwDxCloseList,
        __hwDxCallList,
        __hwDxDestroyList,

        __hwDxGetTextState,
        __hwDxGetDrawSize,
    };

const hwDisplay
    hwDxDisplay = (const hwDisplay)&__hwDxInitFunc,
    hwDefaultDisplay = (const hwDisplay)&__hwDxInitFunc;

/* The current file descriptor */
static IDirect3D9
    *dx9;
dxDrawable
    *__hwdxCurrentContext;
int
    __hwdxDoPrune = 1,
    __hwdxPruneEye = 0,
    __hwdxShowBoxes = 0,
    __hwdxDisableCull = 0;

/* Some utility functions (see below) */
static void setP1P2( dxDrawable *draw,
                hwInt32 x, hwInt32 y, hwInt32 w, hwInt32 h );
static hwInt32 InitGraphicsFildes( dxDrawable *draw );
static double gettime( void );

static void
    (*eventCallback)(hwDrawable draw, hwWinEvent *event);
/* HACK!  This should probably not be named hw*GL* event handler! */
void
    (*__hwglEventHandler)( HWND hwnd, UINT msg, LPARAM wParam, LPARAM lParam );

static char
    *__hwWindowClass;

static LONG WINAPI __hwWindowProc
(
    HWND hwnd, UINT msg, LPARAM wParam, LPARAM lParam
)
{
    PAINTSTRUCT
        ps;
    hwWinEvent
        event;
    hwInt32
        clipRect[4];
    static int
        buttonState;
    int
        handled = 1;

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
    case WM_CHAR :
        event.type = HW_INPUT_KEYBOARD;
        event.keyboard.key = wParam;
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
            (*eventCallback)((hwDrawable)__hwdxCurrentContext, &event);
        }
        return 0;
    }
    else {
        return DefWindowProc( hwnd, msg, wParam, lParam );
    }
}

static void __hwCheckMessages( void )
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

/* hwInit - initialize the X display connection */
hwDisplay __hwDxCreate
(
    hwDisplay disp, hwDisplay shared,
    OS_DISPLAY_TYPE display
)
{
    dxDisplay
        *result;

    result = malloc( sizeof(dxDisplay) );
    if( !result )       return 0;

    memset(result, 0, sizeof(dxDisplay));

    result->disp.vtab = __hwDxInitFunc;
    __hwInternalInitDisplay( (hwDisplay)result, shared );

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
    result->dbuffer = 1; // Only support double buffer

    /* Set up the current time-of-day clock */
    __hwIntStartTime = __hwIntCurrTime = gettime();

    return (hwDisplay)result;
}

/* hwChooseVisual - choose an X visual which matches the request */
hwInt32 __hwDxChooseVisual
(
    hwDisplay display, hwInt32 required, hwInt32 desired
)
{
    // DX always uses the same pixel format (???)
    return 1;
}

OS_VISUAL_TYPE __hwDxExtractVisual( hwDisplay display )
{
    // DX has no "visual"
    return 0;
}

OS_DRAWABLE_TYPE __hwDxExtractWin( hwDisplay disp, hwDrawable draw )
{
    dxDrawable
        *win = (dxDrawable *)draw;

    return win->hDC;
}

/* Create a stoopid window of the given characteristics */
hwDrawable hwDxCreateWinUtility
(
    hwDisplay display,  /* Display to use */
    char *winName,      /* Name of the window to create */
    HWND parent,        /* Name of the parent window, NULL if not a child */
    hwInt32 winX, hwInt32 winY, /* Placement */
    hwInt32 winW, hwInt32 winH, /* Size */
    hwInt32 flags
)
{
    int
        mode;
    dxDisplay
        *disp = (dxDisplay *)display;
    dxDrawable
        *result;
    DWORD
        style;

    result = malloc( sizeof( dxDrawable ) );
    if( !result )       return 0;

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
    UpdateWindow( result->win );
    __hwCheckMessages();

    return (hwDrawable)result;
}

hwDrawable __hwDxCreateChildWin
(
    hwDisplay display,  /* Display to use */
    char *winName,      /* Name of the window to create */
    HWND parent,        /* Parent window */
    hwInt32 winX, hwInt32 winY, /* Placement */
    hwInt32 winW, hwInt32 winH, /* Size */
    hwInt32 flags
)
{
    return hwDxCreateWinUtility(display, winName, parent, winX, winY, winW, winH, flags);
}
hwDrawable __hwDxCreateWin
(
    hwDisplay display,  /* Display to use */
    char *winName,      /* Name of the window to create */
    hwInt32 winX, hwInt32 winY, /* Placement */
    hwInt32 winW, hwInt32 winH, /* Size */
    hwInt32 flags
)
{
    return hwDxCreateWinUtility(display, winName, NULL, winX, winY, winW, winH, flags);
}

void __hwDxCheckInput(hwDisplay display, void (*cb)(hwDrawable, hwWinEvent *))
{
    eventCallback = cb;
    __hwCheckMessages();
    eventCallback = 0;
}

void __hwDxGetMousePos(hwDisplay display, hwInt32 *x, hwInt32 *y )
{
    /* TBD */
}

hwDrawable __hwDxInitDrawable( hwDisplay display, OS_DRAWABLE_TYPE draw )
{
    dxDisplay
        *disp = (dxDisplay *)display;
    dxDrawable
        *Result;

    Result = malloc( sizeof(dxDrawable) );
    if( !Result ) {
        return 0;
    }
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

static void setP1P2( dxDrawable *draw,
                hwInt32 x, hwInt32 y, hwInt32 w, hwInt32 h )
{
    D3DVIEWPORT9 view;

    if( !draw ) {
        return;
    }
    draw->VDC_XMax = draw->winW = w;
    draw->VDC_YMax = draw->winH = h;

    view.X = x;
    view.Y = y;
    view.Width = w;
    view.Height = h;
    view.MinZ = 0.0f;
    view.MaxZ = 1.0f;
    IDirect3DDevice9_SetViewport( draw->d3dDevice, &view );
}

static hwInt32 InitGraphicsFildes( dxDrawable *draw )
{
    D3DDISPLAYMODE d3ddm;
    D3DPRESENT_PARAMETERS d3dpp; 
    RECT rect;

    if (!dx9) {
        dx9 = Direct3DCreate9( D3D_SDK_VERSION );
        if (!dx9) {
            return 0;
        }
    }

    if( FAILED( IDirect3D9_GetAdapterDisplayMode( dx9, D3DADAPTER_DEFAULT, &d3ddm ) ) ) {
        printf("D3D8_GetAdapterDisplayMode failed\n");
        return 0;
    }

    GetClientRect( draw->win, &rect );

    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed   = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = d3ddm.Format;
    d3dpp.BackBufferWidth = rect.right;
    d3dpp.BackBufferHeight = rect.bottom;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
    d3dpp.hDeviceWindow = draw->win;

    if( FAILED( IDirect3D9_CreateDevice( dx9,
                                   D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                                   draw->win,
                                   D3DCREATE_HARDWARE_VERTEXPROCESSING,
                                   &d3dpp, &draw->d3dDevice ) ) ) {
        printf("D3D8_create failed\n");
        return 0;
    }

    setP1P2( draw, 0, 0, rect.right, rect.bottom );

    if( FAILED( IDirect3DDevice9_Clear(draw->d3dDevice,
                                  0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                                  (D3DCOLOR) 0, 1.0f, 0) ) ) {
        printf("D3D8_Clear failed\n");
        return 0;
    }
    if( FAILED( IDirect3DDevice9_BeginScene(draw->d3dDevice) ) ) {
        printf("D3D8_BeginScene failed\n");
        return 0;
    }

    if( getenv( "HW_DISABLE_PRUNE" ) ) {
        __hwdxDoPrune = 0;
    }
    if( getenv( "HW_ENABLE_PRUNE" ) ) {
        __hwdxDoPrune = 1;
    }
    if( getenv( "HW_SHOW_BOXES" ) ) {
        __hwdxShowBoxes = 1;
    }
    if( getenv( "HW_PRUNE_EYE" ) ) {
        __hwdxPruneEye = 1;
    }
    if( getenv( "HW_DISABLE_CULL" ) ) {
        __hwdxDisableCull = 1;
    }
    IDirect3DDevice9_SetRenderState(draw->d3dDevice, D3DRS_LIGHTING, FALSE);
    IDirect3DDevice9_SetRenderState(draw->d3dDevice,
                                    D3DRS_SPECULARMATERIALSOURCE,
                                    D3DMCS_MATERIAL);
    IDirect3DDevice9_SetRenderState(draw->d3dDevice, D3DRS_LOCALVIEWER, TRUE);
    IDirect3DDevice9_SetRenderState(draw->d3dDevice, D3DRS_NORMALIZENORMALS,    
                                       TRUE);

    hwIdentity( draw->camMat );

    // TBD
    draw->maxTexNum = 1;

    draw->numLights = 0;
    draw->camSet = 0;
    draw->lightOn = 1;
    draw->ambFact = 1.0;
    draw->stackDepth = 0;
    draw->planesValid = 0;
    draw->animMask = 0xFFFFFFFF;
    draw->clearColor = 0;
    draw->renderMode = HW_RENDER_DEFAULT;
    __hwInitTextState( &draw->textState );
    return 1;
}

void __hwDxInitState( dxDrawable *draw )
{
    int
        i;

    draw->color[0] = -1.0;
    draw->color[1] = -1.0;
    draw->color[2] = -1.0;
    draw->ambFactSave = -1.0;
    draw->shininess = -1.0;
    draw->specColor[0] = -1.0;
    draw->specColor[1] = -1.0;
    draw->specColor[2] = -1.0;
    draw->shadeFlat = -1;
    draw->primSize = -1.0;
    draw->wireframeState = -1;
    if( draw->transp != -1 ) {
        draw->transp = -1;
        //glDisable( GL_POLYGON_STIPPLE );
    }
    if( draw->tmActive != -1 ) {
        draw->tmActive = -1;
        for( i = 0; i < HW_MAX_TEXTURES; i++ ) {
            //glActiveTextureARB( GL_TEXTURE0_ARB+i );
            //glDisable( GL_TEXTURE_2D );
            draw->tmId[i] = -1;
        }
    }
    if( draw->blend ) {
        draw->blend = 0;
        //glDisable( GL_BLEND );
    }
}

void __hwDxMakeCurrent( hwDisplay disp, hwDrawable draw )
{
    __hwdxCurrentContext = (dxDrawable *)draw;
    //wglMakeCurrent( __hwdxCurrentContext->hDC, __hwdxCurrentContext->ctx );
    __hwDxInitState( __hwdxCurrentContext );
}

const char *__hwDxGetInfo( hwDisplay disp, hwInt32 whichInfo )
{
    switch( whichInfo ) {
    case HW_INFO_HW :
        return "DX";
    case HW_INFO_VENDOR :
        return "Microsoft";
    case HW_INFO_RENDERER :
        return "DX9";
    case HW_INFO_VERSION :
        return "1.0";
    case HW_INFO_EXTENSIONS :
        return "";
    default :
        return 0;
    }
}

void __hwDxViewport
(
    hwDisplay disp, hwInt32 x, hwInt32 y, hwInt32 width, hwInt32 height
)
{
    setP1P2( __hwdxCurrentContext, x, y, width, height );
}

void __hwDxPushMat( hwDisplay disp, hwFloat Mat[4][4] )
{
    int
        n;

    if( !__hwdxCurrentContext ) return;
    n = __hwdxCurrentContext->stackDepth++;
    if( __hwdxCurrentContext->stackDepth > MSD )        return;

    if( n > 0 ) {
        hwMatMult(      __hwdxCurrentContext->matStack[n],
                        Mat,
                        __hwdxCurrentContext->matStack[n-1] );
        hwMatMult(      __hwdxCurrentContext->rawMatStack[n],
                        Mat,
                        __hwdxCurrentContext->rawMatStack[n-1] );
    }
    else {
        hwMatMult(      __hwdxCurrentContext->matStack[0],
                        Mat,
                        __hwdxCurrentContext->camMat );
        (void)memcpy( __hwdxCurrentContext->rawMatStack[0],
                        Mat, 4*4*sizeof(hwFloat) );
    }

    IDirect3DDevice9_SetTransform( __hwdxCurrentContext->d3dDevice,
                    D3DTS_WORLD,
                    (D3DMATRIX *)&__hwdxCurrentContext->matStack[n][0][0] );
}

void __hwDxPopMat( hwDisplay disp )
{
    int
        n;

    if( !__hwdxCurrentContext ) return;
    if( __hwdxCurrentContext->stackDepth < 1 )  return;

    n = --__hwdxCurrentContext->stackDepth;

    IDirect3DDevice9_SetTransform( __hwdxCurrentContext->d3dDevice,
                    D3DTS_WORLD,
                    (D3DMATRIX *)&__hwdxCurrentContext->matStack[n][0][0] );
}

void __hwDxXformPoint( hwDisplay disp, hwFloat point[3] )
{
    /* TBD */
}

void __hwDxUpdate( hwDisplay disp, hwInt32 updateFlags )
{
    double
        myTime;
    dxDrawable
        *draw = __hwdxCurrentContext;
    HRESULT
        res;

    if( !__hwdxCurrentContext ) return;

    __hwCheckMessages();

    if( updateFlags & HW_UPDATE_GUI ) {
        __hwDxDrawElements( disp );
    }

    (void) IDirect3DDevice9_EndScene(draw->d3dDevice);

    if( __hwdxCurrentContext->disp->dbuffer && (updateFlags & HW_UPDATE_SWAP)) {
        res = IDirect3DDevice9_Present(draw->d3dDevice,
                                 NULL, NULL,
                                 NULL/*__hwdxCurrentContext->win*/, NULL);
        if (res != D3D_OK) {
            printf("present failed, res = %d (%08x)\n", res, res);
            printf("INVALIDCALL = %08x\n", D3DERR_INVALIDCALL);
            printf("DEVICELOST = %08x\n", D3DERR_DEVICELOST);
        }
    }

    if( updateFlags & HW_UPDATE_CLEAR_COLOR ) {
        if( updateFlags & HW_UPDATE_CLEAR_DEPTH ) {
            (void) IDirect3DDevice9_Clear(draw->d3dDevice,
                                  0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                                  draw->clearColor, 1.0f, 0);

        }
        else {
            (void) IDirect3DDevice9_Clear(draw->d3dDevice,
                                  0, NULL, D3DCLEAR_TARGET,
                                  draw->clearColor, 0.0f, 0);
        }
    }
    else if( updateFlags & HW_UPDATE_CLEAR_DEPTH ) {
        (void) IDirect3DDevice9_Clear(draw->d3dDevice,
                                  0, NULL, D3DCLEAR_ZBUFFER,
                                  draw->clearColor, 0.0f, 0);
    }

    if( updateFlags & HW_UPDATE_MATRIX ) {
        /* TBD: Clear matrices to unit? */
        __hwdxCurrentContext->stackDepth = 0;

        __hwdxCurrentContext->numLights = 0;
        __hwdxCurrentContext->planesValid = 0;
        __hwdxCurrentContext->camSet = 0;

        hwIdentity( __hwdxCurrentContext->camMat );
    }

    if( updateFlags & HW_CLEAR_GUI ) {
        if( __hwdxCurrentContext->guiTail ) {
            __hwdxCurrentContext->guiTail->next = __hwdxCurrentContext->guiFree;
            __hwdxCurrentContext->guiFree = __hwdxCurrentContext->guiList;
        }
        __hwdxCurrentContext->guiList = NULL;
        __hwdxCurrentContext->guiTail = NULL;
    }

    if( updateFlags & HW_UPDATE_TIME ) {
        /* Step to the next frame */
        myTime = gettime();
        if( myTime < __hwIntCurrTime ) myTime = __hwIntCurrTime;
        __hwIntCurrTime = myTime;
        __hwIntElapsedTime = myTime - __hwIntStartTime;
    }
    IDirect3DDevice9_BeginScene(draw->d3dDevice);
}

hwInt32 __hwDxRenderMode ( hwDisplay disp, hwInt32 which )
{
    hwInt32
        oldMode;
#if 0
    if( !__hwdxCurrentContext ) return(0);

    if( which == HW_RENDER_QUERY) {
        return( __hwdxCurrentContext->renderMode);
    }

    switch( which & HW_RENDER_MASK ) {
    case HW_RENDER_DRAW:
        if( which & HW_RENDER_Z_TEST ) {
            glEnable( GL_DEPTH_TEST );
        }
        else {
            glDisable( GL_DEPTH_TEST );
        }
        if( which & HW_RENDER_Z_WRITE ) {
            glDepthMask( GL_TRUE );
        }
        else {
            glDepthMask( GL_FALSE );
        }

        if( which & HW_RENDER_MULTISAMPLE ) {
            glEnable( GL_HACK_MULTISAMPLE );
            glEnable( HP_HACK_MULTISAMPLE );
            __hwdxCurrentContext->disp->dbuffer |= 0x80000000;
        }
        else {
            if( __hwdxCurrentContext->disp->dbuffer & 0x80000000 ) {
                glDisable( GL_HACK_MULTISAMPLE );
                glDisable( HP_HACK_MULTISAMPLE );
            }
            __hwdxCurrentContext->disp->dbuffer &= ~0x80000000;
        }

        if( which & HW_RENDER_XOR ) {
            SetROP2( __hwdxCurrentContext->hDC, R2_XORPEN );
        }
        else {
            SetROP2( __hwdxCurrentContext->hDC, R2_COPYPEN );
        }
        if( which & HW_RENDER_EDGE_MODE ) {
            glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
        }
        else {
            glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
        }
        __hwdxCurrentContext->wireframeState = -1;
        break;
    case HW_RENDER_SELECT :
        __hwdxCurrentContext->sel.picked = 0;
        __hwdxCurrentContext->sel.dist = 1.e38;
        __hwdxCurrentContext->currObj = 0;
        break;
    }

    if( __hwdxDisableCull ) which &= ~HW_RENDER_CULL_FACE;


    oldMode = __hwdxCurrentContext->renderMode;
    __hwdxCurrentContext->renderMode = which;
    return(oldMode);
#else
    return 0;
#endif
}

void __hwDxSelectionInfo
(
    hwDisplay disp, hwInt32 flags,
    hwFloat x, hwFloat y, hwFloat aperture
)
{
#if 0
    if( !__hwdxCurrentContext ) return;

    __hwdxCurrentContext->sel.flags = flags;
    __hwdxCurrentContext->sel.point[0] = x;
    __hwdxCurrentContext->sel.point[1] = y;
    __hwdxCurrentContext->sel.aperture = aperture;
#endif
}

hwInt32 __hwDxWasSelected( hwDisplay disp, hwObject *obj, hwInt32 *vert )
{
#if 0
    int
        result;

    if( !__hwdxCurrentContext ) return 0;
    if( obj )   *obj = __hwdxCurrentContext->sel.obj;
    if( vert )  *vert = __hwdxCurrentContext->sel.vert;
    return __hwdxCurrentContext->sel.picked;
#else
    return 0;
#endif
}

void __hwDxCurrObject( hwDisplay disp, hwObject obj )
{
#if 0
    if( !__hwdxCurrentContext ) return;

    __hwdxCurrentContext->currObj = obj;
#endif
}

void __hwDxCurrChild( hwDisplay disp, hwObject obj )
{
#if 0
    if( !__hwdxCurrentContext ) return;

    if( __hwdxCurrentContext->sel.flags & HW_SELECT_CHILD ) {
        __hwdxCurrentContext->currObj = obj;
    }
#endif
}


void __hwDxSetVisibility( hwDisplay disp, hwInt32 mask )
{
    if( !__hwdxCurrentContext ) return;
    __hwdxCurrentContext->animMask = mask;
}

void __hwDxSetInvisibility( hwDisplay disp, hwInt32 incl, hwInt32 excl )
{
    if( !__hwdxCurrentContext ) return;
    __hwdxCurrentContext->animMask = 0;
    __hwdxCurrentContext->invisibleInclude = incl;
    __hwdxCurrentContext->invisibleExclude = excl;
}

hwInt32 __hwDxGetVisibility( hwDisplay disp)
{
    if( !__hwdxCurrentContext ) return(0);
    return(__hwdxCurrentContext->animMask);
}

hwTextState *__hwDxGetTextState( hwDisplay disp )
{
    if( !__hwdxCurrentContext )                 return 0;
    return &__hwdxCurrentContext->textState;
}

static double gettime( void )
{
    struct _timeb val;
    static struct _timeb FirstTime;
    static int isFirst = 1;

    if( isFirst ) {
        _ftime(&FirstTime);
        isFirst = 0;
    }

    _ftime(&val );
    /* Ugly hack - for some reason, when building DX, this calculation
     * is done in single precision.  So scale back to close to zero.
     */
     val.time -= FirstTime.time;
     if( val.millitm < FirstTime.millitm ) {
        val.millitm += 1000.0;
        val.time--;
    }
    val.millitm -= FirstTime.time;
    return( ((double)val.time) + (((double)val.millitm) / 1000.0) );
}

void __hwDxGetDrawSize( hwDisplay disp, hwInt32 *retSize )
{
    if( !__hwdxCurrentContext ) return;

    retSize[0] = __hwdxCurrentContext->winW;
    retSize[1] = __hwdxCurrentContext->winH;
}

/*** EOF dx_visual.c ***/
