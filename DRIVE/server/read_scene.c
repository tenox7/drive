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
#include <time.h>
#include <string.h>

#ifndef WIN32
#include <sys/time.h>
#include <unistd.h>
#endif	

#ifdef WIN32
#include <sys/timeb.h>
#endif

#include "object.h"
#include "global.h"
#include "drive_server.h"
#include "demo_physics.h"

#define MAX_OBJECTS 100

DRIVE_OBJECT *new_object;

FILE *fp, *fopen();

extern int cur_vehicle_types;
extern Driveable driveables[];


int cur_seg = 0;  /* current segment for display list creation */


OBJECT_ID *find_object(
    int objnum,
    char *objname)
{
    OBJECT_ID *oi;
    char lcname[1024],*cptr1,*cptr2;

    if (objnum != INVALID) {
	/* Make sure that number is valid. */
	for (oi=object_id; oi->number != INVALID; ++oi) {
	    if (oi->number == objnum) break;
	}
	if (oi->number == INVALID) objnum = INVALID;
    }

    if (objnum == INVALID) {
	if ((objname != NULL) && (*objname != '\0')) {
	    /* Search by name */
	    /* Change to lower case */
	    cptr1 = objname;
	    cptr2 = lcname;
	    while (*cptr1) {
		if ((*cptr1 >= 'A') && (*cptr1 <= 'Z')) {
		    *cptr2++ = (char) ((int) (*cptr1++) - 'A' + 'a');
		}
		else {
		    *cptr2++ = *cptr1++;
		}
	    }
	    *cptr2++ = '\0';

	    /* Try to pick up the objnum by the name */
	    for (oi=object_id; oi->number != INVALID; ++oi) {
		if (strcmp(oi->lcname,lcname) == 0) break;
	    }
	    if (oi->number == INVALID) {
		fprintf(stderr, "Bad object name or number: %s %d\n",
		    objname,objnum);
	    }
	}
	else {
	    fprintf(stderr, "Bad object name or number: %s %d\n",
		objname,objnum);
	    /* Bad one: return the invalid oi */
	    for (oi=object_id; oi->number != INVALID; ++oi) {
	    }
	}
    }

    return(oi);
}


void add_object_to_list(
    DRIVE_OBJECT *object_list[],
    DRIVE_OBJECT *object)
{
    /* Add it to the head of the doubly-linked list */
    object->previous = NULL;
    object->next     = *object_list;
    if (*object_list != NULL) {
	(*object_list)->previous = object;
    }
    *object_list = object;
}



static void update_driveables(
    DRIVE_OBJECT *new_obj)
{
    int i,dl,unique;
    Driveable *d;
    VEHICLE_AUXDATA *vaux;

    /* First, we need to be sure that the object is not already in 
     * the list.  Since each vehicle has a unique display list number,
     * check to make sure that the number is NOT in the list.
     */
    unique = TRUE;
    if ((dl = new_obj->display_list) == INVALID)
	dl = new_obj->global_display_list;
    for (i=0; i<cur_vehicle_types; ++i) { 
	if (dl == driveables[i].display_list) {
	    unique = FALSE;
	    break;
	}
    }

    if (unique) {
	d = driveables + (cur_vehicle_types++);
	d->object_number = new_obj->idptr->number;
	d->display_list = dl;
	strcpy(d->name, new_obj->idptr->name);
	vaux = new_obj->vehicle_auxdata;
	d->horsepower        = vaux->horsepower;
	d->left_gauge_class  = vaux->left_gauge_class;
	d->right_gauge_class = vaux->right_gauge_class;
	d->max_speed         = vaux->max_speed;
	d->max_rpm           = vaux->max_rpm;
	d->max_altitude      = vaux->max_altitude;
    }
}


void complete_object(
    DRIVE_OBJECT *object_list[],
    DRIVE_OBJECT *new_obj)
{
    int i;

    new_obj->upd = NULL;
    new_obj->update_controls = NULL;
    new_obj->update_self =  NULL;
    new_obj->surface_chars_xyz = NULL;
    new_obj->surface_chars_bbox =  NULL;
    new_obj->collision_routine = do_collision;
    new_obj->child_list =  NULL;
    new_obj->pobj =  NULL;
    new_obj->vehicle_auxdata = NULL;
    new_obj->aeroplane = NULL;
    new_obj->lights_on = FALSE;
    new_obj->global_display_list = INVALID;

    new_obj->turbo_boosts = 0;
    new_obj->ammo_used = 0;
    new_obj->time_to_checkpt = 0.0;
    new_obj->start_lapsec = 0;
    new_obj->start_lapusec = 0;
    new_obj->checkpoints = 0;

    /* default bounding box...all zeros */
    for(i=0; i<6; i++) {
	new_obj->bound_mc[i] = 0.0;
	new_obj->bound_wc[i] = 0.0;
    }

    _hp_invert(new_obj->xform,new_obj->ixform,0);
    if (object_list != NULL) add_object_to_list(object_list,new_obj);

    (*(new_obj->idptr->init_routine))(new_obj);

    if (new_obj->upd != NULL) {
	new_obj->upd->name[0] = '\0';
    }


    if (new_obj->idptr->flags & OBJECTCLASS_DRIVEABLE) {
	if ((new_obj->idptr->flags & VEHICLE_MASK) & allowable_vehicles) {
	    /* This object can be driven.... */
#ifdef WIN32
            struct _timeb _time;

	    /* Set the start time correctly */
            _ftime(&_time);
	    new_obj->start_lapsec  = _time.time;
	    new_obj->start_lapusec = _time.millitm * 1000;
#else
	    struct timeval _time;
	    struct timezone _tz;

	    /* Set the start time correctly */
	    gettimeofday(&_time,&_tz);
	    new_obj->start_lapsec  = _time.tv_sec;
	    new_obj->start_lapusec = _time.tv_usec;

#endif
	    update_driveables(new_obj);
	}
    }
}


/*****************************************************************************
 *        get_dl_segment()
 *
 *  This routine returns the next display list segment available for use.
 *  Any time *any* display list segment is opened, this routine should be
 *  called to obtain the segment to open  (thus to avoid collisions).
 ****************************************************************************
 */

int get_dl_segment(
    void)
{
   return INVALID; /*(cur_seg++);*/

}


/*****************************************************************************
 *      initialize table of object init routines to null
 *****************************************************************************
 */

void object_initialization(
    void)
{

    init_physics_state();
}
