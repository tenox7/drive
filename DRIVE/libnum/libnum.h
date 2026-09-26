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


/* numlib header file */
#ifndef _NUMLIB_H_INCLUDED
#define _NUMLIB_H_INCLUDED

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

#ifndef M_PI_2
#define M_PI_2 1.5707963267948966192313216916398
#endif

#ifndef M_PI_4
#define M_PI_4 0.78539816339744830961566084581988
#endif

#ifndef M_SQRT1_2
#define M_SQRT1_2 0.70710678118654752440084436210485
#endif

/* typedefs */
typedef float	**MATRIX;
typedef double	**DMATRIX;
typedef int	**IMATRIX;

typedef float	*VECTOR;
typedef double	*DVECTOR;
typedef int	*IVECTOR;

typedef struct {
    float	s;
    VECTOR	r;	/* [1..3] */
} QUATERNION;

typedef void (*ODE_DERIVATIVES_ROUTINE)(
    float,
    float *,
    float *);
typedef void (*CURVEFIT_EVALUATION_ROUTINE)(
    void *,
    float *,
    float *,
    float *,
    int);
typedef void (*LFIT_EVALUATION_ROUTINE)(
    float,
    VECTOR,
    int);


/********* FROM NRERROR.C **********/
extern void nrerror(
    char *error_text);
extern void disable_numerical_library_errors(
    void);
extern void enable_numerical_library_errors(
    void);

/********* FROM MATRIX.C ***********/
extern MATRIX matrix(
    int lorow, int hirow,
    int locol, int hicol);
extern MATRIX submatrix(
    MATRIX a,
    int lorow, int hirow,
    int locol, int hicol,
    int newlorow, int newlocol);
extern void free_matrix(
    MATRIX m,
    int lorow, int hirow,
    int locol, int hicol);
extern void free_submatrix(
    MATRIX m,
    int lorow, int hirow,
    int locol, int hicol);
extern MATRIX convert_matrix(
    float *a,
    int lorow, int hirow,
    int locol, int hicol);
extern void free_convert_matrix(
    MATRIX a,
    int lorow, int hirow,
    int locol, int hicol);
extern void transpose_matrix(
    MATRIX m,
    int rlo, int rhi,
    int clo, int chi);
extern VECTOR vector(
    int lo, int hi);
extern void free_vector(
    VECTOR v,
    int lo, int hi);
extern void vector_dual(
    VECTOR v,
    MATRIX M);
extern int compare_matrices(
    MATRIX a, MATRIX b,
    int alorow, int ahirow,
    int alocol, int ahicol,
    int blorow, int blocol);
extern void multiply_matrices(
    MATRIX a, MATRIX b, MATRIX result,
    int alorow, int ahirow,
    int alocol, int ahicol,
    int blorow, int blocol,
    int rlorow, int rlocol);
extern void diag_times_matrix(
    MATRIX d, MATRIX b, MATRIX result,
    int dlorow, int dhirow,
    int dlocol, int dhicol,
    int blorow, int blocol,
    int rlorow, int rlocol);
extern void matrix_times_diag(
    MATRIX a, MATRIX d, MATRIX result,
    int alorow, int ahirow,
    int alocol, int ahicol,
    int dlorow, int dlocol,
    int rlorow, int rlocol);
extern void matrix_times_vector(
    MATRIX a,
    VECTOR b, VECTOR result,
    int alorow, int ahirow,
    int alocol, int ahicol,
    int blo, int rlo);
extern float dot_product(
    VECTOR a, VECTOR b,
    int alo, int ahi, int blo);
extern void cross_product(
    VECTOR a, VECTOR b, VECTOR result,
    int alo, int blo, int rlo);

/********* FROM DMATRIX.C ***********/
extern DMATRIX dmatrix(
    int lorow, int hirow,
    int locol, int hicol);
extern DMATRIX dsubmatrix(
    DMATRIX a,
    int lorow, int hirow,
    int locol, int hicol,
    int newlorow, int newlocol);
extern void free_dmatrix(
    DMATRIX m,
    int lorow, int hirow,
    int locol, int hicol);
extern void free_dsubmatrix(
    DMATRIX m,
    int lorow, int hirow,
    int locol, int hicol);
extern DMATRIX convert_dmatrix(
    double *a,
    int lorow, int hirow,
    int locol, int hicol);
extern void free_convert_dmatrix(
    DMATRIX a,
    int lorow, int hirow,
    int locol, int hicol);
extern DVECTOR dvector(
    int lo,
    int hi);
extern void free_dvector(
    DVECTOR v,
    int lo, int hi);
extern int compare_dmatrices(
    DMATRIX a, DMATRIX b,
    int alorow, int ahirow,
    int alocol, int ahicol,
    int blorow, int blocol);
extern void multiply_dmatrices(
    DMATRIX a, DMATRIX b, DMATRIX result,
    int alorow, int ahirow,
    int alocol, int ahicol,
    int blorow, int blocol,
    int rlorow, int rlocol);
extern void dmatrix_times_dvector(
    DMATRIX a,
    DVECTOR b, DVECTOR result,
    int alorow, int ahirow,
    int alocol, int ahicol,
    int blo, int rlo);
extern double ddot_product(
    DVECTOR a, DVECTOR b,
    int alo, int ahi,
    int blo);
extern void dcross_product(
    DVECTOR a, DVECTOR b, DVECTOR result,
    int alo, int blo, int rlo);


/********* FROM IVECTOR.C ***********/
extern IVECTOR ivector(
    int lo, int hi);
extern void free_ivector(
    IVECTOR v,
    int lo, int hi);

/********* FROM IMATRIX.C ***********/
extern IMATRIX imatrix(
    int lorow, int hirow,
    int locol, int hicol);
extern IMATRIX isubmatrix(
    IMATRIX a,
    int lorow, int hirow,
    int locol, int hicol,
    int newlorow, int newlocol);
extern void free_imatrix(
    IMATRIX m,
    int lorow, int hirow,
    int locol, int hicol);
extern void free_isubmatrix(
    IMATRIX m,
    int lorow, int hirow,
    int locol, int hicol);
extern IMATRIX convert_imatrix(
    int *a,
    int lorow, int hirow,
    int locol, int hicol);
extern void free_convert_imatrix(
    int *a,
    int lorow, int hirow,
    int locol, int hicol);
extern int compare_imatrices(
    IMATRIX a, IMATRIX b,
    int alorow, int ahirow,
    int alocol, int ahicol,
    int blorow, int blocol);
extern void multiply_imatrices(
    IMATRIX a, IMATRIX b, IMATRIX result,
    int alorow, int ahirow,
    int alocol, int ahicol,
    int blorow, int blocol,
    int rlorow, int rlocol);
extern void imatrix_times_ivector(
    IMATRIX a,
    IVECTOR b, IVECTOR result,
    int alorow, int ahirow,
    int alocol, int ahicol,
    int blo, int rlo);
extern int idot_product(
    IVECTOR a, IVECTOR b,
    int alo, int ahi, int blo);
extern void icross_product(
    IVECTOR a, IVECTOR b, IVECTOR result,
    int alo, int blo, int rlo);

/************* FROM LUDCMP.C ************/
extern int LU_decompose(
    MATRIX a,
    int n,
    IVECTOR indx,
    float *parity);
extern void LU_solve(
    MATRIX a,
    int n,
    IVECTOR indx,
    VECTOR b);
extern float LU_determinant(
    MATRIX a,
    int size,
    IVECTOR indx,
    float parity);
extern int LU_invert(
    MATRIX a,
    MATRIX inverse,
    int size,
    IVECTOR indx,
    float parity);
extern int invert_matrix(
    MATRIX a,
    MATRIX inverse,
    int size);

/************* FROM QUATERNION.C ************/
extern QUATERNION *allocate_quaternion(
    void);
extern void free_quaternion(
    QUATERNION *q);
extern void multiply_quaternions(
    QUATERNION *q1,
    QUATERNION *q2,
    QUATERNION *q3);
extern void vector_times_quaternion(
    VECTOR v,
    QUATERNION *q,
    QUATERNION *result);
extern void quaternion_to_matrix(
    QUATERNION *q,
    MATRIX M);
extern void normalize_quaternion(
    QUATERNION *q);
extern void cmatrix4x4_to_quaternion(
    float mat[4][4],
    QUATERNION *q);
extern void quaternion_to_cmatrix4x4(
    QUATERNION *q,
    float mat[4][4]);

/************* FROM ODE.C *****************/
extern int bs_odeint(
    VECTOR ystart,
    int nv,
    float x1,
    float x2,
    float eps,
    float h1,
    float hmin,
    int *nok,
    int *nbad,
    ODE_DERIVATIVES_ROUTINE derivs);

/************* FROM GAUSSJ.C **************/
extern int gaussj(
    MATRIX a,
    int n,
    MATRIX b,
    int m);

/************* FROM INTEGRATE.C *************/
extern float gauss_legendre_integrate(
    float (*func)(float),
    float a, float b);

/************* FROM CURVEFIT.C ************/
extern int lfit(
    VECTOR x,
    VECTOR y,
    VECTOR sig,
    int ndata,
    VECTOR a, int ma,
    IVECTOR lista,
    int mfit,
    float **covar,
    float *chisq,
    LFIT_EVALUATION_ROUTINE funcs);
extern float polynomial_curvefit(
    VECTOR x,
    VECTOR y,
    int ndata,
    VECTOR coeffs,
    int maxorder);
extern float nonlinear_curvefit(
    void *x[],
    float y[],
    int ndata,
    float a[],
    int ma,
    int lista[],
    int mfit,
    CURVEFIT_EVALUATION_ROUTINE funcs);
/* 
 * x[1..ndata],y[1..ndata] are datapoints.
 * a[1..ma] are the coefficients to the equation, initialized to a reasonable
 *   guess.
 * lista[1..ma] numbers the parameters a such that the first "mfit" of them
 *   are adjustable and the rest are fixed.
 * funcs is a function pointer that fits this prototype
 *   void (*funcs)(float x, float a[], float *y_returned, float dyda_returned[],
 *      int na)
 *
 * x can be a pointer to a data vector or data itself -- it's up to your
 * routine to cast it correctly.
 *
 * Return value is chi-squared for the approximation.
 */


/************* FROM ROOT.C ************/
typedef void (*ROOT_ROUTINE)(
    float independent_variable,
    float *function_value,
    float *function_first_derivative_value,
    void  *function_data);

extern float num_findroot(
    ROOT_ROUTINE funcd,
    float x1, float x2,			/* initial bounds */
    float accuracy,			/* find solution within +-accuracy */
    void *function_data);		/* passed to the function */
/* Returns number of real roots */
extern int num_quadratic_solution(
    float c2, float c1, float c0,	/* coefficients */
    float *r0, float *r1);		/* roots */
/* Returns number of real roots */
extern int num_cubic_solution(
    float c3, float c2, float c1, float c0,	/* coefficients */
    float *r0, float *r1, float *r2);		/* roots */



/*************** MACROS *****************/
/* copy matrix source to dest */
#define COPY_MATRIX(source,dest,slorow,shirow,slocol,shicol,dlorow,dlocol)     \
{   int _sr,_sc,_dr,_dc;						       \
    for (_sr=(slorow),_dr=(dlorow); _sr<=(shirow); ++_sr,++_dr)		       \
	for (_sc=(slocol),_dc=(dlocol); _sc<=(shicol); ++_sc,++_dc)	       \
	    dest[_dr][_dc] = source[_sr][_sc];				       \
}


/* copy vector source to dest */
#define COPY_VECTOR(source,dest,slo,shi,dlo) 				       \
{   int _s,_d;								       \
    for (_s=(slo),_d=(dlo); _s<=(shi); ++_s,++_d) dest[_d] = source[_s];       \
}

/*************** 3D VECTOR SHORTCUTS ***********/
#define TRIVECTOR_EPSILON	(1.0e-4)
#define TRIVECTOR_IS_ZERO(v1) \
    (  (v1[1] > -TRIVECTOR_EPSILON) \
    && (v1[1] <  TRIVECTOR_EPSILON) \
    && (v1[2] > -TRIVECTOR_EPSILON) \
    && (v1[2] <  TRIVECTOR_EPSILON) \
    && (v1[3] > -TRIVECTOR_EPSILON) \
    && (v1[3] <  TRIVECTOR_EPSILON))
#define ZERO_TRIVECTOR(v1) \
    v1[1] = 0.0, v1[2] = 0.0, v1[3] = 0.0
#define ZERO_TRIVECTOR(v1) \
    v1[1] = 0.0, v1[2] = 0.0, v1[3] = 0.0
#define LENGTH_TRIVECTOR(v1) \
    HYPOT3(v1[1],v1[2],v1[3])
#define COPY_TRIVECTOR(v2,v1) \
{   v2[1] = v1[1]; \
    v2[2] = v1[2]; \
    v2[3] = v1[3]; \
}
#define ADDTO_TRIVECTOR(v2,v1) \
{   v2[1] += v1[1]; \
    v2[2] += v1[2]; \
    v2[3] += v1[3]; \
}
#define COPY_TRIVECTOR_MULT(v2,v1,mult) \
{   register float _m = (mult); \
    v2[1] = v1[1] * _m; \
    v2[2] = v1[2] * _m; \
    v2[3] = v1[3] * _m; \
}
#define ADDTO_TRIVECTOR_MULT(v2,v1,mult) \
{   register float _m = (mult); \
    v2[1] += v1[1] * _m; \
    v2[2] += v1[2] * _m; \
    v2[3] += v1[3] * _m; \
}
#define ADD_TRIVECTORS(v3,v1,v2) \
{   v3[1] = v1[1] + v2[1]; \
    v3[2] = v1[2] + v2[2]; \
    v3[3] = v1[3] + v2[3]; \
}
#define ADDTO_TRIVECTORS(v3,v1,v2) \
{   v3[1] += v1[1] + v2[1]; \
    v3[2] += v1[2] + v2[2]; \
    v3[3] += v1[3] + v2[3]; \
}
#define ADD_TRIVECTORS_MULT(v3,v1,v2,mult) \
{   register float _m = (mult); \
    v3[1] = (v1[1] + v2[1]) * _m; \
    v3[2] = (v1[2] + v2[2]) * _m; \
    v3[3] = (v1[3] + v2[3]) * _m; \
}
#define ADDTO_TRIVECTORS_MULT(v3,v1,v2,mult) \
{   register float _m = (mult); \
    v3[1] += (v1[1] + v2[1]) * _m; \
    v3[2] += (v1[2] + v2[2]) * _m; \
    v3[3] += (v1[3] + v2[3]) * _m; \
}
#define ADD_TRIVECTORS_2MULT(v3,v1,mult1, v2,mult2) \
{   register float _m1 = (mult1), _m2 = (mult2); \
    v3[1] = v1[1] * _m1 + v2[1] * _m2; \
    v3[2] = v1[2] * _m1 + v2[2] * _m2; \
    v3[3] = v1[3] * _m1 + v2[3] * _m2; \
}
#define ADDTO_TRIVECTORS_2MULT(v3,v1,mult1, v2,mult2) \
{   register float _m1 = (mult1), _m2 = (mult2); \
    v3[1] += v1[1] * _m1 + v2[1] * _m2; \
    v3[2] += v1[2] * _m1 + v2[2] * _m2; \
    v3[3] += v1[3] * _m1 + v2[3] * _m2; \
}
#define DOTPRODUCT_TRIVECTOR(v1,v2) \
    ((v1[1] * v2[1]) + (v1[2] * v2[2]) + (v1[3] * v2[3]))


#if !defined(FAILURE)
# define FAILURE	(-1)
#endif
#if !defined(SUCCESS)
# define SUCCESS	0
#endif

#endif /* _NUMLIB_H_INCLUDED */
