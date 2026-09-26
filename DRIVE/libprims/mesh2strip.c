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
#include <math.h>
#include "prims.ih"

void mesh_to_strips(
    int fildes,
    float *mesh,
    int rows, int columns,
    float *gnormals,
    int cpv, int direction)
{
    float *strip;
    int i,j,k;
    float *row1,*row2,*fptr;

    if (gnormals != NULL) {
	/* bail out */
	quadrilateral_mesh(fildes,mesh,rows,columns,gnormals);
	return;
    }

    if ((strip = (float *)
	    malloc((unsigned int) columns*2*sizeof(float)*cpv)) == NULL) {
	/* bail out */
	quadrilateral_mesh(fildes,mesh,rows,columns,gnormals);
	return;
    }

    row1 = mesh;
    row2 = mesh + columns*cpv;

    if (direction == COUNTER_CLOCKWISE) {
	for (i=0; i<rows-1; ++i) {
	    fptr = strip;
	    for (j=0; j<columns; ++j) {
		for (k=0; k<cpv; ++k) *fptr++ = *row1++;
		for (k=0; k<cpv; ++k) *fptr++ = *row2++;
	    }
	    triangular_strip(fildes,strip,columns*2,NULL);
	}
    }
    else {
	for (i=0; i<rows-1; ++i) {
	    fptr = strip;
	    for (j=0; j<columns; ++j) {
		for (k=0; k<cpv; ++k) *fptr++ = *row2++;
		for (k=0; k<cpv; ++k) *fptr++ = *row1++;
	    }
	    triangular_strip(fildes,strip,columns*2,NULL);
	}
    }

    free((void *) strip);
}


void quad_to_strips(
    int fildes,
    float *mesh,
    int rows, int columns,
    float *gnormals)
{
    int coord,use,rgb,normals,order;

    inquire_vertex_format(fildes,&coord,&use,&rgb,&normals,&order);
    mesh_to_strips(fildes,mesh,rows,columns,gnormals,coord+3,order);
}
