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
#include "object.h"
#include "scene.h"
#include "demo_physics.h"
#include "local_physics.h"
#include "drive_server.h"

extern void intersect_wormhole(
    DRIVE_OBJECT *obj,
    DRIVE_OBJECT *wormhole);

#define COG_Y_COLLISION_BAILOUT TRUE

typedef struct {
    int x,y,z;
} TRIPLE;

static TRIPLE corner[8] = {
    { 0,	1,	2 },
    { 0,	1,	5 },
    { 3,	1,	2 },
    { 3,	1,	5 },
    { 0,	4,	2 },
    { 0,	4,	5 },
    { 3,	4,	2 },
    { 3,	4,	5 },
};

#define BBOXES_INTERSECT(bbox1,bbox2) \
    ((((bbox2[0] >= bbox1[0]) && (bbox2[0] <= bbox1[3])) \
	|| ((bbox2[3] >= bbox1[0]) && (bbox2[3] <= bbox1[3])) \
	|| ((bbox2[0] <= bbox1[0]) && (bbox2[3] >= bbox1[3]))) \
    && (((bbox2[1] >= bbox1[1]) && (bbox2[1] <= bbox1[4])) \
	|| ((bbox2[4] >= bbox1[1]) && (bbox2[4] <= bbox1[4])) \
	|| ((bbox2[1] <= bbox1[1]) && (bbox2[4] >= bbox1[4]))) \
    && (((bbox2[2] >= bbox1[2]) && (bbox2[2] <= bbox1[5])) \
	|| ((bbox2[5] >= bbox1[2]) && (bbox2[5] <= bbox1[5])) \
	|| ((bbox2[2] <= bbox1[2]) && (bbox2[5] >= bbox1[5]))))



void mc_to_wc(
    DRIVE_OBJECT *obj,
    float mx, float my, float mz,
    float *wx, float *wy, float *wz)
{
    *wx = obj->xform[0][0] * mx
	+ obj->xform[1][0] * my
	+ obj->xform[2][0] * mz
	+ obj->xform[3][0];
    *wy = obj->xform[0][1] * mx
	+ obj->xform[1][1] * my
	+ obj->xform[2][1] * mz
	+ obj->xform[3][1];
    *wz = obj->xform[0][2] * mx
	+ obj->xform[1][2] * my
	+ obj->xform[2][2] * mz
	+ obj->xform[3][2];
}


void wc_to_mc(
    DRIVE_OBJECT *obj,
    float wx, float wy, float wz,
    float *mx, float *my, float *mz)
{
    *mx = obj->ixform[0][0] * wx
	+ obj->ixform[1][0] * wy
	+ obj->ixform[2][0] * wz
	+ obj->ixform[3][0];
    *my = obj->ixform[0][1] * wx
	+ obj->ixform[1][1] * wy
	+ obj->ixform[2][1] * wz
	+ obj->ixform[3][1];
    *mz = obj->ixform[0][2] * wx
	+ obj->ixform[1][2] * wy
	+ obj->ixform[2][2] * wz
	+ obj->ixform[3][2];
}


void mc_normal_to_wc_normal(
    DRIVE_OBJECT *obj,
    float mc_normal[3],
    float wc_normal[3])
{
    float x = mc_normal[0];
    float y = mc_normal[1];
    float z = mc_normal[2];

    wc_normal[0] =
	(obj->xform[0][0] * x +
	 obj->xform[1][0] * y +
	 obj->xform[2][0] * z)
	/ HYPOT3(obj->xform[0][0],obj->xform[1][0],obj->xform[2][0]);
    wc_normal[1] =
	(obj->xform[0][1] * x +
	 obj->xform[1][1] * y +
	 obj->xform[2][1] * z)
	/ HYPOT3(obj->xform[0][1],obj->xform[1][1],obj->xform[2][1]);
    wc_normal[2] =
	(obj->xform[0][2] * x +
	 obj->xform[1][2] * y +
	 obj->xform[2][2] * z)
	/ HYPOT3(obj->xform[0][2],obj->xform[1][2],obj->xform[2][2]);
}



static void transform_bbox(
    float bbox_in[6],
    float bbox_out[6],
    float xform[4][4])
{
    float point_mc[8][3];
    float x,y,z;
    float minx,miny,minz,maxx,maxy,maxz;
    int i;

    for (i=0; i<8; ++i) {
	point_mc[i][0] = bbox_in[corner[i].x];
	point_mc[i][1] = bbox_in[corner[i].y];
	point_mc[i][2] = bbox_in[corner[i].z];
    }

    minx = miny = minz = (1e+30);
    maxx = maxy = maxz = -(1e+30);

    for (i=0; i<8; ++i) {
	x = point_mc[i][0] * xform[0][0] +
	    point_mc[i][1] * xform[1][0] +
	    point_mc[i][2] * xform[2][0] +
	    xform[3][0];
	if (x < minx) minx = x;
	if (x > maxx) maxx = x;
	y = point_mc[i][0] * xform[0][1] +
	    point_mc[i][1] * xform[1][1] +
	    point_mc[i][2] * xform[2][1] +
	    xform[3][1];
	if (y < miny) miny = y;
	if (y > maxy) maxy = y;
	z = point_mc[i][0] * xform[0][2] +
	    point_mc[i][1] * xform[1][2] +
	    point_mc[i][2] * xform[2][2] +
	    xform[3][2];
	if (z < minz) minz = z;
	if (z > maxz) maxz = z;
    }

    bbox_out[0] = minx;
    bbox_out[1] = miny;
    bbox_out[2] = minz;
    bbox_out[3] = maxx;
    bbox_out[4] = maxy;
    bbox_out[5] = maxz;
}


void update_wc_bounds(
    DRIVE_OBJECT *obj)
{
    transform_bbox(obj->bound_mc,obj->bound_wc,obj->xform);
}


static void check_intersections_xyz(
    DRIVE_OBJECT *list,			/* which list to search */
    DRIVE_OBJECT *myobj,			/* which object to ignore */
    float x, float y, float z,		/* WCs */
    WC_SURFACE_CHARACTERISTICS *wc_sc)	/* output */
{
    DRIVE_OBJECT *obj,*child;
    float mcx,mcy,mcz;
    float wcx,wcy,wcz;
    MC_SURFACE_CHARACTERISTICS mc_sc;

    for (obj = list; obj != NULL; obj = obj->next) {
	/* Do not intersect yourself. */
	if (obj == myobj) continue;

	/* Ignore objects that "aren't there" at the current time */
	if (!(obj->nameset_bits & current_nameset_bit)) continue;

	/* Check to see if the point x,y,z is in the world coordinate
	 * bounding box of the current object 
	 */
	if (!POINT_IN_BBOX(obj->bound_wc,x,y,z)) continue;

	/* Now check in MCs */
	wc_to_mc(obj,x,y,z,&mcx,&mcy,&mcz);
	if (!POINT_IN_BBOX(obj->bound_mc,mcx,mcy,mcz)) continue;

	if (obj->surface_chars_xyz != NULL) {
	    /* Set defaults */
	    mc_sc.mc_x = mcx;
	    mc_sc.mc_y = mcy;
	    mc_sc.mc_z = mcz;
	    mc_sc.mc_normal[0] = 0.0;
	    mc_sc.mc_normal[1] = 1.0;
	    mc_sc.mc_normal[2] = 0.0;
	    mc_sc.friction  = 1.0;
	    mc_sc.roughness = 0.0;
	    /* Call the object's intersection routine. */
	    if (!(*obj->surface_chars_xyz)(obj,mcx,mcy,mcz,&mc_sc)) continue;

	    /* OK!  Get the WC point of intersection. */
	    mc_to_wc(obj, mc_sc.mc_x,mc_sc.mc_y,mc_sc.mc_z, &wcx,&wcy,&wcz);

	    if (obj->idptr->flags & OBJECTCLASS_DYNAMIC) {
		/* Just use the highest point on the object */
		wcy = obj->bound_wc[4];
	    }

	    /* Make sure this is the highest object at this point. */
	    if (wcy <= wc_sc->wc_y) continue;

	    /* else found a good object! */
	    wc_sc->wc_x = wcx; wc_sc->wc_y = wcy; wc_sc->wc_z = wcz;
	    mc_normal_to_wc_normal(obj,mc_sc.mc_normal,wc_sc->wc_normal);
	    wc_sc->friction		= mc_sc.friction;
	    wc_sc->roughness		= mc_sc.roughness;
	    wc_sc->roughness_frequency	= mc_sc.roughness_frequency;
	    wc_sc->whichobj		= obj;
	}

	if ((child = obj->child_list) != NULL) {
	    /* object has children.  Must call this on each. */
	    while (child != NULL) {
		if (POINT_IN_BBOX(child->bound_wc,x,y,z)) {
		    check_intersections_xyz(child,myobj,x,y,z,wc_sc);
		}
		child = child->next;
	    }
	}
    }
}


static void check_intersections_bbox(
    DRIVE_OBJECT *list,			/* which list to search */
    DRIVE_OBJECT *myobj,			/* which object to ignore */
    WC_SURFACE_CHARACTERISTICS *wc_sc)	/* output */
{
    DRIVE_OBJECT *obj,*child;
    float bbox_mc[6];
    float mc_to_mc_mat[4][4];
    float wcx,wcy,wcz;
    MC_SURFACE_CHARACTERISTICS mc_sc;

    for (obj = list; obj != NULL; obj = obj->next) {
	/* Do not intersect yourself. */
	if (obj == myobj) continue;

	/* Ignore objects that "aren't there" at the current time */
	if (!(obj->nameset_bits & current_nameset_bit)) continue;

	/* Check to see if the WC bounding boxes overlap. */
	if (!BBOXES_INTERSECT(obj->bound_wc,myobj->bound_wc)) continue;

	/* Now check in MCs */
	/* Transform him into my MCs and make sure we intersect. */
	concat_matrix(obj->xform,myobj->ixform,mc_to_mc_mat);
	transform_bbox(obj->bound_mc,bbox_mc,mc_to_mc_mat);
	if (!BBOXES_INTERSECT(myobj->bound_mc,bbox_mc)) continue;

	/* Transfrom me into his MCs and make sure we intersect. */
	concat_matrix(myobj->xform,obj->ixform,mc_to_mc_mat);
	transform_bbox(myobj->bound_mc,bbox_mc,mc_to_mc_mat);
	if (!BBOXES_INTERSECT(obj->bound_mc,bbox_mc)) continue;

	if (obj->surface_chars_bbox != NULL) {
	    /* Set defaults */
	    /* Use the MCs we've still got. */
	    mc_sc.mc_x = (bbox_mc[0] + bbox_mc[3])/2.0;
	    mc_sc.mc_y = (bbox_mc[1] + bbox_mc[4])/2.0;
	    mc_sc.mc_z = (bbox_mc[2] + bbox_mc[5])/2.0;
	    mc_sc.mc_normal[0] = 0.0;
	    mc_sc.mc_normal[1] = 1.0;
	    mc_sc.mc_normal[2] = 0.0;
	    mc_sc.friction  = 1.0;
	    mc_sc.roughness = 0.0;
	    /* Call the object's intersection routine. */
	    if (!(*obj->surface_chars_bbox)(obj,bbox_mc,&mc_sc)) continue;

	    /* OK!  Get the WC point of intersection. */
	    mc_to_wc(obj, mc_sc.mc_x,mc_sc.mc_y,mc_sc.mc_z, &wcx,&wcy,&wcz);

	    if (obj->idptr->flags & OBJECTCLASS_DRIVEABLE) {
		/* Just use the highest point on the object */
		wcy = obj->bound_wc[4];
	    }

	    /* Make sure this is the highest object at this point. */
	    if (wcy <= wc_sc->wc_y) continue;

	    /* else found a good object! */
	    wc_sc->wc_x = wcx; wc_sc->wc_y = wcy; wc_sc->wc_z = wcz;
	    mc_normal_to_wc_normal(obj,mc_sc.mc_normal,wc_sc->wc_normal);
	    wc_sc->friction		= mc_sc.friction;
	    wc_sc->roughness		= mc_sc.roughness;
	    wc_sc->roughness_frequency	= mc_sc.roughness_frequency;
	    wc_sc->whichobj		= obj;

	    /* See if the object we intersected was a checkpoint type object.
	     * If so, call the appropriate routine and let the checkpoint
	     * object update the driveable type object.
	     * Or, maybe it's a wormhole.
	     */
	    if (obj->idptr->flags & OBJECTCLASS_CHECKPOINT) {
		if (myobj->last_checkpt != obj) {
		    if (debug) fprintf(stderr,"Intersected A Checkpoint! \n");
		    intersect_checkpoint(myobj,obj);
		}
		myobj->last_checkpt = obj;
	    }
	    else if (obj->idptr->flags & OBJECTCLASS_WORMHOLE) {
		    if (debug) fprintf(stderr,"Intersected A Wormhole! \n");
		    intersect_wormhole(myobj,obj);
	    }
	}

	if ((child = obj->child_list) != NULL) {
	    /* object has children.  Must call this on each. */
	    while (child != NULL) {
		if (BBOXES_INTERSECT(child->bound_wc,myobj->bound_wc)) {
		    check_intersections_bbox(child,myobj,wc_sc);
		}
		child = child->next;
	    }
	}
    }
}


void intersect_objects_xyz(
    DRIVE_OBJECT *myobj,
    float x, float y, float z,
    WC_SURFACE_CHARACTERISTICS *wc_sc)
{
    DRIVE_OBJECT *list;

    wc_sc->whichobj = NULL;
    wc_sc->wc_y	    = -(1.0e+30);
    list            = ((SCENE *) (myobj->scene))->object_head;
    check_intersections_xyz(list,myobj,x,y,z,wc_sc);
}


void intersect_objects_bbox(
    DRIVE_OBJECT *myobj,
    WC_SURFACE_CHARACTERISTICS *wc_sc)
{
    DRIVE_OBJECT *list;

    wc_sc->whichobj = NULL;
    wc_sc->wc_y	    = -(1.0e+30);
    list            = ((SCENE *) (myobj->scene))->object_head;
    check_intersections_bbox(list,myobj,wc_sc);
}


int point_in_object(
    DRIVE_OBJECT *obj,
    float wcx, float wcy, float wcz)
{
    float mcx,mcy,mcz;
    MC_SURFACE_CHARACTERISTICS mc_sc;

    if (!POINT_IN_BBOX(obj->bound_wc,wcx,wcy,wcz)) return(FALSE);

    /* Ignore objects that "aren't there" at the current time */
    if (!(obj->nameset_bits & current_nameset_bit)) return(FALSE);

    /* Try it in MCs -- sometimes that helps */
    wc_to_mc(obj,wcx,wcy,wcz,&mcx,&mcy,&mcz);
    if (!POINT_IN_BBOX(obj->bound_mc,mcx,mcy,mcz)) return(FALSE);

    /* If the object doesn't completely fill the bounding box,
     * we might be able to miss it still.
     */
    return((*obj->surface_chars_xyz)(obj,mcx,mcy,mcz,&mc_sc));
}


#define MAXDIST	(1.0e+20)

void find_bbox_point_closest_to_point(
    float bbox[6],
    float cx, float cy, float cz,
    float *intx, float *inty, float *intz)
{
    float dist,mindist;
    float x,y,z;
    int i;

    if (POINT_IN_BBOX(bbox,cx,cy,cz)) {
	/* The point is actually in the bounding box -- return it! */
	*intx = cx;
	*inty = cy;
	*intz = cz;
	return;
    }
    /* else */

    /* Find the corner of the bbox closest to the point. */
    mindist = DISTSQ(bbox[corner[0].x],bbox[corner[0].y],bbox[corner[0].z],
	cx,cy,cz);
    *intx = bbox[corner[0].x];
    *inty = bbox[corner[0].y];
    *intz = bbox[corner[0].z];

    for (i=1; i<8; ++i) {
	x = bbox[corner[i].x]; y = bbox[corner[i].y]; z = bbox[corner[i].z];
	if ((dist = DISTSQ(x,y,z, cx,cy,cz)) < mindist) {
	    mindist = dist;
	    *intx = x;
	    *inty = y;
	    *intz = z;
	}
    }
}


#define INTPT_GRANULARITY	(10.0)

static int refine_collision_point(
    DRIVE_OBJECT *obj1, DRIVE_OBJECT *obj2,
    float t, float dt,
    float vx, float vy, float vz,
    float *intx, float *inty, float *intz,
    float *t_int)
{
    int obj1_in_cnt,obj2_in_cnt;
    int obj1_pt_in[8],obj2_pt_in[8];
    float obj1_corner[8][3], obj2_corner[8][3];
    int i,count;
    float x=0.0,y=0.0,z=0.0;


    /* Assume that the collision occured at one of the corners of
     * one of the objects and figure out where and when the collision
     * really occured in WCs.
     */
    for (i=0; i<8; ++i) {
	mc_to_wc(obj1,
	    obj1->bound_mc[corner[i].x],
	    obj1->bound_mc[corner[i].y],
	    obj1->bound_mc[corner[i].z],
	    &(obj1_corner[i][0]),
	    &(obj1_corner[i][1]),
	    &(obj1_corner[i][2]));
	mc_to_wc(obj2,
	    obj2->bound_mc[corner[i].x],
	    obj2->bound_mc[corner[i].y],
	    obj2->bound_mc[corner[i].z],
	    &(obj2_corner[i][0]),
	    &(obj2_corner[i][1]),
	    &(obj2_corner[i][2]));
    }

    /* Keep track of which corners of each object are in and out of
     * the other object.  We're going to back time up (or move it
     * forward) until none of the corners are in the other object.
     */
    obj1_in_cnt = obj2_in_cnt = 0;
    for (i=0; i<8; ++i) {
	if ((obj1_pt_in[i] = point_in_object(obj2,
		obj1_corner[i][0] + vx*t,
		obj1_corner[i][1] + vy*t,
		obj1_corner[i][2] + vz*t))) {
	    ++obj1_in_cnt;
	}
	if ((obj2_pt_in[i] = point_in_object(obj1,
		obj2_corner[i][0] - vx*t,
		obj2_corner[i][1] - vy*t,
		obj2_corner[i][2] - vz*t))) {
	    ++obj2_in_cnt;
	}
    }

    if (obj1_in_cnt + obj2_in_cnt == 0) {
	return(FALSE);
    }

    /* Take a step forward */
    t += dt;

    /* Now, as long as there's points in the other object... */
    count = 0;
    while (count++ < (INTPT_GRANULARITY+1)) {
	for (i=0; i<8; ++i) {
	    if (obj1_pt_in[i]) {
		x = obj1_corner[i][0] + vx*t;
		y = obj1_corner[i][1] + vy*t;
		z = obj1_corner[i][2] + vz*t;

		if (!point_in_object(obj2,x,y,z)) {
		    obj1_pt_in[i] = FALSE;
		    --obj1_in_cnt;
		    if ((obj1_in_cnt + obj2_in_cnt) == 0) {
			/* got it! */
			*intx = x;
			*inty = y;
			*intz = z;
			*t_int = t;
			return(TRUE);
		    }
		}
	    }
	    if (obj2_pt_in[i]) {
		x = obj2_corner[i][0] - vx*t;
		y = obj2_corner[i][1] - vy*t;
		z = obj2_corner[i][2] - vz*t;

		if (!point_in_object(obj1,x,y,z)) {
		    obj2_pt_in[i] = FALSE;
		    --obj2_in_cnt;
		    if ((obj1_in_cnt + obj2_in_cnt) == 0) {
			/* got it! */
			*intx = x;
			*inty = y;
			*intz = z;
			*t_int = t;
			return(TRUE);
		    }
		}
	    }
	}
	t += dt;
    }

    *intx = x;
    *inty = y;
    *intz = z;
    *t_int = t;
    return(TRUE);
}


static void find_intersection_center(
    DRIVE_OBJECT *obj1, DRIVE_OBJECT *obj2,
    float *cx, float *cy, float *cz)
{
    float mc_to_mc_mat[4][4];
    float bbox[6],mcbbox[6],minmcbbox[6],smallest_bbox[6];

    /* Just return the center of the intersection of the bounding boxes.
     * Do the intersection in WC's, obj1's MC's, and obj2's MC's and use
     * the solution that provides the tightest bounds.
     */

    /* Do the intersection in WCs. */
    smallest_bbox[0] = MAX(obj1->bound_wc[0],obj2->bound_wc[0]);
    smallest_bbox[1] = MAX(obj1->bound_wc[1],obj2->bound_wc[1]);
    smallest_bbox[2] = MAX(obj1->bound_wc[2],obj2->bound_wc[2]);
    smallest_bbox[3] = MIN(obj1->bound_wc[3],obj2->bound_wc[3]);
    smallest_bbox[4] = MIN(obj1->bound_wc[4],obj2->bound_wc[4]);
    smallest_bbox[5] = MIN(obj1->bound_wc[5],obj2->bound_wc[5]);

    /* Try doing the intersection in obj2's MCs. */
    concat_matrix(obj1->xform,obj2->ixform,mc_to_mc_mat);
    transform_bbox(obj1->bound_mc,mcbbox,mc_to_mc_mat);
    minmcbbox[0] = MAX(obj2->bound_mc[0],mcbbox[0]);
    minmcbbox[1] = MAX(obj2->bound_mc[1],mcbbox[1]);
    minmcbbox[2] = MAX(obj2->bound_mc[2],mcbbox[2]);
    minmcbbox[3] = MIN(obj2->bound_mc[3],mcbbox[3]);
    minmcbbox[4] = MIN(obj2->bound_mc[4],mcbbox[4]);
    minmcbbox[5] = MIN(obj2->bound_mc[5],mcbbox[5]);
    /* Convert back to WCs. */
    transform_bbox(minmcbbox,bbox,obj2->xform);
    /* Use smallest bounds. */
    if (bbox[0] > smallest_bbox[0]) smallest_bbox[0] = bbox[0];
    if (bbox[1] > smallest_bbox[1]) smallest_bbox[1] = bbox[1];
    if (bbox[2] > smallest_bbox[2]) smallest_bbox[2] = bbox[2];
    if (bbox[3] < smallest_bbox[3]) smallest_bbox[3] = bbox[3];
    if (bbox[4] < smallest_bbox[4]) smallest_bbox[4] = bbox[4];
    if (bbox[5] < smallest_bbox[5]) smallest_bbox[5] = bbox[5];

    /* Now try doing the intersection in obj1's MCs. */
    concat_matrix(obj2->xform,obj1->ixform,mc_to_mc_mat);
    transform_bbox(obj2->bound_mc,mcbbox,mc_to_mc_mat);
    minmcbbox[0] = MAX(obj1->bound_mc[0],mcbbox[0]);
    minmcbbox[1] = MAX(obj1->bound_mc[1],mcbbox[1]);
    minmcbbox[2] = MAX(obj1->bound_mc[2],mcbbox[2]);
    minmcbbox[3] = MIN(obj1->bound_mc[3],mcbbox[3]);
    minmcbbox[4] = MIN(obj1->bound_mc[4],mcbbox[4]);
    minmcbbox[5] = MIN(obj1->bound_mc[5],mcbbox[5]);
    /* Convert back to WCs. */
    transform_bbox(minmcbbox,bbox,obj1->xform);
    /* Use smallest bounds. */
    if (bbox[0] > smallest_bbox[0]) smallest_bbox[0] = bbox[0];
    if (bbox[1] > smallest_bbox[1]) smallest_bbox[1] = bbox[1];
    if (bbox[2] > smallest_bbox[2]) smallest_bbox[2] = bbox[2];
    if (bbox[3] < smallest_bbox[3]) smallest_bbox[3] = bbox[3];
    if (bbox[4] < smallest_bbox[4]) smallest_bbox[4] = bbox[4];
    if (bbox[5] < smallest_bbox[5]) smallest_bbox[5] = bbox[5];

    *cx = (smallest_bbox[0]+smallest_bbox[3])/2.0;
    *cy = (smallest_bbox[1]+smallest_bbox[4])/2.0;
    *cz = (smallest_bbox[2]+smallest_bbox[5])/2.0;
}


#define COLLISION_VELOCITY_EPSILON (0.1)

/* Returns rough amount of time until the objects no longer intersect. */
float get_rough_time_estimate(
    DRIVE_OBJECT *obj1, DRIVE_OBJECT *obj2,
    float t_interval,
    float vx, float vy, float vz)
{
    float t_out,t_in,t_diff,t_last,t;
    float tmpwcbbox[6],tmpmcbbox1[6],tmpmcbbox2[6];
    float mc_to_mc_mat[4][4];
    float vx_mc1,vy_mc1,vz_mc1;
    float vx_mc2,vy_mc2,vz_mc2;
    float dx,dy,dz;
    float basex,basey,basez;
    MC_SURFACE_CHARACTERISTICS mc_sc;


    /* SIMPLIFICATION:  assume collision point moving linearly.
     * Run time backwards/forwards in increments of t_interval
     * until the objects no longer intersect.  That way we know
     * roughly the time when things happened.
     */
    memcpy(tmpwcbbox,obj1->bound_wc,sizeof(tmpwcbbox));

    /* Convert obj1's MC bbox to obj2's MCs */
    concat_matrix(obj1->xform,obj2->ixform,mc_to_mc_mat);
    transform_bbox(obj1->bound_mc,tmpmcbbox1,mc_to_mc_mat);

    /* Convert obj2's MC bbox to obj1's MCs */
    concat_matrix(obj2->xform,obj1->ixform,mc_to_mc_mat);
    transform_bbox(obj2->bound_mc,tmpmcbbox2,mc_to_mc_mat);

    wc_to_mc(obj2,vx,vy,vz,&vx_mc1,&vy_mc1,&vz_mc1);
    wc_to_mc(obj2,0.0,0.0,0.0,&basex,&basey,&basez);
    vx_mc1 -= basex; vy_mc1 -= basey; vz_mc1 -= basez;

    wc_to_mc(obj1,-vx,-vy,-vz,&vx_mc2,&vy_mc2,&vz_mc2);
    wc_to_mc(obj1,0.0,0.0,0.0,&basex,&basey,&basez);
    vx_mc2 -= basex; vy_mc2 -= basey; vz_mc2 -= basez;

    /* First find some time when the objects no longer collide.
     * Since we just want something in the area, double the time increment
     * each time.  We'll refine the time later. 
     */
    t_in = t_last = 0.0;
    t_out = t_interval;
    while (1) {
	t_diff = t_out - t_last;
	t_last = t_out;
	tmpwcbbox[0] += (dx = vx*t_diff);
	tmpwcbbox[1] += (dy = vy*t_diff);
	tmpwcbbox[2] += (dz = vz*t_diff);
	tmpwcbbox[3] += dx;
	tmpwcbbox[4] += dy;
	tmpwcbbox[5] += dz;
	if (!BBOXES_INTERSECT(tmpwcbbox,obj2->bound_wc)) break;

	tmpmcbbox1[0] += (dx = vx_mc1*t_diff);
	tmpmcbbox1[1] += (dy = vy_mc1*t_diff);
	tmpmcbbox1[2] += (dz = vz_mc1*t_diff);
	tmpmcbbox1[3] += dx;
	tmpmcbbox1[4] += dy;
	tmpmcbbox1[5] += dz;
	if (!BBOXES_INTERSECT(tmpmcbbox1,obj2->bound_mc)) break;

	tmpmcbbox2[0] += (dx = vx_mc2*t_diff);
	tmpmcbbox2[1] += (dy = vy_mc2*t_diff);
	tmpmcbbox2[2] += (dz = vz_mc2*t_diff);
	tmpmcbbox2[3] += dx;
	tmpmcbbox2[4] += dy;
	tmpmcbbox2[5] += dz;
	if (!BBOXES_INTERSECT(obj1->bound_mc,tmpmcbbox2)) break;

	/* Well, their bounding boxes still intersect, but do *they?* */
	if (obj2->surface_chars_bbox != NULL)
	    if (!(*obj2->surface_chars_bbox)(obj2,tmpmcbbox1,&mc_sc)) break;

	/* Could check the other way too, but since it's usually a car,
	 * it's of little use.
	 */

	/* Still in! */
	t_in = t_out;
	t_out *= 2.0;
    }

    /* Now refine with binary search... */
    while (ABS(t_out - t_in) > ABS(t_interval*1.1)) {
	t = (t_out + t_in) / 2.0;
	t_diff = t - t_last;
	t_last = t;
	tmpwcbbox[0] += (dx = vx*t_diff);
	tmpwcbbox[1] += (dy = vy*t_diff);
	tmpwcbbox[2] += (dz = vz*t_diff);
	tmpwcbbox[3] += dx;
	tmpwcbbox[4] += dy;
	tmpwcbbox[5] += dz;
	if (!BBOXES_INTERSECT(tmpwcbbox,obj2->bound_wc)) {
	    t_out = t;
	    continue;
	}

	tmpmcbbox1[0] += (dx = vx_mc1*t_diff);
	tmpmcbbox1[1] += (dy = vy_mc1*t_diff);
	tmpmcbbox1[2] += (dz = vz_mc1*t_diff);
	tmpmcbbox1[3] += dx;
	tmpmcbbox1[4] += dy;
	tmpmcbbox1[5] += dz;
	if (!BBOXES_INTERSECT(tmpmcbbox1,obj2->bound_mc)) {
	    t_out = t;
	    continue;
	}

	tmpmcbbox2[0] += (dx = vx_mc2*t_diff);
	tmpmcbbox2[1] += (dy = vy_mc2*t_diff);
	tmpmcbbox2[2] += (dz = vz_mc2*t_diff);
	tmpmcbbox2[3] += dx;
	tmpmcbbox2[4] += dy;
	tmpmcbbox2[5] += dz;
	if (!BBOXES_INTERSECT(obj1->bound_mc,tmpmcbbox2)) {
	    t_out = t;
	    continue;
	}

	/* Well, their bounding boxes still intersect, but do *they?* */
	if (obj2->surface_chars_bbox != NULL) {
	    if (!(*obj2->surface_chars_bbox)(obj2,tmpmcbbox1,&mc_sc)) {
		t_out = t;
		continue;
	    }
	}

	/* Could check the other way too, but since it's usually a car,
	 * it's of little use.
	 */

	/* Still in! */
	t_in = t;
    }

    return(t_out);
}


void find_collision_point(
    DRIVE_OBJECT *obj1, DRIVE_OBJECT *obj2,		/* IN: the objects */
    float t_interval,			/* IN: time step interval */
    float *v1x, float *v1y, float *v1z,	/* IN+OUT: WC velocity */
    float *v2x, float *v2y, float *v2z,	/* IN+OUT: WC velocity */
    float *intx, float *inty, float *intz,/* OUT: intersection point */
    float *t_int)			/* OUT: time of intersection */
{
    float tpos,tneg,t_out,t,dt,time_sign;
    int count;
    float vx,vy,vz,velocity;

    vx = *v1x - *v2x; vy = *v1y - *v2y; vz = *v1z - *v2z;

    /* If not moving, not much we can do. */
    if ((velocity = HYPOT3(vx,vy,vz)) < COLLISION_VELOCITY_EPSILON) {
	find_intersection_center(obj1,obj2,intx,inty,intz);
	*t_int = 0.0;
	return;
    }

    tneg = get_rough_time_estimate(obj1,obj2,-t_interval,vx,vy,vz);
    tpos = get_rough_time_estimate(obj1,obj2, t_interval,vx,vy,vz);

    /* Weigh it a little in negative's favor. */
    if (-tneg < tpos*2.0) {
	time_sign = -1;
	t_out = tneg;
    }
    else {
	time_sign = 1;
	t_out = tpos;
    }

    /* If it's too long, try just moving the objects apart from each other.
     * This will do a better job of taking care of tricky situations like
     * intersections with curved walls, where moving either forward or 
     * backward in time still results in a collision.
     */
    if (t_out*time_sign > 4*t_interval) {
	float dx,dy,dz,vl;

	/* No need for correct magnitude since we'll normalize later. */
	dx = (obj2->bound_wc[0] + obj2->bound_wc[3]) - 
	     (obj1->bound_wc[0] + obj1->bound_wc[3]);
#ifdef COG_Y_COLLISION_BAILOUT
	dy = 0.0;
#else /* !COG_Y_COLLISION_BAILOUT */
	dy = (obj2->bound_wc[1] + obj2->bound_wc[4]) - 
	     (obj1->bound_wc[1] + obj1->bound_wc[4]);
#endif /* COG_Y_COLLISION_BAILOUT else */
	dz = (obj2->bound_wc[2] + obj2->bound_wc[5]) - 
	     (obj1->bound_wc[2] + obj1->bound_wc[5]);

	/* Make it the same magnitude as the velocity. */
	if ((vl = HYPOT3(dx,dy,dz)) > COLLISION_VELOCITY_EPSILON) {
	    /* Now make it the same length as the velocity string */
	    dx *= velocity/vl; dy *= velocity/vl; dz *= velocity/vl;
	    t = get_rough_time_estimate(obj1,obj2,-t_interval,dx,dy,dz);
	    if (-t < (t_out*time_sign/4.0)) {
		/* This is better */
		t_out = t;
		time_sign = -1;
		vx = dx; vy = dy; vz = dz;
		*v1x = dx; *v1y = dy; *v1z = dz;
		*v2x = 0.0; *v2y = 0.0; *v2z = 0.0;
	    }
	}
    }

    /* Go back to where we still intersected. */
    t  = t_out - t_interval*time_sign;
    dt = t_interval*time_sign/INTPT_GRANULARITY;
    count = 0;
    while (count++ < (INTPT_GRANULARITY*2)) {
	if (refine_collision_point(obj1,obj2,t,dt,vx,vy,vz,
		intx,inty,intz,t_int))
	    return;
	/* else  back up one time interval and try again. */
	t -= dt;
    }

    /* Our nice refinement routine has failed us.  Just guess. */
    find_intersection_center(obj1,obj2,intx,inty,intz);
    *t_int = t_out - t_interval*time_sign;
}


void find_collision_normal(
    DRIVE_OBJECT *obj1, DRIVE_OBJECT *obj2,	/* The objects */
    float vx, float vy, float vz,		/* of obj1 relative to obj2 */
    float collx, float colly, float collz,	/* WC intersection point */
    float t_int,				/* time of intersection */
    float wc_normal[3])				/* Returned WC normal. */
{
    DRIVE_OBJECT *whichobj;
    float d1,d2,dx,dy,dz,dist_obj1,dist_obj2;
    float cpx_mc1,cpy_mc1,cpz_mc1;
    float cpx_mc2,cpy_mc2,cpz_mc2;
    float mcx,mcy,mcz;
    MC_SURFACE_CHARACTERISTICS mc_sc;
    int count,got_normal;


    /* Convert intersection point to obj1 modelling coordinates */
    wc_to_mc(obj1, collx,colly,collz, &cpx_mc1,&cpy_mc1,&cpz_mc1);

    /* See how close we are to a corner. */
    if ((d1 = ABS(cpx_mc1 - obj1->bound_mc[0]))
	    < (d2 = ABS(cpx_mc1 - obj1->bound_mc[3])))
	dx = d1;
    else dx = d2;

    if ((d1 = ABS(cpy_mc1 - obj1->bound_mc[1]))
	    < (d2 = ABS(cpy_mc1 - obj1->bound_mc[4])))
	dy = d1;
    else dy = d2;

    if ((d1 = ABS(cpz_mc1 - obj1->bound_mc[2]))
	    < (d2 = ABS(cpz_mc1 - obj1->bound_mc[5])))
	dz = d1;
    else dz = d2;
    dist_obj1 = dx*dx + dy*dy + dz*dz;


    /* Now try obj2 */
    wc_to_mc(obj2, collx,colly,collz, &cpx_mc2,&cpy_mc2,&cpz_mc2);

    /* See how close we are to a corner. */
    if ((d1 = ABS(cpx_mc2 - obj2->bound_mc[0]))
	    < (d2 = ABS(cpx_mc2 - obj2->bound_mc[3])))
	dx = d1;
    else dx = d2;

    if ((d1 = ABS(cpy_mc2 - obj2->bound_mc[1]))
	    < (d2 = ABS(cpy_mc2 - obj2->bound_mc[4])))
	dy = d1;
    else dy = d2;

    if ((d1 = ABS(cpz_mc2 - obj2->bound_mc[2]))
	    < (d2 = ABS(cpz_mc2 - obj2->bound_mc[5])))
	dz = d1;
    else dz = d2;
    dist_obj2 = dx*dx + dy*dy + dz*dz;


    /* Work with the one furthest from a corner. */
    if (dist_obj1 > dist_obj2) {
	whichobj = obj1;
	mcx = cpx_mc1;
	mcy = cpy_mc1;
	mcz = cpz_mc1;
    }
    else {
	whichobj = obj2;
	mcx = cpx_mc2;
	mcy = cpy_mc2;
	mcz = cpz_mc2;
    }

    got_normal = FALSE;
    if (whichobj->surface_chars_xyz != NULL) {
	/* Set defaults (we only care about the normal). */
	mc_sc.mc_normal[0] = 0.0;
	mc_sc.mc_normal[1] = 1.0;
	mc_sc.mc_normal[2] = 0.0;

	got_normal = TRUE;
	count = 0;
	/* Get the normal at that point */
	while (!(*whichobj->surface_chars_xyz)(whichobj,mcx,mcy,mcz,&mc_sc)) {
	    /* Run it forward some more, so the point is in whichobj. */
	    if (++count > 10) {
		got_normal = FALSE;
		break;
	    }
	    if (whichobj == obj1) {
		wc_to_mc(whichobj, collx + vx*t_int*count,
		    colly + vy*t_int*count, collz + vz*t_int*count,
		    &mcx,&mcy,&mcz);
	    }
	    else {
		wc_to_mc(whichobj, collx - vx*t_int*count,
		    colly - vy*t_int*count, collz - vz*t_int*count,
		    &mcx,&mcy,&mcz);
	    }
	}
	if (got_normal)
	    mc_normal_to_wc_normal(whichobj,mc_sc.mc_normal,wc_normal);
    }

    if (!got_normal) {
	float whichobj_mcnormal[3];

	/* Just guess by where the collision point is. */
	get_box_mc_normal(whichobj,mcx,mcy,mcz,whichobj_mcnormal);
	mc_normal_to_wc_normal(whichobj,whichobj_mcnormal,wc_normal);
    }
}
