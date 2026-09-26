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


#include <stdio.h>
#include <string.h>
#include <math.h>
#include "libnum.h"
#include "physics.h"
#include "object.h"
#include "demo_physics.h"


#define GROUND_ROUGHNESS		(0.1)
#define GROUND_ROUGHNESS_FREQUENCY	(5.0)

void surface_chars(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    WC_SURFACE_CHARACTERISTICS *wc_sc)
{
    static WC_SURFACE_CHARACTERISTICS ground_sc = {
	0.0, 0.0, 0.0,			/* wc_x,wc_y,wc_z */
	{ 0.0, 1.0, 0.0 }, 		/* wc_normal */
	1.0,				/* friction */
	GROUND_ROUGHNESS,		/* roughness */
	GROUND_ROUGHNESS_FREQUENCY,	/* roughness frequency */
	NULL				/* whichobj */
    };

    intersect_objects_xyz(obj,x,y,z,wc_sc);

    if (wc_sc->whichobj == NULL) {
	/* Didn't intersect anything -- return the ground */
	ground_sc.wc_x		= x;
	ground_sc.wc_z		= z;
	memcpy(wc_sc,&ground_sc,sizeof(WC_SURFACE_CHARACTERISTICS));
    }
}
