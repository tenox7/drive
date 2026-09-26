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
 * dlist.c - Display list routines.
 */


#include "global.h"
#include "connection.h"
#include "message.h"
#include "drive_server.h"
#include "drive.h"

connection_type *connection_list = 0;

#define	MAX_MESSAGE_SIZE	(1 << 20)	/* A megabyte */

/***************** Utilities for memory-based graphics stuff *****************/
static struct SrvGraphic
    *gfxBuff = 0;

static void *openGfxBuff( int segnum )
{
    (void)hwGetError();
    if( !gfxBuff ) {
	gfxBuff = malloc( MSG_SIZE + SRV_GRAPHIC_SIZE(MAX_MESSAGE_SIZE) );
	if( !gfxBuff ) {
	    exit( 1 );
	}
	gfxBuff->MsgType = SRV_GRAPHIC;
    }
    gfxBuff->MsgLength = SRV_GRAPHIC_SIZE(0);
    gfxBuff->segnum = segnum;
    return gfxBuff;
}

static int gfxBuffWrite( void *buf, int size, int count, void *outF )
{
    int
	total;

    total = size * count;
    if( (gfxBuff->MsgLength + total) > MAX_MESSAGE_SIZE )	return 0;

    (void)memcpy( gfxBuff->data + gfxBuff->MsgLength - SRV_GRAPHIC_SIZE(0),
			buf, total );
    gfxBuff->MsgLength += total;

    return count;
}

static void *closeGfxBuff( void *buf )
{
    struct SrvGraphic
	*result;
    hwInt32 n;

    result = malloc( gfxBuff->MsgLength + MSG_SIZE );
    if( !result )	return 0;

    (void)memcpy( result, buf, gfxBuff->MsgLength + MSG_SIZE );
    n = hwGetError();
    if( n ) {
	printf( "HW error: %d\n", (int) n );
    }
    return result;
}

static int
    gfxMsgLength;
static char
    *gfxMsgBuff;

static int gfxRead( void *buf, int size, int count, void *fileP )
{
    int
	actual;

    actual = size * count;
    if( actual > gfxMsgLength ) return 0;
    (void)memcpy( buf, gfxMsgBuff, actual );
    gfxMsgLength -= actual;
    gfxMsgBuff += actual;
    return count;
}

/***************** Utilities for HW segment hash table stuff *****************/

#define	HW_HASH_SIZE	512
#define	HASH_SEG(segnum)	((segnum) % HW_HASH_SIZE)

static struct HwObjCache
    *hwObjCache[HW_HASH_SIZE];
static int
    hwSegCounter;

int createHwSegmentFromObj( hwObject *objList, int objListSize )
{
    struct HwObjCache
	*newObj;
    int
	i, h;

    newObj = malloc( sizeof(struct HwObjCache) );
    if( !newObj ) exit( 1 );

    hwSegCounter++;
    newObj->segnum = hwSegCounter;
    newObj->objList = objList;
    newObj->objListSize = objListSize;
    newObj->segList = 0;
    newObj->matList = 0;

    /* Haven't sent it to anybody */
    for( i = 0; i < (MAX_FDS/32); i++ ) {
	newObj->sentList[i] = 0;
    }

    newObj->msg = openGfxBuff( newObj->segnum );
    (void)hwBeginBinary( gfxBuffWrite, newObj->msg,
		HW_FILE_WRITE_HDR, (hwInt32 *)&objListSize );
    for( i = 0; i < objListSize; i++ ) {
	(void)hwWriteBinary( objList[i], gfxBuffWrite, newObj->msg );
    }
    (void)hwEndBinary( gfxBuffWrite, newObj->msg );
    newObj->msg = closeGfxBuff( newObj->msg );

    h = HASH_SEG(hwSegCounter);
    newObj->next = hwObjCache[h];
    hwObjCache[h] = newObj;

    if( debug ) printf( "Created object segment %d\n", newObj->segnum );
    return newObj->segnum;
}

int createHwSegmentList( int nSegs, int *segs, float (*mats)[4][4] )
{
    int
	i, h;
    struct SrvSeglist
	*segListMsg;
    struct HwObjCache
	*newObj;

    newObj = malloc( sizeof(struct HwObjCache) );
    if( !newObj ) exit( 1 );
    newObj->segList = malloc( nSegs * sizeof(int) );
    newObj->matList = malloc( nSegs * 4*4*sizeof(float) );
    if( (!newObj->segList) || (!newObj->matList) ) exit( 1 );

    /* Haven't sent it to anybody */
    for( i = 0; i < (MAX_FDS/32); i++ ) {
	newObj->sentList[i] = 0;
    }

    hwSegCounter++;
    newObj->segnum = hwSegCounter;
    newObj->objList = 0;
    newObj->objListSize = nSegs;
    (void)memcpy( newObj->segList, segs, nSegs * sizeof(int) );
    (void)memcpy( newObj->matList, mats, nSegs * 4*4*sizeof(float) );

    newObj->msg = malloc( MSG_SIZE + SRV_SEGLIST_SIZE(nSegs) );
    if( !newObj->msg ) exit( 1 );

    segListMsg = (struct SrvSeglist *)newObj->msg;
    segListMsg->MsgType = SRV_SEGLIST;
    segListMsg->MsgLength = SRV_SEGLIST_SIZE(nSegs);
    segListMsg->segnum = newObj->segnum;
    for( i = 0; i < nSegs; i++ ) {
	(void)memcpy( &segListMsg->SegData[i].mat[0][0],
			&mats[i][0][0],
			4*4*sizeof(float) );
	segListMsg->SegData[i].segnum = segs[i];
    }

    h = HASH_SEG(hwSegCounter);
    newObj->next = hwObjCache[h];
    hwObjCache[h] = newObj;

    if( debug ) printf( "Created seglist segment %d\n", newObj->segnum );
    return newObj->segnum;
}

void createHwSegmentFromMsg( struct IPCMsg *msg )
{
    struct SrvSeglist
	*segListMsg = (struct SrvSeglist *)msg;
    struct SrvGraphic
	*graphMsg = (struct SrvGraphic *)msg;
    struct HwObjCache
	*newObj;
    int
	i, n, h;
    hwInt32
	numObjects;
    hwObject
	*objects;

    newObj = malloc( sizeof(struct HwObjCache) );
    if( !newObj ) exit( 1 );

    newObj->objList = 0;
    newObj->objListSize = 0;
    newObj->segList = 0;
    newObj->matList = 0;
    newObj->msg = 0;

    switch( msg->MsgType ) {
    case SRV_GRAPHIC :
	newObj->segnum = graphMsg->segnum;

	gfxMsgLength = msg->MsgLength - 4;
	gfxMsgBuff = (char *)graphMsg->data;

	numObjects = hwReadBinary( gfxRead, msg, 0, &objects );
#if 0 /* Not yet... */
	for( i = 0; i < numObjects; i++ ) {
	    if( cstate.LoResTextures ) {
		HW_MODIFY_1I( objects[i], hwStrMaxDimension, 128 );
	    }
	}
#endif
	newObj->objList = objects;
	newObj->objListSize = numObjects;
	for( i = 0; i < numObjects; i++ ) {
	    if( objects[i] ) {
		HW_MODIFY_1I( objects[i], hwStrOptFlags, HW_OPT_USE_DL );
	    }
	}
	break;

    case SRV_SEGLIST :
	newObj->segnum = segListMsg->segnum;
	n = SRV_SEGLIST_NUM(msg->MsgLength);
	newObj->objListSize = n;
	newObj->segList = malloc( n * sizeof(int) );
	newObj->matList = malloc( n * 4 * 4 * sizeof(float) );
	if( !newObj->segList || !newObj->matList ) {
	    exit( 1 );
	}
	for( i = 0; i < n; i++ ) {
	    (void)memcpy( &newObj->matList[i][0][0],
			&segListMsg->SegData[i].mat[0][0],
			4*4*sizeof(float) );
	    newObj->segList[i] = segListMsg->SegData[i].segnum;
	}
	break;
    }

    h = HASH_SEG(newObj->segnum);
    newObj->next = hwObjCache[h];
    hwObjCache[h] = newObj;
}

struct HwObjCache *findHwSegment( int segnum )
{
    struct HwObjCache
	*curr;
    int
	h;

    h = HASH_SEG(segnum);
    for( curr = hwObjCache[h]; curr; curr = curr->next ) {
	if( curr->segnum == segnum ) {
	    return curr;
	}
    }
    return 0;
}

/*****************************************************************
 * clearSegmentSent
 * 
 */
void clearSegmentSent( int sock )
{
    int
	i, wrd, bit;
    struct HwObjCache
	*curr;

    wrd = sock / 32;
    bit = 1 << (sock % 32);
    for( i = 0; i < HW_HASH_SIZE; i++ ) {
	for( curr = hwObjCache[i]; curr; curr = curr->next ) {
	    if( debug ) {
		printf( "Clearing sock %d from seg %d\n", sock, curr->segnum );
	    }
	    curr->sentList[wrd] &= ~bit;
	}
    }
}

/*****************************************************************
 * transmit_segment
 * 
 */
void transmit_segment(
    int fildes,
    connection_type *con,
    int segno)
{
    struct HwObjCache
	*curr;
    int
	i, sock, bit, wrd;
    int
        idx;

    if( debug ) printf( "Segment %d requested to be sent to sock %d\n", segno,
			con->socket->socketnum );

    curr = findHwSegment( segno );
    if( !curr )	return;

    sock = con->socket->socketnum;
    idx = con->socket->index;
    wrd = idx / 32;
    bit = 1 << (idx % 32);

    /* If already sent, don't resend it */
    if( curr->sentList[wrd] & bit ) {
	if( debug ) {
	    printf( "transmit_segment: sock %d already seen seg %d (%08x)\n",
			sock, segno, curr->sentList[wrd] );
	}
	return;
    }

    curr->sentList[wrd] |= bit;

    if( curr->segList ) {
	/* Oops - gotta send children */
	for( i = 0; i < curr->objListSize; i++ ) {
	    transmit_segment( fildes, con, curr->segList[i] );
	}
    }

    if( debug ) printf( "transmit_segment: sending %d\n", segno );

    (void)PutMsg( curr->msg, sock );
}

/*****************************************************************
 * Obsolete functions
 * 
 */
void transmit_buffer(char *write_buf , int send_size)
{
}

void transmit_segments(
    int fildes,
    int start_segno,
    int end_segno)
{
}

void receive_segment(
    int fildes,
    struct SrvDefine *Msg,
    boolean_type disable_transparency)
{
}

char *receive_segment_from_buffer(
    int fildes,
    char *buffer,
    boolean_type disable_transparency)
{
    return(NULL);
}

int recv_bytes(
    int skt,
    int len,
    int flags,
    char *buf)
{
    return(0);
}
