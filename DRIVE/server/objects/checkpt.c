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


/* Code Module for the checkpt segment object */

#include <stdio.h>
#include <sys/types.h>
#ifndef WIN32
#include <time.h>
#include <sys/time.h>
#else
// TBD!
#endif
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"
#include "message.h"
#include "connection.h"
#include "drive_server.h"

#define Length	(obj->size[SIZE_LENGTH])
#define Height	(obj->size[SIZE_HEIGHT])
#define Width	(obj->size[SIZE_WIDTH])
#define Radius	(obj->radius)
#define Label   (obj->label)
#define Subtype (obj->subtype)

#define DEFAULT_CHECKPT_LENGTH		(60.0)
#define DEFAULT_CHECKPT_HEIGHT		(40.0)
#define DEFAULT_CHECKPT_WIDTH		(100.0)
#define DEFAULT_CHECKPT_RADIUS		(0.5)
#define POLE_CULL_SIZE                  (50.0)

extern OBJECT_ID object_id[];
extern int turbo_boosts;

typedef struct _checkpt_list {
    float length, height, width, radius;
    unsigned int nameset_bits;
    char label[LABEL_STRLEN];
    char subtype[LABEL_STRLEN];
    int dl_number;
    struct _checkpt_list *next;
} CHECKPT_LIST;
static CHECKPT_LIST *checkpt_list=NULL;


static int pole_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    if (x*x + z*z < Radius*Radius) {
	sc->mc_y = Height;
	get_pole_mc_normal(obj,x,y,z,sc->mc_normal);
	return(TRUE);
    }
    /* else */
    return(FALSE);
}

static int pole_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = Height;
    return(TRUE);
}

static int checkpt_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = 0.0;
    return(TRUE);
}

static int checkpt_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = 0.0;
    return(TRUE);
}


static hwObject single_pole_high(
    DRIVE_OBJECT *obj)
{
    hwObject
	res;

    res = hwCone->create( hwCone );
    HW_MODIFY_1F( res, hwStrRadius, Radius );
    HW_MODIFY_1F( res, hwStrHeight, Height );
    HW_MODIFY_1I( res, hwStrGraphN, 2 );
    HW_MODIFY_1I( res, hwStrGraphM, 7 );
    HW_MODIFY_3F( res, hwStrRotate, 90.0, 0.0, 0.0 );
    WOOD_HW(res);
    return res;
}


static hwObject single_pole_low(
    DRIVE_OBJECT *obj)
{
    hwObject
	res;
    res = post(0.0,0.0,0.0,Height,Radius,FALSE,4);
    WOOD_HW(res);
    return res;
}

/* the following three defines are taken from obj_list.c  Make sure that they
 * do not change!
 */

#define START_OBJECT		100
#define FINISH_OBJECT		101
#define CHECKPOINT_OBJECT	102

static hwObject do_banner_graphics(
    DRIVE_OBJECT *obj)
{
    float wid = Width/2.0;
    float len = Length/2.0;
    float pgon[8 * 3];
    float *ptr;
    float char_width, char_height;
    hwObject curr, oList[16];
    int nObjs = 0;


    ptr = pgon;
    *ptr++ =  wid; *ptr++ = Height-Length; *ptr++ = 0.0;
    *ptr++ =  wid; *ptr++ = Height; *ptr++ = 0.0;
    *ptr++ = -wid; *ptr++ = Height; *ptr++ = 0.0;
    *ptr++ = -wid; *ptr++ = Height-Length; *ptr++ = 0.0;

    curr = hwPolygon->create( hwPolygon );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    curr->modify( curr, hwStrData, HW_MAKE_TYPE(HW_TYPE_FLOAT,12), pgon );

    switch(obj->idptr->number)
    {
	case START_OBJECT:
	    HW_MODIFY_3F( curr, hwStrColor, 1.0, 1.0, 1.0 );
	    break;
	case FINISH_OBJECT:
	    HW_MODIFY_3F( curr, hwStrColor, 0.0, 0.0, 0.0 );
	    break;
	case CHECKPOINT_OBJECT:
	    HW_MODIFY_3F( curr, hwStrColor, 1.0, 1.0, 0.0 );
	    break;
    }
    oList[nObjs++] = curr;


    /* Now, draw the back side of the sign */
    ptr = pgon;
    *ptr++ = -wid; *ptr++ = Height-Length; *ptr++ = 0.0;
    *ptr++ = -wid; *ptr++ = Height; *ptr++ = 0.0;
    *ptr++ =  wid; *ptr++ = Height; *ptr++ = 0.0;
    *ptr++ =  wid; *ptr++ = Height-Length; *ptr++ = 0.0;

    curr = hwPolygon->create( hwPolygon );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    curr->modify( curr, hwStrData, HW_MAKE_TYPE(HW_TYPE_FLOAT,12), pgon );
    HW_MODIFY_3F( curr, hwStrColor, 0.5, 0.5, 0.5 );
    oList[nObjs++] = curr;

    char_width = (Width / (strlen(Label) +1))  / 0.7;
    char_height = len;

    curr = draw_string(Label, 0.0, Height - char_height*1.5,  -0.3,
       char_width,char_height,TA_CENTER_SB,PATH_RIGHT,0.1);
    if( curr ) {
	switch(obj->idptr->number)
	{
	    case START_OBJECT:
		HW_MODIFY_3F( curr, hwStrColor, 0.0, 0.0, 0.0 );
		break;
	    case FINISH_OBJECT:
		HW_MODIFY_3F( curr, hwStrColor, 1.0, 1.0, 1.0 );
		break;
	    case CHECKPOINT_OBJECT:
		HW_MODIFY_3F( curr, hwStrColor, 0.0, 0.0, 0.0 );
		break;
	}
	oList[nObjs++] = curr;
    }

    curr = hwGroup->create( hwGroup );
    curr->modify( curr, hwStrChildren,
		HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
    return curr;
}


static void create_checkpt_graphics(
    DRIVE_OBJECT *obj)
{
    hwObject curr, oList[100];
    float lods[100][2];
    int nObjs = 0;


    curr = single_pole_low( obj );
    HW_MODIFY_3F( curr, hwStrPos, (-Width/2.0 - Radius), 0.0, 0.0 );
    oList[nObjs] = curr;
    lods[nObjs][0] = 0.0;
    lods[nObjs][1] = 10.0;
    nObjs++;

    curr = single_pole_low( obj );
    HW_MODIFY_3F( curr, hwStrPos, (Width/2.0 + Radius), 0.0, 0.0 );
    oList[nObjs] = curr;
    lods[nObjs][0] = 0.0;
    lods[nObjs][1] = 10.0;
    nObjs++;

    curr = single_pole_high( obj );
    HW_MODIFY_3F( curr, hwStrPos, (-Width/2.0 - Radius), 0.0, 0.0 );
    oList[nObjs] = curr;
    lods[nObjs][0] = 10.0;
    lods[nObjs][1] = 100.0;
    nObjs++;

    curr = single_pole_high( obj );
    HW_MODIFY_3F( curr, hwStrPos, (Width/2.0 + Radius), 0.0, 0.0 );
    oList[nObjs] = curr;
    lods[nObjs][0] = 10.0;
    lods[nObjs][1] = 100.0;
    nObjs++;

    curr = do_banner_graphics(obj);
    oList[nObjs] = curr;
    lods[nObjs][0] = 0.0;
    lods[nObjs][1] = 100.0;
    nObjs++;

    curr = hwGroup->create( hwGroup );
    curr->modify( curr, hwStrChildren,
		HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
    curr->modify( curr, hwStrLOD,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,2*nObjs), lods );

    obj->display_list = createHwSegmentFromObj( &curr, 1 );
}


void init_checkpt_object(
    DRIVE_OBJECT *obj)
{
    CHECKPT_LIST *ll;
    float x;
    int i;
    DRIVE_OBJECT *child;
    static float mat[4][4] = {
	{ 1.0,	0.0,	0.0,	0.0 },
	{ 0.0,	1.0,	0.0,	0.0 },
	{ 0.0,	0.0,	1.0,	0.0 },
	{ 0.0,	0.0,	0.0,	1.0 }
    };

    if (Length <= 0.0)  Length  = DEFAULT_CHECKPT_LENGTH;
    if (Width <= 0.0)   Width   = DEFAULT_CHECKPT_WIDTH;
    if (Height <= 0.0)  Height  = DEFAULT_CHECKPT_HEIGHT;
    if (Radius <= 0.0)  Radius  = DEFAULT_CHECKPT_RADIUS;

    if(debug) printf(" inside init_checkpt_%d_object() routine \n",(int) Height);

    /* Two pole objects and a special banner object */
    obj->num_children = 3;

    /* See if we've created one like this before... */
    ll = checkpt_list;
    while (ll != NULL)
    {
	if (IS_NEAR(ll->height,Height)
		&& IS_NEAR(ll->radius,Radius)
		&& IS_NEAR(ll->width,Width)
		&& (strcmp(ll->label, Label) == 0)
		&& (ll->nameset_bits == obj->nameset_bits)
		&& IS_NEAR(ll->length,Length))
	    break;
	ll = ll->next;
    }

    if (ll != NULL) 
    {
	/* Good -- I have one like this already. */
	obj->display_list = ll->dl_number;
    }
    else
    {
	/* Nope -- gotta create a new one. */
	if ((ll = (CHECKPT_LIST *) malloc(sizeof(CHECKPT_LIST))) == NULL)
	{
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}

	ll->length  = Length;
	ll->width   = Width;
	ll->height  = Height;
	ll->radius  = Radius;
	ll->nameset_bits = obj->nameset_bits;
	strcpy(ll->label,Label);
	ll->next    = checkpt_list;

    	create_checkpt_graphics(obj);

	ll->dl_number = obj->display_list;
	checkpt_list  = ll;
    }


    /* Initial (mc) bounding box values */
    obj->bound_mc[0] = -Radius;
    obj->bound_mc[1] = 0.0;
    obj->bound_mc[2] = -Radius;
    obj->bound_mc[3] = Radius;
    obj->bound_mc[4] = Height;
    obj->bound_mc[5] = Radius + Length;

    obj->bound_mc[0] = -Width/2.0 - Radius;
    obj->bound_mc[1] = 0.0;
    obj->bound_mc[2] = -Radius;
    obj->bound_mc[3] = Width/2.0 + Radius;
    obj->bound_mc[4] = Height;
    obj->bound_mc[5] = Radius;
    update_wc_bounds(obj);

    /* The parent object should not define these fields -- let the
     * child objects (poles) define them.
    obj->surface_chars_xyz  = pole_surface_chars_xyz;
    obj->surface_chars_bbox = pole_surface_chars_bbox;
    */

    /* Now create subobjects for each pole */
    for (i=0,x=(-Width/2.0 - Radius); i < (obj->num_children -1); 
	     ++i,x+=(Width + (2* Radius)))
    {
	if ((child = (DRIVE_OBJECT *) malloc(sizeof(DRIVE_OBJECT))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    break;
	}
	/* as a good starting point, clone myself */
	memcpy(child,obj,sizeof(DRIVE_OBJECT));
	/* now change relevant parts */
	child->idptr = &(object_id[12]); /* let's say its a pole */
	child->num_children = 0;
	child->child_list = NULL;
	child->display_list = INVALID;
	mat[3][0] = x;
	concat_matrix(mat,obj->xform,child->xform);
	_hp_invert(child->xform,child->ixform,0);
	child->bound_mc[0] = -Radius;
	child->bound_mc[1] = 0.0;
	child->bound_mc[2] = -Radius;
	child->bound_mc[3] = Radius;
	child->bound_mc[4] = Height;
	child->bound_mc[5] = Radius;
	update_wc_bounds(child);
	child->surface_chars_xyz  = pole_surface_chars_xyz;
	child->surface_chars_bbox = pole_surface_chars_bbox;
	add_object_to_list(&(obj->child_list),child);
    }

    /* Finally, create the banner subobject */

    if ((child = (DRIVE_OBJECT *) malloc(sizeof(DRIVE_OBJECT))) == NULL) {
	fprintf(stderr,"Out of malloc space!\n");
	exit(1);
    }
    /* as a good starting point, clone myself */
    memcpy(child,obj,sizeof(DRIVE_OBJECT));
    /* now change relevant parts */

    child->surface_chars_xyz  = checkpt_surface_chars_xyz;
    child->surface_chars_bbox = checkpt_surface_chars_bbox;
    child->num_children = 0;
    child->child_list = NULL;
    child->display_list = INVALID;

    mat[3][0] = 0.0;
    concat_matrix(mat,obj->xform,child->xform);
    _hp_invert(child->xform,child->ixform,0);
    child->bound_mc[0] = -Width/2.0;
    child->bound_mc[1] = 0.0;
    child->bound_mc[2] = -Radius;
    child->bound_mc[3] = Width/2.0;
    child->bound_mc[4] = Height;
    child->bound_mc[5] = Radius;
    update_wc_bounds(child);
    add_object_to_list(&(obj->child_list),child);

    elevate_object_to_terrain_height((SCENE *) obj->scene,obj,FALSE);
}

#define CHECKPOINT_BIT(which)	(1 << ((which)-1))
#define ALL_PREVIOUS_CHECKPOINTS_HIT(ckpts,which) \
    (((ckpts) & (CHECKPOINT_BIT(which)-1)) == (CHECKPOINT_BIT(which)-1))

void intersect_checkpoint(
    DRIVE_OBJECT *vehicle,
    DRIVE_OBJECT *chk)
{
#ifndef WIN32 // TBD!
    /* If we get into this routine, it means that the vehicle object has
       crossed a checkpoint object.  Depending on what sort of checkpoint
       it is, we will do the following:
        A) Starting Line
            1) Send a "Start Race" message to the client.
	    2) Zero out the car's checkpoints field
	    3) calculate the starting time (seconds and microseconds)
	     
	B) Checkpoint (midway)
	    1) calculate time to checkpoint from starting line
	    2) send client a "checkpoint" message, including time to checkpt.
	    3) "OR" in the bit for this checkpoint into the car's checkpt field.

	c) Finish Line
	    1) calculate time from starting line to finish
	    2) ensure that all appropriate intermediate checkpoints were passed
	    3) Send message to client, indicating lap time and checkpoint 
	       hits/misses.
    */

    struct SrvCheckpoint chk_msg;
    struct timeval _time;
    struct timezone tz;
    int sec,usec;
    int begin,end,this;
    connection_type *con;
    boolean_type end_race = FALSE;


    gettimeofday(&_time,&tz);
    chk_msg.checkpoint = 0;

    switch(chk->idptr->number)
    {
	case START_OBJECT:
	    if (server_mode & SERVER_MODE_STARTLINE_START) {
		vehicle->turbo_boosts = 0;    /* reset turbo boosts */
		vehicle->start_lapsec = _time.tv_sec;
		vehicle->start_lapusec = _time.tv_usec;
	    }
	    if (server_mode & SERVER_MODE_CHKPT_RELOAD) {
		int reset;

		if( chk->dimension == 3 )
		    reset = (int)chk->data[2];  /* reset or accumlate values */
		else
		    reset = 1; /* by default, reset the values */ 

		if( chk->dimension == 0) /* no data -- do nothing */
		{
		}
		if( chk->dimension > 0) /* Data for turbos exists */
		{
		    if( reset)
			vehicle->turbo_boosts = current_course->turbo_boosts - 
			    (int)chk->data[0];
		    else
			vehicle->turbo_boosts -= (int)chk->data[0];
		}
		if( chk->dimension > 1) /* Data for ammo exists */
		{
		    if( reset)
			vehicle->ammo_used = current_course->ammo - 
			    (int)chk->data[1];
		    else
			vehicle->ammo_used -= (int)chk->data[1];
		}
	    }
	    vehicle->checkpoints = 0;
	    if (vehicle->start_lapusec > _time.tv_usec) {
		_time.tv_usec += 1000000;
		_time.tv_sec-- ;
	    }
	    sec =  _time.tv_sec -  vehicle->start_lapsec;
	    usec = _time.tv_usec - vehicle->start_lapusec;
	    vehicle->time_to_checkpt = sec + (usec / 1000000.0);
	    /* Now, we need to figure out from the subtype field which
	     * checkpoints apply to this starting line.  The subtype field
	     * should have two integers ( and nothing else) representing the
	     * start and ending numbers of the checkpoints it cares about.
	     */

	    if( sscanf(chk->subtype,"%d %d",&begin,&end) != 2)
	    {
		begin = 0;
		end = 0;
	    }
	    chk_msg.num_checkpoints = end - begin + 1;
	    chk_msg.time = vehicle->time_to_checkpt;
	    chk_msg.visited = vehicle->checkpoints;
	    chk_msg.MsgType = SRV_START_LAP;
	    break;

	case CHECKPOINT_OBJECT:
	    if( sscanf(chk->subtype,"%d %d",&begin,&this) != 2)
	    {
		this = 0;
		chk_msg.checkpoint = 0;
	    }
	    /* Make sure they've hit the previous checkpoints */
	    else if (!ALL_PREVIOUS_CHECKPOINTS_HIT(vehicle->checkpoints,this)) {
		/* Ignore this checkpoint hit. */
		return;
	    }
	    else
	    {
		vehicle->checkpoints |= CHECKPOINT_BIT(this);
		chk_msg.checkpoint = this - begin + 1;
	    }

	    if( vehicle->start_lapusec > _time.tv_usec)
	    {
		_time.tv_usec += 1000000;
		_time.tv_sec-- ;
	    }
	    sec =  _time.tv_sec -  vehicle->start_lapsec;
	    usec = _time.tv_usec - vehicle->start_lapusec;
	    chk_msg.time = sec + (usec / 1000000.0);
	    vehicle->time_to_checkpt = chk_msg.time;
	    chk_msg.visited = vehicle->checkpoints;
	    chk_msg.MsgType = SRV_CHECKPOINT;
	    update_leader_board(STANDINGS_CURRENT);

	    if (server_mode & SERVER_MODE_CHKPT_RELOAD) {
		int reset;

		if( chk->dimension == 3 )
		    reset = (int)chk->data[2];  /* reset or accumlate values */
		else
		    reset = 1; /* by default, reset the values */ 

		if( chk->dimension == 0) /* no data -- do nothing*/
		{
		}
		if( chk->dimension > 0) /* Data for turbos exists */
		{
		    if( reset)
			vehicle->turbo_boosts = current_course->turbo_boosts - 
			    (int)chk->data[0];
		    else
			vehicle->turbo_boosts -= (int)chk->data[0];
		}
		if( chk->dimension > 1) /* Data for ammo exists */
		{
		    if( reset)
			vehicle->ammo_used = current_course->ammo - 
			    (int)chk->data[1];
		    else
			vehicle->ammo_used -= (int)chk->data[1];
		}
	    }
	    break;

	case FINISH_OBJECT:
	    if( chk->start_lapusec > _time.tv_usec)
	    {
		_time.tv_usec += 1000000;
		_time.tv_sec-- ;
	    }
	    sec =  _time.tv_sec -  vehicle->start_lapsec;
	    usec = _time.tv_usec - vehicle->start_lapusec;
	    chk_msg.time = sec + (usec / 1000000.0);
	    vehicle->time_to_checkpt = chk_msg.time;

	    if( sscanf(chk->subtype,"%d %d",&begin,&end) != 2)
	    {
		begin = 0;
		end = 0;
		chk_msg.visited = 0;

	    }
	    /* Make sure they've hit the previous checkpoints */
	    else if (!ALL_PREVIOUS_CHECKPOINTS_HIT(vehicle->checkpoints,end)) {
		/* Ignore this checkpoint hit. */
		return;
	    }
	    else
	    {
		/* Treat the Finishline as a Checkpoint */
		vehicle->checkpoints    |= CHECKPOINT_BIT(end);
		chk_msg.checkpoint = end - begin + 1;
	        chk_msg.checkpoint = end - begin + 1;
	        chk_msg.visited = vehicle->checkpoints;
	    }
	    chk_msg.MsgType = SRV_FINISH_LAP;

	    /* If race is supposed to end when first driver crosses the finish
	     * line, do so now.
	     */
	    if ((server_mode & SERVER_MODE_FINISHLINE_FINISH)
		    && (server_state == RACE_STATE)) {
		end_race = TRUE;
	    }
	    else {
		update_leader_board(STANDINGS_CURRENT);
	    }
	    break;
    }

    chk_msg.MsgLength = SRV_CHECKPOINT_SIZE;
    con = (connection_type *) (vehicle->connection);

    /* Unmanned scene vehicles have no connection -- nothing to report to. */
    if (con && con->socket) {
	PutMsg( (struct IPCMsg *)&chk_msg, con->socket->socketnum);
    }

    if (end_race) {
	next_server_state();
    }
#endif
}
