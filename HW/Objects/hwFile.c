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

/* Internal definitions of file-related flags */
#define HW_FILENAME     1
#define HW_BBOX         2
#define HW_CHILDREN     3

static hwObject create( hwObject );
static void addref( hwObject );
static void destroy( hwObject );
static void modify( hwObject, const char *, hwInt32 type, const void * );
static hwInt32 inquire( hwObject, const char *, void **val );
static void draw( hwObject );

static const char
    *propList[] = {
        /* Specific to file */
        hwStrFileName,

        /* End of list */
        0
    };
static struct _hwObjectStruct
    hwFileDef = {
        0,              /* Parent - NULL */
        "hwFile",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwFile = &hwFileDef;

typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount,
        numObjs,
        bboxValid,
        dirty;
    hwFloat
        BBox[6];
    hwObject
        *objects;
    char
        *filename;
} File;

/* The hash table for file strings */
static void *fileTab = 0;

static hwObject create( hwObject proto )
{
    File
        *result;
    void
        *t;

    LOG_ENTRY
    if( proto != hwFile )       return 0;

    HW_HASH_SETUP(fileTab, 16)
        t = fileTab;
        HW_INSERT( hwStrFileName, HW_FILENAME, t );
        HW_INSERT( hwStrBounds,   HW_BBOX,     t );
        HW_INSERT( hwStrChildren, HW_CHILDREN, t );
    HW_HASH_CLEANUP

    result = malloc( sizeof(File) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }
    result->hdr = *proto;
    result->hdr.parent = hwFile;
    result->refCount = 1;

    result->filename = 0;
    result->numObjs = 0;
    result->objects = 0;
    result->bboxValid = 0;
    result->dirty = 1;

    result->hdr.name = 0;
    LOG_EXIT
    return (hwObject)result;
}

static void addref( hwObject obj )
{
    File
        *file = (File *)obj;

    LOG_ENTRY
    file->refCount++;
    LOG_EXIT
}

static void destroy( hwObject obj )
{
    File
        *file = (File *)obj;
    hwInt32
        n;
    hwObject
        *objs;

    LOG_ENTRY
    if( --file->refCount > 0 )  return;
    if( file->filename )        free( file->filename );
    objs = file->objects;
    if( objs ) {
        n = file->numObjs;
        while( n-- > 0 ) {
            objs[n]->destroy( objs[n] );
        }
        free( objs );
    }
    free( file );
    LOG_EXIT
}

static void modify
(
    hwObject obj, const char *prop,
    hwInt32 type, const void *val
)
{
    File
        *file = (File *)obj;
    hwObject
        *objs;
    hwInt32
        i, n;

    LOG_ENTRY
    switch( hwLookup( prop, fileTab ) ) {
    case HW_FILENAME :
        if( type != HW_TYPE_STRING )    goto BadType;
        if( file->filename ) {
            free( file->filename );
            file->filename = 0;
        }
        file->filename = malloc( strlen( (char *)val ) + 1 );
        if( !file->filename ) {
            __hwIntSetError( HW_ERROR_NO_MEMORY );
            return;
        }
        (void)strcpy( file->filename, val );
        objs = file->objects;
        if( objs ) {
            n = file->numObjs;
            while( n-- > 0 ) {
                objs[n]->destroy( objs[n] );
            }
            free( objs );
        }
        file->numObjs = hwParseFile( file->filename, &file->objects );
        file->bboxValid = 0;
        file->dirty = 1;
        break;
    default :
        objs = file->objects;
        if( objs ) {
            n = file->numObjs;
            for( i = 0; i < n; i++ ) {
                objs[i]->modify( objs[i], prop, type, val );
            }
        }
        break;
    }
    LOG_EXIT
    return;

BadType:
    __hwIntSetError( HW_ERROR_BAD_TYPE );
}

static void cook( File *file )
{
    hwFloat
        bbox[6],
        *ptr;
    hwInt32
        i, n;
    hwObject
        *objs;

    LOG_ENTRY
    n = file->numObjs;
    objs = file->objects;
    if( !objs ) return;

    bbox[0] = bbox[1] = bbox[2] = HW_MAX_FLOAT;
    bbox[3] = bbox[4] = bbox[5] = -HW_MAX_FLOAT;
    file->bboxValid = 1;
    for( i = 0; i < n; i++ ) {
        if( objs[i]->inquire( objs[i], hwStrBounds, (void **)&ptr )
                == HW_MAKE_TYPE(HW_TYPE_FLOAT,6) )
        {
            if( ptr[0] < bbox[0] ) bbox[0] = ptr[0];
            if( ptr[1] < bbox[1] ) bbox[1] = ptr[1];
            if( ptr[2] < bbox[2] ) bbox[2] = ptr[2];
            if( ptr[3] > bbox[3] ) bbox[3] = ptr[3];
            if( ptr[4] > bbox[4] ) bbox[4] = ptr[4];
            if( ptr[5] > bbox[5] ) bbox[5] = ptr[5];
        }
        else {
            file->bboxValid = 0;
        }
    }
    file->dirty = 0;
    LOG_EXIT
}

static hwInt32 inquire( hwObject obj, const char *prop, void **val )
{
    File
        *file = (File *)obj;

    LOG_ENTRY
    switch( hwLookup( prop, fileTab ) ) {
    case HW_FILENAME :
        if( !file->filename )   return 0;
        *val = file->filename;
        return HW_TYPE_STRING;
    case HW_BBOX :
        if( file->dirty ) cook( file );
        if( !file->bboxValid )  return 0;
        *val = file->BBox;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,6);
    case HW_CHILDREN :
        if( !file->objects )    return 0;
        *val = file->objects;
        return HW_MAKE_TYPE(HW_TYPE_OBJECT,file->numObjs);
    }
    __hwIntSetError( HW_ERROR_BAD_PROP );
    LOG_EXIT
    return 0;
}

static void draw( hwObject obj )
{
    hwInt32
        i, n;
    hwObject
        *objs;
    File
        *file = (File *)obj;
    struct __hwDisplayInternal
        *intDisp;
    HW_USE_CURR_DISP;

    LOG_ENTRY

    intDisp = (struct __hwDisplayInternal *)__hwDisp;

    objs = file->objects;
    n = file->numObjs;
    if( !objs ) return;

    if( file->dirty ) cook( file );
    if( file->bboxValid && (n > 1) ) {
        intDisp->scratchSurf.flags = 0;
        intDisp->scratchSurf.visibility = 0xFFFFFFFF;
        if( !__hwDisp->boundsVisible( __hwDisp,
                                &intDisp->scratchSurf,
                                file->BBox, n*20 ) )
        {
            return;
        }
    }

    for( i = 0; i < n; i++ ) {
        __hwDisp->currentChild( __hwDisp, objs[i] );
        objs[i]->draw( objs[i] );
    }
    LOG_EXIT
}

/*** EOF hwFile.c ***/
