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
#include "hw_dx.h"
#include "hw_vecmath.h"
#include "math.h"

void __dxCamera( CameraArg *Camera, hwSelectionType *sel, int Width, int Height )
{
    float
        FOVY, Distance,
        Left, Right, Bottom, Top,
        Aspect, Near, Far, f;
    D3DXVECTOR3
        camPos, lookPos, up;
    D3DMATRIX
        lookat, frustum;

    camPos.x = Camera->camx;
    camPos.y = Camera->camy;
    camPos.z = Camera->camz;
    lookPos.x = Camera->refx;
    lookPos.y = Camera->refy;
    lookPos.z = Camera->refz;
    up.x = Camera->upx;
    up.y = Camera->upy;
    up.z = Camera->upz;

    D3DXMatrixLookAtLH(&lookat, &camPos, &lookPos, &up);
    IDirect3DDevice9_SetTransform( __hwdxCurrentContext->d3dDevice,
                                   D3DTS_VIEW, (D3DMATRIX *)&lookat);

    FOVY = D3DXToRadian(Camera->field_of_view);

    Aspect = (double)Width / (double)Height;

    /* In PEX, 'Near' and 'Far' are defined in terms of distance from the
    ** view referece point (the thing being looked at) -- the values contained
    ** in the Camera structure are defined similarly.  In OGL (gluPerspective),
    ** the distances are measured from the EYE point, and are always positive.
    */
    Distance  = (lookPos.x - camPos.x)*(lookPos.x - camPos.x);
    Distance += (lookPos.y - camPos.y)*(lookPos.y - camPos.y);
    Distance += (lookPos.z - camPos.z)*(lookPos.z - camPos.z);
    Distance = sqrt(Distance);

    Near = Distance + Camera->front;
    Far =  Distance + Camera->back;

    if( Aspect < 1.0 ) {
        FOVY /= Aspect;
    }

    /* TBD: Parallel cameras... */
    D3DXMatrixPerspectiveFovLH(&frustum, FOVY, Aspect, Near, Far);
    IDirect3DDevice9_SetTransform( __hwdxCurrentContext->d3dDevice,
                                   D3DTS_PROJECTION, (D3DMATRIX *)&frustum);
}

/*** EOF dx_camera.c ***/

