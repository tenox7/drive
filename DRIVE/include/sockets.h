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
 * sockets.h - Definitions for socket communication funtions.
 */
#ifndef _SOCKETS_INCLUDED
#define _SOCKETS_INCLUDED

#ifdef WIN32

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#else

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

#endif

#include "newMessage.h"

/* Socket type definitions. */

typedef struct sock socket_type;

/* Socket structure definition. */

struct sock
{
    int socketnum;
    int index;
    struct sockaddr_in address;
};

/*
 * Return value declarations.
 */
extern socket_type *soc_create(
    void);
extern void soc_close(
    socket_type *s);
extern void soc_bind(
    socket_type *s,
    char *name);
extern void soc_listen(
    socket_type *s);
extern boolean_type soc_connect(
    socket_type *s,
    char *host,
    char *name);
extern socket_type *soc_accept(
    socket_type *s);
extern void soc_write(
    socket_type *s,
    char *buf,
    int size);
extern int soc_read(
    socket_type *s,
    char *buf,
    int size);
extern int FlushMsg(
    IPC_SOCK Sock);
extern void TermIPC(
    IPC_SOCK Sock);
extern IPC_SOCK InitIPC(
    char *Host,
    char *Service);

#define SOC_RETRIES		15
#define SOC_TIMEOUT		5
#define DEFAULT_SOCKET_STRING	"22747"

#endif /* _SOCKETS_INCLUDED */
