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
 * drive_server.h - Server definitions.
 *
 */

#ifndef _DRIVE_SERVER_INCLUDED
#define _DRIVE_SERVER_INCLUDED

#include <time.h>

#include "connection.h"
#include "message.h"
#include "scene.h"
#include "newMessage.h"

/******************************* MACROS ***************************************/
#define SERVER_TSLICE		40000
#define T_INTERVAL		((float) SERVER_TSLICE/1000000.0)
#define LOOP_PER_SECOND		(1000000/SERVER_TSLICE)
#define LOOP_PER_MINUTE		(60*LOOP_PER_SECOND)
#define MAX_START_LOCATIONS	100
#define CLOCK_FROZEN		(-999999)

#define TURBO_NONE		0
#define TURBO_UNLIMITED		1
#define TURBO_LIMITED		2

#define GUNS_NONE		0
#define GUNS_UNLIMITED		1
#define GUNS_LIMITED		2

/* bits for server_mode bitfield */
#define SERVER_MODE_NORMAL		(1<<0)
#define SERVER_MODE_RACE		(1<<1)
#define SERVER_MODE_DEMO		(1<<2)
#define SERVER_MODE_DEMO_INTERRUPT	(1<<3)

#define SERVER_MODE_NIGHT_DRIVING	(1<<4)

#define SERVER_MODE_WRECK_NO_ACTION	(1<<5)
#define SERVER_MODE_WRECK_UPRIGHT	(1<<6)
#define SERVER_MODE_WRECK_RESTART	(1<<7)
#define SERVER_WRECK_MODES \
    (SERVER_MODE_WRECK_NO_ACTION \
    | SERVER_MODE_WRECK_UPRIGHT \
    | SERVER_MODE_WRECK_RESTART)

#define SERVER_MODE_EXPLOSION_DISABLED	(1<<8)
#define SERVER_MODE_EXPLOSION_VISUAL	(1<<9)
#define SERVER_MODE_EXPLOSION_FORCE	(1<<10)
#define SERVER_MODE_EXPLOSION_RESTART	(1<<11)
#define SERVER_EXPLOSION_MODES \
    (SERVER_MODE_EXPLOSION_DISABLED \
    | SERVER_MODE_EXPLOSION_VISUAL \
    | SERVER_MODE_EXPLOSION_FORCE \
    | SERVER_MODE_EXPLOSION_RESTART)

#define SERVER_MODE_SIMULTANEOUS_START	(1<<12)
#define SERVER_MODE_STARTLINE_START	(1<<13)
#define SERVER_START_MODES \
    (SERVER_MODE_SIMULTANEOUS_START | SERVER_MODE_STARTLINE_START)\

#define SERVER_MODE_TIMEOUT_FINISH	(1<<14)
#define SERVER_MODE_FINISHLINE_FINISH	(1<<15)
/* Could also be "either", in which case both would be on */
#define SERVER_FINISH_MODES \
    (SERVER_MODE_TIMEOUT_FINISH	| SERVER_MODE_FINISHLINE_FINISH)

/* Whether or not to allow spacecraft to race */
#define SERVER_MODE_ALLOW_SPACE_RACES	(1<<16)

/* Whether or not to allow checkpoints to reload turbos and shells */
#define SERVER_MODE_CHKPT_RELOAD	(1<<17)

#define DEFAULT_SERVER_MODE \
    ( SERVER_MODE_NORMAL\
    | SERVER_MODE_WRECK_UPRIGHT \
    | SERVER_MODE_EXPLOSION_DISABLED \
    | SERVER_MODE_SIMULTANEOUS_START \
    | SERVER_MODE_TIMEOUT_FINISH)

/* Parameters for update_leader_board */
#define STANDINGS_CURRENT 	0
#define STANDINGS_FINAL		1

/************************** STRUCTURES AND TYPES ******************************/
typedef struct _course {
    char coursename[256];
    int start_scene_x;		/* X coordinate of Starting Scene */
    int	start_scene_z;		/* Z coordinate of Starting Scene */
    float start_angle;		/* Which way to face when starting */
    float start_position[3];	/* Where to start putting cars */
    int num_positions;		/* How many vehicles per row */
    float vehicle_hspacing;	/* How far to move for next vehicle in the row*/
    float vehicle_vspacing;	/* How far to move for next row of vehicles */
    int turbo_mode;
    int	turbo_boosts; 		/* Number of times user can use turbo */
    int guns_mode;
    int	ammo; 			/* Number of times user can shoot */

    int practice_seconds;
    int	pre_race_seconds;
    int	race_seconds;
    int	post_race_seconds;	/* How long each phase should last */

    struct _course *next;
} COURSE;

typedef struct _spline_curve {
    int order;			/* B-spline order. */
    int size;			/* Control polygon size. */
    int dimension;		/* Vertex dimension. */
    float *polygon;		/* Control polygon. */
    float *kvector;		/* Knot vector. */
} SPLINE_CURVE;



/************************** PROGRAM GLOBALS ***********************************/
/*** From drive_server.c ***/
extern unsigned int server_mode;
extern connection_type *connection_list;
extern int img_fildes;
extern unsigned int current_nameset_bit;
extern unsigned int allowable_vehicles;
extern boolean_type server_reread_scenefiles;
extern int frames,updates;
extern int cur_vehicle_types;
extern Driveable driveables[];

/*** From server_interface.c ***/
extern int
    server_state,
    loop_timer,
    ignore_input;
extern COURSE *course;	/* Linked list of valid courses */
extern COURSE *current_course;
extern COURSE default_course;
extern time_value   server_virtual_time;
extern boolean_type server_virtual_time_frozen;

/*** From drive_msg.c ***/
extern int cur_seg;

/************************** FUNCTION PROTOTYPES *******************************/
/*** From drive_server.c ***/
extern void shutdown_server(
    void);
extern void update_all_clients_state(
    int client_state);
extern boolean_type server_restart_client(
    connection_type *c,
    struct IPCMsg *Msg);
extern boolean_type server_upright(
    connection_type *c,
    struct IPCMsg *Msg);
extern void update_leader_board(
    int standings_type);
extern void reset_all_clients(
    void);
extern void cause_explosion(
    SCENE *scene,
    float x, float y, float z,
    float radius, float base_force, float base_torque);
extern void change_virtual_time(
    boolean_type time_frozen,
    time_value virtual_time);


/*** From server_interface.c ***/
extern void outputString( const char *str );
extern void open_server_interface(
    int argc,
    char **argv);
extern void process_X_events(
    void);
extern void next_server_state(
    void);
extern void loop_timer_done(
    void);
extern void set_practice_mode(
    void);	/* Practice forever, state clock frozen */
extern void state_clock_tick(
    void);	/* Call once per second */
extern void set_virtual_time_slider(
    boolean_type override,	/* replace it even if the user's changed it */
    time_value newtime,
    boolean_type freeze_it);
extern void virtual_clock_tick(
    void);	/* Call once per minute */
extern time_t time_value_to_secs(
    time_value vt);


/*** From parse_scenefile.c ***/
extern int preview_scenefile(
    const char path[]);
extern int read_scenefile(
    const char path[], int *retval);


/*** From parse_config.c ***/
extern void read_config_file(
    void);


/*** From dlist.c ***/
extern void transmit_segment(
    int fildes,
    connection_type *con,
    int segno);


/*** From query.c ***/
extern int setup_drive_query_socket(
    void);
extern void check_drive_query_socket(
    int skt);


/*** From spline.c ***/
extern boolean_type barycentrictest(
    float x, float z, float *p0, float *p1, float *p2);
extern void curve_coeff(
    SPLINE_CURVE *c, int r, int index, float *result);
void curve_eval(
    SPLINE_CURVE *c, int r, float t, float *result);
float curve_amax(
    SPLINE_CURVE *c, int index);
float curve_vavg(
    SPLINE_CURVE *c, int index);
void curve_open(
    SPLINE_CURVE *c);
float curve_length(
    SPLINE_CURVE *c, int index);



#endif /* _DRIVE_SERVER_INCLUDED */
