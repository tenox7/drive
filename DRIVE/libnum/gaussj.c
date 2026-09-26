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

static float _abstmp;
#define FABS(a) (((_abstmp = (a)) < 0.0) ? -_abstmp : _abstmp)
#define SWAP(a,b) {float _tmp = (a); (a) = (b); (b) = _tmp; }

int gaussj(
    MATRIX a,
    int n,
    MATRIX b,
    int m)
{
    IVECTOR indxc,indxr,ipiv;
    int i,icol=1,irow=1,j,k,l,ll;
    float big;
    double dum,pivinv;

    if ((indxc = ivector(1,n)) == NULL) {
	return(FAILURE);
    }
    if ((indxr = ivector(1,n)) == NULL) {
	free_ivector(indxc,1,n);
	return(FAILURE);
    }
    if ((ipiv  = ivector(1,n)) == NULL) {
	free_ivector(indxr,1,n);
	free_ivector(indxc,1,n);
	return(FAILURE);
    }
    for (j=1; j<=n; ++j) ipiv[j] = 0;
    for (i=1; i<=n; ++i) {
	big = 0.0;
	for (j=1; j<=n; ++j) {
	    if (ipiv[j] != 1) {
		for (k=1; k<=n; ++k) {
		    if (ipiv[k] == 0) {
			if (FABS(a[j][k]) >= big) {
			    big = FABS(a[j][k]);
			    irow = j;
			    icol = k;
			}
		    }
		    else if (ipiv[k] > 1) {
			nrerror("GAUSSJ: singular matrix");
			free_ivector(ipiv,1,n);
			free_ivector(indxr,1,n);
			free_ivector(indxc,1,n);
			return(FAILURE);
		    }
		}
	    }
	}

	++(ipiv[icol]);
	if (irow != icol) {
	    for (l=1; l<=n; ++l) SWAP(a[irow][l],a[icol][l]);
	    for (l=1; l<=m; ++l) SWAP(b[irow][l],b[icol][l]);
	}
	indxr[i] = irow;
	indxc[i] = icol;
	if (a[icol][irow] == 0.0) {
	    nrerror("GAUSSJ: singular matrix");
	    free_ivector(ipiv,1,n);
	    free_ivector(indxr,1,n);
	    free_ivector(indxc,1,n);
	    return(FAILURE);
	}
	pivinv = 1.0/a[icol][icol];
	/* a[icol][icol] = 1.0; */
	for (l=1; l<=n; ++l) a[icol][l] *= pivinv;
	for (l=1; l<=m; ++l) b[icol][l] *= pivinv;
	for (ll=1; ll<=n; ++ll) {
	    if (ll != icol) {
		dum = a[ll][icol];
		/* a[ll][icol] = 0.0; */
		for (l=1; l<=n; ++l) a[ll][l] -= a[icol][l] * dum;
		for (l=1; l<=m; ++l) b[ll][l] -= b[icol][l] * dum;
	    }
	}
    }
    for (l=n; l>=1; --l) {
	if (indxr[l] != indxc[l]) {
	    for (k=1; k<=n; ++k) {
		SWAP(a[k][indxr[l]],a[k][indxc[l]]);
	    }
	}
    }

    free_ivector(ipiv,1,n);
    free_ivector(indxr,1,n);
    free_ivector(indxc,1,n);
    return(SUCCESS);
}
