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
#include <string.h>
#include "hw.h"
#include "hw_internal.h"
#include "TexFont.h"

/* Internal definitions of font-related flags */
#define HW_FILENAME     1
#define HW_FONT_HEIGHT  2
#define HW_FONT         3
#define HW_DIRTY        4

static hwObject create( hwObject );
static void addref( hwObject );
static void destroy( hwObject );
static void modify( hwObject, const char *, hwInt32 type, const void * );
static hwInt32 inquire( hwObject, const char *, void **val );
static void draw( hwObject );

static const char
    *propList[] = {
        /* Specific to font */
        hwStrFileName,
        hwStrFontHeight,

        /* End of list */
        0
    };
static struct _hwObjectStruct
    hwFontDef = {
        0,              /* Parent - NULL */
        "hwFont",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwFont = &hwFontDef;

typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount,
        dirty,
        fontHeight;             /* If static font */
    char
        *fontName;              /* If dynamic font */
    TexFont
        *font;
} HwFont;

/* The hash table for font strings */
static void *fontTab = 0;

static hwObject create( hwObject proto )
{
    HwFont
        *result;
    void
        *t;

    LOG_ENTRY
    if( proto != hwFont )       return 0;

    HW_HASH_SETUP(fontTab, 16)
        t = fontTab;
        HW_INSERT( hwStrFileName,   HW_FILENAME,    t );
        HW_INSERT( hwStrFontHeight, HW_FONT_HEIGHT, t );
        HW_INSERT( hwStrFont,       HW_FONT,        t );
        HW_INSERT( hwStrDirty,      HW_DIRTY,       t );
    HW_HASH_CLEANUP

    result = malloc( sizeof(HwFont) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }
    result->hdr = *proto;
    result->hdr.parent = hwFont;
    result->refCount = 1;

    result->fontName = 0;
    result->font = 0;
    result->fontHeight = 0;

    result->dirty = 1;

    result->hdr.name = 0;
    LOG_EXIT
    return (hwObject)result;
}

static void addref( hwObject obj )
{
    HwFont
        *font = (HwFont *)obj;

    LOG_ENTRY
    font->refCount++;
    LOG_EXIT
}

static void freeFont( HwFont *font )
{
    if( font->fontName ) {
        free( font->fontName );
        font->fontName = 0;

        if( font->font ) {
            txfUnloadFont( font->font );
            font->font = 0;
        }
    }
    font->fontHeight = 0;
}

static void destroy( hwObject obj )
{
    HwFont
        *font = (HwFont *)obj;

    LOG_ENTRY
    if( --font->refCount > 0 )  return;

    freeFont( font );

    LOG_EXIT
}

static void modify
(
    hwObject obj, const char *prop,
    hwInt32 type, const void *val
)
{
    HwFont
        *font = (HwFont *)obj;
    hwObject
        tmp, *objs;
    hwInt32
        i, n;

    LOG_ENTRY
    switch( hwLookup( prop, fontTab ) ) {
    case HW_FILENAME :
        freeFont( font );
        if( type != HW_TYPE_STRING )    goto BadType;
        font->fontName = malloc( strlen( (char *)val ) + 1 );
        if( !font->fontName ) {
            __hwIntSetError( HW_ERROR_NO_MEMORY );
            return;
        }
        (void)strcpy( font->fontName, val );
        font->dirty = 1;
        break;
    case HW_FONT_HEIGHT :
        freeFont( font );
        if( type == HW_TYPE_1I )        font->fontHeight = *(hwInt32 *)val;
        else if( type == HW_TYPE_1F )   font->fontHeight = *(hwFloat *)val;
        else                            goto BadType;
        font->dirty = 1;
        break;
    case HW_DIRTY :
        font->dirty = 1;
        if( font->font ) {
            tmp = font->font->texobj;
            if( tmp ) {
                HW_MODIFY_1I( tmp, hwStrDirty, 1 );
            }
        }
        break;
    default :
        __hwIntSetError( HW_ERROR_BAD_PROP );
        break;
    }
    LOG_EXIT
    return;

BadType:
    __hwIntSetError( HW_ERROR_BAD_TYPE );
}

static void cook( HwFont *font )
{
    FILE
        *f;

    LOG_ENTRY

    font->dirty = 0;

    if( font->fontName && font->font ) {
        txfUnloadFont( font->font );
    }

    font->font = 0;
    if( font->fontName ) {
        f = hwFopen( font->fontName, "rb" );
        if( f ) {
            font->font = txfLoadFont( (TxfReadFunc)hwFread, f );
            hwFclose( f );
        }
    }
    else if( font->fontHeight ) {
        font->font = txfLoadStaticFont( font->fontHeight );
    }

    LOG_EXIT
}

static hwInt32 inquire( hwObject obj, const char *prop, void **val )
{
    HwFont
        *font = (HwFont *)obj;

    LOG_ENTRY

    if( font->dirty ) cook( font );

    switch( hwLookup( prop, fontTab ) ) {
    case HW_FILENAME :
        *val = font->fontName;
        return HW_TYPE_STRING;
    case HW_FONT_HEIGHT :
        *val = &font->fontHeight;
        return HW_TYPE_1I;
    case HW_FONT :
        *val = font->font;
        return HW_TYPE_FONT;
    }

    __hwIntSetError( HW_ERROR_BAD_PROP );
    LOG_EXIT
    return 0;
}

static void draw( hwObject obj )
{
    LOG_ENTRY

    /* Nothing to draw */

    LOG_EXIT
}

/*** EOF hwFont.c ***/
