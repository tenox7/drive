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
        /* Specific to HwLabel */
        hwStrPosX, hwStrPosY,
        hwStrWidth, hwStrHeight,
        hwStrFontHeight, hwStrLabel,
        hwStrColor, hwStrBackgroundColor,
        hwStrFont, hwStrTexture,
        hwStrAlign, hwStrBorderColor,

        /* End of list */
        0
    };

#define HW_POS_X        1
#define HW_POS_Y        2
#define HW_WIDTH        3
#define HW_HEIGHT       4
#define HW_FONT_HEIGHT  5
#define HW_LABEL        6
#define HW_COLOR        7
#define HW_BACKGROUND   8
#define HW_FONT         9
#define HW_TEXTURE      10
#define HW_ALIGN        11
#define HW_PARENT       12
#define HW_BOUNDS       13
#define HW_DIRTY        14
#define HW_BORDER       15
#define HW_TRANSPARENCY 16

static struct _hwObjectStruct
    hwLabelStruct = {
        0,      /* Parent - NULL */
        "hwLabel",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwLabel = &hwLabelStruct;

/* Hash table for label strings */
static void *labelTab = 0;

typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount;               /* Reference count */
    hwObject
        parent;                 /* Parent container */
    hwInt32
        dirty,                  /* Modified but not cooked? */
        posX, posY,             /* Position of widget */
        width, height,          /* Size of widget */
        bounds[4],              /* Bounding box */
        parentBounds[4],        /* ... of parent container */
        fontHeight,             /* Height of font chars */
        fgColor, bgColor,       /* Color of the widget */
        border,                 /* Border color, if any */
        align,                  /* Alignment flags */
        tmID,                   /* Texture ID to use, if any */
        ID;                     /* Identifier, for easy use in switch stmts */
    hwObject
        texture;                /* Texture, if any */
    TexFont
        *font;                  /* The font to use */
    hwObject
        fontObj;                /* The font object it came from */
    char
        *label;                 /* Child widgets */
    hwCallback
        callback;               /* Thing to call when label hit */
} HwLabel;

static void cook( hwDisplay disp, HwLabel *label );

static hwObject create( hwObject label )
{
    HwLabel
        *result;
    void
        *t;

    if( label != hwLabel )      return 0;

    HW_HASH_SETUP(labelTab, 24)
        t = labelTab;
        HW_INSERT( hwStrPosX,            HW_POS_X,        t );
        HW_INSERT( hwStrPosY,            HW_POS_Y,        t );
        HW_INSERT( hwStrWidth,           HW_WIDTH,        t );
        HW_INSERT( hwStrHeight,          HW_HEIGHT,       t );
        HW_INSERT( hwStrFontHeight,      HW_FONT_HEIGHT,  t );
        HW_INSERT( hwStrLabel,           HW_LABEL,        t );
        HW_INSERT( hwStrColor,           HW_COLOR,        t );
        HW_INSERT( hwStrTransparency,    HW_TRANSPARENCY, t );
        HW_INSERT( hwStrBackgroundColor, HW_BACKGROUND,   t );
        HW_INSERT( hwStrFont,            HW_FONT,         t );
        HW_INSERT( hwStrTexture,         HW_TEXTURE,      t );
        HW_INSERT( hwStrAlign,           HW_ALIGN,        t );
        HW_INSERT( hwStrParent,          HW_PARENT,       t );
        HW_INSERT( hwStrBounds,          HW_BOUNDS,       t );
        HW_INSERT( hwStrDirty,           HW_DIRTY,        t );
        HW_INSERT( hwStrBorderColor,     HW_BORDER,       t );
    HW_HASH_CLEANUP

    result = malloc( sizeof(HwLabel) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }
    memset(result, 0, sizeof(HwLabel));

    /* Class-specific initialization goes here */

    result->hdr = *label;
    result->hdr.parent = hwLabel;
    result->hdr.name = 0;

    result->refCount = 1;

    result->width = 100;
    result->height = 20;
    result->fontHeight = 12;
    result->posX = 100;
    result->posY = 100;
    result->fgColor = 0xFFFFFFFF;
    result->bgColor = 0;
    result->border = 0;
    result->label = NULL;
    result->texture = NULL;
    result->font = NULL;
    result->fontObj = NULL;
    result->align = 0;
    result->parent = NULL;
    result->tmID = -1;
    result->dirty = 1;

    return (hwObject)result;
}

static void addref( hwObject obj )
{
    HwLabel
        *label = (HwLabel *)obj;;

    /* This method only rarely needs to be changed */
    label->refCount++;
}

static void destroy( hwObject obj )
{
    HwLabel
        *label = (HwLabel *)obj;
    int
        i, n;

    if( --label->refCount > 0 ) return;

    if( label->texture ) {
        label->texture->destroy( label->texture );
    }
    if( label->fontObj ) {
        label->fontObj->destroy( label->fontObj );
    }
    if( label->label ) {
        free( label->label );
    }
    free( label );
}

static void modify
(
    hwObject obj, const char *prop,
    hwInt32 type, const void *val
)
{
    HwLabel
        *label = (HwLabel *)obj;
    const char 
        *str;
    hwInt32
        trans,
        dirty = 1;

    switch( hwLookup( prop, labelTab ) ) {
    case HW_POS_X :
        if( type == HW_TYPE_1F )      label->posX = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) label->posX = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_POS_Y :
        if( type == HW_TYPE_1F )      label->posY = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) label->posY = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_WIDTH :
        if( type == HW_TYPE_1F )      label->width = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) label->width = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_HEIGHT :
        if( type == HW_TYPE_1F )      label->height = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) label->height = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_FONT_HEIGHT :
        if( type == HW_TYPE_1F )      label->fontHeight = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) label->fontHeight = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_ALIGN :
        if( type == HW_TYPE_1F )      label->align = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) label->align = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_PARENT :
        if( type != HW_TYPE_OBJECT )    goto BadType;
        label->parent = (hwObject)val;
        break;
    case HW_LABEL :
        if( type != HW_TYPE_STRING )    goto BadType;
        str = val;
        if( label->label ) free( label->label );
        label->label = malloc( strlen(val) + 1 );
        if( !label->label ) {
            __hwIntSetError( HW_ERROR_NO_MEMORY );
            return;
        }
        strcpy( label->label, str );
        break;
    case HW_COLOR :
        label->fgColor = __hwIntGuiColor( type, val );
        break;
    case HW_TRANSPARENCY :
        if( type == HW_TYPE_1F )      trans = 255*(1.0 - *(hwFloat *)val);
        else if( type == HW_TYPE_1I ) trans = 255 - *(hwInt32 *)val;
        else                          goto BadType;
        if( trans < 0 ) trans = 0;
        if( trans > 255 ) trans = 255;
        label->fgColor &= 0x00FFFFFF;
        label->fgColor |= trans << 24;
        label->bgColor &= 0x00FFFFFF;
        label->bgColor |= trans << 24;
        break;
    case HW_BACKGROUND :
        label->bgColor = __hwIntGuiColor( type, val );
        break;
    case HW_BORDER :
        label->border = __hwIntGuiColor( type, val );
        break;
    case HW_FONT :
        if( type != HW_TYPE_OBJECT )    goto BadType;
        if( label->fontObj ) label->fontObj->destroy( label->fontObj );
        label->fontObj = (hwObject) val;
        label->fontObj->addref( label->fontObj );
        label->font = NULL;
        break;
    case HW_TEXTURE :
        if( type != HW_TYPE_OBJECT )    goto BadType;
        if( label->texture ) label->texture->destroy( label->texture );
        label->texture = (hwObject) val;
        label->texture->addref( label->texture );
        label->tmID = -1;
        break;
    case HW_DIRTY :
        if( label->fontObj ) {
            label->fontObj->modify( label->fontObj, prop, type, val );
        }
        if( label->texture ) {
            label->texture->modify( label->texture, prop, type, val );
        }
        break;
    default :
        /* If you don't recognize a string, make sure to set an error */
        __hwIntSetError( HW_ERROR_BAD_PROP );
        break;
    }

    if( dirty ) label->dirty = 1;
    return;

BadType:
    /* You should check the types of properties, and "goto BadType"
     * if they don't match what you expect
     */
    __hwIntSetError( HW_ERROR_BAD_TYPE );
}

static hwInt32 inquire( hwObject obj, const char *prop, void **val )
{
    HwLabel
        *label = (HwLabel *)obj;
    HW_USE_CURR_DISP;

    if( label->dirty ) {
        cook( __hwDisp, label );
    }

    switch( hwLookup( prop, labelTab ) ) {
    case HW_POS_X :
        *val = &label->posX;
        return HW_TYPE_1I;
    case HW_POS_Y :
        *val = &label->posY;
        return HW_TYPE_1I;
    case HW_WIDTH :
        *val = &label->width;
        return HW_TYPE_1I;
    case HW_HEIGHT :
        *val = &label->height;
        return HW_TYPE_1I;
    case HW_ALIGN :
        *val = &label->align;
        return HW_TYPE_1I;
    case HW_PARENT :
        *val = label->parent;
        return HW_TYPE_OBJECT;
    case HW_BOUNDS :
        *val = label->bounds;
        return HW_TYPE_4I;
    case HW_FONT_HEIGHT :
        *val = &label->fontHeight;
        return HW_TYPE_1I;
    case HW_LABEL :
        *val = label->label;
        return HW_TYPE_STRING;
    case HW_COLOR :
        *val = &label->fgColor;
        return HW_TYPE_1I;
    case HW_BACKGROUND :
        *val = &label->bgColor;
        return HW_TYPE_1I;
    case HW_BORDER :
        *val = &label->bgColor;
        return HW_TYPE_1I;
    case HW_FONT :
        *val = label->fontObj;
        return HW_TYPE_OBJECT;
    case HW_TEXTURE :
        *val = label->texture;
        return HW_TYPE_OBJECT;
    default :
        __hwIntSetError( HW_ERROR_BAD_PROP );
        return 0;
    }

    /* NOTREACHED */
    return 0;
}

static void cook( hwDisplay disp, HwLabel *label )
{
    void
        *iPtr;
    hwObject
        obj;

    label->dirty = 0;

    __hwIntGetParentBounds( disp, label->parent, label->parentBounds );

    __hwIntPositionWidget( label->bounds, label->parentBounds,
                           label->posX, label->posY,
                           label->width, label->height,
                           label->align,
                           0.0 );

    if( label->texture ) {
        obj = label->texture;
        obj->draw( obj );
        if( obj->inquire( obj, hwStrTextureID, &iPtr ) == HW_TYPE_1I ) {
            label->tmID = *(hwInt32 *)iPtr;
        }
    }
    if( !label->font ) {
        obj = label->fontObj;
        if( obj && (obj->inquire( obj, hwStrFont, &iPtr ) == HW_TYPE_FONT) ) {
            label->font = (TexFont *) iPtr;
        }
        if( !label->font ) {
            label->font = txfLoadStaticFont( label->fontHeight );
        }
    }
}

static void draw( hwObject obj )
{
    HwLabel
        *label = (HwLabel *)obj;
    HW_USE_CURR_DISP;

    if( label->dirty ) {
        cook( __hwDisp, label );
    }

    __hwIntDrawButton( __hwDisp, label->label, label->fontHeight,
                       label->font, label->fgColor, label->bgColor,
                       label->border, label->tmID,
                       label->bounds, label->parentBounds, label->align,
                       -1 );
}

/*** EOF hwLabel.c ***/
