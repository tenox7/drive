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

static hwObject create( hwObject );
static void addref( hwObject );
static void destroy( hwObject );
static void modify( hwObject, const char *, hwInt32 type, const void * );
static hwInt32 inquire( hwObject, const char *, void ** );
static void draw( hwObject );

static const char
    *propList[] = {
        /* Specific to hwText2D */
        hwStrPos,
        hwStrScale,
        hwStrColor,
        hwStrTransparency,
        hwStrLabel,
        hwStrFont,
        hwStrFontHeight,
        hwStrAlign,

        /* End of list */
        0
    };

#define HW_POS          1
#define HW_FONT_HEIGHT  2
#define HW_LABEL        3
#define HW_COLOR        4
#define HW_FONT         5
#define HW_ALIGN        6
#define HW_TRANSPARENCY 7
#define HW_SCALE        8

static struct _hwObjectStruct
    hwText2DStruct = {
        0,      /* Parent - NULL */
        "hwText2D",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwText2D = &hwText2DStruct;

/* Hash table for text2d strings */
static void *text2dTab = 0;

typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount;               /* Reference count */
    hwObject
        parent;                 /* Parent container */
    hwFloat
        pos[3],                 /* Posiiton */
        scale[3],               /* Size */
        fontHeight;             /* Height of the font chars */
    hwInt32
        dirty,                  /* Modified but not cooked? */
        color,                  /* Color */
        align;                  /* Alignment flags */
    TexFont
        *font;                  /* The font to use */
    hwObject
        fontObj;                /* The font object it came from */
    char
        *label;                 /* Child widgets */
    hwCallback
        callback;               /* Thing to call when text2d hit */
} HwText2D;

static void cook( hwDisplay disp, HwText2D *text2d );

static hwObject create( hwObject text2d )
{
    HwText2D
        *result;
    void
        *t;

    if( text2d != hwText2D )      return 0;

    HW_HASH_SETUP(text2dTab, 16)
        t = text2dTab;
        HW_INSERT( hwStrPos,          HW_POS,          t );
        HW_INSERT( hwStrFontHeight,   HW_FONT_HEIGHT,  t );
        HW_INSERT( hwStrLabel,        HW_LABEL,        t );
        HW_INSERT( hwStrColor,        HW_COLOR,        t );
        HW_INSERT( hwStrTransparency, HW_TRANSPARENCY, t );
        HW_INSERT( hwStrFont,         HW_FONT,         t );
        HW_INSERT( hwStrAlign,        HW_ALIGN,        t );
        HW_INSERT( hwStrScale,        HW_SCALE,        t );
    HW_HASH_CLEANUP

    result = malloc( sizeof(HwText2D) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }
    memset(result, 0, sizeof(HwText2D));

    /* Class-specific initialization goes here */

    result->hdr = *text2d;
    result->hdr.parent = hwText2D;
    result->hdr.name = 0;

    result->refCount = 1;

    result->fontHeight = 24.0;
    result->pos[0] = 0.;
    result->pos[1] = 0.;
    result->pos[2] = 0.;
    result->scale[0] = 0.;
    result->scale[1] = 0.;
    result->scale[2] = 0.;
    result->color = 0xFFFFFFFF;
    result->label = NULL;
    result->font = NULL;
    result->fontObj = NULL;
    result->align = 0;
    result->dirty = 1;

    return (hwObject)result;
}

static void addref( hwObject obj )
{
    HwText2D
        *text2d = (HwText2D *)obj;;

    /* This method only rarely needs to be changed */
    text2d->refCount++;
}

static void destroy( hwObject obj )
{
    HwText2D
        *text2d = (HwText2D *)obj;
    int
        i, n;

    if( --text2d->refCount > 0 ) return;

    if( text2d->fontObj ) {
        text2d->fontObj->destroy( text2d->fontObj );
    }
    if( text2d->label ) {
        free( text2d->label );
    }
    free( text2d );
}

static void modify
(
    hwObject obj, const char *prop,
    hwInt32 type, const void *val
)
{
    HwText2D
        *text2d = (HwText2D *)obj;
    const char 
        *str;
    hwFloat
        *pFloat;
    hwInt32
        trans,
        dirty = 1;

    switch( hwLookup( prop, text2dTab ) ) {
    case HW_POS :
        if( type != HW_TYPE_3F )        goto BadType;
        text2d->pos[0] = ((hwFloat *)val)[0];
        text2d->pos[1] = ((hwFloat *)val)[1];
        text2d->pos[2] = ((hwFloat *)val)[2];
        break;
    case HW_SCALE :
        if( type != HW_TYPE_3F )        goto BadType;
        text2d->scale[0] = ((hwFloat *)val)[0];
        text2d->scale[1] = ((hwFloat *)val)[1];
        text2d->scale[2] = ((hwFloat *)val)[2];
        break;
    case HW_FONT_HEIGHT :
        if( type == HW_TYPE_1I )        text2d->fontHeight = *(hwInt32 *)val;
        if( type == HW_TYPE_1F )        text2d->fontHeight = *(hwFloat *)val;
        else                            goto BadType;
        break;
    case HW_ALIGN :
        if( type == HW_TYPE_1F )        text2d->align = *(hwFloat *)val;
        else if( type == HW_TYPE_1I )   text2d->align = *(hwInt32 *)val;
        else                            goto BadType;
        break;
    case HW_LABEL :
        if( type != HW_TYPE_STRING )    goto BadType;
        str = val;
        if( text2d->label ) free( text2d->label );
        text2d->label = malloc( strlen(val) + 1 );
        if( !text2d->label ) {
            __hwIntSetError( HW_ERROR_NO_MEMORY );
            return;
        }
        strcpy( text2d->label, str );
        break;
    case HW_COLOR :
        text2d->color = __hwIntGuiColor( type, val );
        break;
    case HW_TRANSPARENCY :
        if( type == HW_TYPE_1F )      trans = 255*(1.0 - *(hwFloat *)val);
        else if( type == HW_TYPE_1I ) trans = 255 - *(hwInt32 *)val;
        else                          goto BadType;
        if( trans < 0 ) trans = 0;
        if( trans > 255 ) trans = 255;
        text2d->color &= 0x00FFFFFF;
        text2d->color |= trans << 24;
        break;
    case HW_FONT :
        if( type != HW_TYPE_OBJECT )    goto BadType;
        if( text2d->fontObj ) text2d->fontObj->destroy( text2d->fontObj );
        text2d->fontObj = (hwObject) val;
        text2d->fontObj->addref( text2d->fontObj );
        text2d->font = NULL;
        break;
    default :
        /* If you don't recognize a string, make sure to set an error */
        __hwIntSetError( HW_ERROR_BAD_PROP );
        break;
    }

    if( dirty ) text2d->dirty = 1;
    return;

BadType:
    /* You should check the types of properties, and "goto BadType"
     * if they don't match what you expect
     */
    __hwIntSetError( HW_ERROR_BAD_TYPE );
}

static hwInt32 inquire( hwObject obj, const char *prop, void **val )
{
    HwText2D
        *text2d = (HwText2D *)obj;
    HW_USE_CURR_DISP;

    if( text2d->dirty ) {
        cook( __hwDisp, text2d );
    }

    switch( hwLookup( prop, text2dTab ) ) {
    case HW_POS :
        *val = text2d->pos;
        return HW_TYPE_3F;
    case HW_SCALE :
        *val = text2d->scale;
        return HW_TYPE_3F;
    case HW_ALIGN :
        *val = &text2d->align;
        return HW_TYPE_1I;
    case HW_FONT_HEIGHT :
        *val = &text2d->fontHeight;
        return HW_TYPE_1F;
    case HW_LABEL :
        *val = text2d->label;
        return HW_TYPE_STRING;
    case HW_COLOR :
        *val = &text2d->color;
        return HW_TYPE_1I;
    case HW_FONT :
        *val = text2d->fontObj;
        return HW_TYPE_OBJECT;
    default :
        __hwIntSetError( HW_ERROR_BAD_PROP );
        return 0;
    }

    /* NOTREACHED */
    return 0;
}

static void cook( hwDisplay disp, HwText2D *text2d )
{
    void
        *iPtr;
    hwObject
        obj;
    hwInt32
        hgt,
        bounds[2];

    text2d->dirty = 0;

    if( !text2d->font ) {
        obj = text2d->fontObj;
        if( obj && (obj->inquire( obj, hwStrFont, &iPtr ) == HW_TYPE_FONT) ) {
            text2d->font = (TexFont *) iPtr;
        }
        if( !text2d->font ) {
            disp->getDrawSize(disp, bounds);
            hgt = bounds[1]*text2d->fontHeight + 0.5;
            text2d->font = txfLoadStaticFont( hgt );
        }
    }
}

static void draw( hwObject obj )
{
    HwText2D
        *text2d = (HwText2D *)obj;
    hwFloat
        posX, posY,
        width, height,
        point[3];
    hwInt32
        align,
        hgt,
        bbox[4],
        parentBbox[4],
        bounds[2];
    HW_USE_CURR_DISP;

    if( text2d->dirty ) {
        cook( __hwDisp, text2d );
    }

    __hwDisp->getDrawSize(__hwDisp, bounds);
    parentBbox[0] = 0;         parentBbox[1] = 0;
    parentBbox[2] = bounds[0]; parentBbox[3] = bounds[1];

    point[0] = text2d->pos[0];
    point[1] = text2d->pos[1];
    point[2] = text2d->pos[2];

    __hwDisp->xformPoint(__hwDisp, point);

    posX = (1.0 + point[0])*0.5; posY = (1.0 - point[1])*0.5;
    width = text2d->scale[0]; height = text2d->scale[1];

    if( text2d->align & HW_GUI_REL_RIGHT )      posX = 1.0 - posX;
    if( text2d->align & HW_GUI_CENTER )         posX -= width * 0.5;

    if( text2d->align & HW_GUI_REL_BOT )        posY = 1.0 - posY;
    if( text2d->align & HW_GUI_CENTER )         posY -= height * 0.5;

    if( text2d->align & HW_GUI_REL_WIDTH )      width = 1.0 - width;
    if( text2d->align & HW_GUI_REL_HEIGHT )     height = 1.0 - height;

    bbox[0] = posX * bounds[0] + 0.5;
    bbox[1] = posY * bounds[1] + 0.5;
    bbox[2] = width * bounds[0] + 0.5;
    bbox[3] = height * bounds[1] + 0.5;

    align = text2d->align & ~(HW_GUI_FONT_FRACTIONAL|HW_GUI_FONT_FRACT_W);

    if( text2d->align & HW_GUI_FONT_FRACTIONAL ) {
        hgt = bounds[1]*text2d->fontHeight + 0.5;
    }
    else if( text2d->align & HW_GUI_FONT_FRACT_W ) {
        hgt = bounds[0]*text2d->fontHeight + 0.5;
    }
    else {
        hgt = text2d->fontHeight + 0.5;
    }

    __hwIntDrawButton( __hwDisp,
                       text2d->label, hgt, text2d->font,
                       text2d->color, 0, 0, -1,
                       bbox, parentBbox,
                       align, -1 );
}

/*** EOF hwText2D.c ***/
