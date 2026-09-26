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


/* 
 * drive.c - Client.
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#ifndef WIN32
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <netinet/in.h>

#ifndef MAC
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#ifdef _HPUX_SOURCE
#include <X11/XHPlib.h>
#endif
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#endif
#endif

#include <math.h>

#include "libnum.h"

#include "global.h"
#include "message.h"
#include "newMessage.h"
#include "sockets.h"
#include "gauge.h"
#include "camera.h"
#include "trigtable.h"
#include "drive.h"
#if !(defined(WIN32) || defined(MAC))
#include "gui.h"
#include "wsutils.h"
#endif
#include "filenames.h"

#include "hw.h"

#ifdef _IBM_SOURCE
#include <sys/select.h>
#endif

#include "newMessage.h"


/*************************** PROGRAM GLOBALS **********************************/
socket_type *server_socket;
drive_state_type cstate;
int
    save_XPointer_numerator,
    save_XPointer_denominator,
    save_XPointer_threshold;
int img_fildes;
int connection_id;
int num_cars;
Driveable driveables[MAX_DRIVEABLES];
gear_type  gear;
gauge_type left_main_gauge,right_main_gauge, turbometer, blastometer;
compass_type compass;
radar_type radar;
char *hostname;
int debug;
int showfps;

/**************************** MODULE GLOBALS **********************************/
/* Send and receive buffers. */
static struct timeval
    start_session_time,
    stop_session_time;	/* for debug */

#define TIMING_ITERATIONS		4
#define MAXIMUM_LARGEWINDOW_FRAMETIME	(1.0/14.0)	/* seconds */
#define MINIMUM_INVIS_FRAMETIME_RATIO	1.2

#if defined(WIN32) || defined(MAC)
/* Stub GUI routines */
hwDisplay disp;

/* The gauge, gear and dash entry points live in hwdash.c for the GLFW
 * build, which draws them as an overlay on the main window.
 */
#ifndef GLFW
void initGaugeModule(void) {}
void addGauge(gauge_type *g) {}
void removeGauge(gauge_type *g) {}
void gaugeDraw(gauge_type *g) {}
void gaugeUpdate(gauge_type *g, float value, boolean_type force) {}
void addGear(gear_type *g) {}
void removeGear(gear_type *g) {}
void gearDraw(gear_type *g) {}
void compassDraw(void) {}
void compassUpdate(compass_type *c, float travel_angle, float home_angle, boolean_type force) {}
void bargraphUpdate(float value, boolean_type force) {}
void redrawDash(void) {}
#endif
void segment_receive_indicator_on(void) {}
void segment_receive_indicator_off(void) {}

void mapVehicleSelectionWindow(void) {}
void grabCursor(void) {}

static void winCallback( hwDrawable draw, hwWinEvent *event )
{
#ifdef GLFW
    /* Let the menu bar have first refusal, so clicking a menu does not
     * also shift gear.
     */
    if (dashMenuEvent( event )) return;
#endif
    switch (event->type) {
    case HW_INPUT_CONFIG :
#ifdef GLFW
        /* hwdash.c reserves the dash and the acc/brake bar, and sets the
         * viewport and cstate.gWin* to what is left for the 3D view.
         */
        dashResize( event->config.width, event->config.height );
#else
        cstate.gWinWidth = event->config.width;
        cstate.gWinHeight = event->config.height;
        disp->viewport(disp, 0, 0, event->config.width, event->config.height );
#endif
        break;
    case HW_INPUT_KEYBOARD :
        process_keypress( event->keyboard.key, event->keyboard.mods );
        break;
    case HW_INPUT_POINTER :
        /*cstate.mouseX = event->pointer.x;*/
        /*cstate.mouseY = event->pointer.y;*/
        cstate.mouseMods = event->pointer.buttonState;
        cstate.mouseMods |= event->pointer.kbdMods << 4;
        break;
    case HW_INPUT_BUTTON_PRESS :
        cstate.mouseMods &= 0xF;
        cstate.mouseMods |= 1 << event->button.button;
        cstate.mouseMods |= event->button.kbdMods << 4;
        break;
    case HW_INPUT_BUTTON_RELEASE :
        cstate.mouseMods &= 0xF;
        cstate.mouseMods &= ~(1 << event->button.button);
        cstate.mouseMods |= event->button.kbdMods << 4;
        break;
    }
}

void processXEvents(void)
{
    disp->getMousePos( disp, &cstate.mouseX, &cstate.mouseY );
}

void smallWindowCallback(void) {}
void updateGUIVehicles(Driveable *driveables, int num_cars) {}
void updateRacePosition( char *text, int position, boolean_type highlight,
    float r, float b, float g) {}
void updateRacePositionType(boolean_type is_final, int players) {}
extern void radarUpdate(radar_type *r) {}
void redrawCheckpoint(void) {}
void updateStateWindow(int state) {}
void showPlace(int position) {}
void mapTitleWindow(void) {}
void unmapTitleWindow(void) {}
void drawWheel(float xval) {}
void undrawWheel(void) {}
int createGUI(
    int argc,
    char *argv[],
    int forceSmall,
    void (*resize_routine)
	(int width, int height),
    int (*select_visuals_routine)
	(void *vdash, void *vgraphics))
{
    hwDrawable draw;

    if (!hwInit(argc, argv)) return 0;
    disp = hwDefaultDisplay->create( hwDefaultDisplay, NULL, NULL );
    if (!disp) {
        return 0;
    }
    if( !disp->chooseVisual(disp, HW_VIS_DBUFF | HW_VIS_DEPTH, 0) ) {
        return 0;
    }
    /* The original was WIN_LARGEWIDTH x WIN_LARGEHEIGHT from
     * libgui/windows.h.  At this width the 800px dash artwork lands
     * roughly 1:1, so it is not upscaled.  Override with DRIVE_WINDOW=WxH.
     */
    {
        int w = DRIVE_WIN_WIDTH, h = DRIVE_WIN_HEIGHT;
        char *e = getenv("DRIVE_WINDOW");

        if (e) sscanf(e, "%dx%d", &w, &h);
        if (w < 320) w = 320;
        if (h < 240) h = 240;

        draw = disp->createWindow( disp, "Drive", 50, 50, w, h, HW_WIN_INPUT );
        cstate.gWinWidth = w;
        cstate.gWinHeight = h;
    }
    if( !draw ) {
        return 0;
    }
    disp->makeCurrent( disp, draw );
    disp->inputHandler( disp, winCallback );
#ifdef GLFW
    /* No resize event arrives for the initial size, so lay the dash and
     * menu bar out for it here.  Must follow makeCurrent(): it sets the
     * viewport, and there is no GL context before that.
     */
    dashResize( cstate.gWinWidth, cstate.gWinHeight );
#endif
    return 1;
}
#ifndef GLFW
void redrawDash(void) {}
#endif
void xDrawSight(int width, int height, boolean_type do_crosshair) {}
void set_pex_table_entries(void *color_lut) {}

#if 0
Boolean PEXGetVisuals(
    XVisualInfo *vdash,
    XVisualInfo *vgraphics);

Boolean StarbaseGetVisuals(
    XVisualInfo *vdash,
    XVisualInfo *vgraphics);
#endif

#ifndef WIN32
/*****************************************************************
 * start_private_server
 *
 *	Run our own drive_server, with no console and no terminal of its
 *	own, so the game is a single self-contained program.  Enabled
 *	with DRIVE_LOCAL_SERVER=1, and always inside a macOS .app, which
 *	has nowhere to set it.  The server exits when we disconnect, and
 *	is killed outright if we quit first.
 */
static pid_t private_server_pid;

static void stop_private_server(void)
{
    if (private_server_pid > 0) kill(private_server_pid,SIGTERM);
    private_server_pid = 0;
}

/* Inside a .app, construct_filenames() leaves us in Contents/Resources. */
static boolean_type in_app_bundle(void)
{
    static const char rsrc[] = "/Contents/Resources";
    int n = strlen(drivedir) - (sizeof(rsrc) - 1);

    return (n > 0) && (strcmp(drivedir + n, rsrc) == 0);
}

static void start_private_server(void)
{
    boolean_type bundled = in_app_bundle();
    pid_t pid;

    if (!bundled && (getenv("DRIVE_LOCAL_SERVER") == NULL)) return;

    pid = fork();
    if (pid < 0) return;

    if (pid == 0) {
	/* The server opens its textures relative to the game directory. */
	chdir(drivedir);
	setenv("DRIVE_NO_CONSOLE","1",1);
	setenv("DRIVE_EXIT_WHEN_EMPTY","1",1);
	/* Endless practice is what a double-clicked app is for. */
	if (bundled && (getenv("DRIVE_PRACTICE_MODE") == NULL))
	    setenv("DRIVE_PRACTICE_MODE","1",1);
	freopen("/dev/null","r",stdin);
	freopen("/dev/null","w",stdout);
	freopen("/dev/null","w",stderr);
	/* Absolute argv[0]: the server finds its scenes the same way we do. */
	execl(server_programname,server_programname,(char *)NULL);
	_exit(1);
    }

    private_server_pid = pid;
    atexit(stop_private_server);
}
#else
#define start_private_server()
#endif

char *findServer(void)
{
    static char result[1024];
#ifndef WIN32
    /* Our own server is on the loopback, not on whatever this host calls
     * itself. */
    if (private_server_pid > 0) return "localhost";
#endif
    if (!*result) {
        gethostname(result, sizeof(result));
    }
    return result;
}

void badServerMessage(char *bad_server_name) {}
void colornameToRGB(char *colorname, float *r, float *g, float *b) {}
void cuserid( char *foo ) {}

char *getlogin(void) {return "";}

#endif // ]

/******************************************************************************/

void update_gauge_from_message(
    gauge_type *gauge,
    struct SrvFrameGauge *gauge_msg)
{

    switch (gauge->class) {
	case GAUGE_ANALOG_MPH:
	case GAUGE_DIGITAL_MPH:
	    gaugeUpdate(gauge, gauge_msg->mph, FALSE);
	    break;
	case GAUGE_ANALOG_RPM:
	    gaugeUpdate(gauge, gauge_msg->rpm/1000.0, FALSE);
	    break;
	case GAUGE_DIGITAL_RPM:
	    gaugeUpdate(gauge, gauge_msg->rpm, FALSE);
	    break;
	case GAUGE_ANALOG_ALTITUDE:
	    gaugeUpdate(gauge, gauge_msg->altitude/1000.0, FALSE);
	    break;
	case GAUGE_DIGITAL_ALTITUDE:
	    gaugeUpdate(gauge, gauge_msg->altitude, FALSE);
	    break;
    }
}


static void refresh_gauges(
    void)
{
    gaugeDraw(&left_main_gauge);
    gaugeDraw(&right_main_gauge);
    gaugeDraw(&turbometer);
    gaugeDraw(&blastometer);
    compassUpdate(&compass,cstate.compass_angle,0.0,FALSE);
    gearDraw(&gear);
    drawWheel(cstate.xpointer_value);
    bargraphUpdate(cstate.accBrk_value,FALSE);
}

static void check_skycam_dirty()
{
    /* If values have changed, recalculate camera position
     * and up vector.
     */
    if (cstate.camera.skycam_dirty) {
	float cosalpha = cos_table[cstate.camera.skycam_up_angle];
	float sinalpha = sin_table[cstate.camera.skycam_up_angle];
	float costheta = cos_table[cstate.camera.skycam_around_angle];
	float sintheta = sin_table[cstate.camera.skycam_around_angle];

	cstate.camera.skycam_x = cstate.camera.skycam_distance
	    * cosalpha * costheta;
	cstate.camera.skycam_y = cstate.camera.skycam_distance
	    * sinalpha;
	cstate.camera.skycam_z = cstate.camera.skycam_distance
	    * cosalpha * sintheta;

	cstate.camera.skycam_dirty = FALSE;
    }

}

void process_keypress(
#if defined(WIN32) || defined(MAC)
    int key, int mod
#else
    KeySym key, unsigned int keyState
#endif
)
{
    switch (key) {
#ifdef GLFW
        case '?' : case '/' : /* Keyboard help */
            toggleHelpPanel();
            break;
#endif
#if defined(WIN32) || defined(MAC)
        case '.' : case '>' :
#else
	case XK_period: /* Shift up */
        case XK_greater:
#endif
            process_button(3);
            break;
#if defined(WIN32) || defined(MAC)
        case ',' : case '<' :
#else
	case XK_comma: /* Shift down */
        case XK_less:
#endif
            process_button(1);
            break;
#if defined(WIN32) || defined(MAC)
        case 'r' : case 'R' :
#else
	case XK_r: /* Restart */
#endif
            client_restart();
            break;
#if defined(WIN32) || defined(MAC)
        case 'u' : case 'U' :
#else
	case XK_u: /* Upright */
#endif
            client_upright();
            break;
#if defined(WIN32) || defined(MAC)
        case 'f' : case 'F' : case HW_KEY_UP :
#else
	case XK_f: case XK_Up:  /* front */
#endif
	   if ((cstate.camera.mode == CAM_MODE_SKYCAM)
		   || (cstate.camera.mode == CAM_MODE_AUTOCAM)) {
	       if ((cstate.camera.skycam_up_angle
		       += SKYCAM_UPANGLE_INC) > SKYCAM_UPANGLE_MAX) {
		   cstate.camera.skycam_up_angle = SKYCAM_UPANGLE_MAX;
	       }
	       cstate.camera.skycam_dirty = TRUE;
	   }
	   else {
	       cstate.camera.direction = CAM_DIRECTION_FRONT;
	   }
	   break;
#if defined(WIN32) || defined(MAC)
        case 's' : case 'S' : case HW_KEY_LEFT :
#else
	case XK_s: case XK_Left:  /* left */
#endif
	   if ( cstate.camera.mode == CAM_MODE_SKYCAM ) {
	       if ((cstate.camera.skycam_around_angle
		       -= SKYCAM_AROUNDANGLE_INC) < 0) {
		   cstate.camera.skycam_around_angle += 360;
	       }
	       cstate.camera.skycam_dirty=TRUE;
	   }
	   else {
	       cstate.camera.direction = CAM_DIRECTION_LEFT;
	   }
	   break;

#if defined(WIN32) || defined(MAC)
        case 'd' : case 'D' : case HW_KEY_RIGHT :
#else
	case XK_d: case XK_Right:  /* right */
#endif
	   if ( cstate.camera.mode == CAM_MODE_SKYCAM ) {
	       if ((cstate.camera.skycam_around_angle
		       += SKYCAM_AROUNDANGLE_INC) > 359) {
		   cstate.camera.skycam_around_angle -= 360;
	       }
	       cstate.camera.skycam_dirty=TRUE;
	   }
	   else {
	       cstate.camera.direction = CAM_DIRECTION_RIGHT;
	   }
	   break;

#if defined(WIN32) || defined(MAC)
        case 'a' : case 'A' : case HW_KEY_DOWN :
#else
	case XK_a: case XK_Down:  /* back */
#endif
	   if ((cstate.camera.mode == CAM_MODE_SKYCAM) 
		   || (cstate.camera.mode == CAM_MODE_AUTOCAM)) {
	       if ((cstate.camera.skycam_up_angle
			-= SKYCAM_UPANGLE_INC) < SKYCAM_UPANGLE_MIN) {
		   cstate.camera.skycam_up_angle = SKYCAM_UPANGLE_MIN;
	       }
	       cstate.camera.skycam_dirty = TRUE;
	   }
	   else {
	       cstate.camera.direction = CAM_DIRECTION_BACK;
	   }
	   break;

#if defined(WIN32) || defined(MAC)
        case 'i' : case 'I' : case HW_KEY_NEXT :
#else
	case XK_Next: case XK_i:
#endif
	   if ((cstate.camera.mode == CAM_MODE_SKYCAM)
		   || (cstate.camera.mode == CAM_MODE_AUTOCAM)) {
#if defined(WIN32) || defined(MAC)
               if ((key == 'I') || (mod & HW_KBD_MOD_SHIFT))
#else
	       if (ShiftMask & keyState)
#endif
               {
		   cstate.camera.skycam_distance -= SKYCAM_DISTANCE_INC * 10;
	       }
	       else {
		   cstate.camera.skycam_distance -= SKYCAM_DISTANCE_INC;
	       }
	       if (cstate.camera.skycam_distance < SKYCAM_DISTANCE_MIN) {
		   cstate.camera.skycam_distance = SKYCAM_DISTANCE_MIN;
	       }
	       cstate.camera.skycam_dirty = TRUE;
	   }
	   else if( cstate.camera.mode == CAM_MODE_ZOOM_VIEW)
	   {
#if defined(WIN32) || defined(MAC)
               if ((key == 'I') || (mod & HW_KBD_MOD_SHIFT))
#else
	       if (ShiftMask & keyState)
#endif
               {
		   cstate.gunsight_fov -= SKYCAM_DISTANCE_INC;
		   if (cstate.gunsight_fov < 1.0) cstate.gunsight_fov = 1.0;
	       }
	       else {
		   cstate.gunsight_fov -= SKYCAM_DISTANCE_INC / 4.0;
		   if (cstate.gunsight_fov < 1.0) cstate.gunsight_fov = 1.0;
	       }
	       cstate.camera.skycam_dirty = TRUE;
	   }
	   break;

#if defined(WIN32) || defined(MAC)
        case 'o' : case 'O' : case HW_KEY_PREV :
#else
	case XK_Prior: case XK_o:
#endif
	   if ((cstate.camera.mode == CAM_MODE_SKYCAM) 
		   || (cstate.camera.mode == CAM_MODE_AUTOCAM)) {
#if defined(WIN32) || defined(MAC)
               if ((key == 'O') || (mod & HW_KBD_MOD_SHIFT))
#else
	       if (ShiftMask & keyState)
#endif
               {
		   cstate.camera.skycam_distance += SKYCAM_DISTANCE_INC * 10;
	       }
	       else {
		   cstate.camera.skycam_distance += SKYCAM_DISTANCE_INC;
	       }
	       cstate.camera.skycam_dirty = TRUE;
	   }
	   else if( cstate.camera.mode == CAM_MODE_ZOOM_VIEW)
	   {
#if defined(WIN32) || defined(MAC)
               if ((key == 'O') || (mod & HW_KBD_MOD_SHIFT))
#else
	       if (ShiftMask & keyState)
#endif
               {
		   cstate.gunsight_fov += SKYCAM_DISTANCE_INC;
		   if (cstate.gunsight_fov > 100.0) cstate.gunsight_fov = 100.0;
	       }
	       else {
		   cstate.gunsight_fov += SKYCAM_DISTANCE_INC / 4.0;
		   if (cstate.gunsight_fov > 100.0) cstate.gunsight_fov = 100.0;
	       }
	       cstate.camera.skycam_dirty = TRUE;
	   }
	   break;

#ifdef OLD_TRANSMISSION_SWITCHING
#if defined(WIN32) || defined(MAC)
        case 'a' : case 'A' :
#else
	case XK_a:  /* Automatic Tranmission */
#endif
	    gear.type = GEAR_AUTOMATIC;
	    gear.labels[0] = "R";
	    gear.labels[1] = "N";
	    gear.labels[2] = "L1";
	    gear.labels[3] = "L2";
	    gear.labels[4] = "L3";
	    gear.labels[5] = "D";
	    gear.labels[6] = "OD";
	    gearDraw(&gear);
	    break;

#if defined(WIN32) || defined(MAC)
        case 'm' : case 'M' :
#else
	case XK_m:  /* Manual Transmission */
#endif
	    gear.type =  GEAR_STANDARD;
	    gear.labels[0] = "R";
	    gear.labels[1] = "N";
	    gear.labels[2] = "1";
	    gear.labels[3] = "2";
	    gear.labels[4] = "3";
	    gear.labels[5] = "4";
	    gear.labels[6] = "5";
	    gearDraw(&gear);
	    break;
#endif /* OLD_TRANSMISSION_SWITCHING */

#ifdef NEED_KEYBOARD_ENABLER
#if defined(WIN32) || defined(MAC)
        case 'k' : case 'K' :
#else
	case XK_k:
#endif
	   cstate.mode ^= CLIENT_KEYACC_MODE;
	   break;
#endif /* NEED_KEYBOARD_ENABLER */

#if defined(WIN32) || defined(MAC)
        case 't' : case 'T' :
#else
	case XK_t: /* Turbo Mode! */
#endif
	   if (!(cstate.mode & CLIENT_ROBUST_MODE)) {
		cstate.mode |= CLIENT_TURBO_MODE;
	    }
	    break;

#if defined(WIN32) || defined(MAC)
        case 'w' : case 'W' :
#else
	case XK_w: /* Toggle name identification */
#endif
	    if( cstate.mode & CLIENT_SHOW_NAMES_MODE) 
	    {
		cstate.mode &= ~CLIENT_SHOW_NAMES_MODE;
	    }
	    else
	    {
		cstate.mode |= CLIENT_SHOW_NAMES_MODE;
	    }

	    break;

#if defined(WIN32) || defined(MAC)
        case ' ' :
#else
	case XK_space: /* Full brake -- don't move pointer */
#endif
	    cstate.mode |= CLIENT_BRAKES_ON;
	    break;

#if !(defined(WIN32) || defined(MAC))
	case XK_c: /* Confine/Unconfine Cursor */
	   if(cstate.mode & CLIENT_CONSTRAIN_CURSOR_MODE ) {
	       cstate.mode ^= CLIENT_CONSTRAIN_CURSOR_MODE;
	       XUngrabPointer(cstate.display,CurrentTime);
	       printf("Release cursor lock\n");
	   }
	   else {
	       cstate.mode |= CLIENT_CONSTRAIN_CURSOR_MODE;
	       grabCursor();
	       printf("Grab cursor \n");
	   }
#if 0
	   XWarpPointer(cstate.display,None,cstate.graphicsWindow,
	       0,0,0,0,(cstate.gWinWidth / 2),(cstate.gWinHeight /2) );
#endif
	   break;
#endif

#if defined(WIN32) || defined(MAC)
        case 'l' : case 'L' :
#else
	case XK_l: /* Headlight on/off. */
#endif
	   headlight_toggle( img_fildes, &cstate.headlight,
		server_socket, &cstate.daylight.mask );
	   break;

#if defined(WIN32) || defined(MAC)
        case 'h' : case 'H' :
#else
	case XK_h: /* Headlight high beam on/off. */
#endif
	   headlight_highbeam( &cstate.headlight );
	   break;

#if defined(WIN32) || defined(MAC)
        case HW_KEY_HOME : case 'v' : case 'V' :
#else
	case XK_Home: case XK_v:
#endif
	   if ( cstate.camera.mode == CAM_MODE_SKYCAM )
	       cstate.camera.mode = CAM_MODE_NORMAL;
	   else
	       cstate.camera.mode = CAM_MODE_SKYCAM;
	   break;

#if defined(WIN32) || defined(MAC)
        case 'g' : case 'G' :
#else
	case XK_g:
#endif
	   if (cstate.camera.mode == CAM_MODE_ALT_VIEW)
	       cstate.camera.mode = CAM_MODE_ZOOM_VIEW;
	   else if( cstate.camera.mode == CAM_MODE_ZOOM_VIEW) 
	       cstate.camera.mode = CAM_MODE_NORMAL;
	   else 
	       cstate.camera.mode = CAM_MODE_ALT_VIEW;
	   break;

#if defined(WIN32) || defined(MAC)
        case '+' : case 'z' : case 'Z' :
#else
	case XK_KP_Add: /* acceleration */
	case XK_plus:
	case XK_z:
#endif
	    if (cstate.acc_value < 0.0) {
		cstate.acc_value = 0.0;
	    }
	    else {
		if ((cstate.acc_value += ACC_INC) > ACC_MAX) {
		    cstate.acc_value = ACC_MAX;
		}
		cstate.mode |= CLIENT_USE_KEYACCEL;
	    }
	    if( cstate.desired_throttle < 1.0 )
		cstate.desired_throttle += THROTTLE_DELTA;
	    break;

#if defined(WIN32) || defined(MAC)
        case '-' : case 'x' : case 'X' :
#else
	case XK_KP_Subtract: /* Brake */
	case XK_minus:
	case XK_x:
#endif
	    if (cstate.acc_value > 0.0) {
		cstate.acc_value = 0.0;
	    }
	    else {
		if ((cstate.acc_value -= BRAKE_INC) < BRAKE_MAX) {
		    cstate.acc_value = BRAKE_MAX;
		}
		cstate.mode |= CLIENT_USE_KEYACCEL;
	    }
	    if( cstate.desired_throttle > 0.0 )
		cstate.desired_throttle -= THROTTLE_DELTA;
	    break;

#if defined(WIN32) || defined(MAC)
        case '\r' : case '\n' : case'\033' :
#else
	case XK_KP_Enter: /* Cruise Control */
	case XK_Escape:
#endif
	    cstate.mode ^= CLIENT_CRUISE_CONTROL_MODE;
	    break;
#if defined(WIN32) || defined(MAC)
        case 'b' : case 'B' :
#else
	case XK_b:  /* Honk the horn */
#endif
	    play_sound( SOUND_MYHORN);
	    break;
#if defined(WIN32) || defined(MAC)
        case '`' :
#else
	case XK_grave:
#endif
	    cstate.desired_throttle = 0.0;
	    break;
#if defined(WIN32) || defined(MAC)
        case '1' :
#else
	case XK_1:
#endif
	    cstate.desired_throttle = 0.1;
	    break;
#if defined(WIN32) || defined(MAC)
        case '2' :
#else
	case XK_2:
#endif
	    cstate.desired_throttle = 0.2;
	    break;
#if defined(WIN32) || defined(MAC)
        case '3' :
#else
	case XK_3:
#endif
	    cstate.desired_throttle = 0.3;
	    break;
#if defined(WIN32) || defined(MAC)
        case '4' :
#else
	case XK_4:
#endif
	    cstate.desired_throttle = 0.4;
	    break;
#if defined(WIN32) || defined(MAC)
        case '5' :
#else
	case XK_5:
#endif
	    cstate.desired_throttle = 0.5;
	    break;
#if defined(WIN32) || defined(MAC)
        case '6' :
#else
	case XK_6:
#endif
	    cstate.desired_throttle = 0.6;
	    break;
#if defined(WIN32) || defined(MAC)
        case '7' :
#else
	case XK_7:
#endif
	    cstate.desired_throttle = 0.7;
	    break;
#if defined(WIN32) || defined(MAC)
        case '8' :
#else
	case XK_8:
#endif
	    cstate.desired_throttle = 0.8;
	    break;
#if defined(WIN32) || defined(MAC)
        case '9' :
#else
	case XK_9:
#endif
	    cstate.desired_throttle = 0.9;
	    break;
#if defined(WIN32) || defined(MAC)
        case '0' :
#else
	case XK_0:
#endif
	    cstate.desired_throttle = 1.0;
	    break;
    }

    check_skycam_dirty();
}

void process_button(
    unsigned int button)
{
    switch (button) {
	case 1:
	    if (cstate.mode & CLIENT_ROBUST_MODE) {
		if (gear.actual > 0) gear.actual = 0;
	    }
	    else {
		if ((--gear.actual) < 0) gear.actual = 0;
	    }
	    gearDraw(&gear);
	    break;
	case 3:
	    if (cstate.mode & CLIENT_ROBUST_MODE) {
		if (gear.actual == 0) gear.actual = 2;
	    }
	    else {
		if ((++gear.actual) >= gear.num_gears) {
		    gear.actual = gear.num_gears-1;
		}
	    }
	    gearDraw(&gear);
	    break;
    }
}


static Driveable *find_driveable(
    char *name)
{
    Driveable *d;
    int i;

    /* Which driveable is it? */
    for (i=0,d=driveables; i<num_cars; ++i,++d) {
        if (strcmp(d->name,name) == 0) break;
    }
    if (i == num_cars) {
        for (i=0,d=driveables; i<num_cars; ++i,++d) {
            if (strcmp(d->name,DEFAULT_VEHICLE) == 0) break;
        }
        if (i == num_cars) {
	    d = driveables + 1;
        }
    }

    return(d);
}


static void setup_gauge(
    gauge_type *gauge,
    GAUGE_CLASS class,
    float x, float y,
    float max_speed,
    float max_rpm,
    float max_altitude,
    int font_index)
{
    gauge->class = class;
    gauge->font_index   = font_index;

    switch (class) {
	case GAUGE_ANALOG_MPH:
	    gauge->type		= GAUGE_ANALOG;
	    gauge->min_input	= 0.0;
	    gauge->max_input	= max_speed;
	    gauge->begin	= -120;
	    gauge->end		= 120;
	    gauge->value	= 0.0;
	    gauge->label	= "mph";
	    gauge->min		= 0;
	    gauge->max		= (int) max_speed;
	    break;

	case GAUGE_DIGITAL_MPH:
	    gauge->type		= GAUGE_DIGITAL;
	    gauge->min_input	= 0.0;
	    gauge->max_input	= max_speed;
	    gauge->begin	= 0;
	    gauge->end		= (int) max_speed;
	    gauge->value	= -1;
	    gauge->label	= "MPH";
	    gauge->min		= 0;
	    gauge->max		= (int) max_speed;
	    break;

	case GAUGE_ANALOG_RPM:
	    gauge->type	= GAUGE_ANALOG;
	    gauge->min_input	= 0.0;
	    gauge->max_input	= max_rpm / 1000.0;
	    gauge->begin	= -120;
	    gauge->end		= 120;
	    gauge->value	= 0.0;
	    gauge->label	= "rpm X 1000";
	    gauge->min		= 0;
	    gauge->max		= (int) ((max_rpm+999.9)/1000.0);
	    break;

	case GAUGE_DIGITAL_RPM:
	    gauge->type		= GAUGE_DIGITAL;
	    gauge->min_input	= 0.0;
	    gauge->max_input	= max_rpm;
	    gauge->begin	= 0;
	    gauge->end		= (int) max_rpm;
	    gauge->value	= -1;
	    gauge->label	= "RPM";
	    gauge->min		= 0;
	    gauge->max		= (int) max_rpm;
	    break;

	case GAUGE_ANALOG_ALTITUDE:
	    gauge->type	= GAUGE_ANALOG;
	    gauge->min_input	= 0.0;
	    gauge->max_input	= max_altitude/1000.0;
	    gauge->begin	= -120;
	    gauge->end		= 120;
	    gauge->value	= 0.0;
	    gauge->label	= "alt X 1000";
	    gauge->min		= 0;
	    gauge->max		= (int) ((max_altitude+999.9)/1000.0);
	    break;

	case GAUGE_DIGITAL_ALTITUDE:
	    gauge->type		= GAUGE_DIGITAL;
	    gauge->min_input	= 0.0;
	    gauge->max_input	= max_altitude;
	    gauge->begin	= 0;
	    gauge->end		= (int) max_altitude;
	    gauge->value	= -1;
	    gauge->label	= "ALT";
	    gauge->min		= 0;
	    gauge->max		= (int) max_altitude;
	    break;

	case GAUGE_DIGITAL_TURBO:
	    gauge->type		= GAUGE_DIGITAL;
	    gauge->min_input	= 0.0;
	    gauge->max_input	= max_altitude;
	    gauge->begin	= 0;
	    gauge->end		= (int) max_altitude;
	    gauge->value	= -1;
	    gauge->label	= "TURBO";
	    gauge->min		= 0;
	    gauge->max		= (int) max_altitude;
	    break;

	case GAUGE_DIGITAL_SHELLS:
	    gauge->type		= GAUGE_DIGITAL;
	    gauge->min_input	= 0.0;
	    gauge->max_input	= max_altitude;
	    gauge->begin	= 0;
	    gauge->end		= (int) max_altitude;
	    gauge->value	= -1;
	    gauge->label	= "SHELLS";
	    gauge->min		= 0;
	    gauge->max		= (int) max_altitude;
	    break;
    }

    if ((gauge->major = gauge->max / 10.0) == 0)  gauge->major = 1;
    if ((gauge->minor = gauge->major / 2.0) == 0) gauge->minor = 1;

    gauge->x = x;
    gauge->y = y;

    addGauge(gauge);
}



/*****************************************************************
 * initialize_gauges
 * 
 * 	State dependent initialization of client data structures.
 *	       
 */
static void initialize_gauges(
    void)
{
    Driveable *driveable;

    driveable = find_driveable(cstate.car_name);

    /* Set up left_main_gauge. */
    setup_gauge(&left_main_gauge,driveable->left_gauge_class,
	MAINGAUGE_X_LEFT,MAINGAUGE_Y,
	driveable->max_speed,driveable->max_rpm,driveable->max_altitude,0);

    /* Set up right_main_gauge. */
    setup_gauge(&right_main_gauge,driveable->right_gauge_class,
	MAINGAUGE_X_RIGHT,MAINGAUGE_Y,
	driveable->max_speed,driveable->max_rpm,driveable->max_altitude,0);

    /* Set up turbometer. */
    setup_gauge(&turbometer,GAUGE_DIGITAL_TURBO,20,10,
	999.0,999.0,999.0,2);

    /* Set up blastometer. */
    setup_gauge(&blastometer,GAUGE_DIGITAL_SHELLS,760,10,
	999.0,999.0,999.0,2);


    gear.type = GEAR_AUTOMATIC;
    gear.updates = 0;
    gear.max_gears = 5;
    gear.num_gears = 7;
    gear.actual =  2;
    gear.color[0] = gear.color[1] = gear.color[2] = 1.0;
    gear.scolor[0] = 1.0; gear.scolor[1] = gear.scolor[2] = 0.0;
    gear.rev_speed = 10.0;
    switch (gear.type) {
	case GEAR_AUTOMATIC:
	    gear.desired = 2; /* 1st gear */
	    gear.labels[0] = "R";
	    gear.labels[1] = "N";
	    gear.labels[2] = "L1";
	    gear.labels[3] = "L2";
	    gear.labels[4] = "L3";
	    gear.labels[5] = "D";
	    gear.labels[6] = "OD";
	    break;

	case GEAR_STANDARD:
	    gear.desired = 1; /* Neutral */
	    gear.labels[0] = "R";
	    gear.labels[1] = "N";
	    gear.labels[2] = "1";
	    gear.labels[3] = "2";
	    gear.labels[4] = "3";
	    gear.labels[5] = "4";
	    gear.labels[6] = "5";
	    break;

    }
    addGear(&gear);
}


/*****************************************************************
 * read_command_line
 * 
 * 	Process command line arguments.
 */
static void read_command_line(
    int argc,
    char **argv)
{
    int watch_arg = 0;
    int autowatch_arg = 0;
    int infinite=0,got_record = 0,got_playback = 0;
    char *recordfile,*playbackfile;

    /* Read hostname from command line. */
    if (!scanargs( argc, argv, "%  f%- w%- W%- D%- I%- R%-recordfile!s P%-playbackfile!s hostname%s",
	       &showfps, &watch_arg, &autowatch_arg, &debug, &infinite, 
	       &got_record, &recordfile,
	       &got_playback, &playbackfile,
	       &hostname)) {
	exit( -1 );
    }
    if (infinite) cstate.mode = CLIENT_INFINITE_MODE;

    /* Set up state based on command line arguments. */
    if (autowatch_arg) {
	cstate.mode |= CLIENT_AUTOWATCH_MODE;
    }
    else if (watch_arg) {
	cstate.mode |= CLIENT_WATCH_MODE;
    }

    if (got_record && got_playback) {
	fprintf(stderr,"Cannot record and playback simultaneously.\n");
	exit(1);
    }

#ifndef WIN32
    if (got_record) {
	if ((cstate.record_fildes
		= open(recordfile,O_WRONLY|O_CREAT,0664)) == -1) {
	    fprintf(stderr,"Cannot open record file: %s\n",recordfile);
	    exit(-1);
	}
    }
    else if (got_playback) {
	if ((cstate.playback_fildes
		= open(playbackfile,O_RDONLY)) == -1) {
	    fprintf(stderr,"Cannot open playback file: %s\n",playbackfile);
	    exit(-1);
	}
    }
#endif
}



/*****************************************************************
 * read_environment
 * 
 * 	Initialize client state from environment.
 */
static void read_environment(
    void)
{
    putenv( "HW_DISABLE_CULL=1" );
    putenv( "SB_710_VM_DB=TRUE" );
    /* putenv( "SB_X_SHARED_CMAP=TRUE" ); */

    if (getenv("DRIVE_WATCH_MODE") != NULL)
	cstate.mode |= CLIENT_WATCH_MODE;

    if (getenv("DRIVE_AUTOWATCH_MODE") != NULL)
	cstate.mode |= CLIENT_AUTOWATCH_MODE;

    if (getenv("DRIVE_SMALL_WINDOW") != NULL)
	cstate.mode |= CLIENT_SMALLWINDOW_MODE;

    if (getenv("DRIVE_AUTOSTART_MODE") != NULL)
	cstate.mode |= CLIENT_AUTOSTART_MODE;

    /* Same thing the "Vehicle:" line in .driverc does, without needing one. */
    if (getenv("DRIVE_VEHICLE") != NULL)
	cstate.car_name = getenv("DRIVE_VEHICLE");
}



/*****************************************************************
 * set_defaults
 * 
 * 	Set default client values.
 *	       
 */
static void set_defaults(
    void)
{
    /* Initial client state. */
    cstate.mode = CLIENT_NORMAL_MODE;
    cstate.frames = 0;
    cstate.watch_frames = CLIENT_AUTOWATCH_FRAMES;

    /* Initial client camera state. */
    cstate.camera.mode = CAM_MODE_NORMAL;
    cstate.camera.skycam_up_angle = 15;
    cstate.camera.skycam_around_angle = 270;
    cstate.camera.skycam_distance = 30.0;
    cstate.camera.skycam_dirty = TRUE;
    cstate.camera.direction = CAM_DIRECTION_FRONT;

    cstate.start_pos.scene_x = POSITION_INVALID;
    cstate.start_pos.scene_z = POSITION_INVALID;
    cstate.start_pos.angle = POSITION_INVALID;  
    cstate.start_pos.pos[0] = POSITION_INVALID;
    cstate.start_pos.pos[1] = POSITION_INVALID;
    cstate.start_pos.pos[2] = POSITION_INVALID;
#ifndef WIN32
    {  struct passwd *p = getpwuid(geteuid());
        if (p)
            strncpy(cstate.nickname, p->pw_name, sizeof(cstate.nickname) - 1);
        cstate.nickname[sizeof(cstate.nickname) - 1] = '\0';
    }
#else
    cuserid( &(cstate.nickname[0]) );
#endif
    cstate.nickname[9] = '\0';

    /* Defaults for time-based background color. */
    cstate.daylight.use_server_time = FALSE;
    cstate.daylight.static_time = FALSE;
    cstate.daylight.mask = 0x3;
    cstate.daylight.back_clr[0] = SKY_RED;
    cstate.daylight.back_clr[1] = SKY_GREEN;
    cstate.daylight.back_clr[2] = SKY_BLUE;
}


/*****************************************************************
 * client_is_ready
 * 
 *     Tell the server that the client is ready to receive frames,
 *     as well as what sort of car to give us.
 * 	
 * Inputs:
 *	cstate.car_name  -- global name of car as selected from selection box.
 * Outputs:
 *		None
 */
static void client_is_ready(
    void)
{
    struct CliReady 
	Msg;

    Driveable *driveable;

    /* Which driveable is it? */
    driveable = find_driveable(cstate.car_name);
    cstate.upd.invis[1] = driveable->object_number;

    memcpy(cstate.upd.name, cstate.nickname,10);
    cstate.upd.name[9] = '\0';

    /* Now, let the server know that we are ready to receive frames */
    Msg.MsgType = CLI_READY;
    Msg.MsgLength = CLI_READY_SIZE;
    memcpy(&Msg.upd, &cstate.upd, sizeof(UpdateDisp));
    if( ! PutMsg( (struct IPCMsg *)&Msg, server_socket->socketnum) ) {
	perror("PutMsg in select_new_vehicle");
    }
    FlushMsg(server_socket->socketnum);

    /* Now, just in case the title window is still mapped, unmap it */
    unmapTitleWindow();

#ifndef WIN32
    /* Re-enable interrupts */
    signal(SIGINT, SIG_DFL);
#endif
}



/*****************************************************************
 * select_new_vehicle
 * 
 *     Tell the server that the client wants a new vehicle type.
 * 	
 * Outputs:
 *		None
 */
void select_new_vehicle(
    char *new_vehicle)
{
    struct CliReady Msg;
    Driveable *driveable;


    /* Which driveable is it? */
    driveable = find_driveable(new_vehicle);
    cstate.car_name = driveable->name;
    cstate.upd.invis[1] = driveable->object_number;

    /* Now, let the server know that we are ready to receive frames */

    Msg.MsgType = CLI_READY;
    Msg.MsgLength = CLI_READY_SIZE;
    memcpy(&Msg.upd, &cstate.upd, sizeof(UpdateDisp));
    if( ! PutMsg( (struct IPCMsg *)&Msg, server_socket->socketnum) ) {
	perror("PutMsg in select_new_vehicle");
    }
    FlushMsg(server_socket->socketnum);

    /* Redo the gauges */
    removeGauge(&left_main_gauge);
    removeGauge(&right_main_gauge);
    removeGauge(&turbometer);
    removeGauge(&blastometer);
    removeGear(&gear);
    initialize_gauges();
    redrawDash();
    refresh_gauges();
}



static int check_server_socket( int doSleep )
{
    fd_set server_mask;
    struct timeval timeout;
    int done = 0;

    /* Set up mask to watch server socket. */
    FD_ZERO(&server_mask );
    FD_SET(server_socket->socketnum, &server_mask);

    /* Block until we have input on the socket. */
    timeout.tv_sec = 0;
    timeout.tv_usec = doSleep ? 10000 : 0;
    if (select(FD_SETSIZE, &server_mask,
	    NULL, NULL, &timeout) < 0) {
	if (errno == EINTR) return 0;	/* signal, not an error */
	perror( "Client select" );
	exit( -1 );
    }

    /* Check socket for pending messages. */
    if (FD_ISSET(server_socket->socketnum, &server_mask)) {
	done = client_read();
    }

    return(done);
}



static void run_graphics_timings(
    void)
{
#if !(defined(WIN32) || defined(MAC))
    struct timeval start,stop;
    struct timezone tz;
    int which_dl,i;
    float straight_frametime,invis_frametime;
    Driveable *driveable;

    driveable = find_driveable(DEFAULT_VEHICLE);
    if ((which_dl = driveable->display_list) == INVALID) return;

    /* Prime the pump */
    redraw_spinning_vehicle(which_dl,FALSE);

    /* Measure vehicle redraw time */
    gettimeofday(&start,&tz);
    for (i=0; i<TIMING_ITERATIONS; ++i) {
	redraw_spinning_vehicle(which_dl,FALSE);
    }
    gettimeofday(&stop,&tz);
    straight_frametime = ((float) (stop.tv_sec - start.tv_sec)
	    + (float) (stop.tv_usec - start.tv_usec) / 1000000.0)
	/ TIMING_ITERATIONS;

    /* Try it with invisibility on */
    gettimeofday(&start,&tz);
    for (i=0; i<TIMING_ITERATIONS; ++i) {
	redraw_spinning_vehicle(which_dl,TRUE);
    }
    gettimeofday(&stop,&tz);
    invis_frametime = ((float) (stop.tv_sec - start.tv_sec)
	    + (float) (stop.tv_usec - start.tv_usec) / 1000000.0)
	/ TIMING_ITERATIONS;

    /* If it's too slow, default to a small window */
    if (straight_frametime > MAXIMUM_LARGEWINDOW_FRAMETIME) {
#ifdef DISABLE_AUTO_RESIZE
#else
	/* Make the window small */
	smallWindowCallback();
#endif
    }

    /* If invisibility slows it down much, disable it */
    cstate.disable_transparency = 
	(invis_frametime/straight_frametime > MINIMUM_INVIS_FRAMETIME_RATIO);
#endif
}

#if defined(WIN32) || defined(MAC)
static int car_num = 0;
static void vehicleChooseCB( hwDrawable draw, hwWinEvent *event )
{
#ifdef GLFW
    /* Clicking a name in the list picks that vehicle; START begins. */
    if (event->type == HW_INPUT_BUTTON_PRESS) {
        int hit = dashPickerHit( event->pointer.x, event->pointer.y );

        if (hit == -2) {
            cstate.car_selected = 1;
        }
        else if (hit >= 0 && hit < num_cars) {
            car_num = hit;
            cstate.car_name = driveables[hit].name;
        }
        return;
    }
#endif
    if (event->type != HW_INPUT_KEYBOARD) {
        return;
    }

    switch (event->keyboard.key) {
    case ' ' :
        car_num = car_num + 1;
        if(car_num >= num_cars) {
            car_num = num_cars - 1;
        }
        cstate.car_name = driveables[car_num].name;
        break;
    case '\b' :
        car_num = car_num - 1;
        if(car_num < 0) {
            car_num = 0;
        }
        cstate.car_name = driveables[car_num].name;
        break;
        break;
    case '\r' : case '\n' :
        cstate.car_selected = 1;
        break;
    case '\033' :
        exit(1);
        break;
    case 'r' :
        cstate.upd.color[0] += 0.05;
        if (cstate.upd.color[0] > 1.0) {
            cstate.upd.color[0] = 1.0;
        }
        break;
    case 'g' :
        cstate.upd.color[1] += 0.05;
        if (cstate.upd.color[1] > 1.0) {
            cstate.upd.color[1] = 1.0;
        }
        break;
    case 'b' :
        cstate.upd.color[2] += 0.05;
        if (cstate.upd.color[2] > 1.0) {
            cstate.upd.color[2] = 1.0;
        }
        break;
    case 'R' :
        cstate.upd.color[0] -= 0.05;
        if (cstate.upd.color[0] < 0.0) {
            cstate.upd.color[0] = 0.0;
        }
        break;
    case 'G' :
        cstate.upd.color[1] -= 0.05;
        if (cstate.upd.color[1] < 0.0) {
            cstate.upd.color[1] = 0.0;
        }
        break;
    case 'B' :
        cstate.upd.color[2] -= 0.05;
        if (cstate.upd.color[2] < 0.0) {
            cstate.upd.color[2] = 0.0;
        }
        break;
    }
}
#endif

static void select_vehicle(
    void)
{
    int which_dl=0;
    int refreshes = 0;
    static struct timeval
	start_session_time,
	stop_session_time;	/* for debug */
#ifndef WIN32
    struct timezone tz;
#endif
    float fseconds;
    int seconds, usec;
    Driveable *driveable;

    /* Now, just in case the title window is still mapped, unmap it */
    unmapTitleWindow();
    processXEvents();

    run_graphics_timings();

#ifndef WIN32
    gettimeofday( &start_session_time, &tz );
#endif

#if defined(WIN32) || defined(MAC)
    disp->inputHandler( disp, vehicleChooseCB );
#endif

    while (!cstate.car_selected) {
	if (check_server_socket( 1 )) return;

#if !(defined(WIN32) || defined(MAC))
	processXEvents();
#endif

	/* Decide which display list to show off */
	driveable = find_driveable(cstate.car_name);
	which_dl = driveable->display_list;

	if (which_dl != INVALID) {
	    redraw_spinning_vehicle(which_dl,FALSE);
	}
	refreshes++;
	if(debug)
	{
	    if( refreshes == 10)
	    {
		refreshes = 0;
#ifndef WIN32
		gettimeofday( &stop_session_time, &tz );
#endif
		if (start_session_time.tv_usec > stop_session_time.tv_usec) {
		    stop_session_time.tv_usec += 1000000;
		    stop_session_time.tv_sec--;
		}
		seconds = stop_session_time.tv_sec - start_session_time.tv_sec;
		usec = stop_session_time.tv_usec - start_session_time.tv_usec;

		fseconds = seconds + (float)usec / 1000000;
		fprintf(stderr,"rate: %f\r", 10.0 / fseconds  );

#ifndef WIN32
		gettimeofday( &start_session_time, &tz );
#endif
	    }
	}
    }

#if defined(WIN32) || defined(MAC)
    disp->inputHandler( disp, winCallback );
#endif
}


static void validate_color(
    float *r, float *g, float *b)
{
    float hue,saturation,brightness;

    /* Make sure it's minimally visible */
    rgb_to_hsv(*r,*g,*b, &hue,&saturation,&brightness);
    if (brightness < 0.5) brightness = 0.5;
    hsv_to_rgb(hue,saturation,brightness, r,g,b);
}


static void drive_initialize(
    int argc,
    char *argv[])
{
#ifndef WIN32
    struct timezone tz;
#endif
    int i;


    seedrand(getpid() ^ time(0) ^ 0x12345678);

    cstate.pex_version = FALSE;

    img_fildes  = INVALID;
    connection_id = INVALID;
    num_cars = 0;
    hostname = NULL;
#if !(defined(WIN32) || defined(MAC))
    cstate.display = NULL;
#endif
    cstate.twisted_elk = FALSE;
    cstate.cull_multiplier = 1.0;
    gear.type = GEAR_AUTOMATIC;
    cstate.record_fildes = cstate.playback_fildes = INVALID;
    cstate.racestart.tv_sec = cstate.racestart.tv_usec = 0;
    cstate.throttle = 0.0;
    cstate.desired_throttle = 0.0;
    cstate.gunsight_fov = CAM_ALT_FOV;
    cstate.use_joystick = 0;
    cstate.use_sound = 0;
    cstate.sound_config[0] = 0;

    /* Override DISPLAY with GDISPLAY if set.
     * This allows us to use a visual debugger on one display
     * while running the program on another.
     */
    {   char *c;
	if ((c = getenv("GDISPLAY")) != NULL) {
	    char str[256];
	    printf("GDISPLAY=%s\n",c);
	    sprintf(str,"DISPLAY=%s",c);
	    putenv(str);
	}
    }

#if 0
    {
	Display *tmpDisplay;
	char *displayString;
	/* Open the X11 "display" and get the default screen.  If the "display"
	 * name wasn't passed as a program argument, then it will be obtained
	 * from the DISPLAY environment variable.
	 */
	displayString = getenv("DISPLAY");
	if ((tmpDisplay = XOpenDisplay(displayString)) == NULL) {
	    /* Could not open display: */
	    if (displayString == NULL) {
		fprintf(stderr, "Could not open display!!!\n");
	    }
	    else {
		fprintf(stderr, "Could not open display: %s!!!\n", displayString);
	    }
	    exit(1);
	}

	/* TBD: Dunno how to do this in HoverWare... */
	/* Create and gopen a starbase window to get an
	 * idea of what sort of device we are running here.
	 */
	if (check_sb_device(tmpDisplay)) {
	    cstate.mode |= CLIENT_SMALLWINDOW_MODE;
	}
    }
#endif

    /* Set *default* values for client data structures. */
    set_defaults();

    /* Read the user's .driverc file. */
    read_rcfile();

    /* Read environment variables. */
    read_environment();
#ifdef GLFW
    driveFogInit();
#endif

    /* Read command line options. */
    read_command_line(argc, argv);

    /* set the appropriate skycam values, so Watch Mode with Overhead Camera
     * will start out correctly
     */
    check_skycam_dirty();

    /* Build the filenames I need to fetch.  Base them off argv[0] */
    construct_filenames(argv[0]);

    /* Get our own server going while the window comes up. */
    start_private_server();

    /* Must come after construct_filenames */
    check_graphics_configuration();

    /* Skip all the start up in watch mode. */
    if ((cstate.mode & CLIENT_WATCH_MODE)
	    || (cstate.mode & CLIENT_AUTOWATCH_MODE) ) {
	cstate.mode |= CLIENT_AUTOSTART_MODE;
    }

    if( cstate.use_joystick )
    {
	init_joystick_device("/dev/gameport");
    }
    if( cstate.use_sound )
    {
	if( cstate.sound_config[0] == 0 ) {
	    sprintf(cstate.sound_config,"%s%s",drivedir,"/sound.cnf");
	    init_sound(cstate.sound_config);
	}
	else {
	    init_sound(cstate.sound_config);
	}
    }

    client_message_initialize();

#if defined(MOTIF_GUI)
    img_fildes = createGUI(argc,argv,
	(cstate.mode & CLIENT_SMALLWINDOW_MODE) != 0,
	resize_sb_window,
	StarbaseGetVisuals);
#else
    img_fildes = createGUI(argc,argv,
	(cstate.mode & CLIENT_SMALLWINDOW_MODE) != 0,
	NULL,
	NULL);
#endif

    /*mapTitleWindow();*/

    /* If colorname was specified, fetch it, now that X is open */
    if ((cstate.colorname != NULL) && (*(cstate.colorname) != '\0')) {
	colornameToRGB(cstate.colorname, &(cstate.upd.color[0]),
	    &(cstate.upd.color[1]), &(cstate.upd.color[2]));
    }

    /* Make sure the color chosen was bright enough */
    validate_color(&(cstate.upd.color[0]),
	&(cstate.upd.color[1]), &(cstate.upd.color[2]));

    setup_img_fildes();

    /* Initialize headlights. */
    headlight_initialize( &cstate.headlight );
    daylight_initialize( &cstate.daylight );

    cstate.state = WELCOME_STATE;
    updateStateWindow(cstate.state);

    if (!PrepareForIPC()) {
        printf("Unable to prepare for IPC, exiting\n");
        exit(1);
    }

    do {
	if ((hostname == NULL) || (*hostname == '\0')) hostname = findServer();

	server_socket = (socket_type *)malloc(sizeof(socket_type)); 

	server_socket->socketnum = InitIPC(hostname,"drive");
	if(server_socket->socketnum == IPC_INVALID_SOCK ) {
	    server_socket->socketnum = InitIPC(hostname, DEFAULT_SOCKET_STRING);
	}
	if(server_socket->socketnum == IPC_INVALID_SOCK ) {
            printf("Unable to connect to %s; retrying in 10 seconds\n", hostname);
#ifdef WIN32
            Sleep( 10 * 1000 );
#else
            (void)sleep(10);
#endif
        }
    } while (server_socket->socketnum == IPC_INVALID_SOCK);

    /* Wait until we've gotten all the vehicles. */
    while (connection_id == INVALID) {
	if (check_server_socket(0)) return;
    }

    /* Let the GUI know about all the vehicle types */
    updateGUIVehicles(driveables,num_cars);
   
    /* Double-check the vehicle name we got from the startup file */
    if (cstate.car_name != NULL) {
	/* Got a vehicle from the rcfile.  Make sure it's good. */
	for (i=0; i< num_cars; i++) {
	    if (strcmp(driveables[i].name,cstate.car_name) == 0) {
		break;
	    }
	}
	if (i == num_cars) {
	    /* bad name */
	    cstate.car_name = NULL;
	}
    }

    if (!(cstate.mode & CLIENT_AUTOSTART_MODE)
		|| (cstate.car_name == NULL)) {
	if (cstate.car_name == NULL) cstate.car_name = DEFAULT_VEHICLE;
	mapVehicleSelectionWindow();
	select_vehicle();
    }
    else if (cstate.car_name == NULL) {
	cstate.car_name = DEFAULT_VEHICLE;
    }
    cstate.car_selected = TRUE;

    initialize_gauges();
    refresh_gauges();

#if !(defined(WIN32) || defined(MAC))
    /* Center the pointer if the thing is confined */
    if (cstate.mode
	    & (CLIENT_CONFINE_CURSOR_MODE|CLIENT_CONSTRAIN_CURSOR_MODE)) {
	XWarpPointer(cstate.display,None,cstate.graphicsWindow,0,0,0,0,400,40);
    }
#endif

    /* Tell the server that we're here. */
    client_register();

#if !(defined(WIN32) || defined(MAC))
    /* Save off the Pointer values, since we are going to blast them */
    XGetPointerControl(cstate.display,&save_XPointer_numerator,
	&save_XPointer_denominator,&save_XPointer_threshold);
 
#ifdef SET_POINTER
    /* Set the mouse acceleration to be as low as possible normal */
    XChangePointerControl(cstate.display,1,1,1,1,0);
#endif /* SET_POINTER */

    if (debug) {
	/* Initialize frame rate counter. */
	gettimeofday( &start_session_time, &tz );
    }

    /*  Ignore signal SIG_INT until we have all our messages from the server*/
    signal( SIGINT, SIG_IGN );
#endif

    /* Confine the cursor if need be */
    if (cstate.mode
	    & (CLIENT_CONFINE_CURSOR_MODE|CLIENT_CONSTRAIN_CURSOR_MODE)) {
       grabCursor();
    }

    /* Initialize automatic camera. */
    if (cstate.camera.mode == CAM_MODE_AUTOCAM) {
	cstate.camera.skycam_up_angle = 10;
	cstate.camera.skycam_distance = 45.0;
	cstate.camera.skycam_dirty = TRUE;
    }

#if !(defined(WIN32) || defined(MAC))
    XFlush(cstate.display);
#endif
}


/*****************************************************************
 *****************************************************************
 * 
 *                   DRIVE CLIENT main
 * 
 *****************************************************************
 *****************************************************************/
#ifdef GLFW
/* Set by the Vehicle menu; acted on from the main loop, because the picker
 * runs its own modal loop and must not start inside an event callback.
 */
static boolean_type vehicle_select_pending = FALSE;

void request_vehicle_select(void)
{
    vehicle_select_pending = TRUE;
}

static void do_vehicle_select(void)
{
    vehicle_select_pending = FALSE;

    if (cstate.car_name == NULL) cstate.car_name = DEFAULT_VEHICLE;
    cstate.car_selected = FALSE;
    dashSetPicker(1);
    mapVehicleSelectionWindow();
    select_vehicle();
    dashSetPicker(0);
    cstate.car_selected = TRUE;

    /* Tell the server, and rebuild the gauges for the new vehicle. */
    select_new_vehicle(cstate.car_name);
    initialize_gauges();
    refresh_gauges();
}
#endif

int main(
    int argc,
    char *argv[])
{
    boolean_type done = FALSE;
#ifndef WIN32
    struct timezone tz;
#endif

    drive_initialize(argc,argv);
    client_is_ready();

    /* Main client select loop. */
    do {
#ifdef GLFW
	if (vehicle_select_pending) do_vehicle_select();
#endif
	done = check_server_socket( 1 );

#if 0
	/* If we have received a whole frame then display it. */
	if (cstate.frame_ready) {
	    client_update();
	}
#endif
    } while (!done);

    if( cstate.use_joystick && (cstate.joystick != NULL) ){
	/* Tell the joystick daemon that we are quitting */
	cstate.joystick->Valid = -1;
    }


    if (debug) {
	int seconds, usec;

#ifndef WIN32
	/* Print out session frame rate. */
	gettimeofday( &stop_session_time, &tz );
#endif

	if (start_session_time.tv_usec > stop_session_time.tv_usec) {
	    stop_session_time.tv_usec += 1000000;
	    stop_session_time.tv_sec--;
	}
	seconds = stop_session_time.tv_sec - start_session_time.tv_sec;
	usec = stop_session_time.tv_usec - start_session_time.tv_usec;

	printf("\n\nSession took %d seconds, %d micro seconds.\n",
	       seconds, usec );
	printf("Displayed %d frames\n", cstate.frames );
	printf("Session frame rate was %f frames per second.\n",
	       (float)cstate.frames / (float)seconds );
    }

    close( server_socket->socketnum );
    fprintf( stderr, "Client terminating.\n" );
    return(0);
}
