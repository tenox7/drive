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

#define TINY 1.0e-20

#define fabsf fabs

int LU_decompose(
    MATRIX a,
    int n,
    IVECTOR indx,
    float *parity)
{
    int i,imax=1,j,k;
    float big,dum,sum,temp;
    VECTOR scaling;

    if ((scaling = vector(1,n)) == NULL) {
	return(FAILURE);
    }
    *parity = 1.0;	/* no row interchanges yet */
    for (i=1;i<=n;++i) {
	/* find the largest element in each row */
	big = 0.0;
	for (j=1;j<=n;++j)
	    if ((temp=fabsf(a[i][j])) > big) big = temp;
	if (big == 0.0) {
#ifdef DEBUG
	    nrerror("Singular matrix in LU_decompose");
#endif
	    free_vector(scaling,1,n);
	    return(FAILURE);
	}
	scaling[i] = 1.0/big;
    }

    /* Solve for U except for diagonal */
    for (j=1;j<=n;++j) {	/* for each column j */
	/* don't bother changing the top row */
	for (i=2;i<j;++i) {
	    /* a[i][j] -= sum      a[i][k]*a[k][j];
	     *            k=1,i-1
	     */
	    sum = a[i][j];
	    for (k=1;k<i;++k) sum -= a[i][k]*a[k][j];
	    a[i][j] = sum;
	}

	big = 0.0;
	for (i=j;i<=n;++i) {
	    /* figure diagonal and L values for this column */
	    sum = a[i][j];
	    for (k=1;k<j;++k) sum -= a[i][k]*a[k][j];
	    a[i][j] = sum;
	    /* where is largest scaled number in L or diagonal? */
	    if ((dum=scaling[i]*fabsf(sum)) >= big) {  
		/* This is a better pivot */
		big = dum;
		imax = i;
	    }
	}

	/* Do we need to interchange rows? */
	if (j != imax) {
	    /* yes-- partial pivot */
	    for (k=1;k<=n;++k) {
		dum = a[imax][k];
		a[imax][k] = a[j][k];
		a[j][k] = dum;
	    }
	    *parity = -(*parity);         /* change parity */
	    scaling[imax] = scaling[j];   /* and scale factor */
	}

	indx[j] = imax;
	/* If singular, approximate */
	if (a[j][j] == 0.0) a[j][j] = TINY;
	if (j != n) {
	    /* Divide by the pivot element to put a 1.0
	     * in the diagonal of the L matrix
	     */
	    dum = 1.0/(a[j][j]);
	    for (i=j+1;i<=n;++i) a[i][j] *= dum;
	}
    }

    free_vector(scaling,1,n);
    return(SUCCESS);
}


/* solve Ax=b for x, and return x in b */
void LU_solve(
    MATRIX a,
    int n,
    IVECTOR indx,
    VECTOR b)
{
    int i,j,nonzero_position=0,pivoted_i;
    float sum;

    /* forward-substitute in L */
    for (i=1;i<=n;++i) {
	pivoted_i = indx[i];
	/* account for pivoting */
	sum = b[pivoted_i];
	b[pivoted_i] = b[i];
	if (nonzero_position)
		for (j=nonzero_position;j<=i-1;++j) sum -= a[i][j]*b[j];
	else if (sum) nonzero_position = i;
	b[i] = sum;
    }

    /* back-substitute in U */
    for (i=n;i>=1;--i) {
	sum = b[i];
	for (j=i+1;j<=n;++j) sum -= a[i][j]*b[j];
	b[i] = sum/a[i][i];
    }
}


float LU_determinant(
    MATRIX a,
    int size,
    IVECTOR indx,
    float parity)
{
    register float det;
    register int i;

    det = parity;
    for (i=1;i<size;++i) det *= a[i][i];

    return(det);
}


int LU_invert(
    MATRIX a,
    MATRIX inverse,
    int size,
    IVECTOR indx,
    float parity)
{
    int i,j;
    VECTOR column;

    if ((column = vector(1,size)) == NULL) {
	return(FAILURE);
    }

    /* suppose that LU_decomposition has already been done */
    for (j=1;j<=size;++j) { 	/* for each column */
	/* set solution equal to column of I */
	for (i=1;i<=size;++i) column[i] = 0.0;
	column[j] = 1.0;

	LU_solve(a,size,indx,column);
	for (i=1;i<=size;++i) inverse[i][j] = column[i];
    }

    free_vector(column,1,size);
    return(SUCCESS);
}


int invert_matrix(
    MATRIX a,
    MATRIX inverse,
    int size)
{
    IVECTOR indx;
    float parity;
    MATRIX LU_a;

    if ((indx = ivector(1,size)) == NULL) {
	return(FAILURE);
    }
    if ((LU_a = matrix(1,size,1,size)) == NULL) {
	free_ivector(indx,1,size);
	return(FAILURE);
    }

    COPY_MATRIX(a,LU_a,1,size,1,size,1,1);
    if (LU_decompose(LU_a,size,indx,&parity) == FAILURE) {
	free_ivector(indx,1,size);
	free_matrix(LU_a,1,size,1,size);
	return(FAILURE);
    }
    if (LU_invert(LU_a,inverse,size,indx,parity) == FAILURE) {
	free_ivector(indx,1,size);
	free_matrix(LU_a,1,size,1,size);
	return(FAILURE);
    }

    free_ivector(indx,1,size);
    free_matrix(LU_a,1,size,1,size);
    return(SUCCESS);
}
