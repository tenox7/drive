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

#include <math.h>
#include <ctype.h>
#include <string.h>
#include "hwedit.h"

int cmdOpen( int nargs, char **args )
{
    ObjectList
        curr, tail;
    hwObject
        *objects;
    int
        i, nobjs;

    /* Start with a fresh project */
    (void)cmdNew( 0, 0 );

    /* Parse the new file */
    nobjs = hwParseFile( args[0], &objects );
    if( !nobjs )        return 0;

    edit.fname = malloc( strlen( args[0] ) + 1 );
    if( !edit.fname )   exit( 1 );
    (void)strcpy( edit.fname, args[0] );

    /* Create the new object list */
    tail = 0;
    for( i = 0; i < nobjs; i++ ) {
        curr = malloc( sizeof(struct _ol) );
        if( !curr ) exit( 1 );
        curr->obj = objects[i];
        curr->flags = 0;
        curr->prev = tail;
        curr->next = 0;
        if( tail ) tail->next = curr;
        else       edit.olist = curr;
        tail = curr;
    }
    free( objects );
    return 1;
}

int cmdRevert( int nargs, char **args )
{
    char
        *s;
    int
        result;

    if( !edit.fname ) return 0;
    s = malloc( strlen( edit.fname ) + 1 );
    if( !s ) exit( 0 );
    args[0] = s;
    result = cmdOpen( 1, args );
    free( s );
    return result;
}

int cmdNew( int nargs, char **args )
{
    ObjectList
        curr, next;

    while( edit.stack ) {
        (void)cmdPopSubset( 0, 0 );
    }
    for( curr = edit.olist; curr; curr = next ) {
        next = curr->next;
        curr->obj->destroy( curr->obj );
        free( curr );
    }
    edit.olist = 0;
    edit.needRedraw = 1;
    if( edit.fname ) free( edit.fname );
    edit.fname = 0;
    hwUnregisterObjects();

    return 1;
}

int cmdSave( int nargs, char **args )
{
    ObjectList
        curr;
    EditStack
        stk;
    FILE
        *f;

    /* Find the root of the stack; TBD: Apply group edit changes now? */
    curr = edit.olist;
    for( stk = edit.stack; stk; stk = stk->next ) {
        curr = stk->list;
    }

    f = fopen( edit.fname, "w" );
    if( !f ) return 0;

    while( curr ) {
        (void)hwWriteAscii( curr->obj, f );
        curr = curr->next;
    }
    (void)fclose( f );

    return 1;
}

int cmdSaveAs( int nargs, char **args )
{
    char
        *s;
    int
        result;

    s = edit.fname;
    edit.fname = malloc( strlen( args[0] ) + 1 );
    if( !edit.fname ) exit( 1 );
    (void)strcpy( edit.fname, args[0] );
    result = cmdSave( 0, 0 );
    if( result ) {
        free( s );
    }
    else {
        free( edit.fname );
        edit.fname = s;
    }
    return result;
}

int cmdQuit( int nargs, char **args )
{
    int
        i;
    CmdStream
        str;

    /* TBD: Yes/no query if project modified */

    for (i = 0; i < NUM_SOCKS; i++) {
        if (edit.socks[i] >= 0) {
            str = edit.cmd[i];
            str->putString(str, COMMAND_EXIT);
        }
    }
    exit( 0 );

    /*NOTREACHED*/
    return 1;
}

int cmdDuplicate( int nargs, char **args )
{
    ObjectList
        curr, next,
        head;
    hwObject
        currObj,
        newObj;
    hwInt32
        type;
    void
        *val;
    const char
        **props;

    if( edit.stack && edit.stack->scalar ) return 0;

    for( curr = edit.olist; curr; curr = next ) {
        next = curr->next;
        if( curr->flags & OF_SELECTED ) {
            /* TBD: Undo */
            curr->flags &= ~OF_SELECTED;
            currObj = curr->obj;
            newObj = currObj->parent->create( currObj->parent );
            if( !newObj ) exit( 1 );
            for( props = currObj->props; *props; props++ ) {
                type = currObj->inquire( currObj, *props, &val );
                if( type && !(type & HW_TYPE_CLEAN) ) {
                    newObj->modify( newObj, *props, type, val );
                }
            }
            head = malloc( sizeof(*head) );
            if( !head ) exit( 1 );
            head->obj = newObj;
            head->flags = OF_SELECTED;
            head->prev = 0;
            head->next = edit.olist;
            edit.olist->prev = head;
            edit.olist = head;
        }
    }
    edit.needRedraw = 1;
    return 1;
}

int cmdDelete( int nargs, char **args )
{
    ObjectList
        curr, next;

    if( edit.stack && edit.stack->scalar ) return 0;

    for( curr = edit.olist; curr; curr = next ) {
        next = curr->next;
        if( curr->flags & OF_SELECTED ) {
            /* TBD: Undo */
            curr->obj->destroy( curr->obj );
            if( curr->prev )    curr->prev->next = next;
            else                edit.olist = next;
            if( curr->next )    curr->next->prev = curr->prev;
            free( curr );
        }
    }
    edit.needRedraw = 1;
    return 1;
}

int cmdUndo( int nargs, char **args )
{
    /* TBD */
    return 0;
}

int cmdRedo( int nargs, char **args )
{
    /* TBD */
    return 0;
}

int cmdSelectAll( int nargs, char **args )
{
    ObjectList
        curr;

    for( curr = edit.olist; curr; curr = curr->next ) {
        curr->flags |= OF_SELECTED;
    }
    edit.needRedraw = 1;
    return 1;
}

int cmdUnselectAll( int nargs, char **args )
{
    ObjectList
        curr;

    for( curr = edit.olist; curr; curr = curr->next ) {
        curr->flags &= ~OF_SELECTED;
    }
    edit.needRedraw = 1;
    return 1;
}

int cmdSelectXY( int nargs, char **args )
{
    /* TBD */
    return 0;
}

int cmdSelectMouse( int nargs, char **args )
{
    /* TBD */
    return 0;
}

int cmdSelectObject( int nargs, char **args )
{
    hwObject
        obj;
    ObjectList
        ptr;

    obj = stringToObject( args[0] );
    if( !obj ) return 0;
    ptr = findObject( obj );
    if( !ptr ) return 0;
    ptr->flags |= OF_SELECTED;
    edit.needRedraw = 1;
    return 1;
}

int cmdUnselectObject( int nargs, char **args )
{
    hwObject
        obj;
    ObjectList
        ptr;

    obj = stringToObject( args[0] );
    if( !obj ) return 0;
    ptr = findObject( obj );
    if( !ptr ) return 0;
    ptr->flags &= ~OF_SELECTED;
    edit.needRedraw = 1;
    return 1;
}

int cmdListSelected( int nargs, char **args )
{
    ObjectList
        curr;
    char
        *name;

    for( curr = edit.olist; curr; curr = curr->next ) {
        if( curr->flags & OF_SELECTED ) {
            name = objectToString( curr->obj );
            edit.activeCmd->putString( edit.activeCmd, name );
            edit.activeCmd->putString( edit.activeCmd, "\n" );
        }
    }
    return 1;
}

int cmdListObjects( int nargs, char **args )
{
    ObjectList
        curr;
    char
        *name;

    for( curr = edit.olist; curr; curr = curr->next ) {
        name = objectToString( curr->obj );
        edit.activeCmd->putString( edit.activeCmd, name );
        edit.activeCmd->putString( edit.activeCmd, "\n" );
    }
    return 1;
}

int cmdPushSubset( int nargs, char **args )
{
    EditStack
        stk;
    ObjectList
        curr, tail;
    hwObject
        obj;
    hwInt32
        type;
    int
        i, n;
    hwObject
        *list;
    void
        *val;

    obj = stringToObject( args[0] );
    if( !obj ) return 0;

    type = obj->inquire( obj, args[1], &val );
    if( !type ) return 0;
    if( HW_GET_BASE(type) != HW_TYPE_OBJECT ) return 0;

    n = HW_GET_COUNT(type);

    /* Push the old object list */
    stk = malloc( sizeof(*stk) + strlen(args[1]) + 1 );
    if( !stk ) exit( 1 );
    stk->list = edit.olist;
    stk->next = edit.stack;
    stk->scalar = (n == 0);
    stk->obj = obj;
    (void)strcpy( stk->prop, args[1] );
    edit.stack = stk;

    /* Create a new object list */
    if( n == 0 ) {
        obj = val;
        curr = malloc( sizeof(*curr) );
        curr->obj = obj;
        curr->flags = OF_SELECTED;
        curr->prev = curr->next = 0;
        edit.olist = curr;
    }
    else {
        list = val;
        tail = 0;
        for( i = 0; i < n; i++ ) {
            curr = malloc( sizeof(*curr) );
            if( !curr ) exit( 1 );
            curr->obj = list[i];
            curr->flags = 0;
            curr->prev = tail;
            curr->next = 0;
            if( tail ) tail->next = curr;
            else edit.olist = curr;
            tail = curr;
        }
    }

    edit.needRedraw = 1;

    return 1;
}

int cmdPopSubset( int nargs, char **args )
{
    ObjectList
        curr, next;
    EditStack
        last;
    int
        i, n;
    hwObject
        obj, *list;
    void
        *val;
    hwInt32
        type;

    if( !edit.stack ) return 0;

    if( edit.stack->scalar ) {
        list = 0;
        obj = edit.olist->obj;
        free( edit.olist );
        obj->addref( obj );
        type = HW_TYPE_OBJECT;
        val = obj;
    }
    else {
        for( n = 0, curr = edit.olist; curr; curr = next ) {
            next = curr->next;
            n++;
        }
        list = malloc( n * sizeof(hwObject) );
        if( !list ) exit( 1 );

        for( n = 0, curr = edit.olist; curr; curr = next ) {
            next = curr->next;
            obj = curr->obj;
            obj->addref( obj ); /* Just to be safe */
            list[n++] = obj;
            free( curr );
        }
        type = HW_MAKE_TYPE(HW_TYPE_OBJECT,n);
        val = list;
    }

    last = edit.stack;
    last->obj->modify( last->obj, last->prop, type, val );
    edit.olist = last->list;
    edit.stack = last->next;
    free( last );
    edit.needRedraw = 1;

    if( list ) {
        for( i = 0; i < n; i++ ) {
            obj = list[i];
            obj->destroy( obj );
        }
        free( list );
    }
    else {
        obj->destroy( obj );
    }
    return 1;
}

int cmdGetClass( int nargs, char **args )
{
    hwObject
        obj;

    obj = stringToObject( args[0] );
    if( !obj ) return 0;
    if( !obj->parent ) return 0;
    if( !obj->parent->name ) return 0;
    edit.activeCmd->putString( edit.activeCmd, (char *)obj->parent->name );
    edit.activeCmd->putString( edit.activeCmd, "\n" );
    return 1;
}

int cmdGetProp( int nargs, char **args )
{
    hwObject
        obj;
    hwInt32
        type;
    void
        *val;
    ObjectList
        curr;

    obj = 0;
    for( curr = edit.olist; curr; curr = curr->next ) {
        if( curr->flags & OF_SELECTED ) {
            obj = curr->obj;
            break;
        }
    }
    if( !obj ) return 0;

    type = obj->inquire( obj, args[0], &val );
    if( !type ) return 0;

    printProp( type, val );
    return 1;
}

int cmdSetProp( int nargs, char **args )
{
    hwObject
        obj;
    hwInt32
        type;
    void
        *val;
    int
        needFree;
    char
        *ptr;
    ObjectList
        curr;

    ptr = args[1];
    type = getValue( &ptr, &val, &needFree );
    if( !type ) return 0;

    for( curr = edit.olist; curr; curr = curr->next ) {
        if( !(curr->flags & OF_SELECTED) )      continue;
        obj = curr->obj;
        obj->modify( obj, args[0], type, val );
    }

    edit.needRedraw = 1;
    if( needFree ) {
        free( val );
    }
    return 1;
}

int cmdGetObjProp( int nargs, char **args )
{
    hwObject
        obj;
    hwInt32
        type;
    void
        *val;

    obj = stringToObject( args[0] );
    if( !obj ) return 0;

    type = obj->inquire( obj, args[1], &val );
    if( !type ) return 0;

    printProp( type, val );
    return 1;
}

int cmdSetObjProp( int nargs, char **args )
{
    hwObject
        obj;
    hwInt32
        type;
    void
        *val;
    int
        needFree;
    char
        *ptr;

    obj = stringToObject( args[0] );
    if( !obj ) return 0;

    ptr = args[2];
    type = getValue( &ptr, &val, &needFree );
    if( !type ) return 0;

    obj->modify( obj, args[1], type, val );
    edit.needRedraw = 1;
    if( needFree ) {
        free( val );
    }
    return 1;
}

int cmdListProps( int nargs, char **args )
{
    hwObject
        obj;
    char
        **ptr;

    obj = stringToObject( args[0] );
    if( !obj ) return 0;

    for( ptr = (char **)obj->props; ptr && *ptr; ptr++ ) {
        edit.activeCmd->putString( edit.activeCmd, *ptr );
        edit.activeCmd->putString( edit.activeCmd, "\n" );
    }
    return 1;
}

int cmdNewObject( int nargs, char **args )
{
    hwObject
        obj, cls;
    ObjectList
        curr;

    if( edit.stack && edit.stack->scalar ) return 0;

    cls = hwFindClass( args[0] );
    if( !cls ) return 0;
    obj = cls->create( cls );
    if( !obj ) return 0;
    obj->name = 0;

    for( curr = edit.olist; curr; curr = curr->next ) {
        curr->flags &= ~OF_SELECTED;
    }

    curr = malloc( sizeof(*curr) );
    if( !curr ) exit( 1 );
    curr->obj = obj;
    curr->flags = OF_SELECTED;
    curr->prev = 0;
    curr->next = edit.olist;
    if( edit.olist ) edit.olist->prev = curr;
    edit.olist = curr;
    edit.needRedraw = 1;

    return 1;
}

int cmdNewNamedObject( int nargs, char **args )
{
    hwObject
        obj, cls;
    ObjectList
        curr;

    if( edit.stack && edit.stack->scalar ) return 0;

    cls = hwFindClass( args[0] );
    if( !cls ) return 0;
    obj = cls->create( cls );
    if( !obj ) return 0;
    obj->name = malloc( strlen( args[1] ) + 1 );
    if( !obj->name ) exit( 1 );
    (void)strcpy( (char *)obj->name, args[1] );
    hwRegisterObject( obj );

    for( curr = edit.olist; curr; curr = curr->next ) {
        curr->flags &= ~OF_SELECTED;
    }

    curr = malloc( sizeof(*curr) );
    if( !curr ) exit( 1 );
    curr->obj = obj;
    curr->flags = OF_SELECTED;
    curr->prev = 0;
    curr->next = edit.olist;
    if( edit.olist ) edit.olist->prev = curr;
    edit.olist = curr;
    edit.needRedraw = 1;

    return 1;
}

int cmdTranslate( int nargs, char **args )
{
    hwObject
        obj;
    ObjectList
        curr;
    hwFloat
        tx, ty, tz,
        newPos[3];
    hwInt32
        type;
    void
        *val;

    tx = atof( args[0] );
    ty = atof( args[1] );
    tz = atof( args[2] );
    for( curr = edit.olist; curr; curr = curr->next ) {
        if( !(curr->flags & OF_SELECTED) )      continue;
        obj = curr->obj;

        type = obj->inquire( obj, hwStrPos, &val );
        if( (type & ~HW_TYPE_CLEAN) != HW_TYPE_3F )     continue;

        newPos[0] = tx + ((hwFloat *)val)[0];
        newPos[1] = ty + ((hwFloat *)val)[1];
        newPos[2] = tz + ((hwFloat *)val)[2];
        obj->modify( obj, hwStrPos, HW_TYPE_3F, newPos );
    }
    edit.needRedraw = 1;
    return 1;
}

int cmdRotate( int nargs, char **args )
{
    /* TBD */
    return 0;
}

int cmdScale( int nargs, char **args )
{
    hwObject
        obj;
    ObjectList
        curr;
    hwFloat
        cx, cy, cz,
        sx, sy, sz,
        newPos[3],
        newScale[3];
    hwInt32
        type;
    void
        *val;

    cx = atof( args[0] );
    cy = atof( args[1] );
    cz = atof( args[2] );
    sx = atof( args[3] );
    sy = atof( args[4] );
    sz = atof( args[5] );
    for( curr = edit.olist; curr; curr = curr->next ) {
        if( !(curr->flags & OF_SELECTED) )      continue;
        obj = curr->obj;

        type = obj->inquire( obj, hwStrPos, &val );
        if( (type & ~HW_TYPE_CLEAN)  != HW_TYPE_3F )            continue;

        newPos[0] = cx + (((hwFloat *)val)[0] - cx) * sx;
        newPos[1] = cy + (((hwFloat *)val)[1] - cx) * sy;
        newPos[2] = cz + (((hwFloat *)val)[2] - cx) * sz;

        type = obj->inquire( obj, hwStrScale, &val );
        if( (type & ~HW_TYPE_CLEAN)  != HW_TYPE_3F )            continue;

        newScale[0] = sx * ((hwFloat *)val)[0];
        newScale[1] = sy * ((hwFloat *)val)[0];
        newScale[2] = sz * ((hwFloat *)val)[0];

        obj->modify( obj, hwStrPos, HW_TYPE_3F, newPos );
        obj->modify( obj, hwStrScale, HW_TYPE_3F, newScale );
    }
    edit.needRedraw = 1;
    return 1;
}

int cmdResetView( int nargs, char **args )
{
    resetCamera();
    return 1;
}

int cmdCamPos( int nargs, char **args )
{
    edit.camPos[0] = atof( args[0] );
    edit.camPos[1] = atof( args[1] );
    edit.camPos[2] = atof( args[2] );
    setCam();
    edit.needRedraw = 1;
    return 1;
}

int cmdCamField( int nargs, char **args )
{
    edit.camField = atof( args[0] );
    edit.cam->modify( edit.cam, hwStrField, HW_TYPE_1F, &edit.camField );
    edit.needRedraw = 1;
    return 1;
}

int cmdCamDir( int nargs, char **args )
{
    hwFloat
        x, y, z, d;

    x = atof( args[0] ); y = atof( args[1] ); z = atof( args[2] );
    d = sqrt( x*x + y*y + z*z );
    if( d > 0.f ) d = 1.f / d;
    edit.camDir[0] = x * d; edit.camDir[1] = y * d; edit.camDir[2] = z * d;
    rollCam( 0.0 );     /* Make sure other angles are updated */
    setCam();

    edit.needRedraw = 1;
    return 1;
}

int cmdCamUp( int nargs, char **args )
{
    hwFloat
        x, y, z, d;

    x = atof( args[0] ); y = atof( args[1] ); z = atof( args[2] );
    d = sqrt( x*x + y*y + z*z );
    if( d > 0.f ) d = 1.f / d;
    edit.camUp[0] = x * d; edit.camUp[1] = y * d; edit.camUp[2] = z * d;
    yawCam( 0.0 );
    setCam();

    edit.needRedraw = 1;
    return 1;
}

int cmdCamPlanes( int nargs, char **args )
{
    edit.camPlanes[0] = atof( args[0] );
    edit.camPlanes[1] = atof( args[1] );
    edit.cam->modify( edit.cam, hwStrPlanes, HW_TYPE_2F, edit.camPlanes );
    edit.needRedraw = 1;
    return 1;
}

int cmdCamDist( int nargs, char **args )
{
    edit.camDist = atof( args[0] );
    setCam();
    edit.needRedraw = 1;
    return 1;
}

int cmdVisibility( int nargs, char **args )
{
    (void)sscanf( args[0], "%x", &edit.visibility );
    edit.needRedraw = 1;
    return 1;
}

int cmdMouseView( int nargs, char **args )
{
    edit.mouseCommand = MOUSE_VIEW;
    return 1;
}

int cmdMouseSelect( int nargs, char **args )
{
    edit.mouseCommand = MOUSE_SELECT;
    return 1;
}

int cmdMouseMove( int nargs, char **args )
{
    edit.mouseCommand = MOUSE_MOVE;
    return 1;
}

int cmdWireframe( int nargs, char **args )
{
    if( tolower( args[0][0] ) == 't' ) {
        edit.renderMode = HW_RENDER_DEFAULT | HW_RENDER_EDGE_MODE;
        edit.renderMode &= ~HW_RENDER_CULL_FACE;
        edit.selectMode = HW_SELECT_EDGES;
    }
    else {
        edit.renderMode = HW_RENDER_DEFAULT;
        edit.selectMode = HW_SELECT_CULL_FACE;
    }
    edit.disp->renderMode( edit.disp, edit.renderMode );
    edit.needRedraw = 1;
    return 1;
}

int cmdAnimate( int nargs, char **args )
{
    edit.animating = 1;
    edit.needRedraw = 1;
    return 1;
}

int cmdHelp( int nargs, char **args )
{
    Command
        curr;

    for( curr = edit.cmdList; curr; curr = curr->next ) {
        edit.activeCmd->putString( edit.activeCmd, curr->name );
        if( curr->nargs != 0 ) {
            edit.activeCmd->putString( edit.activeCmd, " " );
            edit.activeCmd->putString( edit.activeCmd, curr->template );
        }
        edit.activeCmd->putString( edit.activeCmd, "\n" );
    }
    return 1;
}

int cmdButtonDown( int nargs, char **args )
{
    edit.eq[edit.nq].eventX = atoi( args[0] );
    edit.eq[edit.nq].eventY = atoi( args[1] );
    edit.eq[edit.nq].eventButton = atoi( args[2] );
    edit.eq[edit.nq].eventType = EVENT_CLICK;
    edit.eq[edit.nq].eventMods = 0;
    edit.nq++;
    printf("cmdButtonDown\n");
    return 1;
}

int cmdButtonUp( int nargs, char **args )
{
    edit.eq[edit.nq].eventX = atoi( args[0] );
    edit.eq[edit.nq].eventY = atoi( args[1] );
    edit.eq[edit.nq].eventButton = atoi( args[2] );
    edit.eq[edit.nq].eventType = EVENT_RELEASE;
    edit.eq[edit.nq].eventMods = 0;
    edit.nq++;
    printf("cmdButtonUp\n");
    return 1;
}

int cmdMotion( int nargs, char **args )
{
    int 
        state;

    if( edit.nq && (edit.eq[edit.nq-1].eventType == EVENT_MOVE) ) {
        /* Merge pending motion events */
        edit.nq--;
    }
    edit.eq[edit.nq].eventX = atoi( args[0] );
    edit.eq[edit.nq].eventY = atoi( args[1] );
    edit.eq[edit.nq].eventType = EVENT_MOVE;
    edit.eq[edit.nq].eventMods = 0;
    edit.eq[edit.nq].eventMods = atoi( args[2] );
    edit.nq++;
    printf("cmdMotion\n");
    return 1;
}
