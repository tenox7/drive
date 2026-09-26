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

static float _sqrarg;
#define SQR(a) (_sqrarg = (a),_sqrarg*_sqrarg)

static void covsrt(
    MATRIX covar,
    int ma,
    IVECTOR lista,
    int mfit)
{
    int i,j;
    float swap;

    for (j=1; j<ma; ++j) {
	for (i=j+1; i<=ma; ++i) {
	    covar[i][j] = 0.0;
	}
    }

    for (i=1; i<mfit; ++i) {
	for (j=i+1; j<=mfit; ++j) {
	    if (lista[j] > lista[i])
		covar[lista[j]][lista[i]] = covar[i][j];
	    else 
		covar[lista[i]][lista[j]] = covar[i][j];
	}
    }

    swap = covar[1][1];
    for (j=1; j<=ma; ++j) {
	covar[1][j] = covar[j][j];
	covar[j][j] = 0.0;
    }
    covar[lista[1]][lista[1]] = swap;

    for (j=2; j<=mfit; ++j) {
	covar[lista[j]][lista[j]] = covar[1][j];
    }
    for (j=2; j<=ma; ++j) {
	for (i=1; i<=j; ++i) {
	    covar[i][j] = covar[j][i];
	}
    }
}


int lfit(
    VECTOR x,
    VECTOR y,
    VECTOR sig,
    int ndata,
    VECTOR a, int ma,
    IVECTOR lista,
    int mfit,
    float **covar,
    float *chisq,
    LFIT_EVALUATION_ROUTINE funcs)
{
    int k,kk,j,i,result;
    float ym,wt,sum,sig2i;
    MATRIX beta;
    VECTOR afunc;

    if ((beta = matrix(1,ma,1,1)) == NULL) {
	return(FAILURE);
    }
    if ((afunc = vector(1,ma)) == NULL) {
	free_matrix(beta,1,ma,1,1);
	return(FAILURE);
    }
    kk = mfit+1;

    for (j=1; j<=ma; ++j) {
	int ihit;

	ihit = 0;
	for (k=1; k<=mfit; ++k) {
	    if (lista[k] == j) ++ihit;
	}
	if (ihit == 0) {
	    lista[kk++] = j;
	}
	else if (ihit > 1) {
	    nrerror("Bad LISTA permutation in LFIT-1");
	    free_vector(afunc,1,ma);
	    free_matrix(beta,1,ma,1,1);
	    return(FAILURE);
	}
    }

    if (kk != (ma+1)) {
	nrerror("Bad LISTA permution in LFIT-2");
	free_vector(afunc,1,ma);
	free_matrix(beta,1,ma,1,1);
	return(FAILURE);
    }

    for (j=1; j<=mfit; ++j) {
	for (k=1; k<=mfit; ++k) {
	    covar[j][k] = 0.0;
	}
	beta[j][1] = 0.0;
    }

    for (i=1; i<=ndata; ++i) {
	(*funcs)(x[i],afunc,ma);
	ym = y[i];
	if (mfit < ma) {
	    for (j=(mfit+1); j<ma; ++j) {
		ym -= a[lista[j]]*afunc[lista[j]];
	    }
	}
	sig2i = 1.0/SQR(sig[i]);
	for (j=1; j<=mfit; ++j) {
	    wt = afunc[lista[j]]*sig2i;
	    for (k=1; k<=j; ++k) {
		covar[j][k] += wt*afunc[lista[k]];
	    }
	    beta[j][1] += ym*wt;
	}
    }

    if (mfit > 1) {
	for (j=2; j<=mfit; ++j) {
	    for (k=1; k<=j; ++k) {
		covar[k][j] = covar[j][k];
	    }
	}
    }

    disable_numerical_library_errors();
    result = gaussj(covar,mfit,beta,1);
    enable_numerical_library_errors();

    if (result == FAILURE) {
	free_vector(afunc,1,ma);
	free_matrix(beta,1,ma,1,1);
	return(FAILURE);
    }
    for (j=1; j<=mfit; ++j) {
	a[lista[j]] = beta[j][1];
    }
    *chisq = 0.0;
    for (i=1; i<=ndata; ++i) {
	(*funcs)(x[i],afunc,ma);
	for (sum=0.0,j=1; j<=ma; ++j) {
	    sum += a[j]*afunc[j];
	}
	*chisq += SQR((y[i]-sum)/sig[i]);
    }
    covsrt(covar,ma,lista,mfit);
    free_matrix(beta,1,ma,1,1);
    free_vector(afunc,1,ma);

    return(SUCCESS);
}


static void polynomials(
    float x,
    VECTOR afunc,
    int ma)
{
    int i;
    float product;

    afunc[1] = product = 1.0;
    for (i=2; i<=ma; ++i) {
	product *= x;
	afunc[i] = product;
    }
}


float polynomial_curvefit(
    VECTOR x,
    VECTOR y,
    int ndata,
    VECTOR coeffs,
    int maxorder)
{
    VECTOR sig;
    IVECTOR lista;
    MATRIX covar;
    int i,retval;
    float chisq;
    

    if ((sig = vector(1,ndata)) == NULL) {
	return((float) FAILURE);
    }
    if ((lista = ivector(1,ndata)) == NULL) {
	free_vector(sig,1,ndata);
	return((float) FAILURE);
    }
    if ((covar = matrix(1,ndata,1,ndata)) == NULL) {
	free_vector(sig,1,ndata);
	free_ivector(lista,1,ndata);
	return((float) FAILURE);
    }
    for (i=1; i<=ndata; ++i) {
	if ((sig[i] = x[i]*0.01) < 0.0) sig[i] = -sig[i];
	if (sig[i] < 0.01) sig[i] = 0.01;
	lista[i] = i;
    }

    retval = lfit(x,y,sig,ndata,coeffs,maxorder,lista,maxorder,covar,
	&chisq,polynomials);

    free_vector(sig,1,ndata);
    free_ivector(lista,1,ndata);
    free_matrix(covar,1,ndata,1,ndata);

    if (retval == FAILURE) return((float) FAILURE);
    else return(chisq);
}


static void mrqcof(
    void *x[],
    float y[],
    float sig[],
    int ndata,
    float a[], int ma,
    int lista[],
    int mfit,
    float **alpha,
    float beta[],
    float *chisq,
    CURVEFIT_EVALUATION_ROUTINE funcs)
{
    int k,j,i;
    float ymod,wt,sig2i,dy,*dyda;

    dyda = vector(1,ma);
    for (j=1; j<=mfit; ++j) {
	for (k=1; k<=j; ++k) alpha[j][k] = 0.0;
	beta[j] = 0.0;
    }
    *chisq = 0.0;
    for (i=1; i<ndata; ++i) {
	(*funcs)(x[i],a,&ymod,dyda,ma);
	sig2i = 1.0/(sig[i]*sig[i]);
	dy = y[i] - ymod;
	for (j=1; j<=mfit; ++j) {
	    wt = dyda[lista[j]]*sig2i;
	    for (k=1; k<=j; ++k) alpha[j][k] += wt*dyda[lista[k]];
	    beta[j] += dy*wt;
	}
	(*chisq) += dy*dy*sig2i;
    }
    for (j=2; j<=mfit; ++j) {
	for (k=1; k<=j-1; ++k) alpha[k][j] = alpha[j][k];
    }
    free_vector(dyda,1,ma);
}

/* The length of each step is derived from the diagonal elements of the 
 * curvature matrix alpha multiplied by some constant lambda.  Lambda
 * starts out small and shrinks or grows as we proceed.  An initial lambda
 * of 0.001 is fairly conservative.
 */
#define INITIAL_LAMBDA	0.001

static void mrqmin(
    void *x[],
    float y[],
    float sig[],
    int ndata,
    float a[],
    int ma,
    int lista[],
    int mfit,
    float **covar,
    float **alpha,
    float *chisq,
    CURVEFIT_EVALUATION_ROUTINE funcs,
    float *alambda)
{
    int k,kk,j,ihit;
    static float *da,*atry,**oneda,*beta,ochisq;

    if (*alambda < 0.0) {
	oneda = matrix(1,mfit,1,1);
	atry = vector(1,ma);
	da = vector(1,ma);
	beta = vector(1,ma);
	kk = mfit+1;
	for (j=1; j<=ma; ++j) {
	    ihit = 0;
	    for (k=1; k<=mfit; ++k) if (lista[k] == j) ++ihit;
	    if (ihit == 0) lista[kk++] = j;
	    else if (ihit > 1) nrerror("Bad LISTA permutation in mrqmin");
	}
	if (kk != ma+1) nrerror("Bad LISTA permutation in mrqmin");
	*alambda = INITIAL_LAMBDA;
	mrqcof(x,y,sig,ndata,a,ma,lista,mfit,alpha,beta,chisq,funcs);
	ochisq = *chisq;
    }

    for (j=1; j<=mfit; ++j) {
	for (k=1; k<= mfit; ++k) covar[j][k] = alpha[j][k];
	covar[j][j] = alpha[j][j] * (1.0+(*alambda));
	oneda[j][1] = beta[j];
    }

    disable_numerical_library_errors();
    gaussj(covar,mfit,oneda,1);
    enable_numerical_library_errors();

    for (j=1; j<=mfit; ++j) da[j] = oneda[j][1];
    if (*alambda == 0.0) {
	covsrt(covar,ma,lista,mfit);
	free_vector(beta,1,ma);
	free_vector(da,1,ma);
	free_vector(atry,1,ma);
	free_matrix(oneda,1,mfit,1,1);
	return;
    }
    for (j=1; j<=ma; ++j) atry[j] = a[j];
    for (j=1; j<=mfit; ++j) atry[lista[j]] = a[lista[j]] + da[j];
    mrqcof(x,y,sig,ndata,atry,ma,lista,mfit,covar,da,chisq,funcs);
    if (*chisq < ochisq) {
	*alambda *= 0.1;
	ochisq = *chisq;
	for (j=1; j<=mfit; ++j) {
	    for (k=1; k<=mfit; ++k) alpha[j][k] = covar[j][k];
	    beta[j] = da[j];
	    a[lista[j]] = atry[lista[j]];
	}
    }
    else {
	*alambda *= 10.0;
	*chisq = ochisq;
    }
}

#define EPSILON			(1e-12)

/* If chisq only gets this much better, stop */
#define NONLINEAR_STOP_DELTA	(1e-5)
/* Also stop if lambda gets this big */
#define LAMBDA_STOP		(INITIAL_LAMBDA*1e12)

/* 
 * x[1..ndata],y[1..ndata] are datapoints.
 * a[1..ma] are the coefficients to the equation, initialized to a reasonable
 *   guess.
 * lista[1..ma] numbers the parameters a such that the first "mfit" of them
 *   are adjustable and the rest are fixed.
 * funcs is a function pointer that fits this prototype
 *   void (*funcs)(void *x, float a[], float *y_returned, float dyda_returned[],
 *      int na)
 *
 * Return value is chi-squared for the approximation.
 */
float nonlinear_curvefit(
    void *x[],
    float y[],
    int ndata,
    float a[],
    int ma,
    int lista[],
    int mfit,
    CURVEFIT_EVALUATION_ROUTINE funcs)
{
    VECTOR sig;
    MATRIX covar,alpha;
    int i;
    float chisq,oldchisq,alambda,oldlambda;

    sig = vector(1,ndata);
    for (i=1; i<=ndata; ++i) sig[i] = 1.0;
    covar = matrix(1,ma,1,ma);
    alpha = matrix(1,ma,1,ma);
    alambda = -1.0;

    /* Initial pass with negative alambda initializes everything */
    mrqmin(x,y,sig,ndata,a,ma,lista,mfit,covar,alpha,&chisq,funcs,&alambda);

    if (mfit > 0) {
	/* Do not stop on an iteration where lambda increases -- 
	 * the step constant is still adjusting itself.
	 */
	do {
	    oldchisq  = chisq;
	    oldlambda = alambda;
	    mrqmin(x,y,sig,ndata,a,ma,lista,mfit,
		covar,alpha,&chisq,funcs,&alambda);
	    if (chisq < EPSILON) break;
	    if (alambda > LAMBDA_STOP) break;
	} while ((alambda > oldlambda)
	    || ((oldchisq - chisq)/oldchisq > NONLINEAR_STOP_DELTA));
    }

    /* Call once more with lambda==0.0 to clean up. */
    alambda = 0.0;
    mrqmin(x,y,sig,ndata,a,ma,lista,mfit,covar,alpha,&chisq,funcs,&alambda);

    free_vector(sig,1,ndata);
    free_matrix(covar,1,ma,1,ma);
    free_matrix(alpha,1,ma,1,ma);
    return(chisq);
}
