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
 * sockets.c - Socket communication functions.
 */

#include <stdio.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include "global.h"
#include "sockets.h"

/* From utils.c */
extern void popup_error_dialog(int);

#define	BLOCKING_OFF(Sock) (void)fcntl( Sock, F_SETFL, O_NONBLOCK )

#define	BLOCKING_ON(Sock) (void)fcntl( Sock, F_SETFL, 0 )

/*****************************************************************
 * soc_create
 * 
 * 	Creates a socket.
 *
 * Inputs:
 *		none.
 * Outputs:
 *		Returns socket descriptor.
 */
socket_type *soc_create(
    void)
{
    socket_type *s;
	
    /* Malloc a new socket structure. */
	
    s = (socket_type *)malloc( sizeof( socket_type ) );
    if ( s == NULL ) {
		fprintf(stderr,"soc_create: Out of memory.\n");
		return( (socket_type *)NULL );
    }
	
    /* Create the actual socket. */
	
    s->socketnum = socket( AF_INET, SOCK_STREAM, 0 );
    if ( s->socketnum < 0 ) 
    {
		perror( "soc_create" );
		fprintf( stderr, "soc_create: Unable to create socket.\n" );
		exit( -1 );
    }
	
    return s;
}


/*****************************************************************
 * soc_close
 * 
 * 	Shutdown socket and free file descriptor. 
 *
 * Inputs:
 *		s		Socket structure.
 *
 * Outputs:
 *		[none]
 */
void soc_close(
    socket_type *s)
{
    /* Shutdown socket for both reading and writing. */
	
    if ( shutdown( s->socketnum, 2 ) < 0 )
    {
	perror( "soc_close" );
	exit( -1 );
    }
	
    /* Free up this file descriptor. */
	
    if ( close( s->socketnum ) < 0 )
    {
	perror( "soc_close" );
	exit( -1 );
    }
}


/*****************************************************************
 * soc_bind
 * 
 * 	Bind name to socket.
 *
 * Inputs:
 *		sock	Socket descriptor.
 *		port	Port number.
 * Outputs:
 *		[none]
 */
void soc_bind(
    socket_type *s,
    char *name)
{
    struct servent *servinfo;
    int retry;
	
    /* Find socket address using wildcards. */
	
    s->address.sin_family = AF_INET;
    s->address.sin_addr.s_addr = INADDR_ANY;
	
    /* Look at /etc/services to get the port number. */
	
    servinfo = getservbyname( name, "tcp" );
    if ( servinfo == NULL) 
    {
	fprintf( stderr, "Entry \"%s\" not in /etc/services.\n", name );
	popup_error_dialog(2);
    }
    s->address.sin_port = servinfo->s_port;
	
    /* Bind name to socket. */
	
    retry = 0;
    while (( bind( s->socketnum, &s->address, sizeof( struct sockaddr_in ) ) )
	   && (retry < SOC_RETRIES)) 
    {
	if ( errno != EADDRINUSE ) 
	{
	    perror( "soc_bind" );
	    fprintf( stderr, "Unable to bind socket address.\n" );
	    exit( 1 );
	}
	else 
	{
	    retry++;
	    fprintf(stderr,"Socket in use (waiting %d seconds)...\n",
		    SOC_TIMEOUT );
	    sleep( SOC_TIMEOUT );
	}
    }
	
    /* If we have too many retries, then scream and die. */
    if ( retry == SOC_RETRIES )
    {
	fprintf( stderr, "Socket retry failed.\n" );
	exit( -1 );
    }
}


/*****************************************************************
 * soc_listen
 * 
 * 	Creates a socket, then does "the right thing" dependant 
 *      upon whether or not the client flag is true or false.
 *
 * Inputs:
 *		name		Name for port in /etc/services.
 *		client		TRUE if called from a client.
 * Outputs:
 *		none
 */
void soc_listen(
    socket_type *s)
{
    if ( listen( s->socketnum, 5 ) < 0 )
    {
	perror( "soc_listen" );
	fprintf( stderr, "Unable to listen on socket.\n" );
	exit( -1 );
    }
}


/*****************************************************************
 * soc_connect
 * 
 *	Connects the specified socket to the server at port
 *	running on host.
 * 
 * Inputs:
 * 		host		Host where server is running.
 *              name		Name for /etc/services entry.
 * 		socket		Socket to use for connection.
 * Outputs:
 *		returns status
 */
boolean_type soc_connect(
    socket_type *s,
    char *host,
    char *name)
{
    struct hostent *hostinfo;
    struct servent *servinfo;
	
    /* Read the information for the specified host. */
    s->address.sin_family = AF_INET;
    hostinfo = gethostbyname(host);
    if (hostinfo == NULL) {
	return(FALSE);
    }
	
    /* Look at /etc/services to get the port number. */
    s->address.sin_addr.s_addr =
	((struct in_addr *)(hostinfo->h_addr))->s_addr;
    servinfo = getservbyname(name, "tcp");
    if (servinfo == NULL) {
	fprintf(stderr, "Entry \"%s\" not in /etc/services\n", name);
	popup_error_dialog(2);
    }
    s->address.sin_port = servinfo->s_port;
	
    /* Attempt to initiate a socket connection with the server on the 
     * given host with the port number specified in /etc/services. 
     */
    if (connect(s->socketnum,&s->address,sizeof(struct sockaddr_in)) < 0) {
	return(FALSE);
    }

    fprintf(stderr, "Connected to host: %s.\n", host);
    return(TRUE);
}


/*****************************************************************
 * soc_accept
 * 
 *	Accepts a connection with the given socket to the specified
 *	server.
 * 
 * Inputs:
 *		address		Structure to contain connection
 *				address.
 * Outputs:
 *		msgsock		Socket for server.
 */
socket_type *soc_accept(
    socket_type *s)
{
    socket_type *connection;
    int size;

    /* Malloc a new socket structure. */
	
    connection = (socket_type *)malloc( sizeof( socket_type ) );
    if ( connection == NULL ) {
	fprintf(stderr,"soc_accept: Out of memory.\n");
	return( (socket_type *)NULL );
    }
	
    /* Accept connection on socket. */
    size = sizeof( struct sockaddr_in );
    connection->socketnum = accept( s->socketnum, &(connection->address),
				   &size );

    if ( connection->socketnum < 0 ) 
    {
	perror( "soc_accept" );
	exit( -1 );
    }
	
    return( connection );
}


/*****************************************************************
 * soc_write
 * 
 *	Makes a connection with the given socket to the specified
 *	server, and sends the header message.
 * 
 * Inputs:
 *		buf		Message to send.
 *		size		Size of message.
 * Outputs:
 *		[None]
 */
void soc_write(
    socket_type *s,
    char *buf,
    int size)
{
    /* Send message. */
	
    BLOCKING_ON(s->socketnum);
    if ( write( s->socketnum, buf, size ) < 0 )
	perror( "soc_write" );
    BLOCKING_OFF(s->socketnum);
}


/*****************************************************************
 * soc_read
 * 
 *	Makes a connection with the given socket to the specified
 *	server, and reads some stuff.
 * 
 * Inputs:
 *		buf		Message to read.
 *		size		Size of message.
 * Outputs:
 *		[None]
 */
int soc_read(
    socket_type *s,
    char *buf,
    int size)
{
    int status;
    int bytes=0;

    /* Read message. */
	
    BLOCKING_ON(s->socketnum);

    while (bytes < size) {
	status = read(s->socketnum,buf+bytes,size-bytes);
	/* if none received, the socket has closed */
	if( status == 0)
	    return(0);
	else if (status == -1)
	    perror( "soc_read" );
	bytes += status;
    }
    BLOCKING_OFF(s->socketnum);
    return(bytes);
}
