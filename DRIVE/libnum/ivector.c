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


/* NR int matrix allocation/free routines */
#include <stdlib.h>
#include <stdio.h>
#include "libnum.h"

/* allocate int vector[lo..hi] */
IVECTOR ivector(
    int lo, int hi)
{
    IVECTOR v;

    if ((v = (IVECTOR) malloc((unsigned)(hi-lo+1)*sizeof(int))) == NULL) {
	nrerror("malloc failure in ivector()");
	return(NULL);
    }
    return (v-lo);
}


/* free the int ivector v from ivector() */
void free_ivector(
    IVECTOR v,
    int lo, int hi)
{
    free((void *) (v+lo));
}
