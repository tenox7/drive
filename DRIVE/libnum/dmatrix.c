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


/* NR double matrix allocation/free routines */
#include <stdlib.h>
#include <stdio.h>
#include "libnum.h"

/* allocate a double dmatrix of the form m[lorow..hirow][locol..hicol] */
DMATRIX dmatrix(
    int lorow, int hirow,
    int locol, int hicol)
{
    int i;
    unsigned colsize;
    DMATRIX m;

    /* malloc row pointers */
    if ((m = (DMATRIX)
	    malloc((unsigned)(hirow-lorow+1)*(sizeof(double*)))) == NULL) {
	nrerror("row pointer malloc failure in dmatrix()");
	return(NULL);
    }
    m -= lorow;

    /* malloc rows and initialize the pointers */
    colsize = (hicol-locol+1) * sizeof(double);
    for (i=lorow;i<=hirow;++i) {
	if ((m[i] = (double *) malloc(colsize)) == NULL) {
	    nrerror("row malloc failed in dmatrix()");
	    free((void *) m);
	    return(NULL);
	}
	m[i] -= locol;
    }

    return(m);
}


/* return double dsubmatrix
 *   m[newlorow..newlorow+(rows)][newlocol..newlocol+(newlorows)]
 * pointing into original "a" matrix
 */
DMATRIX dsubmatrix(
    DMATRIX a,
    int lorow, int hirow,
    int locol, int hicol,
    int newlorow, int newlocol)
{
    int i,j;
    DMATRIX m;

    /* malloc pointers to rows */
    if ((m = (DMATRIX)
		malloc((unsigned)(hirow-lorow+1)*sizeof(double*))) == NULL) {
	nrerror("malloc error in dsubmatrix()");
	return(NULL);
    }

    /* initialize pointers */
    for (i=lorow,j=newlorow;i<=hirow;++i,++j)
	m[j] = a[i] + locol - newlocol;

    return (m);
}


/* free a dmatrix malloc'ed with dmatrix() */
void free_dmatrix(
    DMATRIX m,
    int lorow, int hirow,
    int locol, int hicol)
{
    int i;

    for (i=hirow;i>=lorow;--i) free((void *)(m[i]+locol));
    free((void *)(m+lorow));
}


/* free a dsubmatrix malloc'ed with dsubmatrix() */
void free_dsubmatrix(
    DMATRIX m,
    int lorow, int hirow,
    int locol, int hicol)
{
    free((void *)(m+lorow));
}
 

/* create dmatrix m[lorow..hirow][locol..hicol] pointing
 * to standard C dmatrix a[0..hirow-lorow+1][0..hicol-locol+1]
 */
DMATRIX convert_dmatrix(
    double *a,
    int lorow, int hirow,
    int locol, int hicol)
{
    int i,j,nrow,ncol;
    DMATRIX m;

    nrow = hirow-lorow+1;
    ncol = hicol-locol+1;
    if ((m = (DMATRIX)
		    malloc((unsigned)nrow*sizeof(double*))) == NULL) {
	nrerror("malloc error in convert_dmatrix()");
	return(NULL);
    }

    m -= lorow;
    for (i=0,j=lorow;i<=nrow-1;++i,++j) m[j] = a+ncol*i-locol;

    return(m);
}


/* free pointers from convert_dmatrix() */
void free_convert_dmatrix(
    DMATRIX a,
    int lorow, int hirow,
    int locol, int hicol)
{
    free((void *)(a+lorow));
}


/* allocate double dvector[lo..hi] */
DVECTOR dvector(
    int lo,
    int hi)
{
    double *v;

    if ((v = (DVECTOR) malloc((unsigned)(hi-lo+1)*sizeof(double))) == NULL) {
	nrerror("malloc failure in dvector()");
	return(NULL);
    }
    return (v-lo);
}


/* free the double dvector v from dvector() */
void free_dvector(
    DVECTOR v,
    int lo, int hi)
{
    free((void *) (v+lo));
}


int compare_dmatrices(
    DMATRIX a, DMATRIX b,
    int alorow, int ahirow,
    int alocol, int ahicol,
    int blorow, int blocol)
{
    int ar,ac,br,bc;

    for (ar=alorow,br=blorow;ar<=ahirow;++ar,++br)
	for (ac=alocol,bc=blocol;ac<=ahicol;++ac,++bc)
	    if (a[ar][ac] != b[br][bc]) return(0);

    return(1);
}


void multiply_dmatrices(
    DMATRIX a, DMATRIX b, DMATRIX result,
    int alorow, int ahirow,
    int alocol, int ahicol,
    int blorow, int blocol,
    int rlorow, int rlocol)
{
    int ar,ac,br,bc,rr,rc,size;

    size = ahirow-alorow+1;

    for (rr=rlorow,ar=alorow;rr<rlorow+size-1;++rr,++ar) {
	for (rc=rlocol,bc=blocol;rc<rlocol+size-1;++rc,++bc) {
	    result[rr][rc] = 0.0;
	    for (ac=alocol,br=blorow;ac<=ahicol;++ac,++br)
		result[rr][rc] += a[ar][ac]*b[br][bc];
	}
    }
}


void dmatrix_times_dvector(
    DMATRIX a,
    DVECTOR b, DVECTOR result,
    int alorow, int ahirow,
    int alocol, int ahicol,
    int blo, int rlo)
{
    register int ri,bi,ac,ar;
    int size;

    size = ahirow-alorow+1;
    for (ri=rlo,ar=alorow;ri<rlo+size;++ri,++ar) {
	result[ri] = 0.0;
	for (ac=alocol,bi=blo;ac<=ahicol;++ac,++bi)
	    result[ri] += a[ar][ac]*b[bi];
    }
}


double ddot_product(
    DVECTOR a, DVECTOR b,
    int alo, int ahi,
    int blo)
{
    register int ai,bi;
    register double sum=0.0;

    for (ai=alo,bi=blo;ai<=ahi;++ai,++bi) sum += a[ai]*b[bi];

    return(sum);
}


/* 3d vectors only! */
void dcross_product(
    DVECTOR a, DVECTOR b, DVECTOR result,
    int alo, int blo, int rlo)
{
    result[rlo]   = a[alo+1]*b[blo+2] - a[alo+2]*b[blo+1];
    result[rlo+1] = a[alo+2]*b[blo  ] - a[alo  ]*b[blo+2];
    result[rlo+2] = a[alo  ]*b[blo+1] - a[alo+1]*b[blo];
}
