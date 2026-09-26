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
 * drive_msg.c - Client messages
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef WIN32
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <sys/param.h>
#include <pwd.h>
#else
#include <sys/timeb.h>
#endif

#include "global.h"
#include "message.h"
#include "newMessage.h"
#include "sockets.h"
#include "gauge.h"
#include "drive.h"
#if !(defined(WIN32) || defined(MAC))
#include "gui.h"
#include "joystick.h"
#endif

#include "hw_types.h"

#ifndef EPSILON
# define EPSILON (1.0e-6)
#endif

/**************************** MODULE GLOBALS **********************************/
static char *recv_buf;
static char *send_buf;

static boolean_type client_close(
    struct IPCMsg *m);
static boolean_type client_define(
    struct IPCMsg *m);
static boolean_type client_beginframe(
    struct IPCMsg *m);
static boolean_type client_identify(
    struct IPCMsg *m);
static boolean_type client_driveable(
    struct IPCMsg *m);
static boolean_type client_start_lap(
    struct IPCMsg *m);
static boolean_type client_checkpoint(
    struct IPCMsg *m);
static boolean_type client_finish_lap(
    struct IPCMsg *m);
static boolean_type client_update_state(
    struct IPCMsg *m);
static boolean_type client_standing(
    struct IPCMsg *m);
static boolean_type client_explosion(
    struct IPCMsg *m);
static boolean_type client_define_many(
    struct IPCMsg *m);
static boolean_type client_virtual_time(
    struct IPCMsg *m);

boolean_type (* client_table[])(struct IPCMsg *m) = {
    client_close,		/* Quit. */
    client_define,		/* Define display lists. */
    client_beginframe,		/* Beginning of frame message. */
    client_identify,		/* get connection identification */
    client_driveable,		/* receive driveable vehicle info. */
    client_start_lap,		/* hit a start checkpoint */
    client_checkpoint,		/* hit an intermediate checkpoint */
    client_finish_lap,		/* hit a finish checkpoint */
    client_update_state,	/* get what state we are in */
    client_standing,		/* What place did we come in ? */
    client_explosion,		/* Do explosion here */
    client_define_many,		/* Define (potentially) many display lists */
    client_virtual_time,	/* Set virtual time */
};


/*****************************************************************
 * client_close
 * 
 * 	Terminates client.
 *
 * Inputs:
 *		m	Message.
 * Outputs:
 *		TRUE 	to terminate client.
 */
static boolean_type client_close(
    struct IPCMsg *m)
{
    /* Restore the Pointer values */
#ifdef SET_POINTER
    XChangePointerControl(cstate.display,1,1,save_XPointer_numerator,
	save_XPointer_denominator,save_XPointer_threshold);
#endif
    return TRUE;
}

/*****************************************************************
 * client_define
 * 
 * 	Get a display list from the server.
 * 	
 * Inputs:
 *		m	Message.
 * Outputs:
 *		FALSE 	to keep client going.
 */
static boolean_type client_define(
    struct IPCMsg *m)
{
    static int num_segs = 0; 
    static float tot_size = 0.0;

    segment_receive_indicator_on();
    /*
    printf("Segment Message Size: %d",m->data.size);
    */
    num_segs++;
    tot_size += m->MsgLength;
    /*
    printf("	Ave. Size: %f \n",tot_size / num_segs);
    */

    receive_segment( img_fildes, (struct SrvDefine *)m,
	cstate.disable_transparency );
    segment_receive_indicator_off();

    return FALSE;
}

/*****************************************************************
 * client_define_many
 * 
 * 	Get possibly several display lists from the server.
 * 	
 * Inputs:
 *		m	Message.
 * Outputs:
 *		FALSE 	to keep client going.
 */
static boolean_type client_define_many(
    struct IPCMsg *m)
{
#if !defined(__linux__) /* [ */
    int strid, id;
    static int num_segs = 0; 
    static float tot_size = 0.0;
    static char read_buf[8192];
    char *read_buf_ptr;
    int done;
    struct IPCMsg *msg;

#if 0 /* [ */
    segment_receive_indicator_on();
/*
    if(debug) {
	num_segs++;
	tot_size += m->data.size;
	printf("Mongo Segment Message Size:   %d",m->data.size);
	printf("	Ave. Size: %f \n",tot_size / num_segs);
    }
*/

    soc_read( server_socket, read_buf, m->data.size);
    read_buf_ptr = read_buf;
    msg = (struct IPCMsg *) read_buf_ptr;
    read_buf_ptr += sizeof(struct IPCMsg);
    ( msg->type == FRAME_END)? (done = 1) : (done = 0);

    while( !done)
    {
	read_buf_ptr = (char *) receive_segment_from_buffer( img_fildes, read_buf_ptr,
	    cstate.disable_transparency );

	msg = (struct IPCMsg *) read_buf_ptr;
	read_buf_ptr += sizeof(struct IPCMsg);
	( msg->type == FRAME_END)? (done = 1) : (done = 0);
    }
    segment_receive_indicator_off();

#endif  /* ] */

#endif /* ] */
    return FALSE;
}


/*****************************************************************
 * client_virtual_time
 * 
 * 	Server says update your virtual time of day.
 * 	
 * Inputs:
 *		m	Message.
 * Outputs:
 *		FALSE 	to keep client going.
 */
static boolean_type client_virtual_time(
    struct IPCMsg *m)
{
    struct SrvVirtualTime *vt_msg = (struct SrvVirtualTime *)m;
    daylight_type daylight;

    daylight.use_server_time = TRUE;
    daylight.static_time     = vt_msg->static_time;
    daylight.initial_time    = time(NULL);
    daylight.reference_time  = vt_msg->reference_time;
    daylight.mask            = 0x03;	/* ? */

    daylight_update(img_fildes,&daylight,TRUE);

    return FALSE;
}


static void autoshift(
    float yval)
{
    int actual_save;

    /* Only do the automatic shift stuff every three updates, and then
     * only if the gears are positive. (ie, don't mess with it if the
     * guy is in neutral or reverse.
     */
    if ((++gear.updates >= 3) && (gear.actual > 1)) {
	gear.updates = 0;
	actual_save = gear.actual;
	/* Figure out what gear the weenie should actually be in */
	if (cstate.rpm > 7000.0) {
	    /* better shift him up one */
	    if ((++gear.actual) >= gear.num_gears) {
		gear.actual = gear.num_gears-1; 
	    }
	}
	else if ((cstate.rpm > 4000.0) && (yval > 0.0)) {
	    if ((++gear.actual) >= gear.num_gears) { 
	       gear.actual = gear.num_gears-1;
	   }
	}
	else if ((cstate.rpm < 3000.0) && (yval < 0.0)) {
	    if ((--gear.actual) < 2) gear.actual = 2; 
	}
	else if ((cstate.rpm < 1500.0) && yval > 0.3) {
	    /* Vehicle is stalling -- time to shift down */
	    if ((--gear.actual) < 2) gear.actual = 2; 
	}

	if (actual_save != gear.actual) {
	    gearDraw(&gear);
	}
    }
}


#define FLOOR_IT(msgptr) \
{ \
    (msgptr)->x_val =  0.0; \
    (msgptr)->y_val =  1.0; \
    if (gear.actual < 2) gear.actual = 2; \
    else autoshift(1.0); \
    (msgptr)->gear_wanted = gear.actual; \
    (msgptr)->key_mask = 0; \
    (msgptr)->keypresses = 0; \
}


#ifdef WIN32
#define GET_OFFSET_TIME(timeptr) \
{  struct _timeb tmp; \
    _ftime(&tmp);  \
    (timeptr)->tv_sec = tmp.time; \
    (timeptr)->tv_usec = tmp.millitm * 1000; \
    if (((timeptr)->tv_usec -= cstate.racestart.tv_usec) < 0) { \
	--((timeptr)->tv_sec); \
	(timeptr)->tv_usec += 1000000; \
    } \
    (timeptr)->tv_sec -= cstate.racestart.tv_sec; \
}
#else
#define GET_OFFSET_TIME(timeptr) \
{   struct timezone tz; \
    gettimeofday((timeptr),&tz); \
    if (((timeptr)->tv_usec -= cstate.racestart.tv_usec) < 0) { \
	--((timeptr)->tv_sec); \
	(timeptr)->tv_usec += 1000000; \
    } \
    (timeptr)->tv_sec -= cstate.racestart.tv_sec; \
}
#endif

#define FLOATTIME(t1) \
    ((float) (t1).tv_sec + (float) (t1).tv_usec / 1000000.0)


#define TIMEDIFF(t1,t2) \
    (  (float) ((t2).tv_sec  - (t1).tv_sec) \
    + ((float) ((t2).tv_usec - (t1).tv_usec) / 1000000.0))
	

static struct timeval last_read_input = {0,0},
	next_read_input = {0,0};
static float timediff = EPSILON;

struct CliInput 
    nextMsg,
    lastMsg;

/*****************************************************************
 * client_send_input
 * 
 * 	Poll client input and send to server.
 * 	
 */
static void client_send_input(
    void)
{
    /* TBD */
    struct CliInput 
	Msg;

#if !(defined(WIN32) || defined(MAC))
    Window root, child;
    int root_x, root_y;
    Bool same_screen;
#endif
    int win_x,win_y; 
    unsigned int keys_buttons;
    unsigned int stick_buttons;
    float xval, yval;
    float xval1, yval1;
    int   intx, inty;
    float divisor;
    int sign;
    char *send_ptr;
    static int trigger1=0, thumb1=0;
    static int last_sound = -1;
    int squealed_or_screeched = 0;
    static float last_speed = 0;

    /* Set buffer pointer to beginning of buffer. */
    send_ptr = send_buf;

    /* Write message header */
    Msg.MsgType = CLI_INPUT;
    Msg.MsgLength = CLI_INPUT_SIZE;

    if (cstate.playback_fildes != INVALID) {
	/* Read it from a file */
	struct timeval tm;

	if (cstate.racestart.tv_sec == 0) {
	    /* Haven't crossed starting line yet. */
	    FLOOR_IT(&Msg);
	}
	else {
	    float blend1,blend2;
	    GET_OFFSET_TIME(&tm);

	    if ((tm.tv_sec > next_read_input.tv_sec)
		    || ((tm.tv_sec == next_read_input.tv_sec)
			&& (tm.tv_usec > next_read_input.tv_usec))) {
		/* Need new values */
		do {
		    /* move next to last */
		    memcpy(&last_read_input,&next_read_input,
			sizeof(struct timeval));
		    memcpy(&lastMsg,&nextMsg,
			sizeof(pointer_msg_type));

		    if (read(cstate.playback_fildes,&next_read_input,
				sizeof(struct timeval))
			    != sizeof(struct timeval)) {
			cstate.racestart.tv_sec = cstate.racestart.tv_usec = 0;
			FLOOR_IT(&nextMsg);
			break;
		    }
		    if (read(cstate.playback_fildes,&nextMsg,
				sizeof(struct CliInput))
			    != sizeof(struct CliInput)) {
			cstate.racestart.tv_sec = cstate.racestart.tv_usec = 0;
			FLOOR_IT(&nextMsg);
			break;
		    }
		} while ((tm.tv_sec > next_read_input.tv_sec)
		    || ((tm.tv_sec == next_read_input.tv_sec)
			&& (tm.tv_usec > next_read_input.tv_usec)));
		if ((timediff = TIMEDIFF(last_read_input,next_read_input))
			< EPSILON) {
		    timediff = EPSILON;
		}
	    }

	    blend2 = TIMEDIFF(last_read_input,tm)/timediff;
	    blend1 = 1.0 - blend2;
	    if (debug) {
		printf("time %f, %f%% from %f to %f\n",
		    FLOATTIME(tm),blend2,FLOATTIME(last_read_input),
		    FLOATTIME(next_read_input));
	    }
			
	    Msg.x_val = blend1*lastMsg.x_val
		+ blend2*nextMsg.x_val;
	    Msg.y_val = blend1*lastMsg.y_val
		+ blend2*nextMsg.y_val;
	    if (blend1 > 0.5) {
		Msg.gear_wanted = lastMsg.gear_wanted;
		Msg.key_mask    = lastMsg.key_mask;
		Msg.keypresses  = lastMsg.keypresses;
	    }
	    else {
		Msg.gear_wanted = nextMsg.gear_wanted;
		Msg.key_mask    = nextMsg.key_mask;
		Msg.keypresses  = nextMsg.keypresses;
	    }
	    if (Msg.gear_wanted != gear.actual) {
		gear.actual = Msg.gear_wanted;
		gearDraw(&gear);
	    }
	 }
    }
    else if (cstate.use_joystick
#if !(defined(WIN32) || defined(MAC))
        && (cstate.joystick != NULL)
#endif
    ) {
#if !(defined(WIN32) || defined(MAC))
	cstate.joystick->Valid = 3; /* Tell the daemon we are still here */
#endif
	get_joystick_values(&xval, &yval, &xval1, &yval1, &intx, &inty, &stick_buttons );
	keys_buttons = 0;

	/* Check the buttons for Turbos, Shells, and Mines */
	if( (stick_buttons & (TRIGGER2 | THUMB2)) == (TRIGGER2 | THUMB2) ) {
             /* drop  a mine */
	     keys_buttons |= 2 | (HW_KBD_MOD_SHIFT << 4);
	}
	else if( stick_buttons & THUMB2) {
	     keys_buttons |= 2;              /* fire a shell */
	}
	else if( stick_buttons & TRIGGER2)
	{
	    cstate.mode |= CLIENT_TURBO_MODE;		/* hit some turbo */
	}
	if( stick_buttons & THUMB1 ) {
	    if( !trigger1 ) process_button(1);  	/* shift up */
	    trigger1 = 1;
	}
	else {
	    trigger1 = 0;
	}
	if( stick_buttons & TRIGGER1 ) {
	    if( !thumb1 ) process_button(3);  		/* shift down */
	    thumb1 = 1;
	}
	else {
	    thumb1 = 0;
	}

	/* Write pointer message into buffer. */
	xval =  xval * 1.1;
	sign = (xval < 0)? -1 : 1;
	xval = xval * xval;
	if( xval > 1.0) xval = 1.0;
	Msg.x_val = xval * sign;

	yval =  yval * 1.1;
	if( yval > 1.0) yval = 1.0;
	if( yval < -1.0) yval = -1.0;

	if (cstate.mode & CLIENT_TURBO_MODE) {
	    yval *= 10.0;
	    cstate.mode &= (~CLIENT_TURBO_MODE);
	}

	Msg.y_val = -yval;

	if( cstate.throttle < ( cstate.desired_throttle-(THROTTLE_DELTA*0.9)))
	    cstate.throttle += THROTTLE_DELTA;
	else if( cstate.throttle>(cstate.desired_throttle+(THROTTLE_DELTA*0.9)))
	    cstate.throttle -= THROTTLE_DELTA;

	if( cstate.throttle < (THROTTLE_DELTA/1.2) ) cstate.throttle = 0.0;
	Msg.throttle =  cstate.throttle;

	if (gear.type == GEAR_AUTOMATIC) {
	    autoshift(-yval);
	}

	Msg.gear_wanted = gear.actual;
	Msg.key_mask = keys_buttons;

	/* set these so that sound will work below */
	xval = Msg.x_val;
	yval = -yval;
    }
    else {
#if defined(WIN32) || defined(MAC)
        win_x = cstate.mouseX;
        win_y = cstate.mouseY;
        keys_buttons = cstate.mouseMods;
#else
	/* Query the X server for mouse location */
	same_screen = XQueryPointer(cstate.display, cstate.graphicsWindow,
	    &root, &child, &root_x, &root_y, &win_x, &win_y, &keys_buttons);
#endif

	divisor = 5.0;
#if defined(WIN32) || defined(MAC)
	if (1)
#else
	if (same_screen)
#endif
        {
	    xval =  (((float)win_x/(float)cstate.gWinWidth)*2.0 - 1.0)
		* divisor;

	    if (xval < 0.0 ) {
	      sign = -1; 
	      xval = -xval;
	    }
	    else {
		sign = 1; 
	    }

	    if ( xval <= 1.0) {
	       xval =  xval * xval;
	       xval *= 0.1;
	    }
	    else if (xval <= 4.5 ) {
	       /*range between 0.1 and 0.8 */
	       xval = (xval - 1.0) * (0.7/3.5) + 0.1 ; 
	    }
	    else {
	      /* range between 0.8 and 1.0 */
	      xval = (xval - 4.5) * (0.1/0.5) + 0.8;
	    }
	    xval *= sign;
	    yval = - ((2.0 * (float)win_y / (float)cstate.gWinHeight) - 1.0);
	    yval *= 1.5;

	    xval = BOUND(-1.0, xval, 1.0);
	    yval = BOUND(-1.0, yval, 1.0);
	}
	else {
	    xval = 0.0;
	    yval = 0.0;
	}

	if (cstate.mode & CLIENT_KEYACC_MODE) {
	    /* get yval from keys, not from pointer */
	    if (!(cstate.mode &
		    (CLIENT_CRUISE_CONTROL_MODE|CLIENT_USE_KEYACCEL))) {
		if (cstate.acc_value > ACC_INC) {
		    cstate.acc_value -= ACC_INC;
		}
		else if (cstate.acc_value < (-ACC_INC*3) ) {
		    cstate.acc_value += (ACC_INC*3);
		}
		else {
		    cstate.acc_value = 0.0;
		}
	    }
	    yval = cstate.acc_value;
	    cstate.mode &= (~CLIENT_USE_KEYACCEL);
	}

	if (cstate.mode & CLIENT_BRAKES_ON) {
	    yval = -1.0;
	    cstate.mode &= (~CLIENT_BRAKES_ON);
	}

	if (cstate.mode & CLIENT_TURBO_MODE) {
	    yval *= 10.0;
	    cstate.mode &= (~CLIENT_TURBO_MODE);
	}

	/* Write pointer message into buffer. */
	Msg.x_val =  xval;
	Msg.y_val =  yval;
	if( cstate.throttle < ( cstate.desired_throttle-(THROTTLE_DELTA*0.9)))
	    cstate.throttle += THROTTLE_DELTA;
	else if( cstate.throttle>(cstate.desired_throttle+(THROTTLE_DELTA*0.9)))
	    cstate.throttle -= THROTTLE_DELTA;

	if( cstate.throttle < (THROTTLE_DELTA/1.2) ) cstate.throttle = 0.0;
	Msg.throttle =  cstate.throttle;

	if (gear.type == GEAR_AUTOMATIC) {
	    autoshift(yval);
	}

	Msg.gear_wanted = gear.actual;
	Msg.key_mask = keys_buttons;
    }

    /* try to figure out if a sound should be played here */
    if( (yval >= 0.98) && ( cstate.mph < 30.0 ) ) {
	    if( last_sound != SOUND_PEAL ) {
		play_sound(SOUND_PEAL); /* Peel out */
		last_sound = SOUND_PEAL;
	    }
	    squealed_or_screeched = 1;
    } 
    else if ((yval <= 0.98) || ( cstate.mph < 30.0 ) ){
	stop_sound( SOUND_PEAL );
    }
    if( (yval <= -0.98) && ( cstate.mph > 50.0 ) ) {
	    if( last_sound != SOUND_SCREECH ) {
		play_sound(SOUND_SCREECH);  /* Slam on brakes */
		last_sound = SOUND_SCREECH;
	    }
	    squealed_or_screeched = 1;
    }
    else {
	stop_sound( SOUND_SCREECH );
    }

    if( ((xval >= 0.80) || (xval <= -0.80)) && (yval > -0.98) ) {
	if( cstate.mph > 60.0 )
	    if( last_sound != SOUND_SQUEAL ) {
		play_sound(SOUND_SQUEAL);  /* Squeal around the corner */
		last_sound = SOUND_SQUEAL;
	    }
	    squealed_or_screeched = 1;
    }
    else {
	stop_sound( SOUND_SQUEAL );
    }

    if( !squealed_or_screeched ) {
	last_sound = -1;
    }

    if( Msg.key_mask & 2) play_sound(SOUND_SHOOT);

    /* Check to see if our speed took a sudden big drop.  If so, assue
    ** we have collided with something */
    if( cstate.mph < last_speed - 40.0 ) {
	play_sound( SOUND_COLLISION );
    }
    last_speed = cstate.mph;

    /* Send Message. */

    if( !PutMsg((struct IPCMsg *)&Msg, server_socket->socketnum)) {
	perror("PutMsg in client_send_input");
    }
    FlushMsg(server_socket->socketnum);


    if ((cstate.record_fildes != INVALID)
	    && (cstate.racestart.tv_sec > 0)) {
	/* Write it to a file */
	struct timeval tm;

	GET_OFFSET_TIME(&tm);
	write(cstate.record_fildes,&tm,sizeof(struct timeval));
	write(cstate.record_fildes,&Msg,sizeof(struct CliInput));
    }
    
    processXEvents();
}


/*****************************************************************
 * client_beginframe
 * 
 * 	Process a new frame from the server.
 * 	
 * Inputs:
 *		m	Message.
 * Outputs:
 *		FALSE 	to keep client going.
 */
static boolean_type client_beginframe(
    struct IPCMsg *m)
{
    client_send_input();

    return FALSE;
}



/*****************************************************************
 * client_identify
 * 
 *     Get connection id from server
 * 	
 * Inputs:
 *		m	Message.
 * Outputs:
 *		FALSE 	to keep client going.
 */
static boolean_type client_identify(
    struct IPCMsg *m)
{
    struct SrvIdentify *mmsg;
    float fversion;

    /* Point at identify message in receive buffer and increment
     * receive buffer pointer.
     */
    mmsg = (struct SrvIdentify *)m;

    connection_id = mmsg->connection;

#ifndef NO_VERSION_CHECKING
    /* Check version number sent from server to see if it is 
     * the same as the version number from the client.
     */
    sscanf(version, "$Revision: %f", &fversion);
    if (mmsg->version != (int) floor(fversion)) {
	fprintf(stderr,
		"Drive client and server version numbers do not match.\n" );
	fprintf(stderr,
	    "Please update client and server to same version.\n" );
	exit(0);
    }
    else if (debug) {
	fprintf(stderr, "Client and server are same version: %d %d \n",
		mmsg->version, (int)floor(fversion));
    }
#endif /* NO_VERSION_CHECKING */

    if (debug) {
	printf("Connection id from server is:  %d\n",connection_id);
    }

    /* Get time information from server. */
    cstate.daylight.use_server_time = mmsg->use_server_time;

    daylight_update(img_fildes, &cstate.daylight, TRUE);

    return FALSE;
}


/*****************************************************************
 * client_driveable
 * 
 * 	get information about a driveable vehicle
 * 	
 * Inputs:
 *		m	Message.
 * Outputs:
 *		FALSE 	to keep client going.
 */
static boolean_type client_driveable(
    struct IPCMsg *m)
{
    struct SrvDriveable *dMsg = (struct SrvDriveable *)m;
    Driveable  *car;


    /* Increment input buffer pointer past gauge message. */
    car = &(dMsg->driveable);

    memcpy(&(driveables[num_cars]),car,sizeof(Driveable));

    if (debug) {
	printf(" Vehicle Name:  %s\n",driveables[num_cars].name);
	printf(" Vehicle display list: %d\n",
	       driveables[num_cars].display_list);
	printf(" Vehicle horsepower :  %f\n",driveables[num_cars].horsepower);
    }

    ++num_cars;

    return FALSE;
}


/*****************************************************************
 * client_start_lap
 * 
 *     client hit a checkpoint object
 * 	
 * Inputs:
 *		m	Message.
 * Outputs:
 *		FALSE 	to keep client going.
 */
static boolean_type client_start_lap(
    struct IPCMsg *m)
{
    struct SrvCheckpoint *cmsg;

    cmsg = (struct SrvCheckpoint *)m;

    if (debug) {
	printf("Received Checkpoint Message!\n");
	printf("Time: %f\n",cmsg->time);
	printf("Checkpoint number: %d\n",cmsg->checkpoint);
	printf("Checkpoints visited: %d\n",cmsg->visited);
	printf("Number of Checkpoints : %d\n\n",cmsg->num_checkpoints);
    }

    cstate.lap_time = 0.0;
    cstate.checkpoint.time = cmsg->time;
    cstate.checkpoint.checkpoint = cmsg->checkpoint;
    cstate.checkpoint.visited = cmsg->visited;
    cstate.checkpoint.num_checkpoints = cmsg->num_checkpoints;
    redrawCheckpoint();

    return FALSE;
}


/*****************************************************************
 * client_checkpoint
 * 
 *     client hit a checkpoint object
 * 	
 * Inputs:
 *		m	Message.
 * Outputs:
 *		FALSE 	to keep client going.
 */
static boolean_type client_checkpoint(
    struct IPCMsg *m)
{
    struct SrvCheckpoint *cmsg;

    cmsg = (struct SrvCheckpoint *)m;


    if (debug) { 
	printf("Received Checkpoint Message!\n");
	printf("Time: %f\n",cmsg->time);
	printf("Checkpoint number: %d\n",cmsg->checkpoint);
	printf("Checkpoints visited: %d\n\n",cmsg->visited);
    }
    cstate.checkpoint.time = cmsg->time;
    cstate.checkpoint.checkpoint = cmsg->checkpoint;
    cstate.checkpoint.visited = cmsg->visited;
 
    redrawCheckpoint();

    return FALSE;
}

/*****************************************************************
 * client_finish_lap
 * 
 *     client hit a checkpoint object
 * 	
 * Inputs:
 *		m	Message.
 * Outputs:
 *		FALSE 	to keep client going.
 */
static boolean_type client_finish_lap(
    struct IPCMsg *m)
{
    struct SrvCheckpoint *cmsg;

    cmsg = (struct SrvCheckpoint *)m;

    if (debug) {
	printf("Received Checkpoint Message!\n");
	printf("Time: %f\n",cmsg->time);
	printf("Checkpoint number: %d\n",cmsg->checkpoint);
	printf("Checkpoints visited: %d\n",cmsg->visited);
    }

    if (cmsg->checkpoint == -1) {
	/* oops -- the client missed some checkpoints */
	if (debug) printf("Oh oh!!! You missed at least one checkpoint! \n");
    }
    else {
	cstate.lap_time = cmsg->time;
    }

    cstate.checkpoint.time = cmsg->time;
    cstate.checkpoint.checkpoint = cmsg->checkpoint;
    cstate.checkpoint.visited = cmsg->visited;
    cstate.lap_time = cmsg->time;
    redrawCheckpoint();

    if (debug) printf("\n");

    cstate.racestart.tv_sec = cstate.racestart.tv_usec = 0;

    return FALSE;
}


/*****************************************************************
 * client_update_state
 * 
 * 	Server wants us to be rotating that vehicle again.
 * 	
 * Inputs:
 *		m	Message.
 * Outputs:
 *		FALSE 	to keep client going.
 */
static boolean_type client_update_state(
    struct IPCMsg *m)
{
    struct SrvUpdateState *sMsg = (struct SrvUpdateState *)m;

    cstate.state = sMsg->state;

    updateStateWindow(cstate.state);

    /* Reset the X Fighter's controls so that it starts out in a 
     * controllable state when the next state becomes active */
    if(strcmp(cstate.car_name, "X Fighter") == 0)
    {
       cstate.throttle = 0.0;
       cstate.desired_throttle = 0.0;
#if !(defined(WIN32) || defined(MAC))
       XWarpPointer(cstate.display,None,cstate.graphicsWindow,
	   0,0,0,0,(cstate.gWinWidth / 2),(cstate.gWinHeight /2) );
#endif
    }

    if (cstate.state == POST_RACE_STATE) {
	if(cstate.use_sound)
	    update_rpm(1000);

	cstate.rpm = 1000;
	cstate.mph = 0.0;
	auto_rotate_vehicle();
    }

    if ((cstate.record_fildes != INVALID)
	    || (cstate.playback_fildes != INVALID)) {
	if (cstate.state == RACE_STATE) {
#ifndef WIN32
	    /* Just started race! */
	    struct timezone tz;
	    gettimeofday(&(cstate.racestart),&tz);
#endif
	}
	else {
	    cstate.racestart.tv_sec = cstate.racestart.tv_usec = 0;
	    if (cstate.playback_fildes != INVALID) {
		/* rewind playback file */
		if (lseek(cstate.playback_fildes,0,0) != 0) {
		    perror("lseek on playback file");
		}
		last_read_input.tv_sec = (last_read_input.tv_usec = 0);
		next_read_input.tv_sec = (next_read_input.tv_usec = 0);
		FLOOR_IT(&lastMsg);
		FLOOR_IT(&nextMsg);
		timediff = EPSILON;
	    }
	    else if ((cstate.record_fildes != INVALID)
		    && (cstate.state == POST_RACE_STATE)) {
		/* Finished.  Close it and wrap up. */
		close(cstate.record_fildes);
		cstate.record_fildes = INVALID;
	    }
	}
    }
    if( cstate.state == PRE_RACE_STATE ) {
	play_sound( SOUND_KLAXON );
	if(cstate.use_sound)
	    update_rpm(1000);
	cstate.rpm = 1000;
	cstate.mph = 0.0;
    }

    return FALSE;
}


/*****************************************************************
 * client_standing
 * 
 * 	Server tells us what place we finished the race
 * 	
 * Inputs:
 *		m	Message.
 * Outputs:
 *		FALSE 	to keep client going.
 */
static boolean_type client_standing(
    struct IPCMsg *m)
{
    struct SrvStanding *st_msg = (struct SrvStanding *)m;

    if (st_msg->user[0] == '\0') {
	if (st_msg->flags & STANDINGS_TYPE_CURRENT)
	    updateRacePositionType(FALSE,st_msg->position);
	else 
	    updateRacePositionType(TRUE,st_msg->position);
    }
    else {
	updateRacePosition(st_msg->user,st_msg->position,
	    ((st_msg->flags & STANDINGS_TYPE_PERSONAL) != 0),
	    st_msg->r,st_msg->g,st_msg->b);
	if ((st_msg->flags & STANDINGS_TYPE_FINAL)
		&& (st_msg->flags & STANDINGS_TYPE_PERSONAL))
	    showPlace(st_msg->position);
    }

    return FALSE;
}


/*****************************************************************
 * client_explosion
 * 
 * 	Server says do an explosion here.
 * 	
 * Inputs:
 *		m	Message.
 * Outputs:
 *		FALSE 	to keep client going.
 */
static boolean_type client_explosion(
    struct IPCMsg *m)
{
    struct SrvExplosion *exp_msg = (struct SrvExplosion *)m;

    explosion_create(exp_msg->x,exp_msg->y,exp_msg->z,
	1.0,1.0,0.0, exp_msg->radius);
    
    play_sound( SOUND_EXPLOSION );
    return FALSE;
}


/*****************************************************************
 * client_read
 * 
 */
static char *msgNames[] = {
    "CLI_CLOSE",
    "CLI_REGISTER",
    "CLI_INPUT",
    "CLI_DISCONNECT",
    "CLI_READY",
    "CLI_RESTART",
    "CLI_LIGHTS",
    "CLI_UPRIGHT",
    "CLI_NEW_VEHICLE",

    "SRV_CLOSE",
    "SRV_DEFINE",
    "SRV_BEGINFRAME",
    "SRV_IDENTIFY",
    "SRV_DRIVEABLE",
    "SRV_START_LAP",
    "SRV_CHECKPOINT",
    "SRV_FINISH_LAP",
    "SRV_UPDATE_STATE",
    "SRV_STANDING",
    "SRV_EXPLOSION",
    "SRV_DEFINE_MANY",
    "SRV_VIRTUAL_TIME",
    "SRV_GRAPHIC",
    "SRV_SEGLIST",

    "SRV_FRAME_END",
    "SRV_FRAME_POSITION",
    "SRV_FRAME_GAUGE",
    "SRV_FRAME_MATRIX",
    "SRV_FRAME_UPDATE",
    "SRV_FRAME_STATIC_SEG",
    "SRV_FRAME_RADAR",
    "SRV_FRAME_GLOBAL_NAMESET",
    "SRV_DIE"
};

boolean_type client_read(
    void)
{
    boolean_type done = FALSE;
    struct IPCMsg *Msg;
#if defined(MESSAGE_HEADER_READ)
    int status;
#endif
    
    Msg = GetMsg( server_socket->socketnum );
    while( Msg ) {
	switch( Msg->MsgType ) {
	case SRV_DIE:
	case SRV_CLOSE:
	case IPC_KILL :
	case IPC_TERM :
	    (void)puts( "IPC_KILL received; shutting down" );
	    /* Now, kill the IPC and die */
	    client_close( Msg );
	    TermIPC( server_socket->socketnum );
	    exit( 1 );
	    /* NOTREACHED */
	    break;

	case SRV_DEFINE:
	    done = client_define(Msg);
	    break;
	case SRV_BEGINFRAME:
	    done = client_beginframe(Msg);
	    break;
	case SRV_IDENTIFY:
	    done = client_identify(Msg);
	    break;
	case SRV_DRIVEABLE:
	    done = client_driveable(Msg);
	    break;
	case SRV_START_LAP:
	    done = client_start_lap(Msg);
	    break;
	case SRV_CHECKPOINT:
	    done = client_checkpoint(Msg);
	    break;
	case SRV_FINISH_LAP:
	    done = client_finish_lap(Msg);
	    break;
	case SRV_UPDATE_STATE:
	    done = client_update_state(Msg);
	    break;
	case SRV_STANDING:
	    done = client_standing(Msg);
	    break;
	case SRV_EXPLOSION:
	    done = client_explosion(Msg);
	    break;
	case SRV_DEFINE_MANY:
	    break;
	case SRV_VIRTUAL_TIME:
	    done = client_virtual_time(Msg);
	    break;

	case SRV_FRAME_END:
	    frame_end(Msg);
	    break;
	case SRV_FRAME_POSITION:
	    frame_position(Msg);
	    break;
	case SRV_FRAME_GAUGE:
	    frame_gauge(Msg);
	    break;
	case SRV_FRAME_MATRIX:
	    frame_matrix(Msg);
	    break;
	case SRV_FRAME_UPDATE:
	    frame_update(Msg);
	    break;
	case SRV_FRAME_STATIC_SEG:
	    frame_static_seg(Msg);
	    break;
	case SRV_FRAME_RADAR:
	    frame_radar(Msg);
	    break;
	case SRV_FRAME_GLOBAL_NAMESET:
	    frame_global_nameset(Msg);
	    break;

	case SRV_GRAPHIC :
	case SRV_SEGLIST :
	    createHwSegmentFromMsg( Msg );
	    break;

	default:
	    printf("Unknown Message type: %d\n", Msg->MsgType);
	}
	Msg = GetMsg( server_socket->socketnum );

    }	/* End while Msg */

    return done;
#if defined(MESSAGE_HEADER_READ)
    /* Read message header. */
    status = soc_read(server_socket,
	(char *) &msg, sizeof( message_header_type ));
    
    switch ( status ) {
      case -1:
	/* Bad read. Scream and die. */
	perror( "Client read" );
	done = TRUE;
	break;
	
      case 0:
	/* Server is not active anymore. */
	fprintf(stderr, "Server terminated.\n");
	close(server_socket->socketnum);
	done = TRUE;
	break;
	
      default:
	/* Good read.  Messages cause calls to the functions in the 
	 * client table above.  The table is indexed by the type of
	 * message.
	 */
	if ((msg.type < (sizeof(client_table) >> 2)) 
		&& (msg.type >= 0)) {
	    done = (*client_table[msg.type])(&msg);
	}
	else if (debug) {
	    done = client_error(&msg);
	    if (debug) {
		fprintf( stderr, "Invalid message type: %d \n", msg.type);
		fprintf( stderr, "Table size: %d\n", sizeof(client_table)>>2 );
	    }
	}
    }

    return done;
#endif
}


void client_message_initialize(
    void)
{
    /* Malloc room for the packet receive buffer. */
    recv_buf = (char *) malloc(
	(MAX_OBJECTS_PER_SCENE+MAX_PLAYERS) * sizeof(matrix_msg_type)
	+ (MAX_OBJECTS_PER_SCENE+MAX_PLAYERS+2) * sizeof(any_msg_type) 
	+ sizeof(camera_arg));

    /* Allocate space for the packet send buffer. */
    send_buf = (char *) malloc(sizeof(any_msg_type));
}


/*****************************************************************
 * client_register
 * 
 * 	Register this client with the server process. 
 * 	
 */
void client_register(
    void)
{

    struct CliRegister
	Msg;

    /* Write header  */
    Msg.MsgType = CLI_REGISTER;
    Msg.MsgLength = CLI_REGISTER_SIZE;

    /* Write register message into buffer.  Both fields are fixed-size and
     * are not guaranteed to be NUL-terminated by the source APIs.
     */
    memset( Msg.hostname, 0, sizeof(Msg.hostname) );
    memset( Msg.username, 0, sizeof(Msg.username) );
    gethostname( Msg.hostname, sizeof(Msg.hostname) - 1 );
#ifdef WIN32
    cuserid( Msg.username );
#else
    {  struct passwd *p = getpwuid(geteuid());
        if (p) strncpy(Msg.username, p->pw_name, sizeof(Msg.username) - 1);
    }
#endif
    if ( (cstate.mode & CLIENT_WATCH_MODE) ||
	 (cstate.mode & CLIENT_AUTOWATCH_MODE) )
	Msg.mode = CLIENT_WATCH_MODE;
    else
	Msg.mode = cstate.mode;
    memcpy(&(Msg.start_pos),&cstate.start_pos, sizeof(drive_start_type));
    
    if( !PutMsg((struct IPCMsg *)&Msg, server_socket->socketnum)) {
	perror("PutMsg in client_register");
    }
    FlushMsg(server_socket->socketnum);

}



/*****************************************************************
 * client_quit
 * 
 */
void client_quit(
    void)
{
    struct IPCMsg 
	Msg;

    /* Notify the Joystick daemon process (if any) that we are quitting */
    if( cstate.use_joystick )
    {
#if !(defined(WIN32) || defined(MAC))
	cstate.joystick->Valid = -1;
#endif
    }
    /* Shut down the sound daemon process (if any ) */
    terminate_audio();

    /* Restore the Pointer values */

#ifdef SET_POINTER
    XChangePointerControl(cstate.display,1,1,save_XPointer_numerator,
	save_XPointer_denominator,save_XPointer_threshold);
#endif

    Msg.MsgType = CLI_DISCONNECT;
    Msg.MsgLength = 0;

    if( !PutMsg((struct IPCMsg *)&Msg, server_socket->socketnum)) {
	perror("PutMsg in client_quit");
    }
    FlushMsg(server_socket->socketnum);

}


/*****************************************************************
 * client_restart
 * 
 */
void client_restart(
    void)
{
    struct IPCMsg 
	Msg;

    /* don't want to do this if we aren't driving */
    if ((cstate.car_selected) || (cstate.mode & CLIENT_AUTOSTART_MODE))  {
	Msg.MsgType = CLI_RESTART;
	Msg.MsgLength = 0;
	if( !PutMsg((struct IPCMsg *)&Msg, server_socket->socketnum)) {
	    perror("PutMsg in client_restart");
	}
	FlushMsg(server_socket->socketnum);
    }

    cstate.checkpoint.time = 0.0;
    cstate.checkpoint.checkpoint = 0;
    cstate.checkpoint.visited = 0;
    cstate.checkpoint.num_checkpoints = 0;
    redrawCheckpoint();
}


/*****************************************************************
 * client_upright
 * 
 */
void client_upright(
    void)
{
    struct IPCMsg
	Msg;

    if (cstate.mode & (CLIENT_WATCH_MODE|CLIENT_AUTOWATCH_MODE)) return;

    /* don't want to do this if we aren't driving */
    if ((cstate.car_selected) || (cstate.mode & CLIENT_AUTOSTART_MODE))  {
	Msg.MsgType = CLI_UPRIGHT;
	Msg.MsgLength = 0;

	if( !PutMsg((struct IPCMsg *)&Msg, server_socket->socketnum)) {
	    perror("PutMsg in client_upright");
	}
	FlushMsg(server_socket->socketnum);
    }
}
