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
 * headlight.c - Headlight code.
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#if !(defined(WIN32) || defined(MAC))
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <Xm/Xm.h>
#endif

#include "global.h"
#include "hw.h"
#include "drive.h"
#include "message.h"
#include "newMessage.h"
#include "sockets.h"

/*****************************************************************
 * headlight_initialize
 * 
 * 	Initialize headlight structure.  Default is low beam,
 * 	headlights off.
 */
void headlight_initialize(
    headlight_type *h)
{
    h->enabled = FALSE;
    h->high_beam = FALSE;
    h->angle = HEADLIGHT_LO_ANG;
    h->x = HEADLIGHT_LO_X;
    h->y = HEADLIGHT_LO_Y;
    h->z = HEADLIGHT_LO_Z;
    h->dirx = HEADLIGHT_LO_DIRX;
    h->diry = HEADLIGHT_LO_DIRY;
    h->dirz = HEADLIGHT_LO_DIRZ;
}

/*****************************************************************
 * headlight_update
 * 
 * 	Update headlight position using xform.
 * 	
 */
void headlight_update(
    int fildes,
    headlight_type *h,
    float xform[4][4])
{
    extern hwDisplay
	disp;
    float
	pos[3],
	dir[3],
	color[3] = { HEADLIGHT_R, HEADLIGHT_G, HEADLIGHT_B };

    /* If car has its lights on, update light source. */
    if ( h->enabled )
    {
	pos[0] = h->x; pos[1] = h->y; pos[2] = h->z;
	dir[0] = h->dirx; dir[1] = h->diry; dir[2] = h->dirz;
	hwTransform( xform, pos );
	hwTransform( xform, dir );
	dir[0] -= pos[0];
	dir[1] -= pos[1];
	dir[2] -= pos[2];
	disp->positionalLight( disp,
			color,
			pos,
			dir,
			h->angle, HEADLIGHT_SPOT/128.0, 0.0 );
    }
}


/*****************************************************************
 * headlight_highbeam
 * 
 * 	Toggle between headlight low and high beam.
 * 	
 */
void headlight_highbeam(
    headlight_type *h)
{
    if ( h->high_beam )
    {
	/* Toggle to low beam. */
	h->high_beam = FALSE;
	h->angle = HEADLIGHT_LO_ANG;
	h->x = HEADLIGHT_LO_X;
	h->y = HEADLIGHT_LO_Y;
	h->z = HEADLIGHT_LO_Z;
	h->dirx = HEADLIGHT_LO_DIRX;
	h->diry = HEADLIGHT_LO_DIRY;
	h->dirz = HEADLIGHT_LO_DIRZ;
    }
    else
    {
	/* Toggle to high beam. */
	h->high_beam = TRUE;
	h->angle = HEADLIGHT_HI_ANG;
	h->x = HEADLIGHT_HI_X;
	h->y = HEADLIGHT_HI_Y;
	h->z = HEADLIGHT_HI_Z;
	h->dirx = HEADLIGHT_HI_DIRX;
	h->diry = HEADLIGHT_HI_DIRY;
	h->dirz = HEADLIGHT_HI_DIRZ;
    }
}


/*****************************************************************
 * headlight_toggle
 * 
 * 	Toggle between headlight on and off.
 * 	
 */
void headlight_toggle(
    int fildes,
    headlight_type *h,
    socket_type *skt,
    int *light_mask)
{
    struct CliLights 
	Msg;

    if ( h->enabled )
    {
	/* Toggle to off. */
	h->enabled = FALSE;

	/* Turn off headlight. */
	*light_mask &= (~0x4);
#if 0
	light_switch( fildes, *light_mask );
#endif


	/* Send off message to server. */
	Msg.MsgType = CLI_LIGHTS;
	Msg.MsgLength = CLI_LIGHTS_SIZE;
	Msg.flags = FALSE;
	if( !PutMsg( (struct IPCMsg *)&Msg, skt->socketnum) ) {
	    perror("PutMsg in headlight_toggle");
	}
    }
    else
    {
	/* Toggle to on. */
	h->enabled = TRUE;

	/* Turn on headlight. */
	*light_mask |= (0x4);
#if 0
	light_switch( fildes, *light_mask );
#endif

	/* Send on message to server. */
	Msg.MsgType = CLI_LIGHTS;
	Msg.MsgLength = CLI_LIGHTS_SIZE;
	Msg.flags = TRUE;
	if( !PutMsg( (struct IPCMsg *)&Msg, skt->socketnum) ) {
	    perror("PutMsg in headlight_toggle");
	}
    }
}
