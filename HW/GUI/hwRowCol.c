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
        /* Specific to RowCol */
        hwStrPosX, hwStrPosY,
        hwStrWidth, hwStrHeight,
        hwStrRows, hwStrColumns,
        hwStrChildren,
        hwStrPadding,
        hwStrBackgroundColor,
        hwStrTexture,
        hwStrInvisible,
        hwStrAlign,

        /* End of list */
        0
    };

#define HW_POS_X        1
#define HW_POS_Y        2
#define HW_WIDTH        3
#define HW_HEIGHT       4
#define HW_ROWS         5
#define HW_COLS         6
#define HW_CHILDREN     7
#define HW_EVENT        8
#define HW_PADDING      9
#define HW_BG           10
#define HW_TEXTURE      11
#define HW_INVISIBLE    12
#define HW_ALIGN        13
#define HW_PARENT       14
#define HW_BOUNDS       15
#define HW_DIRTY        16

static struct _hwObjectStruct
    hwRowColStruct = {
        0,      /* Parent - NULL */
        "hwRowCol",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwRowCol = &hwRowColStruct;

/* Hash table for rowcol strings */
static void *rowcolTab = 0;

typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount,               /* Reference count */
        dirty;                  /* Dirty flag */
    hwObject
        parent;                 /* Parent container */
    hwInt32
        posX, posY,             /* Position of widget */
        width, height,          /* Size of widget */
        bounds[4],              /* Bounding box */
        parentBounds[4],        /* ... of the parent container */
        rows, cols,             /* Arrangement of children */
        childW, childH,         /* Calculated size */
        bgColor,                /* Background color */
        invisible,              /* If invisible, don't draw BG */
        active,                 /* Active child index */
        pad[2],                 /* Spacing around children */
        tmID,                   /* BG texture ID, if any */
        align,                  /* Alignment flags */
        numChildren;            /* Number of children */
    hwObject
        texture,                /* Background texture, if any */
        *children;              /* Child widgets */
} RowCol;

static void cook( RowCol *rowcol, hwDisplay disp );

static hwObject create( hwObject rowcol )
{
    RowCol
        *result;
    void
        *t;

    if( rowcol != hwRowCol )      return 0;

    HW_HASH_SETUP(rowcolTab, 24)
        t = rowcolTab;
        HW_INSERT( hwStrPosX,            HW_POS_X,     t );
        HW_INSERT( hwStrPosY,            HW_POS_Y,     t );
        HW_INSERT( hwStrWidth,           HW_WIDTH,     t );
        HW_INSERT( hwStrHeight,          HW_HEIGHT,    t );
        HW_INSERT( hwStrRows,            HW_ROWS,      t );
        HW_INSERT( hwStrColumns,         HW_COLS,      t );
        HW_INSERT( hwStrChildren,        HW_CHILDREN,  t );
        HW_INSERT( hwStrEvent,           HW_EVENT,     t );
        HW_INSERT( hwStrPadding,         HW_PADDING,   t );
        HW_INSERT( hwStrBackgroundColor, HW_BG,        t );
        HW_INSERT( hwStrTexture,         HW_TEXTURE,   t );
        HW_INSERT( hwStrInvisible,       HW_INVISIBLE, t );
        HW_INSERT( hwStrAlign,           HW_ALIGN,     t );
        HW_INSERT( hwStrParent,          HW_PARENT,    t );
        HW_INSERT( hwStrBounds,          HW_BOUNDS,    t );
        HW_INSERT( hwStrDirty,           HW_DIRTY,     t );
    HW_HASH_CLEANUP

    result = malloc( sizeof(RowCol) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }
    memset(result, 0, sizeof(RowCol));

    /* Class-specific initialization goes here */

    result->hdr = *rowcol;
    result->hdr.parent = hwRowCol;
    result->hdr.name = 0;

    result->refCount = 1;
    result->dirty = 1;

    result->width = result->height = 100;
    result->posX = result->posY = 0;
    result->rows = result->cols = 1;
    result->numChildren = 0;
    result->texture = NULL;
    result->tmID = -1;
    result->active = -1;
    result->invisible = 0;
    result->align = 0;
    result->parent = NULL;
    result->bgColor = 0;        /* Default to no BG color */
    result->pad[0] = result->pad[1] = 0;
    result->children = NULL;


    return (hwObject)result;
}

static void addref( hwObject obj )
{
    RowCol
        *rowcol = (RowCol *)obj;;

    /* This method only rarely needs to be changed */
    rowcol->refCount++;
}

static void destroy( hwObject obj )
{
    RowCol
        *rowcol = (RowCol *)obj;
    int
        i, n;

    if( --rowcol->refCount > 0 ) return;

    if( rowcol->texture ) rowcol->texture->destroy( rowcol->texture );
    n = rowcol->numChildren;
    for( i = 0; i < n; i++ ) {
        if( rowcol->children[i] ) {
            rowcol->children[i]->destroy( rowcol->children[i] );
        }
    }
    if( rowcol->children ) {
        free( rowcol->children );
    }
    free( rowcol );
}

static void doEvent( hwObject obj, hwWinEvent *event )
{
    RowCol
        *rowcol;
    hwInt32
        i, j, n,
        x, y, w, h,
        cmd;
    hwObject
        child;
    hwWinEvent
        cookEvent;
    HW_USE_CURR_DISP;

    rowcol = (RowCol *)obj;

    if( (event->type == HW_INPUT_CONFIG) || rowcol->dirty ) {
        cook( rowcol, __hwDisp );
    }

    cookEvent = *event;
    cookEvent.pointer.x -= rowcol->bounds[0];
    cookEvent.pointer.y -= rowcol->bounds[1];

    x = cookEvent.pointer.x;
    y = cookEvent.pointer.y;
    w = rowcol->bounds[2];
    h = rowcol->bounds[3];

    switch( event->type ) {
    case HW_INPUT_BUTTON_PRESS :
        if( (x < 0) || (y < 0) || (x > w) || (y > h) ) break;
        i = y / rowcol->childH;
        j = x / rowcol->childW;
        n = i*rowcol->cols + j;
        if( n >= rowcol->numChildren ) break;
        rowcol->active = n;
        child = rowcol->children[n];
        child->modify( child, hwStrEvent, HW_TYPE_EVENT, &cookEvent );
        break;
    case HW_INPUT_BUTTON_RELEASE :
        if( rowcol->active < 0 ) return;
        n = rowcol->active;
        child = rowcol->children[n];
        child->modify( child, hwStrEvent, HW_TYPE_EVENT, &cookEvent );
        rowcol->active = -1;
        break;
    case HW_INPUT_POINTER :
        if( rowcol->active < 0 ) return;
        n = rowcol->active;
        child = rowcol->children[n];
        child->modify( child, hwStrEvent, HW_TYPE_EVENT, &cookEvent );
        break;
    case HW_INPUT_CONFIG :
        /* Tell all the children about the config event */
        n = 0;
        for( i = 0; i < rowcol->rows; i++ ) {
            for( j = 0; j < rowcol->cols; j++ ) {
                /* If this cell doesn't exist, all done */
                if( n >= rowcol->numChildren ) break;
                child = rowcol->children[n];

                child->modify( child, hwStrEvent, HW_TYPE_EVENT, event );
                n++;
            }
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
    RowCol
        *rowcol = (RowCol *)obj;
    int
        i, n,
        dirty = 1;      /* Presume that mods make the object dirty */
    const hwObject
        *list;

    switch( hwLookup( prop, rowcolTab ) ) {
    case HW_POS_X :
        if( type == HW_TYPE_1F )      rowcol->posX = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) rowcol->posX = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_POS_Y :
        if( type == HW_TYPE_1F )      rowcol->posY = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) rowcol->posY = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_WIDTH :
        if( type == HW_TYPE_1F )      rowcol->width = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) rowcol->width = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_HEIGHT :
        if( type == HW_TYPE_1F )      rowcol->height = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) rowcol->height = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_ROWS :
        if( type == HW_TYPE_1F )      rowcol->rows = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) rowcol->rows = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_COLS :
        if( type == HW_TYPE_1F )      rowcol->cols = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) rowcol->cols = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_ALIGN :
        if( type == HW_TYPE_1F )      rowcol->align = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) rowcol->align = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_INVISIBLE :
        if( type != HW_TYPE_1B )      goto BadType;
        rowcol->invisible = *(hwInt32 *)val;
        break;
    case HW_PARENT :
        if( type != HW_TYPE_OBJECT )  goto BadType;
        rowcol->parent = (hwObject)val;
        break;
    case HW_CHILDREN :
        if( HW_GET_BASE(type) != HW_TYPE_OBJECT )    goto BadType;
        if( rowcol->children ) {
            n = rowcol->numChildren;
            for( i = 0; i < n; i++ ) {
                if( rowcol->children [i] ) {
                    rowcol->children[i]->destroy( rowcol->children[i] );
                }
            }
            free( rowcol->children );
            rowcol->numChildren = 0;
        }
        n = HW_GET_COUNT(type);
        rowcol->children = malloc( n * sizeof(hwObject) );
        if( !rowcol->children ) {
            __hwIntSetError( HW_ERROR_NO_MEMORY );
            break;
        }
        rowcol->numChildren = n;
        list = val;
        for( i = 0; i < n; i++ ) {
            rowcol->children[i] = list[i];
            list[i]->addref( list[i] );
        }
        break;
    case HW_TEXTURE :
        if( type != HW_TYPE_OBJECT ) goto BadType;
        if( rowcol->texture ) rowcol->texture->destroy( rowcol->texture );
        rowcol->texture = (hwObject) val;
        rowcol->texture->addref( rowcol->texture );
        rowcol->tmID = -1;
        goto ModKids;
    case HW_DIRTY :
        if( rowcol->texture ) {
            rowcol->texture->modify( rowcol->texture, prop, type, val );
        }
        goto ModKids;
    case HW_PADDING :
        if( type == HW_TYPE_2F ) {
            rowcol->pad[0] = ((hwFloat *)val)[0];
            rowcol->pad[1] = ((hwFloat *)val)[1];
        }
        else if( type == HW_TYPE_2I ) {
            rowcol->pad[0] = ((hwInt32 *)val)[0];
            rowcol->pad[1] = ((hwInt32 *)val)[1];
        }
        else {
            goto BadType;
        }
        break;
    case HW_BG :
        rowcol->bgColor = __hwIntGuiColor( type, val );
        goto ModKids;
    case HW_EVENT :
        if( type != HW_TYPE_EVENT ) goto BadType;
        dirty = 0;      /* Does not change appearance */
        doEvent( obj, (hwWinEvent *)val );
        break;
    default :
ModKids :
        n = rowcol->numChildren;
        list = rowcol->children;
        for( i = 0; i < n; i++ ) {
            list[i]->modify( list[i], prop, type, val );
        }
        break;
    }

    if (dirty) rowcol->dirty = 1;
    return;

BadType:
    /* You should check the types of properties, and "goto BadType"
     * if they don't match what you expect
     */
    __hwIntSetError( HW_ERROR_BAD_TYPE );
}

static hwInt32 inquire( hwObject obj, const char *prop, void **val )
{
    RowCol
        *rowcol = (RowCol *)obj;
    HW_USE_CURR_DISP;

    if( rowcol->dirty ) {
        cook( rowcol, __hwDisp );
    }

    switch( hwLookup( prop, rowcolTab ) ) {
    case HW_POS_X :
        *val = &rowcol->posX;
        return HW_TYPE_1I;
    case HW_POS_Y :
        *val = &rowcol->posY;
        return HW_TYPE_1I;
    case HW_WIDTH :
        *val = &rowcol->width;
        return HW_TYPE_1I;
    case HW_HEIGHT :
        *val = &rowcol->height;
        return HW_TYPE_1I;
    case HW_ROWS :
        *val = &rowcol->rows;
        return HW_TYPE_1I;
    case HW_COLS :
        *val = &rowcol->cols;
        return HW_TYPE_1I;
    case HW_CHILDREN :
        *val = rowcol->children;
        return HW_MAKE_TYPE(HW_TYPE_OBJECT, rowcol->numChildren);
    case HW_TEXTURE :
        *val = rowcol->texture;
        return HW_TYPE_OBJECT;
    case HW_INVISIBLE :
        *val = &rowcol->invisible;
        return HW_TYPE_1B;
    case HW_ALIGN :
        *val = &rowcol->align;
        return HW_TYPE_1I;
    case HW_PARENT :
        *val = rowcol->parent;
        return HW_TYPE_OBJECT;
    case HW_BOUNDS :
        *val = rowcol->bounds;
        return HW_TYPE_4I;
    case HW_PADDING :
        *val = rowcol->pad;
        return HW_TYPE_2I;
    case HW_BG :
        *val = &rowcol->bgColor;
        return HW_TYPE_1I;
    default :
        __hwIntSetError( HW_ERROR_BAD_PROP );
        return 0;
    }

    /* NOTREACHED */
    return 0;
}

static void cook( RowCol *rowcol, hwDisplay disp )
{
    hwInt32
        align,
        padX, padY,
        i, j, n, tx, ty, aw, ah, ax, ay;
    hwObject
        child;
    void
        *ptr;

    rowcol->dirty = 0;

    /* Cook the bounds */
    __hwIntGetParentBounds( disp, rowcol->parent, rowcol->parentBounds );

    __hwIntPositionWidget( rowcol->bounds, rowcol->parentBounds,
                           rowcol->posX, rowcol->posY,
                           rowcol->width, rowcol->height,
                           rowcol->align,
                           0.0 );

    /* Calculate width/height of grid cell */
    rowcol->childW = rowcol->bounds[2] / rowcol->cols;
    rowcol->childH = rowcol->bounds[3] / rowcol->rows;

    /* Subtract padding to create adjusted width/height */
    padX = rowcol->pad[0];
    padY = rowcol->pad[1];

    if( rowcol->align & HW_GUI_FRACTIONAL ) {
        padX = (padX*rowcol->parentBounds[2] + 500) / 1000;
        padY = (padY*rowcol->parentBounds[3] + 500) / 1000;
    }

    aw = rowcol->childW - 2*padX;
    ah = rowcol->childH - 2*padY;

    /* For each cell... */
    n = 0;
    for( i = 0; i < rowcol->rows; i++ ) {
        /* Y pos and adjusted Y pos of current row */
        ty = i * rowcol->childH;
        ay = ty + padY;

        for( j = 0; j < rowcol->cols; j++ ) {
            /* If this cell doesn't exist, all done */
            if( n >= rowcol->numChildren ) break;
            child = rowcol->children[n];

            /* X pos and adjusted X pos of current col */
            tx = j * rowcol->childW;
            ax = tx + padX;

            /* Clear out unsupported alignment flags */
            if( child->inquire( child, hwStrAlign, &ptr ) == HW_TYPE_1I ) {
                align = *(hwInt32 *)ptr;
                align &= ~HW_GUI_POS_MASK;
                HW_MODIFY_1I( child, hwStrAlign, align );
            }

            /* Tell the child about the parent */
            child->modify( child, hwStrParent, HW_TYPE_OBJECT, rowcol );

            /* Set the actual position / size of child */
            HW_MODIFY_1I( child, hwStrPosX, ax );
            HW_MODIFY_1I( child, hwStrPosY, ay );
            HW_MODIFY_1I( child, hwStrWidth, aw );
            HW_MODIFY_1I( child, hwStrHeight, ah );

            /* Advance to next child */
            n++;
        }
    }
    child = rowcol->texture;
    if( child ) {
        rowcol->tmID = -1;
        child->draw( child );
        if( child->inquire( child, hwStrTextureID, &ptr ) == HW_TYPE_1I ) {
            rowcol->tmID = *(hwInt32 *)ptr;
        }
    }
}

static void draw( hwObject obj )
{
    RowCol
        *rowcol = (RowCol *)obj;
    hwInt32
        flags, bg,
        i, j, n;
    hwObject
        child;
    HW_USE_CURR_DISP;

    if( rowcol->dirty ) {
        cook( rowcol, __hwDisp );
    }

    bg = rowcol->bgColor;
    if( (bg || (rowcol->tmID >= 0)) && !rowcol->invisible ) {
        if( rowcol->tmID >= 0 ) {
            __hwDisp->guiRaster( __hwDisp, 
                                rowcol->bounds[0], rowcol->bounds[1],
                                rowcol->bounds[2], rowcol->bounds[3],
                                bg, rowcol->tmID );
        }
        else {
            flags = ((bg & 0xFF000000) != 0xFF000000) ? HW_GUI_BLEND : 0;
            __hwDisp->guiRectangle( __hwDisp, flags, bg,
                                rowcol->bounds[0], rowcol->bounds[1],
                                rowcol->bounds[2], rowcol->bounds[3],
                                0 );
        }
    }

    n = 0;
    for( i = 0; i < rowcol->rows; i++ ) {
        for( j = 0; j < rowcol->cols; j++ ) {
            /* If this cell doesn't exist, all done */
            if( n >= rowcol->numChildren ) break;
            child = rowcol->children[n];

            child->draw( child );

            n++;
        }
    }
}

/*** EOF hwRowCol.c ***/
