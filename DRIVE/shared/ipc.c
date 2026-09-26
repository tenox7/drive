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



/* ipc.c */

/* Use configure data if available; if not, guess. */
#if defined(HAVE_CONFIG_H)
# include "config.h"
#elif defined(_HPUX_SOURCE)
# define WORDS_BIGENDIAN 1
#else
# undef WORDS_BIGENDIAN
#endif

/* If you don't want error messages, #define SILENT_MODE */
#define SILENT_MODE     1

#ifdef SILENT_MODE
#    define PERROR(a)
#    define PRINTF(a)
#    define FPRINTF(a)
#else
#    define PERROR(a) perror(a)
#    define PRINTF(a) printf a
#    define FPRINTF(a) fprintf a
#endif


/* For some reason, byte-swapping was turned off in the original linux
 * version of this.  Some day we should make this work for interoperability.
 */
#if defined(__linux__) || defined(MAC)
# define WORDS_BIGENDIAN 1
#endif

#include <time.h>

#ifndef WIN32
#include <sys/time.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>

#if defined(_HPUX_SOURCE) || defined(__linux__) || defined(MAC) /* [ */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

#include "global.h"
#include "newMessage.h"

#define	BLOCKING_OFF(Sock) (void)fcntl( Sock, F_SETFL, O_NONBLOCK )

#define	BLOCKING_ON(Sock) (void)fcntl( Sock, F_SETFL, 0 )

typedef fd_set *SELTYPE;

#define	FAR

#endif	/* ] _HPUX_SOURCE  */

#ifdef WIN32 /* [ */

#include <windows.h>
#include "global.h"
#include "newMessage.h"
#define errno WSAGetLastError()
#define close closesocket
//#ifndef EAGAIN
#define EAGAIN WSAEWOULDBLOCK
//#endif
#define ENOTCONN WSAENOTCONN

typedef fd_set *SELTYPE;

#define	BLOCKING_OFF(Sock) { \
    long val = 1; \
    (void)ioctlsocket( Sock, FIONBIO, &val ); \
}

#define	BLOCKING_ON(Sock) { \
    long val = 0; \
    (void)ioctlsocket( Sock, FIONBIO, &val ); \
}

#endif	/* ] */

/*
#include "globals.h"
*/

/* Forward prototype */
int FlushMsg(IPC_SOCK Sock);
/* From nquery.c */
extern void CheckQuerySocket(IPC_SOCK skt);

#if defined(WORDS_BIGENDIAN)	/* [ */
# define SWAB_LONG( Val )
# define SWAB_SHORT( Val )

#else /* ][ */

# define SWAB_LONG( Val ) { \
    unsigned char *__t__, __u__; \
    __t__ = (unsigned char *)Val; \
    __u__ = __t__[0]; __t__[0] = __t__[3]; __t__[3] = __u__; \
    __u__ = __t__[1]; __t__[1] = __t__[2]; __t__[2] = __u__; \
}

# define SWAB_SHORT( Val ) { \
    unsigned char *__t__, __u__; \
    __t__ = (unsigned char *)Val; \
    __u__ = __t__[0]; __t__[0] = __t__[1]; __t__[1] = __u__; \
}

#endif /* WORDS_BIGENDIAN else */

#define	BSIZ	8192	/* 8K buffer is the largest a socket can choke down */


static IPC_SOCK
    ReadSock = IPC_INVALID_SOCK,	WriteSock = IPC_INVALID_SOCK;
static int
    ReadSize = 0,	WriteSize = 0,
    ReadPos = 0;
static char
    ReadBuff[BSIZ],
    WriteBuff[BSIZ];

/* #define	DEBUGGING */

#ifdef DEBUGGING
static char *MsgName( int MsgType )
{
    switch( MsgType ) {
	case CLI_CLOSE 			:	return "CLI_CLOSE ";
	case CLI_REGISTER		:	return "CLI_REGISTER";
	case CLI_INPUT			:	return "CLI_INPUT ";
	case CLI_DISCONNECT		:	return "CLI_DISCONNECT ";
	case CLI_READY			:	return "CLI_READY";
	case CLI_RESTART		:	return "CLI_RESTART ";
	case CLI_LIGHTS			:	return "CLI_LIGHTS ";
	case CLI_UPRIGHT		:	return "CLI_UPRIGHT ";
	case CLI_NEW_VEHICLE		:	return "CLI_NEW_VEHICLE ";
	case SRV_CLOSE			:	return "SRV_CLOSE ";
	case SRV_DEFINE			:	return "SRV_DEFINE";
	case SRV_BEGINFRAME		:	return "SRV_BEGINFRAME";
	case SRV_IDENTIFY		:	return "SRV_IDENTIFY";
	case SRV_DRIVEABLE		:	return "SRV_DRIVEABLE";
	case SRV_START_LAP		:	return "SRV_START_LAP";
	case SRV_CHECKPOINT		:	return "SRV_CHECKPOINT";
	case SRV_FINISH_LAP		:	return "SRV_FINISH_LAP";
	case SRV_UPDATE_STATE		:	return "SRV_UPDATE_STATE";
	case SRV_STANDING		:	return "SRV_STANDING";
	case SRV_EXPLOSION		:	return "SRV_EXPLOSION";
	case SRV_DEFINE_MANY		:	return "SRV_DEFINE_MANY";
	case SRV_VIRTUAL_TIME		:	return "SRV_VIRTUAL_TIME";
	case SRV_GRAPHIC		:	return "SRV_GRAPHIC";
	case SRV_SEGLIST		:	return "SRV_SEGLIST";
	case SRV_FRAME_END		:	return "SRV_FRAME_END";
	case SRV_FRAME_POSITION		:	return "SRV_POSITION";
	case SRV_FRAME_GAUGE		:	return "SRV_FRAME_GAUGE";
	case SRV_FRAME_MATRIX		:	return "SRV_FRAME_MATRIX";
	case SRV_FRAME_UPDATE		:	return "SRV_FRAME_UPDATE";
	case SRV_FRAME_STATIC_SEG	:	return "SRV_FRAME_STATIC_SEG";
	case SRV_FRAME_RADAR		:	return "SRV_FRAME_RADAR";
	case SRV_FRAME_GLOBAL_NAMESET	:	return "SRV_FRAME_GLOBAL_NAMSET";
	default				:	return "UNKNOWN";
    }
}
#endif

IPC_SOCK CreateClientDGRAMSocket( int TCPSock, int UDPSock, 
				int RemotePort, int *LocalPort)
{
    struct sockaddr_in
	PeerAddr,
	myaddr_in;
    int 
	tsize = sizeof( struct sockaddr_in ),
	Sock;

    memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
    memset ((char *)&PeerAddr, 0, sizeof(struct sockaddr_in));

    if( UDPSock == IPC_INVALID_SOCK ) {
	/* We need to create a UDP socket, bind it to a local address/port,
	** then return that local port and socket number 
	*/

	Sock = socket( AF_INET, SOCK_DGRAM, 0 );
	if( Sock < 0 ) {
	    PERROR( "CreateClientDGRAMSocket: socket" );
	    return IPC_INVALID_SOCK;
	}


	myaddr_in.sin_family = AF_INET;
	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	myaddr_in.sin_port = 0;  /* Let the system pick one for us */

	if( bind( Sock, (const struct sockaddr *)&myaddr_in, sizeof(struct sockaddr_in) ) < 0 ) {
	    PERROR( "CreateClientDGRAMSocket: bind" );
	    return IPC_INVALID_SOCK;
	}

	/* See which port we were assigned to... */
	getsockname( Sock, (struct sockaddr *)&myaddr_in, &tsize );

	SWAB_SHORT(&myaddr_in.sin_port);

	*LocalPort = myaddr_in.sin_port;

	BLOCKING_OFF(Sock);

	return Sock;
    }
    else {

	/* We have previously created the UDP socket, now all that remains 
	** to be done is to connect it to the provided port.
	*/
	if( getpeername( TCPSock, (struct sockaddr *)&PeerAddr, &tsize ) < 0 )
	{
	    PERROR( "CreateClientDGRAMSocket: getpeername" );
	    return IPC_INVALID_SOCK;
	}
	PeerAddr.sin_port = RemotePort;

	SWAB_SHORT(&PeerAddr.sin_port);

	/* Connect to the remote socket */
	if( connect( UDPSock, (struct sockaddr *)&PeerAddr, sizeof(struct sockaddr_in) ) < 0 ) {
	    PERROR( "CreateClientDGRAMSocket: connect" );
	    return IPC_INVALID_SOCK;
	}

	BLOCKING_OFF(UDPSock);
	return(UDPSock);
    }
}

IPC_SOCK CreateServerDGRAMSocket( int TCPSock, int RemotePort, int *LocalPort)
{
    struct sockaddr_in 
	PeerAddr,
	myaddr_in;
    int 
	tsize = sizeof( struct sockaddr_in ),
	Port;
    int
	Sock;


    /* Create the UDP socket */
    Sock = socket( AF_INET, SOCK_DGRAM, 0 );
    if( Sock < 0 ) {
	PERROR( "CreateServerDGRAMSocket: socket" );
	return IPC_INVALID_SOCK;
    }

    memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
    memset ((char *)&PeerAddr, 0, sizeof(struct sockaddr_in));


    myaddr_in.sin_family = AF_INET;
    myaddr_in.sin_addr.s_addr = INADDR_ANY;
    myaddr_in.sin_port = 0;  /* Let the system pick one for us */

    if( getpeername( TCPSock, (struct sockaddr *)&PeerAddr, &tsize ) < 0 )
    {
	PERROR(" CreateServerDGRAMSocket: getpeername");
	return IPC_INVALID_SOCK;
    }

    PeerAddr.sin_port = RemotePort;

    SWAB_SHORT(&PeerAddr.sin_port);

#if 0
    /* We could bind the socket to a local address and port, but Lamont
    ** says this would cause trouble in socksify'ing this (as if we ever
    ** wanted to do that), since it can't handle bind, connect.  Rather,
    ** he says, just do the connect, and let the bind happen implicitly.
    */
    if( bind( Sock, &myaddr_in, sizeof(struct sockaddr_in) ) < 0 ) {
	PERROR( "CreateServerDGRAMSocket: bind" );
	return IPC_INVALID_SOCK
    }
#endif

    /* Connect to the remote socket */
    if( connect( Sock, (struct sockaddr *)&PeerAddr, sizeof(struct sockaddr_in) ) < 0 ) {
	PERROR( "CreateServerDGRAMSocket: connect" );
	return IPC_INVALID_SOCK;
    }

    /* Okay. Now that we are connected, lets find out what local port we 
    ** were bound to.
    */

    getsockname(Sock, (struct sockaddr *)&myaddr_in, &tsize );

    SWAB_SHORT( &myaddr_in.sin_port) ;

    Port = myaddr_in.sin_port;	/* See what port the system picked for us */

    *LocalPort = Port;

    BLOCKING_OFF(Sock);

    return Sock;
}

void SendDGRAMSocketToClient( int TCPSock, int UDPPort )
{
#if 0
    struct SrvInit ini;

    ini.MsgType = SRV_INIT;
    ini.MsgLength = SRV_INIT_SIZE(1);
    ini.SrvUDPPort = UDPPort;

    (void)PutMsg( (struct IPCMsg *)&ini, TCPSock);
    (void)FlushMsg( TCPSock);
#endif
}

int PrepareForIPC( void )
{
#if !(defined(_HPUX_SOURCE) || defined(__linux__) || defined(MAC))
    static int init = 0;
    WSADATA wsaData;
    if( !init ) {
	if( WSAStartup( 0x0002, &wsaData ) != 0 ) {
	    PRINTF(( "WSAStartup: %d\n", errno ));
	    exit( 1 );
	}
	init = 1;
    }
#endif
    return 1;
}

/* Returns socket address, or IPC_INVALID_SOCK for error */
IPC_SOCK InitIPC( char *Host, char *Service )
{
    struct sockaddr_in
	PeerAddr;
    struct hostent
	FAR *hp;
    struct servent
	FAR *sp;
    IPC_SOCK
	Sock;

    /* Get the peer address */
    (void)memset( &PeerAddr, 0, sizeof(struct sockaddr_in) );
    PeerAddr.sin_family = AF_INET;
    if( Host ) {
	/* A client is requesting connection to a server */
	hp = gethostbyname( Host );
	if( !hp ) {
            PERROR("gethostbyname");
            return IPC_INVALID_SOCK;
        }
	PeerAddr.sin_addr.s_addr = ((struct in_addr *)hp->h_addr)->s_addr;
    }
    else {
	PeerAddr.sin_addr.s_addr = INADDR_ANY;
    }

    if( isdigit( Service[0] ) ) {
	/* Allow running without /etc/services entry */
	PeerAddr.sin_port = atoi( Service );
    }
    else {
	sp = getservbyname( Service, "tcp" );
	if( !sp ) {
	    return IPC_INVALID_SOCK;
	}
	PeerAddr.sin_port = sp->s_port;
    }

    SWAB_SHORT( &PeerAddr.sin_port );

    /* Create the socket */
    Sock = socket( AF_INET, SOCK_STREAM, 0 );
    if( Sock == IPC_INVALID_SOCK ) {
	PERROR( "InitIPC: socket" );
	return IPC_INVALID_SOCK;
    }

    if( Host ) {
	/* Connect to the remote server */
	if( connect( Sock, (const struct sockaddr *)&PeerAddr, sizeof(struct sockaddr_in) ) < 0 ) {
	    PERROR( "InitIPC: connect" );
	    return IPC_INVALID_SOCK;
	}
    }
    else {
	int reuse = 1;

	/* Allow the socket to be re-used, in case server dies */
	setsockopt(Sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(int));

	/* Bind the listen address to the socket */
	if( bind( Sock, (const struct sockaddr *)&PeerAddr, sizeof(struct sockaddr_in) ) < 0 ) {
	    PERROR( "InitIPC: bind" );
	    return IPC_INVALID_SOCK;
	}
	/* Set up the listening */
	if( listen( Sock, 5 ) < 0 ) {
	    PERROR( "InitIPC: listen" );
	    return IPC_INVALID_SOCK;
	}
    }

    BLOCKING_OFF( Sock );

    return Sock;
}

/* Wait for connections/messages */
int IPCServer( IPC_SOCK Sock, IPC_SOCK Query, int Wait, IPC_SOCK *Socks, 
    int NumSocks )
{
    static struct IPCMsg
	Msg = { SRV_DIE, 0 };
    struct timeval
	Timeout;
    static fd_set
	Bits;
    int
	i, n, AddrLen;
    struct sockaddr_in
	PeerAddr;

    Timeout.tv_sec = Wait / 1000000;
    Timeout.tv_usec = Wait % 1000000;
    AddrLen = sizeof(struct sockaddr_in);

    FD_ZERO( &Bits );

    FD_SET( Sock, &Bits );
    if( Query != IPC_INVALID_SOCK )	FD_SET( Query,&Bits );

    for( i = 0; i < NumSocks; i++ ) {
	if( Socks[i] != IPC_INVALID_SOCK )	FD_SET( Socks[i], &Bits );
    }

    n = select( FD_SETSIZE, (SELTYPE)&Bits, 0, 0, &Timeout );
    if( n < 0 ) {
	PERROR( "IPCServer: select" );
	return -1;
    }
    /*if( !n )			return 0;*/

    if( (Query != IPC_INVALID_SOCK) && FD_ISSET( Query, &Bits ) ) {
	CheckQuerySocket( Query );
    }

    if( !FD_ISSET( Sock, &Bits ) )	return 0;

    /* Find an unused socket */
    for( i = 0; i < NumSocks; i++ ) {
	if( Socks[i] < 0 ) {
	    /* Unused - accept the connection */
	    Socks[i] = accept( Sock, (struct sockaddr *)&PeerAddr, &AddrLen );
	    if( Socks[i] < 0 ) {
		PERROR( "IPCServer: accept" );
		return IPC_INVALID_SOCK;
	    }

	    BLOCKING_OFF( Socks[i] );

	    /* Return the socket number we modified */
	    return i+1;
	}
    }

    /* If we get here, there are NO unused sockets... */
    n = accept( Sock, (struct sockaddr *)&PeerAddr, &AddrLen );
    if( n < 0 ) {
	PERROR( "IPCServer: accept" );
	return -1;
    }

    /* Put the DIE message to the client... */
    (void)PutMsg( &Msg, n );
    (void)FlushMsg( n );

    /* Now close it and return failure... */
    (void)shutdown( n, 2 );
    (void)close( n );
    return 0;
}

/* Shutdown a socket */
void TermIPC( IPC_SOCK Sock/*, int Final*/ )
{
    (void)shutdown( Sock, 2 );
    (void)close( Sock );

    if( Sock == WriteSock ) {
        WriteSize = 0;
        WriteSock = IPC_INVALID_SOCK;
    }
    if( Sock == ReadSock ) {
        ReadSize = 0;
        ReadSock = IPC_INVALID_SOCK;
    }
#if 0 /*!defined(_HPUX_SOURCE)*/
	if( Final ) {
		(void)WSACleanup();
	}
#endif
}

#ifdef _INCLUDE_BYTE_SWAPPING_MESSAGES
static void SwabLongArray( SP_I32 *Src, SP_I32 *Dst, int n )
{
    int
	i, t;

    for( i = 0; i < n; i++ ) {
	t = *Src++;
	SWAB_LONG( &t );
	*Dst++ = t;
    }
}

static void SwabShortArray( SP_I16 *Src, SP_I16 *Dst, int n )
{
    int
	i, t;

    for( i = 0; i < n; i++ ) {
	t = *Src++;
	SWAB_SHORT( &t );
	*Dst++ = t;
    }
}

static void SwabMsg( struct IPCMsg *Msg, struct IPCMsg *Res )
{
    SP_I32
	*Src, *Dst,
	i, n, Tmp;

    /* Initial copy - so we don't have to do anything for no swap */
    if( Res != Msg ) memcpy( Res, Msg, MSG_SIZE+Msg->MsgLength );

    /* Set up assuming all 32-bits */
    Src = (SP_I32 *)Msg->MsgData;
    Dst = (SP_I32 *)Res->MsgData;
    n = Msg->MsgLength / sizeof(SP_I32);

    switch( Msg->MsgType ) {
    /* Client->server messages */
    case CLI_INIT :
	/* Swap 6 words: Version, Flags, CliUDPPort, CliColor[3] */
	SwabLongArray( Src, Dst, 6 );
	/* The others are strings; leave them alone */
	break;

    case CLI_CONTROLS :
	/* Swap the entire message */
	SwabLongArray( Src, Dst, n );
	break;

    case CLI_QUIT :
	/* No data */
	break;

    case CLI_SEND_AUDIO :
	/* Swap 2 longs: sequence and ID */
	SwabLongArray( Src, Dst, 2 );
	break;

    /* Server->client messages */
    case SRV_DIE :
	/* No data */
	break;

    case SRV_ERROR :
	/* Character string - no swap */
	break;

    case SRV_GRAPHIC :
	SwabLongArray( Src, Dst, 2 );
	break;

    case SRV_RENDER :
	/* All longs */
	SwabLongArray( Src, Dst, n );
	break;

    case SRV_RENDER_FIXED :
	/* 12 shorts = 6 words */
	SwabShortArray( (SP_I16 *)Src, (SP_I16 *)Dst, 12 );
	/* 4 bytes = 1 word - skip */
	/* The rest longs */
	SwabLongArray( Src+7, Dst+7, n-7 );
	break;

    case SRV_TRANSLATE :
	/* All longs */
	SwabLongArray( Src, Dst, n );
	break;

    case SRV_MARKER :
	/* Bummer - we've got embedded bytes in an array.  Gotta
	 * do it right...
	 */
	n = SRV_MARKER_NUM( Msg->MsgLength );
	while( n-- ) {
	    /* Skip 4 bytes = 1 word */
	    Src += 1; Dst += 1;
	    /* Swab 3 floats */
	    SwabLongArray( Src, Dst, 3 );
	    Src += 3; Dst += 3;
	}
	break;

    case SRV_EYE :
	/* 2 longs */
	SwabLongArray( Src, Dst, 2 );
	Src += 2; Dst +=2;
	/* 6 shorts */
	SwabShortArray((SP_I16 *)Src, (SP_I16 *)Dst, 6);
	/* Ignore the rest */
	break;

    case SRV_UPDATE :
	/* No data */
	break;

    case SRV_TEXT :
	/* Character string - don't swap */
	break;

    case SRV_PLAY_AUDIO :
	/* Swap 2 longs: sequence and ID */
	SwabLongArray( Src, Dst, 2 );
	break;

    case SRV_RENDER_TEXT :
	/* Swap 12 shorts, ignore the rest */
	SwabShortArray( (SP_I16 *)Src, (SP_I16 *)Dst, 12 );
	break;

    case SRV_SCORE_INFO :
	/* All bytes - no swap */
	break;

    case SRV_EXPLOSIONS :
	SwabShortArray( (SP_I16 *)Src, (SP_I16 *)Dst, Msg->MsgLength / 2 );
	break;

    case SRV_TARGET_INFO :
	/* All bytes - no swap */
	break;

    case SRV_NAMES :
	/* All bytes - no swap */
	break;

    case SRV_SLOWTEXT :
	/* Character string - don't swap */
	break;

    case SRV_SOUNDEVENT:
	SwabLongArray(Src, Dst, 1);
	Src++; Dst++;
	SwabShortArray( (SP_I16 *)Src, (SP_I16 *)Dst, 3);
	break;
    case SRV_ROTATED:
	n = SRV_ROTATED_NUM( Msg->MsgLength);
	for( i = 0; i < n; i++ ) {
	    SwabShortArray( (SP_I16 *)Src, (SP_I16 *)Dst, 8);
	    Src += 4; Dst += 4;
	    SwabLongArray( Src, Dst, 1); 
	    Src++; Dst++;
	}
	break;
    case SRV_INIT :
	/* 1 long */
	SwabLongArray(Src, Dst, 1);
	break;
    }
}
#endif /* ] */



/* Read a message from the given socket */
struct IPCMsg *GetMsg( IPC_SOCK Sock )
{
    struct IPCMsg
	Hdr;
    long
	n;
    char
	*Temp;
    static struct IPCMsg
	*Res = 0;
    static long
	ResSize = 0;
    static struct IPCMsg
	QuitMsg = {IPC_TERM, 0};

    if( Sock < 0 )	return 0;

    if( (Sock != ReadSock) && (ReadPos < ReadSize) ) {
	FPRINTF(( stderr, "Oops; didn't read %d bytes from %d\n",
		ReadSize - ReadPos, ReadSock ));
	ReadPos = ReadSize = 0;
    }

    ReadSock = Sock;
    if( ReadPos >= ReadSize ) {
	/* Read a new buffer */
	ReadPos = 0;
	ReadSize = recv( Sock, ReadBuff, BSIZ, 0 );
	if( ReadSize < 0 ) {
	    if( errno != EAGAIN ) {
		PERROR( "GetMsg: recv" );
		return &QuitMsg;
	    }
	    ReadSize = 0;
	}
	else if( ReadSize == 0 ) {
	    /* Oops - remote end closed! */
	    return &QuitMsg;
	}
    }

    if( !ReadSize )	return 0;

    if( (ReadSize - ReadPos) < MSG_SIZE ) {
	n = ReadSize - ReadPos;
	(void)memcpy( (void *)&Hdr, (void *)(ReadBuff+ReadPos), n );

	/* Read a new buffer */
	do {
	    ReadSize = recv( Sock, ReadBuff, BSIZ, 0 );
	    if( ReadSize < 0 ) {
		if( errno != EAGAIN ) {
                    PRINTF(( "GetMsg: %d\n", errno ));
		    PERROR( "GetMsg: recv" );
		    return &QuitMsg;
		}
		ReadSize = 0;
	    }
	    else if( ReadSize == 0 ) {
		/* Oops - remote end closed! */
		return &QuitMsg;
	    }
	} while( !ReadSize );
	(void)memcpy( (void *)(n + (char *)&Hdr), (void *)ReadBuff,
				MSG_SIZE - n );
	ReadPos = MSG_SIZE - n;
    }
    else {
	(void)memcpy( (void *)&Hdr, (void *)(ReadBuff+ReadPos), MSG_SIZE );
	ReadPos += MSG_SIZE;
    }

    SWAB_LONG( &Hdr.MsgType );
    SWAB_LONG( &Hdr.MsgLength );

#ifdef DEBUGGING
    (void)printf( "GetMsg: %s(%d)\n", MsgName(Hdr.MsgType), Hdr.MsgLength );
    (void)fflush( stdout );
#endif

    /* Allocate a larger buffer, if needed */
    if( !Res ) {
	Res = malloc( MSG_SIZE + Hdr.MsgLength );
	ResSize = Hdr.MsgLength;
    }
    else if( Hdr.MsgLength > ResSize ) {
	Res = realloc( Res, MSG_SIZE + Hdr.MsgLength );
	ResSize = Hdr.MsgLength;
    }
    if( !Res ) {
	ResSize = 0;
	return 0;
    }
    (void)memcpy( Res, &Hdr, MSG_SIZE );

    if( (ReadSize - ReadPos) < Hdr.MsgLength ) {
	(void)memcpy( (void *)(MSG_SIZE + (char *)Res),
			(void *)(ReadBuff + ReadPos),
			ReadSize - ReadPos );

	/* Number of bytes left to read */
	n = Hdr.MsgLength - (ReadSize - ReadPos);
	Temp = MSG_SIZE + (ReadSize - ReadPos) + (char *)Res;

	while( n > 0 ) {
	    /* Read a new buffer */
	    ReadSize = recv( Sock, ReadBuff, BSIZ, 0 );
	    if( ReadSize < 0 ) {
		if( errno != EAGAIN ) {
                    PRINTF(( "GetMsg: %d\n", errno ));
		    PERROR( "GetMsg: recv" );
		    return &QuitMsg;
		}
	    }
	    else if( ReadSize == 0 ) {
		/* Oops - remote end closed! */
		return &QuitMsg;
	    }
	    else if( n > ReadSize ) {
		(void)memcpy( (void *)Temp, (void *)ReadBuff, ReadSize );
		Temp += ReadSize;
		n -= ReadSize;
	    }
	    else {
		(void)memcpy( (void *)Temp, (void *)ReadBuff, n );
		Temp += n;
		ReadPos = n;
		n = 0;
	    }
	}
    }
    else {
	(void)memcpy( (void *)(MSG_SIZE + (char *)Res),
			(void *)(ReadBuff + ReadPos),
			Hdr.MsgLength );
	ReadPos += Hdr.MsgLength;
    }

#if !defined(WORDS_BIGENDIAN) && defined(_INCLUDE_BYTE_SWAPPING_MESSAGES)
    SwabMsg( Res, Res );
#endif
    return Res;
}

/* Put a message to the given socket */
int PutMsg( struct IPCMsg *Msg, IPC_SOCK Sock )
{
    int
	n, r;
    char
	*Temp,
	*Buff;
#if !defined(WORDS_BIGENDIAN)	/* [ */
    static struct IPCMsg
	*Swab = 0;
    static int
	SwabSize = 0;
#endif

    if( Sock < 0 )	return 0;

#if defined(_HPUX_SOURCE) || defined(__linux__) || defined(MAC)
    (void)signal( SIGPIPE, SIG_IGN );
#endif

#ifdef DEBUGGING
    (void)printf( "PutMsg: %s(%d)\n", MsgName(Msg->MsgType), Msg->MsgLength );
    (void)fflush( stdout );
#endif

    if( (Sock != WriteSock) && (WriteSize > 0) ) {
	/* Flush the previous buffer */
	BLOCKING_ON( WriteSock );
	Buff = WriteBuff;
	while( WriteSize > 0 ) {
	    r = send( WriteSock, Buff, WriteSize, 0 );
	    if( r < 0 ) {
		PERROR( "PutMsg: send" );
#if defined(_HPUX_SOURCE) || defined(__linux__) || defined(MAC)
		(void)signal( SIGPIPE, SIG_DFL );
#endif
		return 0;
	    }
	    WriteSize -= r;
	    Buff += r;
	}
	BLOCKING_OFF( WriteSock );
	WriteSize = 0;
    }
    WriteSock = Sock;

#if !defined(WORDS_BIGENDIAN)	/* [ */
    if( !Swab ) {
	Swab = malloc( MSG_SIZE + Msg->MsgLength );
	SwabSize = Msg->MsgLength;
    }
    else if( Msg->MsgLength > SwabSize ) {
	Swab = realloc( Swab, MSG_SIZE + Msg->MsgLength );
	SwabSize = Msg->MsgLength;
    }
    if( !Swab ) {
	SwabSize = 0;
	return 0;
    }
# if defined(_INCLUDE_BYTE_SWAPPING_MESSAGES)
    SwabMsg( Msg, Swab );
# else
    (void)memcpy(Swab, Msg, Msg->MsgLength + MSG_SIZE);
# endif
    Msg = Swab;
#endif /* ] !BIGENDIAN */

    n = Msg->MsgLength + MSG_SIZE;
    Temp = (char *)Msg;

    SWAB_LONG( &Msg->MsgType );
    SWAB_LONG( &Msg->MsgLength );

    if( (WriteSize + n) > BSIZ ) {
	/* Copy what we can */
	(void)memcpy( WriteBuff+WriteSize, Temp, BSIZ-WriteSize );
	Temp += BSIZ - WriteSize;
	n -= BSIZ - WriteSize;

	/* Flush the buffer */
	BLOCKING_ON( Sock );
	WriteSize = BSIZ;
	Buff = WriteBuff;
	while( WriteSize > 0 ) {
	    r = send( Sock, Buff, WriteSize, 0 );
	    if( r < 0 ) {
		PERROR( "PutMsg: send" );
#if defined(_HPUX_SOURCE) || defined(__linux__) || defined(MAC)
		(void)signal( SIGPIPE, SIG_DFL );
#endif
		return 0;
	    }
	    WriteSize -= r;
	    Buff += r;
	}

	/* Send the rest of the message */
	while( n > 0 ) {
	    r = send( Sock, Temp, n, 0 );
	    if( r < 0 ) {
		PERROR( "PutMsg: send" );
#if defined(_HPUX_SOURCE) || defined(__linux__) || defined(MAC)
		(void)signal( SIGPIPE, SIG_DFL );
#endif
		return 0;
	    }
	    n -= r;
	    Temp += r;
	}
	BLOCKING_OFF( Sock );
	WriteSize = 0;
    }
    else {
	(void)memcpy( WriteBuff+WriteSize, Temp, n );
	WriteSize += n;
    }

#if defined(_HPUX_SOURCE) || defined(__linux__) || defined(MAC)
    (void)signal( SIGPIPE, SIG_DFL );
#endif

#ifdef HACK_HACK
    FlushMsg( Sock );
#endif

    return 1;
}


int FlushMsg( IPC_SOCK Sock )
{
    int
	n;
    char
	*Buff;

    if( !WriteSize )	return 1;

    if( (Sock != WriteSock) && (WriteSock != IPC_INVALID_SOCK) ) {
	FPRINTF(( stderr, "Umm; FlushMsg confused (%d != %d)\n",
			Sock, WriteSock ));
    }
    WriteSock = Sock;

    BLOCKING_ON( Sock );
    Buff = WriteBuff;
    while( WriteSize > 0 ) {
	n = send( Sock, Buff, WriteSize, 0 );
	if( n < 0 ) {
	    PERROR( "FlushMsg: send" );
	    return 0;
	}
	Buff += n;
	WriteSize -= n;
    }

    BLOCKING_OFF( Sock );
    WriteSize = 0;
    return 1;
}

int IPCWait( IPC_SOCK Sock, long Wait )
{
    struct timeval
	Timeout;
    static fd_set
	Bits;
    int
	n;

    Timeout.tv_sec = Wait / 1000000;
    Timeout.tv_usec = Wait % 1000000;

    FD_ZERO( &Bits );
    FD_SET( Sock, &Bits );

    n = select( FD_SETSIZE, (SELTYPE)&Bits, 0, 0, &Timeout );
    if( n < 0 ) {
	PERROR( "IPCWait" );
	return -1;
    }
    return (n != 0);
}


/******************************************************************************
*   CheckSocketConnection()
*
*   This routine takes the specified socket and sees whether or not a 
*   connection for this socket has been made.  Note that this really only
*   makes sense to use with UDP sockets.
******************************************************************************/

int CheckSocketConnection(int Sock)
{
    int size;
    struct sockaddr_in
	PeerAddr;
    
    size = sizeof(struct sockaddr_in);
    if( getpeername(Sock, (struct sockaddr *)&PeerAddr, &size) < 0) {
	if( errno == ENOTCONN ) 
	    return(0);
	else
	    return(1);
    }
    else {
	return(1);
    }
}


/*** EOF ipc.c ***/
