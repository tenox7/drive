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

/* allocate a int imatrix of the form m[lorow..hirow][locol..hicol] */
IMATRIX imatrix(
    int lorow, int hirow,
    int locol, int hicol)
{
    int i;
    unsigned colsize;
    IMATRIX m;

    /* malloc row pointers */
    if ((m = (IMATRIX)
		malloc((unsigned)(hirow-lorow+1)*(sizeof(int*)))) == NULL) {
	nrerror("row pointer malloc failure in imatrix()");
	return(NULL);
    }
    m -= lorow;

    /* malloc rows and initialize the pointers */
    colsize = (hicol-locol+1) * sizeof(int);
    for (i=lorow;i<=hirow;++i) {
	if ((m[i] = (int *) malloc(colsize)) == NULL) {
	    nrerror("row malloc failed in imatrix()");
	    free((void *) m);
	    return(NULL);
	}
	m[i] -= locol;
    }

    return(m);
}


/* return int isubmatrix
 *   m[newlorow..newlorow+(rows)][newlocol..newlocol+(newlorows)]
 * pointing into original "a" matrix
 */
IMATRIX isubmatrix(
    IMATRIX a,
    int lorow, int hirow,
    int locol, int hicol,
    int newlorow, int newlocol)
{
    int i,j;
    IMATRIX m;

    /* malloc pointers to rows */
    if ((m = (IMATRIX) malloc((unsigned)(hirow-lorow+1)*sizeof(int*))) == NULL){
	nrerror("malloc error in isubmatrix()");
	return(NULL);
    }

    /* initialize pointers */
    for (i=lorow,j=newlorow;i<=hirow;++i,++j) m[j] = a[i] + locol - newlocol;

    return (m);
}


/* free a imatrix malloc'ed with imatrix() */
void free_imatrix(
    IMATRIX m,
    int lorow, int hirow,
    int locol, int hicol)
{
    int i;

    for (i=hirow;i>=lorow;--i) free((void *)(m[i]+locol));
    free((void *)(m+lorow));
}


/* free a isubmatrix malloc'ed with isubmatrix() */
void free_isubmatrix(
    IMATRIX m,
    int lorow, int hirow,
    int locol, int hicol)
{
    free((void *)(m+lorow));
}


/* create imatrix m[lorow..hirow][locol..hicol] pointing
 * to standard C imatrix a[0..hirow-lorow+1][0..hicol-locol+1]
 */
IMATRIX convert_imatrix(
    int *a,
    int lorow, int hirow,
    int locol, int hicol)
{
    int i,j,nrow,ncol;
    IMATRIX m;

    nrow = hirow-lorow+1;
    ncol = hicol-locol+1;
    if ((m = (IMATRIX) malloc((unsigned)nrow*sizeof(int*))) == NULL) {
	nrerror("malloc error in convert_imatrix()");
	return(NULL);
    }

    m -= lorow;
    for (i=0,j=lorow;i<=nrow-1;++i,++j) m[j] = a+ncol*i-locol;

    return(m);
}


/* free pointers from convert_imatrix() */
void free_convert_imatrix(
    int *a,
    int lorow, int hirow,
    int locol, int hicol)
{
    free((void *)(a+lorow));
}


int compare_imatrices(
    IMATRIX a, IMATRIX b,
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


void multiply_imatrices(
    IMATRIX a, IMATRIX b, IMATRIX result,
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


void imatrix_times_ivector(
    IMATRIX a,
    IVECTOR b, IVECTOR result,
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


int idot_product(
    IVECTOR a, IVECTOR b,
    int alo, int ahi, int blo)
{
    register int ai,bi;
    register int sum=0.0;

    for (ai=alo,bi=blo;ai<=ahi;++ai,++bi) sum += a[ai]*b[bi];

    return(sum);
}


/* 3d vectors only! */
void icross_product(
    IVECTOR a, IVECTOR b, IVECTOR result,
    int alo, int blo, int rlo)
{
    result[rlo]   = a[alo+1]*b[blo+2] - a[alo+2]*b[blo+1];
    result[rlo+1] = a[alo+2]*b[blo  ] - a[alo  ]*b[blo+2];
    result[rlo+2] = a[alo  ]*b[blo+1] - a[alo+1]*b[blo];
}
