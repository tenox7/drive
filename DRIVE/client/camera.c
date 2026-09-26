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
 * camera.c - Client camera routines.
 */

#include "hw.h"
#include "global.h"
#include "camera.h"
#include <stdlib.h>
#include "drive.h"
#include <math.h>

#if !(defined(WIN32) || defined(MAC))
#include <X11/Xutil.h>  /* Needed????? */
#endif

#define  DEG  *M_PI/180.0

/* Haze strength; larger saturates nearer.  Override with DRIVE_FOG. */

/* KLUDGE KLUDGE KLUDGE */
float camera_pos[3];

/* Depth cue strength; 0 turns it off.  Set from DRIVE_FOG, and from the
 * Config menu at run time.
 */
float driveFogAmount = DEFAULT_FOG_AMOUNT;

void driveFogInit( void )
{
    char *e = getenv("DRIVE_FOG");

    if (getenv("DISABLE_DEPTH_CUE") != NULL) {
	driveFogAmount = 0.0;
    }
    else if (e) {
	driveFogAmount = atof(e);
	if (driveFogAmount < 0.0) driveFogAmount = 0.0;
    }
}

/**********************************************************************
 * point_xform - multiply a point and a matrix.
 *
 */
void point_xform(
    float *x, float *y, float *z,
    float xform[4][4])
{
    float newx, newy, newz;

    newx = xform[0][0] * *x
	 + xform[1][0] * *y
	 + xform[2][0] * *z
	 + xform[3][0];
    newy = xform[0][1] * *x
	 + xform[1][1] * *y
	 + xform[2][1] * *z
	 + xform[3][1];
    newz = xform[0][2] * *x
	 + xform[1][2] * *y
	 + xform[2][2] * *z
	 + xform[3][2];

    *x = newx;
    *y = newy;
    *z = newz;
}


#define CAM_HEIGHT	3.0

/**********************************************************************
 * cam_update
 *
 */
void cam_update(
    int fildes,
    camera_type *c,
    float xform[4][4])

{
    float reference_x, reference_y, reference_z;
    float product_x, product_y, product_z;
    float sintheta, costheta;
    float refx, refy, refz, tmp;
    static hwCamStruct camera;
    extern hwDisplay disp;

    /* Set initial camera position. */
    camera.pos[0] = 0.0;
    camera.pos[1] = CAM_HEIGHT;
    camera.pos[2] = 0.0;
    point_xform( &camera.pos[0], &camera.pos[1], &camera.pos[2], xform );

    /* Set initial camera up vector. */
    camera.up[0] = xform[1][0];
    camera.up[1] = xform[1][1];
    camera.up[2] = xform[1][2];
    tmp = camera.up[0]*camera.up[0]
	+ camera.up[1]*camera.up[1]
	+ camera.up[2]*camera.up[2];
    if( tmp < 0.001 ) {
	/* Singularity! */
	camera.up[0] = 0.0;
	camera.up[1] = 1.0;
	camera.up[2] = 0.0;
    }

    /* Calculate raw camera reference vector. */
    refx = 0.0;
    if ( (c->mode == CAM_MODE_ALT_VIEW) || (c->mode == CAM_MODE_ZOOM_VIEW) ) 
	 refy = CAM_HEIGHT;
    else refy = 0.0;

    refz = 100.0;
    point_xform( &refx, &refy, &refz, xform );
    reference_x = refx - camera.pos[0];
    reference_y = refy - camera.pos[1];
    reference_z = refz - camera.pos[2];

    switch (c->mode) {
      case CAM_MODE_SKYCAM:
      case CAM_MODE_AUTOCAM:
	/* Skycam camera reference point is where the normal camera's
	 * position is.
	 */
	refx = camera.pos[0];
	refy = camera.pos[1];
	refz = camera.pos[2];

	/* Get normalized vector in X-Z plane and use to get values for
	 * sin and cos.
	 */
	NORMALIZE2( reference_x, reference_z );
	costheta = reference_z;
	sintheta = reference_x;

	/* Change camera position. */
	camera.pos[0] += c->skycam_x * costheta + c->skycam_z * sintheta;
	camera.pos[1] += c->skycam_y;
	camera.pos[2] += c->skycam_z * costheta - c->skycam_x * sintheta;

	/* Change camera up vector. */
	camera.up[0] = 0.0;
	camera.up[1] = 1.0;
	camera.up[2] = 0.0;

	/* Make sure the rest of camera is correct. */
	camera.field = CAM_FOV;
	camera.planes[0] = -c->skycam_distance + 2.0 ;
	camera.planes[1] = CAM_DISTANCE_TO_BACK;
	camera.perspective = 1;
	break;

      case CAM_MODE_PREVIEW:
	/******TEMPORARY (should calculate this) ******/
	c->preview_distance = 20.0;

	/* One degree per frame was a sane spin on a 15fps workstation; on
	 * anything modern it is a blur, so go by elapsed time instead.
	 */
	{
	    static double lastSpin = -1.0;
	    double now = disp->getElapsedTime( disp );

	    if (lastSpin < 0.0 || now < lastSpin) lastSpin = now;
	    c->preview_angle += (float)((now - lastSpin) * PREVIEW_SPIN_RATE);
	    lastSpin = now;
	}
	if (c->preview_angle > (2.0 * M_PI))
	    c->preview_angle -= (2.0 * M_PI);

	refx = 0.0;
	refy = 0.0;
	refz = 0.0;

	camera.pos[0] = c->preview_distance
	    * (float)cos((double)(c->preview_angle));
	camera.pos[1] = 10.0;
	camera.pos[2] = c->preview_distance
	    * (float)sin((double)(c->preview_angle));

	camera.up[0] = 0.0;
	camera.up[1] = 1.0;
	camera.up[2] = 0.0;

	camera.field = CAM_FOV;
	camera.planes[0] = - c->preview_distance;
	camera.planes[1] = 100.0;
	camera.perspective = 1;
	break;

      case CAM_MODE_NORMAL:
      case CAM_MODE_ALT_VIEW:
      case CAM_MODE_ZOOM_VIEW:
      default:
	switch( c->direction )
	{
	  case CAM_DIRECTION_BACK:
	    /* Mirror the reference vector. */
	    refx = camera.pos[0] - reference_x;
	    refy = camera.pos[1] - reference_y;
	    refz = camera.pos[2] - reference_z;
	    break;

	  case CAM_DIRECTION_RIGHT:
	    /* Calculate cross_product( up_vector, reference_vector ). */
	    product_x = camera.up[1] * reference_z - camera.up[2] * reference_y;
	    product_y = camera.up[2] * reference_x - camera.up[0] * reference_z;
	    product_z = camera.up[0] * reference_y - camera.up[1] * reference_x;

	    /* Change reference vector. */
	    refx = camera.pos[0] + product_x;
	    refy = camera.pos[1] + product_y;
	    refz = camera.pos[2] + product_z;
	    break;

	  case CAM_DIRECTION_LEFT:
	    /* Calculate cross_product( reference_vector, up_vector ). */
	    product_x = reference_y * camera.up[2] - reference_z * camera.up[1];
	    product_y = reference_z * camera.up[0] - reference_x * camera.up[2];
	    product_z = reference_x * camera.up[1] - reference_y * camera.up[0];

	    /* Change reference vector. */
	    refx = camera.pos[0] + product_x;
	    refy = camera.pos[1] + product_y;
	    refz = camera.pos[2] + product_z;
	    break;

	  case CAM_DIRECTION_FRONT:
	  default:
	    /* Camera sent from server looks forward.  No need to
	     * change it.
	     */
	    break;
	}

	/* Calculate new camera reference vector. */
	reference_x = refx - camera.pos[0];
	reference_y = refy - camera.pos[1];
	reference_z = refz - camera.pos[2];

	/* Pull the camera back to give us a wider view. */
	NORMALIZE3( reference_x, reference_y, reference_z );
	camera.pos[0] -= reference_x * CAM_DISTANCE_TO_CENTER;
	camera.pos[1] -= reference_y * CAM_DISTANCE_TO_CENTER;
	camera.pos[2] -= reference_z * CAM_DISTANCE_TO_CENTER;

	camera.dir[0] = reference_x;
	camera.dir[1] = reference_y;
	camera.dir[2] = reference_z;

	/* Make sure the rest of camera is correct. */
	if (c->mode == CAM_MODE_ZOOM_VIEW) 
	   camera.field = cstate.gunsight_fov;
	else camera.field = CAM_FOV;
	camera.planes[0] = -CAM_DISTANCE_TO_REF;
	camera.planes[1] = CAM_DISTANCE_TO_BACK;
	camera.perspective = 1;
    }

#if 0
    /* Convert Starbase planes to Hoverware planes */
    camera.planes[1] -= camera.planes[0];
    camera.planes[0] = -camera.planes[0];
#endif

    /* Update camera. */
    camera.dir[0] = cstate.view_direction[0] = refx - camera.pos[0];
    camera.dir[1] = cstate.view_direction[1] = refy - camera.pos[1];
    camera.dir[2] = cstate.view_direction[2] = refz - camera.pos[2];

#if 1
    tmp =	   camera.dir[0]*camera.dir[0]	
		+  camera.dir[1]*camera.dir[1]
		+  camera.dir[2]*camera.dir[2];
    tmp = sqrt( tmp );

    camera.planes[0] += tmp;
    camera.planes[1] += tmp;

    camera.planes[1] *= 2.0;

    if( tmp > 0.0 ) tmp = 1.0 / tmp;
    camera.dir[0] *= tmp;
    camera.dir[1] *= tmp;
    camera.dir[2] *= tmp;
#endif

    camera_pos[0] = camera.pos[0];
    camera_pos[1] = camera.pos[1];
    camera_pos[2] = camera.pos[2];

    /* HACK!  We need window aspect ratio here */
    if (cstate.gWinHeight > 0) {
	camera.aspect = (float)cstate.gWinWidth / (float)cstate.gWinHeight;
    }
    else {
	camera.aspect = 1.0;
    }

    disp->setCamera( disp, &camera );

#ifdef GLFW
    /* The Starbase version called depth_cue_range() here across the whole
     * view volume, which is what gave the scene its haze.  Linear fog
     * between the clip planes is the equivalent; planes[1] was doubled
     * above for the clip, so halve it to fade out at the horizon.
     */
    if (driveFogAmount > 0.0 && c->mode != CAM_MODE_PREVIEW) {
	float p[2];

	/* Bigger amount saturates nearer the camera, i.e. more haze. */
	p[0] = camera.planes[0];
	p[1] = p[0] + (camera.planes[1] * 0.5 - p[0]) / driveFogAmount;
	if (p[1] <= p[0]) p[1] = p[0] + 1.0;
	disp->doFog( disp, HW_TRUE, cstate.daylight.back_clr );
	disp->fogParams( disp, HW_FOG_LINEAR, p, 0.0 );
    }
    else {
	/* No depth cue on the spinning vehicle preview: its near plane is
	 * behind the camera, which would fog the model into the sky.
	 */
	disp->doFog( disp, HW_FALSE, cstate.daylight.back_clr );
    }
#endif
}
