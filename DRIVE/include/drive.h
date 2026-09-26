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
 * drive.h - Client definitions.
 *
 */

#ifndef _DRIVE_INCLUDED
#define _DRIVE_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <time.h>

#ifndef WIN32
#include <sys/time.h>

#ifndef MAC
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <Xm/Xm.h>
#endif

#endif

#include "camera.h"
#include "daylight.h"
#include "headlight.h"
#include "sockets.h"
#if 0 /*ndef USE_STDARG*/
# include <varargs.h>
#else
# include <stdarg.h>
#endif
#include "gauge.h"
#include "message.h"
#include "newMessage.h"
#include "joystick.h"
#include "sound.h"

#ifndef CONST_DECL
#define CONST_DECL
#endif

/******************************** MACROS **************************************/
/* Client mode definitions. */
#define CLIENT_NORMAL_MODE		(0)
#define CLIENT_WATCH_MODE		(1<<0)
#define CLIENT_AUTOWATCH_MODE		(1<<1)
#define CLIENT_SMALLWINDOW_MODE		(1<<2)
#define CLIENT_AUTOSTART_MODE		(1<<3)

#define CLIENT_KEYACC_MODE		(1<<4)
#define CLIENT_USE_KEYACCEL		(1<<5)
#define CLIENT_CRUISE_CONTROL_MODE	(1<<6)
#define CLIENT_BRAKES_ON		(1<<7)
#define CLIENT_TURBO_MODE		(1<<8)
#define CLIENT_ROBUST_MODE		(1<<9)
#define CLIENT_INFINITE_MODE		(1<<10)
#define CLIENT_CONFINE_CURSOR_MODE	(1<<11)
#define CLIENT_CONSTRAIN_CURSOR_MODE	(1<<12)
#define CLIENT_SHOW_NAMES_MODE		(1<<13)
#define CLIENT_INITIAL_RADAR		(1<<14)
#define CLIENT_INITIAL_STANDINGS	(1<<15)
#define CLIENT_DUAL_RADAR_MODE		(1<<16)
#define CLIENT_HUD_MODE			(1<<17)

/* Number of frames between switching in autowatch mode. */
#define CLIENT_AUTOWATCH_FRAMES	(100)

/* Camera params */
#define CAM_FOV		(40.0)
#define CAM_ALT_FOV	(12.0)

/* Gunsight params */
#define SIGHT_MARGIN		10
#define SIGHT_ELEVATION_MARKS	5

/* For keyboard control of altitude */
#define ACC_MAX		1.0
#define BRAKE_MAX	(-1.0)
#define ACC_INC		0.025
#define BRAKE_INC	0.1

/* Original window size: WIN_LARGEWIDTH x WIN_LARGEHEIGHT (libgui/windows.h)
 * = (800 dash + 5 + 25 acc/brake bar + 5) x (500 view + 300 dash + 140 fixed)
 */
#define DRIVE_WIN_WIDTH		835
#define DRIVE_WIN_HEIGHT	940

#define DEFAULT_VEHICLE	"Sports Car"

#define VENDOR_UNKNOWN		(1<<0)
#define VENDOR_HP		(1<<1)
#define VENDOR_IBM		(1<<2)
#define VENDOR_SUN		(1<<3)
#define VENDOR_OKI		(1<<4)
#define VENDOR_NCD		(1<<5)
#define VENDOR_DEC		(1<<6)
#define VENDOR_TEKTRONIX	(1<<7)
#define VENDOR_SHOGRAPHICS	(1<<8)
#define LAST_VENDOR		VENDOR_SHOGRAPHICS

#define ALL_VENDORS		((LAST_VENDOR<<1)-1)
#define VENDORS_WITH_BOGUS_DEPTH_CUEING	(ALL_VENDORS^VENDOR_HP)

#define THROTTLE_DELTA  (0.01)

/************************* STRUCTURE DEFINITIONS ******************************/
/* Client state structure:
 * The rest of the code communicates to the GUI code through this structure
 * and calls to the gauge entrypoints, as well as the big routine that
 * sets up the GUI.
 */
typedef struct {
    boolean_type pex_version;	/* Running pex? */
    unsigned int mode;		/* Client mode. */
    camera_type camera;		/* Client camera. */
    headlight_type headlight;	/* Headlight structure. */
    daylight_type daylight;	/* Time of day parameters. */
    unsigned int frames;	/* Number of frames for this session. */
    int watch_frames;		/* Seconds to watch each connection. */

    int state;			/* Startup, running, etc. */
    char *car_name;		/* Which car selected? */
    char *colorname;
    boolean_type car_selected;
    UpdateDisp upd;
    int acc_value;		/* for cruise control */
    float throttle;		/* for vehicles whose accel is not the mouse*/
    float desired_throttle;	/* Where I would like the throttle to be. */
    checkpoint_msg_type checkpoint;
    float lap_time;
    drive_start_type start_pos;
    char nickname[100];
    unsigned int global_nameset_bits;
    float rpm;			/* rpm for autoshift */
    float mph;			/* mph for sound effects */

    boolean_type frame_ready;
#if !(defined(WIN32) || defined(MAC))
    Display *display;
    Window graphicsWindow;
    Drawable renderBuf;
#else
    int mouseX, mouseY, mouseMods;
#endif
    int gWinWidth,gWinHeight;
    int whichBuffer;
    unsigned int vendor;

    /* Graphics information */
    boolean_type twisted_elk;	/* Are we on an elk? */
    boolean_type disable_transparency; /* Too slow to use */

    /*** Controls and displays ***/
    float compass_angle;	/* radians, N=0 */
    float accBrk_value;		/* -1.0 to 1.0 */
    float xpointer_value;	/* -1.0 to 1.0 */
    float view_direction[3];	/* which direction are we looking? */

    float cull_multiplier;	/* detail level controls for PEX version */

    /* Record/playback data */
    int record_fildes;
    int playback_fildes;
    struct timeval racestart;

    /* for gun sight zooming */
    float gunsight_fov;
    int   use_joystick;		/* Use joystick or mouse for input? */
    struct Joystick *joystick;	/* structure for shmem joystick info */
    int   use_sound;		/* Should we make some noise? */
    struct Sound     *sound;    /* structure for shmem sound info */
    char   sound_config[1024];	/* filename of sound configuration file */
} drive_state_type;


/************************* PROGRAM GLOBALS ************************************/
extern drive_state_type cstate;
extern socket_type *server_socket;
extern int
    save_XPointer_numerator,
    save_XPointer_denominator,
    save_XPointer_threshold;
extern int img_fildes;
extern gear_type gear;
extern gauge_type left_main_gauge,right_main_gauge,turbometer, blastometer;
extern compass_type compass;
extern radar_type radar;
extern int connection_id;
extern int num_cars;
extern Driveable driveables[];
extern char *hostname;


/********************* FUNCTION PROTOTYPES ************************************/
/*** From drive.c ***/
extern void process_keypress(
#if defined(WIN32) || defined(MAC)
    int key, int mod
#else
    KeySym key, unsigned int keyState
#endif
    );
extern void process_button(
    unsigned int button);
extern void update_gauge_from_message(
    gauge_type *gauge,
    struct SrvFrameGauge *gauge_msg);
extern void select_new_vehicle(
    char *new_vehicle);


/*** From camera.c ***/
extern float driveFogAmount;
extern void driveFogInit(
    void);


/*** From hwdash.c (GLFW builds) ***/
extern void drawDashboard(
    void);
extern void request_vehicle_select(
    void);
extern int dashMenuEvent(
    void *event);
extern int dashPickerActive(
    void);
extern void drawVehiclePicker(
    void);
extern int dashPickerHit(
    int mx, int my);
extern void dashSetPicker(
    int on);
extern void dashBeginOverlay(
    void);
extern void dashEndOverlay(
    void);
extern void dashResize(
    int w, int h);
extern void toggleHelpPanel(
    void);


/*** From scanargs.c ***/
/* Must always be a variadic prototype: on ABIs such as arm64 an
 * unprototyped call passes arguments in registers instead of on the
 * varargs stack, which corrupts the argument list. */
extern int scanargs ( int argc, char **argv, CONST_DECL char *format, ... );


/*** From explosion.c ***/
extern void explosion_create(
    float x, float y, float z,
    float r, float g, float b,
    float radius);
extern void explosion_update(
    int fildes);


/*** From dlist.c ***/
extern void receive_segment(
    int fildes,
    struct SrvDefine *Msg,
    boolean_type disable_transparency);
extern void createHwSegmentFromMsg(
    struct IPCMsg *msg);


/*** From frame.c ***/
extern void frame_end( struct IPCMsg *Msg);
extern void frame_position(struct IPCMsg *Msg);
extern void frame_gauge(struct IPCMsg *Msg);
extern void frame_matrix(struct IPCMsg *Msg);
extern void frame_static_seg(struct IPCMsg *Msg);
extern void frame_update(struct IPCMsg *Msg);
extern void frame_global_nameset(struct IPCMsg *Msg);
extern void frame_radar(struct IPCMsg *Msg);


/*** From parse_rcfile.c ***/
extern void read_rcfile(
    void);


/*** From camera.c ***/
extern void point_xform(
    float *x, float *y, float *z,
    float xform[4][4]);


/*** From sbwin.c ***/
extern char *sb_make_window(
#if !(defined(WIN32) || defined(MAC))
    Display     *display,
#endif
    char	*geometryString,
    int		backingStoreFlag,
    char	*title,
    int		cmapHint,
    int		desiredDepth, int flexibility,
    int		argc,
#if defined(WIN32) || defined(MAC)
    char	*argv[]);
#else
    char	*argv[],
    Window	*window);
#endif


/*** From clook.c ***/
extern char *lookup_rgb(
    float r, float g, float b);
extern void rgb_to_hsv(
    float r, float g, float b,
    float *h, float *s, float *v);
extern void hsv_to_rgb(
    float h, float s, float v,
    float *r, float *g, float *b);


/*** From drive_sb.c ***/
extern void setup_img_fildes(
    void);
extern void auto_rotate_vehicle(
    void);
extern void client_update(
    void);
extern int check_sb_device(
#if !(defined(WIN32) || defined(MAC))
    Display *display
#endif
    );
extern void redraw_spinning_vehicle(
    int which_dl,
    boolean_type draw_half_invisible);
extern void resize_sb_window(
    int width, int height);


/*** From drive_msg.c ***/
extern boolean_type client_read(
    void);
extern void client_message_initialize(
    void);
extern void client_register(
    void);
extern void client_quit(
    void);
extern void client_restart(
    void);
extern void client_upright(
    void);


/*** From query.c ***/
extern void find_active_servers(
    char servername[256][256],
    int *num_servers);

/*** From sound.c ****/
extern void update_rpm( float);


#endif /* _DRIVE_INCLUDED */
