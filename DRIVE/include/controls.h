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

#ifndef _CONTROLS_H_INCLUDED
#define _CONTROLS_H_INCLUDED

typedef struct {
    float pointer_x,pointer_y;
    float aim_pointer_x,aim_pointer_y;
    int gear;
    float altitude_accel;
    float throttle;
} CONTROLS;

#ifdef COPY_GLOBAL_CONTROLS_NEEDED
/* extern declarations for all control routines */
extern void copy_global_controls(
    struct object_struct *obj);
#endif /* COPY_GLOBAL_CONTROLS_NEEDED */

#endif /* _CONTROLS_H_INCLUDED */
