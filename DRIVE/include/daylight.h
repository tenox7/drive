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
 * daylight.h - Sunlight definitions.
 */

#ifndef _DAYLIGHT_INCLUDED
#define _DAYLIGHT_INCLUDED

/* Daylight structure. */

typedef struct {
    boolean_type use_server_time;	/* TRUE if server is dictating time. */
    boolean_type static_time;		/* TRUE if time should not change. */
    time_t initial_time;		/* Actual time when reference is set. */
    time_t reference_time;		/* Time sent from server. */
    int mask;				/* Which lights are on? */
    float back_clr[3];			/* OUTPUT from daylight routine. */
} daylight_type;


extern void daylight_initialize(
    daylight_type *d);
extern void daylight_update(
    int fildes,
    daylight_type *d,
    boolean_type force_update);

#endif /* _DAYLIGHT_INCLUDED */
