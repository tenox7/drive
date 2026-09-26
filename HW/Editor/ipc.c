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

#include "ipc.h"

#include <time.h>
#include "hwedit.h"

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

FILE *logFile;

#if !defined(WIN32) /* [ */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

#define BLOCKING_OFF(Sock) (void)fcntl( Sock, F_SETFL, O_NONBLOCK )

#define BLOCKING_ON(Sock) (void)fcntl( Sock, F_SETFL, 0 )

typedef fd_set *SELTYPE;

#define FAR

/* If you are on a little-endian system, uncomment the following line... */
/* #define      BYTE_SWAP */

#endif  /* ] !WIN32 */

#define BYTE_SWAP

#ifdef WIN32 /* [ */

#  include <windows.h>
#  define errno WSAGetLastError()
#  define close closesocket
#  define DO_OVER WSAEWOULDBLOCK
#  define ENOTCONN WSAENOTCONN

  typedef fd_set *SELTYPE;

#  define       BLOCKING_OFF(Sock) { \
      long val = 1; \
      (void)ioctlsocket( Sock, FIONBIO, &val ); \
  }

#  define       BLOCKING_ON(Sock) { \
      long val = 0; \
      (void)ioctlsocket( Sock, FIONBIO, &val ); \
  }

/* If you are on a little-endian system, uncomment the following line... */
#  define       BYTE_SWAP

#else /* ] [ */
#define DO_OVER EAGAIN
#endif  /* ] */

#ifdef BYTE_SWAP        /* [ */

#define SWAB_LONG( Val ) { \
    unsigned char *__t__, __u__; \
    __t__ = (unsigned char *)Val; \
    __u__ = __t__[0]; __t__[0] = __t__[3]; __t__[3] = __u__; \
    __u__ = __t__[1]; __t__[1] = __t__[2]; __t__[2] = __u__; \
}

#define SWAB_SHORT( Val ) { \
    unsigned char *__t__, __u__; \
    __t__ = (unsigned char *)Val; \
    __u__ = __t__[0]; __t__[0] = __t__[1]; __t__[1] = __u__; \
}
#else /* ] [ */

#define SWAB_LONG( Val )
#define SWAB_SHORT( Val )

#endif /* ] */


/* #define      DEBUGGING */

int PrepareForIPC( void )
{
#if defined(WIN32)
    static int init = 0;
    WSADATA wsaData;
    if( !init ) {
        if( WSAStartup( 0x0002, &wsaData ) != 0 ) {
            return 0;
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
        if( !hp )       return IPC_INVALID_SOCK;
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
        perror( "InitIPC: socket" );
        return IPC_INVALID_SOCK;
    }

    if( Host ) {
        /* Connect to the remote server */
        if( connect( Sock, (struct sockaddr *)&PeerAddr,
                     sizeof(struct sockaddr_in) ) < 0 )
        {
            perror( "InitIPC: connect" );
            return IPC_INVALID_SOCK;
        }
    }
    else {
        int reuse = 1;

        /* Allow the socket to be re-used, in case server dies */
        setsockopt(Sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));

        /* Bind the listen address to the socket */
        if( bind( Sock, (struct sockaddr *)&PeerAddr,
                  sizeof(struct sockaddr_in) ) < 0 )
        {
            perror( "InitIPC: bind" );
            return IPC_INVALID_SOCK;
        }
        /* Set up the listening */
        if( listen( Sock, 5 ) < 0 ) {
            perror( "InitIPC: listen" );
            return IPC_INVALID_SOCK;
        }
    }

    BLOCKING_OFF( Sock );

    return Sock;
}

/* Wait for connections/messages */
int IPCServer( IPC_SOCK Sock, int Wait, IPC_SOCK *Socks, int NumSocks )
{
    struct timeval
        Timeout;
    static fd_set
        Bits;
    int
        i, n;
    socklen_t
        AddrLen;
    struct sockaddr_in
        PeerAddr;

    Timeout.tv_sec = Wait / 1000000;
    Timeout.tv_usec = Wait % 1000000;
    AddrLen = sizeof(struct sockaddr_in);

    FD_ZERO( &Bits );

    FD_SET( Sock, &Bits );

    for( i = 0; i < NumSocks; i++ ) {
        if( Socks[i] != IPC_INVALID_SOCK )      FD_SET( Socks[i], &Bits );
    }

    n = select( FD_SETSIZE, (SELTYPE)&Bits, 0, 0, &Timeout );
    if( n < 0 ) {
        perror( "IPCServer: select" );
        return -1;
    }
    /*if( !n )                  return 0;*/

    if( !FD_ISSET( Sock, &Bits ) )      return 0;

    /* Find an unused socket */
    for( i = 0; i < NumSocks; i++ ) {
        if( Socks[i] < 0 ) {
            /* Unused - accept the connection */
            Socks[i] = accept( Sock, (struct sockaddr *)&PeerAddr, &AddrLen );
            if( Socks[i] < 0 ) {
                perror( "IPCServer: accept" );
                return IPC_INVALID_SOCK;
            }
            if (logFile) {
                fprintf(logFile,
                        "Socket Connection Accepted! Socket: %d \n", Sock);
                fflush(logFile);
            }

            BLOCKING_OFF( Socks[i] );

            /* Return the socket number we modified */
            return i+1;
        }
    }

    /* If we get here, there are NO unused sockets... */
    n = accept( Sock, (struct sockaddr *)&PeerAddr, &AddrLen );
    if( n < 0 ) {
        perror( "IPCServer: accept" );
        return -1;
    }

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
#if 0 /*!defined(_HPUX_SOURCE)*/
        if( Final ) {
                (void)WSACleanup();
        }
#endif
}

/* Read a string from the given socket */
int GetMsg( IPC_SOCK Sock, char msg[BSIZ] )
{
    int
        n;

    if( Sock < 0 ) {
        return -1;
    }

    n = recv( Sock, msg, BSIZ, 0 );
    if( n < 0 ) {
        if( errno != DO_OVER ) {
            return -1;
        }
        n = 0;
    }
    if(n && logFile) {
        fprintf(logFile,"GetMsg: %s\n", msg);
        fflush(logFile);
    }
    return n;

}

/* Put a message to the given socket */
int PutMsg( char msg[BSIZ], IPC_SOCK Sock )
{
    int
        n, r;

    if( Sock < 0 )      return -1;

#if !defined(WIN32)
    (void)signal( SIGPIPE, SIG_IGN );
#endif

    n = strlen(msg);
    r = send( Sock, msg, n+1, 0 );
    if( r < 0 ) {
#if !defined(WIN32)
        (void)signal( SIGPIPE, SIG_DFL );
#endif
        return -1;
    }

    if (logFile) {
        fprintf(logFile,"PutMsg: %s\n", msg);
        fflush(logFile);
    }

    return r;
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
        perror( "IPCWait" );
        return -1;
    }
    return (n != 0);
}

#if 0 /* Test code [ */
#define NUMSOCKS        16
void main(int argc, char **argv)
{
    int i, n, server = 0, numClients = 0;
    char msg[BSIZ];
    char hostname[256];
    int sock, socks[NUMSOCKS];

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-server") == 0) {
            server = 1;
        }
    }

    if (!PrepareForIPC()) exit(1);
    if (server) {
        sock = InitIPC(NULL, "31415");
        if (sock < 0) exit(1);
        for (i = 0; i < NUMSOCKS; i++) {
            socks[i] = -1;
        }
        while (1) {
            i = IPCServer( sock, numClients ? 100 : 500000, socks, NUMSOCKS );
            if (i > 0) {
                numClients++;
                fprintf(logFile,"Connected new client #%d\n", numClients);
                fflush(logFile);
            }
            if (numClients) {
                for (i = 0; i < NUMSOCKS; i++) {
                    if (socks[i] < 0) continue;

                    n = GetMsg(socks[i], msg);
                    if (n < 0) {
                        socks[i] = -1;
                        numClients--;
                    } else if (n) {
                        puts(msg);
                        sprintf(msg, "%d chars", strlen(msg));
                        PutMsg(msg, socks[i]);
                    }
                }
            }
        }
    } else {
        gethostname(hostname, sizeof(hostname));
        sock = InitIPC(hostname, "31415");
        if (sock < 0) exit(1);
        while (!feof(stdin)) {
            gets(msg);
            if (PutMsg(msg, sock) < 0) {
                exit(1);
            }
            do {
                n = IPCWait(sock, 1000000);
            } while (!n);
            if (n < 0) {
                exit(1);
            }
            if (GetMsg(sock, msg) < 0) {
                exit(1);
            }
            puts(msg);
        }
    }
}
#endif /* ] */

/*** EOF ipc.c ***/
