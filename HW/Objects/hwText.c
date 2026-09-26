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

#include <stdlib.h>
#include <string.h>
#include "hw.h"
#include "hw_internal.h"

/* Internal definitions of text-related flags */
#define HW_DATA         1
#define HW_SURFACE      2
#define HW_POS          3
#define HW_UP           4
#define HW_DIR          5
#define HW_SCALE        6
#define HW_EXTRUDE      7
#define HW_ALIGN        8
#define HW_OPT_FLAGS    9
#define HW_VISIBILITY   10
#define HW_DIRTY        11

static hwObject create( hwObject );
static void addref( hwObject );
static void destroy( hwObject );
static void modify( hwObject, const char *, hwInt32 type, const void * );
static hwInt32 inquire( hwObject, const char *, void ** );
static void draw( hwObject );

static const char
    *propList[] = {
        /* Specific to text */
        hwStrData, hwStrSurface,
        hwStrPos, hwStrUp, hwStrDir,
        hwStrScale, hwStrExtrude, hwStrAlign,
        hwStrOptFlags,

        /* End of list */
        0
    };
static struct _hwObjectStruct
    hwTextStruct = {
        0,              /* Parent - NULL */
        "hwText",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwText = &hwTextStruct;
    
typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount,
        optFlags,               /* From HW_OPT_* */
        visibility,             /* Visibility Bits */
        dirty,                  /* If we need to create DL */
        dl,                     /* The display list for optimization */
        align[3];               /* Alignment parameters */
    hwObject
        surfObjs[3];            /* The list of surface objects */
    hwSurfaceType
        surfaces[3];            /* The list of surfaces */
    char
        *textStr;               /* The string to draw */
    hwFloat
        pos[3], dir[3], up[3],  /* Position/orientation parameters */
        scale,                  /* Size parameter */
        extrude[3];             /* Extrusion parameter */
} Text;

/* The hash table for text strings */
static void *textTab = 0;

static hwObject create( hwObject proto )
{
    Text
        *result;

    LOG_ENTRY
    if( proto != hwText )       return 0;

    HW_HASH_SETUP(textTab, 16)
        HW_INSERT( hwStrData,       HW_DATA,       textTab );
        HW_INSERT( hwStrSurface,    HW_SURFACE,    textTab );
        HW_INSERT( hwStrPos,        HW_POS,        textTab );
        HW_INSERT( hwStrUp,         HW_UP,         textTab );
        HW_INSERT( hwStrDir,        HW_DIR,        textTab );
        HW_INSERT( hwStrScale,      HW_SCALE,      textTab );
        HW_INSERT( hwStrExtrude,    HW_EXTRUDE,    textTab );
        HW_INSERT( hwStrAlign,      HW_ALIGN,      textTab );
        HW_INSERT( hwStrOptFlags,   HW_OPT_FLAGS,  textTab );
        HW_INSERT( hwStrVisibility, HW_VISIBILITY, textTab );
        HW_INSERT( hwStrDirty,      HW_DIRTY,      textTab );
    HW_HASH_CLEANUP

    result = malloc( sizeof(Text) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }
    result->hdr = *proto;
    result->hdr.parent = hwText;
    result->refCount = 1;

    /* Identity text */
    result->align[0] = result->align[1]
                = result->align[2] = HW_TEXT_ALIGN_CENTER;
    result->surfObjs[0] = result->surfObjs[1] = result->surfObjs[2] = 0;
    hwDefaultSurf( &result->surfaces[0] );
    hwDefaultSurf( &result->surfaces[1] );
    hwDefaultSurf( &result->surfaces[2] );
    result->textStr = 0;
    result->pos[0] = result->pos[1] = result->pos[2] = 0.0;
    result->dir[0] = 1.0; result->dir[1] = 0.0; result->dir[2] = 0.0;
    result->up[0] = 0.0; result->up[1] = 1.0; result->up[2] = 0.0;
    result->scale = 1.0;
    result->extrude[0] = .0; result->extrude[1] = 0.; result->extrude[2] = .5;
    result->dirty = 1;
    result->dl = -1;
    result->optFlags = hwDefaultOptFlags;
    result->visibility = 0xffffffff;

    result->hdr.name = 0;
    LOG_EXIT
    return (hwObject)result;
}

static void addref( hwObject obj )
{
    Text
        *text = (Text *)obj;

    LOG_ENTRY
    text->refCount++;
    LOG_EXIT
}

static void destroy( hwObject obj )
{
    Text
        *text = (Text *)obj;
    int
        i;

    LOG_ENTRY
    if( --text->refCount > 0 )  return;
    for( i = 0; i < 3; i++ ) {
        if( text->surfObjs[i] ) text->surfObjs[i]->destroy( text->surfObjs[i] );
    }

    if( text->dl >= 0 ) {
        __hwIntDestroyList( text->dl);
    }

    if( text->textStr ) free( text->textStr );
    free( text );
    LOG_EXIT
}

static void modify
(
    hwObject obj, const char *prop,
    hwInt32 type, const void *val
)
{
    Text
        *text = (Text *)obj;
    int
        i, n;
    char
        *s;
    hwObject
        tmp;

    LOG_ENTRY
    switch( hwLookup( prop, textTab ) ) {
    case HW_OPT_FLAGS :
        if( type != HW_TYPE_1I )        goto BadType;
        text->optFlags = *(hwInt32 *)val;
        break;
    case HW_VISIBILITY :
        if( type != HW_TYPE_1I )        goto BadType;
        text->visibility = *(hwInt32 *)val;
        break;
    case HW_DATA :
        if( type != HW_TYPE_STRING )    goto BadType;
        s = (char *)val;
        text->textStr = realloc( text->textStr, strlen(s) + 1 );
        if( !text->textStr ) {
            __hwIntSetError( HW_ERROR_NO_MEMORY );
            return;
        }
        (void)strcpy( text->textStr, s );
        break;
    case HW_SURFACE :
        switch( type ) {
        case HW_TYPE_OBJECT :
            for( i = 0; i < 3; i++ ) {
                if( text->surfObjs[i] ) {
                    text->surfObjs[i]->destroy( text->surfObjs[i] );
                }
                text->surfObjs[i] = 0;
            }
            tmp = (hwObject)val;
            tmp->addref( tmp );
            text->surfObjs[0] = tmp;
            break;
        case HW_MAKE_TYPE(HW_TYPE_OBJECT,3 ) :
            for( i = 0; i < 3; i++ ) {
                if( text->surfObjs[i] ) {
                    text->surfObjs[i]->destroy( text->surfObjs[i] );
                }
            }
            (void)memcpy( text->surfObjs, val, 3*sizeof(hwObject) );
            for( i = 0; i < 3; i++ ) {
                tmp = text->surfObjs[i];
                tmp->addref( tmp );
            }
            break;
        default :
            goto BadType;
        }
        break;
    case HW_POS :
        if( type != HW_TYPE_3F )        goto BadType;
        (void)memcpy( text->pos, val, 3*sizeof(hwFloat) );
        break;
    case HW_UP :
        if( type != HW_TYPE_3F )        goto BadType;
        (void)memcpy( text->up, val, 3*sizeof(hwFloat) );
        break;
    case HW_DIR :
        if( type != HW_TYPE_3F )        goto BadType;
        (void)memcpy( text->dir, val, 3*sizeof(hwFloat) );
        break;
    case HW_SCALE :
        switch( type ) {
        case HW_TYPE_1I :
            text->scale = *(hwInt32 *)val;
            break;
        case HW_TYPE_1F :
            text->scale = *(hwFloat *)val;
            break;
        default :
            goto BadType;
        }
        break;
    case HW_EXTRUDE :
        if( type != HW_TYPE_3F )        goto BadType;
        (void)memcpy( text->extrude, val, 3*sizeof(hwFloat) );
        break;
    case HW_ALIGN :
        if( type == HW_MAKE_TYPE(HW_TYPE_FLOAT,3) )
        {
            float tmp[3];
            (void)memcpy( tmp, val, 3*sizeof(hwFloat) );
            text->align[0] = (int)tmp[0]; 
            text->align[1] = (int)tmp[1]; 
            text->align[2] = (int)tmp[2]; 
        }
        else if ( type != HW_MAKE_TYPE(HW_TYPE_INT,3) ) 
            goto BadType;
        else
            (void)memcpy( text->align, val, 3*sizeof(hwInt32) );

        break;
    case HW_DIRTY :
        for( i = 0; i < 3; i++ ) {
            if( text->surfObjs[i] ) {
                HW_MODIFY_1I( text->surfObjs[i], hwStrDirty, 1 );
            }
        }
        break;
    default :
        n = hwLookup( prop, __hwIntSurfTab );
        if( n ) {
            if( !text->surfObjs[0] ) {
                text->surfObjs[0] = hwSurface->create( hwSurface );
                if( !text->surfObjs[0] )        return;
            }

            tmp = text->surfObjs[0];
            tmp->modify( tmp, prop, type, val );

            if( text->surfObjs[1] ) {
                tmp = text->surfObjs[1];
                tmp->modify( tmp, prop, type, val );
            }
            if( text->surfObjs[2] ) {
                tmp = text->surfObjs[2];
                tmp->modify( tmp, prop, type, val );
            }
        }
        else {
            __hwIntSetError( HW_ERROR_BAD_PROP );
        }
        break;
    }
    text->dirty = 1;
    LOG_EXIT
    return;

BadType :
    __hwIntSetError( HW_ERROR_BAD_TYPE );
}

static hwInt32 inquire( hwObject obj, const char *prop, void **val )
{
    Text
        *text = (Text *)obj;

    LOG_ENTRY
    switch( hwLookup( prop, textTab ) ) {
    case HW_OPT_FLAGS :
        *val = &text->optFlags;
        return HW_TYPE_1I;
    case HW_DATA :
        *val = text->textStr;
        return HW_TYPE_STRING;
    case HW_SURFACE :
        if( text->surfObjs[1] ) {
            *val = text->surfObjs;
            return HW_MAKE_TYPE(HW_TYPE_OBJECT,3);
        }
        else if( text->surfObjs[0] ) {
            *val = text->surfObjs[0];
            return HW_TYPE_OBJECT;
        }
        else {
            return 0;
        }
    case HW_POS :
        *val = text->pos;
        return HW_TYPE_3F;
    case HW_UP :
        *val = text->up;
        return HW_TYPE_3F;
    case HW_DIR :
        *val = text->dir;
        return HW_TYPE_3F;
    case HW_SCALE :
        *val = &text->scale;
        return HW_TYPE_1F;
    case HW_EXTRUDE :
        *val = text->extrude;
        return HW_TYPE_3F;
    case HW_ALIGN :
        *val = text->align;
        return HW_MAKE_TYPE(HW_TYPE_INT,3);
    default :
        __hwIntSetError( HW_ERROR_BAD_PROP );
        break;
    }
    __hwIntSetError( HW_ERROR_BAD_PROP );
    LOG_EXIT
    return 0;
}

static void draw( hwObject obj )
{
    Text
        *text = (Text *)obj;
    int
        i, j, n, numSurf;
    hwObject
        tmp;
    void
        *v;
    HW_USE_CURR_DISP;

    LOG_ENTRY
    if( text->dirty ) {
        if( text->dl >= 0 ) {
            __hwDisp->destroyList( __hwDisp, text->dl );
            text->dl = -1;
        }
        text->dirty = 0;
        if( text->surfObjs[1] ) {
            /* Three separate surfaces */
            numSurf = 3;
        }
        else {
            /* Only 1 surface */
            numSurf = 1;
            text->surfaces[1] = text->surfaces[0];
            text->surfaces[2] = text->surfaces[0];
        }
        for( i = 0; i < numSurf; i++ ) {
            tmp = text->surfObjs[i];
            if( tmp->inquire( tmp, hwStrSurface, &v ) ) {
                text->surfaces[i] = *(hwSurfaceType *)v;
                n = text->surfaces[i].numTextures;
                for( j = 0; j < n; j++ ) {
                    text->surfaces[i].textures[j]->addref(
                                    text->surfaces[i].textures[j] );
                }
            }
        }
    }

    /* Text does not call the boundsVisible routine that most primitives
    ** do --- I'm not entirely sure why.  However, we still need to check
    ** to see if the animation bits tell us not to draw, so check it here.
    */

    {
        hwInt32 mask;

        mask = __hwDisp->getVisibility( __hwDisp );
        if( (text->visibility & mask ) != mask ) return;
    }

    if( text->dl >= 0 ) {
        __hwDisp->callList( __hwDisp, text->dl );
    }
    else {
        __hwIntTextAlign( __hwDisp, text->align[0],
                                text->align[1], text->align[2] );
        __hwIntTextExtrude( __hwDisp, text->extrude[0], text->extrude[1],
                                text->extrude[2] );
        __hwIntTextHeight( __hwDisp, text->scale );
        __hwIntTextOrient( __hwDisp, text->up[0], text->up[1], text->up[2],
                        text->dir[0], text->dir[1], text->dir[2] );
        __hwIntTextAttrs(__hwDisp, text->surfaces+0, text->surfaces+1,
                        text->surfaces+2);
        if( text->optFlags & (HW_OPT_USE_DL|HW_OPT_DL_ATTRS) ) {
            text->dl = __hwDisp->openList( __hwDisp );
            if( text->dl < 0 ) {
                text->optFlags &= ~(HW_OPT_USE_DL|HW_OPT_DL_ATTRS);
            }
        }
        __hwIntText3d( __hwDisp, text->pos[0], text->pos[1], text->pos[2],
                                text->textStr );
        if( text->optFlags & (HW_OPT_USE_DL|HW_OPT_DL_ATTRS) ) {
            __hwDisp->closeList( __hwDisp );
            __hwDisp->callList( __hwDisp, text->dl );
        }
    }
    LOG_EXIT
}

/*** EOF hwText.c ***/
