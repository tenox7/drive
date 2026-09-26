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
#include "libnum.h"
#include "physics.h"
#include "physics.ih"

#define OLD_METHOD 1

void free_physical_object(
    PHYSICAL_OBJECT *obj)
{
    if (obj == NULL) return;

    if (obj->Rinv != NULL) FREE_MATRIX(obj->Rinv,3,3);
    if (obj->R    != NULL) FREE_MATRIX(obj->R,3,3);
    if (obj->w    != NULL) FREE_VECTOR(obj->w,3);
    if (obj->v    != NULL) FREE_VECTOR(obj->v,3);
    if (obj->Q    != NULL) free_quaternion(obj->Q);
    if (obj->L    != NULL) FREE_VECTOR(obj->L,3);
    if (obj->p    != NULL) FREE_VECTOR(obj->p,3);
    if (obj->x    != NULL) FREE_VECTOR(obj->x,3);
    if (obj->Iinv != NULL) FREE_MATRIX(obj->Iinv,3,3);
    if (obj->I    != NULL) FREE_MATRIX(obj->I,3,3);
    free((void *) obj);
}


PHYSICAL_OBJECT *allocate_physical_object(
    void)
{
    PHYSICAL_OBJECT *obj;
    
    if ((obj = (PHYSICAL_OBJECT *) malloc(sizeof(PHYSICAL_OBJECT))) == NULL) {
	nrerror("allocate_physical_object:  out of space");
	return(NULL);
    }
    memset((void *) obj,0,sizeof(obj));
    if ((obj->I = ALLOCATE_MATRIX(3,3)) == NULL) {
	free_physical_object(obj);
	return(NULL);
    }
    if ((obj->Iinv = ALLOCATE_MATRIX(3,3)) == NULL) {
	free_physical_object(obj);
	return(NULL);
    }

    if ((obj->x = ALLOCATE_VECTOR(3)) == NULL) {
        free_physical_object(obj);
	return(NULL);
    }
    if ((obj->Q = allocate_quaternion()) == NULL) {
	free_physical_object(obj);
	return(NULL);
    }
    if ((obj->p = ALLOCATE_VECTOR(3)) == NULL) {
        free_physical_object(obj);
	return(NULL);
    }
    if ((obj->w = ALLOCATE_VECTOR(3)) == NULL) {
        free_physical_object(obj);
	return(NULL);
    }

    if ((obj->v = ALLOCATE_VECTOR(3)) == NULL) {
        free_physical_object(obj);
	return(NULL);
    }
    if ((obj->R = ALLOCATE_MATRIX(3,3)) == NULL) {
	free_physical_object(obj);
	return(NULL);
    }
    if ((obj->Rinv = ALLOCATE_MATRIX(3,3)) == NULL) {
	free_physical_object(obj);
	return(NULL);
    }
    if ((obj->L = ALLOCATE_VECTOR(3)) == NULL) {
        free_physical_object(obj);
	return(NULL);
    }

    return(obj);
}


void update_angular_momentum(
    PHYSICAL_OBJECT *obj)
{
    static MATRIX Iwc = NULL, Mtmp = NULL;
    /* Assume they'll be used again, so don't deallocate them. */

    if (Iwc == NULL) {
	if ((Iwc = ALLOCATE_MATRIX(3,3)) == NULL) {
	    return;
	}
    }
    if (Mtmp == NULL) {
	if ((Mtmp = ALLOCATE_MATRIX(3,3)) == NULL) {
	    FREE_MATRIX(Iwc,3,3);
	    Iwc = NULL;
	    return;
	}
    }

    /* L = Iw */
    /* first convert Iinv to world coordinates */
#ifdef OLD_METHOD
    MATxDIAG(obj->Rinv,obj->I,Mtmp,3,3);
    MATxMAT(Mtmp,obj->R,Iwc,3,3);
#else
    MATxDIAG(obj->R,obj->I,Mtmp,3,3);
    MATxMAT(Mtmp,obj->Rinv,Iwc,3,3);
#endif
    MATxVEC(Iwc,obj->w,obj->L,3,3);
}
