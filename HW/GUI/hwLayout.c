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
        /* Specific to Layout */
        hwStrChildren,
        hwStrPosX, hwStrPosY,
        hwStrWidth, hwStrHeight,
        hwStrAlign, hwStrBackgroundColor,
        hwStrInvisible,

        /* End of list */
        0
    };

#define HW_CHILDREN     1
#define HW_POS_X        2
#define HW_POS_Y        3
#define HW_WIDTH        4
#define HW_HEIGHT       5
#define HW_EVENT        6
#define HW_ALIGN        7
#define HW_PARENT       8
#define HW_BOUNDS       9
#define HW_ASPECT       10
#define HW_BACKGROUND   11
#define HW_INVISIBLE    12

static struct _hwObjectStruct
    hwLayoutStruct = {
        0,      /* Parent - NULL */
        "hwLayout",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwLayout = &hwLayoutStruct;

/* Hash table for layout strings */
static void *layoutTab = 0;

typedef struct { hwInt32 x, y, w, h; } lBBox;

typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount;               /* Reference count */
    hwObject
        parent;                 /* Parent container */
    hwInt32
        posX, posY,             /* Position and size */
        width, height,          /* for children who are REL_* */
        bounds[4],              /* Bounding box */
        parentBounds[4],        /* ... of the parent container */
        background,             /* Background color */
        dirty,                  /* Need to re-cook bounds? */
        active,                 /* Active child */
        invisible,              /* Invisible? */
        align,                  /* Alignment flags */
        numChildren;            /* Number of children */
    hwFloat
        aspectRatio;            /* Desired aspect ratio, 0 if none */
    lBBox
        *chBounds;           /* Array of bounding boxes for children */
    hwObject
        *children;              /* Child widgets */
} Layout;

static void cook( hwDisplay disp, Layout *layout );

static hwObject create( hwObject layout )
{
    Layout
        *result;
    void
        *t;

    if( layout != hwLayout )      return 0;

    HW_HASH_SETUP(layoutTab, 24)
        t = layoutTab;
        HW_INSERT( hwStrPosX,            HW_POS_X,      t );
        HW_INSERT( hwStrPosY,            HW_POS_Y,      t );
        HW_INSERT( hwStrWidth,           HW_WIDTH,      t );
        HW_INSERT( hwStrHeight,          HW_HEIGHT,     t );
        HW_INSERT( hwStrChildren,        HW_CHILDREN,   t );
        HW_INSERT( hwStrEvent,           HW_EVENT,      t );
        HW_INSERT( hwStrAlign,           HW_ALIGN,      t );
        HW_INSERT( hwStrParent,          HW_PARENT,     t );
        HW_INSERT( hwStrBounds,          HW_BOUNDS,     t );
        HW_INSERT( hwStrAspect,          HW_ASPECT,     t );
        HW_INSERT( hwStrBackgroundColor, HW_BACKGROUND, t );
        HW_INSERT( hwStrInvisible,       HW_INVISIBLE,  t );
    HW_HASH_CLEANUP

    result = malloc( sizeof(Layout) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }
    memset(result, 0, sizeof(Layout));

    /* Class-specific initialization goes here */

    result->hdr = *layout;
    result->hdr.parent = hwLayout;
    result->hdr.name = 0;

    result->refCount = 1;

    result->numChildren = 0;
    result->active = -1;
    result->chBounds = NULL;
    result->children = NULL;
    result->align = 0;
    result->background = 0;
    result->invisible = 0;
    result->aspectRatio = 0.0;
    result->parent = NULL;
    result->dirty = 1;

    return (hwObject)result;
}

static void addref( hwObject obj )
{
    Layout
        *layout = (Layout *)obj;;

    /* This method only rarely needs to be changed */
    layout->refCount++;
}

static void destroy( hwObject obj )
{
    Layout
        *layout = (Layout *)obj;
    int
        i, n;

    if( --layout->refCount > 0 ) return;

    n = layout->numChildren;
    for( i = 0; i < n; i++ ) {
        if( layout->children[i] ) {
            layout->children[i]->destroy( layout->children[i] );
        }
    }
    if( layout->children ) {
        free( layout->children );
    }
    if( layout->chBounds ) {
        free( layout->chBounds );
    }
    free( layout );
}

static void doEvent( hwObject obj, hwWinEvent *event )
{
    Layout
        *layout;
    hwInt32
        i, n, x, y, w, h, cx, cy, cw, ch;
    hwObject
        child;
    hwWinEvent
        cookEvent;
    HW_USE_CURR_DISP;

    layout = (Layout *)obj;

    if( (event->type == HW_INPUT_CONFIG) || layout->dirty ) {
        cook( __hwDisp, layout );
    }

    cookEvent = *event;
    cookEvent.pointer.x -= layout->bounds[0];
    cookEvent.pointer.y -= layout->bounds[1];

    x = cookEvent.pointer.x;    y = cookEvent.pointer.y;
    w = layout->bounds[2];      h = layout->bounds[3];

    switch( event->type ) {
    case HW_INPUT_BUTTON_PRESS :
        if( (x < 0) || (y < 0) || (x > w) || (y > h) ) break;
        n = layout->numChildren;
        for( i = 0; i < n; i++ ) {
            cx = x - layout->chBounds[i].x;
            cy = y - layout->chBounds[i].y;
            cw = layout->chBounds[i].w;
            ch = layout->chBounds[i].h;
            if( (cx >= 0) && (cy >= 0) && (cx <= cw) && (cy <= ch) ) break;
        }
        if( i >= n ) break;
        layout->active = i;
        child = layout->children[i];
        child->modify( child, hwStrEvent, HW_TYPE_EVENT, &cookEvent );
        break;
    case HW_INPUT_BUTTON_RELEASE :
        if( layout->active < 0 ) return;
        n = layout->active;
        child = layout->children[n];
        child->modify( child, hwStrEvent, HW_TYPE_EVENT, &cookEvent );
        layout->active = -1;
        break;
    case HW_INPUT_POINTER :
        if( layout->active < 0 ) return;
        n = layout->active;
        child = layout->children[n];
        child->modify( child, hwStrEvent, HW_TYPE_EVENT, &cookEvent );
        break;
    case HW_INPUT_CONFIG :
        n = layout->numChildren;
        for( i = 0; i < n; i++ ) {
            child = layout->children[i];
            child->modify( child, hwStrEvent, HW_TYPE_EVENT, event );
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
    Layout
        *layout = (Layout *)obj;
    int
        i, n,
        dirty = 1;      /* Presume that mods make the object dirty */
    const hwObject
        *list;

    switch( hwLookup( prop, layoutTab ) ) {
    case HW_POS_X :
        if( type == HW_TYPE_1F )      layout->posX = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) layout->posX = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_POS_Y :
        if( type == HW_TYPE_1F )      layout->posY = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) layout->posY = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_WIDTH :
        if( type == HW_TYPE_1F )      layout->width = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) layout->width = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_HEIGHT :
        if( type == HW_TYPE_1F )      layout->height = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) layout->height = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_ALIGN :
        if( type == HW_TYPE_1F )      layout->align = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) layout->align = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_ASPECT :
        if( type != HW_TYPE_1F )        goto BadType;
        layout->aspectRatio = *(hwFloat *)val;
        break;
    case HW_BACKGROUND :
        layout->background = __hwIntGuiColor( type, val );
        goto ModKids;
    case HW_INVISIBLE :
        if( type != HW_TYPE_1B )        goto BadType;
        layout->invisible = *(hwInt32 *)val;
        goto ModKids;
    case HW_CHILDREN :
        if( HW_GET_BASE(type) != HW_TYPE_OBJECT )    goto BadType;
        if( layout->children ) {
            n = layout->numChildren;
            for( i = 0; i < n; i++ ) {
                if( layout->children [i] ) {
                    layout->children[i]->destroy( layout->children[i] );
                }
            }
            free( layout->children );
            free( layout->chBounds );
            layout->numChildren = 0;
        }
        n = HW_GET_COUNT(type);
        layout->children = malloc( n * sizeof(hwObject) );
        layout->chBounds = malloc( n * sizeof(lBBox) );
        if( !(layout->children && layout->chBounds) ) {
            __hwIntSetError( HW_ERROR_NO_MEMORY );
            break;
        }
        layout->numChildren = n;
        list = val;
        for( i = 0; i < n; i++ ) {
            layout->children[i] = list[i];
            list[i]->addref( list[i] );
        }
        break;
    case HW_PARENT :
        if( type != HW_TYPE_OBJECT ) goto BadType;
        layout->parent = (hwObject)val;
        break;
    case HW_EVENT :
        if( type != HW_TYPE_EVENT ) goto BadType;
        dirty = 0;
        doEvent( obj, (hwWinEvent *)val );
        break;
    default :
ModKids :
        n = layout->numChildren;
        list = layout->children;
        for( i = 0; i < n; i++ ) {
            list[i]->modify( list[i], prop, type, val );
        }
        break;
    }

    if (dirty) layout->dirty = 1;
    return;

BadType:
    /* You should check the types of properties, and "goto BadType"
     * if they don't match what you expect
     */
    __hwIntSetError( HW_ERROR_BAD_TYPE );
}

static hwInt32 inquire( hwObject obj, const char *prop, void **val )
{
    Layout
        *layout = (Layout *)obj;
    HW_USE_CURR_DISP;

    if( layout->dirty ) {
        cook( __hwDisp, layout );
    }
    switch( hwLookup( prop, layoutTab ) ) {
    case HW_ALIGN :
        *val = &layout->align;
        return HW_TYPE_1I;
    case HW_PARENT :
        *val = layout->parent;
        return HW_TYPE_OBJECT;
    case HW_BOUNDS :
        *val = layout->bounds;
        return HW_TYPE_4I;
    case HW_POS_X :
        *val = &layout->posX;
        return HW_TYPE_1I;
    case HW_POS_Y :
        *val = &layout->posY;
        return HW_TYPE_1I;
    case HW_WIDTH :
        *val = &layout->width;
        return HW_TYPE_1I;
    case HW_HEIGHT :
        *val = &layout->height;
        return HW_TYPE_1I;
    case HW_ASPECT :
        *val = &layout->aspectRatio;
        return HW_TYPE_1F;
    case HW_BACKGROUND :
        *val = &layout->background;
        return HW_TYPE_1I;
    case HW_INVISIBLE :
        *val = &layout->invisible;
        return HW_TYPE_1B;
    case HW_CHILDREN :
        *val = layout->children;
        return HW_MAKE_TYPE(HW_TYPE_OBJECT, layout->numChildren);
    default :
        __hwIntSetError( HW_ERROR_BAD_PROP );
        return 0;
    }

    /* NOTREACHED */
    return 0;
}

static void cook( hwDisplay disp, Layout *layout )
{
    hwInt32
        i, n, x, y, w, h;
    hwObject
        child;
    void
        *ptr;

    layout->dirty = 0;

    __hwIntGetParentBounds( disp, layout->parent, layout->parentBounds );

    __hwIntPositionWidget( layout->bounds, layout->parentBounds,
                           layout->posX, layout->posY,
                           layout->width, layout->height,
                           layout->align,
                           layout->aspectRatio );

    n = layout->numChildren;
    for( i = 0; i < n; i++ ) {
        child = layout->children[i];
        x = y = w = h = 0;

        child->modify( child, hwStrParent, HW_TYPE_OBJECT, layout );

        if( child->inquire( child, hwStrBounds, &ptr ) == HW_TYPE_4I ) {
            x = ((hwInt32 *)ptr)[0];
            y = ((hwInt32 *)ptr)[1];
            w = ((hwInt32 *)ptr)[2];
            h = ((hwInt32 *)ptr)[3];
        }

        layout->chBounds[i].x = x;
        layout->chBounds[i].y = y;
        layout->chBounds[i].w = w;
        layout->chBounds[i].h = h;
    }
}

static void draw( hwObject obj )
{
    Layout
        *layout = (Layout *)obj;
    hwInt32
        bg, flags,
        i, n;
    hwObject
        child;
    HW_USE_CURR_DISP;

    if( layout->dirty ) {
        cook( __hwDisp, layout );
    }

    bg = layout->background;
    if( bg && !layout->invisible ) {
        flags = ((bg & 0xFF000000) != 0xFF000000) ? HW_GUI_BLEND : 0;
        __hwDisp->guiRectangle( __hwDisp, flags, bg,
                            layout->bounds[0], layout->bounds[1],
                            layout->bounds[2], layout->bounds[3],
                            0 );
    }

    n = layout->numChildren;
    for( i = 0; i < n; i++ ) {
        child = layout->children[i];
        child->draw( child );
    }
}

/*** EOF hwLayout.c ***/
