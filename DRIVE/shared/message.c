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
 * message.c - Functions to manipulate message structures.
 */

#include <stdlib.h>
#include "global.h"
#include "message.h"

/*****************************************************************
 * msg_create
 *
 *	Creates and initializes a message structure.
 *
 * Algorithm:
 *		none.
 * Returns:
 *		Pointer to newly created message structure, or NULL
 *              if memory for structure could not be allocated.
 * Arguments:
 *		none.
 */
message_header_type *msg_create(
    void)
{
    message_header_type *newmsg;

    newmsg = (message_header_type *)malloc( sizeof( message_header_type ) );

    if ( newmsg == NULL ) {
	fprintf(stderr,"msg_create: Out of memory.\n");
	return( (message_header_type *)NULL );
    }

    newmsg->type = 0;
    newmsg->data.size = 0;

    return( newmsg );
}


/*****************************************************************
 * msg_dump
 *
 *	Dumps the contents of an message structure.
 *
 * Algorithm:
 *		none.
 * Returns:
 *		nothing.
 * Arguments:
 *		s	String to print.
 *		msg	Message structure.
 *              l	Indentation level.
 */
void msg_dump(
    message_header_type *msg,
    char *s,
    int l)
{
    INDENT( l );
    fprintf( stderr, "message_header_type %s:\n", s );

    if ( msg )
    {
	INDENT( l+1 );
	fprintf( stderr, "type: %02X\n", msg->type );
	INDENT( l+1 );
	fprintf( stderr, "size: %02X\n", msg->data.size );
	fprintf( stderr, "\n" );
    }
    else
    {
	INDENT( l+1 );
	fprintf( stderr, "(nil)\n" );
    }
}
