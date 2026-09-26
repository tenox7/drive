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
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "global.h"
#include "object.h"
#include "scene.h"
#include "connection.h"
#include "message.h"
#include "drive_server.h"

#define REREAD_CHECK_INTERVAL	1.0	/* seconds */
#define REREAD_UPDATE_COUNT \
	((int) (((float) REREAD_CHECK_INTERVAL)/((float) T_INTERVAL)))

SCENE *find_scene(
    int x, int z)
{
    SCENE *sceneptr;

    sceneptr = scene_head;
    while (sceneptr != NULL) {
	if ((sceneptr->xscene == x)
		&& (sceneptr->zscene == z)) {
	    return(sceneptr);
	}
	sceneptr = sceneptr->next;
    }

    return(NULL);
}


void delete_object_from_scene(
    SCENE *scene,
    DRIVE_OBJECT *object)
{
    DRIVE_OBJECT *obj;

    if( scene == NULL ) return;

    obj = scene->object_head;
    if (obj == object) {
	if (obj->next == NULL ) {
	    /* only object in scene!  can't happen? */
	    scene->object_head = NULL;
	}
	else {
	    obj->next->previous = NULL;
	    scene->object_head = obj->next;
	}
    }
    else {
	while ((obj != object) && (obj)) obj = obj->next;
	if (obj != NULL) {
	    obj->previous->next = obj->next;
	    if (obj->next != NULL) obj->next->previous = obj->previous;
	}
	else {
	    fprintf(stderr,"Cannot find object in scene to delete!\n");
	}
    }
}


int add_scene_to_list(
    SCENE *scene)
{
    SCENE *sceneptr;

    /* First check to make sure this scene doesn't already exist. */
    if ((sceneptr = find_scene(scene->xscene,scene->zscene)) != NULL) {
	/* Scene already exists!  */
	return(0);
    }

    /* Look for my left, right, up, down adjacent scenes */
    scene->left = scene->right = scene->up = scene->down = NULL;
    sceneptr = scene_head;
    while (sceneptr != NULL) {
	if (sceneptr->xscene == (scene->xscene - 1)) {
	    if (sceneptr->zscene == scene->zscene) {
		/* This one's to the left of me */
		sceneptr->right = scene;
		scene->left  = sceneptr;
	    }
	}
	else if (sceneptr->xscene == scene->xscene) {
	    if (sceneptr->zscene == (scene->zscene - 1)) {
		/* This one's below me */
		sceneptr->up   = scene;
		scene->down = sceneptr;
	    }
	    else if (sceneptr->zscene == (scene->zscene + 1)) {
		/* This one's above me */
		sceneptr->down = scene;
		scene->up   = sceneptr;
	    }
	}
	else if (sceneptr->xscene == (scene->xscene + 1)) {
	    if (sceneptr->zscene == scene->zscene) {
		/* This one's to the right of me */
		sceneptr->left  = scene;
		scene->right = sceneptr;
	    }
	}
	sceneptr = sceneptr->next;
    }

    /* Add to head of scene list */
    if (scene_head != NULL) scene_head->previous = scene;
    scene->previous      = NULL;
    scene->next          = scene_head;
    scene_head           = scene;

    return(1);
}


static void read_defined_scene(
    SCENE *scene, 
    int xscene, int zscene)
{
    struct stat statbuf;
    int num_segs;
    int retval;

    num_segs = cur_seg;  /* remember what the segments were before we do this */

    /* Make sure it's a normal file. */
    if (scene->scenefilename == NULL) return;
    if (stat(scene->scenefilename,&statbuf) == -1) return;
    if (statbuf.st_mode & 0070000) return;

    read_scenefile(scene->scenefilename, &retval);
    if (retval == 0) {
	scene->defined = 0;
	scene = generate_random_scene(scene, xscene,zscene);
    }
    else  {
#if 0
	/* We have just read in all the scenes from the file containing
	 * this scene.  Now, we need to send all the newly created segments
	 * out to all the clients */
	update_segment_lists( num_segs );
#endif
    }
}

void delete_object(
    DRIVE_OBJECT *obj)
{
    DRIVE_OBJECT *child,*next;

    if (obj->pobj) free_physical_object(obj->pobj);
    if (obj->previous != NULL) (obj->previous)->next = obj->next;
    if (obj->next != NULL) (obj->next)->previous = obj->previous;

    child = obj->child_list;
    while (child != NULL) {
	next = child->next;
	delete_object(child);
	child = next;
    }

    free(obj);
}


#define MAX_SCENES_PER_FILE	256
static void reread_scenefile(
    char *filename)
{
    DRIVE_OBJECT *saved_objects[MAX_SCENES_PER_FILE];
    SCENE *scene,*sceneptr[MAX_SCENES_PER_FILE];
    DRIVE_OBJECT *obj,*next;
    int i,scene_count;

    scene_count = 0;

    /* For every scene that uses this scenefile... */
    scene = scene_head;
    while (scene != NULL) {
	if ((scene->scenefilename != NULL) 
		&& (scene->defined)
		&& (scene->read_in)
		&& (strcmp(scene->scenefilename, filename) == 0)) {
	    /* This is one of the scenes we want */
	    sceneptr[scene_count] = scene;
	    /* Save aside all non-scene objects, delete the rest. */
	    saved_objects[scene_count] = NULL;
	    obj = scene->object_head;
	    while (obj != NULL) {
		next = obj->next;
		if (obj->connection) {
		    obj->next = saved_objects[scene_count];
		    obj->previous = NULL;
		    saved_objects[scene_count] = obj;
		}
		else {
		    delete_object(obj);
		}
		obj = next;
	    }
	    /* Clear out the scene. */
	    scene->read_in     = FALSE;
	    scene->object_head = NULL;
	    scene->static_seg  = INVALID;
	    if ((++scene_count) >= MAX_SCENES_PER_FILE) break;
	}
	scene = scene->next;
    }

    if (scene_count == 0) return;

    /* Now re-read the scenefile */
    read_defined_scene(sceneptr[0],sceneptr[0]->xscene,sceneptr[0]->zscene);

    /* Now re-add the saved objects */
    for (i=0; i<scene_count; ++i) {
	obj = saved_objects[i];
	while (obj != NULL) {
	    next = obj->next;
	    add_object_to_list(&(sceneptr[i]->object_head),obj);
	    obj = next;
	}
    }
}


/* When was it last modified? */
time_t scenefile_mtime(
    char *scenefilename)
{
    struct stat statbuf;
	
    if (stat(scenefilename,&statbuf) == -1) return(0);
    else return(statbuf.st_mtime);
}


void check_all_defined_scenefiles(
    void)
{
    SCENE *scene;

    /* Look for changed scene files; scan the whole list. */
    scene = scene_head;
    while (scene != NULL) {
	if ((scene->defined)
		&& (scene->read_in)) {
	    if (scenefile_mtime(scene->scenefilename) > scene->file_mtime) {
		reread_scenefile(scene->scenefilename);
	    }
	}
	scene = scene->next;
    }
}


static void check_this_scene(
    SCENE **scene, 
    int xscene, int zscene)
{
    if (*scene == NULL) {						
	*scene = generate_random_scene(NULL,xscene,zscene);	
    }									
    else if ((*scene)->object_head == NULL) {				
        if ((*scene)->defined) {					
	    if ((*scene)->read_in) {
		/* Must be a random scene defined in the file */
		*scene = generate_random_scene(*scene,xscene,zscene);
	    } 
	    else {
		read_defined_scene(*scene,xscene,zscene);		
	    }
        }								
	else {								
	    *scene = generate_random_scene(*scene,xscene,zscene);
        }								
    }
}


SCENE *check_scenes(
    DRIVE_OBJECT *obj,
    float xvehicle, float zvehicle)
{
    float xp,xn,zp,zn;
    int xscene,zscene;
    SCENE *scene;
    time_t when;

    scene = (SCENE *) obj->scene;

    /* First, check to make sure I'm in the right scene */
    if (xvehicle > 0.0)
	 xscene = (xvehicle + (SCENE_SIZE/2.0)) / SCENE_SIZE;
    else xscene = (xvehicle - (SCENE_SIZE/2.0)) / SCENE_SIZE;
    if (zvehicle > 0.0)
	 zscene = (zvehicle + (SCENE_SIZE/2.0)) / SCENE_SIZE;
    else zscene = (zvehicle - (SCENE_SIZE/2.0)) / SCENE_SIZE;

    if ((xscene != scene->xscene)
	    || (zscene != scene->zscene)) {
	/* new scene */
	scene = (void *) find_scene(xscene,zscene);
	check_this_scene(&scene,xscene,zscene);
	delete_object_from_scene((SCENE *) obj->scene,obj);
	add_object_to_list(&(scene->object_head),obj);
    }

    /* Now, make sure my neighbors are good. */
    xp = xscene*SCENE_SIZE + (SCENE_SIZE/2.0) - xvehicle;
    xn = SCENE_SIZE - xp;
    zp = zscene*SCENE_SIZE + (SCENE_SIZE/2.0) - zvehicle;
    zn = SCENE_SIZE - zp;

    if (xp < HORIZON) {
	check_this_scene(&(scene->right),scene->xscene+1,zscene);

	/* May be able to see right and up */
	if ((zp < HORIZON)
		&& (xp*xp + zp*zp < (HORIZON*HORIZON))) {
	    check_this_scene(&(scene->right->up),scene->xscene+1,zscene+1);
	}

	/* Or right and down */
	if ((zn < HORIZON)
		&& (xp*xp + zn*zn < (HORIZON*HORIZON))) {
	    check_this_scene(&(scene->right->down),scene->xscene+1,zscene-1);
	}
    }

    if (xn < HORIZON) {
	check_this_scene(&(scene->left),scene->xscene-1,zscene);

	/* May be able to see left and up */
	if ((zp < HORIZON)
		&& (xn*xn + zp*zp < (HORIZON*HORIZON))) {
	    check_this_scene(&(scene->left->up), scene->xscene-1,zscene+1);
	}

	/* Or left and down */
	if ((zn < HORIZON)
		&& (xn*xn + zn*zn < (HORIZON*HORIZON))) {
	    check_this_scene(&(scene->left->down), scene->xscene-1,zscene-1);
	}
    }

    if (zp < HORIZON) {
	check_this_scene(&(scene->up), scene->xscene,zscene+1);
    }

    if (zn < HORIZON) {
	check_this_scene(&(scene->down), scene->xscene,zscene-1);
    }

    /* Check periodically to see if I need to reread this scene */
    if ((server_reread_scenefiles)
	    && (scene->defined)
	    && (scene->read_in)
	    && ((updates % REREAD_UPDATE_COUNT) == 0)) {
	if ((when = scenefile_mtime(scene->scenefilename))
		> scene->file_mtime) {
	    /* Delay a little if the file is still being written */
	    if (when >= (time(NULL)-1)) {
		_hp_high_res_sleep(0.3);
	    }
	    reread_scenefile(scene->scenefilename);
	}
    }

    return(scene);
}

void create_scene_mongo_dl(
    SCENE *scene,
    int fildes)
{
    DRIVE_OBJECT *obj;
    int nobj;
    static float (*mats)[4][4];
    static int *segs, segAlloc;

    /* Count objects in the scene */
    obj = scene->object_head; nobj = 0;
    while( obj ) {
	if( obj->idptr->flags & OBJECTCLASS_DYNAMIC ) {
	    obj = obj->next;
	    continue;
	}
	if( obj->display_list == INVALID ) {
	    obj = obj->next;
	    continue;
	}
	nobj++; obj = obj->next;
    }

    /* Update static segment list allocation */
    if( nobj > segAlloc ) {
	segAlloc = nobj;
	mats = realloc( mats, segAlloc * 4*4*sizeof(float) );
	segs = realloc( segs, segAlloc * sizeof(int) );
	if( !segs || !mats ) exit( 1 );
    }

    /* Fill it in */
    obj = scene->object_head; nobj = 0;
    while( obj ) {
	if( obj->idptr->flags & OBJECTCLASS_DYNAMIC ) {
	    obj = obj->next;
	    continue;
	}
	if( obj->display_list == INVALID ) {
	    obj = obj->next;
	    continue;
	}
	(void)memcpy( &mats[nobj][0][0], &obj->xform[0][0], 4*4*sizeof(float) );
	segs[nobj] = obj->display_list;
	obj = obj->next;
	nobj++;
    }

    /* Create the scene message */
    scene->static_seg = createHwSegmentList( nobj, segs, mats );
}


/*****************************************************************
 * scene_update
 * 
 * Traverse each scene, updating the objects in scenes that have
 * been recently sent to a client. 
 * 	
 */
void scene_update(
    float interval)
{
    SCENE *scene;
    DRIVE_OBJECT *obj,*del_obj;
    
    scene = scene_head;

    while (scene != NULL) {
	if ((scene->count)++ < SCENE_LAST_SEEN) {
	    obj = scene->object_head;
	    while (obj != NULL) {
		if (obj->update_controls) (*(obj->update_controls))(obj);

		/* This section must come at the end of the loop */
		if (obj->update_self) {
		    switch((*(obj->update_self))(obj, interval)) {
			case RETURN_OK:
			    obj = obj->next;
			    break;

			case RETURN_DELETE_ME:
			    del_obj = obj;
			    obj = obj->next;
			    delete_object_from_scene((SCENE *) del_obj->scene,
				del_obj);
			    delete_object(del_obj);
			    break;
			
			default:
			    obj = obj->next;
			    if (debug) {
				fprintf(stderr,
				"ILLEGAL RETURN CONDITION obj->update_self!\n");
			    }
			    break;
		    }
		}
		else {
			obj = obj->next;
		}
	    }
	}
	scene = scene->next;
    }
}
