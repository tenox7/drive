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

#include <math.h>
#include "hwedit.h"

#define EPSILON 0.00001

void transform3x3( float Mat[3][3], float Vect[3] )
{
    float
        a, b, c;

    a = Vect[0]; b = Vect[1]; c = Vect[2];
    Vect[0] = a*Mat[0][0] + b*Mat[1][0] + c*Mat[2][0];
    Vect[1] = a*Mat[0][1] + b*Mat[1][1] + c*Mat[2][1];
    Vect[2] = a*Mat[0][2] + b*Mat[1][2] + c*Mat[2][2];
}

void getRot( hwFloat XYZ[3], hwFloat Angle, hwFloat Mat[3][3] )
{
    hwFloat
        S, C, T,
        X, Y, Z;

    S = sin(Angle);
    C = cos(Angle);
    T = 1.0 - C;
    X = XYZ[0]; Y = XYZ[1]; Z = XYZ[2];
    Mat[0][0] = T*X*X+C;   Mat[1][0] = T*X*Y+S*Z; Mat[2][0] = T*X*Z-S*Y;
    Mat[0][1] = T*X*Y-S*Z; Mat[1][1] = T*Y*Y+C;   Mat[2][1] = T*Y*Z+S*X;
    Mat[0][2] = T*X*Z+S*Y; Mat[1][2] = T*Y*Z-S*X; Mat[2][2] = T*Z*Z+C;
}

void vecCross( hwFloat R[3], hwFloat A[3], hwFloat B[3] )
{
    double
        d, x, y, z;

    x = A[1]*B[2] - B[1]*A[2];
    y = A[2]*B[0] - B[2]*A[0];
    z = A[0]*B[1] - B[0]*A[1];
    d = sqrt( x*x + y*y + z*z );
    if( d < EPSILON ) {
        R[0] = A[0]; R[1] = A[1]; R[2] = A[2];
    }
    else {
        d = 1.0 / d;
        R[0] = x*d; R[1] = y*d; R[2] = z*d;
    }
}

void yawCam( hwFloat Angle )
{
    hwFloat
        Mat[3][3], d;

    getRot( edit.camUp, Angle, Mat );
    transform3x3( Mat, edit.camDir );
    d = 1.0 / sqrt( edit.camDir[0]*edit.camDir[0] +
                        edit.camDir[1]*edit.camDir[1] +
                        edit.camDir[2]*edit.camDir[2] );
    edit.camDir[0] *= d; edit.camDir[1] *= d; edit.camDir[2] *= d;
    vecCross( edit.camRight, edit.camUp, edit.camDir );
}

void pitchCam( hwFloat Angle )
{
    hwFloat
        Mat[3][3], d;

    getRot( edit.camRight, Angle, Mat );
    transform3x3( Mat, edit.camDir );
    d = 1.0 / sqrt( edit.camDir[0]*edit.camDir[0] +
                        edit.camDir[1]*edit.camDir[1] +
                        edit.camDir[2]*edit.camDir[2] );
    edit.camDir[0] *= d; edit.camDir[1] *= d; edit.camDir[2] *= d;
    vecCross( edit.camUp, edit.camDir, edit.camRight );
}

void rollCam( hwFloat Angle )
{
    hwFloat
        Mat[3][3], d;

    getRot( edit.camDir, Angle, Mat );
    transform3x3( Mat, edit.camUp );
    d = 1.0 / sqrt( edit.camUp[0]*edit.camUp[0] +
                        edit.camUp[1]*edit.camUp[1] +
                        edit.camUp[2]*edit.camUp[2] );
    edit.camUp[0] *= d; edit.camUp[1] *= d; edit.camUp[2] *= d;
    vecCross( edit.camRight, edit.camUp, edit.camDir );
}

void setCam( void )
{
    hwFloat
        vec[3];

    vec[0] = edit.camPos[0] - edit.camDir[0]*edit.camDist;
    vec[1] = edit.camPos[1] - edit.camDir[1]*edit.camDist;
    vec[2] = edit.camPos[2] - edit.camDir[2]*edit.camDist;
    edit.cam->modify( edit.cam, hwStrPos, HW_TYPE_3F, vec );
    edit.cam->modify( edit.cam, hwStrDir, HW_TYPE_3F, edit.camDir );
    edit.cam->modify( edit.cam, hwStrUp, HW_TYPE_3F, edit.camUp );
}
