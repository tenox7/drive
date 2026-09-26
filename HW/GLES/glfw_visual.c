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

#ifdef GLFW_DEBUG /* [ */

#   define GLFW_CALL(name, args) do { \
        printf("%s\n", #name); \
        name args; \
    } while (0)

#   define GLFW_FUNC(result, name, args) do { \
        printf("%s\n", #name); \
        result = name args; \
    } while (0)

#else /* ] [ */

#   define GLFW_CALL(name, args) name args
#   define GLFW_FUNC(result, name, args) result = name args

#endif

static GLFWwindow
    *origContext;

/* Some utility functions (see below) */
static void setP1P2( glDrawable *draw,
                hwInt32 x, hwInt32 y, hwInt32 w, hwInt32 h );
static hwInt32 InitGraphicsFildes( glDrawable *draw );

static void key_CB(GLFWwindow *win, int key, int scancode,
                         int action, int mods)
{
    hwWinEvent event;
    void (*eventCallback)(hwDrawable, hwWinEvent *);
    FETCH_GL_CTX;
    
    if (!glctx || !glctx->eventCB) return;
    eventCallback = glctx->eventCB;

    if (action == GLFW_RELEASE) return;

    switch (key) {
    case GLFW_KEY_ESCAPE :      key = '\033';           break;
    case GLFW_KEY_ENTER :       key = '\n';             break;
    case GLFW_KEY_TAB :         key = '\t';             break;
    case GLFW_KEY_BACKSPACE :   key = '\010';           break;
    case GLFW_KEY_DELETE :      key = '\177';           break;
    case GLFW_KEY_RIGHT :       key = HW_KEY_RIGHT;     break;
    case GLFW_KEY_LEFT :        key = HW_KEY_LEFT;      break;
    case GLFW_KEY_DOWN :        key = HW_KEY_DOWN;      break;
    case GLFW_KEY_UP :          key = HW_KEY_UP;        break;
    case GLFW_KEY_PAGE_UP :     key = HW_KEY_PREV;      break;
    case GLFW_KEY_PAGE_DOWN :   key = HW_KEY_NEXT;      break;
    case GLFW_KEY_HOME :        key = HW_KEY_HOME;      break;
    case GLFW_KEY_END :         key = HW_KEY_END;       break;
    case GLFW_KEY_F1 :          key = HW_KEY_F1;        break;
    case GLFW_KEY_F2 :          key = HW_KEY_F2;        break;
    case GLFW_KEY_F3 :          key = HW_KEY_F3;        break;
    case GLFW_KEY_F4 :          key = HW_KEY_F4;        break;
    case GLFW_KEY_F5 :          key = HW_KEY_F5;        break;
    case GLFW_KEY_F6 :          key = HW_KEY_F6;        break;
    case GLFW_KEY_F7 :          key = HW_KEY_F7;        break;
    case GLFW_KEY_F8 :          key = HW_KEY_F8;        break;
    case GLFW_KEY_F9 :          key = HW_KEY_F9;        break;
    case GLFW_KEY_F10 :         key = HW_KEY_F10;       break;
    case GLFW_KEY_F11 :         key = HW_KEY_F11;       break;
    case GLFW_KEY_F12 :         key = HW_KEY_F12;       break;
    default :                                           return;
    }

    event.type = HW_INPUT_KEYBOARD;
    event.keyboard.key = key;

    event.keyboard.mods = 0;
    if (mods & GLFW_MOD_SHIFT) {
        event.keyboard.mods |= HW_KBD_MOD_SHIFT;
    }
    if (mods & GLFW_MOD_CONTROL) {
        event.keyboard.mods |= HW_KBD_MOD_CTRL;
    }
    if (mods & GLFW_MOD_ALT) {
        event.keyboard.mods |= HW_KBD_MOD_ALT;
    }

    (*eventCallback)((hwDrawable)glctx, &event);
}

static void char_CB(GLFWwindow *win, unsigned int ch)
{
    FETCH_GL_CTX;
    hwWinEvent event;
    void (*eventCallback)(hwDrawable, hwWinEvent *);
    
    if (!glctx || !glctx->eventCB) return;
    eventCallback = glctx->eventCB;

    if ((ch < 32) || (ch > 127)) return;

    event.type = HW_INPUT_KEYBOARD;
    event.keyboard.key = (hwInt32) ch;
    event.keyboard.mods = 0;

    (*eventCallback)((hwDrawable)glctx, &event);
}

static void mouse_CB(GLFWwindow *win, int button, int action, int mods)
{
    FETCH_GL_CTX;
    hwWinEvent event;
    void (*eventCallback)(hwDrawable, hwWinEvent *);
    
    if (!glctx || !glctx->eventCB) return;
    eventCallback = glctx->eventCB;

    switch (button) {
    case 1 : button = 2; break;
    case 2 : button = 1; break;
    }

    if (action == GLFW_PRESS) {
        event.type = HW_INPUT_BUTTON_PRESS;
        glctx->buttonState |= (1 << button);
    }
    else {
        event.type = HW_INPUT_BUTTON_RELEASE;
        glctx->buttonState &= ~(1 << button);
    }

    event.button.kbdMods = 0;
    if (mods & GLFW_MOD_SHIFT) {
        event.button.kbdMods |= HW_KBD_MOD_SHIFT;
    }
    if (mods & GLFW_MOD_CONTROL) {
        event.button.kbdMods |= HW_KBD_MOD_CTRL;
    }
    if (mods & GLFW_MOD_ALT) {
        event.button.kbdMods |= HW_KBD_MOD_ALT;
    }

    glctx->kbdMods = event.button.kbdMods;

    event.button.button = button;
    event.button.x = glctx->mouseX;
    event.button.y = glctx->mouseY;

    (*eventCallback)((hwDrawable)glctx, &event);
}

static void motion_CB(GLFWwindow *win, double x, double y)
{
    FETCH_GL_CTX;
    hwWinEvent event;
    void (*eventCallback)(hwDrawable, hwWinEvent *);
    int ix, iy;
    
    if (!glctx || !glctx->eventCB) return;
    eventCallback = glctx->eventCB;

    ix = (int)(x + 0.5); iy = (int)(y + 0.5);

    glctx->mouseX = ix; glctx->mouseY = iy;

    if (!glctx->buttonState) return;

    event.type = HW_INPUT_POINTER;
    event.pointer.x = ix;
    event.pointer.y = iy;
    event.pointer.buttonState = glctx->buttonState;
    event.pointer.kbdMods = glctx->kbdMods;

    (*eventCallback)((hwDrawable)glctx, &event);
}

static void winClose_CB(GLFWwindow *win)
{
    /* TBD.  We should define an HW_INPUT_CLOSE event type. */
    GLFW_CALL(glfwTerminate,());
    exit(0);
}

static void winPos_CB(GLFWwindow *win, int x, int y)
{
    FETCH_GL_CTX;
    hwWinEvent event;
    void (*eventCallback)(hwDrawable, hwWinEvent *);
    int fw, fh;
    
    if (!glctx || !glctx->eventCB) return;
    eventCallback = glctx->eventCB;

    GLFW_CALL(glfwGetFramebufferSize,(win, &fw, &fh));
    glctx->winX = x; glctx->winY = x;
    glctx->winW = fw; glctx->winH = fh;

    event.type = HW_INPUT_CONFIG;
    event.config.posx = x;
    event.config.posy = y;
    event.config.width = glctx->winW;
    event.config.height = glctx->winH;

    (*eventCallback)((hwDrawable)glctx, &event);
}

static void winSize_CB(GLFWwindow *win, int w, int h)
{
    FETCH_GL_CTX;
    hwWinEvent event;
    void (*eventCallback)(hwDrawable, hwWinEvent *);
    
    if (!glctx || !glctx->eventCB) return;
    eventCallback = glctx->eventCB;

    GLFW_CALL(glfwGetFramebufferSize,(win, &w, &h));

    glctx->winW = w; glctx->winH = h;

    event.type = HW_INPUT_CONFIG;
    event.config.posx = glctx->winX;
    event.config.posy = glctx->winY;
    event.config.width = w;
    event.config.height = h;

    (*eventCallback)((hwDrawable)glctx, &event);
}

static void winFocus_CB(GLFWwindow *win, int focus)
{
    FETCH_GL_CTX;
    hwWinEvent event;
    void (*eventCallback)(hwDrawable, hwWinEvent *);
    
    if (!glctx || !glctx->eventCB) return;
    eventCallback = glctx->eventCB;

    event.type = HW_INPUT_FOCUS;
    event.focus.hasFocus = focus;

    (*eventCallback)((hwDrawable)glctx, &event);
}

static void winRefresh_CB(GLFWwindow *win)
{
    FETCH_GL_CTX;
    hwWinEvent event;
    void (*eventCallback)(hwDrawable, hwWinEvent *);
    hwInt32 clipRect[4];
    
    if (!glctx || !glctx->eventCB) return;
    eventCallback = glctx->eventCB;

    /* GLFW does not give us an exposure clip list */
    clipRect[0] = 0;
    clipRect[1] = 0;
    clipRect[2] = glctx->winW;
    clipRect[3] = glctx->winH;

    event.type = HW_INPUT_EXPOSE;
    event.expose.numClipRects = 1;
    event.expose.clipRectList = clipRect;

    (*eventCallback)((hwDrawable)glctx, &event);
}

static void checkJoystick(glDrawable *glctx)
{
    hwWinEvent
        event;
    void
        (*eventCallback)(hwDrawable, hwWinEvent *);
    glJoystick
        joy;
    const float
        *fp;
    const unsigned char
        *ucp;
    int
        i, j, num;

    if (!glctx) return;

    eventCallback = glctx->eventCB;
    if (!eventCallback) return;

    for (j = 0; j < HW_MAX_JOYSTICKS; j++) {
        joy = glctx->joystick[j];
        if (!joy) continue;

        fp = glfwGetJoystickAxes(GLFW_JOYSTICK_1+j, &num);
        if (fp) {
            for (i = 0; i < num; i++) {
                event.joystickAxis.type = HW_INPUT_JOY_AXIS;
                event.joystickAxis.joystickNum = j;
                event.joystickAxis.axis = i;
                event.joystickAxis.value = fp[i];
                eventCallback((hwDrawable)glctx, &event);
            }
        }

        ucp = glfwGetJoystickButtons(GLFW_JOYSTICK_1+j, &num);
        if (ucp) {
            if (num > JOY_MAX_BUTTONS) num = JOY_MAX_BUTTONS;
            for (i = 0; i < num; i++) {
                if (ucp[i] != joy->buttons[i]) {
                    event.joystickButton.type = ucp[i]
                                ? HW_INPUT_JOY_PRESS
                                : HW_INPUT_JOY_RELEASE;
                    event.joystickButton.joystickNum = j;
                    event.joystickButton.value = i;
                    eventCallback((hwDrawable)glctx, &event);
                    joy->buttons[i] = ucp[i];
                }
            }
        }
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
    const GLFWvidmode
        *vidMode;

    result = malloc( sizeof(glDisplay) );
    if( !result )       return 0;

    memset(result, 0, sizeof(glDisplay));

    result->disp.vtab = __hwGlInitFunc;
    __hwInternalInitDisplay( (hwDisplay)result, shared );

    __hwGlGetEnvVars( result );

    if (!glfwInit()) {
        free(result);
        return 0;
    }

    GLFW_FUNC(result->primaryMonitor, glfwGetPrimaryMonitor, ());
    if (!result->primaryMonitor) goto error;

    GLFW_FUNC(vidMode, glfwGetVideoMode, (result->primaryMonitor));
    if (!vidMode) goto error;

    result->w = vidMode->width;
    result->h = vidMode->height;

    if (0) {
error :
        free(result);
        GLFW_CALL(glfwTerminate, ());
        return 0;
    }

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

    /* Combine required flags into desired flags */
    desired |= required;

    /* Reset window hints to defaults */
    GLFW_CALL(glfwDefaultWindowHints,());

    if (desired & HW_VIS_STEREO) {
        GLFW_CALL(glfwWindowHint,(GLFW_STEREO, GL_TRUE));
    }
    if (desired & HW_VIS_AA_HI) {
        GLFW_CALL(glfwWindowHint,(GLFW_SAMPLES, 16));
    }
    else if (desired & HW_VIS_AA_MED) {
        GLFW_CALL(glfwWindowHint,(GLFW_SAMPLES, 8));
    }
    else if (desired & HW_VIS_AA) {
        GLFW_CALL(glfwWindowHint,(GLFW_SAMPLES, 4));
    }
    else {
        GLFW_CALL(glfwWindowHint,(GLFW_SAMPLES, 0));
    }

    disp->dbuffer = 1;  /* Always double-buffered */

    return desired | HW_VIS_SUCCESS;    /* Assume it will work... */
}

OS_VISUAL_TYPE __hwGlExtractVisual( hwDisplay display )
{
    glDisplay
        *disp = (glDisplay *)display;;

    return (OS_VISUAL_TYPE)disp;
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
    GLFWwindow *parent,
    hwInt32 winX, hwInt32 winY, /* Placement */
    hwInt32 winW, hwInt32 winH, /* Size */
    hwInt32 flags
)
{
    GLFWmonitor
        *mon;
    GLFWwindow
        *win;
    glDrawable
        *Result;
    glDisplay
        *disp = (glDisplay *)display;
    const char
        *s;

    Result = malloc( sizeof(glDrawable) );
    if( !Result ) {
        return 0;
    }

    memset(Result, 0, sizeof(glDrawable) );

    Result->disp = disp;
    Result->buffer = 0;

    mon = NULL;

    if (flags & HW_WIN_FULLSCREEN) {
        winX = 0;
        winY = 0;
        winW = disp->w;
        winH = disp->h;
        mon = disp->primaryMonitor;
        GLFW_CALL(glfwWindowHint,(GLFW_RESIZABLE, GL_FALSE));
        GLFW_CALL(glfwWindowHint,(GLFW_DECORATED, GL_FALSE));
    }
    else {
        /* Start invisible so we can position the window first */
        GLFW_CALL(glfwWindowHint,(GLFW_VISIBLE, GL_FALSE));
    }

    GLFW_FUNC(win, glfwCreateWindow, (winW, winH, winName, mon, origContext));
    if (!win) {
        free( Result );
        return 0;
    }

    Result->win = win;

    if (!(flags & HW_WIN_FULLSCREEN)) {
        //GLFW_CALL(glfwSetWindowPos,(win, winX, winY));
        GLFW_CALL(glfwShowWindow,(win));
    }

    if (flags & HW_WIN_INPUT) {
        GLFW_CALL(glfwSetKeyCallback,(win, key_CB));
        GLFW_CALL(glfwSetCharCallback,(win, char_CB));
        GLFW_CALL(glfwSetMouseButtonCallback,(win, mouse_CB));
        GLFW_CALL(glfwSetCursorPosCallback,(win, motion_CB));
        GLFW_CALL(glfwSetWindowPosCallback,(win, winPos_CB));
        GLFW_CALL(glfwSetWindowSizeCallback,(win, winSize_CB));
        GLFW_CALL(glfwSetWindowFocusCallback,(win, winFocus_CB));
        GLFW_CALL(glfwSetWindowRefreshCallback,(win, winRefresh_CB));
    }

    if (flags & HW_WIN_JOYSTICK) {
        int joy;

        for (joy = 0; joy < HW_MAX_JOYSTICKS; joy++) {
            if (!glfwJoystickPresent(GLFW_JOYSTICK_1+joy)) continue;
            Result->joystick[joy] = malloc(sizeof(struct __glJoystick));
            if (!Result->joystick[joy]) {
                free(Result);
                return 0;
            }
            GLFW_FUNC(s, glfwGetJoystickName, (GLFW_JOYSTICK_1+joy));
            Result->joystick[joy]->name = strdup(s);
        }
    }

    Result->VDC_XMax = Result->winW = winW;
    Result->VDC_YMax = Result->winH = winH;

    /* We always do this */
    GLFW_CALL(glfwSetWindowCloseCallback,(win, winClose_CB));


    if( !InitGraphicsFildes( Result ) ) {
        free( Result );
        return NULL;
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
    OS_WINDOW_TYPE parent,
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
    double
        winX, winY;

    GLFW_CALL(glfwGetCursorPos,(glctx->win, &winX, &winY));
    *x = (int)(winX + 0.5);
    *y = (int)(winY + 0.5);
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
    int fw, fh;

    if (!origContext) {
        origContext = draw->win;
    }

    GLFW_CALL(glfwMakeContextCurrent,(draw->win));

    GLFW_CALL(glfwGetFramebufferSize,(draw->win, &fw, &fh));
    draw->winW = fw;
    draw->winH = fh;

    setP1P2( draw, 0, 0, draw->winW, draw->winH);

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

    GLFW_CALL(glfwMakeContextCurrent,(glctx->win));
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
    USE_GL_CTX(disp);

    GLFW_CALL(glfwPollEvents,());
    if (glctx->joystick) {
        checkJoystick(glctx);
    }
}

void __hwGlSwapBuffers( glDrawable *glctx )
{
    GLFW_CALL(glfwSwapBuffers, (glctx->win));
}

/*** EOF glfw_visual.c ***/
