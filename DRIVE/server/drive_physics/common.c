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
#include "libnum.h"
#include "physics.h"
#include "object.h"
#include "demo_physics.h"
#include "scene.h"
#include "local_physics.h"
#include "obj_common.h"


#ifdef USE_REAL_SUN_DIRECTION
# define SHADOW_SUN_DIRECTION_X	SUN_DIRECTION_X
# define SHADOW_SUN_DIRECTION_Y	SUN_DIRECTION_Y
# define SHADOW_SUN_DIRECTION_Z	SUN_DIRECTION_Z
#else
# define SHADOW_SUN_DIRECTION_X	0.0
# define SHADOW_SUN_DIRECTION_Y	1.0
# define SHADOW_SUN_DIRECTION_Z	0.0
#endif


float _crtmp;
float invsquareroot[1024];
int
    lower_point[4],	/* the lower four points of the object */
    contact_points=0,	/* number points nearly touching ground */
    class,		/* see OBJECT_ON_* macros below */
    travelling_backwards;
WC_SURFACE_CHARACTERISTICS
    surchar[4];		/* the surface below those points */
float
    ave_friction;	/* average surface friction factor */
float
    ave_nx,ave_ny,ave_nz;	/* average surface normal */
float
    upper_nx,upper_ny,upper_nz;	/* normal of the side now on top */
float
    COG_wc[3],
    bbox_wc[8][3];
static float
    sceneclipped_bbox_wc[8][3];

#define SHADOW_FLOAT	0.25

/* A table of random numbers between -1.0 and 1.0 for use in object roughness */
#define PRIME1	4409
#define PRIME2	2741
#define PRIME3	541
static float rand_rough[PRIME3];
#define ROUGHNESS_DELTA(x,z,roughness,freq)				       \
    ((roughness) *							       \
    rand_rough[((int)(ABS(x)/(freq))*PRIME1+(int)(ABS(z)/(freq))*PRIME2)       \
	% PRIME3])

/* A table of cube roots for fast work */
float unitcuberoot[1024];




/* Transform the physics bounding box and COG of the object into
 * world coordinates.
 */
void transform_object(
    DRIVE_OBJECT *obj)
{
    int p,d;
    VEHICLE_AUXDATA *vaux = obj->vehicle_auxdata;
    SCENE *myscene = (SCENE *) obj->scene;
    float minx = (myscene->xscene * SCENE_SIZE) - (SCENE_SIZE/2.0 - EPSILON);
    float minz = (myscene->zscene * SCENE_SIZE) - (SCENE_SIZE/2.0 - EPSILON);
    float maxx = minx + (SCENE_SIZE-EPSILON*2.0);
    float maxz = minz + (SCENE_SIZE-EPSILON*2.0);

    for (p=0; p<8; ++p) {
	for (d=0; d<3; ++d) {
	    bbox_wc[p][d] =
		vaux->bbox_mc[p][0] * obj->xform[0][d] + 
		vaux->bbox_mc[p][1] * obj->xform[1][d] + 
		vaux->bbox_mc[p][2] * obj->xform[2][d] + 
		obj->xform[3][d];
	}
	if      (bbox_wc[p][0] < minx) sceneclipped_bbox_wc[p][0] = minx;
	else if (bbox_wc[p][0] > maxx) sceneclipped_bbox_wc[p][0] = maxx;
	else sceneclipped_bbox_wc[p][0] = bbox_wc[p][0];
	sceneclipped_bbox_wc[p][1] = bbox_wc[p][1];
	if      (bbox_wc[p][2] < minz) sceneclipped_bbox_wc[p][2] = minz;
	else if (bbox_wc[p][2] > maxz) sceneclipped_bbox_wc[p][2] = maxz;
	else sceneclipped_bbox_wc[p][2] = bbox_wc[p][2];
    }

    for (d=0; d<3; ++d) {
	COG_wc[d] =
	    vaux->COG_mc[0] * obj->xform[0][d] + 
	    vaux->COG_mc[1] * obj->xform[1][d] + 
	    vaux->COG_mc[2] * obj->xform[2][d] + 
	    obj->xform[3][d];
    }
}


void update_xforms(
    DRIVE_OBJECT *obj,
    PHYSICAL_OBJECT *pobj)
{
    obj->xform[0][0] = pobj->R[XD][XD];
    obj->xform[0][1] = pobj->R[XD][YD];
    obj->xform[0][2] = pobj->R[XD][ZD];
    obj->xform[1][0] = pobj->R[YD][XD];
    obj->xform[1][1] = pobj->R[YD][YD];
    obj->xform[1][2] = pobj->R[YD][ZD];
    obj->xform[2][0] = pobj->R[ZD][XD];
    obj->xform[2][1] = pobj->R[ZD][YD];
    obj->xform[2][2] = pobj->R[ZD][ZD];
    obj->xform[3][0] = pobj->x[XD];
    obj->xform[3][1] = pobj->x[YD];
    obj->xform[3][2] = pobj->x[ZD];

    _hp_invert(obj->xform,obj->ixform,0);

    /* Transpose rotation matrix */
    obj->ixform[0][0] = pobj->R[XD][XD];
    obj->ixform[0][1] = pobj->R[YD][XD];
    obj->ixform[0][2] = pobj->R[ZD][XD];
    obj->ixform[1][0] = pobj->R[XD][YD];
    obj->ixform[1][1] = pobj->R[YD][YD];
    obj->ixform[1][2] = pobj->R[ZD][YD];
    obj->ixform[2][0] = pobj->R[XD][ZD];
    obj->ixform[2][1] = pobj->R[YD][ZD];
    obj->ixform[2][2] = pobj->R[ZD][ZD];
    /* And offset */
    obj->ixform[3][0] =
	-pobj->x[XD] * obj->ixform[0][0]
	-pobj->x[YD] * obj->ixform[1][0]
	-pobj->x[ZD] * obj->ixform[2][0];
    obj->ixform[3][1] =
	-pobj->x[XD] * obj->ixform[0][1]
	-pobj->x[YD] * obj->ixform[1][1]
	-pobj->x[ZD] * obj->ixform[2][1];
    obj->ixform[3][2] =
	-pobj->x[XD] * obj->ixform[0][2]
	-pobj->x[YD] * obj->ixform[1][2]
	-pobj->x[ZD] * obj->ixform[2][2];
}


static void create_temporary_physics_object(
    DRIVE_OBJECT *obj,
    float mass)
{
    PHYSICAL_OBJECT *pobj;
    VEHICLE_AUXDATA *vaux;
    int i,j;


    pobj = allocate_physical_object();
    obj->pobj = pobj;
    pobj->mass = mass;
    pobj->I[XD][XD] = pobj->I[YD][YD] = pobj->I[ZD][ZD] = mass * (1e+6);
    pobj->I[XD][YD] = pobj->I[XD][ZD] = pobj->I[YD][XD] =
	    pobj->I[YD][ZD] = pobj->I[ZD][XD] = pobj->I[ZD][YD] = 0.0;
    pobj->Iinv[XD][XD] = pobj->Iinv[YD][YD] = pobj->Iinv[ZD][ZD] = 1.0/pobj->I[XD][XD];
    pobj->Iinv[XD][YD] = pobj->Iinv[XD][ZD] = pobj->Iinv[YD][XD] =
	    pobj->Iinv[YD][ZD] = pobj->Iinv[ZD][XD] = pobj->Iinv[ZD][YD] = 0.0;
    pobj->x[XD] = obj->xform[3][0];
    pobj->x[XD] = obj->xform[3][1];
    pobj->x[XD] = obj->xform[3][2];
    pobj->Q->s = 1.0;
    pobj->Q->r[XD] = pobj->Q->r[YD] = pobj->Q->r[ZD] = 0.0;
    pobj->p[XD] = pobj->p[YD] = pobj->p[ZD] = 0.0;
    pobj->w[XD] = pobj->w[YD] = pobj->w[ZD] = 0.0;
    pobj->v[XD] = pobj->v[YD] = pobj->v[ZD] = 0.0;
    pobj->L[XD] = pobj->L[YD] = pobj->L[ZD] = 0.0;
    for (i=0; i<3; ++i) {
	for (j=0; j<3; ++j) {
	    pobj->R[i+LIBNUM_XINDEX][j+LIBNUM_XINDEX] =
	        obj->xform[i][j];
#if PHYSICS_VERSION >= 2
	    pobj->Rinv[i+LIBNUM_XINDEX][j+LIBNUM_XINDEX] = 
	        obj->ixform[i][j];
#else
	    pobj->Ri[i+LIBNUM_XINDEX][j+LIBNUM_XINDEX] =
	        obj->ixform[i][j];
#endif
	}
    }

    update_wc_bounds(obj);

    if ((vaux = (VEHICLE_AUXDATA *) malloc(sizeof(VEHICLE_AUXDATA))) == NULL) {
	fprintf(stderr,"Out of malloc space!\n");
	exit(1);
    }
    obj->vehicle_auxdata = vaux;
    vaux->COG_mc[0] = (obj->bound_mc[0] + obj->bound_mc[3])/2.0;
    vaux->COG_mc[1] = (obj->bound_mc[1] + obj->bound_mc[4])/2.0;
    vaux->COG_mc[2] = (obj->bound_mc[2] + obj->bound_mc[5])/2.0;

    vaux->bbox_mc[0][0] = obj->bound_mc[0];
    vaux->bbox_mc[0][1] = obj->bound_mc[1];
    vaux->bbox_mc[0][2] = obj->bound_mc[5];

    vaux->bbox_mc[1][0] = obj->bound_mc[3];
    vaux->bbox_mc[1][1] = obj->bound_mc[1];
    vaux->bbox_mc[1][2] = obj->bound_mc[5];

    vaux->bbox_mc[2][0] = obj->bound_mc[3];
    vaux->bbox_mc[2][1] = obj->bound_mc[1];
    vaux->bbox_mc[2][2] = obj->bound_mc[2];

    vaux->bbox_mc[3][0] = obj->bound_mc[0];
    vaux->bbox_mc[3][1] = obj->bound_mc[1];
    vaux->bbox_mc[3][2] = obj->bound_mc[2];

    vaux->bbox_mc[4][0] = obj->bound_mc[0];
    vaux->bbox_mc[4][1] = obj->bound_mc[4];
    vaux->bbox_mc[4][2] = obj->bound_mc[5];

    vaux->bbox_mc[5][0] = obj->bound_mc[3];
    vaux->bbox_mc[5][1] = obj->bound_mc[4];
    vaux->bbox_mc[5][2] = obj->bound_mc[5];

    vaux->bbox_mc[6][0] = obj->bound_mc[3];
    vaux->bbox_mc[6][1] = obj->bound_mc[4];
    vaux->bbox_mc[6][2] = obj->bound_mc[2];

    vaux->bbox_mc[7][0] = obj->bound_mc[0];
    vaux->bbox_mc[7][1] = obj->bound_mc[4];
    vaux->bbox_mc[7][2] = obj->bound_mc[2];
}


void do_collision(
    DRIVE_OBJECT *obj, DRIVE_OBJECT *hit_obj,
    float t_interval)
{
    PHYSICAL_OBJECT *pobj = obj->pobj, *phit_obj = hit_obj->pobj;
    float c1[3+LIBNUM_XINDEX],c2[3+LIBNUM_XINDEX],
       icoll[3+LIBNUM_XINDEX],jcoll[3+LIBNUM_XINDEX];
    int allocated_object = FALSE;
    int collide;
    float t_int,t;
    float en1,en2,en1b,en2b;
    float vx,vy,vz;
    float v1x,v1y,v1z;
    float v2x,v2y,v2z;
    float avevx,avevy,avevz;
    float vxz;
    float collx,colly,collz;
    float COG_wc_hit[3];

    /* Ran into something!!! */
    if (phit_obj == NULL) {
	/* need to create an object temporarily */
	create_temporary_physics_object(hit_obj,pobj->mass*(1e+6));
	phit_obj = hit_obj->pobj;
	allocated_object = TRUE;
    }

    COG_wc_hit[0] = (hit_obj->bound_wc[0] + hit_obj->bound_wc[3]) / 2.0;
    COG_wc_hit[1] = (hit_obj->bound_wc[1] + hit_obj->bound_wc[4]) / 2.0;
    COG_wc_hit[2] = (hit_obj->bound_wc[2] + hit_obj->bound_wc[5]) / 2.0;

    v1x = pobj->v[XD];     v1y = pobj->v[YD];     v1z = pobj->v[ZD];
    v2x = phit_obj->v[XD]; v2y = phit_obj->v[YD]; v2z = phit_obj->v[ZD];

    find_collision_point(obj,hit_obj,t_interval,
	&v1x,&v1y,&v1z,
	&v2x,&v2y,&v2z,
	&collx,&colly,&collz,
	&t_int);
    avevx = (v1x+v2x)/2.0; avevy = (v1y+v2y)/2.0; avevz = (v1z+v2z)/2.0;
    vx = v1x - v2x; vy = v1y - v2y; vz = v1z - v2z;

    if (t_int == 0.0) t_int = -t_interval/10.0;
    collide = (t_int < 0.0);

    /* Move the objects backwards to the collision point */
    obj->xform[3][0]  = (pobj->x[XD]  += t_int*v1x);
    obj->xform[3][1]  = (pobj->x[YD]  += t_int*v1y);
    obj->xform[3][2]  = (pobj->x[ZD]  += t_int*v1z);
    if (!allocated_object) {
	hit_obj->xform[3][0] = (phit_obj->x[XD] += t_int*v2x);
	hit_obj->xform[3][1] = (phit_obj->x[YD] += t_int*v2y);
	hit_obj->xform[3][2] = (phit_obj->x[ZD] += t_int*v2z);
    }

    if (collide) {
	float wc_normal[3];

	/* Update the inverse transforms and bounding boxes since they might
	 * be used by the normal calculating routines.
	 */
	_hp_invert(obj->xform,obj->ixform,0);
	transform_object(obj);
	update_wc_bounds(obj);
	if (!allocated_object) {
	    _hp_invert(hit_obj->xform,hit_obj->ixform,0);
	    update_wc_bounds(hit_obj);
	}

	/* moving into object */
	/* c1 and c2 give the vector from the COG to the collision point */
	c1[XD] = collx - COG_wc[0];
	c1[YD] = colly - COG_wc[1];
	c1[ZD] = collz - COG_wc[2];
	c2[XD] = collx - COG_wc_hit[0];
	c2[YD] = colly - COG_wc_hit[1];
	c2[ZD] = collz - COG_wc_hit[2];

#ifdef COG_Y_COLLISIONS
	/* SIMPLIFICATION -- Assume collision occurs at COG y */
	c1[YD] = c2[YD] = 0.0;
#endif /* COG_Y_COLLISIONS */

	/* icoll & jcoll define plane of collision.
	 * They should be in the plane of the penetrated face of hit_obj,
	 * so that icoll X jcoll = normal to the plane.
	 */
	find_collision_normal(obj,hit_obj,vx,vy,vz,collx,colly,collz,
	    t_int,wc_normal);

	if (1.0-wc_normal[1] < EPSILON) {
	    /* Normal is close to Y axis. i=Z, j=X */
	    icoll[XD] = 0.0; icoll[YD] = 0.0; icoll[ZD] = 1.0;
	    jcoll[XD] = 1.0; jcoll[YD] = 0.0; jcoll[ZD] = 0.0;
	}
	else if (1.0+wc_normal[1] < EPSILON) {
	    /* Normal is close to -Y axis. i=X, j=Z */
	    icoll[XD] = 1.0; icoll[YD] = 0.0; icoll[ZD] = 0.0;
	    jcoll[XD] = 0.0; jcoll[YD] = 0.0; jcoll[ZD] = 1.0;
	}
	else {
	    /* Let i be nXY, j is nXi */
	    icoll[XD] = -wc_normal[2];
	    icoll[YD] = 0.0;
	    icoll[ZD] = wc_normal[0];
	    jcoll[XD] = wc_normal[1]*icoll[ZD] - wc_normal[2]*icoll[YD];
	    jcoll[YD] = wc_normal[2]*icoll[XD] - wc_normal[0]*icoll[ZD];
	    jcoll[ZD] = wc_normal[0]*icoll[YD] - wc_normal[1]*icoll[XD];
	}
    }

    /* Move the objects away from the collision point somewhat. */
    if ((vxz = FHYPOT2(vx,vz)) > VELOCITY_EPSILON) {
	if ((t = COLLISION_OFFSET / vxz / 2.0) > t_interval/2.0) {
	    t = t_interval/2.0;
	}
	if (t_int < 0) t = -t;
	/* Use the relative speed between the objects to do this offset. */
	obj->xform[3][0] = (pobj->x[XD] += t*vx);
	obj->xform[3][1] = (pobj->x[YD] += t*vy);
	obj->xform[3][2] = (pobj->x[ZD] += t*vz);
	/* Matrices will be updated and objects transformed below... */
	if (!allocated_object) {
	    hit_obj->xform[3][0] = (phit_obj->x[XD] -= t*vx);
	    hit_obj->xform[3][1] = (phit_obj->x[YD] -= t*vy);
	    hit_obj->xform[3][2] = (phit_obj->x[ZD] -= t*vz);
	    /* Matrices will be updated and objects transformed below... */
	}
    }

    /* We moved the objects to where they collided.  Now put them back
     * to roughly where they were (though now they're seperated)
     * so they don't bounce back and forth.
     */
    obj->xform[3][0]  = (pobj->x[XD]  -= t_int/2.0 * avevx);
    obj->xform[3][1]  = (pobj->x[YD]  -= t_int/2.0 * avevy);
    obj->xform[3][2]  = (pobj->x[ZD]  -= t_int/2.0 * avevz);
    /* Update the inverse transforms and bounding boxes. */
    _hp_invert(obj->xform,obj->ixform,0);
    transform_object(obj);
    update_wc_bounds(obj);

    if (!allocated_object) {
	hit_obj->xform[3][0] = (phit_obj->x[XD] -= t_int/2.0 * avevx);
	hit_obj->xform[3][1] = (phit_obj->x[YD] -= t_int/2.0 * avevy);
	hit_obj->xform[3][2] = (phit_obj->x[ZD] -= t_int/2.0 * avevz);
	_hp_invert(hit_obj->xform,hit_obj->ixform,0);
	update_wc_bounds(hit_obj);
    }

    if (collide) {
	/* moving into object */
#ifdef DEBUG_COLLISIONS
	en1 = energy(pobj,GRAVITY_CONSTANT);
	en2 = energy(phit_obj,GRAVITY_CONSTANT);
#endif /* DEBUG_COLLISIONS */

	solve_collision(pobj,phit_obj,c1,c2,icoll,jcoll,COLLISION_ELASTICITY);
	/* copy updated R and x to xform */
	update_xforms(obj,pobj);
	/* Update the WC bounding box for this car. */
	update_wc_bounds(obj);
	/* Take dot product of velocity and MC Z.  If negative, backwards */
	travelling_backwards = 
	    ( pobj->v[XD]*pobj->R[ZD][XD]
	    + pobj->v[ZD]*pobj->R[ZD][ZD] < 0.0);

	if (!allocated_object) {
	    /* copy updated R and x to xform */
	    update_xforms(hit_obj,phit_obj);
	    /* Update the WC bounding box for this car. */
	    update_wc_bounds(hit_obj);
	}


#ifdef DEBUG_COLLISIONS
	en1b = energy(pobj,GRAVITY_CONSTANT);
	en2b = energy(phit_obj,GRAVITY_CONSTANT);
	if ((ABS(en1b+en2b) > ABS(en1+en2)*1.2) && debug) {
	    printf("Before collision, obj1 energy=%f, hit_obj energy=%f.\n",
		en1,en2);
	    printf("After collision,  obj1 energy=%f, hit_obj energy=%f.\n",
		en1b,en2b);
	}
#endif /* DEBUG_COLLISIONS */
    }

    if (allocated_object) {
	free_physical_object(phit_obj);
	free(hit_obj->vehicle_auxdata);
	hit_obj->pobj = NULL;
	hit_obj->vehicle_auxdata = NULL;
    }
}


/* Make sure the object hasn't fallen through the ground and
 * check for collisions.
 */
void displace_object(
    DRIVE_OBJECT *obj,
    float t_interval,
    int check_collisions)
{
    PHYSICAL_OBJECT *pobj = obj->pobj;
    VEHICLE_AUXDATA *vaux = obj->vehicle_auxdata;
    float underground,u;
    int i,p,did_collision;
    DRIVE_OBJECT *collision_obj = NULL;
    WC_SURFACE_CHARACTERISTICS local_sc;
    MC_SURFACE_CHARACTERISTICS local_mc_sc;
    float intx,inty,intz,t_int;
    float p1x,p1y,p1z;
    float p2x,p2y,p2z;
    float v1x,v1y,v1z;
    float v2x,v2y,v2z;

    /* see if any points are through the ground. */
    underground = -(1e+30);
    for (i=0; i<4; ++i) {
	p = lower_point[i];
	surface_chars(obj,
	    sceneclipped_bbox_wc[p][0],
	    sceneclipped_bbox_wc[p][1],
	    sceneclipped_bbox_wc[p][2],
	    &(surchar[i]));
	if (surchar[i].roughness > 0.0) {
	    surchar[i].wc_y +=
		ROUGHNESS_DELTA(sceneclipped_bbox_wc[p][0],
		    sceneclipped_bbox_wc[p][2],
		    surchar[i].roughness,surchar[i].roughness_frequency);
	}
	if ((u = surchar[i].wc_y - bbox_wc[p][1]) > underground) {
	    underground = u;
	    intx = bbox_wc[p][0];
	    inty = bbox_wc[p][1];
	    intz = bbox_wc[p][2];
	    collision_obj = surchar[i].whichobj;
	}
    }

    did_collision = FALSE;
    if (check_collisions) {
	if ((underground > vaux->max_obstacle_height)
		&& (collision_obj != NULL)
		&& (collision_obj->surface_chars_bbox != NULL)
		&& (collision_obj->collision_routine != NULL)) {
	    (*(collision_obj->collision_routine))(obj,collision_obj,t_interval);
	    did_collision = TRUE;
	}
	else {
	    /* check for collision anywhere on the body */
	    intersect_objects_bbox(obj,&local_sc);
	    if ((collision_obj = local_sc.whichobj) != NULL) {
		if (travelling_backwards) {
		    mc_to_wc(obj,
			obj->bound_mc[0],obj->bound_mc[1],obj->bound_mc[2],
			&p1x,&p1y,&p1z);
		    mc_to_wc(obj,
			obj->bound_mc[3],obj->bound_mc[1],obj->bound_mc[2],
			&p2x,&p2y,&p2z);
		}
		else {
		    mc_to_wc(obj,
			obj->bound_mc[0],obj->bound_mc[1],obj->bound_mc[5],
			&p1x,&p1y,&p1z);
		    mc_to_wc(obj,
			obj->bound_mc[3],obj->bound_mc[1],obj->bound_mc[5],
			&p2x,&p2y,&p2z);
		}
		p1x = (p1x + p2x) / 2.0;
		p1y = (p1y + p2y) / 2.0;
		p1z = (p1z + p2z) / 2.0;
		if (local_sc.wc_y - p1y > vaux->max_obstacle_height) {
		    v1x = pobj->v[XD]; v1y = pobj->v[YD]; v1z = pobj->v[ZD];
		    if (collision_obj->pobj == NULL) {
			v2x = 0.0; v2y = 0.0; v2z = 0.0;
		    }
		    else {
			v2x = collision_obj->pobj->v[XD];
			v2y = collision_obj->pobj->v[YD];
			v2z = collision_obj->pobj->v[ZD];
		    }
		    find_collision_point(obj,collision_obj,t_interval,
			&v1x,&v1y,&v1z,
			&v2x,&v2y,&v2z,
			&intx,&inty,&intz,&t_int);
#define CHECK_T_INT
#ifdef CHECK_T_INT
		    if ((t_int <= 0.0)
			    && (t_int > -t_interval*4.0))
#endif /* CHECK_T_INT */
		    {
			if (collision_obj->surface_chars_xyz != NULL) {
			    /* check the heights of the collision point. */
			    wc_to_mc(collision_obj,intx,inty,intz,
				&p2x,&p2y,&p2z);
			    /* Set defaults */
			    local_mc_sc.mc_x = p2x;
			    local_mc_sc.mc_y = p2y;
			    local_mc_sc.mc_z = p2z;
			    if ((*collision_obj->surface_chars_xyz)(collision_obj,
				    p2x,p2y,p2z,&local_mc_sc)) {
				mc_to_wc(collision_obj, local_mc_sc.mc_x,
				    local_mc_sc.mc_y, local_mc_sc.mc_z,
				    &p1x,&p1y,&p1z);
				if ((p1y - inty > vaux->max_obstacle_height) 
					&& (collision_obj->collision_routine
					    != NULL)) {
				    (*(collision_obj->collision_routine))(obj,
					collision_obj,t_interval);
				    did_collision = TRUE;
				}
			    }
			}
			else if (collision_obj->collision_routine != NULL) {
			    (*(collision_obj->collision_routine))(obj,
				collision_obj,t_interval);
			    did_collision = TRUE;
			}
		    }
		}
	    }
	}
    }

    if ((!did_collision)
	    && (underground > 0.0)) {
	/* displace the object to above ground level */
	pobj->x[YD] += underground;
	obj->xform[3][1] += underground;
	_hp_invert(obj->xform,obj->ixform,0);
	COG_wc[1] += underground;
	for (i=0; i<8; ++i) {
	    bbox_wc[i][1] += underground;
	}
    }

    contact_points = 0;
    ave_friction = ave_nx = ave_ny = ave_nz = 0.0;
    for (i=0; i<4; ++i) {
	if (bbox_wc[lower_point[i]][1] - surchar[i].wc_y < ON_GROUND_EPSILON) {
	    ++contact_points;
	    ave_friction += surchar[i].friction;
	    ave_nx += surchar[i].wc_normal[0];
	    ave_ny += surchar[i].wc_normal[1];
	    ave_nz += surchar[i].wc_normal[2];
	}
    }

    if (contact_points == 0) {
	class = OBJECT_IN_AIR;
    }
    else {
	float k = 1.0 / (float) contact_points;
	ave_friction *= k;
	ave_nx *= k; ave_ny *= k; ave_nz *= k;
    }
}


/* Which side will the object land on? */
void classify_object(
    DRIVE_OBJECT *obj)
{
    PHYSICAL_OBJECT *pobj = obj->pobj;

    /* The up vector points along R[2][1],R[2][2],R[2][3], since
     * in modelling coordinates, it's along the Y axis.
     */
    if (pobj->R[YD][YD] > COS45) {
	class = OBJECT_ON_BOTTOM;
	lower_point[0] = 0;
	lower_point[1] = 1;
	lower_point[2] = 2;
	lower_point[3] = 3;
	upper_nx = pobj->R[YD][XD];
	upper_ny = pobj->R[YD][YD];
	upper_nz = pobj->R[YD][ZD];
    }
    else if (pobj->R[YD][YD] < -COS45) {
	class = OBJECT_ON_TOP;
	lower_point[0] = 4;
	lower_point[1] = 5;
	lower_point[2] = 6;
	lower_point[3] = 7;
	upper_nx = -pobj->R[YD][XD];
	upper_ny = -pobj->R[YD][YD];
	upper_nz = -pobj->R[YD][ZD];
    }
    /* The front of the car is along the -Z axis. */
    else if (pobj->R[ZD][YD] < -COS45) {
	class = OBJECT_ON_FRONT;
	lower_point[0] = 0;
	lower_point[1] = 1;
	lower_point[2] = 4;
	lower_point[3] = 5;
	upper_nx = pobj->R[ZD][XD];
	upper_ny = pobj->R[ZD][YD];
	upper_nz = pobj->R[ZD][ZD];
    }
    else if (pobj->R[ZD][YD] > COS45) {
	class = OBJECT_ON_REAR;
	lower_point[0] = 2;
	lower_point[1] = 3;
	lower_point[2] = 6;
	lower_point[3] = 7;
	upper_nx = -pobj->R[ZD][XD];
	upper_ny = -pobj->R[ZD][YD];
	upper_nz = -pobj->R[ZD][ZD];
    }
    /* The right side is along -X axis. */
    else if (pobj->R[XD][YD] < -COS45) {
	class = OBJECT_ON_RIGHT;
	lower_point[0] = 1;
	lower_point[1] = 2;
	lower_point[2] = 5;
	lower_point[3] = 6;
	upper_nx = pobj->R[XD][XD];
	upper_ny = pobj->R[XD][YD];
	upper_nz = pobj->R[XD][ZD];
    }
    else {
	class = OBJECT_ON_LEFT;
	lower_point[0] = 0;
	lower_point[1] = 3;
	lower_point[2] = 4;
	lower_point[3] = 7;
	upper_nx = -pobj->R[XD][XD];
	upper_ny = -pobj->R[XD][YD];
	upper_nz = -pobj->R[XD][ZD];
    }
}


void init_physics(
    void)
{
    int i;

    for (i=0; i<PRIME3; ++i) {
	rand_rough[i] = BOUNDED_FLOATRAND(-1.0,1.0);
    }

    for (i=0; i<UNITCUBEROOT_SIZE; ++i) {
	unitcuberoot[i] =
	    FPOW((double)i/((double) UNITCUBEROOT_SIZE-1), (1.0/3.0));
    }

    invsquareroot[0] = 1.0;
    for (i=1; i<INVSQUAREROOT_SIZE; ++i) {
	invsquareroot[i] = 1.0/FSQRT(i);
    }
}


void update_shadow_matrix(
    DRIVE_OBJECT *obj,
    WC_SURFACE_CHARACTERISTICS surface[4])
{
    float xc,yc,zc;
    float x1,y1,z1;
    float x2,y2,z2;
    float xn,yn,zn,iyn;
    DRIVE_OBJECT *child;
    float projmat[4][4];

    /* Check each of my children, and call this on them. */
    child = obj->child_list;
    while (child != NULL) {
	if (child->idptr->number == SHADOW_OBJECT) {
	    /* Figure the centerpoint */
	    xc = (surface[0].wc_x + surface[1].wc_x
		+ surface[2].wc_x + surface[3].wc_x) / 4.0;
	    yc = (surface[0].wc_y + surface[1].wc_y
		+ surface[2].wc_y + surface[3].wc_y) / 4.0;
	    zc = (surface[0].wc_z + surface[1].wc_z
		+ surface[2].wc_z + surface[3].wc_z) / 4.0;

	    /* Lift the shadow slightly off the ground */
	    yc += SHADOW_FLOAT;

	    /* Use diagonals to figure normal */
	    x1 = surface[0].wc_x - surface[2].wc_x;
	    y1 = surface[0].wc_y - surface[2].wc_y;
	    z1 = surface[0].wc_z - surface[2].wc_z;

	    x2 = surface[1].wc_x - surface[3].wc_x;
	    y2 = surface[1].wc_y - surface[3].wc_y;
	    z2 = surface[1].wc_z - surface[3].wc_z;

	    xn = y1*z2 - y2*z1;
	    yn = x2*z1 - x1*z2;
	    zn = x1*y2 - x2*y1;
	    NORMALIZE3(xn,yn,zn);

	    if (ABS(yn) < 0.5) {
		xn = 0.0;
		yn = 1.0;
		zn = 0.0;
	    }

	    iyn = 1.0/yn;
	    projmat[0][0] = 1.0;
	    projmat[1][0] = -SHADOW_SUN_DIRECTION_X/SHADOW_SUN_DIRECTION_Y;
	    projmat[2][0] = 0.0;
	    projmat[3][0] = yc*SHADOW_SUN_DIRECTION_X/SHADOW_SUN_DIRECTION_Y;

	    projmat[0][1] = -xn*iyn;
	    projmat[1][1] = EPSILON;
	    projmat[2][1] = -zn*iyn;
	    projmat[3][1] = (xc*xn + zc*zn)*iyn + yc;

	    projmat[0][2] = 0.0;
	    projmat[1][2] = -SHADOW_SUN_DIRECTION_Z/SHADOW_SUN_DIRECTION_Y;
	    projmat[2][2] = 1.0;
	    projmat[3][2] = yc*SHADOW_SUN_DIRECTION_Z/SHADOW_SUN_DIRECTION_Y;

	    projmat[0][3] = 0.0;
	    projmat[1][3] = 0.0;
	    projmat[2][3] = 0.0;
	    projmat[3][3] = 1.0;

	    concat_matrix(obj->xform,projmat,child->xform);
	    /* _hp_invert(child->xform,child->ixform,0); */
	}
	else {
	    update_shadow_matrix(child,surface);
	}
	child = child->next;
    }
}


void reset_rotation_matrices(
    DRIVE_OBJECT *obj,
    float yangle)		/* radians -- rotation around Y axis */
{
    PHYSICAL_OBJECT *pobj = obj->pobj;
    float angle2;

    angle2 = yangle;
    while (angle2 < -M_PI) angle2 += (2.0*M_PI);
    while (angle2 >  M_PI) angle2 -= (2.0*M_PI);
    angle2 /= 2.0;

    pobj->Q->s    = FCOS(angle2);
    pobj->Q->r[XD] = 0.0;
    pobj->Q->r[YD] = -FSIN(angle2);
    pobj->Q->r[ZD] = 0.0;
    quaternion_to_matrix(pobj->Q,pobj->R);
#if PHYSICS_VERSION >= 2
    invert_matrix(pobj->R,pobj->Rinv,3);
#else
    invert_matrix(pobj->R,pobj->Ri,3);
#endif
}


void reset_vehicle_physics(
    DRIVE_OBJECT *obj,
    float x, float y, float z,	/* new center */
    float yangle)		/* radians -- rotation around Y axis */
{
    PHYSICAL_OBJECT *pobj = obj->pobj;
    float sin_t = FSIN(yangle),
	  cos_t = FCOS(yangle);

    _hp_identity(obj->xform);
    obj->xform[0][0] = obj->xform[2][2] = cos_t;
    obj->xform[0][2] = sin_t;
    obj->xform[2][0] = -sin_t;

    pobj->x[XD] = obj->xform[3][0] = x;
    pobj->x[YD] = obj->xform[3][1] = y;
    pobj->x[ZD] = obj->xform[3][2] = z;

    ZERO_TRIVECTOR(pobj->p);
    ZERO_TRIVECTOR(pobj->v);
    ZERO_TRIVECTOR(pobj->w);
    ZERO_TRIVECTOR(pobj->L);

    reset_rotation_matrices(obj,yangle);

    /* Now, update the object's notion as to where it is */
    obj->scene = (void *) check_scenes(obj,pobj->x[XD],pobj->x[ZD]);
}


boolean_type set_vehicle_upright(
    DRIVE_OBJECT *obj)
{
    int i,p;
    float maxy,dirx,dirz,angle;


    transform_object(obj);
    classify_object(obj);

    /* Ignore if unnecessary */
    if ((class == OBJECT_ON_BOTTOM)
	    || (class == OBJECT_IN_AIR)) {
	return(FALSE);
    }

    /* Find the highest point */
    maxy = -1e+20;
    for (i=0; i<4; ++i) {
	p = lower_point[i];
	surface_chars(obj,
	    sceneclipped_bbox_wc[p][0],
	    sceneclipped_bbox_wc[p][1],
	    sceneclipped_bbox_wc[p][2],
	    &(surchar[i]));
	if (surchar[i].wc_y > maxy) maxy = surchar[i].wc_y;
    }

    /* Get the angle the front is pointing */
    dirx = obj->xform[2][0]; dirz = obj->xform[2][2];
    NORMALIZE2(dirx,dirz);
    if ((ABS(dirx) < EPSILON) && (ABS(dirz) < EPSILON)) dirz = 1.0;
    angle = FATAN2(dirz,dirx) - (M_PI/2.0);

    reset_vehicle_physics(obj,
	obj->xform[3][0], maxy + 10.0, obj->xform[3][2],
	angle);

    return(TRUE);
}


/* Do air drag, to keep it from accelerating forever. */
void do_air_drag(
    DRIVE_OBJECT *obj,
    VECTOR Ftot, VECTOR Ttot)
{
    PHYSICAL_OBJECT *pobj = obj->pobj;
    VEHICLE_AUXDATA *vaux = obj->vehicle_auxdata;
    float velocity, drag, r;

    velocity = LENGTH_TRIVECTOR(pobj->v);
    if (velocity > VELOCITY_EPSILON) {
	if (travelling_backwards) 
	    r = MAX(surchar[PT_UBR].roughness,surchar[PT_UBL].roughness);
	else 
	    r = MAX(surchar[PT_UFR].roughness,surchar[PT_UFL].roughness);
	drag = vaux->coefficient_of_drag * velocity * velocity * DRAG_MULT
	    + OFFROAD_FACTOR(vaux,r)
	    + INHERENT_DRAG;
	if (debug) {
		printf("%f     \r",drag);
		fflush(stdout);
	}

	/* drag in opposite direction of velocity */
	drag /= -velocity;
	/* cut it if no road friction */
	if (class == OBJECT_IN_AIR) drag /= 2.0;
	ADDTO_TRIVECTOR_MULT(Ftot, pobj->v, drag);
    }

    /*%%%% could be done for Ttot too %%%%%%%*/
}
