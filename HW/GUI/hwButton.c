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
        /* Specific to HwButton */
        hwStrPosX, hwStrPosY,
        hwStrWidth, hwStrHeight,
        hwStrFontHeight, hwStrLabel,
        hwStrColor, hwStrBackgroundColor,
        hwStrActiveColor, hwStrActiveBackground,
        hwStrCallback, hwStrID,
        hwStrFont, hwStrTexture,
        hwStrAlign, hwStrBorderColor,
        hwStrActiveBorderColor, hwStrActiveTexture,
        hwStrRepeat, hwStrRepeatDelay, hwStrRepeatInterval,
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
#define HW_ALIGN        16
#define HW_PARENT       17
#define HW_BOUNDS       18
#define HW_DIRTY        19
#define HW_BORDER       20
#define HW_ACTIVE_BORD  21
#define HW_ACTIVE_TEX   22
#define HW_REPEAT       23
#define HW_REPEAT_DEL   24
#define HW_REPEAT_INT   25
#define HW_ACTIVE       26
#define HW_EXECUTE      27

static struct _hwObjectStruct
    hwButtonStruct = {
        0,      /* Parent - NULL */
        "hwButton",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwButton = &hwButtonStruct;

/* Hash table for button strings */
static void *buttonTab = 0;

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
        parentBounds[4],        /* ...of parent */
        fontHeight,             /* Height of font chars */
        fgColor, bgColor,       /* Color of the widget */
        fgActive, bgActive,     /* Color while pressed */
        borderColor, borderActive, /* Border colors */
        repeats,                /* Does it repeat? */
        repeating,              /* *Is* it repeating? */
        tmID,                   /* Texture ID to use, if any */
        activeTmID,             /* Active texture ID to use, if any */
        active,                 /* Is the button active? */
        state,                  /* If so, what state is it in? */
        align,                  /* Alignment flags */
        ID;                     /* Identifier, for easy use in switch stmts */
    double
        whenClicked;            /* Time when clicked */
    hwFloat
        repeatDelay,            /* How long before it starts repeating? */
        repeatInterval;         /* How long between repeats? */
    hwObject
        texture,                /* Texture, if any */
        activeTexture;          /* Active texture, if any */
    TexFont
        *font;                  /* The font to use */
    hwObject
        fontObj;                /* The font object it came from */
    char
        *label;                 /* Child widgets */
    hwCallback
        callback;               /* Thing to call when button hit */
} HwButton;

static void cook( hwDisplay disp, HwButton *button );

static hwObject create( hwObject button )
{
    HwButton
        *result;
    void
        *t;

    if( button != hwButton )      return 0;

    HW_HASH_SETUP(buttonTab, 32)
        t = buttonTab;
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
        HW_INSERT( hwStrAlign,             HW_ALIGN,       t );
        HW_INSERT( hwStrParent,            HW_PARENT,      t );
        HW_INSERT( hwStrBounds,            HW_BOUNDS,      t );
        HW_INSERT( hwStrDirty,             HW_DIRTY,       t );
        HW_INSERT( hwStrBorderColor,       HW_BORDER,      t );
        HW_INSERT( hwStrActiveBorderColor, HW_ACTIVE_BORD, t );
        HW_INSERT( hwStrActiveTexture,     HW_ACTIVE_TEX,  t );
        HW_INSERT( hwStrRepeat,            HW_REPEAT,      t );
        HW_INSERT( hwStrRepeatDelay,       HW_REPEAT_DEL,  t );
        HW_INSERT( hwStrRepeatInterval,    HW_REPEAT_INT,  t );
        HW_INSERT( hwStrActive,            HW_ACTIVE,      t );
        HW_INSERT( hwStrExecute,           HW_EXECUTE,     t );
    HW_HASH_CLEANUP

    result = malloc( sizeof(HwButton) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }
    memset(result, 0, sizeof(HwButton));

    /* Class-specific initialization goes here */

    result->hdr = *button;
    result->hdr.parent = hwButton;
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
    result->borderColor = 0;
    result->borderActive = 0;
    result->label = NULL;
    result->texture = NULL;
    result->activeTexture = NULL;
    result->repeats = 0;
    result->repeating = 0;
    result->repeatDelay = 0.20;
    result->repeatInterval = 0.10;
    result->font = NULL;
    result->fontObj = NULL;
    result->ID = 0;
    result->align = 0;
    result->parent = NULL;
    result->tmID = -1;
    result->activeTmID = -1;
    result->dirty = 1;
    result->active = 0;
    result->state = 0;

    return (hwObject)result;
}

static void addref( hwObject obj )
{
    HwButton
        *button = (HwButton *)obj;;

    /* This method only rarely needs to be changed */
    button->refCount++;
}

static void destroy( hwObject obj )
{
    HwButton
        *button = (HwButton *)obj;
    int
        i, n;

    if( --button->refCount > 0 ) return;

    if( button->texture ) {
        button->texture->destroy( button->texture );
    }
    if( button->activeTexture ) {
        button->activeTexture->destroy( button->activeTexture );
    }
    if( button->fontObj ) {
        button->fontObj->destroy( button->fontObj );
    }
    if( button->label ) {
        free( button->label );
    }
    free( button );
}

static void doEvent( hwObject obj, hwWinEvent *event )
{
    HwButton
        *button;
    hwInt32
        dx, dy, w, h, cmd;
    HW_USE_CURR_DISP;

    button = (HwButton *)obj;

    if( (event->type == HW_INPUT_CONFIG) || button->dirty ) {
        cook( __hwDisp, button );
    }

    dx = event->pointer.x - button->bounds[0];
    dy = event->pointer.y - button->bounds[1];
    w = button->bounds[2];
    h = button->bounds[3];

    switch( event->type ) {
    case HW_INPUT_BUTTON_PRESS :
        if( (dx < 0) || (dy < 0) || (dx > w) || (dy > h) ) break;
        button->active = 1;
        button->state = 1;
        if( button->callback ) button->callback( obj, HW_CB_ACTIVATE );
        if( button->repeats ) {
            button->whenClicked = __hwDisp->getElapsedTime(__hwDisp);
        }
        break;
    case HW_INPUT_BUTTON_RELEASE :
        if( !button->active ) return;
        if( (dx < 0) || (dy < 0) || (dx > w) || (dy > h) ) {
            /* Cancelled */
            cmd = HW_CB_DEACTIVATE;
        }
        else {
            cmd = HW_CB_COMPLETE;
        }
        if( button->callback ) button->callback( obj, cmd );
        button->active = 0;
        button->state = 0;
        button->repeating = 0;
        break;
    case HW_INPUT_POINTER :
        if( !button->active ) return;
        if( (dx < 0) || (dy < 0) || (dx > w) || (dy > h) ) {
            button->state = 0;
        }
        else {
            button->state = 1;
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
    HwButton
        *button = (HwButton *)obj;
    const char 
        *str;
    hwInt32
        dirty = 1;

    switch( hwLookup( prop, buttonTab ) ) {
    case HW_POS_X :
        if( type == HW_TYPE_1F )      button->posX = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) button->posX = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_POS_Y :
        if( type == HW_TYPE_1F )      button->posY = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) button->posY = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_WIDTH :
        if( type == HW_TYPE_1F )      button->width = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) button->width = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_HEIGHT :
        if( type == HW_TYPE_1F )      button->height = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) button->height = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_FONT_HEIGHT :
        if( type == HW_TYPE_1F )      button->fontHeight = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) button->fontHeight = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_ALIGN :
        if( type == HW_TYPE_1F )      button->align = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) button->align = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_PARENT :
        if( type != HW_TYPE_OBJECT )   goto BadType;
        button->parent = (hwObject) val;
        break;
    case HW_LABEL :
        if( type != HW_TYPE_STRING )    goto BadType;
        str = val;
        if( button->label ) free( button->label );
        button->label = malloc( strlen(val) + 1 );
        if( !button->label ) {
            __hwIntSetError( HW_ERROR_NO_MEMORY );
            return;
        }
        strcpy( button->label, str );
        break;
    case HW_COLOR :
        button->fgColor = __hwIntGuiColor( type, val );
        break;
    case HW_BACKGROUND :
        button->bgColor = __hwIntGuiColor( type, val );
        break;
    case HW_ACTIVE_FG :
        button->fgActive = __hwIntGuiColor( type, val );
        break;
    case HW_ACTIVE_BG :
        button->bgActive = __hwIntGuiColor( type, val );
        break;
    case HW_ACTIVE :
        if( type != HW_TYPE_1B )        goto BadType;
        button->active = *(hwInt32 *)val;
        button->state = *(hwInt32 *)val;
        if( button->callback ) {
            if( button->active ) button->callback( obj, HW_CB_ACTIVATE );
            else                 button->callback( obj, HW_CB_DEACTIVATE );
        }
        break;
    case HW_EXECUTE :
        if( button->callback ) button->callback( obj, HW_CB_COMPLETE );
        dirty = 0; /* Does not change appearance */
        break;
    case HW_BORDER :
        button->borderColor = __hwIntGuiColor( type, val );
        break;
    case HW_ACTIVE_BORD :
        button->borderActive = __hwIntGuiColor( type, val );
        break;
    case HW_REPEAT :
        dirty = 0; /* Does not change appearance */
        if( type != HW_TYPE_1B )        goto BadType;
        button->repeats = *(hwInt32 *)val;
        break;
    case HW_REPEAT_DEL :
        dirty = 0; /* Does not change appearance */
        if( type != HW_TYPE_1F )        goto BadType;
        button->repeatDelay = *(hwFloat *)val;
        break;
    case HW_REPEAT_INT :
        dirty = 0; /* Does not change appearance */
        if( type != HW_TYPE_1F )        goto BadType;
        button->repeatInterval = *(hwFloat *)val;
        break;
    case HW_CALLBACK :
        dirty = 0; /* Does not change appearance */
        if( type == HW_TYPE_STRING ) {
            button->callback = hwFindCallback( (char *)val );
        }
        else if( type == HW_TYPE_CALLBACK ) {
            button->callback = (hwCallback) val;
        }
        else {
            goto BadType;
        }
        break;
    case HW_ID :
        dirty = 0; /* Does not change appearance */
        if( type == HW_TYPE_1F )      button->ID = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) button->ID = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_FONT :
        if( type != HW_TYPE_OBJECT )    goto BadType;
        if( button->fontObj ) {
            button->fontObj->destroy( button->fontObj );
        }
        button->fontObj = (hwObject) val;
        button->fontObj->addref( button->fontObj );
        button->font = NULL;
        break;
    case HW_TEXTURE :
        if( type != HW_TYPE_OBJECT )    goto BadType;
        if( button->texture ) button->texture->destroy( button->texture );
        button->texture = (hwObject) val;
        button->texture->addref( button->texture );
        button->tmID = -1;
        break;
    case HW_ACTIVE_TEX :
        if( type != HW_TYPE_OBJECT )    goto BadType;
        if( button->activeTexture ) {
            button->activeTexture->destroy( button->activeTexture );
        }
        button->activeTexture = (hwObject) val;
        button->activeTexture->addref( button->activeTexture );
        button->activeTmID = -1;
        break;
    case HW_EVENT :
        if( type != HW_TYPE_EVENT )    goto BadType;
        dirty = 0; /* Does not change appearance */
        doEvent( obj, (hwWinEvent *)val );
        break;
    case HW_DIRTY :
        if( button->fontObj ) {
            button->fontObj->modify( button->fontObj, prop, type, val );
        }
        if( button->texture ) {
            button->texture->modify( button->texture, prop, type, val );
        }
        break;
    default :
        /* If you don't recognize a string, make sure to set an error */
        __hwIntSetError( HW_ERROR_BAD_PROP );
        break;
    }

    if( dirty ) button->dirty = 1;
    return;

BadType:
    /* You should check the types of properties, and "goto BadType"
     * if they don't match what you expect
     */
    __hwIntSetError( HW_ERROR_BAD_TYPE );
}

static hwInt32 inquire( hwObject obj, const char *prop, void **val )
{
    HwButton
        *button = (HwButton *)obj;
    HW_USE_CURR_DISP;

    if( button->dirty ) {
        cook( __hwDisp, button );
    }
    switch( hwLookup( prop, buttonTab ) ) {
    case HW_POS_X :
        *val = &button->posX;
        return HW_TYPE_1I;
    case HW_POS_Y :
        *val = &button->posY;
        return HW_TYPE_1I;
    case HW_WIDTH :
        *val = &button->width;
        return HW_TYPE_1I;
    case HW_HEIGHT :
        *val = &button->height;
        return HW_TYPE_1I;
    case HW_ALIGN :
        *val = &button->align;
        return HW_TYPE_1I;
    case HW_PARENT :
        *val = button->parent;
        return HW_TYPE_OBJECT;
    case HW_BOUNDS :
        *val = button->bounds;
        return HW_TYPE_4I;
    case HW_FONT_HEIGHT :
        *val = &button->fontHeight;
        return HW_TYPE_1I;
    case HW_LABEL :
        *val = button->label;
        return HW_TYPE_STRING;
    case HW_COLOR :
        *val = &button->fgColor;
        return HW_TYPE_1I;
    case HW_BACKGROUND :
        *val = &button->bgColor;
        return HW_TYPE_1I;
    case HW_ACTIVE_FG :
        *val = &button->fgActive;
        return HW_TYPE_1I;
    case HW_ACTIVE :
        *val = &button->active;
        return HW_TYPE_1B;
    case HW_ACTIVE_BG :
        *val = &button->bgActive;
        return HW_TYPE_1I;
    case HW_BORDER :
        *val = &button->borderColor;
        return HW_TYPE_1I;
    case HW_ACTIVE_BORD :
        *val = &button->borderActive;
        return HW_TYPE_1I;
    case HW_REPEAT :
        *val = &button->repeats;
        return HW_TYPE_1B;
    case HW_REPEAT_DEL :
        *val = &button->repeatDelay;
        return HW_TYPE_1F;
    case HW_REPEAT_INT :
        *val = &button->repeatInterval;
        return HW_TYPE_1F;
    case HW_CALLBACK :
        *val = (void *)button->callback;
        return HW_TYPE_CALLBACK;
    case HW_ID :
        *val = (void *)&button->ID;
        return HW_TYPE_1I;
    case HW_FONT :
        *val = button->fontObj;
        return HW_TYPE_OBJECT;
    case HW_TEXTURE :
        *val = button->texture;
        return HW_TYPE_OBJECT;
    case HW_ACTIVE_TEX :
        *val = button->activeTexture;
        return HW_TYPE_OBJECT;
    default :
        __hwIntSetError( HW_ERROR_BAD_PROP );
        return 0;
    }

    /* NOTREACHED */
    return 0;
}

static void cook( hwDisplay disp, HwButton *button )
{
    void
        *iPtr;
    hwObject
        obj;

    button->dirty = 0;

    __hwIntGetParentBounds( disp, button->parent, button->parentBounds );

    __hwIntPositionWidget( button->bounds, button->parentBounds,
                           button->posX, button->posY,
                           button->width, button->height,
                           button->align,
                           0.0 );

    if( button->texture ) {
        obj = button->texture;
        obj->draw( obj );
        if( obj->inquire( obj, hwStrTextureID, &iPtr ) == HW_TYPE_1I ) {
            button->tmID = *(hwInt32 *)iPtr;
        }
    }
    if( button->activeTexture ) {
        obj = button->activeTexture;
        obj->draw( obj );
        if( obj->inquire( obj, hwStrTextureID, &iPtr ) == HW_TYPE_1I ) {
            button->activeTmID = *(hwInt32 *)iPtr;
        }
    }
    if( !button->font ) {
        obj = button->fontObj;
        if( obj && (obj->inquire( obj, hwStrFont, &iPtr ) == HW_TYPE_FONT) ) {
            button->font = (TexFont *) iPtr;
        }
        if( !button->font ) {
            button->font = txfLoadStaticFont( button->fontHeight );
        }
    }
}

static void draw( hwObject obj )
{
    HwButton
        *button = (HwButton *)obj;
    hwInt32
        tm, fg, bg, bord;
    double
        dt;
    HW_USE_CURR_DISP;

    if( button->dirty ) {
        cook( __hwDisp, button );
    }

    if( button->state ) {
        fg = button->fgActive;
        bg = button->bgActive;
        tm = button->activeTexture ? button->activeTmID : button->tmID;
        bord = button->borderActive;
    }
    else {
        fg = button->fgColor;
        bg = button->bgColor;
        tm = button->tmID;
        bord = button->borderColor;
    }
    __hwIntDrawButton( __hwDisp, button->label, button->fontHeight,
                       button->font, fg, bg, bord, tm,
                       button->bounds, button->parentBounds, button->align,
                       -1 );
    /* TBD: this may not be the best place to put this but we know it
     * happens once per frame
     */
    if( button->active && button->repeats && button->callback ) {
        dt = __hwDisp->getElapsedTime(__hwDisp) - button->whenClicked;
        if( button->repeating ) {
            if( dt >= button->repeatInterval ) {
                /* Send another command */
                button->callback( obj, HW_CB_ACTIVATE );
                /* Reset timer */
                button->whenClicked += button->repeatInterval;
            }
        }
        else {
            if( dt >= button->repeatDelay ) {
                /* Send the first command */
                button->callback( obj, HW_CB_ACTIVATE );
                /* Reset timer */
                button->whenClicked += button->repeatDelay;
                button->repeating = 1;
            }
        }
    }
}

/*** EOF hwButton.c ***/
