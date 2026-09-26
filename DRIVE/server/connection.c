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
 * connection.c - Functions to manipulate connection structures.
 */

#include "global.h"
#include "connection.h"

/*****************************************************************
 * TAG( con_create )
 *
 *	Creates and initializes a connection structure.
 *
 * Algorithm:
 *		none.
 * Returns:
 *		Pointer to newly created connection structure, or NULL
 *              if memory for structure could not be allocated.
 * Arguments:
 *		none.
 */
connection_type *con_create(
    void)
{
    connection_type *newcon;

    newcon = (connection_type *)malloc( sizeof( connection_type ) );

    if ( newcon == NULL ) {
	fprintf(stderr,"con_create: Out of memory.\n");
	return( (connection_type *)NULL );
    }

    newcon->hostname[0] = '\0';
    newcon->username[0] = '\0';
    newcon->socket = NULL;
    newcon->obj = NULL;
    newcon->frame_count = 0;
    newcon->mode = CON_DEFAULT_MODE;

    newcon->watch = NULL;
    newcon->next = NULL;
    newcon->prev = NULL;

    return newcon;
}


/*****************************************************************
 * TAG( con_dump )
 *
 *	Dumps the contents of an connection structure.
 *
 * Algorithm:
 *		none.
 * Returns:
 *		nothing.
 * Arguments:
 *		c	Connection structure.
 *		s	String to print.
 *              l	Indentation level.
 */
void con_dump(
    connection_type *c,
    char *s,
    int l)
{
    INDENT( l );

    fprintf( stderr, "CONNECTION %s:\n", s );

    if ( c ) {
	INDENT( l+1 );
	fprintf( stderr, "hostname: %s\n", c->hostname );
	INDENT( l+1 );
	fprintf( stderr, "username: %s\n", c->username );
	fprintf( stderr, "\n" );
    } else {
	INDENT( l+1 );
	fprintf( stderr, "(nil)\n" );
    }
}
