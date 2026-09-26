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
#include <stdlib.h>
#include <string.h>
#include "prims.ih"

#define MALLOC_ERRMSG 		"%s: Cannot malloc space.\n"
#define EMPTY_STACK_ERRMSG	"%s: Stack is empty.\n"

typedef struct {
    float *knots;
    int   size;
} KVECTOR;

typedef struct _knode {
    KVECTOR      vector;
    struct _knode *next;
} KNODE;

static KNODE u_stack_default = { { NULL,0 }, NULL };
static KNODE v_stack_default = { { NULL,0 }, NULL };
static KNODE *current_u_knot = &u_stack_default;
static KNODE *current_v_knot = &v_stack_default;



static void free_knode(
    KNODE *ptr)
{
    if (ptr->vector.size > 0)
	if (ptr->vector.knots != NULL)
	    free((void *) (ptr->vector.knots));
    free((void *) ptr);
}
	

static KNODE *free_knotstack(
    KNODE *stack)
{
    KNODE *next;

    /* don't delete the bottom of the stack */
    if (stack->next == NULL) return(stack);

    next = stack->next;
    free_knode(stack);
    return(free_knotstack(next));
}


void _prim_default_knots(
    int fildes)
{
    current_u_knot = free_knotstack(current_u_knot);
    current_v_knot = free_knotstack(current_v_knot);

    current_u_knot->vector.knots = NULL;
    current_u_knot->vector.size  = 0;
    current_u_knot->next         = NULL;

    current_v_knot->vector.knots = NULL;
    current_v_knot->vector.size  = 0;
    current_v_knot->next         = NULL;

    default_knots(fildes);
}


void _prim_u_knot_vector(
    int fildes,
    float *knot_vector,
    int numpts)
{
    unsigned int size = numpts*sizeof(float);

    current_u_knot = free_knotstack(current_u_knot);

    current_u_knot->vector.size  = numpts;
    current_u_knot->next         = NULL;
    if ((current_u_knot->vector.knots = (float *) malloc(size)) == NULL) {
	fprintf(stderr,MALLOC_ERRMSG,"prim_u_knot_vector");
	return;
    }
    memcpy((void *) (current_u_knot->vector.knots),(void *) knot_vector,size);

    u_knot_vector(fildes,knot_vector,numpts);
}


void _prim_v_knot_vector(
    int fildes,
    float *knot_vector,
    int numpts)
{
    unsigned int size = numpts*sizeof(float);

    current_v_knot = free_knotstack(current_v_knot);

    current_v_knot->vector.size  = numpts;
    current_v_knot->next         = NULL;
    if ((current_v_knot->vector.knots = (float *) malloc(size)) == NULL) {
	fprintf(stderr,MALLOC_ERRMSG,"prim_v_knot_vector");
	return;
    }
    memcpy((void *) (current_v_knot->vector.knots),(void *) knot_vector,size);

    v_knot_vector(fildes,knot_vector,numpts);
}


static void default_u(
    int fildes)
{
    default_knots(fildes);

    /* if v knots are currently not default, push them */
    if (current_v_knot->vector.size)
	v_knot_vector(fildes,current_v_knot->vector.knots,
	    current_v_knot->vector.size);
}

static void default_v(
    int fildes)
{

    default_knots(fildes);

    /* if u knots are currently not default, push them */
    if (current_u_knot->vector.size)
	u_knot_vector(fildes,current_u_knot->vector.knots,
	    current_u_knot->vector.size);
}


void _prim_push_default_u(
    int fildes)
{
    KNODE *uknot;

    if ((uknot = (KNODE *) malloc(sizeof(KNODE))) == NULL) {
	fprintf(stderr,MALLOC_ERRMSG,"push_default_u");
	return;
    }

    uknot->vector.size = 0;
    uknot->next        = current_u_knot;
    current_u_knot     = uknot;

    default_u(fildes);
}

void _prim_push_default_v(
    int fildes)
{
    KNODE *vknot;

    if ((vknot = (KNODE *) malloc(sizeof(KNODE))) == NULL) {
	fprintf(stderr,MALLOC_ERRMSG,"push_default_v");
	return;
    }

    vknot->vector.size = 0;
    vknot->next        = current_v_knot;
    current_v_knot     = vknot;

    default_v(fildes);
}

void _prim_push_u_knot(
    int fildes,
    float *knot_vector,
    int numpts)
{
    KNODE *uknot;
    unsigned int size = numpts*sizeof(float);

    if ((uknot = (KNODE *) malloc(sizeof(KNODE))) == NULL) {
	fprintf(stderr,MALLOC_ERRMSG,"_prim_push_u_knot");
	return;
    }

    uknot->vector.size = numpts;
    uknot->next        = current_u_knot;
    current_u_knot     = uknot;
    if ((uknot->vector.knots = (float *) malloc(size)) == NULL) {
	    fprintf(stderr,MALLOC_ERRMSG,"_prim_push_u_knot");
	    free((void *) uknot);
	    return;
    }
    memcpy((void *) (uknot->vector.knots),(void *) knot_vector,size);

    u_knot_vector(fildes,knot_vector,numpts);
}

void _prim_push_v_knot(
    int fildes,
    float *knot_vector,
    int numpts)
{
    KNODE *vknot;
    unsigned int size = numpts*sizeof(float);

    if ((vknot = (KNODE *) malloc(sizeof(KNODE))) == NULL) {
	fprintf(stderr,MALLOC_ERRMSG,"_prim_push_v_knot");
	return;
    }

    vknot->vector.size = numpts;
    vknot->next        = current_v_knot;
    current_v_knot     = vknot;
    if ((vknot->vector.knots = (float *) malloc(size)) == NULL) {
	    fprintf(stderr,MALLOC_ERRMSG,"_prim_push_v_knot");
	    free((void *) vknot);
	    return;
    }
    memcpy((void *) (vknot->vector.knots),(void *) knot_vector,size);

    v_knot_vector(fildes,knot_vector,numpts);
}


void _prim_pop_u_knot(
    int fildes)
{
    KNODE *oldknot;

    if (current_u_knot->next == NULL) {
	fprintf(stderr,EMPTY_STACK_ERRMSG,"_prim_pop_u_knot");
	return;
    }

    oldknot = current_u_knot;
    current_u_knot = oldknot->next;

    free_knode(oldknot);

    if (current_u_knot->vector.size == 0) default_u(fildes);
    else u_knot_vector(fildes,current_u_knot->vector.knots,
	    current_u_knot->vector.size);
}

void _prim_pop_v_knot(
    int fildes)
{
    KNODE *oldknot;

    if (current_v_knot->next == NULL) {
	fprintf(stderr,EMPTY_STACK_ERRMSG,"_prim_pop_v_knot");
	return;
    }

    oldknot = current_v_knot;
    current_v_knot = oldknot->next;

    free_knode(oldknot);

    if (current_v_knot->vector.size == 0) default_v(fildes);
    else v_knot_vector(fildes,current_v_knot->vector.knots,
	    current_v_knot->vector.size);
}


void _prim_pop_knots(
    int fildes)
{
    _prim_pop_u_knot(fildes);
    _prim_pop_v_knot(fildes);
}
