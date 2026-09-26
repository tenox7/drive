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



#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <signal.h>
#include <netdb.h>
#include <sys/time.h>

#include "global.h"

#define QUERY_SERVICE		"drivequery"
#define SEARCH_QUERY_TIMEOUT	(2000000/256)	/* microseconds */
#define SELECT_TIMEOUT		0

static char hostname[256];
static int hostnamelen;


/*** Called by the searcher ***/
void find_active_servers(
    char servername[256][256],
    int *num_servers)
{
    int i,s;
    struct hostent *hp;		/* pointer to host info for nameserver host */
    struct servent *sp;		/* pointer to service information */
    struct sockaddr_in myaddr_in;	/* for local socket address */
    struct sockaddr_in servaddr_in;	/* for server socket address */
    char response[256];
#ifdef __linux__
    fd_set readfds;
#else
    int readfds;
#endif
    static struct timeval timeout = {0,SEARCH_QUERY_TIMEOUT};

    *num_servers = 0;

    /* clear out address structures */
    memset((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
    memset((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));

    /* Set up the server address. */
    servaddr_in.sin_family = AF_INET;

    /* Where am I? */
    gethostname(hostname,sizeof(hostname));
    if ((hp = gethostbyname(hostname)) == NULL) {
	fprintf(stderr, "%s not found in /etc/hosts\n",hostname);
	return;
    }

    servaddr_in.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;

    if ((sp = getservbyname(QUERY_SERVICE, "udp")) == NULL) {
	fprintf(stderr,"%s not found in /etc/services\n",QUERY_SERVICE);
	return;
    }

    servaddr_in.sin_port = sp->s_port;

    /* Create the socket. */
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
	perror("");
	fprintf(stderr,"Unable to create query socket\n");
	return;
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
    if (bind(s, &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
	perror("");
	fprintf(stderr,"Unable to bind query socket\n");
	return;
    }

    /* Send to each node on the local subnet. */
    for (i=0; i<255; ++i) {
	servaddr_in.sin_addr.s_addr &= 0xffffff00;
	servaddr_in.sin_addr.s_addr |= i;

	if (sendto(s,hostname,strlen(hostname)+1, 0, &servaddr_in,
		sizeof(struct sockaddr_in)) == -1) {
	    continue;
	}

	/* See if they answered */
#ifdef __linux__
	FD_ZERO(&readfds);
	FD_SET(s,&readfds);
#else
	readfds = 1<<s;
#endif
	if (select(s+1,&readfds,0,0,&timeout) != SELECT_TIMEOUT) {
	    if (recv(s, &response, sizeof(response), 0) != -1) {
		strcpy(servername[(*num_servers)++],response);
	    }
	}
    }
}


/*** Called by the server.  Returns INVALID if failed. */
int setup_drive_query_socket(
    void)
{
    struct servent *sp;		/* pointer to service information */
    struct sockaddr_in myaddr_in;	/* for local socket address */
    int skt;

    gethostname(hostname,sizeof(hostname)-1);
    hostnamelen = strlen(hostname)+1;

    /* clear out address structures */
    memset((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));

    /* Set up address structure for the socket. */
    myaddr_in.sin_family = AF_INET;
    myaddr_in.sin_addr.s_addr = INADDR_ANY;

    if ((sp = getservbyname(QUERY_SERVICE, "udp")) == NULL) {
	fprintf(stderr,"%s not found in /etc/services\n",QUERY_SERVICE);
	return(INVALID);
    }

    myaddr_in.sin_port = sp->s_port;

    /* Create the socket. */
    if ((skt = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
	perror("");
	fprintf(stderr,"Unable to create query socket\n");
	return(INVALID);
    }

    /* Bind the server's address to the socket. */
    if (bind(skt, &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
	perror("");
	fprintf(stderr,"Unable to bind address for query socket.\n");
	return(INVALID);
    }

    return(skt);
}


/*** Called by the server occasionally to see if anyone is looking for
 *** a server.
 ***/
void check_drive_query_socket(
    int skt)
{
    static struct timeval timeout = {0,1};
    int addrlen;
#ifdef __linux__
    fd_set readfds;
#else
    int readfds;
#endif
    char buffer[256];
    struct sockaddr_in clientaddr_in;	/* for client's socket address */

    if (skt < 0) return;

#ifdef __linux__
    FD_ZERO(&readfds);
    FD_SET(skt,&readfds);
#else
    readfds = 1<<skt;
#endif
    if (select(skt+1,&readfds,0,0,&timeout) != SELECT_TIMEOUT) {
	memset((char *)&clientaddr_in, 0, sizeof(struct sockaddr_in));
	addrlen = sizeof(struct sockaddr_in);
	if (recvfrom(skt,buffer,(sizeof(buffer)-1),0,&clientaddr_in,&addrlen)
		!= -1) {
	    sendto(skt,hostname,hostnamelen,0,&clientaddr_in,addrlen);
	}
    }
}
