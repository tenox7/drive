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
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "hw.h"
#include "hw_internal.h"

void __hwIntDrawButton( hwDisplay disp,
                        char *label, hwInt32 size, TexFont *txf,
                        hwInt32 fg, hwInt32 bg, hwInt32 border, hwInt32 tm,
                        hwInt32 bbox[], hwInt32 parentBbox[],
                        hwInt32 align,
                        hwInt32 checkBox )
{
    hwInt32
        hAlign, vAlign,
        flags,
        ax, ay, aw;
    char
        *ind;
    hwInt32
        borderPoly[5*2],
        i, numSub,
        cx, cy,
        x, y, w, h;
    char
        *ptr,
        sub[1024];       /* TBD: define this max somewhere */

    x = bbox[0] + parentBbox[0];
    y = bbox[1] + parentBbox[1];
    w = bbox[2];
    h = bbox[3];

    if( bg || (tm >= 0) ) {
        if( tm >= 0 ) {
            disp->guiRaster( disp, x, y, w, h, bg, tm );
        }
        else {
            flags = ((bg & 0xFF000000) != 0xFF000000) ? HW_GUI_BLEND : 0;
            disp->guiRectangle( disp, flags, bg, x, y, w, h, 0 );
        }
    }

    if( border ) {
        borderPoly[0] = x;          borderPoly[1] = y;
        borderPoly[2] = x + w - 1;  borderPoly[3] = y;
        borderPoly[4] = x + w - 1;  borderPoly[5] = y + h - 1;
        borderPoly[6] = x;          borderPoly[7] = y + h - 1;
        borderPoly[8] = x;          borderPoly[9] = y;
        flags = ((border & 0xFF000000) != 0xFF000000) ? HW_GUI_BLEND : 0;
        disp->guiPolyline( disp, flags, border, 5, borderPoly );
    }

    /* If no foreground color, don't draw foreground */
    if( !fg ) return;

    ax = x; ay = y; aw = w;

    if( checkBox >= 0 ) {
        /* Make room for the check box */
        if( align & HW_GUI_TOGGLE_RIGHT ) {
            cx = ax + aw - 24;
        }
        else {
            cx = ax + 8;
            ax = x + 32;
        }
        cy = y + h / 2 - 8;
        aw -= 32;
    }

    /* Figure out how many substrings we have */
    numSub = 1;
    for( i = 0; label[i]; i++ ) {
        if( label[i] == '\n' ) {
            numSub++;
        }
    }

    /* Resize the font if needed */
    if( align & HW_GUI_FONT_FRACTIONAL ) {
        size = (size * bbox[3] + 500) / 1000;
    }
    else if( align & HW_GUI_FONT_FRACT_W ) {
        size = (size * bbox[2] + 500) / 1000;
    }

    /* Locate the text baseline start point */
    if( align & HW_GUI_LABEL_RIGHT )            ax += aw;
    else if( !(align & HW_GUI_LABEL_LEFT) )     ax += aw / 2;

    if( align & HW_GUI_LABEL_BOTTOM )           ay += h;
    else if( !(align & HW_GUI_LABEL_TOP) )      ay += (h-(numSub-1)*size) / 2;

    if( align & HW_GUI_LABEL_LEFT )             hAlign = HW_TEXT_ALIGN_LEFT;
    else if( align & HW_GUI_LABEL_RIGHT )       hAlign = HW_TEXT_ALIGN_RIGHT;
    else                                        hAlign = HW_TEXT_ALIGN_CENTER;

    if( align & HW_GUI_LABEL_TOP )              vAlign = HW_TEXT_ALIGN_TOP;
    else if( align & HW_GUI_LABEL_BOTTOM )      vAlign = HW_TEXT_ALIGN_BOTTOM;
    else                                        vAlign = HW_TEXT_ALIGN_CENTER;

    for( i = 0; i < numSub; i++ ) {
        strcpy(sub, label);
        if( i < (numSub - 1) ) {
            ptr = strchr(sub, '\n');
            *ptr = 0;
            label += (ptr - sub) + 1;
        }
        disp->guiText( disp, txf, fg, hAlign, vAlign, size, ax, ay,
                       (unsigned char *)sub );
        ay += size;
    }


    if( checkBox >= 0 ) {
        disp->guiRaster( disp, cx, cy, 16, 16, fg, checkBox );
    }
}

void __hwIntGetParentBounds( hwDisplay disp, hwObject parent,
                             hwInt32 parentBounds[] )
{
    hwInt32
        *pBounds = NULL,
        bbox[4];
    void
        *ptr;

    /* Get the context bounding box */
    if( parent ) {
        if( parent->inquire( parent, hwStrBounds, &ptr ) == HW_TYPE_4I ) {
            pBounds = ptr;
        }
    }
    if( !pBounds ) {
        pBounds = bbox;
        bbox[0] = bbox[1] = 0;
        if( disp ) {
            disp->getDrawSize( disp, bbox+2 );
        }
        else {
            /* No display. Assume VGA res. */
            bbox[1] = 640;
            bbox[2] = 480;
        }
    }
    parentBounds[0] = pBounds[0];
    parentBounds[1] = pBounds[1];
    parentBounds[2] = pBounds[2];
    parentBounds[3] = pBounds[3];

    /* Traverse up the tree to get the cumulative position */
    while( parent ) {
        if( parent->inquire( parent, hwStrParent, &ptr ) == HW_TYPE_OBJECT ) {
            parent = ptr;
            if( parent &&
                (parent->inquire( parent, hwStrBounds, &ptr ) == HW_TYPE_4I) )
            {
                pBounds = ptr;
                parentBounds[0] += pBounds[0];
                parentBounds[1] += pBounds[1];
            }
        }
        else {
            parent = NULL;
        }
    }
}

void __hwIntPositionWidget( hwInt32 bounds[],
                            hwInt32 parentBounds[],
                            hwInt32 posX, hwInt32 posY,
                            hwInt32 width, hwInt32 height,
                            hwInt32 align,
                            hwFloat aspectRatio )
{
    hwFloat
        currAspect;
    hwInt32
        prev;

    /* First, adjust request if fractional */
    if( align & HW_GUI_FRACTIONAL ) {
        posX = (posX * parentBounds[2] + 500) / 1000;
        posY = (posY * parentBounds[3] + 500) / 1000;
        width = (width * parentBounds[2] + 500) / 1000;
        height = (height * parentBounds[3] + 500) / 1000;
    }

    /* Now, adjust position */
    if( align & HW_GUI_REL_RIGHT )  posX = parentBounds[2] - posX;
    if( align & HW_GUI_CENTER )     posX -= width / 2;

    if( align & HW_GUI_REL_BOT )    posY = parentBounds[3] - posY;
    if( align & HW_GUI_CENTER )     posY -= height / 2;

    /* ...and width */
    if( align & HW_GUI_REL_WIDTH )  width = parentBounds[2] - width;
    if( align & HW_GUI_REL_HEIGHT ) height = parentBounds[3] - height;

    if( aspectRatio > 0.0 ) {
        currAspect = width / (hwFloat)height;
        if( currAspect > aspectRatio ) {
            prev = width;
            width = (hwInt32)((height * aspectRatio) + 0.5);
            posX += (prev - width) / 2;
        }
        else {
            prev = height;
            height = (hwInt32)((width / aspectRatio) + 0.5);
            posY += (prev - height) / 2;
        }
    }

    bounds[0] = posX;
    bounds[1] = posY;
    bounds[2] = width;
    bounds[3] = height;
}

/*** EOF hwGuiUtil.c ***/
