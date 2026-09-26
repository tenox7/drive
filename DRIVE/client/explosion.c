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
 * explosion.c - Routines for drawing special effects.
 */


#include "global.h"
#include "object.h"
#include "obj_common.h"
#include "prims.h"
#include "drive.h"
#if defined(HAVE_CONFIG_H)
# include "config.h"
#endif

#define EXP_TRIANGLES	20
#define EXP_FRAMES	20
#define EXP_MIN_RADIUS	2.0
#define EXP_POLYSIZE	1.5

/* Explosion type definitions. */
typedef struct explosion_struct explosion_type;

/* Explosion structure definition. */
struct explosion_struct
{
    float x, y, z;
    float r, g, b;
    float radius;

    int duration;
    int frame_count;

    explosion_type *prev;
    explosion_type *next;
};

/* List of explosions currently in progress. */
static explosion_type * explosion_list = (explosion_type *)NULL;

/*****************************************************************
 * explosion_create
 *
 * 	Begins an explosion of radius r at x,y,z.
 */
void explosion_create(
    float x, float y, float z,
    float r, float g, float b,
    float radius)
{
    explosion_type *newexp;

    newexp = (explosion_type *)malloc( sizeof( explosion_type ) );
    if ( newexp == NULL ) {
	fprintf(stderr,"explosion_create: Out of memory.\n");
	return;
    }

    newexp->x = x;
    newexp->y = y;
    newexp->z = z;

    newexp->r = r;
    newexp->g = g;
    newexp->b = b;

    newexp->radius = radius;

    /* Initialize frame count. */
    newexp->duration = EXP_FRAMES;
    newexp->frame_count = EXP_FRAMES;

    /* Add explosion to explosion list. */
    newexp->prev = (explosion_type *)NULL;
    newexp->next = explosion_list;
    if ( explosion_list )
	explosion_list->prev = newexp;
    explosion_list = newexp;
}


/*****************************************************************
 * explosion_draw
 *
 * 	Draws an explosion with current parameters.
 */
void explosion_draw(
    explosion_type *currexp)
{
    float clist[3*6];
    float current_radius;
    float *floatptr;
    float xdir, ydir, zdir;
    float xpos, ypos, zpos;
    int triangle, vertex;
    hwSurfaceType surf;
    extern hwDisplay disp;
    static hwObject sphere;

    surf.color[0] = 1.0;
    surf.color[1] = 1.0;
    surf.color[2] = 1.0;
    surf.transp = 0.0;
    surf.specColor[0] = 1.0;
    surf.specColor[1] = 1.0;
    surf.specColor[2] = 1.0;
    surf.shininess = 0.0;
    surf.visibility = 0xFFFFFFFF;
    surf.flags = HW_SURF_EMISSIVE | HW_SURF_TWOSIDED;
    surf.internalFlags = 0;
    surf.numTextures = 0;
    disp->surfAttrs( disp, &surf );

    /* Calculate how big the explosion should be at this frame. */
    current_radius = EXP_MIN_RADIUS
	+ (currexp->duration - currexp->frame_count)
	* ((currexp->radius - EXP_MIN_RADIUS) / currexp->duration);

    for ( triangle=0; triangle<EXP_TRIANGLES; triangle++ )
    {
	/* Find a random direction vector. */
	xdir = EITHERSIGN_FLOATRAND( -1.0, 1.0 );
	/* mostly on the top half of the sphere */
	if (FLOATRAND(1.0) < 0.8)
	    ydir = BOUNDED_FLOATRAND( 0.0, 1.0 );
	else
	    ydir = BOUNDED_FLOATRAND( -1.0, 0.0 );
	zdir = EITHERSIGN_FLOATRAND( -1.0, 1.0 );
	NORMALIZE3( xdir, ydir, zdir );

	/* Find location of center of triangle. */
	xpos = currexp->x + current_radius * xdir;
	ypos = currexp->y + current_radius * ydir;
	zpos = currexp->z + current_radius * zdir;

	floatptr = clist;
	for ( vertex=0; vertex<3; vertex++ )
	{
	    /* Find a random direction vector. */
	    xdir = EITHERSIGN_FLOATRAND( -1.0, 1.0 );
	    ydir = EITHERSIGN_FLOATRAND( -1.0, 1.0 );
	    zdir = EITHERSIGN_FLOATRAND( -1.0, 1.0 );
	    NORMALIZE3( xdir, ydir, zdir );

	    /* Vertex position. */
	    *floatptr++ = xpos + EXP_POLYSIZE * xdir;
	    *floatptr++ = ypos + EXP_POLYSIZE * ydir;
	    *floatptr++ = zpos + EXP_POLYSIZE * zdir;

	    /* Vertex color. */
	    switch ( vertex )
	    {
	      case 0:
		*floatptr++ = currexp->r;
		*floatptr++ = currexp->g;
		*floatptr++ = currexp->b;
		break;

	      case 1:
		*floatptr++ = 1.0;
		*floatptr++ = 0.0;
		*floatptr++ = 0.0;
		break;

	      case 2:
		*floatptr++ = 1.0;
		*floatptr++ = 1.0;
		*floatptr++ = 0.0;
		break;
	    }
	}
	disp->drawPolygon( disp, clist, HW_DATA_RGB, 3 );
    }

    if( !sphere ) {
	sphere = hwSphere->create( hwSphere );
	HW_MODIFY_1I( sphere, hwStrGraphN, 5 );
	HW_MODIFY_1I( sphere, hwStrGraphM, 7 );
	HW_MODIFY_1I( sphere, hwStrOptFlags, 0 );
	HW_MODIFY_3F( sphere, hwStrColor, 1.0, 0.8, 0.0 );
	HW_MODIFY_1F( sphere, hwStrTransparency, 0.5 );
    }
    HW_MODIFY_1F( sphere, hwStrRadius, current_radius );
    HW_MODIFY_3F( sphere, hwStrPos, currexp->x, currexp->y, currexp->z );
    sphere->draw( sphere );
}


/*****************************************************************
 * explosion_update
 *
 * 	Updates current explosions.
 */
void explosion_update( int fildes )
{
    explosion_type *currexp,*oldexp;

    /* Loop through current explosion list and update each. */
    currexp = explosion_list;
    while ( currexp )
    {
	if ( currexp->frame_count > 0 )
	{
	    /* Explosion is in progress, draw in new position. */
	    explosion_draw( currexp );

	    /* Decrement frame count. */
	    currexp->frame_count--;
	    currexp = currexp->next;
	}
	else
	{
	    /* Explosion has finished, remove from the list. */
	    if ( currexp->prev )  
	    {
		/* Not first in list, point prev to next. */
		currexp->prev->next = currexp->next;

		if ( currexp->next ) 
		    /* Not last in list, point next to prev. */
		    currexp->next->prev = currexp->prev;	
	    }
	    else
	    {
		/* First in list.  Point list head to next. */
		explosion_list = currexp->next;

		if ( currexp->next )
		    /* Not last in list, set next prev to NULL. */
		    currexp->next->prev = NULL;
	    }
	    oldexp = currexp;
	    currexp = currexp->next;
	    free((void *) oldexp);
	}
    }
}
