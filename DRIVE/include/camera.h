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
 * camera.h - Client camera definitions.
 */

#ifndef _CAMERA_INCLUDED
#define _CAMERA_INCLUDED


/* Camera type definitions. */

typedef struct camera_struct camera_type;

/* Camera structure definition. */

struct camera_struct
{
    unsigned int mode;

    /* Normal mode parameters. */
    int direction;

    /* Skycam mode parameters. */
    boolean_type skycam_dirty;
    float skycam_x, skycam_y, skycam_z;
    int skycam_up_angle;
    int skycam_around_angle;
    float skycam_distance;

    /* Preview mode parameters. */
    float preview_angle;
    float preview_distance;
};

/* Camera mode definitions. */
#define CAM_MODE_NORMAL		0
#define CAM_MODE_SKYCAM		1
#define CAM_MODE_PREVIEW	2
#define CAM_MODE_AUTOCAM	3
#define CAM_MODE_ALT_VIEW	4
#define CAM_MODE_ZOOM_VIEW	5

/* Camera direction definitions. */
#define CAM_DIRECTION_FRONT	0
#define CAM_DIRECTION_BACK	1
#define CAM_DIRECTION_RIGHT	2
#define CAM_DIRECTION_LEFT	3

#define CAM_DISTANCE_TO_REF	100.0

/* Depth cue strength; see driveFogAmount in camera.c */
#define DEFAULT_FOG_AMOUNT	1.8

/* Vehicle preview spin, radians per second */
#define PREVIEW_SPIN_RATE	0.55
#define CAM_DISTANCE_TO_BACK	HORIZON
#define CAM_DISTANCE_TO_CENTER	3.0

/* Skycam. */
#define SKYCAM_UPANGLE_MIN	2
#define SKYCAM_UPANGLE_MAX	85
#define SKYCAM_UPANGLE_INC	2
#define SKYCAM_AROUNDANGLE_INC	2
#define SKYCAM_DISTANCE_INC	2.0
#define SKYCAM_DISTANCE_MIN	10.0

#ifdef PEXDRIVE
extern void cam_update(
    camera_type *c,
    float old_xform[4][4]);
#else
extern void cam_update(
    int fildes,
    camera_type *c,
    float xform[4][4]);
#endif  /* PEXDRIVE */

#endif /* _CAMERA_INCLUDED */
