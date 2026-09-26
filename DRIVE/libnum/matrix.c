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


/* NR float matrix allocation/free routines */
#include <stdlib.h>
#include <stdio.h>
#include "libnum.h"

/* allocate a float matrix of the form m[lorow..hirow][locol..hicol] */
MATRIX matrix(
    int lorow, int hirow,
    int locol, int hicol)
{
    int i;
    unsigned colsize;
    MATRIX m;

    /* malloc row pointers */
    if ((m = (MATRIX)
	    malloc((unsigned)(hirow-lorow+1)*(sizeof(float*)))) == NULL) {
	nrerror("row pointer malloc failure in matrix()");
	return(NULL);
    }
    m -= lorow;

    /* malloc rows and initialize the pointers */
    colsize = (hicol-locol+1) * sizeof(float);
    for (i=lorow;i<=hirow;++i) {
	    if ((m[i] = (float *) malloc(colsize)) == NULL) {
		nrerror("row malloc failed in matrix()");
		free((void *) m);
		return(NULL);
	    }
	    m[i] -= locol;
    }

    return(m);
}


/* return float submatrix
 *   m[newlorow..newlorow+(rows)][newlocol..newlocol+(newlorows)]
 * pointing into original "a" matrix
 */
MATRIX submatrix(
    MATRIX a,
    int lorow, int hirow,
    int locol, int hicol,
    int newlorow, int newlocol)
{
    int i,j;
    MATRIX m;

    /* malloc pointers to rows */
    if ((m = (MATRIX)
		malloc((unsigned)(hirow-lorow+1)*sizeof(float*))) == NULL) {
	nrerror("malloc error in submatrix()");
	return(NULL);
    }

    /* initialize pointers */
    for (i=lorow,j=newlorow;i<=hirow;++i,++j)
	m[j] = a[i] + locol - newlocol;

    return (m);
}


/* free a matrix malloc'ed with matrix() */
void free_matrix(
    MATRIX m,
    int lorow, int hirow,
    int locol, int hicol)
{
    int i;

    for (i=hirow;i>=lorow;--i) free((void *)(m[i]+locol));
    free((void *)(m+lorow));
}


/* free a submatrix malloc'ed with submatrix() */
void free_submatrix(
    MATRIX m,
    int lorow, int hirow,
    int locol, int hicol)
{
    free((void *)(m+lorow));
}


/* create matrix m[lorow..hirow][locol..hicol] pointing
 * to standard C matrix a[0..hirow-lorow+1][0..hicol-locol+1]
 */
MATRIX convert_matrix(
    float *a,
    int lorow, int hirow,
    int locol, int hicol)
{
    int i,j,nrow,ncol;
    MATRIX m;

    nrow = hirow-lorow+1;
    ncol = hicol-locol+1;
    if ((m = (MATRIX) malloc((unsigned)nrow*sizeof(float*))) == NULL) {
	nrerror("malloc error in convert_matrix()");
	return(NULL);
    }

    m -= lorow;
    for (i=0,j=lorow;i<=nrow-1;++i,++j) m[j] = a+ncol*i-locol;

    return(m);
}


/* free pointers from convert_matrix() */
void free_convert_matrix(
    MATRIX a,
    int lorow, int hirow,
    int locol, int hicol)
{
    free((void *)(a+lorow));
}


void transpose_matrix(
    MATRIX m,
    int rlo, int rhi,
    int clo, int chi)
{
    int i,j;
    float swap;

    for (i=rlo+1; i<=rhi; ++i) {
	for (j=clo; j<(i-rlo+clo); ++j) {
	    swap = m[i][j];
	    m[i][j] = m[j][i];
	    m[j][i] = swap;
	}
    }
}
			

/* allocate float vector[lo..hi] */
VECTOR vector(
    int lo, int hi)
{
    VECTOR v;

    if ((v = (VECTOR) malloc((unsigned)(hi-lo+1)*sizeof(float))) == NULL) {
	nrerror("malloc failure in vector()");
	return(NULL);
    }
    return (v-lo);
}


/* free the float vector v from vector() */
void free_vector(
    VECTOR v,
    int lo, int hi)
{
    free((void *) (v+lo));
}


/*  v[1..3] -> its dual M[1..3][1..3] */
void vector_dual(
    VECTOR v,
    MATRIX M)
{
    M[1][1] = M[2][2] = M[3][3] = 0.0;
    M[1][2] =  v[3];
    M[1][3] = -v[2];
    M[2][1] = -v[3];
    M[2][3] =  v[1];
    M[3][1] =  v[2];
    M[3][2] = -v[1];
}


int compare_matrices(
    MATRIX a, MATRIX b,
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

void multiply_matrices(
    MATRIX a, MATRIX b, MATRIX result,
    int alorow, int ahirow,
    int alocol, int ahicol,
    int blorow, int blocol,
    int rlorow, int rlocol)
{
    register int ar,ac,br,bc,rr,rc;
    int size;

    size = ahirow-alorow+1;

    for (rr=rlorow,ar=alorow;rr<rlorow+size;++rr,++ar) {
	for (rc=rlocol,bc=blocol;rc<rlocol+size;++rc,++bc) {
	    result[rr][rc] = 0.0;
	    for (ac=alocol,br=blorow;ac<=ahicol;++ac,++br)
		result[rr][rc] += a[ar][ac]*b[br][bc];
	}
    }
}


void diag_times_matrix(
    MATRIX d, MATRIX b, MATRIX result,
    int dlorow, int dhirow,
    int dlocol, int dhicol,
    int blorow, int blocol,
    int rlorow, int rlocol)
{
    register int dr,dc,br,bc,rr,rc;
    register float value;
    int bhicol;

    bhicol = blocol - dlorow + dhirow;
    for (br=blorow,rr=rlorow,dr=dlorow,dc=dlocol;
		    dr<=dhirow; ++br,++rr,++dr,++dc) {
	value = d[dr][dc];
	for (bc=blocol,rc=rlocol; bc<=bhicol; ++bc,++rc) {
	    result[rr][rc] = value*b[br][bc];
	}
    }
}


void matrix_times_diag(
    MATRIX a, MATRIX d, MATRIX result,
    int alorow, int ahirow,
    int alocol, int ahicol,
    int dlorow, int dlocol,
    int rlorow, int rlocol)
{
    register int ar,ac,dr,dc,rr,rc;
    register float value;

    dr = dlorow; dc = dlocol;
    for (ac=alocol,rc=rlocol; ac<=ahicol; ++ac,++rc) {
	value = d[dr++][dc++];
	for (ar=alorow,rr=rlorow; ar<=ahirow; ++ar,++rr) {
		result[rr][rc] = value*a[ar][ac];
	}
    }
}


void matrix_times_vector(
    MATRIX a,
    VECTOR b, VECTOR result,
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


float dot_product(
    VECTOR a, VECTOR b,
    int alo, int ahi, int blo)
{
    register int ai,bi;
    register float sum=0.0;

    for (ai=alo,bi=blo;ai<=ahi;++ai,++bi) sum += a[ai]*b[bi];

    return(sum);
}


/* 3d vectors only! */
void cross_product(
    VECTOR a, VECTOR b, VECTOR result,
    int alo, int blo, int rlo)
{
    result[rlo]   = a[alo+1]*b[blo+2] - a[alo+2]*b[blo+1];
    result[rlo+1] = a[alo+2]*b[blo  ] - a[alo  ]*b[blo+2];
    result[rlo+2] = a[alo  ]*b[blo+1] - a[alo+1]*b[blo];
}
