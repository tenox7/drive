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
 * frame.c - Routines for parsing frame messages.
 */

#include "object.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "global.h"
#include "message.h"
#include "gauge.h"
#include "drive.h"
#include "headlight.h"
#if !(defined(WIN32) || defined(MAC))
#include "gui.h"
#endif
#if defined(HAVE_CONFIG_H)
# include "config.h"
#endif

/* Frame parsing jump table. */

static char last_dynamic_name[10];

static int debug_frame = 0;

#define PRINT_ROUTINE_NAME(NAME)	\
    if(debug_frame) printf("inside: (NAME) \n");

/*****************************************************************
 * frame_end
 * 
 * 	Process end-of-frame message.
 * 	
 * Inputs:
 *		m	Message.
 * Outputs:
 *		none
 */
void frame_end( struct IPCMsg *Msg)
{
    PRINT_ROUTINE_NAME( frame_end );
    cstate.frame_ready = TRUE;
/* HACK! */
    client_update();
}


/*****************************************************************
 * frame_position
 * 
 * 	Get a new car position from the server.
 * 	
 * Inputs:
 *		m	Message.
 * Outputs:
 *		none
 */
void frame_position( struct IPCMsg *Msg)
{
    float dirangle;
    float xdir_home, zdir_home, dirangle_home;
    struct SrvFramePosition *pmsg;
    float xdir = 0.0;
    float ydir = 0.0;
    float zdir = 1.0;

    PRINT_ROUTINE_NAME( frame_position );

    /* Point at position message in receive buffer and increment
     * receive buffer pointer.
     */
    pmsg = (struct SrvFramePosition  *)Msg;

    /* Get position from server message and update camera. */
    if ( (cstate.camera.mode == CAM_MODE_ALT_VIEW) || 
	 (cstate.camera.mode == CAM_MODE_ZOOM_VIEW))
	cam_update( img_fildes, &cstate.camera, pmsg->alt_view_xform );
    else
	cam_update( img_fildes, &cstate.camera, pmsg->xform );

    /* Now that the camera has been updated, update the explosions, if any */
    explosion_update(img_fildes);

    /* Update daylight (HW needs this every frame right after the camera) */
    daylight_update( -1, &cstate.daylight, TRUE );

    /* Update headlight position. */
    headlight_update( img_fildes, &cstate.headlight, pmsg->xform );

    /* Calculate direction of travel vector. */
    point_xform( &xdir, &ydir, &zdir, pmsg->xform );
    xdir -= pmsg->xform[3][0];
    zdir -= pmsg->xform[3][2];
    dirangle = FATAN2( xdir, zdir );

    /* Calculate direction back to Noobyville vector. */
    xdir_home = -pmsg->xform[3][0];
    zdir_home = -pmsg->xform[3][2];
    NORMALIZE2( xdir_home, zdir_home );
    dirangle_home = FATAN2( xdir_home, zdir_home );

    compassUpdate(&compass,dirangle,dirangle_home,FALSE);

    /* Update radar information. */
    radar.xpos = pmsg->xform[3][0];
    radar.ypos = pmsg->xform[3][1];
    radar.zpos = pmsg->xform[3][2];
    memcpy( radar.xform, pmsg->xform, sizeof(radar.xform) );
#if 0
    _hp_invert(pmsg->xform,radar.ixform,0);
#endif
    radar.angle = dirangle;

}


/*****************************************************************
 * frame_gauge
 * 
 * 	Update gauges.
 * 	
 * Inputs:
 *		m	Message.
 * Outputs:
 *		FALSE 	to continue parsing.
 */
void frame_gauge( struct IPCMsg *Msg)
{
    struct SrvFrameGauge *gauge;

    /* Increment input buffer pointer past gauge message. */
    gauge = (struct SrvFrameGauge *)Msg;

    PRINT_ROUTINE_NAME( frame_gauge );

    /* Update gauges. */
    cstate.rpm = gauge->rpm;
    cstate.mph = gauge->mph;

    if( cstate.use_sound ) {
	if (cstate.rpm <= 1000) 
	    update_rpm(1000);
	else if (cstate.rpm >= 8000) 
	    update_rpm(8000);
	else
	    update_rpm(cstate.rpm);
    }

    update_gauge_from_message(&left_main_gauge,gauge);
    update_gauge_from_message(&right_main_gauge,gauge);
    gaugeUpdate(&turbometer,  gauge->num_turbos, FALSE);
    gaugeUpdate(&blastometer, gauge->num_shells, FALSE);

    bargraphUpdate(gauge->y_val, FALSE );

    if (ABS(gauge->x_val - cstate.xpointer_value) > 0.01) {
	undrawWheel();
	cstate.xpointer_value = gauge->x_val;
	drawWheel(gauge->x_val);
    }

}


void draw_hw_seg( float xform[4][4], int seg )
{
    extern hwDisplay
	disp;
    struct HwObjCache
	*cache;
    int
	i;

    cache = findHwSegment( seg );
    if( !cache ) return;

    if( xform ) disp->pushMatrix( disp, xform );
    if( cache->segList ) {
	for( i = 0; i < cache->objListSize; i++ ) {
	    draw_hw_seg( cache->matList[i], cache->segList[i] );
	}
    }
    else {
	for( i = 0; i < cache->objListSize; i++ ) {
	    if( cache->objList[i] ) {
		cache->objList[i]->draw( cache->objList[i] );
	    }
	}
    }
    if( xform ) disp->popMatrix( disp );
}

/*****************************************************************
 * frame_matrix
 * 
 * 	Get a new object matrix from the server.
 * 	
 * Inputs:
 *		m	Message.
 * Outputs:
 *		FALSE 	to continue parsing.
 */
void frame_matrix( struct IPCMsg *Msg)
{
#ifdef GLFW
    /* The vehicle picker owns the window; do not paint the world into it. */
    if (dashPickerActive()) return;
#endif
    struct SrvFrameMatrix *mmsg;

    /* Point at matrix message in receive buffer and increment
     * receive buffer pointer.
     */
    mmsg = (struct SrvFrameMatrix *)Msg;

    PRINT_ROUTINE_NAME( frame_matrix );

    /* Update display list segment matrix using the matrix sent
     * from the server.
     */
    if((cstate.camera.mode == CAM_MODE_SKYCAM) ||
       (cstate.camera.mode == CAM_MODE_AUTOCAM) ||
       (mmsg->connection != connection_id))
    {
#if 1
	draw_hw_seg( mmsg->xform, mmsg->segment );
#else
	push_matrix3d( img_fildes, mmsg->xform );
	refresh_segment( img_fildes, mmsg->segment );
	if(cstate.mode & CLIENT_SHOW_NAMES_MODE )  
	{
	    if( last_dynamic_name[0] != '\0')
	    {
		character_height(img_fildes,3.0);
		text_alignment(img_fildes,TA_CENTER_SB, TA_BASE,0,0);
		text_color(img_fildes, 1.0, 1.0, 1.0);
		line_color(img_fildes,1.0, 1.0, 1.0);
		move3d(img_fildes, 0.0, 0.0, 0.0);
		draw3d(img_fildes, 0.0, 4.5, 0.0);
		text3d(img_fildes, 0.0, 4.5, 0.0, last_dynamic_name,
		    WORLD_COORDINATE_TEXT, 0);
		last_dynamic_name[0] = '\0';
	    }
	}
	pop_matrix( img_fildes );
#endif
    }

}

/*****************************************************************
 * frame_radar
 * 
 * 	Get a radar message from the server.
 * 	
 * Inputs:
 *		m	Message.
 * Outputs:
 *		FALSE 	to continue parsing.
 */
void frame_radar( struct IPCMsg *Msg)
{


#if !defined(__linux__)  /* [ */
    int i;
    struct SrvFrameRadar *rmsg;
    float x,y,z, vx, vy, vz, dist;
    float red, green, blue;
    int dcx, dcy, dcz;
    char buff[10];

#if 0  /* [ */
    /* Set up for distance printing */
    dccharacter_width( img_fildes, 8 );
    dccharacter_height( img_fildes, 16 );

    rmsg = (struct SrvFrameRadar *)Msg;

    PRINT_ROUTINE_NAME( frame_radar );

    /* Unpack radar message structures. */
    for (i=0; i<SRV_FRAME_RADAR_NUM(rmsg->MsgLength); i++)
    {

	/* Copy the information into the car_list */
	radar.car_list[i].x = x = rmsg->RadarTypes[i].x_pos;
	radar.car_list[i].y = y = rmsg->RadarTypes[i].y_pos;
	radar.car_list[i].z = z = rmsg->RadarTypes[i].z_pos;
	red = radar.car_list[i].r = rmsg->RadarTypes[i].r;
	green = radar.car_list[i].g = rmsg->RadarTypes[i].g;
	blue = radar.car_list[i].b = rmsg->RadarTypes[i].b;

	/* Draw the 'heads-up' position of the cars */

	if(cstate.mode & CLIENT_HUD_MODE ) 
	{
	    /* Calculate dot product of view direction and car direction */
	    transform_point( img_fildes, WORLD_TO_MC, x, y, z, &x, &y, &z );
	    vx = radar.xpos; vy = radar.ypos; vz = radar.zpos;
	    transform_point( img_fildes, WORLD_TO_MC, vx, vy, vz, 
		&vx, &vy, &vz );
	    vx = x - vx; vy = y - vy; vz = z - vz;
	    dist = sqrt( (double)(vx*vx + vy*vy + vz*vz) );
	    vx *= cstate.view_direction[0];
	    vy *= cstate.view_direction[1];
	    vz *= cstate.view_direction[2];
	    if((dist >= 500.0) && (dist <= 10000.0) && ((vx + vy + vz) >= 0.0))
	    {
	      /* They are not behind us and are an appropriate distance away */
		transform_point( img_fildes, MC_TO_VDC, x, y, z, &vx, &vy, &vz);
		vdc_to_dc( img_fildes, vx, vy, vz, &dcx, &dcy, &dcz );
		fill_color( img_fildes, red, green, blue );
		dcrectangle( img_fildes, dcx-2, dcy-2, dcx+2,dcy+2 );
		text_color( img_fildes, red,green,blue);
		sprintf( buff, "%d", (int)(0.5 + dist / 100.0) );
		dctext( img_fildes, dcx+6, dcy+8, buff );
	    }
	}
    }

    radar.num_cars = SRV_FRAME_RADAR_NUM(rmsg->MsgLength);

    /* Update radar. */
    radarUpdate(&radar);

#endif /* ] */
#endif /* ] */
}

/*****************************************************************
 * frame_static_seg
 * 
 * 	Draw a Scene's Mongo (Static) Display list Segment
 * 	
 * Inputs:
 *		m	Message.
 * Outputs:
 *		FALSE 	to continue parsing.
 */
void frame_static_seg( struct IPCMsg *Msg)
{
#ifdef GLFW
    if (dashPickerActive()) return;
#endif
#if !defined(HOVERWARE_MODEL)
    static int all_invis_bits = ALL_INVISIBILITY_BITS;
#endif
    struct SrvFrameStaticSeg *staticseg;
    extern hwDisplay disp;

    staticseg = (struct SrvFrameStaticSeg *)Msg;

    PRINT_ROUTINE_NAME( frame_static_seg );
#if 1
    disp->setInvisibility( disp, ALL_INVISIBILITY_BITS,
				cstate.global_nameset_bits );
    draw_hw_seg( NULL, staticseg->segment );
#else
    set_invisibility_filter(img_fildes,1,&all_invis_bits,
	1,(int *) &(cstate.global_nameset_bits));
    refresh_segment(img_fildes, staticseg->segment);
#endif
}


/*****************************************************************
 * frame_update
 * 
 * 	Get display update info from server
 * 	
 * Inputs:
 *		m	Message.
 * Outputs:
 *		FALSE 	to continue parsing.
 */
void frame_update( struct IPCMsg *Msg)
{
    struct CliReady *cmsg;
    UpdateDisp *mmsg;
    static int all_invis_bits = ALL_INVISIBILITY_BITS;
    extern hwDisplay disp;

    /* Point at matrix message in receive buffer and increment
     * receive buffer pointer.
     */
    cmsg = (struct CliReady *)Msg;
    mmsg = &(cmsg->upd);

    PRINT_ROUTINE_NAME( frame_update );

    /* Get the name of this dynamic object */
    if( mmsg->name[0] != '\0') 
	memcpy(last_dynamic_name,mmsg->name,10);
    else
	last_dynamic_name[0] = '\0';

    /* Update display list segment matrix using the matrix sent
     * from the server.
     */
    {
	static hwSurfaceType surf;
	surf.color[0] = mmsg->color[0];
	surf.color[1] = mmsg->color[1];
	surf.color[2] = mmsg->color[2];
	disp->surfAttrs( disp, &surf );
    }

    if (mmsg->invis_words > 0) {
	mmsg->invis[0] |= cstate.global_nameset_bits;
	disp->setInvisibility( disp, all_invis_bits, mmsg->invis[0] );
    }
    else {
	disp->setInvisibility( disp,
			all_invis_bits, cstate.global_nameset_bits );
    }
}


/*****************************************************************
 * frame_global_nameset
 * 
 * 	Get a global nameset bits from the server.
 * 	
 * Inputs:
 *		m	Message.
 * Outputs:
 *		FALSE 	to continue parsing.
 */
void frame_global_nameset( struct IPCMsg *Msg)
{
    struct SrvFrameGlobalNameset *gnmsg = (struct SrvFrameGlobalNameset *)Msg;

    PRINT_ROUTINE_NAME( frame_global_nameset );

    cstate.global_nameset_bits = gnmsg->global_nameset_bits;
}

