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

#if defined(_HPUX_SOURCE) || defined(__linux__) || defined(MAC)/* [ */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

#define	BLOCKING_OFF(Sock)	(void)fcntl( Sock, F_SETFL, O_NONBLOCK )
#define	BLOCKING_ON(Sock)	(void)fcntl( Sock, F_SETFL, 0 )
typedef int *SELTYPE;

#define	FAR

#else	/* ] [ */

#include <windows.h>
#define errno WSAGetLastError()
#define close closesocket
#define EAGAIN WSAEWOULDBLOCK

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

#include "global.h"
#include "newMessage.h"

#define SEARCH_QUERY_TIMEOUT	(2000000/256)	/* Finish in 2 seconds */
#define SELECT_TIMEOUT		0

static char
    HostName[256];
static int
    HostNameLen;

/*** Called by the searcher ***/
int FindActiveServers
(
	char *PortName,
	char *ServerNames[256]
)
{
    IPC_SOCK
	s;
    int
	i, NumServers;
    struct hostent
	*hp;		/* pointer to host info for nameserver host */
    struct servent
	*sp;		/* pointer to service information */
    struct sockaddr_in
	myaddr_in;	/* for local socket address */
    struct sockaddr_in
	servaddr_in;	/* for server socket address */
    char
	Response[256];
#if defined(__linux__) || defined(MAC)
    fd_set 
	readfds;
#else
    int
	readfds;
#endif
    static struct timeval
	timeout = { 0,SEARCH_QUERY_TIMEOUT };

    NumServers = 0;

    /* clear out address structures */
    (void)memset( (char *)&myaddr_in,   0, sizeof(struct sockaddr_in) );
    (void)memset( (char *)&servaddr_in, 0, sizeof(struct sockaddr_in) );

    /* Set up the server address. */
    servaddr_in.sin_family = AF_INET;

    /* Where am I? */
    (void)gethostname( HostName, sizeof(HostName) );
    hp = gethostbyname( HostName );
    if( !hp )	return -1;

    servaddr_in.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;

    if( isdigit( *PortName ) ) {
	servaddr_in.sin_port = atoi( PortName );
    }
    else {
	sp = getservbyname( PortName, "udp" );
	if( !sp ) {
	    return -1;
	}
	servaddr_in.sin_port = sp->s_port;
    }


    /* Create the socket. */
    s = socket( AF_INET, SOCK_DGRAM, 0 );
    if( s == -1 ) {
	perror( "FindActiveServers:socket" );
	return -1;
    }

    /* Bind socket to some local address so that the
     * server can send the reply back.  A port number
     * of zero will be used so that the system will
     * assign any available port number.  An address
     * of INADDR_ANY will be used so we do not have to
     * look up the internet address of the local host.
     */
    myaddr_in.sin_family = AF_INET;
    myaddr_in.sin_port = 0;
    myaddr_in.sin_addr.s_addr = INADDR_ANY;
    if( bind( s, &myaddr_in, sizeof(struct sockaddr_in ) ) == -1 ) {
	perror( "FindActiveServers:bind" );
	return -1;
    }

    /* Send to each node on the local subnet. */
    for( i = 0; i < 256; i++ ) {
	servaddr_in.sin_addr.s_addr &= 0xffffff00;
	servaddr_in.sin_addr.s_addr |= i;

	if( sendto( s, HostName, strlen(HostName)+1, 0,
			&servaddr_in, sizeof(struct sockaddr_in) ) == -1 )
	{
	    continue;
	}

	/* See if they answered */
#if defined(__linux__) || defined(MAC)
	FD_ZERO(&readfds);
	FD_SET(s,&readfds);
#else
	readfds = 1 << s;
#endif
	if( select( s+1, &readfds, 0, 0, &timeout ) != SELECT_TIMEOUT ) {
	    if( recv( s, Response, sizeof(Response), 0 ) != -1 ) {
		ServerNames[NumServers] = malloc( strlen(Response) + 1 );
		if( ServerNames[NumServers] ) {
		    (void)strcpy( ServerNames[NumServers], Response );
		    NumServers++;
		}
	    }
	}
    }
    return NumServers;
}


/*** Called by the server.  Returns INVALID if failed. */
int SetupQuerySocket( char *PortName )
{
    struct servent
	*sp;		/* pointer to service information */
    struct sockaddr_in
	myaddr_in;	/* for local socket address */
    int
	skt;

    (void)gethostname( HostName, sizeof(HostName)-1 );
    HostNameLen = strlen( HostName ) + 1;

    /* clear out address structures */
    (void)memset( (char *)&myaddr_in, 0, sizeof(struct sockaddr_in) );

    /* Set up address structure for the socket. */
    myaddr_in.sin_family = AF_INET;
    myaddr_in.sin_addr.s_addr = INADDR_ANY;

    if( isdigit( *PortName ) ) {
	myaddr_in.sin_port = atoi( PortName );
    }
    else {
	sp = getservbyname( PortName, "udp" );
	if( !sp ) {
	    return -1;
	}
	myaddr_in.sin_port = sp->s_port;
    }

    /* Create the socket. */
    skt = socket( AF_INET, SOCK_DGRAM, 0 );
    if( skt == -1 ) {
	perror( "SetupQuerySocket:socket" );
	return -1;
    }

    /* Bind the server's address to the socket. */
    if( bind(skt, &myaddr_in, sizeof(struct sockaddr_in) ) == -1 ) {
	perror( "SetupQuerySocket:bind" );
	return -1;
    }

    return skt;
}


/*** Called by the server occasionally to see if anyone is looking for
 *** a server.
 ***/
void CheckQuerySocket( IPC_SOCK skt )
{
    static struct timeval
	timeout = {0,1};
    int
	addrlen;
    static fd_set
	Bits;
    char
	buffer[256];
    struct sockaddr_in
	clientaddr_in;	/* for client's socket address */

    if( skt == IPC_INVALID_SOCK )	return;

    FD_ZERO( &Bits );
    FD_SET(skt, &Bits);


    if( select( FD_SETSIZE, &Bits, 0, 0, &timeout ) != SELECT_TIMEOUT ) {
	(void)memset( (char *)&clientaddr_in, 0, sizeof(struct sockaddr_in) );
	addrlen = sizeof( struct sockaddr_in );
	if( recvfrom( skt, buffer, sizeof(buffer), 0,
			&clientaddr_in, &addrlen ) != -1 )
	{
	    (void)sendto( skt, HostName, HostNameLen, 0,
				&clientaddr_in, addrlen );
	}
    }
}

/*** EOF query.c ***/
