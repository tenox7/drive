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
#include "prims.ih"


typedef struct {
    int coord,use,rgb,normals,order;
} VNODE;

#define VF_STACK_SIZE	64
static VNODE vf_default[VF_STACK_SIZE] = {
    { 0,0,0,FALSE,COUNTER_CLOCKWISE },
};
static VNODE *current_vf = vf_default;

	
void _prim_vertex_format(
    int fildes,
    int coord,
    int use,
    int rgb,
    int normals,
    int order)
{
    current_vf = vf_default;
    current_vf->coord   = coord;
    current_vf->use     = use;
    current_vf->rgb     = rgb;
    current_vf->normals = normals;
    current_vf->order   = order;

    vertex_format(fildes,coord,use,rgb,normals,order);
}


void _prim_push_vertex_format(
    int fildes,
    int coord,
    int use,
    int rgb,
    int normals,
    int order)
{
    VNODE *last_vf;

    if (current_vf == (&vf_default[VF_STACK_SIZE])) {
	fprintf(stderr,"Out of VF space in push_vertex_format.\n");
    }
    /* else */

    last_vf = current_vf;
    ++current_vf;

    if (order == TOGGLE_ORDER) {
	if (last_vf->order == CLOCKWISE) order = COUNTER_CLOCKWISE;
	else order = CLOCKWISE;
    }
    else if (order == SAME_ORDER) {
	order = last_vf->order;
    }

    current_vf->coord   = coord;
    current_vf->use     = use;
    current_vf->rgb     = rgb;
    current_vf->normals = normals;
    current_vf->order   = order;

    vertex_format(fildes,coord,use,rgb,normals,order);
}


void _prim_push_vertex_order(
    int fildes,
    int neworder)
{
    if (neworder == TOGGLE_ORDER) {
	if (current_vf->order == CLOCKWISE) neworder = COUNTER_CLOCKWISE;
	else neworder = CLOCKWISE;
    }
    else if (neworder == SAME_ORDER) {
	    neworder = current_vf->order;
    }

    _prim_push_vertex_format(fildes,current_vf->coord,current_vf->use,
	    current_vf->rgb,current_vf->normals,neworder);
}


void _prim_pop_vertex_format(
    int fildes)
{
    if (current_vf == vf_default) {
	fprintf(stderr,"pop_vertex_format of empty stack.\n");
	return;
    }

    --current_vf;

    vertex_format(fildes,current_vf->coord,current_vf->use,
	    current_vf->rgb,current_vf->normals,current_vf->order);
}
