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
 * message.h - Message definitions.
 */

#ifndef _MESSAGE_INCLUDED
#define _MESSAGE_INCLUDED

/* Message structure definitions. */

/* Message header type.  This contains the slots that are ALWAYS 
 * available in any message.
 */
typedef struct {
    int type;
    union {
	int size;
	int value;
	boolean_type flag;
    } data;
} message_header_type;

/* Client registration message type. */
typedef struct
{
    char hostname[10];
    char username[10];
    unsigned int mode;
    drive_start_type start_pos;
} register_msg_type;


#define KEYPRESS_0	0x00000001
#define KEYPRESS_1	0x00000002
#define KEYPRESS_2	0x00000004
#define KEYPRESS_3	0x00000008
#define KEYPRESS_4	0x00000010
#define KEYPRESS_5	0x00000020
#define KEYPRESS_6	0x00000040
#define KEYPRESS_7	0x00000080
#define KEYPRESS_8	0x00000100
#define KEYPRESS_9	0x00000200
#define KEYPRESS_T	0x00000400
typedef unsigned int KEYPRESSES;


typedef struct
{
    float x_val;
    float y_val;
    int gear_wanted;
    unsigned int key_mask;		/* Modifier keys from keysym.h */
    KEYPRESSES keypresses;		/* Other keys passed to server */
    float throttle;			/* for fighter ship types and things*/
} pointer_msg_type;

typedef struct
{
    float mph;
    float rpm;
    float x_val,y_val;
    float altitude;
    int   num_turbos;
    int   num_shells;
} gauge_msg_type;

typedef struct
{
    int segment;
    int connection;
    float xform[4][4];
} matrix_msg_type;

typedef struct
{
    float xform[4][4];
    float alt_view_xform[4][4];
} position_msg_type;

typedef struct
{
    float time;
    int   checkpoint;
    int   num_checkpoints;
    int   visited;
} checkpoint_msg_type;

typedef struct
{
    int connection;			/* Connection ID. */
    int version;			/* Server version. */
    boolean_type use_server_time;	/* TRUE if server is dictating time. */
    char timezone_string[20];		/* Time zone for client to use. */
    char time_string[8];		/* Time of day for client to use. */
    char date_string[10];		/* Date for client to use. */
    boolean_type static_time;		/* TRUE if time should not change. */
} identify_msg_type;

typedef struct
{
    float x_pos, y_pos, z_pos;
    float r, g, b;
} radar_msg_type;

#define STANDINGS_TYPE_CURRENT	(1<<0)
#define STANDINGS_TYPE_FINAL	(1<<1)
#define STANDINGS_TYPE_PERSONAL	(1<<2)	/* this client's position */
typedef struct
{
    char user[25];	/* Name and host of user */
    int position;	/* Position user finished in */
    unsigned int flags;	/* see above */
    float r,g,b;	/* Color of user's car */
} standing_msg_type;

typedef struct
{
    float x,y,z;	/* explosion location */
    float radius;	/* radius at maximum expansion */
} explosion_msg_type;

typedef struct
{
    unsigned int global_nameset_bits;
} global_nameset_msg_type;

typedef struct
{
    boolean_type static_time;		/* is it frozen? */
    time_t reference_time;		/* like returned by time() */
} virtual_time_msg_type;

/* The following structure isn't actually used, but its size determines
 * the largest message type.
 */
typedef struct {
    message_header_type	header;
    union {
	register_msg_type	reg;
	pointer_msg_type	pointer;
	gauge_msg_type		gauge;
	matrix_msg_type		matrix;
	position_msg_type	position;
	checkpoint_msg_type	checkpoint;
	identify_msg_type	identify;
	radar_msg_type		radar;
	standing_msg_type	standing;
	explosion_msg_type	explosion;
	global_nameset_msg_type global_nameset;
	virtual_time_msg_type   virtual_time;
    } u;
} any_msg_type;

/* Client to Server message definitions. */
#define SERVER_CLOSE		0
#define SERVER_REGISTER		1
#define SERVER_INPUT		2
#define SERVER_DISCONNECT 	3
#define SERVER_CREADY		4
#define SERVER_RESTART		5
#define SERVER_LIGHTS		6
#define SERVER_UPRIGHT		7
#define SERVER_NEW_VEHICLE	8
#define SERVER_MAXMSG		SERVER_NEW_VEHICLE

/* Server to Client message definitions. */
#define CLIENT_CLOSE		0
#define CLIENT_DEFINE		1
#define CLIENT_BEGINFRAME	2
#define CLIENT_IDENTIFY        	3
#define CLIENT_DRIVEABLE	4
#define CLIENT_START_LAP	5
#define CLIENT_CHECKPOINT	6
#define CLIENT_FINISH_LAP	7
#define CLIENT_UPDATE_STATE	8
#define CLIENT_STANDING		9
#define CLIENT_EXPLOSION	10
#define CLIENT_DEFINE_MANY	11
#define CLIENT_VIRTUAL_TIME	12
#define CLIENT_MAXMSG		CLIENT_VIRTUAL_TIME

/* Frame component type definitions. */
#define FRAME_END		0
#define FRAME_POSITION		1
#define FRAME_GAUGE		2
#define FRAME_MATRIX		3
#define FRAME_UPDATE		4
#define FRAME_STATIC_SEG	5
#define FRAME_RADAR		6
#define FRAME_GLOBAL_NAMESET	7
#define FRAME_MAXMSG		FRAME_GLOBAL_NAMESET

/* Return value declarations. */

extern message_header_type *msg_create(
    void);
extern void msg_dump(
    message_header_type *msg,
    char *s,
    int l);

#endif /* _MESSAGE_INCLUDED */
