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
 * connection.h - Definitions for client connections.
 */

#ifndef _CONNECTION_INCLUDED
#define _CONNECTION_INCLUDED

#include "sockets.h"
#include "object.h"

/* Connection type definitions. */

typedef struct connection connection_type;

/* Connection structure definition. */

struct connection
{
    char hostname[10];		/* Where host is connecting from. */
    char username[10];		/* Name of connected user. */
    int  number;		/* Position to start the player in */
    int  index;                 /* Index in various arrays */
    socket_type *socket;	/* Client connection. */
    DRIVE_OBJECT *obj;		/* Client vehicle object. */
    drive_start_type start_pos; /* Client's requested starting position */
    int frame_count;		/* Count of unacknowledged frames. */
    int ready;			/* client is ready to receive frames */
    int initialized;		/* client is primed and ready to start */
    unsigned int mode;		/* Connection mode. */
    connection_type *watch;	/* Connection to watch in watch mode. */

    connection_type *prev;
    connection_type *next;
};

/* Connection modes. */
#define CON_DEFAULT_MODE	(0x00000000)
#define CON_WATCH_MODE		(0x00000001)

/*
 * Return value declarations.
 */
connection_type *con_create(void);

#endif /* _CONNECTION_INCLUDED */
