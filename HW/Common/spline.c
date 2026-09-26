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

#include <stdlib.h>
#include <math.h>
#include "hw.h"
#include "hw_internal.h"

/******************************************************************************
 * Math code to create interpolated Bezier segments from polyline
 * control points.
 *****************************************************************************/
#define INTERP( A1, A2, U )     ((A1) + ((A2) - (A1)) * (U))

static void Forward
( 
    hwFloat (*Mat)[5], hwInt32 Num, 
    hwFloat *XVec, hwFloat *YVec, hwFloat *ZVec
)
{
    hwInt32
        I;
    hwFloat
        Ratio;

    for( I = 0; I < Num - 2; I++ ) {
        /* Fill up the band and the last column */
        Ratio = Mat[I+1][1] / Mat[I][2];
        Mat[I+1][1] = 0.0;
        Mat[I+1][2] -= Ratio * Mat[I][3];
        Mat[I+1][4] -= Ratio * Mat[I][4];
        XVec[ I+1 ] -= Ratio * XVec[ I ];
        YVec[ I+1 ] -= Ratio * YVec[ I ];
        ZVec[ I+1 ] -= Ratio * ZVec[ I ];

        /* Fix the last row */
        Ratio = Mat[I][0] / Mat[I][2];
        Mat[I][0] = 0.0;
        Mat[I+1][0] -= Ratio * Mat[I][3];
        Mat[Num-1][0] -= Ratio * Mat[I][4];
        XVec[ Num - 1 ] -= Ratio * XVec[ I ];
        YVec[ Num - 1 ] -= Ratio * YVec[ I ];
        ZVec[ Num - 1 ] -= Ratio * ZVec[ I ];
    }

    /* Fix the last number in the last column */
    Ratio = Mat[Num-2][0] / Mat[Num-2][2];
    Mat[Num-2][0] = 0.0;
    Mat[Num-1][0] -= Ratio * Mat[Num-2][4];
    XVec[ Num - 1 ] -= Ratio * XVec[ Num - 2 ];
    YVec[ Num - 1 ] -= Ratio * YVec[ Num - 2 ];
    ZVec[ Num - 1 ] -= Ratio * ZVec[ Num - 2 ];
}


static void Backward
(
    hwFloat (*Mat)[5],
    hwInt32 Num,
    hwFloat *XVec, hwFloat *YVec, hwFloat *ZVec,
    hwFloat *XRes, hwFloat *YRes, hwFloat *ZRes
)
{
    hwFloat
        Sum;
    hwInt32
        I;

    XRes[ Num - 1 ] = XVec[ Num - 1 ] / Mat[Num-1][0];
    YRes[ Num - 1 ] = YVec[ Num - 1 ] / Mat[Num-1][0];
    ZRes[ Num - 1 ] = ZVec[ Num - 1 ] / Mat[Num-1][0];

    for( I = Num - 2; I >= 0; I-- ) {
        Sum  = XRes[ Num - 1 ] * Mat[I][4];
        Sum += XRes[ I + 1 ] * Mat[I][3];
        XRes[ I ] = (XVec[ I ] - Sum) / Mat[I][2];

        Sum  = YRes[ Num - 1 ] * Mat[I][4];
        Sum += YRes[ I + 1 ] * Mat[I][3];
        YRes[ I ] = (YVec[ I ] - Sum) / Mat[I][2];

        Sum  = ZRes[ Num - 1 ] * Mat[I][4];
        Sum += ZRes[ I + 1 ] * Mat[I][3];
        ZRes[ I ] = (ZVec[ I ] - Sum) / Mat[I][2];
    }
}


static hwInt32
    MaxCount;
static hwFloat
    *XVec = 0, *YVec = 0, *ZVec = 0,
    *XRes = 0, *YRes = 0, *ZRes = 0,
    (*Mat)[5] = 0;

hwInt32 __hwIntCalcOpen
(
    hwFloat *Joints,
    hwInt32 Count,
    hwFloat *BPoints
)
{
    hwInt32
        I, N;

    if( Count > MaxCount ) {
        XVec = realloc( XVec, Count * sizeof( hwFloat ) );
        YVec = realloc( YVec, Count * sizeof( hwFloat ) );
        ZVec = realloc( ZVec, Count * sizeof( hwFloat ) );
        XRes = realloc( XRes, Count * sizeof( hwFloat ) );
        YRes = realloc( YRes, Count * sizeof( hwFloat ) );
        ZRes = realloc( ZRes, Count * sizeof( hwFloat ) );
        Mat = realloc( Mat, Count * 5 * sizeof( hwFloat ) );
        MaxCount = Count;
    }

    if( !XVec || !YVec || !ZVec || !XRes || !YRes || !ZRes || !Mat ) {
        return -1;
    }

    for( I = 0; I < Count; I++ ) {
        XVec[ I ] = 6.0 * Joints[ 3 * I ];
        YVec[ I ] = 6.0 * Joints[ 3 * I + 1 ];
        ZVec[ I ] = 6.0 * Joints[ 3 * I + 2 ];
    }

    for( I = 1; I < Count - 1; I++ ) {
        Mat[I][1] = 1.0;
        Mat[I][2] = 4.0;
        Mat[I][3] = 1.0;
    }
    Mat[0][1] = 0.0; Mat[0][2] = 6.0; Mat[0][3] = 0.0;
    Mat[Count-2][3] = 0.0;

    for( I = 0; I < Count - 2; I++ ) {
        Mat[I][4] = 0.0;
    }
    Mat[Count-2][4] = 1.0;

    for( I = 0; I < Count - 1; I++ ) {
        Mat[I][0] = 0.0;
    }
    Mat[Count-1][0] = 6.0;

    Forward( Mat, Count, XVec, YVec, ZVec );
    Backward( Mat, Count, XVec, YVec, ZVec, XRes, YRes, ZRes );

    for( I = N = 0; I < Count - 1; I++, N += 9 ) {
        BPoints[ N   ] = Joints[ I*3 ];
        BPoints[ N+1 ] = Joints[ I*3 + 1 ];
        BPoints[ N+2 ] = Joints[ I*3 + 2 ];

        BPoints[ N+3 ] = INTERP( XRes[ I ], XRes[ (I+1)%Count ], 0.333333333 );
        BPoints[ N+4 ] = INTERP( YRes[ I ], YRes[ (I+1)%Count ], 0.333333333 );
        BPoints[ N+5 ] = INTERP( ZRes[ I ], ZRes[ (I+1)%Count ], 0.333333333 );

        BPoints[ N+6 ] = INTERP( XRes[ I ], XRes[ (I+1)%Count ], 0.666666667 );
        BPoints[ N+7 ] = INTERP( YRes[ I ], YRes[ (I+1)%Count ], 0.666666667 );
        BPoints[ N+8 ] = INTERP( ZRes[ I ], ZRes[ (I+1)%Count ], 0.666666667 );
    }
    BPoints[ N   ] = Joints[ I*3 ];
    BPoints[ N+1 ] = Joints[ I*3 + 1 ];
    BPoints[ N+2 ] = Joints[ I*3 + 2 ];

    return (Count - 1) * 3;
}


hwInt32 __hwIntCalcClosed
(
    hwFloat *Joints,
    hwInt32 Count,
    hwFloat *BPoints
)
{
    hwInt32
        I, N;

    if( Count > MaxCount ) {
        XVec = realloc( XVec, Count * sizeof( hwFloat ) );
        YVec = realloc( YVec, Count * sizeof( hwFloat ) );
        ZVec = realloc( ZVec, Count * sizeof( hwFloat ) );
        XRes = realloc( XRes, Count * sizeof( hwFloat ) );
        YRes = realloc( YRes, Count * sizeof( hwFloat ) );
        ZRes = realloc( ZRes, Count * sizeof( hwFloat ) );
        Mat = realloc( Mat, Count * 5 * sizeof( hwFloat ) );
        MaxCount = Count;
    }

    if( !XVec || !YVec || !ZVec || !XRes || !YRes || !ZRes || !Mat ) {
        return -1;
    }

    for( I = 0; I < Count; I++ ) {
        XVec[ I ] = 6.0 * Joints[ 3 * I ];
        YVec[ I ] = 6.0 * Joints[ 3 * I + 1 ];
        ZVec[ I ] = 6.0 * Joints[ 3 * I + 2 ];
    }

    for( I = 0; I < Count - 1; I++ ) {
        Mat[I][1] = 1.0;
        Mat[I][2] = 4.0;
        Mat[I][3] = 1.0;
    }
    Mat[0][1] = 0.0;
    Mat[Count-2][3] = 0.0;

    Mat[0][4] = 1.0;
    Mat[Count-2][4] = 1.0;
    for( I = 1; I < Count - 2; I++ ) {
        Mat[I][4] = 0.0;
    }

    Mat[0][0] = 1.0;
    for( I = 1; I < Count - 2; I++ ) {
        Mat[I][0] = 0.0;
    }
    Mat[Count-2][0] = 1.0;
    Mat[Count-1][0] = 4.0;

    Forward( Mat, Count, XVec, YVec, ZVec );
    Backward( Mat, Count, XVec, YVec, ZVec, XRes, YRes, ZRes );

    for( I = N = 0; I < Count; I++, N += 9 ) {
        BPoints[ N   ] = Joints[ I*3 ];
        BPoints[ N+1 ] = Joints[ I*3 + 1 ];
        BPoints[ N+2 ] = Joints[ I*3 + 2 ];
        BPoints[ N+3 ] = INTERP( XRes[ I ], XRes[ (I+1)%Count ], 0.333333333 );
        BPoints[ N+4 ] = INTERP( YRes[ I ], YRes[ (I+1)%Count ], 0.333333333 );
        BPoints[ N+5 ] = INTERP( ZRes[ I ], ZRes[ (I+1)%Count ], 0.333333333 );
        BPoints[ N+6 ] = INTERP( XRes[ I ], XRes[ (I+1)%Count ], 0.666666667 );
        BPoints[ N+7 ] = INTERP( YRes[ I ], YRes[ (I+1)%Count ], 0.666666667 );
        BPoints[ N+8 ] = INTERP( ZRes[ I ], ZRes[ (I+1)%Count ], 0.666666667 );
    }
    BPoints[ N   ] = BPoints[ 0 ];
    BPoints[ N+1 ] = BPoints[ 1 ];
    BPoints[ N+2 ] = BPoints[ 2 ];

    return Count * 3;
}

void __hwIntBezier
(
    hwFloat *BPoints,
    hwFloat U,
    hwFloat *RX, hwFloat *RY, hwFloat *RZ,
    hwFloat *TX, hwFloat *TY, hwFloat *TZ
)
{
    hwFloat
        P1, P2, P3,
        E1, E2,
        TTX, TTY, TTZ;

    P1 = INTERP( BPoints[ 0 ], BPoints[ 3 ], U );
    P2 = INTERP( BPoints[ 3 ], BPoints[ 6 ], U );
    P3 = INTERP( BPoints[ 6 ], BPoints[ 9 ], U );
    E1 = INTERP( P1, P2, U );
    E2 = INTERP( P2, P3, U );
    *RX = INTERP( E1, E2, U );
    TTX = E2 - *RX;
    if( TTX == 0.0 )    TTX = *RX - E1;

    P1 = INTERP( BPoints[ 1 ], BPoints[ 4 ], U );
    P2 = INTERP( BPoints[ 4 ], BPoints[ 7 ], U );
    P3 = INTERP( BPoints[ 7 ], BPoints[ 10 ], U );
    E1 = INTERP( P1, P2, U );
    E2 = INTERP( P2, P3, U );
    *RY = INTERP( E1, E2, U );
    TTY = E2 - *RY;
    if( TTY == 0.0 )    TTY = *RY - E1;

    P1 = INTERP( BPoints[ 2 ], BPoints[ 5 ], U );
    P2 = INTERP( BPoints[ 5 ], BPoints[ 8 ], U );
    P3 = INTERP( BPoints[ 8 ], BPoints[ 11 ], U );
    E1 = INTERP( P1, P2, U );
    E2 = INTERP( P2, P3, U );
    *RZ = INTERP( E1, E2, U );

    TTZ = E2 - *RZ;
    if( TTZ == 0.0 )    TTZ = *RZ - E1;

    if( TX && TY && TZ ) {
        P1 = sqrt( TTX*TTX + TTY*TTY + TTZ*TTZ );
        if( P1 != 0.0 ) P1 = 1.0 / P1;
        *TX = TTX * P1; *TY = TTY * P1; *TZ = TTZ * P1;
    }
}

void __hwIntSplinePos
(
    hwFloat *spl,
    hwFloat *times,
    hwInt32 n,
    hwFloat percent,
    hwFloat result[3]
)
{
    hwFloat
        *Seg, WhichSegf, fraction;
    hwInt32
        i, WhichSeg;

    n /= 3;

    if( times ) {
        for( i = 1; i < n; i++ ) {
            if( times[i] >= percent ) {
                break;
            }
        }
        i--;
        fraction = (percent - times[i]) / (times[i+1] - times[i]);
        WhichSeg = i;
    }
    else {
        WhichSegf = percent * n * (1.0 / 100.0);
        WhichSeg = (int)WhichSegf;  /* Truncate */
        fraction = WhichSegf - WhichSeg;   /* Get the decimal fraction */
    }
    Seg = spl + (WhichSeg * 9); 
    __hwIntBezier( Seg, fraction, &result[0], &result[1], &result[2],
                        (hwFloat *)0, (hwFloat *)0, (hwFloat *)0 );
}

/*** EOF spline.c ***/
