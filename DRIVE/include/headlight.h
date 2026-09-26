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
 * headlight.h - Client definitions.
 */

#ifndef _HEADLIGHT_INCLUDED
#define _HEADLIGHT_INCLUDED

#include "sockets.h"

/* Headlight structure. */

typedef struct {
    boolean_type enabled;	/* TRUE if lights on.. */
    boolean_type high_beam;	/* TRUE if high_beam on. */
    float angle;		/* Angle of headlight cone. */
    float x, y, z;		/* Light position. */
    float dirx, diry, dirz;	/* Light direction. */
} headlight_type;

#ifndef PEXDRIVE
extern void headlight_initialize(
    headlight_type *h);
extern void headlight_toggle(
    int fildes,
    headlight_type *h,
    socket_type *skt,
    int *light_mask);
extern void headlight_highbeam(
    headlight_type *h);
extern void headlight_update(
    int fildes,
    headlight_type *h,
    float xform[4][4]);

#endif

/* Headlight default value definitions. */
#define HEADLIGHT_INDEX		2
#define HEADLIGHT_TYPE		(POSITIONAL)
#define HEADLIGHT_R		(1.0)
#define HEADLIGHT_G		(1.0)
#define HEADLIGHT_B		(1.0)
#define HEADLIGHT_ATTR		(SPOT_LIGHT | CONE_LIGHT)
#define HEADLIGHT_SPOT		6
#define HEADLIGHT_ATTEN		(0.01)
#define HEADLIGHT_LO_ANG	(25.0)
#define HEADLIGHT_LO_X		(0.0)
#define HEADLIGHT_LO_Y		(5.0)
#define HEADLIGHT_LO_Z		(0.0)
#define HEADLIGHT_LO_DIRX	(0.0)
#define HEADLIGHT_LO_DIRY	(0.0)
#define HEADLIGHT_LO_DIRZ	(20.0)
#define HEADLIGHT_HI_ANG	(35.0)
#define HEADLIGHT_HI_X		(0.0)
#define HEADLIGHT_HI_Y		(4.0)
#define HEADLIGHT_HI_Z		(0.0)
#define HEADLIGHT_HI_DIRX	(0.0)
#define HEADLIGHT_HI_DIRY	(0.0)
#define HEADLIGHT_HI_DIRZ	(40.0)

#endif /* _HEADLIGHT_INCLUDED */
