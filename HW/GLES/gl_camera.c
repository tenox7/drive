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

#include "hw.h"
#include "hw_internal.h"
#include "hw_gl.h"
#include "math.h"

static float
    XIdentity[4][4] = {
        -1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    };


void __glCamera( CameraArg *Camera, hwSelectionType *sel, int Width, int Height )
{
    double
        FOVY, Distance,
        Aspect, Near, Far,
        fromX, fromY, fromZ,
        toX, toY,toZ,
        upX, upY, upZ,
        Mag;

    //__android_log_print(ANDROID_LOG_INFO, "HB_HW", "Call glMatrixMode");
    hwGlMatrixMode( HWGL_MODELVIEW );

    fromX = Camera->camx; fromY = Camera->camy; fromZ = Camera->camz;
    toX = Camera->refx; toY = Camera->refy; toZ = Camera->refz;
    upX = Camera->upx; upY = Camera->upy; upZ = Camera->upz;
    hwGlLoadMatrixf( (const float *)XIdentity );
    hwGluLookAt( fromX, fromY, fromZ, toX, toY, toZ, upX, upY, upZ );

    FOVY = Camera->field_of_view;
    Mag =  (fromX - toX) * (fromX - toX);
    Mag += (fromY - toY) * (fromY - toY);
    Mag += (fromZ - toZ) * (fromZ - toZ);
    Distance = sqrt( Mag );

    Aspect = (double)Width / (double)Height;

    /* In PEX, 'Near' and 'Far' are defined in terms of distance from the
    ** view referece point (the thing being looked at) -- the values contained
    ** in the Camera structure are defined similarly.  In OGL
    ** (hwGluPerspective), the distances are measured from the
    ** EYE point, and are always positive.
    */
    Near = Distance + Camera->front;
    Far =  Distance + Camera->back;

    if( Aspect < 1.0 ) {
        FOVY /= Aspect;
    }

    /* TBD: Parallel cameras... */
    hwGlMatrixMode( HWGL_PROJECTION );
    hwGlLoadIdentity();
    if( Camera->mirror ) {
        hwGlScalef(-1.,1.,1.);
        glCullFace( GL_FRONT );
    }
    else {
        glCullFace( GL_BACK );
    }

    if( sel ) {
        GLint viewport[4];
    //__android_log_print(ANDROID_LOG_INFO, "HB_HW", "Call glGetIntegerv");
        glGetIntegerv( GL_VIEWPORT, viewport );
    //__android_log_print(ANDROID_LOG_INFO, "HB_HW", "Call hwGluPickMatrix");
        hwGluPickMatrix( sel->point[0], Height - sel->point[1],
                        sel->aperture, sel->aperture, viewport );
    }

    if(         (Camera->skewX != 0.0)
        ||      (Camera->skewY != 0.0)
        ||      (Camera->jitterX != 0.0)
        ||      (Camera->jitterY != 0.0) )
    {
        double x, y, sx, sy;

        y = Near*sin( (FOVY * 3.141592653589 / 180.0) / 2.0 );
        x = Aspect*y;

        /* Establish jitter */
        sx = 2.0*Camera->jitterX / (float)Width;
        sy = 2.0*Camera->jitterY / (float)Height;
        hwGlTranslatef( sx, sy, 0.0f );

        sx = Camera->skewX * x;
        sy = Camera->skewY * y;

        hwGlFrustum(    (float)(sx - x), (float)(sx + x),
                        (float)(sy - y), (float)(sy + y),
                        (float)(Near),(float)( Far) );
    }
    else {
        hwGluPerspective( FOVY, Aspect, Near, Far );
    }
    hwGlMatrixMode( HWGL_MODELVIEW );
    //__android_log_print(ANDROID_LOG_INFO, "HB_HW", "__glCamera Done");
}

/*** EOF gl_camera.c ***/

