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
        /* Specific to HwToggle */
        hwStrPosX, hwStrPosY,
        hwStrWidth, hwStrHeight,
        hwStrFontHeight, hwStrLabel,
        hwStrColor, hwStrBackgroundColor,
        hwStrActiveColor, hwStrActiveBackground,
        hwStrCallback, hwStrID,
        hwStrFont, hwStrTexture,
        hwStrExclude, hwStrValue,
        hwStrAlign,
        hwStrBorderColor, hwStrActiveBorderColor,
        hwStrActiveTexture,
        hwStrActive,

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
#define HW_ACTIVE_FG    9
#define HW_ACTIVE_BG    10
#define HW_CALLBACK     11
#define HW_EVENT        12
#define HW_ID           13
#define HW_FONT         14
#define HW_TEXTURE      15
#define HW_EXCLUDE      16
#define HW_VALUE        17
#define HW_ALIGN        18
#define HW_PARENT       19
#define HW_BOUNDS       20
#define HW_DIRTY        21
#define HW_BORDER       22
#define HW_ACTIVE_BORD  23
#define HW_ACTIVE_TEX   24
#define HW_ACTIVE       25
#define HW_EXECUTE      26

static struct _hwObjectStruct
    hwToggleStruct = {
        0,      /* Parent - NULL */
        "hwToggle",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwToggle = &hwToggleStruct;

/* Hash table for toggle strings */
static void *toggleTab = 0;

static hwInt32 hollowTM = -1;
static hwInt32 fillTM = -1;
static hwInt32 boxTM = -1;
static hwInt32 checkTM = -1;
static hwInt32 tmCooked = 0;

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
        parentBounds[4],        /* ... of the parent */
        fontHeight,             /* Height of font chars */
        fgColor, bgColor,       /* Color of the widget */
        fgActive, bgActive,     /* Color while pressed */
        border, activeBord,     /* Border, active border */
        tmID,                   /* Texture ID to use, if any */
        activeTmID,             /* Active texture ID to use, if any */
        ID,                     /* Identifier, for easy use in switch stmts */
        align,                  /* Alignment flags */
        active,                 /* Is it active? */
        state,                  /* Is the mouse inside? */
        value,                  /* 0 or 1 */
        numExclude;             /* Size of the exclude array */
    hwObject
        texture,                /* Texture, if any */
        activeTexture,          /* Active texture, if any */
        *exclude;               /* Other toggles to exclude, if any */
    TexFont
        *font;                  /* The font to use */
    hwObject
        fontObj;                /* Object it came from */
    char
        *label;                 /* Child widgets */
    hwCallback
        callback;               /* Thing to call when toggle hit */
} HwToggle;

static void cook( hwDisplay disp, HwToggle *toggle );

static hwObject create( hwObject toggle )
{
    HwToggle
        *result;
    void
        *t;

    if( toggle != hwToggle )      return 0;

    HW_HASH_SETUP(toggleTab, 32)
        t = toggleTab;
        HW_INSERT( hwStrPosX,              HW_POS_X,       t );
        HW_INSERT( hwStrPosY,              HW_POS_Y,       t );
        HW_INSERT( hwStrWidth,             HW_WIDTH,       t );
        HW_INSERT( hwStrHeight,            HW_HEIGHT,      t );
        HW_INSERT( hwStrFontHeight,        HW_FONT_HEIGHT, t );
        HW_INSERT( hwStrLabel,             HW_LABEL,       t );
        HW_INSERT( hwStrColor,             HW_COLOR,       t );
        HW_INSERT( hwStrBackgroundColor,   HW_BACKGROUND,  t );
        HW_INSERT( hwStrActiveColor,       HW_ACTIVE_FG,   t );
        HW_INSERT( hwStrActiveBackground,  HW_ACTIVE_BG,   t );
        HW_INSERT( hwStrCallback,          HW_CALLBACK,    t );
        HW_INSERT( hwStrEvent,             HW_EVENT,       t );
        HW_INSERT( hwStrID,                HW_ID,          t );
        HW_INSERT( hwStrFont,              HW_FONT,        t );
        HW_INSERT( hwStrTexture,           HW_TEXTURE,     t );
        HW_INSERT( hwStrExclude,           HW_EXCLUDE,     t );
        HW_INSERT( hwStrValue,             HW_VALUE,       t );
        HW_INSERT( hwStrAlign,             HW_ALIGN,       t );
        HW_INSERT( hwStrParent,            HW_PARENT,      t );
        HW_INSERT( hwStrBounds,            HW_BOUNDS,      t );
        HW_INSERT( hwStrDirty,             HW_DIRTY,       t );
        HW_INSERT( hwStrBorderColor,       HW_BORDER,      t );
        HW_INSERT( hwStrActiveBorderColor, HW_ACTIVE_BORD, t );
        HW_INSERT( hwStrActiveTexture,     HW_ACTIVE_TEX,  t );
        HW_INSERT( hwStrActive,            HW_ACTIVE,      t );
        HW_INSERT( hwStrExecute,           HW_EXECUTE,     t );
    HW_HASH_CLEANUP

    result = malloc( sizeof(HwToggle) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }
    memset(result, 0, sizeof(HwToggle));

    /* Class-specific initialization goes here */

    result->hdr = *toggle;
    result->hdr.parent = hwToggle;
    result->hdr.name = 0;

    result->refCount = 1;

    result->width = 100;
    result->height = 20;
    result->fontHeight = 12;
    result->posX = 100;
    result->posY = 100;
    result->fgColor = 0xFFFFFFFF;
    result->bgColor = 0;
    result->fgActive = 0xFF000000;
    result->bgActive = 0xC0FFFFFF;
    result->border = 0;
    result->activeBord = 0;
    result->label = NULL;
    result->texture = NULL;
    result->activeTexture = NULL;
    result->font = NULL;
    result->fontObj = NULL;
    result->ID = 0;
    result->active = 0;
    result->state = 0;
    result->value = 0;
    result->align = 0;
    result->parent = NULL;
    result->tmID = -1;
    result->activeTmID = -1;
    result->dirty = 1;

    return (hwObject)result;
}

static void addref( hwObject obj )
{
    HwToggle
        *toggle = (HwToggle *)obj;;

    /* This method only rarely needs to be changed */
    toggle->refCount++;
}

static void destroy( hwObject obj )
{
    HwToggle
        *toggle = (HwToggle *)obj;
    int
        i, n;

    if( --toggle->refCount > 0 ) return;

    if( toggle->texture ) {
        toggle->texture->destroy( toggle->texture );
    }
    if( toggle->activeTexture ) {
        toggle->activeTexture->destroy( toggle->activeTexture );
    }
    if( toggle->fontObj ) {
        toggle->fontObj->destroy( toggle->fontObj );
    }
    n = toggle->numExclude;
    for( i = 0; i < n; i++ ) {
        if( toggle->exclude[i] ) {
            toggle->exclude[i]->destroy( toggle->exclude[i] );
        }
    }
    if( toggle->exclude ) {
        free( toggle->exclude );
    }
    if( toggle->label ) {
        free( toggle->label );
    }
    free( toggle );
}

static void doExecute( HwToggle *toggle )
{
    hwInt32
        i, n;

    n = toggle->numExclude;
    if( n ) {
        toggle->value = 1;
        for( i = 0; i < n; i++ ) {
            HW_MODIFY_1I( toggle->exclude[i], hwStrValue, 0 );
        }
    }
    else {
        toggle->value = !toggle->value;
    }
    if( toggle->callback ) toggle->callback( (hwObject)toggle, HW_CB_COMPLETE );
}

static void doEvent( hwObject obj, hwWinEvent *event )
{
    HwToggle
        *toggle;
    hwInt32
        i, n,
        dx, dy, w, h, cmd;
    HW_USE_CURR_DISP;

    toggle = (HwToggle *)obj;

    if( (event->type == HW_INPUT_CONFIG) || toggle->dirty ) {
        cook( __hwDisp, toggle );
    }

    dx = event->pointer.x - toggle->bounds[0];
    dy = event->pointer.y - toggle->bounds[1];
    w = toggle->bounds[2];
    h = toggle->bounds[3];

    switch( event->type ) {
    case HW_INPUT_BUTTON_PRESS :
        if( (dx < 0) || (dy < 0) || (dx > w) || (dy > h) ) break;
        toggle->active = 1;
        toggle->state = 1;
        if( toggle->callback ) toggle->callback( obj, HW_CB_ACTIVATE );
        break;
    case HW_INPUT_BUTTON_RELEASE :
        if( !toggle->active ) return;
        if( (dx < 0) || (dy < 0) || (dx > w) || (dy > h) ) {
            /* Cancelled */
            if( toggle->callback ) toggle->callback( obj, HW_CB_DEACTIVATE );
        }
        else {
            doExecute( toggle );
        }
        toggle->active = 0;
        toggle->state = 0;
        break;
    case HW_INPUT_POINTER :
        if( !toggle->active ) return;
        if( (dx < 0) || (dy < 0) || (dx > w) || (dy > h) ) {
            toggle->state = 0;
        }
        else {
            toggle->state = 1;
        }
        break;
    }
}

static void modify
(
    hwObject obj, const char *prop,
    hwInt32 type, const void *val
)
{
    HwToggle
        *toggle = (HwToggle *)obj;
    hwObject
        *ptr;
    const char 
        *str;
    hwInt32
        i, n,
        dirty = 1;
    HW_USE_CURR_DISP;

    switch( hwLookup( prop, toggleTab ) ) {
    case HW_POS_X :
        if( type == HW_TYPE_1F )      toggle->posX = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) toggle->posX = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_POS_Y :
        if( type == HW_TYPE_1F )      toggle->posY = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) toggle->posY = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_WIDTH :
        if( type == HW_TYPE_1F )      toggle->width = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) toggle->width = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_HEIGHT :
        if( type == HW_TYPE_1F )      toggle->height = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) toggle->height = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_FONT_HEIGHT :
        if( type == HW_TYPE_1F )      toggle->fontHeight = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) toggle->fontHeight = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_ALIGN :
        if( type == HW_TYPE_1F )      toggle->align = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) toggle->align = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_PARENT :
        if( type != HW_TYPE_OBJECT )  goto BadType;
        toggle->parent = (hwObject)val;
        break;
    case HW_LABEL :
        if( type != HW_TYPE_STRING )    goto BadType;
        str = val;
        if( toggle->label ) free( toggle->label );
        toggle->label= malloc( strlen(val) + 1 );
        if( !toggle->label ) {
            __hwIntSetError( HW_ERROR_NO_MEMORY );
            return;
        }
        strcpy( toggle->label, str );
        break;
    case HW_COLOR :
        toggle->fgColor = __hwIntGuiColor( type, val );
        break;
    case HW_BACKGROUND :
        toggle->bgColor = __hwIntGuiColor( type, val );
        break;
    case HW_ACTIVE_FG :
        toggle->fgActive = __hwIntGuiColor( type, val );
        break;
    case HW_ACTIVE_BG :
        toggle->bgActive = __hwIntGuiColor( type, val );
        break;
    case HW_ACTIVE :
        if( type != HW_TYPE_1B )        goto BadType;
        toggle->active = *(hwInt32 *)val;
        toggle->state = *(hwInt32 *)val;
        if( toggle->callback ) {
            if( toggle->active ) toggle->callback( obj, HW_CB_ACTIVATE );
            else                 toggle->callback( obj, HW_CB_DEACTIVATE );
        }
        break;
    case HW_EXECUTE :
        doExecute( toggle );
        dirty = 0; /* Does not change appearance */
        break;
    case HW_BORDER :
        toggle->border = __hwIntGuiColor( type, val );
        break;
    case HW_ACTIVE_BORD :
        toggle->activeBord = __hwIntGuiColor( type, val );
        break;
    case HW_CALLBACK :
        dirty = 0; /* Does not change appearance */
        if( type == HW_TYPE_STRING ) {
            toggle->callback = hwFindCallback( (char *)val );
        }
        else if( type == HW_TYPE_CALLBACK ) {
            toggle->callback = (hwCallback) val;
        }
        else {
            goto BadType;
        }
        break;
    case HW_ID :
        dirty = 0; /* Does not change appearance */
        if( type == HW_TYPE_1F )      toggle->ID = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) toggle->ID = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_VALUE :
        if( type == HW_TYPE_1F )      toggle->value = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) toggle->value = *(hwInt32 *)val;
        else                          goto BadType;
        if( toggle->callback ) toggle->callback( obj, HW_CB_COMPLETE );
        break;
    case HW_EXCLUDE :
        dirty = 0; /* Does not change appearance */
        if( HW_GET_BASE(type) != HW_TYPE_OBJECT ) goto BadType;
        n = toggle->numExclude;
        for( i = 0; i < n; i++ ) {
            if( toggle->exclude[i] ) {
                toggle->exclude[i]->destroy( toggle->exclude[i] );
            }
        }
        if( toggle->exclude ) free( toggle->exclude );
        n = HW_GET_COUNT(type);
        toggle->exclude = malloc( n * sizeof(hwObject) );
        if( !toggle->exclude ) {
            __hwIntSetError( HW_ERROR_NO_MEMORY );
            return;
        }
        ptr = (hwObject *)val;
        for( i = 0; i < n; i++ ) {
            toggle->exclude[i] = ptr[i];
            ptr[i]->addref( ptr[i] );
        }
        toggle->numExclude = n;
        break;
    case HW_FONT :
        if( type != HW_TYPE_OBJECT )    goto BadType;
        if( toggle->fontObj ) {
            toggle->fontObj->destroy( toggle->fontObj );
        }
        toggle->fontObj = (hwObject) val;
        toggle->fontObj->addref( toggle->fontObj );
        toggle->font = NULL;
        break;
    case HW_TEXTURE :
        if( type != HW_TYPE_OBJECT )    goto BadType;
        if( toggle->texture ) toggle->texture->destroy( toggle->texture );
        toggle->texture = (hwObject) val;
        toggle->texture->addref( toggle->texture );
        toggle->tmID = -1;
        break;
    case HW_ACTIVE_TEX :
        if( type != HW_TYPE_OBJECT )    goto BadType;
        if( toggle->activeTexture ) {
            toggle->activeTexture->destroy( toggle->activeTexture );
        }
        toggle->activeTexture = (hwObject) val;
        toggle->activeTexture->addref( toggle->activeTexture );
        toggle->activeTmID = -1;
        break;
    case HW_DIRTY :
        if( toggle->fontObj ) {
            toggle->fontObj->modify( toggle->fontObj, prop, type, val );
        }
        if( toggle->texture ) {
            toggle->texture->modify( toggle->texture, prop, type, val );
        }
        if( hollowTM >= 0 ) {
            __hwDisp->destroyTexture( __hwDisp, hollowTM );
            __hwDisp->destroyTexture( __hwDisp, fillTM );
            __hwDisp->destroyTexture( __hwDisp, boxTM );
            __hwDisp->destroyTexture( __hwDisp, checkTM );
        }
        hollowTM = fillTM = boxTM = checkTM = -1;
        tmCooked = 0;
        break;
    case HW_EVENT :
        if( type != HW_TYPE_EVENT )    goto BadType;
        dirty = 0; /* Does not change appearance */
        doEvent( obj, (hwWinEvent *)val );
        break;
    default :
        /* If you don't recognize a string, make sure to set an error */
        __hwIntSetError( HW_ERROR_BAD_PROP );
        break;
    }

    if( dirty ) toggle->dirty = 1;
    return;

BadType:
    /* You should check the types of properties, and "goto BadType"
     * if they don't match what you expect
     */
    __hwIntSetError( HW_ERROR_BAD_TYPE );
}

static hwInt32 inquire( hwObject obj, const char *prop, void **val )
{
    HwToggle
        *toggle = (HwToggle *)obj;
    HW_USE_CURR_DISP;

    if( toggle->dirty ) {
        cook( __hwDisp, toggle );
    }

    switch( hwLookup( prop, toggleTab ) ) {
    case HW_POS_X :
        *val = &toggle->posX;
        return HW_TYPE_1I;
    case HW_POS_Y :
        *val = &toggle->posY;
        return HW_TYPE_1I;
    case HW_WIDTH :
        *val = &toggle->width;
        return HW_TYPE_1I;
    case HW_HEIGHT :
        *val = &toggle->height;
        return HW_TYPE_1I;
    case HW_ALIGN :
        *val = &toggle->align;
        return HW_TYPE_1I;
    case HW_PARENT :
        *val = toggle->parent;
        return HW_TYPE_OBJECT;
    case HW_BOUNDS :
        /* TBD: Cook it if dirty */
        *val = toggle->bounds;
        return HW_TYPE_4I;
    case HW_FONT_HEIGHT :
        *val = &toggle->fontHeight;
        return HW_TYPE_1I;
    case HW_LABEL :
        *val = toggle->label;
        return HW_TYPE_STRING;
    case HW_COLOR :
        *val = &toggle->fgColor;
        return HW_TYPE_1I;
    case HW_BACKGROUND :
        *val = &toggle->bgColor;
        return HW_TYPE_1I;
    case HW_ACTIVE_FG :
        *val = &toggle->fgActive;
        return HW_TYPE_1I;
    case HW_ACTIVE_BG :
        *val = &toggle->bgActive;
        return HW_TYPE_1I;
    case HW_ACTIVE :
        *val = &toggle->active;
        return HW_TYPE_1B;
    case HW_BORDER :
        *val = &toggle->border;
        return HW_TYPE_1I;
    case HW_ACTIVE_BORD :
        *val = &toggle->activeBord;
        return HW_TYPE_1I;
    case HW_CALLBACK :
        *val = (void *)toggle->callback;
        return HW_TYPE_CALLBACK;
    case HW_ID :
        *val = &toggle->ID;
        return HW_TYPE_1I;
    case HW_VALUE :
        *val = &toggle->value;
        return HW_TYPE_1I;
    case HW_FONT :
        *val = toggle->fontObj;
        return HW_TYPE_OBJECT;
    case HW_TEXTURE :
        *val = toggle->texture;
        return HW_TYPE_OBJECT;
    case HW_ACTIVE_TEX :
        *val = toggle->activeTexture;
        return HW_TYPE_OBJECT;
    case HW_EXCLUDE :
        *val = toggle->exclude;
        return HW_MAKE_TYPE(HW_TYPE_OBJECT, toggle->numExclude);
    default :
        __hwIntSetError( HW_ERROR_BAD_PROP );
        return 0;
    }

    /* NOTREACHED */
    return 0;
}

static hwOrientType defaultOrient = {
    { 1., 1., 1. }, /* scale */
    { 0., 0., 0. }, /* rotate */
    { 0., 0., 0. }, /* pos */
};

static unsigned char hollowCirc[256] = {
    0x00, 0x00, 0x00, 0x00, 0x38, 0x96, 0xd8, 0xff, 0xff, 0xd8, 0x96, 0x38, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x97, 0xff, 0xff, 0xff, 0xf0, 
    0xf0, 0xff, 0xff, 0xff, 0x97, 0x09, 0x00, 0x00, 0x00, 0x0a, 0xba, 0xff, 
    0xf6, 0x87, 0x33, 0x15, 0x15, 0x33, 0x87, 0xf6, 0xff, 0xba, 0x0a, 0x00, 
    0x00, 0x97, 0xff, 0xdc, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 
    0xdc, 0xff, 0x97, 0x00, 0x35, 0xfe, 0xf3, 0x2d, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x2d, 0xf3, 0xfe, 0x35, 0x98, 0xff, 0x87, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87, 0xff, 0x98, 
    0xdc, 0xfe, 0x2f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x2f, 0xfe, 0xdc, 0xfe, 0xf0, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0xf0, 0xfe, 0xfe, 0xf0, 0x14, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0xf0, 0xfe, 
    0xdc, 0xfe, 0x2f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x2f, 0xfe, 0xdc, 0x98, 0xff, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x87, 0xff, 0x98, 0x35, 0xfe, 0xf3, 0x2d, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2d, 0xf3, 0xfe, 0x35, 
    0x00, 0x97, 0xff, 0xdc, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 
    0xdc, 0xff, 0x97, 0x00, 0x00, 0x0a, 0xba, 0xff, 0xf6, 0x87, 0x33, 0x15, 
    0x15, 0x33, 0x87, 0xf6, 0xff, 0xba, 0x0a, 0x00, 0x00, 0x00, 0x09, 0x97, 
    0xff, 0xff, 0xff, 0xf0, 0xf0, 0xff, 0xff, 0xff, 0x97, 0x09, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x38, 0x96, 0xd8, 0xff, 0xff, 0xd8, 0x96, 0x38, 
    0x00, 0x00, 0x00, 0x00, 
};
static hwImageStruct hollowImg = {
    16, 16, 1,          /* Width, height, depth */
    1, HW_IMG_UBYTE,    /* components, type */
    0x7FFFFFFF,         /* refCount */
    -1,                 /* tmId */
    NULL,               /* fileName */
    hollowCirc,         /* data */
    NULL                /* next */
};

static unsigned char fillCirc[256] = {
    0x00, 0x00, 0x00, 0x00, 0x38, 0x96, 0xd8, 0xff, 0xff, 0xd8, 0x96, 0x38, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x97, 0xff, 0xff, 0xff, 0xf0, 
    0xf0, 0xff, 0xff, 0xff, 0x97, 0x09, 0x00, 0x00, 0x00, 0x0a, 0xba, 0xff, 
    0xf6, 0x87, 0x33, 0x14, 0x14, 0x33, 0x87, 0xf6, 0xff, 0xba, 0x0a, 0x00, 
    0x00, 0x97, 0xff, 0xdc, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 
    0xdc, 0xff, 0x97, 0x00, 0x35, 0xfe, 0xf3, 0x2d, 0x00, 0x12, 0x85, 0xca, 
    0xca, 0x85, 0x12, 0x00, 0x2d, 0xf3, 0xfe, 0x35, 0x98, 0xff, 0x87, 0x00, 
    0x13, 0xce, 0xff, 0xff, 0xff, 0xff, 0xce, 0x13, 0x00, 0x87, 0xff, 0x98, 
    0xdc, 0xfe, 0x2e, 0x00, 0x8d, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x8d, 
    0x00, 0x2e, 0xfe, 0xdc, 0xfe, 0xf0, 0x12, 0x01, 0xd5, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xd5, 0x01, 0x12, 0xf0, 0xfe, 0xfe, 0xf0, 0x12, 0x01, 
    0xd5, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xd5, 0x01, 0x12, 0xf0, 0xfe, 
    0xdc, 0xfe, 0x2e, 0x00, 0x8d, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x8d, 
    0x00, 0x2e, 0xfe, 0xdc, 0x98, 0xff, 0x87, 0x00, 0x13, 0xce, 0xff, 0xff, 
    0xff, 0xff, 0xce, 0x13, 0x00, 0x87, 0xff, 0x98, 0x35, 0xfe, 0xf3, 0x2d, 
    0x00, 0x12, 0x85, 0xca, 0xca, 0x85, 0x12, 0x00, 0x2d, 0xf3, 0xfe, 0x35, 
    0x00, 0x97, 0xff, 0xdc, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 
    0xdc, 0xff, 0x97, 0x00, 0x00, 0x0a, 0xba, 0xff, 0xf6, 0x87, 0x33, 0x14, 
    0x14, 0x33, 0x87, 0xf6, 0xff, 0xba, 0x0a, 0x00, 0x00, 0x00, 0x09, 0x97, 
    0xff, 0xff, 0xff, 0xf0, 0xf0, 0xff, 0xff, 0xff, 0x97, 0x09, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x38, 0x96, 0xd8, 0xff, 0xff, 0xd8, 0x96, 0x38, 
    0x00, 0x00, 0x00, 0x00, 
};
static hwImageStruct fillImg = {
    16, 16, 1,          /* Width, height, depth */
    1, HW_IMG_UBYTE,    /* components, type */
    0x7FFFFFFF,         /* refCount */
    -1,                 /* tmId */
    NULL,               /* fileName */
    fillCirc,           /* data */
    NULL                /* next */
};

static unsigned char roundBox[256] = {
    0x00, 0x27, 0xa6, 0xee, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xec, 0xa0, 0x22, 0x00, 0x27, 0xe6, 0xff, 0xfc, 0xed, 0xed, 0xed, 0xed, 
    0xed, 0xed, 0xed, 0xed, 0xfd, 0xff, 0xe0, 0x20, 0xac, 0xff, 0xb1, 0x26, 
    0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x29, 0xb7, 0xff, 0x9f, 
    0xf6, 0xf7, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x28, 0xfc, 0xe8, 0xff, 0xee, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x1b, 0xf6, 0xf5, 0xff, 0xef, 0x15, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1d, 0xf6, 0xf3, 
    0xff, 0xef, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x1d, 0xf6, 0xf3, 0xff, 0xef, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x1d, 0xf6, 0xf3, 0xff, 0xef, 0x15, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1d, 0xf6, 0xf3, 
    0xff, 0xef, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x1d, 0xf6, 0xf3, 0xff, 0xef, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x1d, 0xf6, 0xf3, 0xff, 0xee, 0x12, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1a, 0xf6, 0xf5, 
    0xf4, 0xf8, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x2b, 0xfc, 0xe6, 0xa7, 0xff, 0xb6, 0x30, 0x1a, 0x1c, 0x1c, 0x1c, 
    0x1c, 0x1c, 0x1c, 0x1b, 0x34, 0xbd, 0xff, 0x9a, 0x22, 0xe0, 0xff, 0xff, 
    0xf4, 0xf5, 0xf5, 0xf5, 0xf5, 0xf5, 0xf5, 0xf4, 0xff, 0xff, 0xd9, 0x1b, 
    0x00, 0x21, 0x9a, 0xe2, 0xf7, 0xf6, 0xf6, 0xf6, 0xf6, 0xf6, 0xf6, 0xf7, 
    0xe0, 0x94, 0x1b, 0x00, 
};
static hwImageStruct boxImg = {
    16, 16, 1,          /* Width, height, depth */
    1, HW_IMG_UBYTE,    /* components, type */
    0x7FFFFFFF,         /* refCount */
    -1,                 /* tmId */
    NULL,               /* fileName */
    roundBox,           /* data */
    NULL                /* next */
};

static unsigned char checkBox[256] = {
    0x00, 0x27, 0xa6, 0xee, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xeb, 0x9d, 0x54, 0x5d, 0x27, 0xe6, 0xff, 0xfc, 0xed, 0xed, 0xed, 0xed, 
    0xed, 0xed, 0xed, 0xea, 0xf2, 0xff, 0xff, 0x4f, 0xac, 0xff, 0xb1, 0x26, 
    0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x0e, 0x18, 0xc0, 0xfa, 0xff, 0x99, 
    0xf6, 0xf7, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0xb6, 
    0xdd, 0x4a, 0xfa, 0xe8, 0xff, 0xee, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x07, 0xba, 0xfb, 0x29, 0x0e, 0xf6, 0xf5, 0xff, 0xef, 0x15, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0xa4, 0xff, 0x55, 0x00, 0x1d, 0xf6, 0xf3, 
    0xff, 0xef, 0x12, 0x0b, 0x56, 0x27, 0x00, 0x00, 0x7f, 0xff, 0x8d, 0x00, 
    0x00, 0x1d, 0xf6, 0xf3, 0xff, 0xee, 0x0f, 0x9b, 0xff, 0xda, 0x04, 0x3f, 
    0xff, 0xd2, 0x08, 0x00, 0x00, 0x1d, 0xf6, 0xf3, 0xff, 0xef, 0x11, 0x23, 
    0xdf, 0xff, 0x83, 0xd1, 0xfe, 0x38, 0x00, 0x00, 0x00, 0x1d, 0xf6, 0xf3, 
    0xff, 0xef, 0x16, 0x00, 0x59, 0xff, 0xff, 0xff, 0x97, 0x00, 0x00, 0x00, 
    0x00, 0x1d, 0xf6, 0xf3, 0xff, 0xef, 0x15, 0x00, 0x05, 0xd6, 0xff, 0xf2, 
    0x1b, 0x00, 0x00, 0x00, 0x00, 0x1d, 0xf6, 0xf3, 0xff, 0xee, 0x12, 0x00, 
    0x00, 0x7b, 0xe3, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1a, 0xf6, 0xf5, 
    0xf4, 0xf8, 0x20, 0x00, 0x00, 0x0f, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x2b, 0xfc, 0xe6, 0xa7, 0xff, 0xb6, 0x30, 0x1a, 0x18, 0x16, 0x1c, 
    0x1c, 0x1c, 0x1c, 0x1b, 0x34, 0xbd, 0xff, 0x9a, 0x22, 0xe0, 0xff, 0xff, 
    0xf4, 0xf5, 0xf5, 0xf5, 0xf5, 0xf5, 0xf5, 0xf4, 0xff, 0xff, 0xd9, 0x1b, 
    0x00, 0x21, 0x9a, 0xe2, 0xf7, 0xf6, 0xf6, 0xf6, 0xf6, 0xf6, 0xf6, 0xf7, 
    0xe0, 0x94, 0x1b, 0x00, 
};
static hwImageStruct checkImg = {
    16, 16, 1,          /* Width, height, depth */
    1, HW_IMG_UBYTE,    /* components, type */
    0x7FFFFFFF,         /* refCount */
    -1,                 /* tmId */
    NULL,               /* fileName */
    checkBox,           /* data */
    NULL                /* next */
};

static void cook( hwDisplay disp, HwToggle *toggle )
{
    hwObject
        obj;
    void
        *iPtr;

    __hwIntGetParentBounds( disp, toggle->parent, toggle->parentBounds );

    __hwIntPositionWidget( toggle->bounds, toggle->parentBounds,
                           toggle->posX, toggle->posY,
                           toggle->width, toggle->height,
                           toggle->align,
                           0.0 );

    toggle->dirty = 0;
    if( toggle->texture ) {
        obj = toggle->texture;
        obj->draw( obj );
        if( obj->inquire( obj, hwStrTextureID, &iPtr ) == HW_TYPE_1I ) {
            toggle->tmID = *(hwInt32 *)iPtr;
        }
    }
    if( toggle->activeTexture ) {
        obj = toggle->activeTexture;
        obj->draw( obj );
        if( obj->inquire( obj, hwStrTextureID, &iPtr ) == HW_TYPE_1I ) {
            toggle->activeTmID = *(hwInt32 *)iPtr;
        }
    }
    if( !toggle->font ) {
        obj = toggle->fontObj;
        if( obj && (obj->inquire( obj, hwStrFont, &iPtr ) == HW_TYPE_FONT) ) {
            toggle->font = (TexFont *) iPtr;
        }
        if( !toggle->font ) {
            toggle->font = txfLoadStaticFont( toggle->fontHeight );
        }
    }
}

static void cookTM( hwDisplay disp )
{
    tmCooked = 1;

    hollowTM = disp->createTexture( disp, &hollowImg,
                                          HW_TM_TRILINEAR,
                                          HW_TM_MODULATE,
                                          HW_TM_CLAMP,
                                          HW_TM_EXPLICIT,
                                          &defaultOrient,
                                          HW_TM_ALPHA );
    fillTM = disp->createTexture( disp, &fillImg,
                                          HW_TM_TRILINEAR,
                                          HW_TM_MODULATE,
                                          HW_TM_CLAMP,
                                          HW_TM_EXPLICIT,
                                          &defaultOrient,
                                          HW_TM_ALPHA );
    boxTM = disp->createTexture( disp, &boxImg,
                                          HW_TM_TRILINEAR,
                                          HW_TM_MODULATE,
                                          HW_TM_CLAMP,
                                          HW_TM_EXPLICIT,
                                          &defaultOrient,
                                          HW_TM_ALPHA );
    checkTM = disp->createTexture( disp, &checkImg,
                                          HW_TM_TRILINEAR,
                                          HW_TM_MODULATE,
                                          HW_TM_CLAMP,
                                          HW_TM_EXPLICIT,
                                          &defaultOrient,
                                          HW_TM_ALPHA );
}

static void draw( hwObject obj )
{
    HwToggle
        *toggle = (HwToggle *)obj;
    hwInt32
        fg, bg, flag, bord, tm, togTM;
    HW_USE_CURR_DISP;

    /* Cook our images */
    if( !tmCooked ) {
        cookTM( __hwDisp );
    }

    if( toggle->dirty ) {
        cook( __hwDisp, toggle );
    }

    if( toggle->state ) {
        fg = toggle->fgActive;
        bg = toggle->bgActive;
        bord = toggle->activeBord;
        tm = toggle->activeTexture ? toggle->activeTmID : toggle->tmID;
    }
    else {
        fg = toggle->fgColor;
        bg = toggle->bgColor;
        bord = toggle->border;
        tm = toggle->tmID;
    }

    if( toggle->value ) {
        togTM = toggle->exclude ? fillTM : checkTM;
    }
    else {
        togTM = toggle->exclude ? hollowTM : boxTM;
    }

    __hwIntDrawButton( __hwDisp, toggle->label, toggle->fontHeight,
                       toggle->font, fg, bg,
                       bord, tm,
                       toggle->bounds, toggle->parentBounds, toggle->align,
                       togTM );
}

/*** EOF hwToggle.c ***/
