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
 * server.c - Server.
 */

#define VIRTUAL_TIME_WORKING TRUE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef WIN32
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "dirent_win.h"
#else

#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>

#ifndef MAC
#include <X11/X.h>
#endif

#ifndef __linux__
#include <dirent.h>
/*
#include <ndir.h>
#include <sigset.h>
#include <sigaction.h>
*/
#include <sys/signal.h>
#else
#include <dirent.h>
#include <signal.h>
#endif

#endif

#include "global.h"
#include "message.h"
#include "newMessage.h"
#include "sockets.h"
#include "connection.h"
#include "object.h"
#include "controls.h"
#include "scene.h"
#include "drive_server.h"
#include "filenames.h"
#include "demo_physics.h"
#include "drive.h"

#ifndef EPSILON
# define EPSILON (1.0e-6)
#endif /* EPSILON */


#define MAX_OBJECTS 300
#define MAX_FRAMES  1		/* Maximum number of frames we can buffer. */
#define MAX_VEHICLE_TYPES 20    /* Maximum number of driveable vehicle types */

extern void delete_object(DRIVE_OBJECT *obj);

/*** GLOBALS ***/
int numSocks = 32, *sockList;
connection_type **playerList;
int Connects, Connected;
#undef StartTime
#undef CurrentTime
double StartTime, CurrentTime;

boolean_type debug=0;
boolean_type show_frames_per_sec=0;
unsigned int server_mode;
unsigned int current_nameset_bit;
unsigned int allowable_vehicles = VEHICLE_MASK & (~(OBJECTCLASS_SPACESHIP) );
boolean_type server_reread_scenefiles = FALSE;
SCENE *scene_head = NULL;
int scene_ed = FALSE;               /* we are NOT in the scene editor! */
int frames,updates;
int cur_vehicle_types = 0;/* number of vehicle types known by server */
Driveable driveables[MAX_VEHICLE_TYPES];
/* Global array to store information about driveable vehicles
 * as they are created.
 */

static boolean_type server_read();
static void server_connect(int);
static void server_update();

#define MIDDLE_BUTTON_MASK	0x200
#define RIGHT_BUTTON_MASK	0x300

extern int cur_seg;

typedef struct place_info {
    int last_checkpoint;
    float time_to_last_checkpoint;
    connection_type *con;
} PLACE_INFO;


#define PRE				0
#define PUSH				1

#define CLOCKWISE			0
#define COUNTER_CLOCKWISE		1

void polygon3d( int, float *, int, int );

/*** Module globals ***/
#ifdef WIN32
static struct _timeb start, finish;
#else
static struct timeval start,finish;
static struct timezone tz;
#endif
static float seconds,usec;
static int num_connections = 0;
#ifndef WIN32
static sigset_t  signals;  /* for use with sigpending */
#endif

double SrvGetTime(void);

static boolean_type server_close(
    connection_type *c,
    struct IPCMsg *Msg);
static boolean_type server_register(
    connection_type *c,
    struct CliRegister *Msg);
static boolean_type server_input(
    connection_type *c,
    struct CliInput *Msg);
static boolean_type server_disconnect(
    connection_type *c,
    struct IPCMsg *Msg);
static boolean_type server_cready(
    connection_type *c,
    struct CliReady *Msg);
static boolean_type server_lights(
    connection_type *c,
    struct CliLights *Msg);
boolean_type server_upright(
    connection_type *c,
    struct IPCMsg *Msg);
#if defined(SERVER_EVENTS)
static boolean_type server_error(
    connection_type *c,
    struct IPCMsg *Msg);
#endif
static boolean_type server_new_vehicle(
    connection_type *c,
    struct CliReady *Msg);		/* Sends data same as CliReady */

#if defined(SERVER_EVENTS)
/* Server event table. */
static boolean_type (* server_table[])(
	message_header_type *m,
	connection_type *c) = {
    server_close,		/* Shut down server. */
    server_register,		/* Register client. */
    server_input,		/* Client input information message. */
    server_disconnect,		/* Client wants to disconnect  */
    server_cready,		/* Client is ready to receive frames */
    server_restart_client,	/* Client is stuck -- restart him */
    server_lights,		/* Turn client object lights on or off. */
    server_upright,		/* Set the vehicle upright */
    server_new_vehicle,		/* Change vehicles */
    server_error		/* ? */
};
#endif

static socket_type *listen_socket;

/*
  This now lives in dlist.c
connection_type *connection_list = (connection_type *)NULL;
*/

#ifndef WIN32
struct itimerval timer, oldtimer;
#endif

connection_type *starting_positions[MAX_PLAYERS];

/* Packet send buffers. */
static char *send_buf;
static char *send_ptr;
static char *radar_buf;
static char *radar_ptr;
static int radar_buf_size;

int img_fildes, do_textures;

hwObject textureArray[1024];
int textureSeg, numTextures;

COURSE default_course = {
    "Default",			/* name */
    0, -2,			/* start_scene_[xz] */
    0.0,			/* start angle */
    { 88.0, 1.0, -110.0 },	/* start position */
    8,				/* num positions */
    15.0, 30.0,			/* hspacing, vspacing */
    TURBO_UNLIMITED, 0,		/* turbo mode, boosts */
    GUNS_UNLIMITED, 0,		/* guns mode, ammo */
    120, 5, 120, 5,		/* times */
    NULL			/* next */
};



static void check_already_running(
    void)
{
#ifndef WIN32
    static struct sembuf sema_op = { 0, -1, SEM_UNDO|IPC_NOWAIT };
    int semid;

    if ((semid = semget(SEMKEY,1,0666)) == -1) {
	/* Not already there.  Create it. */
	if ((semid = semget(SEMKEY,1,0666|IPC_CREAT)) == -1) {
	    fprintf(stderr,"Cannot create semaphore.  Aborting.\n");
	    perror("");
	    exit(1);
	}
	/* Clear it */
	if (semctl(semid,0,SETVAL,1) == -1) {
	    fprintf(stderr,"Cannot clear semaphore.  Aborting.\n");
	    perror("");
	    exit(1);
	}
    }

    if (semop(semid,&sema_op,1) == -1) {
	fprintf(stderr,"ERROR:  server already running.  Aborting.\n");
	exit(1);
    }
#endif
}


static void read_scenefiles(
    void)
{
    DIR *dirp;
    struct dirent *dirent;
#ifdef WIN32
    struct _stat statbuf;
#else
    struct stat statbuf;
#endif
    char scenefile[MAXPATHLEN+1];

    if ((dirp = opendir(scene_dir)) == NULL) {
	/* No scene file directory. */
	return;
    }

    /* First scan through them to get inter-scene dependencies set up. */
    while ((dirent = readdir(dirp)) != NULL) {
	if ((strcmp(dirent->d_name,".") == 0)
		|| (strcmp(dirent->d_name,"..") == 0)
		|| (strcmp(dirent->d_name,"README") == 0))
	    continue;
	sprintf(scenefile,"%s/%s",scene_dir,dirent->d_name);
	/* Make sure it's a normal file. */
#ifdef WIN32
	if (_stat(scenefile,&statbuf) == -1) continue;
	if (statbuf.st_mode & _S_IFDIR) continue;
#else
	if (stat(scenefile,&statbuf) == -1) continue;
	if (statbuf.st_mode & 0070000) continue;
#endif

	preview_scenefile(scenefile);
    }

/* Mike Stroyan and Norm tracked down a defect in rewinddir() when used on
 * a CD file system -- the CD file system reports a block size that rewinddir
 * does not accept, so the call fails.  Closing then re-opening the directory
 * has the same effect as re-winding it */
#ifdef REWINDDIR_BUG_FIXED
    /* Now actually read and set up their contents. */
    rewinddir(dirp);
#else
    closedir(dirp);
    if ((dirp = opendir(scene_dir)) == NULL) {
	/* No scene file directory. */
	return;
    }
#endif

#ifdef OLD_WAY
    while ((dirent = readdir(dirp)) != NULL) {
	if ((strcmp(dirent->d_name,".") == 0)
		|| (strcmp(dirent->d_name,"..") == 0)
		|| (strcmp(dirent->d_name,"README") == 0))
	    continue;
	sprintf(scenefile,"%s/%s",scene_dir,dirent->d_name);

	/* Make sure it's a normal file. */
	if (stat(scenefile,&statbuf) == -1) continue;
	if (statbuf.st_mode & 0070000) continue;

	read_scenefile(scenefile);
    }
#endif

    closedir(dirp);
}

#if 0
static void gopen_display_list(
    void)
{
    img_fildes = gopen("/dev/null",OUTDEV,"display_list",0);
}
#endif

/*****************************************************************
 * server_connect
 * 
 * 	Sets up a new client connection.
 * 	
 * Inputs:
 *		none.
 * Outputs:
 *		nothing.
 */
static void server_connect(
    int playerNum)
{
    connection_type *newcon;

    struct SrvDriveable dMsg;
    struct SrvIdentify  iMsg;
    struct SrvVirtualTime vMsg;

    int i;
    DRIVE_OBJECT *new_object;
    OBJECT_ID *idptr;
#ifndef V4_FILE_SYS
    int bufsize;
#endif
    struct linger linger;
    float fversion;

#ifdef WIN32
    _ftime(&start);
#else
    gettimeofday(&start,&tz);
#endif
    frames = 0;
    updates = 0;
    linger.l_onoff = 1;
    linger.l_linger = 10;

    outputString("New Connection");

    /* Create new connection and the associated socket. */
    newcon = con_create();
    newcon->socket = malloc(sizeof(socket_type));
    if( !newcon->socket ) {
        fprintf(stderr, "server_connect: out of memory\n");
        exit(1);
    }
    newcon->index = playerNum;
    newcon->socket->socketnum = sockList[playerNum];
    newcon->socket->index = playerNum;

    newcon->ready = FALSE;        /* client not ready to receive updates yet */
    newcon->initialized = FALSE;  /* client not ready to receive anything yet */
    /*
    newcon->number = num_connections++;
    Don't increment this number until the client is ready to begin receiving
    frames, and then only if it is not a watch mode client.
    */

    /* Add new connection to the connection list. */
    newcon->next = connection_list;
    if ( connection_list )
	connection_list->prev = newcon;
    connection_list = newcon;
    playerList[playerNum] = newcon;

    /* If any driveable objects haven't been created, do so now. */
    for (idptr=object_id; idptr->number != INVALID; ++idptr) {
	if (idptr->flags & OBJECTCLASS_DRIVEABLE) {
	    if( (idptr->flags & VEHICLE_MASK) & allowable_vehicles ) {
		for (i=0; i<cur_vehicle_types; i++) {
		    if (driveables[i].object_number == idptr->number) break;
		}
		if (i == cur_vehicle_types) {
		    /* need to create it */
		    if ((new_object
			    = (DRIVE_OBJECT *)
			    malloc(sizeof(DRIVE_OBJECT))) == NULL) {
			fprintf(stderr,"Out of malloc space!\n");
			continue;
		    }
		    new_object->idptr = idptr;
		    _hp_identity(new_object->xform);
		    new_object->size[0] = new_object->size[1] = 
			new_object->size[2] = 0.0;
		    new_object->angle  = 0.0;
		    new_object->radius = 0.0;
		    new_object->nameset_bits = ALL_TIMED_NAMESETS;
		    new_object->connection = 0;
		    complete_object(NULL,new_object);
		}
	    }
	}
    }

    /* Clear the "sent" bit for all graphics */
    clearSegmentSent( newcon->socket->index );

    /* Transmit to the client all the driveable vehicles that 
     * we know about.
     */
    for(i=0;i<cur_vehicle_types;i++)
    {
        if(debug) printf(" ------------ Vehicle #%d --------------\n",i);
        if(debug) printf(" Name:  %s\n",driveables[i].name);
        if(debug) printf(" Num:   %d\n",driveables[i].object_number);
        if(debug) printf(" Horsepower:  %f\n",driveables[i].horsepower);
        if(debug) printf(" Display List:  %d\n",driveables[i].display_list);

	transmit_segment( -1, newcon, driveables[i].display_list );

	dMsg.MsgType = SRV_DRIVEABLE;
	dMsg.MsgLength = SRV_DRIVEABLE_SIZE;
	memcpy(&(dMsg.driveable), &(driveables[i]), sizeof(Driveable));
	PutMsg((struct IPCMsg *)&dMsg,newcon->socket->socketnum );
    }

    /* Also send the texture segment */
    if( do_textures ) {
	transmit_segment( -1, newcon, textureSeg );
    }

    /* After all the above stuff is sent, send a client identify 
     * message to:
     * A) Let the client know who he is
     * B) Indicate that all display list segments have been sent.
     * C) Transmit server revision number.
     */
    iMsg.MsgType = SRV_IDENTIFY;
    iMsg.MsgLength = SRV_IDENTIFY_SIZE;

    iMsg.connection = (SP_I32)(intptr_t)newcon;
    sscanf( version, "$Revision: %f", &fversion );
    iMsg.version = (int)floor( fversion );

    /* For right now, use "Night Driving" string in config file. */
    iMsg.use_server_time = (server_mode & SERVER_MODE_NIGHT_DRIVING) != 0;

    PutMsg((struct IPCMsg *)&iMsg,newcon->socket->socketnum );

#ifdef VIRTUAL_TIME_WORKING
    /* Next send a message to update his virtual time clock */


    vMsg.MsgType = SRV_VIRTUAL_TIME;
    vMsg.MsgLength = SRV_VIRTUAL_TIME_SIZE;

    vMsg.static_time    = server_virtual_time_frozen;
    vMsg.reference_time = time_value_to_secs(server_virtual_time);

    PutMsg((struct IPCMsg *)&vMsg,newcon->socket->socketnum );
#endif /* VIRTUAL_TIME_WORKING */

    /* Gotta flush here - we want the client to recieve these msgs NOW */
    FlushMsg( newcon->socket->socketnum );
}


static void get_starting_position(
    connection_type *c)
{
    int i;
    int found =0;

    i = 0;
    while( !found)
    {
       if (starting_positions[i] == (connection_type *)NULL )
       {
	   found = 1;
	   starting_positions[i] = c;
       }
       i++;
    }
}


static void update_client_state(
    connection_type *con,
    int client_state)
{
    struct SrvUpdateState Msg;
    Msg.MsgType = SRV_UPDATE_STATE;
    Msg.MsgLength = SRV_UPDATE_STATE_SIZE;
    Msg.state = client_state;
    PutMsg((struct IPCMsg *)&Msg,con->socket->socketnum);
    FlushMsg(con->socket->socketnum);
}


/*****************************************************************
 * server_cready
 * 
 * 	A Client is ready to begin receiving frames
 * 	
 * Inputs:
 *		
 * Outputs:
 *		nothing.
 */
static boolean_type server_cready(
    connection_type *c,
    struct CliReady *Msg)
{
    UpdateDisp upd;

    c->ready = TRUE;
    c->initialized = TRUE;
    memcpy(&upd, &(Msg->upd), sizeof(UpdateDisp));

    /* If we are in watch mode, don't create an object for this
     * connection.
     */
    if ( c->mode == CON_WATCH_MODE )
	return FALSE;

    if ((c->obj = (DRIVE_OBJECT *) malloc(sizeof(DRIVE_OBJECT))) == NULL) {
	fprintf(stderr,"Out of malloc space.\n");
	return FALSE;
    }

    c->number = num_connections++;

    /* get this new client a position in the starting_positions array */
    get_starting_position(c);

    c->obj->idptr = find_object(upd.invis[1],NULL);
    _hp_identity(c->obj->xform); c->obj->xform[3][2] = -100.0;
    c->obj->size[0] = c->obj->size[1] = c->obj->size[2] = 0.0;
    c->obj->radius = c->obj->angle = 0.0;
    c->obj->nameset_bits = ALL_TIMED_NAMESETS;
    c->obj->scene = scene_head;

    c->obj->connection = c;
    complete_object(&(scene_head->object_head),c->obj);
    c->obj->controls.pointer_x = 0.0;
    c->obj->controls.pointer_y = 0.0;
    c->obj->controls.aim_pointer_x = 0.0;
    c->obj->controls.aim_pointer_y = 0.0;
    c->obj->controls.gear = 0;

    if (c->obj->upd != NULL) {
	c->obj->upd->color[0] = upd.color[0];
	c->obj->upd->color[1] = upd.color[1];
	c->obj->upd->color[2] = upd.color[2];
	memcpy( c->obj->upd->name, upd.name,10);
	c->obj->upd->name[9] = '\0';
    }

    /* Give the player a starting position -- the code to do that is in
     * server_restart_client, so simply call server_restart.
     */
    
    server_restart_client(c,(struct IPCMsg*)Msg);

    /* Now, if we are in demo mode, we need to send the client a message
     * telling it to go to the current state.
     */

    if (server_mode & (SERVER_MODE_DEMO|SERVER_MODE_DEMO_INTERRUPT)) {
	update_client_state(c,server_state);
    }

    return(FALSE);
}


/*****************************************************************
 * server_new_vehicle
 * 
 * 	A Client wants a new vehicle
 * 	
 * Inputs:
 *		
 * Outputs:
 *		nothing.
 */
static boolean_type server_new_vehicle(
    connection_type *c,
    struct CliReady *Msg)
{
    UpdateDisp upd;
    DRIVE_OBJECT *old_obj;
    SCENE *scene = (SCENE *) (c->obj->scene);
    float dirx,dirz,angle;

    memcpy(&upd, &(Msg->upd), sizeof(UpdateDisp) );

    /* If we are in watch mode, don't create an object for this
     * connection.
     */
    if (c->mode == CON_WATCH_MODE) return FALSE;

    old_obj = c->obj;

    /* Make a new one */
    if ((c->obj = (DRIVE_OBJECT *) malloc(sizeof(DRIVE_OBJECT))) == NULL) {
	fprintf(stderr,"Out of malloc space.\n");
	return FALSE;
    }

    c->obj->idptr = find_object(upd.invis[1],NULL);
    memcpy(c->obj->xform,old_obj->xform,sizeof(matrix3d));
    memcpy(c->obj->ixform,old_obj->ixform,sizeof(matrix3d));
    c->obj->scene = (void *) scene;
    c->obj->size[0] = c->obj->size[1] = c->obj->size[2] = 0.0;
    c->obj->radius = c->obj->angle = 0.0;
    c->obj->nameset_bits = ALL_TIMED_NAMESETS;
    c->obj->connection = c;
    complete_object(&(scene->object_head),c->obj);

    c->obj->controls.pointer_x = 0.0;
    c->obj->controls.pointer_y = 0.0;
    c->obj->controls.aim_pointer_x = 0.0;
    c->obj->controls.aim_pointer_y = 0.0;
    c->obj->controls.gear = 0;

    if (c->obj->upd != NULL) {
	c->obj->upd->color[0] = upd.color[0];
	c->obj->upd->color[1] = upd.color[1];
	c->obj->upd->color[2] = upd.color[2];
	memcpy(c->obj->upd->name, upd.name,10);
	c->obj->upd->name[9] = '\0';
    }

    /* Get the angle the front is pointing */
    dirx = c->obj->xform[2][0]; dirz = c->obj->xform[2][2];
    NORMALIZE2(dirx,dirz);
    if ((ABS(dirx) < EPSILON) && (ABS(dirz) < EPSILON)) dirz = 1.0;
    angle = FATAN2(dirz,dirx) - (M_PI/2.0);
    reset_vehicle_physics(c->obj,
	c->obj->xform[3][0],
	c->obj->xform[3][1],
	c->obj->xform[3][2],
	angle);

    /* Delete the old object */
    delete_object_from_scene(scene,old_obj);
    delete_object(old_obj);

    return(FALSE);
}


/*****************************************************************
 * server_watch_next
 * 
 * 	Chooses the next connection to watch for a watch mode 
 *	client.
 *
 * Inputs:
 *		c	Watch mode connection.
 */
static void server_watch_next(
    connection_type *c)
{
    connection_type *con_ptr;
    int real_clients=0;

    /* If the watch mode clients are the only connections, don't
     * watch anything.
     */
    con_ptr = connection_list;
    while(con_ptr != NULL)
    {
       if( !(con_ptr->mode & CON_WATCH_MODE) )
       {
	   real_clients = 1;
	   break;
       }
       con_ptr = con_ptr->next;
    }
    if( real_clients == 0)
    {
	c->watch = NULL;
	return;
    }

    /* Switch to the next connection. */
    if ( c->watch )
	c->watch = c->watch->next;

    while ( (c->watch == NULL) || (c->watch->mode & CON_WATCH_MODE) )
    {
	/* If the connection has not been initialized, or we're
	 * watching the last in the list, reset to point to the
	 * first client in the connection list.
	 */
	if ( c->watch == NULL )
	    c->watch = connection_list;
	else
	    c->watch = c->watch->next;
    }
}


static int find_starting_position(
    connection_type *c)
{
    int i;
    int found =0;

    i = 0;
    while( !found & ( i < MAX_PLAYERS ))
    {
       if (starting_positions[i] == c )
       {
	   found = 1;
	   return(i);
       }
       i++;
    }

    /* If we get to here, it means we did not find the specified connection
     * in the start list.  We are in trouble -- return -1 */

    return( INVALID );

}


/*****************************************************************
 * server_restart_client
 * 
 * 	A Client is stuck and needs to be reset 
 * 	
 * Inputs:
 *		
 * Outputs:
 *		nothing.
 */
boolean_type server_restart_client(
    connection_type *c,
    struct IPCMsg *Msg)
{
    float start_x, start_z, start_y;
    float angle;
    float start_row, start_col;
    float sin_t,cos_t;
    int scene_x, scene_z;
    int my_position;

    if (c == NULL) return FALSE;

    /* For right now, use restart as "switch to next" button for
     * watchmode clients.
     */
    if (c->mode == CON_WATCH_MODE) {
	server_watch_next( c );
	return FALSE;
    }

    start_y = current_course->start_position[1];
    angle   = current_course->start_angle;
    scene_x = current_course->start_scene_x;
    scene_z = current_course->start_scene_z;

    /* Reset all the client's checkpoint information */
    c->obj->checkpoints = 0;
    c->obj->last_checkpt = (DRIVE_OBJECT *)NULL;
    c->obj->time_to_checkpt = 0.0;
    if (server_mode & SERVER_MODE_STARTLINE_START) {
#ifdef WIN32
	c->obj->start_lapsec = start.time;
	c->obj->start_lapusec = 1000 * start.millitm;
#else
	c->obj->start_lapsec = start.tv_sec;
	c->obj->start_lapusec = start.tv_usec;
#endif
	c->obj->turbo_boosts = 0;
	c->obj->ammo_used = 0;
    }
    if (server_mode & SERVER_MODE_CHKPT_RELOAD) {
	/* He gets nothing 'til a checkpoint is crossed */
	c->obj->turbo_boosts = current_course->turbo_boosts;
	c->obj->ammo_used = current_course->ammo;
    }
    
    if ((server_mode & SERVER_MODE_DEMO_INTERRUPT)
	    && !(server_state == PRE_RACE_STATE)) {
	start_x = current_course->start_position[0]
	    + current_course->start_scene_x * 2000;
	start_z = current_course->start_position[2]
	    + current_course->start_scene_z * 2000;
    }
    else if (server_mode
	    & (SERVER_MODE_RACE|SERVER_MODE_DEMO|SERVER_MODE_DEMO_INTERRUPT)) {
	start_x = current_course->start_position[0]
	    + current_course->start_scene_x * 2000;
	start_z = current_course->start_position[2]
	    + current_course->start_scene_z * 2000;
    }
    else {
	/* Use the information provided by the client to start the client where
	 * it wants to be started.  Always use the specified scene coordinates,
	 * but only use the X, Y, Z offsets if they are valid. */

	if (c->start_pos.angle != POSITION_INVALID)
	    angle = c->start_pos.angle;
	if(c->start_pos.scene_x != POSITION_INVALID)
	    scene_x = c->start_pos.scene_x;
	if(c->start_pos.scene_z != POSITION_INVALID)
	    scene_z = c->start_pos.scene_z;

	if( c->start_pos.pos[0] != POSITION_INVALID ) /* X value is valid */
	{
	    start_x = c->start_pos.pos[0] + (scene_x * 2000);
	}
	else
	{
	    start_x = current_course->start_position[0] + (scene_x * 2000);
	}

	if( c->start_pos.pos[1] != POSITION_INVALID ) /* Y value is valid */
	{
	    start_y = c->start_pos.pos[1]; 
	}
	else
	{
	    start_y = current_course->start_position[1];
	}

	if( c->start_pos.pos[2] != POSITION_INVALID ) /* Z value is valid */
	{
	    start_z = c->start_pos.pos[2] + (scene_z * 2000);
	}
	else
	{
	    start_z = current_course->start_position[2] + (scene_z * 2000);
	}
    }

    my_position = find_starting_position(c);
    if (my_position ==  INVALID) {
	printf("Invalid Starting Position! \n");
	my_position = 0;
    }

    start_row = my_position / current_course->num_positions;
    start_col = my_position % current_course->num_positions;

    sin_t = FSIN(DEGREES_TO_RADIANS(angle));
    cos_t = FCOS(DEGREES_TO_RADIANS(angle));

    start_x +=
	 (current_course->vehicle_hspacing * cos_t * start_col)
	+ (current_course->vehicle_vspacing * sin_t * start_row);
    start_z +=
	 (current_course->vehicle_hspacing * sin_t * start_col)
	- (current_course->vehicle_vspacing * cos_t * start_row);

    reset_vehicle_physics(c->obj, start_x,start_y,start_z,
	DEGREES_TO_RADIANS(angle));

    if (Msg == NULL) update_leader_board(STANDINGS_CURRENT);

    return(FALSE);
}


/*****************************************************************
 * server_upright
 * 
 * 	A Client is stuck and needs to be reset 
 * 	
 * Inputs:
 *		
 * Outputs:
 *		nothing.
 */
boolean_type server_upright(
    connection_type *c,
    struct IPCMsg *Msg)
{
    DRIVE_OBJECT *obj;

    if (c == NULL || c->obj == NULL) return FALSE;
    obj = c->obj;
    int penalty_seconds;

    if (set_vehicle_upright(obj)) {
	/* Penalize the driver for his heinous mistake */
	penalty_seconds = current_course->race_seconds / 10;
	obj->start_lapsec -= penalty_seconds;
	obj->time_to_checkpt += penalty_seconds;
    }

    update_leader_board(STANDINGS_CURRENT);

    return(FALSE);
}


/*****************************************************************
 * server_disconnect
 * 
 * 	Closes a client connection.
 * 	
 * Inputs:
 *		none.
 * Outputs:
 *		nothing.
 */
static boolean_type server_disconnect(
    connection_type *c,
    struct IPCMsg *Msg)
{
    struct IPCMsg cMsg;
    int my_position;
    connection_type *cptr;

#ifdef WIN32
    _ftime(&finish);

    if (start.millitm > finish.millitm )
    {
        finish.millitm += 1000;
        finish.time--;
    }
    seconds = finish.time - start.time;
    usec = 1000 * (finish.millitm - start.millitm);
#else
    gettimeofday(&finish,&tz);

    if( start.tv_usec > finish.tv_usec)
    {
	finish.tv_usec += 1000000;
	finish.tv_sec-- ;
    }
    seconds = finish.tv_sec - start.tv_sec;
    usec = finish.tv_usec - start.tv_usec;
#endif
   if(debug) printf("Took  %f  seconds,  %f micro seconds\n",seconds, usec);
   if(debug) printf("Displayed %d frames\n",frames);
   if(debug) printf("Frames per Second:  %f\n",(float)frames / seconds );
   if(debug) printf("Updates: %d \n",updates);
   if(debug) printf("updates per Second:  %f\n",(float)updates / seconds );

    cMsg.MsgType = SRV_CLOSE;
    cMsg.MsgLength = 0;
    PutMsg(&cMsg, c->socket->socketnum);
    FlushMsg(c->socket->socketnum);


    {
        char msg[80];
        sprintf( msg, "%s@%s disconnecting....", c->username,c->hostname);
        outputString(msg);
    }

    /* If someone is watching me, Switch the watcher to the next guy */
    cptr = connection_list;
    while (cptr != NULL )
    {
	if(cptr->mode == CON_WATCH_MODE)
	{
	    if( cptr->watch == c)
		server_watch_next( cptr );
	}
	cptr = cptr->next;

    }

    /* Remove connection from connection list. */
    TermIPC( c->socket->socketnum );

    if ( c->prev )  
    {
	/* I am NOT the first guy in the list, so point my prev to my
	 * next, taking me out of the loop */

	c->prev->next = c->next;
	if( c->next) 
	{
	    /* Also, I am not the last guy in the list, so point my 
	     * next to my prev, getting me out of that loop to! */
	    c->next->prev = c->prev;
	}
    }
    else
    {
	/* I AM the first guy in the list. */
	/* If my next happens to be NULL, then the following assignment
	 * is ok. (ie, connection_list is then null.) If my next is NOT
	 * Null, then it becomes the head of the list. */

	connection_list = c->next;
	if( c->next )
	{
	    c->next->prev = NULL;
	}
    }

    /* Remove object from object list. */
    if ( (c->mode != CON_WATCH_MODE) && c->obj )
    {
	delete_object_from_scene(c->obj->scene,c->obj);
	free(c->obj);
    }

    my_position = find_starting_position(c);
    if( my_position != INVALID)
    {
       starting_positions[my_position] = (connection_type *)NULL;
    }

    free( c->socket );
    playerList[c->index] = NULL;
    sockList[c->index] = IPC_INVALID_SOCK;

    free( c );

    return FALSE;
}

#ifdef SERVER_CHECK_HEADER
static void abnormal_client_shutdown(
    connection_type *c)
{
    /* Remove connection from connection list. */
    TermIPC( c->socket->socketnum );

    if ( c->prev )  
    {
	/* I am NOT the first guy in the list, so point my prev to my
	 * next, taking me out of the loop */

	c->prev->next = c->next;
	if( c->next) 
	{
	    /* Also, I am not the last guy in the list, so point my 
	     * next to my prev, getting me out of that loop to! */
	    c->next->prev = c->prev;
	}
    }
    else
    {
	/* I AM the first guy in the list. */
	/* If my next happens to be NULL, then the following assignment
	 * is ok. (ie, connection_list is then null.) If my next is NOT
	 * Null, then it becomes the head of the list. */

	connection_list = c->next;
	if( c->next )
	{
	    c->next->prev = NULL;
	}
    }

    /* Remove object from object list. */
    if ( c->mode != CON_WATCH_MODE )
    {
	delete_object_from_scene(c->obj->scene,c->obj);
	free(c->obj);
    }

    free( c->socket );
    free( c );
}
#endif /* SERVER_CHECK_HEADER */


/*****************************************************************
 * server_read
 * 
 * 	
 * 	
 * Inputs:
 *		c	Connection that message came in on.
 * Outputs:
 *		nothing.
 */
static boolean_type server_read(
    connection_type *c)
{
    boolean_type done = FALSE;
#if defined(SERVER_CHECK_HEADER)
    message_header_type msg;
    int status;
#endif
    struct IPCMsg *Msg;
    

    Msg = GetMsg( c->socket->socketnum );
    while( Msg ) {

	switch( Msg->MsgType ) {
	case IPC_KILL :
	    (void)puts( "IPC_KILL received; shutting down" );
	    /* Now, kill the IPC and die */
	    TermIPC( c->socket->socketnum );
	    exit( 1 );
	    /* NOTREACHED */


	case CLI_CLOSE :
	    server_close(c, Msg);
	    break;

	case CLI_REGISTER :
	    server_register(c, (struct CliRegister *)Msg);
	    break;

	case CLI_INPUT :
	    server_input(c, (struct CliInput *)Msg);
	    break;

	case IPC_TERM :
	case CLI_DISCONNECT :
	    server_disconnect(c, Msg);
	    c = 0;
	    /* DRIVE_EXIT_WHEN_EMPTY: a private server started by a client
	     * has no reason to outlive it.
	     */
	    if (!connection_list && getenv("DRIVE_EXIT_WHEN_EMPTY"))
		done = TRUE;
	    break;

	case CLI_READY :
	    server_cready(c, (struct CliReady *)Msg);
	    break;

	case CLI_RESTART :
	    server_restart_client(c, Msg);
	    break;

	case CLI_LIGHTS :
	    server_lights(c, (struct CliLights *)Msg);
	    break;

	case CLI_UPRIGHT :
	    server_upright(c, Msg);
	    break;

	case CLI_NEW_VEHICLE :
	    server_new_vehicle(c, (struct CliReady *)Msg);
	    break;

	default:
	    printf("Unknown Message type: %d\n", Msg->MsgType);
	}
	if( c ) {
	    Msg = GetMsg( c->socket->socketnum );
	}
	else {
	    Msg = 0;
	}
    }	/* End while Msg */

#if defined(SERVER_CHECK_HEADER)
    /* Read message header. */
    status = read( c->socket->socketnum, &msg, sizeof( message_header_type ) );
    
    switch ( status )
    {
      case -1:
	/* Bad read. Scream and die. */
	perror( "Server read" );
	abnormal_client_shutdown(c); /* try to avoid killing server */
	/*
	done = TRUE;
	*/
	break;
	
      case 0:
	/* Client is not active anymore.  Remove this client
	 * from the connection list.
	 */
	server_disconnect( &msg,c );
	break;
	
      default:
	/* Good read.  Messages cause calls to the functions in the
	 * server table above.  The table is indexed by the type of
	 * message.
	 */
	if ((msg.type >= 0) && (msg.type <= SERVER_MAXMSG)) {
	    done = (*server_table[msg.type] )( &msg, c );
	}
	else {
	    server_error(&msg,c);
	}
	break;
    }

#endif
    return done;
}


/*****************************************************************
 * server_close
 * 
 * 	Terminates server.
 *
 * Inputs:
 *		m	Message.
 *		c	Connection that message came in on.
 * Outputs:
 *		TRUE 	to terminate server.
 */
static boolean_type server_close(
    connection_type *c,
    struct IPCMsg *Msg)
{
    return TRUE;
}


/*****************************************************************
 * server_lights
 * 
 * 	Turn client object lights on or off.
 * 	
 * Inputs:
 *		m	Message.
 *		c	Connection that message came in on.
 * Outputs:
 *		FALSE to continue server execution.
 */
static boolean_type server_lights(
    connection_type *c,
    struct CliLights *Msg)
{
    if ( Msg->flags )
    {
	/* Turn lights on. */
	if ( c->obj )
	    c->obj->lights_on = TRUE;
    }
    else
    {
	/* Turn lights off. */
	if ( c->obj )
	    c->obj->lights_on = FALSE;
    }

    return FALSE;
}


/*****************************************************************
 * server_register
 * 
 * 	Registers a new client with the server.
 * 	
 * Inputs:
 *		m	Message.
 *		c	Connection that message came in on.
 * Outputs:
 *		FALSE 	to keep server going.
 */
static boolean_type server_register(
    connection_type *c,
    struct CliRegister *Msg)
{
    char msg[80];

    /* Store the information in the connection structure. */
    memcpy( c->hostname, Msg->hostname, 10 );
    memcpy( c->username, Msg->username, 10 );
    c->mode = Msg->mode;
    memcpy( &(c->start_pos), &(Msg->start_pos), sizeof(drive_start_type));

    if ( c->mode == CON_WATCH_MODE )
    {
	sprintf( msg, "Watch mode user \"%s\" connected from host \"%s\".",
		c->username, c->hostname );
	server_watch_next( c );
    }
    else
	sprintf( msg, "User \"%s\" connected from host \"%s\".",
		c->username, c->hostname );
    outputString(msg);

    return FALSE;
}

#ifndef SHELL_SPEED
# define SHELL_SPEED	MPH_TO_FPS(1500.0)
# define RELOAD_TIME	2	/* seconds */
#endif /* !SHELL_SPEED */

static void fire_shell(
    DRIVE_OBJECT *firer, int mine)
{
    SCENE *scene = (SCENE *) (firer->scene);
    DRIVE_OBJECT *obj;
    PHYSICAL_OBJECT *pobj;
    int last_segment;
    float px,py,pz;
    float vx,vy,vz;
    long t;


    if (((t = time(NULL)) - firer->last_shot) < RELOAD_TIME) return;
    /* else */
    firer->last_shot = t;

    if(mine)
    {
	float mpx = 0.0, mpy = 0.0, mpz = -17.0;
	float yfloat;

	if( firer->vehicle_auxdata != NULL)
	    yfloat = -(firer->vehicle_auxdata->bbox_mc[0][1]);  
	else
	    yfloat = 0.0;
	px =  mpx * firer->xform[0][0]
	    + mpy * firer->xform[1][0]
	    + mpz * firer->xform[2][0] + firer->xform[3][0];
	py =  mpx * firer->xform[0][1]
	    + mpy * firer->xform[1][1]
	    + mpz * firer->xform[2][1] + firer->xform[3][1] - yfloat;
	pz =  mpx * firer->xform[0][2]
	    + mpy * firer->xform[1][2]
	    + mpz * firer->xform[2][2] + firer->xform[3][2];

	vx = (*(firer->aim_xform))[2][0];
	vy = (*(firer->aim_xform))[2][1];
	vz = (*(firer->aim_xform))[2][2];

	/* Place it outside the firer */
	last_segment = cur_seg;
	obj = add_object_to_scene(scene,"Land Mine","One Shot","",
	    FATAN2(vz,vx) - M_PI/2.0,
	    px - scene->xscene*SCENE_SIZE,
	    py,
	    pz - scene->zscene*SCENE_SIZE,
	    10.0, 10.0, 0.5,
	    DEFAULT_OBJECT_RADIUS, DEFAULT_OBJECT_ANGLE, 
	    10.0 /* live for 10 sec*/, 0);
	if (obj == NULL) return;
	obj->idptr->flags |= OBJECTCLASS_DYNAMIC;
	memcpy(obj->xform, firer->xform, 4*4*sizeof(float));
	obj->xform[3][0] = px;
	obj->xform[3][1] = py;
	obj->xform[3][2] = pz;

    }
    else /* Regular Shell */
    {
	px =  firer->fire_point[0] * (*(firer->aim_xform))[0][0]
	    + firer->fire_point[1] * (*(firer->aim_xform))[1][0]
	    + firer->fire_point[2] * (*(firer->aim_xform))[2][0]
	    + (*(firer->aim_xform))[3][0];
	py =  firer->fire_point[0] * (*(firer->aim_xform))[0][1]
	    + firer->fire_point[1] * (*(firer->aim_xform))[1][1]
	    + firer->fire_point[2] * (*(firer->aim_xform))[2][1]
	    + (*(firer->aim_xform))[3][1];
	pz =  firer->fire_point[0] * (*(firer->aim_xform))[0][2]
	    + firer->fire_point[1] * (*(firer->aim_xform))[1][2]
	    + firer->fire_point[2] * (*(firer->aim_xform))[2][2]
	    + (*(firer->aim_xform))[3][2];
	vx = (*(firer->aim_xform))[2][0];
	vy = (*(firer->aim_xform))[2][1];
	vz = (*(firer->aim_xform))[2][2];

	/* Place it outside the firer */
	last_segment = cur_seg;
	obj = add_object_to_scene(scene,"Shell","","",
	    FATAN2(vz,vx) - M_PI/2.0,
	    px - scene->xscene*SCENE_SIZE,
	    py,
	    pz - scene->zscene*SCENE_SIZE,
	    DEFAULT_OBJECT_SIZE, DEFAULT_OBJECT_SIZE, DEFAULT_OBJECT_SIZE,
	    DEFAULT_OBJECT_RADIUS, DEFAULT_OBJECT_ANGLE, DEFAULT_OBJECT_SPACING,
	    0);
	if (obj == NULL) return;

	/* Give it some speed! */
	pobj = obj->pobj;
	pobj->v[1] = vx * SHELL_SPEED;
	pobj->v[2] = vy * SHELL_SPEED;
	pobj->v[3] = vz * SHELL_SPEED;
	pobj->p[1] = pobj->v[1] * pobj->mass;
	pobj->p[2] = pobj->v[2] * pobj->mass;
	pobj->p[3] = pobj->v[3] * pobj->mass;
	pobj->w[1] = pobj->w[2] = pobj->w[3] = 0.0;
	pobj->L[1] = pobj->L[2] = pobj->L[3] = 0.0;
    }

#if 0
    if (last_segment != cur_seg) update_segment_lists(last_segment);
#endif
    ++(firer->ammo_used);
}


/*****************************************************************
 * server_input
 * 
 * 	Updates input information from a client.
 * 	
 * Inputs:
 *		m	Message.
 *		c	Connection that message came in on.
 * Outputs:
 *		FALSE 	to keep server going.
 */
static boolean_type server_input(
    connection_type *c,
    struct CliInput *Msg)
{
    /* TBD: Make this OS-independent (we should not be using
       X11 types in our IPC... */

    /* We've gotten input; must be time for a new frame */
    c->ready = TRUE;

    if (!c->obj) {
        return FALSE;
    }

    if (c->mode != CON_WATCH_MODE) {
	/* Update this client's control values. */
#if defined(WIN32) || defined(MAC)
	if (Msg->key_mask == (HW_KBD_MOD_SHIFT << 4))
#else
	if ((Msg->key_mask == ShiftMask)
		|| (Msg->key_mask == LockMask))
#endif
        {
	    c->obj->controls.aim_pointer_x = Msg->x_val;
	    c->obj->controls.aim_pointer_y = Msg->y_val;
	}
	else {
	    c->obj->controls.altitude_accel = 0.0;

#if defined(WIN32) || defined(MAC)
            if (Msg->key_mask & 1) {
		if(Msg->key_mask & (HW_KBD_MOD_SHIFT << 4)) {
		    c->obj->controls.altitude_accel = 1.0;
		}
		else {
		    c->obj->controls.altitude_accel = 0.2;
		}
            }
	    else if( Msg->key_mask & 4) {
		if(Msg->key_mask & (HW_KBD_MOD_SHIFT << 4)) {
		    c->obj->controls.altitude_accel = -1.0;
		}
		else {
		    c->obj->controls.altitude_accel = -0.2;
		}
	    }
#else
            if( Msg->key_mask & Button1Mask ) {
		if(Msg->key_mask & ShiftMask) {
		    c->obj->controls.altitude_accel = 1.0;
		}
		else {
		    c->obj->controls.altitude_accel = 0.2;
		}
	    }
	    else if( Msg->key_mask & Button3Mask ) {
		if(Msg->key_mask & ShiftMask) {
		    c->obj->controls.altitude_accel = -1.0;
		}
		else {
		    c->obj->controls.altitude_accel = -0.2;
		}
	    }
#endif
	    c->obj->controls.aim_pointer_x = 0.0;
	    c->obj->controls.aim_pointer_y = 0.0;
	    c->obj->controls.throttle = Msg->throttle;


	    c->obj->controls.pointer_x = Msg->x_val;
	    switch(current_course->turbo_mode) {
		case 0:   /* always put a cap on the y_val */
		    c->obj->controls.pointer_y = Msg->y_val;
		    if(c->obj->controls.pointer_y > 1.0)
			c->obj->controls.pointer_y = 1.0;
		    if(c->obj->controls.pointer_y < -1.0)
			c->obj->controls.pointer_y = -1.0;
		   break;
	       case 1:   /* always allow turbo */
		    c->obj->controls.pointer_y = Msg->y_val;
		   break;
	       case 2:   /* limited number of boosts */
		    if(c->obj->turbo_boosts < current_course->turbo_boosts)
		    {
			c->obj->controls.pointer_y = Msg->y_val;
			if( (Msg->y_val > 1.0) || (Msg->y_val < -1.0) )
			{
			    c->obj->turbo_boosts++;
			}
		    }
		    else
		    {
			c->obj->controls.pointer_y = Msg->y_val;
			if(c->obj->controls.pointer_y > 1.0)
			    c->obj->controls.pointer_y = 1.0;
			if(c->obj->controls.pointer_y < -1.0)
			    c->obj->controls.pointer_y = -1.0;
		    }
		   break;
	    }
	}

	/* drive thinks gears go 0 thru 6; server thinks -1 thru 5 */
	c->obj->controls.gear      = Msg->gear_wanted-1;

	if (
#if defined(WIN32) || defined(MAC)
            (Msg->key_mask & 2)
		&& (!(Msg->key_mask & (HW_KBD_MOD_CTRL << 4)))
#else
            (Msg->key_mask & Button2Mask)
		&& (!(Msg->key_mask & (ControlMask|Mod1Mask)))
#endif
		&& (!(server_mode & SERVER_MODE_EXPLOSION_DISABLED))
		&& (strcmp(c->obj->idptr->lcname,"ufo") != 0)) {
	    if ((current_course->guns_mode == GUNS_UNLIMITED)
		|| ((current_course->guns_mode == GUNS_LIMITED)
		    && (c->obj->ammo_used < current_course->ammo))) {
#if defined(WIN32) || defined(MAC)
		if(Msg->key_mask & (HW_KBD_MOD_SHIFT << 4))
#else
		if(Msg->key_mask & ShiftMask)
#endif
		    fire_shell(c->obj,1);
		else
		    fire_shell(c->obj,0);
	    }
	}
    }

    /* Decrement this client's frame count. */
    if ( c->frame_count )
	c->frame_count--;

    return FALSE;
}


#if defined(SERVER_EVENTS)
/*****************************************************************
 * server_error
 * 
 * 	Reports a I/O server error.
 *
 * Inputs:
 *		m	Message causing error.
 *		c	Connection that message came in on.
 * Outputs:
 *		FALSE 	to keep server going.
 */
static boolean_type server_error(
    connection_type *c,
    struct IPCMsg *Msg)
{
    /* This might result from a non-fatal error where a new client
     * is sending an unrecognized message to the server.  Handle
     * this as gracefully as you can.  If it's essential that the
     * versions be in sync, the version number should be bumped.
     */
    if (debug) fprintf(stderr,
	"Unrecognized client->server message.  Type: %02X, size %d\n",
	Msg->MsgType, Msg->MsgLength);
		
    return FALSE;
}
#endif /* SERVER_EVENTS */


static void send_dynamic_objects(
    DRIVE_OBJECT *obj, 
    connection_type *conn)
{
    DRIVE_OBJECT *child;
    struct SrvFrameMatrix
	matMsg;
    struct SrvFrameUpdate
	updMsg;

    updMsg.MsgType = SRV_FRAME_UPDATE;
    updMsg.MsgLength = SRV_FRAME_UPDATE_SIZE;

    if ((obj->display_list != INVALID)
	    && (obj->idptr->flags & OBJECTCLASS_DYNAMIC)) {
	/* Send this objects's UpdateDisp struct if necessary */
	if (obj->upd != NULL) {

	    memcpy(&(updMsg.upd),obj->upd,sizeof(UpdateDisp));
	    PutMsg( (struct IPCMsg *)&updMsg, conn->socket->socketnum);
	}

	/* Send this object's display list and matrix to client */

	transmit_segment( -1, conn, obj->display_list );

	matMsg.MsgType = SRV_FRAME_MATRIX;
	matMsg.MsgLength = SRV_FRAME_MATRIX_SIZE;
	matMsg.segment = obj->display_list;
	matMsg.connection = (SP_I32)(intptr_t)obj->connection;
	memcpy(matMsg.xform, obj->xform, 16*sizeof(float));

	PutMsg( (struct IPCMsg *)&matMsg, conn->socket->socketnum);
    }

    if ((child = obj->child_list) != NULL)
        send_dynamic_objects(child,conn);
    if ((child = obj->next) != NULL)
        send_dynamic_objects(child,conn);
}


/*****************************************************************
 * connection_update
 * 
 * 	Update one client.
 * 	
 */
static void connection_update(
    connection_type *sendconn)
{
    connection_type *watchconn;	/* Connection to look at. */
    DRIVE_OBJECT *obj;
    SCENE *myscene,*scene;
    float speed;
    int gear;
    int scenes, scene_no, scene_mask;
    float car_x,car_z;
    float patch_x,patch_z;

    struct IPCMsg
	beginMsg;
    struct SrvFramePosition 
	posMsg;
    struct SrvFrameGauge    
	gaugeMsg;
    struct SrvFrameGlobalNameset 
	namesetMsg;
    struct SrvFrameStaticSeg 
	staticMsg;
	
    if ( sendconn->mode == CON_WATCH_MODE )
	watchconn = sendconn->watch;
    else {
	watchconn = sendconn;
	if (!sendconn->obj)
	    return;
    }
	
    if ( watchconn && watchconn->obj )
	myscene = (SCENE *) (watchconn->obj->scene);
    else
	myscene = scene_head;
	
    /* Leave room for begin frame message and client position message. */
    send_ptr = send_buf + sizeof( message_header_type );
    send_ptr += ( sizeof( message_header_type )
		 + sizeof( position_msg_type )
		 + sizeof( message_header_type )
		 + sizeof( gauge_msg_type ) );

    beginMsg.MsgType = SRV_BEGINFRAME;
    beginMsg.MsgLength = 0;
    PutMsg( &beginMsg, sendconn->socket->socketnum);
	
    /* Send client object matrix, or watched object matrix under
     * separate cover to client.
     */
    posMsg.MsgType = SRV_FRAME_POSITION;
    posMsg.MsgLength = SRV_FRAME_POSITION_SIZE;

    if (sendconn->mode == CON_WATCH_MODE) {
	/* If we've got something to watch, use that object's
	 * transformation matrix.
	 */
	if (sendconn->watch) {
	    memcpy((void *) posMsg.xform,
		(void *) sendconn->watch->obj->xform, sizeof(float)*16);
	    memcpy((void *) posMsg.alt_view_xform,
		(void *) sendconn->watch->obj->aim_xform,sizeof(float)*16);
	}
	else {
	    memset((void *) posMsg.xform, 0, sizeof(float)*16);
	    memset((void *) posMsg.alt_view_xform, 0, sizeof(float)*16);
	}
    }
    /* If we're not in watch mode, use the object xform. */
    else {
	memcpy((void *) posMsg.xform,
	    (void *) sendconn->obj->xform, sizeof(float)*16);
	memcpy((void *) posMsg.alt_view_xform,
	    (void *) sendconn->obj->aim_xform,sizeof(float)*16);
    }

    PutMsg( (struct IPCMsg *)&posMsg, sendconn->socket->socketnum);

    /* Write in gauge message. */
    gaugeMsg.MsgType = SRV_FRAME_GAUGE;
    gaugeMsg.MsgLength = SRV_FRAME_GAUGE_SIZE;
	
    if ( watchconn && watchconn->obj )
    {
	speed = FHYPOT3(watchconn->obj->pobj->v[1],
			watchconn->obj->pobj->v[2],
			watchconn->obj->pobj->v[3]);
	gaugeMsg.mph = FPS_TO_MPH(speed);

	gaugeMsg.altitude = watchconn->obj->pobj->x[2];

	gear  = watchconn->obj->controls.gear + NEUTRAL_GEAR;
	if (gear == NEUTRAL_GEAR) {
	    gaugeMsg.rpm = IDLE_RPM;
	}
	else {
	    if ((gaugeMsg.rpm = speed
		 / watchconn->obj->vehicle_auxdata->gear_best_speed[gear]
		 * watchconn->obj->vehicle_auxdata->peak_power_rpm)
		< IDLE_RPM) {
		gaugeMsg.rpm = IDLE_RPM;
	    }
	}

	gaugeMsg.x_val = watchconn->obj->controls.pointer_x;
	gaugeMsg.y_val = watchconn->obj->controls.pointer_y;
	switch( current_course->turbo_mode)
	{
	   case TURBO_NONE:  /* Turbo Always off */
	    gaugeMsg.num_turbos =  -1;
	    break;
	   case TURBO_UNLIMITED:  /* Turbo Always on */
	    gaugeMsg.num_turbos =  100;
	    break;
	   case TURBO_LIMITED:  /* Limited Turbo */
	    gaugeMsg.num_turbos = current_course->turbo_boosts - 
		watchconn->obj->turbo_boosts;
	    break;
	}
	if( server_mode & SERVER_MODE_EXPLOSION_DISABLED){
	    gaugeMsg.num_shells = -1;
	}
	else {
	    switch( current_course->guns_mode )
	    {
		case  GUNS_NONE:
		    gaugeMsg.num_shells = -1;
		    break;
		case  GUNS_UNLIMITED:
		    gaugeMsg.num_shells = 100;
		    break;
		case  GUNS_LIMITED:
		    gaugeMsg.num_shells = current_course->ammo - 
			watchconn->obj->ammo_used;
		    break;
	    }
	}
    }
    else
    {
	gaugeMsg.mph = 0.0;
	gaugeMsg.altitude = 0.0;
	gaugeMsg.rpm = 0.0;
	gaugeMsg.x_val = 0.0;
	gaugeMsg.y_val = 0.0;
	gaugeMsg.num_turbos = 0;
	gaugeMsg.num_shells = 0;
    }

    PutMsg( (struct IPCMsg *)&gaugeMsg, sendconn->socket->socketnum);

    /* Copy in the global nameset stuff */

    namesetMsg.MsgType = SRV_FRAME_GLOBAL_NAMESET;
    namesetMsg.MsgLength = SRV_FRAME_GLOBAL_NAMESET_SIZE;

    if (server_mode & SERVER_MODE_EXPLOSION_DISABLED) {
	namesetMsg.global_nameset_bits = 0;
    }
    else {
	namesetMsg.global_nameset_bits = CAR_GUN_MASK;
    }
    {
	int which;
#ifdef WIN32
	struct _timeb val;

	_ftime(&val);
	which = ((val.time % 4) * 4) + (val.millitm / 250);
#else
	struct timeval tm;
	struct timezone tz;

	gettimeofday(&tm,&tz);
	which = ((tm.tv_sec % 4) * 4) + (tm.tv_usec / 250000);
#endif
	current_nameset_bit = TIMED_NAMESET(which);
	namesetMsg.global_nameset_bits |= current_nameset_bit;
    }

    PutMsg( (struct IPCMsg *)&namesetMsg, sendconn->socket->socketnum);

    /* ### FOR NOW, DO ALL 9 surrounding SCENES ### */
    scenes = SCENE_ALL;
	
    patch_x = myscene->xscene * SCENE_SIZE;
    patch_z = myscene->zscene * SCENE_SIZE;
	
    if ( watchconn && watchconn->obj )
    {
	car_x = watchconn->obj->xform[3][0];
	car_z = watchconn->obj->xform[3][2];
    }
    else
    {
	car_x = 0.0;
	car_z = 0.0;
    }
	
    if( car_x - patch_x < 0.0)
	scenes &= ~(SCENE_UP_RIGHT | SCENE_RIGHT | SCENE_DOWN_RIGHT);
    else
	scenes &= ~(SCENE_UP_LEFT | SCENE_LEFT | SCENE_DOWN_LEFT);
	
    if( car_z - patch_z < 0.0)
	scenes &= ~(SCENE_UP_LEFT | SCENE_UP | SCENE_UP_RIGHT);
    else
	scenes &= ~(SCENE_DOWN_LEFT | SCENE_DOWN | SCENE_DOWN_RIGHT);
	
    for (scene_no = 0; scene_no < 9; scene_no++ )  {
	scene_mask = 1 << scene_no;
	    
	if( ! (scene_mask & scenes) )
	    continue;
	    
	/* If we make it this far, it means that this client needs to
	 * receive this scene */
	switch( scene_mask )
	{
	  case SCENE_CENTER:
	    scene = myscene;
	    break;
	  case SCENE_UP:
	    scene = myscene->up;
	    break;
	  case SCENE_UP_LEFT:
	    if (myscene->up)        scene = myscene->up->left;
	    else if (myscene->left) scene = myscene->left->up;
	    else scene = NULL;
	    break;
	  case SCENE_UP_RIGHT:
	    if (myscene->up)         scene = myscene->up->right;
	    else if (myscene->right) scene = myscene->right->up;
	    else scene = NULL;
	    break;
	  case SCENE_RIGHT:
	    scene = myscene->right;
	    break;
	  case SCENE_DOWN:
	    scene = myscene->down;
	    break;
	  case SCENE_DOWN_LEFT:
	    if (myscene->down)      scene = myscene->down->left;
	    else if (myscene->left) scene = myscene->left->down;
	    else scene = NULL;
	    break;
	  case SCENE_DOWN_RIGHT:
	    if (myscene->down)       scene = myscene->down->right;
	    else if (myscene->right) scene = myscene->right->down;
	    else scene = NULL;
	    break;
	  case SCENE_LEFT	:
	      scene = myscene->left;
	    break;
	}
	    
	/* scene not created yet */
	if( scene == NULL)
	    continue;
	    
	scene->count = 0;
	    
	if (scene->static_seg == INVALID) {
	    if(scene->object_head == NULL)
		continue;
	    else
		create_scene_mongo_dl(scene,img_fildes);
	}

	transmit_segment( -1, sendconn, scene->static_seg );

	staticMsg.MsgType = SRV_FRAME_STATIC_SEG;
	staticMsg.MsgLength = SRV_FRAME_STATIC_SEG_SIZE;
	staticMsg.segment =  scene->static_seg;

	PutMsg( (struct IPCMsg *)&staticMsg, sendconn->socket->socketnum);
	    
	/* Traverse the list of objects and write matrix messages into
	 * the send buffer.  If the object has a non-null UpdateDisp struct,
	 * send that BEFORE the matrix message.
	 */
	if ((obj = scene->object_head) != NULL)
	    send_dynamic_objects(obj, sendconn);
    }
    
#ifdef NEED_TO_DO_RADAR
    /* Copy in the radar message. */
    memcpy( send_ptr, radar_buf, radar_buf_size );
    send_ptr += radar_buf_size;

#endif
    
    beginMsg.MsgType = SRV_FRAME_END;
    beginMsg.MsgLength = 0;
    PutMsg( &beginMsg, sendconn->socket->socketnum);

    /* Flush out this frame to the client */
    FlushMsg(sendconn->socket->socketnum);

#if 0
    /* Finish off the buffer with an endframe message. */
    header       = (message_header_type *)send_ptr;
    header->type = FRAME_END;
    header->data.size = 0;
    send_ptr += sizeof( message_header_type );

#endif    
    
    sendconn->frame_count++;
	
    frames++;
}


/*****************************************************************
 * build_radar_message
 * 
 * 	Traverses connection list adding a radar_msg_type for
 *      each active client to a temporary radar message buffer.
 *      This buffer is later transmitted as part of a frame 
 *      message.
 */
static void build_radar_message(
    void)
{
    message_header_type *header;
    radar_msg_type *radar_msg;
    connection_type *cptr;
    
    /* Set up message header.  Fill in size later. */
    radar_ptr = radar_buf;
    header = (message_header_type *)radar_ptr;
    header->type = FRAME_RADAR;
    header->data.size = 0;
    radar_ptr += sizeof( message_header_type );
    
    /* Traverse the list of connections and write position and 
     * color information into the buffer.
     */
    cptr = connection_list;
    while ( cptr )
    {
	/* Skip watch mode clients. */
	if (cptr->mode == CON_WATCH_MODE)
	{
	    cptr = cptr->next;
	    continue;
	}

	/* Add only active clients to the list. */
	if ( cptr->initialized )
	{
	    /* Fill in next radar message. */
	    radar_msg = (radar_msg_type *)radar_ptr;
	    radar_msg->x_pos = cptr->obj->xform[3][0];
	    radar_msg->y_pos = cptr->obj->xform[3][1];
	    radar_msg->z_pos = cptr->obj->xform[3][2];
	    radar_msg->r = cptr->obj->upd->color[0];
	    radar_msg->g = cptr->obj->upd->color[1];
	    radar_msg->b = cptr->obj->upd->color[2];
	    radar_ptr += sizeof( radar_msg_type );
	    
	    /* Increment radar message count. */
	    header->data.size++;	
	}
	cptr = cptr->next;
    }
    /* Find the size of the whole thing. */
    radar_buf_size = (int)radar_ptr - (int)radar_buf;
}


/* display frames/sec */
static void check_frames(void)
{
    static unsigned long snapshot_frames = 0;
#ifdef WIN32
    static struct _timeb start_snapshot_time;
    struct _timeb stop_snapshot_time;
#else
    static struct timeval start_snapshot_time;
    struct timeval stop_snapshot_time;
#endif
    unsigned long snapshot_sec, snapshot_usec = 1;
    float fseconds;

    if ( snapshot_frames >= 25 )
    {
#ifdef WIN32
        _ftime( &stop_snapshot_time );
	if ( start_snapshot_time.millitm > stop_snapshot_time.millitm )
	{
	    stop_snapshot_time.millitm += 1000;
	    stop_snapshot_time.time--;
	}

	snapshot_sec = stop_snapshot_time.time
		     - start_snapshot_time.time;
	snapshot_usec = 1000 * (stop_snapshot_time.millitm
                               - start_snapshot_time.millitm);
#else
	gettimeofday( &stop_snapshot_time, &tz );
	if ( start_snapshot_time.tv_usec > stop_snapshot_time.tv_usec )
	{
	    stop_snapshot_time.tv_usec += 1000000;
	    stop_snapshot_time.tv_sec--;
	}

	snapshot_sec = stop_snapshot_time.tv_sec
		     - start_snapshot_time.tv_sec;
	snapshot_usec = stop_snapshot_time.tv_usec
		      - start_snapshot_time.tv_usec;
#endif

	fseconds = (float)snapshot_sec + (float)snapshot_usec / 1000000.0;

        {
            char msg[80];
            sprintf(msg, "rate: %5.2f",(float)snapshot_frames / fseconds);
            outputString(msg);
        }

	/* Reset frame counter. */
#ifdef WIN32
        _ftime( &start_snapshot_time );
#else
	gettimeofday( &start_snapshot_time, &tz );
#endif
	snapshot_frames = 0;
    }
    else
	snapshot_frames++;
}


static void reset_timer(
    void)
{
#ifndef WIN32
    /* Reset timer. */
    timer.it_value.tv_sec     = 0;
    timer.it_value.tv_usec    = SERVER_TSLICE;
    timer.it_interval.tv_sec  = 0;
    timer.it_interval.tv_usec = 0;
    setitimer( ITIMER_REAL, &timer, &oldtimer );
#endif
}


static time_value current_local_time(
    void)
{
    struct tm *t;	
    time_t now;

    now = time(NULL);
    t = localtime(&now);

    return((time_value)
	( (float) t->tm_hour
	+ (float) t->tm_min / 60.0
	+ (float) t->tm_sec / 3600.0));
}


/*****************************************************************
 * server_update
 * 
 * 	Update all clients.
 * 	
 * Inputs:
 *
 * Outputs:
 *		nothing.
 */
static void server_update(
    void)
{
    connection_type *cptr;
   
    if (updates == 0) {
	/* It may have been a while since we first initialized the clock! */
	if (server_mode & SERVER_MODE_NIGHT_DRIVING)
	    set_virtual_time_slider(FALSE,0.0,TRUE);
	else
	    set_virtual_time_slider(FALSE,current_local_time(),FALSE);
    }

    /* Increment update counter. */
    updates++;

    if ((updates % LOOP_PER_MINUTE) == 0) virtual_clock_tick();

    if (show_frames_per_sec) {
	check_frames();
    }

    reset_timer();
    
    /* Okay, Traverse each scene, updating the objects in scenes that have
     * been recently sent to a client. 
     */
    if ( !ignore_input )
	scene_update( T_INTERVAL );
    
    build_radar_message();

    /* Traverse the list of connections and update each one that
     * is ready.
     */
    cptr = connection_list;
    while ( cptr )
    {
	if ( cptr->frame_count < MAX_FRAMES )
	{
	    if ( (cptr->mode == CON_WATCH_MODE) &&
		 (cptr->watch) &&
		 (!(cptr->watch->ready)) )
	    {
		cptr = cptr->next;
		continue;
	    }
	    else if( (cptr->mode == CON_WATCH_MODE) &&(cptr->watch == NULL))
	    {
		/* Watch mode client has no one to watch -- try to switch to
		 * the next connection -- maybe someone just connected */
		server_watch_next( cptr );
		cptr = cptr->next;
		continue;
	    }

	    if ( cptr->ready ) {
		connection_update( cptr );
		/* Don't get another frame until client tells us to */
		cptr->ready = FALSE;
	    }
	}

	cptr = cptr->next;
    }
}

void reset_all_clients(
    void)
{
    struct IPCMsg *Msg = 0;
    connection_type *conn_ptr;
#ifdef WIN32
    struct _timeb _time;
    _ftime( &_time );
#else
    struct timeval _time;
    struct timezone _tz;

    gettimeofday(&_time,&_tz);
#endif
    conn_ptr = connection_list;

    while ( conn_ptr ) 
    {
	if( (conn_ptr->mode != CON_WATCH_MODE) && conn_ptr->initialized
		&& conn_ptr->obj )
	{
	    conn_ptr->obj->checkpoints = 0;
#ifdef WIN32
	    conn_ptr->obj->start_lapsec = _time.time;
	    conn_ptr->obj->start_lapusec = 1000 * _time.millitm;
#else
	    conn_ptr->obj->start_lapsec = _time.tv_sec;
	    conn_ptr->obj->start_lapusec = _time.tv_usec;
#endif
	    conn_ptr->obj->turbo_boosts = 0;
	    conn_ptr->obj->ammo_used = 0;
	    conn_ptr->obj->checkpoints = 0;
	    conn_ptr->obj->time_to_checkpt = 0.0;
	    server_restart_client(conn_ptr, Msg);
	}
	conn_ptr = conn_ptr->next;
    }

}


void update_all_clients_state(
    int client_state)
{
    connection_type *conn_ptr;

    conn_ptr = connection_list;
    while ( conn_ptr ) 
    {
	if(conn_ptr->initialized)  /* Only update client state if it is ready */
	{
	    update_client_state(conn_ptr,client_state);
	}
	conn_ptr = conn_ptr->next;
    }
}


static int find_last_checkpoint(
    connection_type *con)
{
    int checkpoints;
    int i;

    if ( con->mode == CON_WATCH_MODE )
	return 0;
    if (!con->obj)
        return 0;

    checkpoints = con->obj->checkpoints;

    i = 0;
    while( checkpoints & ( 1 << i) )
    {
      i++;
    }

    return(i);
}


void cause_explosion(
    SCENE *scene,
    float x, float y, float z,
    float radius, float base_force, float base_torque)
{
    connection_type *conn_ptr;
    struct SrvExplosion exp_msg;
    DRIVE_OBJECT *obj;

    exp_msg.MsgType = SRV_EXPLOSION;
    exp_msg.MsgLength = SRV_EXPLOSION_SIZE;

    exp_msg.x = x;
    exp_msg.y = y;
    exp_msg.z = z;
    exp_msg.radius = radius;

    /* Send explosion messages */
    conn_ptr = connection_list;
    while (conn_ptr) {
	/* Skip clients that are not ready */
	if (!(conn_ptr->initialized) ) {
	    conn_ptr = conn_ptr->next;
	    continue;
	}

	if (!conn_ptr->obj) {
	    conn_ptr = conn_ptr->next;
	    continue;
	}

	/* Skip if not in the general area */
	if ((ABS(x - conn_ptr->obj->xform[3][0]) > SCENE_SIZE)
		|| (ABS(z - conn_ptr->obj->xform[3][2]) > SCENE_SIZE)) {
	    conn_ptr = conn_ptr->next;
	    continue;
	}

	obj = conn_ptr->obj;

	PutMsg( (struct IPCMsg *)&exp_msg, conn_ptr->socket->socketnum);
	conn_ptr = conn_ptr->next;
    }


    /* Blow things up *real* good */
    /* Cover the "DISABLED" case for land mines placed in the scene. */
    if (server_mode
	    & (SERVER_MODE_EXPLOSION_FORCE|SERVER_MODE_EXPLOSION_DISABLED)) {
	float dx,dy,dz,dist;
	float Force[4],Torque[4];

	obj = scene->object_head;
	while (obj != NULL) {
	    if ((!(obj->idptr->flags & OBJECTCLASS_DYNAMIC))
		    || (obj->pobj == NULL)) {
		obj = obj->next;
		continue;
	    }

	    dx = obj->xform[3][0] - x;
	    if ((dy = obj->xform[3][1] - y + 2.0) < 0.0) dy = FLOATRAND(0.5);
	    dz = obj->xform[3][2] - z;

	    if ((dist = HYPOT3(dx,dy,dz)) < radius) {
		/* Throw the vehicle for a loop */
/* NOTE!!! XFIGHTER_OBJECT MUST BE SAME AS IN objects/obj_common.h!!! */
#define	XFIGHTER_OBJECT	66
		if( obj->idptr->number == XFIGHTER_OBJECT ) {
		    /* X fighter has no torque; therefore, change */
		    /* its orientation. */
		    PHYSICAL_OBJECT
			*pobj = obj->pobj;

		    /* dx is pitch, dy is yaw, dz is roll... */
		    dx = BOUNDED_FLOATRAND( -M_PI, M_PI );
		    dy = BOUNDED_FLOATRAND( -M_PI, M_PI );
		    dz = BOUNDED_FLOATRAND( -M_PI, M_PI );

		    pobj->w[1] = dx*obj->xform[0][0]
				+ dy*obj->xform[1][0]
				+ dz*obj->xform[2][0];
		    pobj->w[2] = dx*obj->xform[0][1]
				+ dy*obj->xform[1][1]
				+ dz*obj->xform[2][1];
		    pobj->w[3] = dx*obj->xform[0][2]
				+ dy*obj->xform[1][2]
				+ dz*obj->xform[2][2];

		    /* dist is speed in feet/second */
		    dist = BOUNDED_FLOATRAND( 50.0, MPH_TO_FPS(400.0) );

		    pobj->v[1] = pobj->R[3][1] * dist;
		    pobj->v[2] = pobj->R[3][2] * dist;
		    pobj->v[3] = pobj->R[3][3] * dist;

		    pobj->p[1] = pobj->v[1] * pobj->mass;
		    pobj->p[2] = pobj->v[2] * pobj->mass;
		    pobj->p[3] = pobj->v[3] * pobj->mass;

		    Force[0] = Force[1] = Force[1] = 0.0;
		    Torque[0] = Torque[1] = Torque[2] = 0.0;
		}
		else {
		    /* Normal vehicle; torque it about... */
		    while (dist < EPSILON) {
			dx = BOUNDED_FLOATRAND(-1.0,1.0);
			dy = BOUNDED_FLOATRAND( 0.0,1.0);
			dz = BOUNDED_FLOATRAND(-1.0,1.0);
			dist = HYPOT3(dx,dy,dz);
		    }
		    Force[1] = (radius - dist)/radius * base_force * dx / dist;
		    Force[2] = (radius - dist)/radius * base_force * dy / dist;
		    Force[3] = (radius - dist)/radius * base_force * dz / dist;
		    Torque[1] = BOUNDED_FLOATRAND(-1.0,1.0) * base_torque;
		    Torque[2] = BOUNDED_FLOATRAND(-1.0,1.0) * base_torque;
		    Torque[3] = BOUNDED_FLOATRAND(-1.0,1.0) * base_torque;
		}

		simple_compute_state(obj->pobj,Force,Torque,0.0,
		    EXPLOSION_TIME_INTERVAL);

		/* copy updated R and x to xform */
		update_xforms(obj,obj->pobj);

		/* Update the WC bounding box for this car. */
		update_wc_bounds(obj);

		/* Make sure I haven't crossed scenes */
		obj->scene = (void *) check_scenes(obj,
		    obj->xform[3][0],obj->xform[3][2]);
	    }
	    obj = obj->next;
	}
    }
    else if (server_mode & SERVER_MODE_EXPLOSION_RESTART) {
	float dx,dy,dz;

	obj = scene->object_head;
	while (obj != NULL) {
	    if ((!(obj->idptr->flags & OBJECTCLASS_DYNAMIC))
		    || (obj->connection == NULL)) {
		obj = obj->next;
		continue;
	    }

	    dx = obj->xform[3][0] - x;
	    dy = obj->xform[3][1] - y;
	    dz = obj->xform[3][2] - z;
	    if (dx*dx + dy*dy + dz*dz < (radius*radius)) {
		/* Restart them */
		server_restart_client(
		    (connection_type *) (obj->connection), NULL);
	    }
	    obj = obj->next;
	}
    }
}


void change_virtual_time(
    boolean_type time_frozen,
    time_value virtual_time)
{
#ifdef VIRTUAL_TIME_WORKING
    connection_type *conn_ptr;
    struct SrvVirtualTime vt_msg;

    vt_msg.MsgType = SRV_VIRTUAL_TIME;
    vt_msg.MsgLength = SRV_VIRTUAL_TIME_SIZE;

    vt_msg.static_time    = time_frozen;
    vt_msg.reference_time = time_value_to_secs(virtual_time);

    /* Send messages to all connected clients */
    conn_ptr = connection_list;
    while (conn_ptr) {
	/* Skip clients that are not ready */
	if (!(conn_ptr->initialized) ) {
	    conn_ptr = conn_ptr->next;
	    continue;
	}

	PutMsg((struct IPCMsg *)&vt_msg, conn_ptr->socket->socketnum);
	conn_ptr = conn_ptr->next;
    }
#endif /* VIRTUAL_TIME_WORKING */
}


static int compare_places(
    const void *e1, 
    const void *e2)
{
    PLACE_INFO *ele1 = (PLACE_INFO *) e1;
    PLACE_INFO *ele2 = (PLACE_INFO *) e2;

    if (ele1->last_checkpoint > ele2->last_checkpoint) return(-1);
    else if (ele1->last_checkpoint < ele2->last_checkpoint) return(1);
    /* else must be equal last_checkpoint */
    else if (ele1->time_to_last_checkpoint < ele2->time_to_last_checkpoint)
	return(-1);
    else if (ele1->time_to_last_checkpoint > ele2->time_to_last_checkpoint)
	return(1);
    else return(0);
}


void update_leader_board(
    int standings_type)
{
    connection_type *conn_ptr;
    int i;
    boolean_type must_update;
    int players=0;
    PLACE_INFO PI[MAX_PLAYERS];
    static PLACE_INFO last_PI[MAX_PLAYERS];
    static int last_type = INVALID;
    struct SrvStanding st_msg;


    /* Initialize */
    if (last_type == INVALID) {
	for (i=0; i<MAX_PLAYERS; ++i) last_PI[i].con = (void *) INVALID;
    }

    conn_ptr = connection_list;
    while (conn_ptr) {
	/* Skip watch mode clients, and clients that are still choosing a
	 * vehicle and so have no object yet.
	 */
	if (conn_ptr->mode == CON_WATCH_MODE || !conn_ptr->obj) {
	    conn_ptr = conn_ptr->next;
	    continue;
	}

	PI[players].last_checkpoint = find_last_checkpoint(conn_ptr);
	PI[players].time_to_last_checkpoint = conn_ptr->obj->time_to_checkpt;
	PI[players].con = conn_ptr;
	conn_ptr = conn_ptr->next;
	players++;
    }

    /* Now that we have all the scores for each player, sort the PI list */
    qsort(PI, players, sizeof(PLACE_INFO), compare_places);

    /* See if anyone's changed positions */
    if (standings_type == STANDINGS_FINAL) {
	st_msg.flags = STANDINGS_TYPE_FINAL;
	must_update = TRUE;
    }
    else {
	st_msg.flags = STANDINGS_TYPE_CURRENT;
	must_update = FALSE;
	for (i=0; i<players; ++i) {
	    if (PI[i].con != last_PI[i].con) {
		must_update = TRUE;
		break;
	    }
	}
    }

    /* Setup constants */
    st_msg.MsgType = SRV_STANDING;
    st_msg.MsgLength = SRV_STANDING_SIZE;

    /* If so, tell everyone. */
    if (must_update) {
	for (i=0; i<players; i++) {
	    if (standings_type == STANDINGS_FINAL) {
		/* Winner starts in last position next time */
		starting_positions[players - i - 1] = (PI[i].con);
	    }
	    else {
		/* skip unchanged positions */
		if (PI[i].con == last_PI[i].con) continue;
	    }

	    sprintf(st_msg.user,"%s@%s",
		 (PI[i].con)->username,(PI[i].con)->hostname);
	    st_msg.position = i+1;
	    st_msg.r = (PI[i].con)->obj->upd->color[0];
	    st_msg.g = (PI[i].con)->obj->upd->color[1];
	    st_msg.b = (PI[i].con)->obj->upd->color[2];

	    conn_ptr = connection_list;
	    while (conn_ptr) {
		/* Skip clients that are not ready */
		if (!(conn_ptr->initialized)) {
		    conn_ptr = conn_ptr->next;
		    continue;
		}

		if (conn_ptr == PI[i].con) {
		    st_msg.flags |= STANDINGS_TYPE_PERSONAL;
		}
		else {
		    st_msg.flags &= ~STANDINGS_TYPE_PERSONAL;
		}

		PutMsg( (struct IPCMsg *)&st_msg, conn_ptr->socket->socketnum);
		conn_ptr = conn_ptr->next;
	    }
	}

	memcpy(last_PI,PI,sizeof(PLACE_INFO)*MAX_PLAYERS);
    }

    if (must_update || (last_type != standings_type)) {
	last_type = standings_type;

	/* Now, send one last STANDING message indicating that
	 * they have all been sent.
	 */
	st_msg.user[0] = '\0';
	st_msg.position = players;
	st_msg.flags &= ~STANDINGS_TYPE_PERSONAL;

	conn_ptr = connection_list;
	while (conn_ptr) {
	    /* Skip clients that are not ready. */
	    if (!(conn_ptr->initialized)) {
		conn_ptr = conn_ptr->next;
		continue;
	    }
	    PutMsg( (struct IPCMsg *)&st_msg, conn_ptr->socket->socketnum);
	    conn_ptr = conn_ptr->next;
	}
    }
}


/* this routine should shutdown all the clients, then exit gracefully */
void shutdown_server(
    void)
{
#if defined(WIN32) || defined(MAC)
    tgTerm();
#endif
    exit(1);
}


static void handle_user_signal(
    int sig)
{
#ifndef WIN32
    printf("Caught a user signal! \n");
    next_server_state();

    /* Now, reset the signal */
    signal(SIGUSR1, handle_user_signal);
#endif
}


static void initialize_globals(
    void)
{
    int i;


    server_mode = DEFAULT_SERVER_MODE;

    course = &default_course;
    current_course = course;

    for (i=0;i<MAX_PLAYERS;i++) {
	starting_positions[i] = (connection_type *)NULL;
    }
}



/*****************************************************************
 * main
 * 
 */
int main(
    int argc,
    char **argv)
{
    boolean_type done = FALSE;
    connection_type *connection_ptr;
    int send_buf_size;
    fd_set server_mask;
    int status;
    int query_socket;
    int n;
    double prevtm, currtm;
    hwDisplay hw_disp;
    hwObject texture;

    check_already_running();
    construct_filenames(argv[0]);

    init_physics();

    sockList = malloc(numSocks * sizeof(int));
    if (!sockList) {
        exit(1);
    }
    for (n = 0; n < numSocks; n++) {
        sockList[n] = IPC_INVALID_SOCK;
    }
    playerList = malloc(numSocks * sizeof(void *));
    if (!playerList) {
        exit(1);
    }
    for (n = 0; n < numSocks; n++) {
        playerList[n] = NULL;
    }

    initialize_globals();
    read_config_file();

    /* Read any command line arguments (debug, display frame rate, etc) */
    if ( !scanargs( argc, argv, "%   D%- F%- T%- ", &debug,
	&show_frames_per_sec, &do_textures  ) )
	exit( -1 );

#ifndef WIN32
    if (server_mode & SERVER_MODE_DEMO_INTERRUPT) {
	/* We are going to want to switch from state to state when we get
	 * hit by a signal (SIGUSR1), so load up a signal handler */

	 signal(SIGUSR1, handle_user_signal);
	 sigblock( sigmask(SIGUSR1) );
    }
#endif

    if (!hwInit(argc, argv)) exit(1);
    hw_disp = hwNullDisplay->create( hwNullDisplay, NULL, NULL );
    hw_disp->makeCurrent(hw_disp, NULL);

    if( do_textures ) {
	/* Create the texture array */
	texture = hwTexture->create( hwTexture );
	texture->name = "RoadTexture";
	texture->modify(texture, hwStrFileName, HW_TYPE_STRING, 
		"textures/Asphalt.jpg");
	HW_MODIFY_1I(texture, hwStrCoordMode, HW_TM_PLANAR);
	HW_MODIFY_3F(texture, hwStrRotate, 90, 0, 0);
	HW_MODIFY_3F(texture, hwStrScale, 20, 20, 20 );

	hwRegisterObject( texture );
	textureArray[numTextures++] = texture;

	texture = hwTexture->create( hwTexture );
	texture->name = "GrassTexture";
	texture->modify(texture, hwStrFileName, HW_TYPE_STRING, 
		"textures/Grass.jpg");
	HW_MODIFY_1I(texture, hwStrCoordMode, HW_TM_PLANAR);
	HW_MODIFY_3F(texture, hwStrRotate, 90, 0, 0);
	HW_MODIFY_3F(texture, hwStrScale, 20, 20, 20 );

	hwRegisterObject( texture );
	textureArray[numTextures++] = texture;

	texture = hwTexture->create( hwTexture );
	texture->name = "ConcreteTexture";
	texture->modify(texture, hwStrFileName, HW_TYPE_STRING, 
		"textures/Concrete.jpg");
	HW_MODIFY_1I(texture, hwStrCoordMode, HW_TM_PLANAR);
	HW_MODIFY_3F(texture, hwStrRotate, 90, 0, 0);
	HW_MODIFY_3F(texture, hwStrScale, 20, 20, 20 );

	hwRegisterObject( texture );
	textureArray[numTextures++] = texture;

	/* Now, create the HW segment */
	textureSeg = createHwSegmentFromObj( textureArray, numTextures );
    }

    send_buf_size =  (MAX_OBJECTS + MAX_PLAYERS)
	       * sizeof(matrix_msg_type) + (MAX_OBJECTS + MAX_PLAYERS + 2)
	       * sizeof(message_header_type) + sizeof(position_msg_type)
               + sizeof(message_header_type) + sizeof(gauge_msg_type);

    /*
   if(debug) printf("Send Buffer size = %d\n",send_buf_size);
    */

    /* Malloc room for the packet send buffers. */
    send_buf = (char *)malloc( send_buf_size);
    if (send_buf == NULL )
    {
	fprintf(stderr,"Server: Out of malloc space!\n");
	exit( -1 );
    }	
    radar_buf = (char *)malloc(sizeof(message_header_type)
			       + MAX_PLAYERS
			       * sizeof(radar_msg_type));
    if (radar_buf == NULL )
    {
	fprintf(stderr,"Server: Out of malloc space!\n");
	exit( -1 );
    }	
    
    {
    IPC_SOCK Sock;

    if (!PrepareForIPC()) {
        exit(1);
    }

    do {
	Sock = InitIPC( 0, "drive" );
	if( Sock == IPC_INVALID_SOCK ) {
	    Sock = InitIPC( 0, DEFAULT_SOCKET_STRING );
	}
	if( Sock == IPC_INVALID_SOCK) {
	    (void)printf(
		    "Unable to create port; retrying after 10 seconds\n" );
	    (void)fflush( stdout );
#ifdef WIN32
	    Sleep( 10 * 1000 );
#else
	    (void)sleep( 10 );
#endif
	}
    } while( Sock == IPC_INVALID_SOCK );

    listen_socket = (socket_type *)malloc(sizeof(socket_type));
    listen_socket->socketnum = (int)Sock;

    query_socket = SetupQuerySocket( "drivequery" );
    if (query_socket == IPC_INVALID_SOCK) {
        query_socket = SetupQuerySocket( "22748" );
    }

    }

#if 0
    /* gopen() a display list fildes to be used in creating the display
     * list segment ... the fildes will be the global img_fildes.
     */
    gopen_display_list();
#endif

    /* Construct filename path, Initialize object list, read in scene */
    object_initialization();
    read_scenefiles();

    /* Set up signal handler and initialize timer. */
#ifndef WIN32
    (void)signal( SIGALRM, SIG_IGN );
#endif

    prevtm = currtm = CurrentTime = StartTime = SrvGetTime();

    server_update();

    /* Initialize virtual time-of-day */
    if (server_mode & SERVER_MODE_NIGHT_DRIVING)
	set_virtual_time_slider(TRUE,0.0,TRUE);
    else
	set_virtual_time_slider(TRUE,current_local_time(),FALSE);

/* HACK! */
server_mode |= SERVER_MODE_DEMO;

    if (server_mode & SERVER_MODE_DEMO) {
	open_server_interface(argc,argv);
    }

    /* DRIVE_PRACTICE_MODE: sit in Practice forever, so clients can just
     * drive.  CLOCK_FROZEN stops the state clock from ever expiring.
     */
    if (getenv("DRIVE_PRACTICE_MODE") != NULL) {
	set_practice_mode();
    }

    /* Main server select loop.  Watches for new connections and activity
     * on current connections.
     */
    while ( !done )
    {
        n = IPCServer(listen_socket->socketnum, query_socket, SERVER_TSLICE,
                      sockList, numSocks);
        if (n < 0) {
            /* Some error of some kind */
        }
        else if (n) {
            /* A new connection */
            Connected = 1;
            Connects++;
            server_connect(n-1);
        }

        currtm = SrvGetTime();
        if ((currtm - prevtm) >= (SERVER_TSLICE / 1000000.0)) {
	    if (server_mode & (SERVER_MODE_DEMO|SERVER_MODE_DEMO_INTERRUPT)) {
		if ((server_state == RACE_STATE)
			|| (server_state == PRACTICE_STATE)
			|| (server_state == PRE_RACE_STATE)) {
		    server_update();
		}
		else {
		    reset_timer();
		}

		if (loop_timer != CLOCK_FROZEN) {
		    if ((--loop_timer) <= 0) {
			loop_timer_done();
		    }
		    else if ((loop_timer % LOOP_PER_SECOND) == 0) {
			state_clock_tick();
		    }
		}
	    }
	    else {
		server_update();
	    }
            prevtm = currtm;
        }

        if (Connects) {
            for (n = 0; n < numSocks; n++) {
                if (sockList[n] == IPC_INVALID_SOCK) continue;
                if (playerList[n]) {
                    done = server_read( playerList[n] );
                }
            }
        }

	if (server_mode & SERVER_MODE_DEMO)
	    process_X_events();

#ifndef WIN32
	if (server_mode & SERVER_MODE_DEMO_INTERRUPT) {
	    if ( sigpending(&signals) == -1 )
	    {
	       printf(" sigpending call failed! \n");
	    }
#if HANDLE_SIGUSR1
	    else /* Lets see if we have a pending SIGUSR1 signal */
	    {
	       int i;
	       sigset_t osignals;
	       for(i=0;i<8;i++)
	       {
		   if( signals.sigset[i] & sigmask(SIGUSR1) )
		   {
		      printf("signals.sigset[%1d] = %d\n",i,signals.sigset[i]);
		      sigprocmask(SIG_UNBLOCK,&signals,&osignals);
		      _hp_high_res_sleep(0.2);
	 	      sigblock( sigmask(SIGUSR1) );

		   }
	       }
	    }
#endif
	}
#endif
    }

    close( listen_socket->socketnum );
    fprintf( stderr, "Server terminating.\n" );
    return(0);
}

double SrvGetTime(void)
{
#ifdef WIN32
    struct _timeb val;

    _ftime(&val);
    return (double) val.time + ((double) val.millitm) / 1000.0;
#else
    struct timeval val;

    gettimeofday(&val,NULL);
    return (double) val.tv_sec + ((double) val.tv_usec) / 1000000.0;
#endif
}

/* All of the routines in this #ifdef are hack stubs for the interim
 * Hoverware port.  All of them, that is, except for _hp_high_res_sleep -
 * it needs to be implemented.  We implement it using the fun select
 * semantics.  Right here:
 */
void _hp_high_res_sleep( double t )
{
    struct timeval tm;

    tm.tv_sec = (int)t;
    tm.tv_usec = (int)(t * 1000000.0);
    (void)select( 0, 0, 0, 0, &tm );
}

/* Oops, _hp_invert needs to be here also. */
void _hp_invert( float a[4][4], float b[4][4], int n )
{
    hwInvertMat( a, b );
}

/* And concat_matrix */
void concat_matrix( float m1[4][4], float m2[4][4], float m3[4][4] )
{
    float
	t1[4][4],
	t2[4][4];

    (void)memcpy( t1, m1, 4*4*sizeof(float) );
    (void)memcpy( t2, m2, 4*4*sizeof(float) );
    hwMatMult( m3, t1, t2 );
}

/* And _hp_identity, Ross you idiot */
void _hp_identity( float mat[4][4] )
{
    hwIdentity( mat );
}

#if 1 /* [ SB HACK! */
/* Received: from hpgtdadm.fc.hp.com by hpross.fc.hp.com with SMTP
 *	(1.38.193.4/15.5+IOS 3.22) id AA08401; Wed, 1 Nov 1995 14:58:04 -0700
 * Return-Path: <cunniff@hpross.fc.hp.com>
 * Received: from hpross.fc.hp.com by hpgtdadm.fc.hp.com with SMTP
 *	(1.37.109.16/15.5+IOS 3.22) id AA157143078; Wed, 1 Nov 1995 14:57:58 -0700
 * Received: by hpross.fc.hp.com
 *	(1.38.193.4/15.5+IOS 3.22) id AA08398; Wed, 1 Nov 1995 14:57:56 -0700
 * Date: Wed, 1 Nov 1995 14:57:56 -0700
 * From: Ross Cunniff <cunniff@hpross.fc.hp.com>
 * Message-Id: <9511012157.AA08398@hpross.fc.hp.com>
 * To: space_team@hpross.fc.hp.com
 * Subject: Starbase to graphed translator...
 *
 * Gang,
 *
 * Here is a file containing lotsa utility routines I wrote (or stole from
 * drive) which can convert a simple Starbase object to graphed/SPACE
 * format.  Just link it in, create a main program which puts the
 * GraphicClass foo { } wrapper around the call, chmod +x it ('cause
 * of all the undefined externals), and run it.  If it doesn't core
 * dump, you should get a GraphicClass object on stdout.  I'll
 * probably keep tweaking this.  It'll be one way to get, say,
 * an AutoCad DXF -> SPACE object translator...
 *
 *
 * Ross
 */
/* $Source: /cvsroot/hoverball/HB_HW/DRIVE/server/drive_server.c,v $
 * $Revision: 1.16 $
 * $Date: 2008/01/16 04:42:29 $
 *
 * Copyright 1991 Daryl Poe
 *
 */

#include "prims.h"

#undef fill_color
#undef line_color
#undef surface_model
#undef vertex_format
#undef quadrilateral_mesh
#undef triangular_strip
#undef partial_polygon3d
#undef polygon3d

static int
    WPV = 3,
    SurfBright = 0,
    SurfShiny = 0,
    HasNormals = 0,
    FacetNormal = 0,
    HasRGB = 0,
    TwoSided = 0,
    Backfacing = 0,
    AnimBits = 0xFFFFFFFF,
    MatPtr = 0;
static float
    MatStack[20][4][4] = {
	{{1.,0.,0.,0.},{0.,1.,0.,0.},{0.,0.,1.,0.},{0.,0.,0.,1.}},
	{{1.,0.,0.,0.},{0.,1.,0.,0.},{0.,0.,1.,0.},{0.,0.,0.,1.}},
	{{1.,0.,0.,0.},{0.,1.,0.,0.},{0.,0.,1.,0.},{0.,0.,0.,1.}},
	{{1.,0.,0.,0.},{0.,1.,0.,0.},{0.,0.,1.,0.},{0.,0.,0.,1.}},
	{{1.,0.,0.,0.},{0.,1.,0.,0.},{0.,0.,1.,0.},{0.,0.,0.,1.}},
	{{1.,0.,0.,0.},{0.,1.,0.,0.},{0.,0.,1.,0.},{0.,0.,0.,1.}},
	{{1.,0.,0.,0.},{0.,1.,0.,0.},{0.,0.,1.,0.},{0.,0.,0.,1.}},
	{{1.,0.,0.,0.},{0.,1.,0.,0.},{0.,0.,1.,0.},{0.,0.,0.,1.}},
	{{1.,0.,0.,0.},{0.,1.,0.,0.},{0.,0.,1.,0.},{0.,0.,0.,1.}},
	{{1.,0.,0.,0.},{0.,1.,0.,0.},{0.,0.,1.,0.},{0.,0.,0.,1.}},
	{{1.,0.,0.,0.},{0.,1.,0.,0.},{0.,0.,1.,0.},{0.,0.,0.,1.}},
	{{1.,0.,0.,0.},{0.,1.,0.,0.},{0.,0.,1.,0.},{0.,0.,0.,1.}},
	{{1.,0.,0.,0.},{0.,1.,0.,0.},{0.,0.,1.,0.},{0.,0.,0.,1.}},
	{{1.,0.,0.,0.},{0.,1.,0.,0.},{0.,0.,1.,0.},{0.,0.,0.,1.}},
	{{1.,0.,0.,0.},{0.,1.,0.,0.},{0.,0.,1.,0.},{0.,0.,0.,1.}},
	{{1.,0.,0.,0.},{0.,1.,0.,0.},{0.,0.,1.,0.},{0.,0.,0.,1.}},
	{{1.,0.,0.,0.},{0.,1.,0.,0.},{0.,0.,1.,0.},{0.,0.,0.,1.}},
	{{1.,0.,0.,0.},{0.,1.,0.,0.},{0.,0.,1.,0.},{0.,0.,0.,1.}},
	{{1.,0.,0.,0.},{0.,1.,0.,0.},{0.,0.,1.,0.},{0.,0.,0.,1.}},
	{{1.,0.,0.,0.},{0.,1.,0.,0.},{0.,0.,1.,0.},{0.,0.,0.,1.}},
    },
    GPosition[3] = {0.,0.,0.},
    Rotation[3] = {0.,0.,0.},
    Scaling[3] = {1.,1.,1.},
    FillColor[3] = {1.,1.,1.}, LineColor[3] = {1.,1.,1.};

void push_vertex_format();
void pop_vertex_format();
void xform_points();
void construct_orientation_matrix();

void add_names_to_set( int fildes, int n, int *bits )
{
    AnimBits = bits[0];
}

void remove_all_names_from_set( int fildes )
{
    AnimBits = 0xFFFFFFFF;
}

void self_lit_on( int fildes )
{
    SurfBright = 1;
}

void self_lit_off( int fildes )
{
    SurfBright = 0;
}

void Transform( float Mat[4][4], float Vect[3] )
{
    float
	a, b, c;

    a = Vect[0]; b = Vect[1]; c = Vect[2];
    Vect[0] = a*Mat[0][0] + b*Mat[1][0] + c*Mat[2][0] + Mat[3][0];
    Vect[1] = a*Mat[0][1] + b*Mat[1][1] + c*Mat[2][1] + Mat[3][1];
    Vect[2] = a*Mat[0][2] + b*Mat[1][2] + c*Mat[2][2] + Mat[3][2];
}

static int InvertMat( float Mat[4][4], float Dst[4][4] )
{
#define	MO	4
    float
	Tmp,
	Src[MO][MO];
    int
	i, j, k;

    /* Copy matrix (since the original is destroyed) */
    (void)memcpy( Src, Mat, MO*MO*sizeof(float) );

    /* Create identity matrix */
    for( i = 0; i < MO; i++ ) {
	for( j = 0; j < MO; j++ ) {
	    Dst[i][j] = (i == j) ? 1.0 : 0.0;
	}
    }

    /* Solve each row */
    for( i = 0; i < MO; i++ ) {
	Tmp = Src[i][i];
	if( Tmp < 0 )	Tmp = -Tmp;
	if( Tmp < EPSILON ) {
	    /* Oops - we need to swap some */
	    for( j = i+1; j < MO; j++ ) {
		Tmp = Src[j][i];
		if( Tmp < 0 )	Tmp = -Tmp;
		if( Tmp > EPSILON ) {
		    /* Found a valid row */
		    break;
		}
	    }
	    if( j >= MO ) {
		/* Matrix cannot be inverted */
		return 0;
	    }
	    /* Swap rows i and j */
	    for( k = 0; k < MO; k++ ) {
		Tmp = Src[i][k]; Src[i][k] = Src[j][k]; Src[j][k] = Tmp;
		Tmp = Dst[i][k]; Dst[i][k] = Dst[j][k]; Dst[j][k] = Tmp;
	    }
	}

	/* OK, now scale to 1.0 */
	Tmp = 1.0 / Src[i][i];
	Src[i][i] = 1.0;
	for( k = i+1; k < MO; k++ ) {
	    Src[i][k] *= Tmp;
	}
	for( k = 0; k < MO; k++ ) {
	    Dst[i][k] *= Tmp;
	}

	/* Now, transform all the rest of the rows so this column is 0 */
	for( j = 0; j < MO; j++ ) {
	    if( i == j )	continue;
	    Tmp = Src[j][i];
	    for( k = 0; k < MO; k++ ) {
		Src[j][k] -= Tmp * Src[i][k];
		Dst[j][k] -= Tmp * Dst[i][k];
	    }
	}
    }

    return 1;
#undef	MO
}

void RotateY( float Res[4][4], float Angle )
{
    float
	ca, sa;

    ca = cos( Angle );
    sa = sin( Angle );
    _hp_identity( Res );
    Res[0][0] = ca;
    Res[0][2] = sa;
    Res[2][0] = -sa;
    Res[2][2] = ca;
}


void RotateZ( float Res[4][4], float Angle )
{
    float
	ca, sa;

    ca = cos( Angle );
    sa = sin( Angle );
    _hp_identity( Res );
    Res[0][0] = ca;
    Res[0][1] = -sa;
    Res[1][0] = sa;
    Res[1][1] = ca;
}

int objPrinting = 0;

static void DeriveParms( void )
{
    float
	Tmp[4][4],
	ITmp[4][4],
	Vec[3],
	XVec[3], YVec[3],
	TMat[4][4], Mat[4][4], IMat[4][4];
    double
	dd;

if( !objPrinting ) return;

printf( "/* %g,%g,%g,  %g,%g,%g,  %g,%g,%g,  %g,%g,%g */\n",
	MatStack[MatPtr][0][0], MatStack[MatPtr][0][1], MatStack[MatPtr][0][2],
	MatStack[MatPtr][1][0], MatStack[MatPtr][1][1], MatStack[MatPtr][1][2],
	MatStack[MatPtr][2][0], MatStack[MatPtr][2][1], MatStack[MatPtr][2][2],
	MatStack[MatPtr][3][0], MatStack[MatPtr][3][1], MatStack[MatPtr][3][2]
);

    GPosition[0] = MatStack[MatPtr][3][0];
    GPosition[1] = MatStack[MatPtr][3][1];
    GPosition[2] = MatStack[MatPtr][3][2];

    /* Derive X scale */
    Vec[0] = 1.0; Vec[1] = 0.0; Vec[2] = 0.0;
    Transform( MatStack[MatPtr], Vec );
    Vec[0] -= GPosition[0]; Vec[1] -= GPosition[1]; Vec[2] -= GPosition[2];
    Scaling[0] = sqrt( Vec[0]*Vec[0] + Vec[1]*Vec[1] + Vec[2]*Vec[2] );
    if( fabs(Scaling[0]) < EPSILON )	Scaling[0] = EPSILON * 10;

    /* Derive Y scale */
    Vec[0] = 0.0; Vec[1] = 1.0; Vec[2] = 0.0;
    Transform( MatStack[MatPtr], Vec );
    Vec[0] -= GPosition[0]; Vec[1] -= GPosition[1]; Vec[2] -= GPosition[2];
    Scaling[1] = sqrt( Vec[0]*Vec[0] + Vec[1]*Vec[1] + Vec[2]*Vec[2] );
    if( fabs(Scaling[1]) < EPSILON )	Scaling[1] = EPSILON * 10;

    /* Derive Z scale */
    Vec[0] = 0.0; Vec[1] = 0.0; Vec[2] = 1.0;
    Transform( MatStack[MatPtr], Vec );
    Vec[0] -= GPosition[0]; Vec[1] -= GPosition[1]; Vec[2] -= GPosition[2];
    Scaling[2] = sqrt( Vec[0]*Vec[0] + Vec[1]*Vec[1] + Vec[2]*Vec[2] );
    if( fabs(Scaling[2]) < EPSILON )	Scaling[2] = EPSILON * 10;

    /* Remove scaling/positioning from matrix */
    _hp_identity( Tmp );
    Tmp[0][0] = Scaling[0]; Tmp[1][1] = Scaling[1]; Tmp[2][2] = Scaling[2];
    Tmp[3][0] = GPosition[0]; Tmp[3][1] = GPosition[1]; Tmp[3][2] = GPosition[2];
    if( !InvertMat( Tmp, ITmp ) ) {
	Scaling[0] = Scaling[1] = Scaling[2] = 1.0;
	Rotation[0] = Rotation[1] = Rotation[2] = 0.0;
printf( "/* ABORT! */\n" );
	return;
    }

    concat_matrix( MatStack[MatPtr], ITmp, Mat );

    /* See how X and Y unit direction vectors get xformed */
    XVec[0] = 1.; XVec[1] = 0.; XVec[2] = 0.;
    Transform( Mat, XVec );
    dd = sqrt(XVec[0]*XVec[0] + XVec[1]*XVec[1] + XVec[2]*XVec[2]);
    if( dd > 0.0 ) dd = 1.0 / dd;
    XVec[0] *= dd; XVec[1] *= dd; XVec[2] *= dd;

    YVec[0] = 0.; YVec[1] = 1.; YVec[2] = 0.;
    Transform( Mat, YVec );
    dd = sqrt(YVec[0]*YVec[0] + YVec[1]*YVec[1] + YVec[2]*YVec[2]);
    if( dd > 0.0 ) dd = 1.0 / dd;
    YVec[0] *= dd; YVec[1] *= dd; YVec[2] *= dd;

    /* Figure out Y rotation angle */
    Rotation[1] = XVec[2];
    if( Rotation[1] < -1. )	Rotation[1] = -1.;
    if( Rotation[1] > 1. )	Rotation[1] = 1.;
    Rotation[1] = -acos( Rotation[1] ) + M_PI / 2.0;

    /* Figure out Z rotation angle */
    Rotation[2] = -atan2( XVec[1], XVec[0] );

    /* Push YVec through inverse of new mat to see what X rot to do */
    Rotation[0] = 0.;
    RotateY( Tmp, Rotation[1] );
    RotateZ( ITmp, Rotation[2] );
    concat_matrix( Tmp, ITmp, TMat );

    if( InvertMat( TMat, IMat ) ) {
	Transform( IMat, YVec );
	Rotation[0] = -atan2( YVec[2], YVec[1] );
    }

    Rotation[0] *= 180.0 / 3.141592653589;
    Rotation[1] *= 180.0 / 3.141592653589;
    Rotation[2] *= 180.0 / 3.141592653589;
}

void concat_transformation3d( int fildes, float mat[4][4], int where, int pp )
{
    float
	Tmp[4][4];

    if( pp == PUSH ) {
	(void)memcpy( MatStack[MatPtr+1], MatStack[MatPtr], 4*4*sizeof(float) );
	MatPtr++;
    }
    if( where == PRE ) {
	concat_matrix( mat, MatStack[MatPtr], Tmp );
    }
    else {
	concat_matrix( MatStack[MatPtr], mat, Tmp );
    }
    (void)memcpy( MatStack[MatPtr], Tmp, 4*4*sizeof(float) );
    DeriveParms();
}

void pop_matrix( int fildes )
{
    if( MatPtr > 0 ) MatPtr--;
    DeriveParms();
}

int hidden_surface( int fildes, int hide, int cull )
{
    return(TwoSided = !cull);
}

void surface_model_d
(
    int fildes, int specular, int highlight,
    double red, double green, double blue
)
{
    SurfShiny = specular;
}

void surface_model_f
(
    int fildes, int specular, int highlight,
    float red, float green, float blue
)
{
    SurfShiny = specular;
}

void fill_color_d( int fildes, double red, double green, double blue )
{
    FillColor[0] = red;
    FillColor[1] = green;
    FillColor[2] = blue;
}

void line_color_d( int fildes, double red, double green, double blue )
{
    LineColor[0] = red;
    LineColor[1] = green;
    LineColor[2] = blue;
}

void fill_color_f( int fildes, float red, float green, float blue )
{
    FillColor[0] = red;
    FillColor[1] = green;
    FillColor[2] = blue;
}

void line_color_f( int fildes, float red, float green, float blue )
{
    LineColor[0] = red;
    LineColor[1] = green;
    LineColor[2] = blue;
}

void fill_color( int fildes, float red, float green, float blue )
{
    FillColor[0] = red;
    FillColor[1] = green;
    FillColor[2] = blue;
}

void line_color( int fildes, float red, float green, float blue )
{
    LineColor[0] = red;
    LineColor[1] = green;
    LineColor[2] = blue;
}

/* Starbase emulation... */
static int globCoord, globUse, globRgb, globNormals, globOrder;

void vertex_format( fildes, coord, use, rgb, normals, order )
int fildes;
int coord;
int use;
int rgb;
int normals;
int order;
{
    WPV = coord + 3;
    switch( use ) {
    case 0 :
    case 1 :
	HasNormals = HasRGB = 0;
	break;
    case 3 :
	if( rgb ) {
	    HasNormals = 0;
	    HasRGB = rgb;
	}
	else {
	    HasNormals = 3;
	    HasRGB = 0;
	}
	break;
    case 6 :
	if( rgb == 1 ) {
	    HasNormals = 6;
	    HasRGB = 3;
	}
	else if( rgb == 4 ) {
	    HasNormals = 3;
	    HasRGB = 6;
	}
	break;
    }
    FacetNormal = normals;
    Backfacing = !(order & COUNTER_CLOCKWISE);

    globCoord = coord;
    globUse = use;
    globRgb = rgb;
    globNormals = normals;
    globOrder = order;
}

void inquire_vertex_format( int gfd, int *coord, int *use, int *rgb, int *normals, int *order )
{
    *coord = globCoord;
    *use = globUse;
    *rgb = globRgb;
    *normals = globNormals;
    *order = globOrder;
}

void quadrilateral_mesh( fildes, clist, numverts_m, numverts_n, gnormals )
int fildes;
float *clist;
int numverts_m;
int numverts_n;
float *gnormals;
{
    int
	i, j;

if( !objPrinting ) return;

    printf( "    GraphType = \"Mesh\"\n" );
    if( HasNormals )	printf( "    HasNormals = True\n" );
    if( HasRGB )	printf( "    HasRGB = True\n" );
    if( Backfacing )	printf( "    Backface = True\n" );
    if( TwoSided )	printf( "    TwoSided = True\n" );
    if( SurfShiny )	printf( "    Shiny = True\n" );
    if( SurfBright )	printf( "    Bright = True\n" );
    printf( "    Animate = \"%08x\"\n", AnimBits );
    printf( "    Position = (%g,%g,%g)\n",
			GPosition[0], GPosition[1], GPosition[2] );
    printf( "    Rotate = (%g,%g,%g)\n",
			Rotation[0], Rotation[1], Rotation[2] );
    printf( "    Scale = (%g,%g,%g)\n",
			Scaling[0], Scaling[1], Scaling[2] );
    printf( "    Color = (%g,%g,%g)\n",
			FillColor[0], FillColor[1], FillColor[2] );
    printf( "    GraphN = %d\n", numverts_m );
    printf( "    GraphM = %d\n", numverts_n );
    /* TBD: Xform matrix... */
    printf( "    Data = (\n" );
    for( i = 0; i < numverts_m; i++ ) {
	for( j = 0; j < numverts_n; j++ ) {
	    printf( "	%g,%g,%g", clist[0], clist[1], clist[2] );
	    if( HasRGB ) {
		printf( ",%g,%g,%g", clist[HasRGB  ], clist[HasRGB+1],
					clist[HasRGB+2] );
	    }
	    if( HasNormals ) {
		printf( ",%g,%g,%g", clist[HasNormals  ], clist[HasNormals+1],
					clist[HasNormals+2] );
	    }
	    if( (i < (numverts_m-1)) || (j < (numverts_n-1)) ) putchar( ',' );
	    putchar( '\n' );
	    clist += WPV;
	}
    }
    printf( "    )\n" );
    printf( "    StoreGraphic\n" );
}

void triangular_strip( fildes, clist, numverts, gnormals )
int fildes;
float *clist;
int numverts;
float *gnormals;
{
    int
	i, n2;
    float
	*ptr;

if( !objPrinting ) return;

    n2 = numverts/2;

    printf( "    GraphType = \"Mesh\" /* Really, strip */\n" );
    if( HasNormals )	printf( "    HasNormals = True\n" );
    if( HasRGB )	printf( "    HasRGB = True\n" );
    if( !Backfacing )	printf( "    Backface = True\n" );
    if( TwoSided )	printf( "    TwoSided = True\n" );
    if( SurfShiny )	printf( "    Shiny = True\n" );
    if( SurfBright )	printf( "    Bright = True\n" );
    printf( "    Animate = \"%08x\"\n", AnimBits );
    printf( "    Position = (%g,%g,%g)\n",
			GPosition[0], GPosition[1], GPosition[2] );
    printf( "    Rotate = (%g,%g,%g)\n",
			Rotation[0], Rotation[1], Rotation[2] );
    printf( "    Scale = (%g,%g,%g)\n",
			Scaling[0], Scaling[1], Scaling[2] );
    printf( "    Color = (%g,%g,%g)\n",
			FillColor[0], FillColor[1], FillColor[2] );
    printf( "    GraphN = 2\n" );
    printf( "    GraphM = %d\n", n2 + (numverts&1) );
    /* TBD: Xform matrix... */
    printf( "    Data = (\n" );
    ptr = clist;
    for( i = 0; i < n2; i++ ) {
	printf( "	%g,%g,%g", ptr[0], ptr[1], ptr[2] );
	if( HasRGB ) {
	    printf( ",%g,%g,%g", ptr[HasRGB  ], ptr[HasRGB+1],
				    ptr[HasRGB+2] );
	}
	if( HasNormals ) {
	    printf( ",%g,%g,%g", ptr[HasNormals  ], ptr[HasNormals+1],
				    ptr[HasNormals+2] );
	}
	putchar( ',' );
	putchar( '\n' );
	ptr += 2*WPV;
    }
    if( numverts & 1 ) {
	printf( "	%g,%g,%g", ptr[0], ptr[1], ptr[2] );
	if( HasRGB ) {
	    printf( ",%g,%g,%g", ptr[HasRGB  ], ptr[HasRGB+1],
				    ptr[HasRGB+2] );
	}
	if( HasNormals ) {
	    printf( ",%g,%g,%g", ptr[HasNormals  ], ptr[HasNormals+1],
				    ptr[HasNormals+2] );
	}
	putchar( ',' );
	putchar( '\n' );
    }
    ptr = clist + WPV;
    for( i = 0; i < n2; i++ ) {
	printf( "	%g,%g,%g", ptr[0], ptr[1], ptr[2] );
	if( HasRGB ) {
	    printf( ",%g,%g,%g", ptr[HasRGB  ], ptr[HasRGB+1],
				    ptr[HasRGB+2] );
	}
	if( HasNormals ) {
	    printf( ",%g,%g,%g", ptr[HasNormals  ], ptr[HasNormals+1],
				    ptr[HasNormals+2] );
	}
	if( i < (n2-1) ) {
	    putchar( ',' );
	    putchar( '\n' );
	}
	ptr += 2*WPV;
    }
    if( numverts & 1 ) {
	/* Replicate final vertex */
	ptr -= 2*WPV;
	putchar( ',' );
	putchar( '\n' );
	printf( "	%g,%g,%g", ptr[0], ptr[1], ptr[2] );
	if( HasRGB ) {
	    printf( ",%g,%g,%g", ptr[HasRGB  ], ptr[HasRGB+1],
				    ptr[HasRGB+2] );
	}
	if( HasNormals ) {
	    printf( ",%g,%g,%g", ptr[HasNormals  ], ptr[HasNormals+1],
				    ptr[HasNormals+2] );
	}
    }
    printf( "\n    )\n" );
    printf( "    StoreGraphic\n" );
}

void partial_polygon3d( fildes, clist, numverts, foo, flags )
int fildes;
float *clist;
int numverts;
int foo;
int flags;
{
    polygon3d( fildes, clist, numverts, flags );
}

void polygon3d( fildes, clist, numverts, flags )
int fildes;
float *clist;
int numverts;
int flags;
{
    int
	i;

if( !objPrinting ) return;

    /* Skip gnorm */
    if( FacetNormal )	clist += 3;

    printf( "    GraphType = \"Polygon\"\n" );
    if( HasNormals )	printf( "    HasNormals = True\n" );
    if( HasRGB )	printf( "    HasRGB = True\n" );
    if( Backfacing )	printf( "    Backface = True\n" );
    if( TwoSided )	printf( "    TwoSided = True\n" );
    if( SurfShiny )	printf( "    Shiny = True\n" );
    if( SurfBright )	printf( "    Bright = True\n" );
    printf( "    Animate = \"%08x\"\n", AnimBits );
    printf( "    Position = (%g,%g,%g)\n",
			GPosition[0], GPosition[1], GPosition[2] );
    printf( "    Rotate = (%g,%g,%g)\n",
			Rotation[0], Rotation[1], Rotation[2] );
    printf( "    Scale = (%g,%g,%g)\n",
			Scaling[0], Scaling[1], Scaling[2] );
    printf( "    Color = (%g,%g,%g)\n",
			FillColor[0], FillColor[1], FillColor[2] );
    printf( "    GraphN = %d\n", numverts );
    /* TBD: Xform matrix... */
    printf( "    Data = (\n" );
    for( i = 0; i < numverts; i++ ) {
	printf( "	%g,%g,%g", clist[0], clist[1], clist[2] );
	if( HasRGB ) {
	    printf( ",%g,%g,%g", clist[HasRGB  ], clist[HasRGB+1],
				    clist[HasRGB+2] );
	}
	if( HasNormals ) {
	    printf( ",%g,%g,%g", clist[HasNormals  ], clist[HasNormals+1],
				    clist[HasNormals+2] );
	}
	if( i < (numverts-1) ) putchar( ',' );
	putchar( '\n' );
	clist += WPV;
    }
    printf( "    )\n" );
    printf( "    StoreGraphic\n" );
}

#if 0 /* [ libprim hack */
float *_prim_malloc_object( size )
int size;
{
    void *malloc();
    return malloc( size * sizeof(float) );
}

void _prim_free_object( obj )
void *obj;
{
    void free();
    if( obj ) free( obj );
}

/* Internal procedure to draw a mesh surface from the 3D points
 * of poly rotated about the y-axis.
 */
static void _mesh_surface_of_revolution(fildes,poly,poly_size,
	facets,cpv,height,times_around,mat)
int fildes,poly_size;
float *poly,height,times_around;
int facets,cpv;
float mat[4][4];
{
    float *object,*fptr,*origptr;
    float x,y,z,sa,ca,sda,cda,tmp;
    float offset,d_offset;
    int i,j,count;
    double d_ang;

    count = facets * times_around + 1;
    if ((fptr = object = _prim_malloc_object(cpv*poly_size*count)) == NULL) {
	perror("mesh_surface_of_revolution");
	return;
    }

    d_ang = (2.0*M_PI)/facets;
    cda = (float) cos(d_ang); sda = (float) sin(d_ang);
    ca = 1.0; sa = 0.0;
    offset = 0.0;
    d_offset = height/facets/times_around;
    /* now revolve around the y axis */
    for (i=0; i<count; ++i) {
	origptr = poly;
	for (j=0; j<poly_size; ++j) {
	    x  = *origptr++;
	    y  = *origptr++;
	    z  = *origptr++;
	    *fptr++ = ca*x - sa*z;
	    *fptr++ = y + offset;
	    *fptr++ = sa*x + ca*z;
	    if (cpv == 6) {
		x  = *origptr++;
		y  = *origptr++;
		z  = *origptr++;
		*fptr++ = ca*x - sa*z;
		*fptr++ = y;
		*fptr++ = sa*x + ca*z;
	    }
	}
	tmp = ca*cda - sa*sda;
	sa  = ca*sda + sa*cda;
	ca  = tmp;
	offset += d_offset;
    }

    /* Get it oriented right */
    xform_points(object,count*poly_size,cpv,(cpv==6),mat);

    /* draw it! */
    push_vertex_format(fildes,cpv-3,cpv-3,0,FALSE,TOGGLE_ORDER);
    quadrilateral_mesh(fildes,object,count,poly_size,NULL);
    pop_vertex_format(fildes);

    _prim_free_object(object);
}



static void _orient_msor(fildes,poly,poly_size,facets,normals,
	x1,y1,z1,x2,y2,z2,offset,times_around,orient)
int fildes,poly_size,facets,normals;
float *poly,x1,y1,z1,x2,y2,z2;
float offset,times_around;
int orient;
{
    float dx,dy,dz,x,y,z,h,length,alpha;
    float cos_theta,sin_theta,cos_phi,sin_phi;
    float mat[4][4],*fptr,tmp;
    int i,cpv;
    float *ptr1,*ptr2,*object;
    float xval, yval, zval, mag;

    if (normals) cpv = 6;
    else cpv = 3;

    dx = x2-x1; dy = y2-y1; dz = z2-z1;
    length = (float) sqrt((double)(dx*dx+dy*dy+dz*dz));
    alpha = (float) sqrt((double)(dx*dx+dz*dz));
    if (alpha == 0.0) {
	sin_theta = 0.0;
	cos_theta = (dy>0.0)?1.0:-1.0;
	sin_phi = 0.0;
	cos_phi = 1.0;
    }
    else {
	sin_theta = alpha/length;
	cos_theta = dy/length;
	sin_phi   = dx/alpha;
	cos_phi   = dz/alpha;
    }

    fptr = (float *) mat;
    *fptr++ = cos_phi;
    *fptr++ = sin_phi*sin_theta;
    *fptr++ = sin_phi*cos_theta;
    *fptr++ = 0.0;

    *fptr++ = 0.0;
    *fptr++ = cos_theta;
    *fptr++ = -sin_theta;
    *fptr++ = 0.0;

    *fptr++ = -sin_phi;
    *fptr++ = cos_phi*sin_theta;
    *fptr++ = cos_phi*cos_theta;
    *fptr++ = 0.0;

    *fptr++ = -x1*mat[0][0]                 -z1*mat[2][0];
    *fptr++ = -x1*mat[0][1] -y1*mat[1][1] -z1*mat[2][1];
    *fptr++ = -x1*mat[0][2] -y1*mat[1][2] -z1*mat[2][2];
    *fptr++ = 1.0;

    /* make the axis of revolution the y-axis */
    if (orient) {
	ptr1 = poly;
	if ((ptr2 = object = _prim_malloc_object(cpv*poly_size)) == NULL) {
	    perror("mesh_surface_of_revolution");
	    return;
	}

	for (i=0; i<poly_size; ++i) {
	    x = *ptr1++;
	    y = *ptr1++;
	    z = *ptr1++;
	    *ptr2++ = x*mat[0][0] + y*mat[1][0] 
		    + z*mat[2][0] + mat[3][0];
	    *ptr2++ = x*mat[0][1] + y*mat[1][1] 
		    + z*mat[2][1] + mat[3][1];
	    *ptr2++ = x*mat[0][2] + y*mat[1][2] 
		    + z*mat[2][2] + mat[3][2];
	    if (normals) {
		x = *ptr1++;
		y = *ptr1++;
		z = *ptr1++;

		xval = x*mat[0][0] + y*mat[1][0] + z*mat[2][0];
		yval = x*mat[0][1] + y*mat[1][1] + z*mat[2][1];
		zval = x*mat[0][2] + y*mat[1][2] + z*mat[2][2];

		mag = xval*xval + yval*yval + zval*zval;
		mag = (float)sqrt( (double)mag);

		*ptr2++ = xval / mag;
		*ptr2++ = yval / mag;
		*ptr2++ = zval / mag;
	    }
	}
    }
    else {
	object = poly;
    }

    /* push something on the stack to put the axis back */

    /* permute the rotation matrix and restore the translation */
    tmp = mat[0][1]; mat[0][1] = mat[1][0]; mat[1][0] = tmp;
    tmp = mat[0][2]; mat[0][2] = mat[2][0]; mat[2][0] = tmp;
    tmp = mat[1][2]; mat[1][2] = mat[2][1]; mat[2][1] = tmp;
    mat[3][0] = x1; mat[3][1] = y1; mat[3][2] = z1;

    _mesh_surface_of_revolution(fildes,object,poly_size,facets,
	cpv,offset,times_around,mat);

    if (orient) _prim_free_object(object);
}


/*                     MESH_SURFACE_OF_REVOLUTION
 *
 * This procedure forms a rational mesh surface by rotation the
 * specified polygon around an arbitrary axis. The control points can
 * be in rational form and u_knot_vector can be specified by the user.
 *
 * fildes    -- integer; file descriptor
 * poly      -- float*; the control polygon
 * poly_size -- integer; the number of vertices in poly
 * facets    -- number of facets in the revolution
 * normals   -- are there normals in the data?
 * x1,y1,z1,x2,y2,z2 -- floats; the axis of rotation
 */
void mesh_surface_of_revolution(fildes,poly,poly_size,facets,normals,
	x1,y1,z1,x2,y2,z2)
int fildes,poly_size,facets,normals;
float *poly,x1,y1,z1,x2,y2,z2;
{
    _orient_msor(fildes,poly,poly_size,facets,normals,
	    x1,y1,z1,x2,y2,z2,0.0,1.0,TRUE);
}


void mesh_torus(fildes,facets,rev_steps,minor_radius,major_radius,
	x1,y1,z1,x2,y2,z2)
int fildes,facets,rev_steps;
float minor_radius,major_radius;
float x1,y1,z1,x2,y2,z2;
{
    static float
	Rot[3] = {0.,0.,0.};

    /*DeriveRotation( 0.0, 0.0, 1.0, x2-x1, y2-y1, z2-z1, Rot );*/
    printf( "    GraphType = \"Torus\"\n" );
    printf( "    Color = (%g,%g,%g)\n",
			FillColor[0], FillColor[1], FillColor[2] );
    printf( "    HasNormals = True\n" );
    printf( "    Shiny = %s\n", SurfShiny ? "True" : "False" );
    printf( "    Bright = %s\n", SurfBright ? "True" : "False" );
    printf( "    Animate = \"%08X\"\n", AnimBits );
    printf( "    GraphN = %d\n", facets );
    printf( "    GraphM = %d\n", rev_steps );
    printf( "    Position = (%g,%g,%g)\n",
			GPosition[0], GPosition[1], GPosition[2] );
    printf( "    Rotate = (%g,%g,%g)\n",
			Rotation[0], Rotation[1], Rotation[2] );
    printf( "    Scale = (%g,%g,%g)\n",
			Scaling[0], Scaling[1], Scaling[2] );
    printf( "    Data = (%g,%g,0,360,0,360)\n", minor_radius, major_radius );
    printf( "    StoreGraphic\n" );
}


void mesh_helix(fildes,facets,rev_steps,minor_radius,major_radius,
	x1,y1,z1,x2,y2,z2,times_around)
int fildes,facets,rev_steps;
float minor_radius,major_radius;
float x1,y1,z1,x2,y2,z2,times_around;
{
    double ang;
    float sa,ca;
    float *poly,*pptr;
    float x,y,xt,offset,dx,dy,dz;
    int i;

    if ((poly = _prim_malloc_object((facets+1)*6)) == NULL) {
	perror("mesh_helix");
	return;
    }

    ang = 2.0*M_PI/(float) facets;
    sa = (float) sin(ang);
    ca = (float) cos(ang);

    pptr = poly;
    x = minor_radius; y=0;
    for (i=0; i<(facets+1); ++i) {
	*pptr++ = x + major_radius;
	*pptr++ = y;
	*pptr++ = 0.0;
	*pptr++ = x;
	*pptr++ = y;
	*pptr++ = 0.0;
	xt = x*ca - y*sa;
	y  = x*sa + y*ca;
	x = xt;
    }

    dx = x2-x1; dy = y2-y1; dz = z2-z1;
    offset = (float) sqrt((double)(dx*dx+dy*dy+dz*dz));

    _orient_msor(fildes,poly,facets+1,rev_steps,TRUE,
	    x1,y1,z1,x2,y2,z2,offset,times_around,FALSE);
    _prim_free_object(poly);
}


void mesh_sphere(fildes, radius, latitudes, longitudes, x,y,z)
int fildes;
float radius;
int latitudes, longitudes;
float x,y,z;
{
    printf( "    GraphType = \"Sphere\"\n" );
    printf( "    Color = (%g,%g,%g)\n",
			FillColor[0], FillColor[1], FillColor[2] );
    printf( "    HasNormals = True\n" );
    printf( "    Shiny = %s\n", SurfShiny ? "True" : "False" );
    printf( "    Bright = %s\n", SurfBright ? "True" : "False" );
    printf( "    Animate = \"%08X\"\n", AnimBits );
    printf( "    GraphN = %d\n", latitudes );
    printf( "    GraphM = %d\n", longitudes );
    printf( "    Position = (%g,%g,%g)\n",
			x + GPosition[0], y + GPosition[1], z + GPosition[2] );
    printf( "    Rotate = (%g,%g,%g)\n",
			Rotation[0], Rotation[1], Rotation[2] );
    printf( "    Scale = (%g,%g,%g)\n",
			Scaling[0], Scaling[1], Scaling[2] );
    printf( "    Data = (%g,-90,90,0,360)\n", radius );
    printf( "    StoreGraphic\n" );
}

void mesh_cone(fildes,b_rad,t_rad,b_cap,t_cap,facets,xb,yb,zb,xt,yt,zt)
int fildes,b_cap,t_cap,facets;
float b_rad,t_rad,xb,yb,zb,xt,yt,zt;
{
    double ang,d_ang;
    float *cone,*bptr,*tptr,ca,sa,raddiff;
    int i;
    float height = HYPOT3(xt-xb,yt-yb,zt-zb);
    float mat[4][4];
    float mag;

    /* Don't let the radii be zero -- this results on degenerate and
     * silly-looking polygons on many devices.
     */
    if (t_rad == 0.0) {
	if (b_rad == 0.0) {
	    t_rad = b_rad = height/1000000.0;
	}
	else t_rad = b_rad/1000000.0;
    }
    else if (b_rad == 0.0) b_rad = t_rad/1000000.0;

    ang     = 0.0;
    d_ang   = (2.0*M_PI)/facets;
    raddiff = b_rad - t_rad;

    if ((bptr = cone = _prim_malloc_object((facets+1)*6*2)) == NULL) {
	perror("mesh_cone");
	return;
    }

    tptr = bptr + (facets+1)*6;
    for (i=0; i<=facets; ++i) {
	sa = (float) sin(ang);
	ca = (float) cos(ang);
	ang += d_ang;
	/* x,y,z's */
	*bptr++ = ca*b_rad;
	*tptr++ = ca*t_rad;
	*bptr++ = sa*b_rad;
	*tptr++ = sa*t_rad;
	*bptr++ = 0.0;
	*tptr++ = height;
	/* normals */
	mag = height*ca *height*ca;
	mag +=  height*sa *  height*sa;
	mag += raddiff * raddiff;
	mag = (float)sqrt( (double)mag);

	*bptr++ = *tptr++ = (height*ca)/mag;;
	*bptr++ = *tptr++ = (height*sa)/mag;
	*bptr++ = *tptr++ = (raddiff)/mag;
    }

    construct_orientation_matrix(mat,xb,yb,zb,xt,yt,zt);
    xform_points(cone,2*(facets+1),6,TRUE,mat);
    push_vertex_format(fildes,3,3,0,FALSE,CLOCKWISE);
    quadrilateral_mesh(fildes,cone,2,(facets+1),NULL);
    pop_vertex_format(fildes);

    if (b_cap) {
	push_vertex_format(fildes,3,0,0,FALSE,COUNTER_CLOCKWISE);
	polygon3d(fildes,cone,facets,FALSE);
	pop_vertex_format(fildes);
    }

    if (t_cap) {
	push_vertex_format(fildes,3,0,0,FALSE,CLOCKWISE);
	polygon3d(fildes,cone+(facets+1)*6,facets,FALSE);
	pop_vertex_format(fildes);
    }

    _prim_free_object(cone);
}

typedef struct {
    int coord,use,rgb,normals,order;
} VNODE;

#define VF_STACK_SIZE	64
static VNODE vf_default[VF_STACK_SIZE] = {
    { 0,0,0,FALSE,COUNTER_CLOCKWISE },
};
static VNODE *current_vf = vf_default;

	
void prim_vertex_format(fildes,coord,use,rgb,normals,order)
int fildes,coord,use,rgb,normals,order;
{
    current_vf = vf_default;
    current_vf->coord   = coord;
    current_vf->use     = use;
    current_vf->rgb     = rgb;
    current_vf->normals = normals;
    current_vf->order   = order;

    vertex_format(fildes,coord,use,rgb,normals,order);
}


void push_vertex_format(fildes,coord,use,rgb,normals,order)
int fildes,coord,use,rgb,normals,order;
{
    VNODE *last_vf;

    if (current_vf == (&vf_default[VF_STACK_SIZE])) {
	fprintf(stderr,"Out of VF space in push_vertex_format.\n");
    }
    /* else */

    last_vf = current_vf;
    ++current_vf;

    if (order == TOGGLE_ORDER) {
	if (last_vf->order == CLOCKWISE) order = COUNTER_CLOCKWISE;
	else order = CLOCKWISE;
    }
    else if (order == SAME_ORDER) {
	order = last_vf->order;
    }

    current_vf->coord   = coord;
    current_vf->use     = use;
    current_vf->rgb     = rgb;
    current_vf->normals = normals;
    current_vf->order   = order;

    vertex_format(fildes,coord,use,rgb,normals,order);
}


void push_vertex_order(fildes,neworder)
int fildes,neworder;
{
    if (neworder == TOGGLE_ORDER) {
	if (current_vf->order == CLOCKWISE) neworder = COUNTER_CLOCKWISE;
	else neworder = CLOCKWISE;
    }
    else if (neworder == SAME_ORDER) {
	    neworder = current_vf->order;
    }

    push_vertex_format(fildes,current_vf->coord,current_vf->use,
	    current_vf->rgb,current_vf->normals,neworder);
}


void pop_vertex_format(fildes)
int fildes;
{
    if (current_vf == vf_default) {
	fprintf(stderr,"pop_vertex_format of empty stack.\n");
	return;
    }

    --current_vf;

    vertex_format(fildes,current_vf->coord,current_vf->use,
	    current_vf->rgb,current_vf->normals,current_vf->order);
}

void circle(fildes,radius,facets,partial,xc,yc,zc,xn,yn,zn)
int fildes,facets,partial;
float radius,xc,yc,zc,xn,yn,zn;
{
    double ang=0.0,d_ang;
    float *circle,*cptr,ca,sa;
    int i;
    float mat[4][4];

    if ((cptr = circle = _prim_malloc_object(facets*3)) == NULL) {
	perror("circle");
	return;
    }

    d_ang = (2.0*M_PI)/facets;
    for (i=0; i<facets; ++i) {
	ang += d_ang;
	/* x,y,z's */
	*cptr++ = FCOS(ang) * radius;
	*cptr++ = FSIN(ang) * radius;
	*cptr++ = 0.0;
    }

    construct_orientation_matrix(mat,xc,yc,zc,xn,yn,zn);
    xform_points(circle,facets,3,FALSE,mat);
    push_vertex_format(fildes,0,0,0,FALSE,COUNTER_CLOCKWISE);
    if (partial) partial_polygon3d(fildes,circle,facets,FALSE,TRUE);
    else polygon3d(fildes,circle,facets,FALSE);
    pop_vertex_format(fildes);

    _prim_free_object(circle);
}

void _trimmed_plane();


/*                     EXTRUSION
 *
 * This procedure forms an extruded object by sweeping the polygon
 * given in clist along a path.  At each step of the path, the normal 
 * of the offset polygon remains equal to the normal of the original
 * polygon, thus, the back face will be "parallel" to the front face,
 * regardless of the path. Each triple in "path" is a vector relative
 * to the origin.
 *
 * fildes     -- integer; file descriptor
 * clist      -- float*; a simple list of the polygon's vertices
 * poly_size  -- integer; the number of vertices in clist
 * path       -- float*; a list of vectors, relative to the origin,
 *                to offset clist by to get the next set of control
 *                points in the extrusion
 * path_size  -- integer; the number of vectors in path
 * front_face -- boolean; whether to draw the front face
 * back_face  -- boolean; whether to draw the back face
 *
 */

void extrusion(fildes,clist,poly_size,path,path_size,front_face,back_face)
int fildes,poly_size,path_size,front_face,back_face;
float *clist,*path;
{
	int i,j;
	float *poly,*oldpoly,*ptr1,*ptr2,*ptr3,*path_ptr=path;
	float side[12];

	/* copy the polygon to temporary storage */
	if ((poly = ptr1 = _prim_malloc_object((poly_size+1)*3)) == NULL) {
	    perror("extrusion");
	    return;
	}
	if ((oldpoly     = _prim_malloc_object((poly_size+1)*3)) == NULL) {
	    perror("extrusion");
	    _prim_free_object(poly);
	    return;
	}

	ptr2 = clist;
	for (i=0;i<3*poly_size;++i) *ptr1++ = *ptr2++;
	/* add the first point back to the end */
	ptr2 = clist;
	*ptr1++ = *ptr2++;
	*ptr1++ = *ptr2++;
	*ptr1++ = *ptr2++;

	/* do the front */
	if (front_face) polygon3d(fildes,poly,poly_size,FALSE);

	/* For each segment of the extrusion, draw the sides of the object*/
	for (j=0;j<path_size;++j) {
		/* Copy the old poly and form the back face by moving the
		 * original poly along the path.
		 */
		ptr1 = oldpoly;
		ptr2 = poly;
		for (i=0;i<poly_size+1;++i) {
			*ptr1++  = *ptr2;
			*ptr1++  = *(ptr2+1);
			*ptr1++  = *(ptr2+2);
			*ptr2++ += *path_ptr;
			*ptr2++ += *(path_ptr+1);
			*ptr2++ += *(path_ptr+2);
		}
		
		/* Do the sides */
		ptr1 = oldpoly;
		ptr2 = poly;
		for (i=0;i<poly_size;++i) {
			ptr3 = side;
			*ptr3++ = *ptr1++; *ptr3++ = *ptr1++; *ptr3++ = *ptr1++;
			*ptr3++ = *ptr2++; *ptr3++ = *ptr2++; *ptr3++ = *ptr2++;
			*ptr3++ = *ptr2; *ptr3++ = *(ptr2+1); *ptr3++ = *(ptr2+2);
			*ptr3++ = *ptr1; *ptr3++ = *(ptr1+1); *ptr3++ = *(ptr1+2);
			polygon3d(fildes,side,4,FALSE);
		}
		
		path_ptr += 3;
	}

	if (back_face) {
		/* do the back face */
		push_vertex_order(fildes,TOGGLE_ORDER);
		polygon3d(fildes,poly,poly_size,FALSE);
		pop_vertex_format(fildes);
	}

	/* clean up */
	_prim_free_object(oldpoly);
	_prim_free_object(poly);
}


static void rotmat(mat,nx,ny,nz)
float *mat;
float nx,ny,nz;
{
	float len,alpha,ct,st,cp,sp;

	if ((len = FSQRT(nx*nx + ny*ny + nz*nz)) != 0.0) {
		nx /= len;
		ny /= len;
		nz /= len;
		alpha = FSQRT(nx*nx + nz*nz);
		if (alpha == 0.0) {
			st = (ny>0.0)?1.0:-1.0;
			ct = 0.0;
			sp = 0.0;
			cp = 1.0;
		}
		else {
			st = ny;
			ct = alpha;
			sp = nz/alpha;
			cp = nx/alpha;
		}
	}
	else {
		ct = cp = 1.0;
		st = sp = 0.0;
	}

	*mat++ = cp*ct;
	*mat++ = -cp*st;
	*mat++ = -sp;

	*mat++ = st;
	*mat++ = ct;
	*mat++ = 0.0;

	*mat++ = sp*ct;
	*mat++ = -sp*st;
	*mat++ = cp;
}


/*                     TUBE_EXTRUSION
 *
 * This procedure forms an extruded object by sweeping the polygon
 * given in clist along a path.  At each step of the path, the normal 
 * of the offset polygon remains parallel to the path, thus, the face
 * remains perpendicular to the path.
 *
 * fildes     -- integer; file descriptor
 * clist      -- float*; a simple list of the polygon's vertices
 * poly_size  -- integer; the number of vertices in clist
 * path       -- float*; a list of vectors, relative to the origin,
 *                to offset clist by to get the next set of control
 *                points in the extrusion
 * path_size  -- integer; the number of vectors in path
 * front_face -- boolean; whether to draw the front face
 * back_face  -- boolean; whether to draw the back face
 *
 */

void tube_extrusion(fildes,clist,poly_size,path,path_size,front_face,back_face)
int fildes,poly_size,path_size,front_face,back_face;
float *clist,*path;
{
	int i,j;
	float *mesh,*ptr1,*ptr2,*ptr3,*path_ptr,*tpoly;
	float nx,ny,nz,ax,ay,az,bx,by,bz;
	float x,y,z,offsetx,offsety,offsetz,orx,ory,orz;
	float mat[3][3];

	/* get temporary storage */
	if ((mesh = _prim_malloc_object((poly_size+1)*(path_size+1)*3)) == NULL) {
	    perror("tube_extrusion");
	    return;
	}
	if ((tpoly = _prim_malloc_object((poly_size+1)*3)) == NULL) {
	    perror("tube_extrusion");
	    _prim_free_object(mesh);
	    return;
	}

	/* find rough center of object */
	ptr1 = clist;
	orx = ory = orz = 0.0;
	for (i=0; i<poly_size; ++i) {
		orx += *ptr1++;
		ory += *ptr1++;
		orz += *ptr1++;
	}
	orx /= (float) poly_size;
	ory /= (float) poly_size;
	orz /= (float) poly_size;

	/* find three non-colinear points */
	ax = *(clist+3) - *(clist);
	ay = *(clist+4) - *(clist+1);
	az = *(clist+5) - *(clist+2);

	ptr2 = clist+6;
	do {
		bx = *ptr2++ - *(clist+3);
		by = *ptr2++ - *(clist+4);
		bz = *ptr2++ - *(clist+5);
		nx = ay*bz - by*az;
		ny = az*bx - bz*ax;
		nz = ax*by - bx*ay;
	} while ((nx == 0.0) && (ny == 0.0) && (nz == 0.0));

	/* compute rotation matrix so normal lies on x axis */
	rotmat((float *) mat,nx,ny,nz);

	/* copy the original poly rotated that way */
	ptr1 = clist;
	ptr2 = tpoly;
	for (i=0; i<poly_size; ++i) {
		x = *ptr1++ - orx;
		y = *ptr1++ - ory;
		z = *ptr1++ - orz;
		*ptr2++ = mat[0][0]*x + mat[1][0]*y + mat[2][0]*z;
		*ptr2++ = mat[0][1]*x + mat[1][1]*y + mat[2][1]*z;
		*ptr2++ = mat[0][2]*x + mat[1][2]*y + mat[2][2]*z;
	}

	/* add the first point back to the end */
	ptr1 = clist;
	x = *ptr1++ - orx;
	y = *ptr1++ - ory;
	z = *ptr1++ - orz;
	*ptr2++ = mat[0][0]*x + mat[1][0]*y + mat[2][0]*z;
	*ptr2++ = mat[0][1]*x + mat[1][1]*y + mat[2][1]*z;
	*ptr2++ = mat[0][2]*x + mat[1][2]*y + mat[2][2]*z;

	/* construct the mesh */
	path_ptr = path;
	ptr2 = mesh;
	offsetx = orx;
	offsety = ory;
	offsetz = orz;
	for (j=0; j<=path_size; ++j) {
		/* normal should be average of this path and next */
		if ((j == 0) || (j == path_size)) {
			/* first normal should be same as first path vector */
			nx = *(path_ptr);
			ny = *(path_ptr+1);
			nz = *(path_ptr+2);
		}
		else {
			nx = *(path_ptr  ) + *(path_ptr+3);
			ny = *(path_ptr+1) + *(path_ptr+4);
			nz = *(path_ptr+2) + *(path_ptr+5);
			path_ptr += 3;
		}
		rotmat((float *) mat,nx,ny,nz);
		/* rotate from poly with normal on X to poly with this normal */

		ptr1 = tpoly;
		ptr3 = ptr2;
		for (i=0; i<(poly_size+1); ++i) {
			x = *ptr1++;
			y = *ptr1++;
			z = *ptr1++;
			/* reverse rotation */
			*ptr2++ = mat[0][0]*x + mat[0][1]*y + mat[0][2]*z + offsetx;
			*ptr2++ = mat[1][0]*x + mat[1][1]*y + mat[1][2]*z + offsety;
			*ptr2++ = mat[2][0]*x + mat[2][1]*y + mat[2][2]*z + offsetz;
		}
		offsetx += *path_ptr;
		offsety += *(path_ptr+1);
		offsetz += *(path_ptr+2);

		if ((j == 0)
				&& front_face) {
			polygon3d(fildes,ptr3,poly_size,FALSE);
		}
		else if ((j == path_size)
				&& back_face) {
			polygon3d(fildes,ptr3,poly_size,FALSE);
		}
	}
	quadrilateral_mesh(fildes,mesh,path_size+1,poly_size+1,FALSE);


	if (back_face) {
		/* do the back face */
		push_vertex_order(fildes,TOGGLE_ORDER);
		polygon3d(fildes,mesh+(path_size)*3,poly_size,FALSE);
		pop_vertex_format(fildes);
	}

	/* clean up */
	_prim_free_object(tpoly);
	_prim_free_object(mesh);
}

void xform_points(points,num_points,coord_per_vertex,normals,xform)
float *points;
int num_points,coord_per_vertex,normals;
float xform[4][4];
{
    float x,y,z;
    int i;

    for (i=0; i<num_points; ++i) {
	x = *(points);
	y = *(points+1);
	z = *(points+2);

	*(points)   = x * xform[0][0] + y * xform[1][0] + z * xform[2][0]
	    + xform[3][0];
	*(points+1) = x * xform[0][1] + y * xform[1][1] + z * xform[2][1]
	    + xform[3][1];
	*(points+2) = x * xform[0][2] + y * xform[1][2] + z * xform[2][2]
	    + xform[3][2];

	if (normals) {
	    x = *(points+3);
	    y = *(points+4);
	    z = *(points+5);

	    *(points+3) = x * xform[0][0] + y * xform[1][0] + z * xform[2][0];
	    *(points+4) = x * xform[0][1] + y * xform[1][1] + z * xform[2][1];
	    *(points+5) = x * xform[0][2] + y * xform[1][2] + z * xform[2][2];
	}

	points += coord_per_vertex;
    }
}

void construct_orientation_matrix(mat,x1,y1,z1,x2,y2,z2)
float mat[4][4],x1,y1,z1,x2,y2,z2;
{
    float height,dx,dy,dz,dx2,dz2;
    float sin_theta,cos_theta,sin_phi,cos_phi,alpha;
    float *fptr;

    dx = x2-x1; dy = y2-y1; dz = z2-z1;
    dx2 = dx*dx;
    dz2 = dz*dz;
    height = FSQRT(dx2+dy*dy+dz2);
    alpha  = FSQRT(dx2+dz2);

    if (alpha == 0.0) {
	    sin_theta = (dy > 0.0) ? 1.0 : -1.0;
	    cos_theta = 0.0;
	    sin_phi = 0.0;
	    cos_phi = 1.0;
    }
    else {
	    sin_theta = dy/height;
	    cos_theta = alpha/height;
	    sin_phi   = dx/alpha;
	    cos_phi   = dz/alpha;
    }

    fptr = (float *) mat;
    *fptr++ = cos_phi;
    *fptr++ = 0.0;
    *fptr++ = -sin_phi;
    *fptr++ = 0.0;

    *fptr++ = -sin_theta*sin_phi;
    *fptr++ = cos_theta;
    *fptr++ = -sin_theta*cos_phi;
    *fptr++ = 0.0;

    *fptr++ = cos_theta*sin_phi;
    *fptr++ = sin_theta;
    *fptr++ = cos_theta*cos_phi;
    *fptr++ = 0.0;

    *fptr++ = x1;
    *fptr++ = y1;
    *fptr++ = z1;
    *fptr++ = 1.0;
}
#endif /* ] */

#else /* ] [ */
void add_names_to_set( int dl, int a, int b[] )
{
}

void remove_all_names_from_set( int seg )
{
}

void concat_transformation3d( int gfd, float m[4][4], int a, int b )
{
}

void pop_matrix( int gfd )
{
}

int hidden_surface( int a, int b, int c )
{
}

void fill_color( int fildes, float a, float b, float c )
{
}

void line_color( int gfd, float r, float g, float b )
{
}

void c_line_color( int fildes, float r, float g, float b )
{
}

void c_fill_color( int gfd, double r, double g, double b )
{
}

static int globCoord, globUse, globRgb, globNormals, globOrder;

void vertex_format( int gfd, int coord, int use, int rgb, int normals, int order )
{
    globCoord = coord;
    globUse = use;
    globRgb = rgb;
    globNormals = normals;
    globOrder = order;
}

void inquire_vertex_format( int gfd, int *coord, int *use, int *rgb, int *normals, int *order )
{
    *coord = globCoord;
    *use = globUse;
    *rgb = globRgb;
    *normals = globNormals;
    *order = globOrder;
}

void quadrilateral_mesh( int a, float b[], int c, int d, float e[] )
{
}

void triangular_strip( int a, float b[], int d, float e[] )
{
}

void partial_polygon3d( int fildes, float clist[], int numverts, int flags,
				int closure )
{
}

void polygon3d( int gfd, float clist[], int n, int m )
{
}

#endif /* ] */

#if 1 /* [ */
/* Real SB hack! */

void polyline3d( int gfd, float clist[], int n, int m )
{
}

void push_matrix3d( int fildes, float xform3[4][4] )
{
}

void cond_return( int fildes, int cond_index_select, int comp_flag )
{
}

void set_ele_ptr_relative( int fildes, int offset )
{
}

void interpret_ele( int fildes, int *ele )
{
}

void interior_style( int fildes, int style, int edged )
{
}

int gopen( char *path, int kind, char *driver, int mode )
{
    return 255;
}

void inq_ele( int a, int *b )
{
}

void cond_execute_segment( int a, int b, int c, int d )
{
}

void inq_ele_size( int a, int *b )
{
}

#if 0
void gescape( int gfd, int n, gescape_arg *a, gescape_arg *b )
{
}
#endif

void surface_model( int gfd, int a1, int a2, float r, float g, float b )
{
}

void execute_segment( int a, int b )
{
}

void move3d( int gfd, float a, float b, float c )
{
}

float _hp_tmp2;
float _hp_tmp3;
float _hp_tmp1;

void cond_call_segment( int a, int b, int c, int d )
{
}

void c_set_cull_size( int gfd, double zzz )
{
}
void set_cull_size( int gfd, double zzz )
{
}

void set_extent( int gfd, float a[2][3] )
{
}

void call_segment( int a, int b )
{
}

void surface_coefficients( int gfd, float amb, float diff, float spec )
{
}

void close_segment( int gfd )
{
}

void inq_ele_ptr_at_bound( int gfd, int *a, int *b )
{
}

void open_segment( int a, int b, int c, int d )
{
}

void draw3d( int a, float b, float c, float d )
{
}

void set_ele_ptr( int a, int b )
{
}

#endif /* ] */
