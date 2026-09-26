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
#include "physics.h"
#include "object.h"
#include "demo_physics.h"

#define D_ANGLE		(2.0*M_PI/180.0)
#define D_VELOCITY	(80.0)
#define V_MAX		(100.0)

#define MAT obj->xform

RETURN_CONDITION simple_physics(
    DRIVE_OBJECT *obj,
    float t_interval)
{
	double angle;
	float x,y,z;
	float velocity;
	float dx,dy,dz;
	float Ax,Ay,Az;
	float sinphi, sinalpha,sintheta;
	float cosphi, cosalpha,costheta;
	WC_SURFACE_CHARACTERISTICS sc;


	/* turn */
	if (obj->controls.pointer_x != 0.0) {
		angle = D_ANGLE * obj->controls.pointer_x;
		obj->sdata.angle += angle;
	}

	/* speed */
	/*
	velocity = FSQRT(pobj->v[XD]*pobj->v[XD]
	               + pobj->v[ZD]*pobj->v[ZD]);
	if (obj->controls.pointer_y != 0.0) {
		velocity += D_VELOCITY*obj->controls.pointer_y;
		if (velocity < 0.0) velocity = 0.0;
	}
	*/

	velocity = D_VELOCITY*obj->controls.pointer_y;

	if (velocity > V_MAX) velocity = V_MAX;
	x = 0.0;
	y = 0.0;
	z = 1.0;

    	dx =  MAT[2][0] * z; 
    	dy =  MAT[2][1] * z;
    	dz =  MAT[2][2] * z;

	obj->xform[3][0] += dx * velocity * t_interval;
	obj->xform[3][1] += dy * velocity * t_interval;
	obj->xform[3][2] += dz * velocity * t_interval;

	surface_chars(obj,MAT[3][0],MAT[3][1],MAT[3][2],&sc);

	/* put it on top of the object is is on */
	obj->xform[3][1] = sc.wc_y;


	/* Now, decide the car's new rotation matrix based on the user's input
	 * and the normal of the ground at this point */
	
        Ax = sc.wc_normal[0];
        Ay = sc.wc_normal[1];
        Az = sc.wc_normal[2];

	sinphi = Ax;
	cosphi = Ay;

	sinalpha = Az;
	cosalpha = Ay;

	sintheta = FSIN( (obj->sdata.angle) );
	costheta = FCOS( (obj->sdata.angle) );

	if( fabs((double)sinphi) + fabs((double)cosphi) < 0.15 )
	{
	    /* the normal is too close to the Z axis...don't rotate 
	     * about Z.
	     */
	    MAT[0][0] =  costheta;
	    MAT[0][1] = sintheta * sinalpha;
	    MAT[0][2] = sintheta * cosalpha;
	    MAT[0][3] = 0.0;

	    MAT[1][0] = 0.0;
	    MAT[1][1] = cosalpha;
	    MAT[1][2] = - sinalpha;
	    MAT[1][3] = 0.0;

	    MAT[2][0] = - sintheta;
	    MAT[2][1] = sinalpha * costheta;
	    MAT[2][2] = cosalpha * costheta;
	    MAT[2][3] = 0.0;
		
	}
	else
	{

	    MAT[0][0] =  costheta*cosphi + sintheta * sinphi * sinalpha;
	    MAT[0][1] =  -(sinphi * costheta) + sinalpha * cosphi * sintheta;
	    MAT[0][2] = cosalpha * sintheta;
	    MAT[0][3] = 0;

	    MAT[1][0] = sinphi * cosalpha;
	    MAT[1][1] = cosalpha * cosphi;
	    MAT[1][2] = -sinalpha;
	    MAT[1][3] = 0;

	    MAT[2][0] = -(cosphi * sintheta) + sinphi*sinalpha*costheta;
	    MAT[2][1] = sinphi*sintheta + sinalpha * cosphi * costheta;
	    MAT[2][2] = -(cosalpha * costheta); /* negate to make L-handed*/
	    MAT[2][3] = 0;
	}

	/* leave the translation terms alone */

	/* now, find the normal  by transforming (0,1,0) by the
	 * new transformation matrix 
	 */

	x = 0.0;
	y = 1.0;
	z = 0.0;

/*
	obj->sdata.normal[0] = MAT[0][0]*x+MAT[1][0]*y+MAT[2][0]*z;
	obj->sdata.normal[1] = MAT[0][1]*x+MAT[1][1]*y+MAT[2][1]*z;
	obj->sdata.normal[2] = MAT[0][2]*x+MAT[1][2]*y+MAT[2][2]*z;
*/
	obj->sdata.normal[0] = sc.wc_normal[0];
	obj->sdata.normal[1] = sc.wc_normal[1];
	obj->sdata.normal[2] = sc.wc_normal[2];

	/* now, find the reference point by transforming (0,0,100) by the
	 * new transformation matrix 
	 */
	
	x = 0.0;
	y = 0.0;
	z = 100.0;
	obj->sdata.ref[0] = MAT[0][0]*x+MAT[1][0]*y+MAT[2][0]*z+MAT[3][0];
	obj->sdata.ref[1] = MAT[0][1]*x+MAT[1][1]*y+MAT[2][1]*z+MAT[3][1];
	obj->sdata.ref[2] = MAT[0][2]*x+MAT[1][2]*y+MAT[2][2]*z+MAT[3][2];

	return(RETURN_OK);
}
