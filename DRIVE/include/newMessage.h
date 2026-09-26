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


#ifndef NEW_MESSAGES_DEFINED /* [ */

#define NEW_MESSAGES_DEFINED

#include "global.h"

typedef int IPC_SOCK;

#ifdef WIN32
#define IPC_INVALID_SOCK INVALID_SOCKET
#else
#define IPC_INVALID_SOCK	-1
#endif

typedef enum {
    CLI_CLOSE =0,	/* User input sent to server */
    CLI_REGISTER,	/* Tell the server who we are */
    CLI_INPUT,		/* Tell the server to close down ?*/
    CLI_DISCONNECT,	/* Tell the server we are quitting */
    CLI_READY,		/* Tell the server we are ready for updates */
    CLI_RESTART,	/* Tell the server to restart us */
    CLI_LIGHTS,		/* Tell the server about our lights. */
    CLI_UPRIGHT,	/* Tell the server to upright our vehicle, please */
    CLI_NEW_VEHICLE,	/* Tell the server we have chosen a new vehicle */

    SRV_CLOSE,
    SRV_DEFINE,		/* Define a single display list segment */
    SRV_BEGINFRAME,	/* Begin a frame.  No longer necessary  */
    SRV_IDENTIFY,
    SRV_DRIVEABLE,
    SRV_START_LAP,
    SRV_CHECKPOINT,
    SRV_FINISH_LAP,
    SRV_UPDATE_STATE,
    SRV_STANDING,
    SRV_EXPLOSION, 	/* An explosion */
    SRV_DEFINE_MANY,	/* Define several display list segments */
    SRV_VIRTUAL_TIME,	
    SRV_GRAPHIC,	/* Define a graphic */
    SRV_SEGLIST,	/* Define a list of graphics */

    SRV_FRAME_END,	
    SRV_FRAME_POSITION,	
    SRV_FRAME_GAUGE,	
    SRV_FRAME_MATRIX,	
    SRV_FRAME_UPDATE,	
    SRV_FRAME_STATIC_SEG,
    SRV_FRAME_RADAR,	
    SRV_FRAME_GLOBAL_NAMESET,
    SRV_DIE
} MessageType;

/* Big bad KILL message to server */
#define	IPC_KILL	9999	/* Kill the server AND all the clients */
#define	IPC_TERM	9998	/* Remote end died... */

#define	MSG_SIZE		8
struct IPCMsg {
    SP_U32
	MsgType,
	MsgLength;
    SP_I16
	MsgData[2];
};

#define CLI_INPUT_SIZE 		(24)
struct CliInput {
    SP_U32
	MsgType,
	MsgLength;
    SP_FLOAT
	x_val,
	y_val,
	throttle;
    SP_I32
	gear_wanted;
    SP_U32
	keypresses,
	key_mask;
};

#define CLI_REGISTER_SIZE	(48)
struct CliRegister {
    SP_U32
	MsgType,
	MsgLength;
    SP_CHAR
	hostname[10],
	username[10];
    SP_U32
	mode;
    drive_start_type
	start_pos;
};

#define CLI_READY_SIZE		(sizeof(UpdateDisp))
struct CliReady {
    SP_U32
	MsgType,
	MsgLength;
    UpdateDisp
	upd;		/* UpdateDisplay structure */
};

#define CLI_RESTART_SIZE	(0)
struct CliRestart {
    SP_U32
	MsgType,
	MsgLength;
};

#define CLI_LIGHTS_SIZE		(4)
struct CliLights {
    SP_U32
	MsgType,
	MsgLength;
    SP_U32
	flags;
};

#define SRV_DEFINE_SIZE(n)	(n)
struct SrvDefine {
    SP_U32
	MsgType,
	MsgLength;
    SP_CHAR
	data[8*1024];			/* Maximum size */
};

#define	SRV_GRAPHIC_SIZE(n)	((n)+4)
struct SrvGraphic {
    SP_U32
	MsgType,
	MsgLength;
    SP_U32
	segnum;
    SP_CHAR
	data[1];
};

#define	SRV_SEGLIST_SIZE(n)	(4+(n)*17*4)
#define	SRV_SEGLIST_NUM(n)	(((n)-4)/(17*4))
struct SrvSeglistData {
    SP_U32
	segnum;
    SP_FLOAT
	mat[4][4];
};
struct SrvSeglist {
    SP_U32
	MsgType,
	MsgLength;
    SP_U32
	segnum;
    struct SrvSeglistData
	SegData[1];
};

#define SRV_IDENTIFY_SIZE 	(34)
struct SrvIdentify {
    SP_U32
	MsgType,
	MsgLength;
    SP_I32
	connection,
	version,
	static_time,
	use_server_time;
    SP_CHAR
	time_string[8],
	date_string[10];
};

#define SRV_DRIVEABLE_SIZE	(sizeof(Driveable))
struct SrvDriveable {
    SP_U32
	MsgType,
	MsgLength;
    Driveable
	driveable;
};

/* Used by SRV_CHECKPOINT, SRV_START_LAP, SRV_FINISH_LAP */

#define SRV_CHECKPOINT_SIZE	(16)
struct SrvCheckpoint {   
    SP_U32
	MsgType,
	MsgLength;
    SP_FLOAT
	time;
    SP_I32
	checkpoint,
	num_checkpoints,
	visited;
};

#define SRV_UPDATE_STATE_SIZE	(4)
struct SrvUpdateState {   
    SP_U32
	MsgType,
	MsgLength;
    SP_U32
	state;
};

#define SRV_STANDING_SIZE	(52)
struct SrvStanding {   
    SP_U32
	MsgType,
	MsgLength;
    SP_CHAR
	user[32];			/* Name and host of user */
    SP_U32
	position,			/* Position user finished in */
	flags;
    SP_FLOAT
	r, g, b;			/* Color of User's car */
};

#define SRV_EXPLOSION_SIZE	(16)
struct SrvExplosion {
    SP_U32
	MsgType,
	MsgLength;
    SP_FLOAT
	x, y, z,			/* Explosion Location */
	radius;				/* radius at maximum expansion */
};

#define SRV_VIRTUAL_TIME_SIZE	(4+sizeof(time_t))
struct SrvVirtualTime {
    SP_U32
	MsgType,
	MsgLength;
    SP_I32
	static_time;			/* Is time frozen? */
    time_t
	reference_time;			/* like returned by time() */
};

#define SRV_FRAME_POSITION_SIZE		(32*4)
struct SrvFramePosition {
    SP_U32
	MsgType,
	MsgLength;
    SP_FLOAT
	xform[4][4],
	alt_view_xform[4][4];
};

#define SRV_FRAME_GAUGE_SIZE		(28)
struct SrvFrameGauge {
    SP_U32
	MsgType,
	MsgLength;
    SP_FLOAT
	mph,
	rpm,
	x_val,
	y_val,
	altitude;
    SP_I32
	num_turbos,
	num_shells;
};

#define SRV_FRAME_MATRIX_SIZE		(18*4)
struct SrvFrameMatrix {
    SP_U32
	MsgType,
	MsgLength;
    SP_FLOAT
	xform[4][4];
    SP_I32
	connection,
	segment;
};

#define SRV_FRAME_UPDATE_SIZE		(sizeof(UpdateDisp))
struct SrvFrameUpdate {
    SP_U32
	MsgType,
	MsgLength;
    UpdateDisp
	upd;		/* UpdateDisplay structure */
};

#define SRV_FRAME_STATIC_SEG_SIZE	(4)
struct SrvFrameStaticSeg {
    SP_U32
	MsgType,
	MsgLength;
    SP_I32
	segment;
};

#define TYP_RADAR_SIZE		128
#define SRV_FRAME_RADAR_NUM(n)  (n/24)
#define SRV_FRAME_RADAR_SIZE(n)	(n*24)

struct SrvFrameRadar {
    SP_U32
	MsgType,
	MsgLength;
    struct {
	SP_FLOAT
	    x_pos, y_pos, z_pos,
	    r, g, b;
    }   RadarTypes[TYP_RADAR_SIZE];
};

#define SRV_FRAME_GLOBAL_NAMESET_SIZE	(4)
struct SrvFrameGlobalNameset {
    SP_U32
	MsgType,
	MsgLength;
    SP_U32
	global_nameset_bits;
};

struct IPCMsg *GetMsg( int );
int PutMsg( struct IPCMsg *, int);

#endif /* ] */
