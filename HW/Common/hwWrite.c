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

#define HW_HDR_MAGIC            0xFF485701 /* 0xFF, 'HW', 01 (version) */
#define HW_HDR_MAGIC_SWAP       0x015748FF /* 0xFF, 'HW, 01, swapped */
#define HW_DEF_MAGIC            0xFF44  /* 0xFF, 'D' */
#define HW_DEF_MAGIC_SWAP       0x44FF  /* 'D', 0xFF */
#define HW_REF_MAGIC            0xFF52  /* 0xFF, 'R' */
#define HW_REF_MAGIC_SWAP       0x52FF  /* 'R', 0xFF */
#define HW_HSH_MAGIC            0xFF48  /* 0xFF, 'H' */
#define HW_HSH_MAGIC_SWAP       0x48FF  /* 'H', 0xFF */


static void indent( int level, FILE *f )
{
    while( level-- > 0 ) {
        (void)putc( ' ', f );
        (void)putc( ' ', f );
        (void)putc( ' ', f );
        (void)putc( ' ', f );
    }
}

hwInt32 __hwWriteAscii( hwObject obj, FILE *f, int level )
{
    int
        i, j, n;
    hwInt32
        type, ival;
    void
        *val;
    const char
        *s;
    hwFloat
        *fPtr;
    hwInt32
        *iPtr;
    char
        **sPtr;
    hwObject
        tmp, *oPtr;

    if( !obj->parent || !obj->parent->name )    return 0;

    level++;
    (void)fprintf( f, "%s %s {\n", obj->parent->name,
                        obj->name ? obj->name : "" );
    for( i = 0; obj->props[i]; i++ ) {
        s = obj->props[i];
        type = obj->inquire( obj, s, &val );
        if( !type )     continue;
        if( type & HW_TYPE_CLEAN )      continue;

        switch( type ) {
        case HW_TYPE_1I :
            indent( level, f );
            ival = *(hwInt32 *)val;
            if( ival < 0 ) {
                (void)fprintf( f, "%s = 0x%08x\n", s, (int)ival );
            }
            else {
                (void)fprintf( f, "%s = %d\n", s, (int)ival );
            }
            break;
        case HW_TYPE_1B :
            indent( level, f );
            (void)fprintf( f, "%s = %s\n", s,
                        *(hwInt32 *)val ? "True" : "False" );
            break;
        case HW_TYPE_1F :
            indent( level, f );
            (void)fprintf( f, "%s = %g\n", s, *(hwFloat *)val );
            break;
        case HW_TYPE_STRING :
            indent( level, f );
            (void)fprintf( f, "%s = \"%s\"\n", s, (char *) val );
            break;
        case HW_TYPE_OBJECT :
            indent( level, f );
            (void)fprintf( f, "%s = ", s );
            tmp = (hwObject)val;
            if( tmp->name )     (void)fprintf( f, "%s\n", tmp->name );
            else                (void)__hwWriteAscii( tmp, f, level );
            break;
        default :
            n = HW_GET_COUNT( type );
            indent( level, f );
            (void)fprintf( f, "%s = {", s );
            switch( HW_GET_BASE( type ) ) {
            case HW_TYPE_INT :
                iPtr = val;
                for( j = 0; j < n; j++ ) {
                    ival = iPtr[j];
                    (void)fprintf( f, (ival < 0) ? "0x%08x" : "%d", (int)ival );
                    if( j < (n-1) )     (void)putc( ',', f );
                    if( (j%8) == 7 ) {
                        (void)putc( '\n', f );
                        indent( level+1, f );
                    }
                }
                break;
            case HW_TYPE_BOOL :
                iPtr = val;
                for( j = 0; j < n; j++ ) {
                    (void)fprintf( f, "%s", iPtr[j] ? "True" : "False" );
                    if( j < (n-1) )     (void)putc( ',', f );
                    if( (j%8) == 7 ) {
                        (void)putc( '\n', f );
                        indent( level+1, f );
                    }
                }
                break;
            case HW_TYPE_FLOAT :
                fPtr = val;
                for( j = 0; j < n; j++ ) {
                    (void)fprintf( f, "%g", fPtr[j] );
                    if( j < (n-1) )     (void)putc( ',', f );
                    if( (j%8) == 7 ) {
                        (void)putc( '\n', f );
                        indent( level+1, f );
                    }
                }
                break;
            case HW_TYPE_STRING :
                sPtr = val;
                for( j = 0; j < n; j++ ) {
                    (void)fprintf( f, "\"%s\"", sPtr[j] );
                    if( j < (n-1) )     (void)putc( ',', f );
                    (void)putc( '\n', f );
                    indent( level+1, f );
                }
                break;
            case HW_TYPE_OBJECT :
                oPtr = val;
                (void)putc( '\n', f );
                level++;
                for( j = 0; j < n; j++ ) {
                    indent( level, f );
                    tmp = (hwObject)oPtr[j];
                    if( tmp->name )     (void)fprintf( f, "%s", tmp->name );
                    else                (void)__hwWriteAscii( tmp, f, level );
                    if( j < (n-1) )     (void)putc( ',', f );
                    (void)putc( '\n', f );
                }
                level--;
                indent( level, f );
                break;
            }
            (void)putc( '}', f );
            (void)putc( '\n', f );
        }
    }
    indent( level-1, f );
    (void)putc( '}', f );
    level--;
    if( !level ) (void)putc( '\n', f );
    return 1;
}

hwInt32 hwWriteAscii( hwObject obj, FILE *f )
{
    return __hwWriteAscii(obj, f, 0);
}

static int
    nameIndex = 1;
static void
    *stringTab = 0;

static hwInt16 swap16( hwInt16 val )
{
    return (val << 8) | (val >> 8);
}

static hwInt32 swap32( hwInt32 val )
{
    return      ((val & 0x000000FF) << 24)
        |       ((val & 0x0000FF00) <<  8)
        |       ((val & 0x00FF0000) >>  8)
        |       ((val & 0xFF000000) >> 24);
}

static int hwWriteName( char *s, hwObjRW oWrite, void *f )
{
    hwInt16
        iVal;
    int
        res;

    if( s ) {
        iVal = hwLookup( s, stringTab );
        if( iVal >= 0 ) {
            res = oWrite( &iVal, sizeof(hwInt16), 1, f );
            if( res != 1 )      return 0;
        }
        else {
            if( !hwInsert( s, nameIndex, stringTab ) )  return 0;

            iVal = HW_HSH_MAGIC;
            res = oWrite( &iVal, sizeof(hwInt16), 1, f );
            if( res != 1 )      return 0;

            iVal = nameIndex++;
            res = oWrite( &iVal, sizeof(hwInt16), 1, f );
            if( res != 1 )      return 0;

            iVal = strlen( s );
            res = oWrite( &iVal, sizeof(hwInt16), 1, f );
            if( res != 1 )      return 0;

            res = oWrite( (void *)s, iVal, 1, f );
            if( res != 1 )      return 0;
        }
    }
    else {
        iVal = 0;
        res = oWrite( &iVal, sizeof(hwInt16), 1, f );
        if( res != 1 )  return 0;
    }

    return 1;
}

static int hwReadName( char s[256], hwObjRW oRead, void *f, int swap )
{
    hwInt16
        iVal,
        index;
    int
        res;

    if( oRead( &iVal, sizeof(hwInt16), 1, f ) != 1 ) return 0;
    if( swap )  iVal = swap16( iVal );

    if( iVal == 0 ) {
        /* No name, so sorry */
        s[0] = 0;
    }
    else if( iVal == (hwInt16) HW_HSH_MAGIC ) {
        /* New hash definition */
        if( oRead( &index, sizeof(hwInt16), 1, f ) != 1 ) return 0;
        if( swap )      index = swap16( index );

        if( oRead( &iVal, sizeof(hwInt16), 1, f ) != 1 ) return 0;
        if( swap )      iVal = swap16( iVal );

        if( oRead( s, (iVal & 0xFFFF), 1, f ) != 1 ) return 0;
        s[iVal] = 0;

        if( !hwInsert( s, index, stringTab ) ) return 0;
    }
    else {
        if( hwFindSym( s, iVal, stringTab ) < 0 ) return 0;
    }

    return 1;
}


hwInt32 hwBeginBinary( hwObjRW oRdWr, void *f, hwInt32 flags, hwInt32 *numObjs )
{
    hwInt32
        magic = HW_HDR_MAGIC;

    if( stringTab )     return 1;

    if( flags & HW_FILE_WRITE_HDR ) {
        magic = HW_HDR_MAGIC;
        if( oRdWr( &magic, sizeof(hwInt32), 1, f ) != 1 ) return 0;
        if( oRdWr( numObjs, sizeof(hwInt32), 1, f ) != 1 ) return 0;
    }
    if( flags & HW_FILE_READ_HDR ) {
        if( oRdWr( &magic, sizeof(hwInt32), 1, f ) != 1 ) return 0;
        if( magic != HW_HDR_MAGIC_SWAP ) {
            if( magic != HW_HDR_MAGIC ) {
                return 0;
            }
        }
        if( oRdWr( numObjs, sizeof(hwInt32), 1, f ) != 1 ) return 0;
        if( magic == HW_HDR_MAGIC_SWAP ) *numObjs = swap32( *numObjs );
    }

    stringTab = hwCreateHash( 256 );
    if( !stringTab )    return 0;
    nameIndex = 1;

    return magic;
}

hwInt32 hwEndBinary( hwObjRW oRdRw, void *f )
{
    hwDestroyHash( stringTab );
    stringTab = 0;
    return 1;
}


hwInt32 hwWriteBinary( hwObject obj, hwObjRW oWrite, void *f )
{
    int
        result = 0,
        doEnd = 0,
        i, j, n, res;
    hwInt32
        type;
    hwInt16
        iVal;
    void
        *val;
    char
        *s;
    char
        **sPtr;
    hwObject
        tmp, *oPtr;
    int
        errnum;

    if( !stringTab ) {
        type = 1;
        if( !hwBeginBinary( oWrite, f, HW_FILE_WRITE_HDR, &type ) ) {
            errnum = 1;
            goto Error;
        }
        doEnd = 1;
    }

    if( !obj->parent || !obj->parent->name ) {
        errnum = 2;
        goto Error;
    }

    /* Define object */
    iVal = HW_DEF_MAGIC;
    res = oWrite( &iVal, sizeof(hwInt16), 1, f );
    if( res != 1 ) {
        errnum = 3;
        goto Error;
    }

    /* Class name */
    s = (char *)obj->parent->name;
    if( !hwWriteName( s, oWrite, f ) ) {
        errnum = 4;
        goto Error;
    }

    /* Object name */
    s = (char *)obj->name;
    if( !hwWriteName( s, oWrite, f ) ) {
        errnum = 5;
        goto Error;
    }

    /* Write out number of properties */
    iVal = 0;
    for( i = 0; obj->props[i]; i++ ) {
        s = (char *)obj->props[i];
        type = obj->inquire( obj, s, &val );
        if( type && !(type & HW_TYPE_CLEAN) ) {
            iVal++;
        }
    }
    res = oWrite( &iVal, sizeof(hwInt16), 1, f );
    if( res != 1 ) {
        errnum = 6;
        goto Error;
    }

    for( i = 0; obj->props[i]; i++ ) {
        s = (char *)obj->props[i];

        type = obj->inquire( obj, s, &val );
        if( !type )     continue;
        if( type & HW_TYPE_CLEAN )      continue;

        /* Prop name */
        if( !hwWriteName( s, oWrite, f ) ) {
            errnum = 7;
            goto Error;
        }

        res = oWrite( &type, sizeof(hwInt32), 1, f );
        if( res != 1 ) {
            errnum = 8;
            goto Error;
        }

        n = HW_GET_COUNT( type );
        switch( HW_GET_BASE( type ) ) {
        case HW_TYPE_INT :
        case HW_TYPE_BOOL :
        case HW_TYPE_FLOAT :
            res = oWrite( val, n*sizeof(hwInt32), 1, f );
            if( res != 1 ) {
                errnum = 9;
                goto Error;
            }
            break;
        case HW_TYPE_SHORT :
            res = oWrite( val, n*sizeof(hwInt16), 1, f );
            if( res != 1 ) {
                errnum = 9;
                goto Error;
            }
            break;
        case HW_TYPE_BYTE :
            res = oWrite( val, n*sizeof(hwInt8), 1, f );
            if( res != 1 ) {
                errnum = 9;
                goto Error;
            }
            break;
        case HW_TYPE_STRING :
            if( n == 0 ) {
                iVal = strlen( (char *)val );
                res = oWrite( &iVal, sizeof(hwInt16), 1, f );
                if( res != 1 ) {
                    errnum = 10;
                    goto Error;
                }

                res = oWrite( val, iVal, 1, f );
                if( res != 1 ) {
                    errnum = 11;
                    goto Error;
                }
            }
            else {
                sPtr = val;
                for( j = 0; j < n; j++ ) {
                    iVal = strlen( sPtr[j] );
                    res = oWrite( &iVal, sizeof(hwInt16), 1, f );
                    if( res != 1 ) {
                        errnum = 12;
                        goto Error;
                    }

                    res = oWrite( sPtr[j], iVal, 1, f );
                    if ( res != 1 ) {
                        errnum = 13;
                        goto Error;
                    }
                }
            }
            break;
        case HW_TYPE_OBJECT :
            if( n == 0 ) {
                tmp = (hwObject)val;
                if( tmp->name ) {
                    iVal = HW_REF_MAGIC;
                    res = oWrite( &iVal, sizeof(hwInt16), 1, f );
                    if( res != 1 ) {
                        errnum = 14;
                        goto Error;
                    }

                    /* Obj name */
                    if( !hwWriteName( (char *)tmp->name, oWrite, f ) ) {
                        errnum = 15;
                        goto Error;
                    }
                }
                else {
                    if( !hwWriteBinary( tmp, oWrite, f ) ) {
                        errnum = 16;
                        goto Error;
                    }
                }
            }
            else {
                oPtr = val;
                for( j = 0; j < n; j++ ) {
                    tmp = (hwObject)oPtr[j];
                    if( tmp->name ) {
                        iVal = HW_REF_MAGIC;
                        res = oWrite( &iVal, sizeof(hwInt16), 1, f );
                        if( res != 1 ) {
                            errnum = 17;
                            goto Error;
                        }

                        /* Obj name */
                        if( !hwWriteName( (char *)tmp->name, oWrite, f ) ) {
                            errnum = 18;
                            goto Error;
                        }
                    }
                    else {
                        if( !hwWriteBinary( tmp, oWrite, f ) ) {
                            errnum = 19;
                            goto Error;
                        }
                    }
                }
            }
            break;
        }
    }

    /* Successful! */
    result = 1;
    if( doEnd ) {
        if( !hwEndBinary( oWrite, f ) ) result = 0;
    }
    return result;

Error:

    if( doEnd ) {
        if( !hwEndBinary( oWrite, f ) ) result = 0;
    }

    return result;
}

static hwObject hwReadObject( hwObjRW oRead, void *f )
{
    hwInt16
        iVal,
        numProps;
    hwInt32
        type;
    char
        cName[256],
        oName[256],
        pName[256],
        *sVal;
    void
        *val = 0;
    int
        i, errnum,
        swap = 0,
        newSize,
        valSize = 0;
    hwObject
        obj = 0,
        oVal = 0,
        objClass = 0;

    if( oRead( &iVal, sizeof(hwInt16), 1, f ) != 1 ) {
        errnum = 1;
        goto Error;
    }
    if( iVal == (hwInt16)HW_DEF_MAGIC_SWAP )    swap = 1;
    if( iVal == (hwInt16)HW_REF_MAGIC_SWAP )    swap = 1;
    if( swap )  iVal = swap16( iVal );

    if( iVal == (hwInt16)HW_REF_MAGIC ) {
        /* Reference to previously defined object */
        if( !hwReadName( oName, oRead, f, swap ) ) {
            errnum = 2;
            goto Error;
        }
        obj = hwFindObject( oName );
        if( !obj ) {
            errnum = 3;
            goto Error;
        }
        obj->addref( obj );     /* We're referring to it again! */
        return obj;
    }

    if( iVal != (hwInt16)HW_DEF_MAGIC ) {
        errnum = 4;
        goto Error;
    }

    if( !hwReadName( cName, oRead, f, swap ) ) {
        errnum = 5;
        goto Error;
    }
    objClass = hwFindClass( cName );
    if( !objClass ) {
        errnum = 6;
        goto Error;
    }

    obj = objClass->create( objClass );
    if( !obj ) {
        errnum = 7;
        goto Error;
    }

    if( !hwReadName( oName, oRead, f, swap ) ) {
        errnum = 8;
        goto Error;
    }
    if( *oName ) {
        obj->name = malloc( strlen( oName ) + 1 );
        if( !obj->name ) {
            errnum = 9;
            goto Error;
        }
        (void)strcpy( (char *)obj->name, oName );
        hwRegisterObject( obj );
    }
    else {
        /* Blank-out existing name */
        obj->name = 0;
    }

    if( oRead( &numProps, sizeof(hwInt16), 1, f ) != 1 ) {
        errnum = 10;
        goto Error;
    }
    if( swap ) numProps = swap16( numProps );

    while( numProps-- > 0 ) {
        if( !hwReadName( pName, oRead, f, swap ) ) {
            errnum = 11;
            goto Error;
        }

        if( oRead( &type, sizeof(hwInt32), 1, f ) != 1 ) {
            errnum = 12;
            goto Error;
        }
        if( swap ) type = swap32( type );

        newSize = HW_GET_COUNT( type );
        if( newSize > valSize ) {
            val = realloc( val, newSize * sizeof(void *) );
            if( !val ) {
                errnum = 13;
                goto Error;
            }
            valSize = newSize;
        }

        switch( HW_GET_BASE( type ) ) {
        case HW_TYPE_INT :
        case HW_TYPE_BOOL :
        case HW_TYPE_FLOAT :
            if( oRead( val, newSize*sizeof(hwInt32), 1, f ) != 1 ) {
                errnum = 14;
                goto Error;
            }
            if( swap ) {
                for( i = 0; i < newSize; i++ ) {
                    ((hwInt32 *)val)[i] = swap32( ((hwInt32 *)val)[i] );
                }
            }
            if( strcmp( pName, hwStrOptFlags ) == 0 ) {
                if( type == HW_TYPE_1I ) {
                    /* Read data is never "safe" */
                    *(hwInt32 *)val &= ~HW_OPT_SAFE_USER_DATA;
                }
            }
            obj->modify( obj, pName, type, val );
            break;
        case HW_TYPE_BYTE :
            if( oRead( val, newSize*sizeof(hwInt8), 1, f ) != 1 ) {
                errnum = 14;
                goto Error;
            }
            obj->modify( obj, pName, type, val );
            break;
        case HW_TYPE_SHORT :
            if( oRead( val, newSize*sizeof(hwInt16), 1, f ) != 1 ) {
                errnum = 14;
                goto Error;
            }
            obj->modify( obj, pName, type, val );
            break;
        case HW_TYPE_STRING :
            if( newSize == 0 ) {
                /* Scalar string */
                if( oRead( &iVal, sizeof(hwInt16), 1, f ) != 1 ) {
                    errnum = 15;
                    goto Error;
                }
                if( swap ) iVal = swap16( iVal );

                sVal = malloc( iVal + 1 );
                if( !sVal ) {
                    errnum = 16;
                    goto Error;
                }

                if( oRead( sVal, iVal, 1, f ) != 1 ) {
                    errnum = 17;
                    goto Error;
                }
                sVal[iVal] = 0;
                obj->modify( obj, pName, type, sVal );
                free( sVal );
            }
            else {
                for( i = 0; i < newSize; i++ ) {
                    if( oRead( &iVal, sizeof(hwInt16), 1, f ) != 1 ) {
                        errnum =18;
                        goto Error;
                    }
                    if( swap ) iVal = swap16( iVal );

                    ((char **)val)[i] = sVal = malloc( iVal + 1 );
                    if( !sVal ) {
                        errnum = 19;
                        goto Error;
                    }

                    if( oRead( sVal, iVal, 1, f ) != 1 ) {
                        errnum = 20;
                        goto Error;
                    }
                    sVal[iVal] = 0;
                }
                obj->modify( obj, pName, type, val );
                for( i = 0; i < newSize; i++ ) {
                    sVal = ((char **)val)[i];
                    free( sVal );
                }
            }
            break;
        case HW_TYPE_OBJECT :
            if( newSize == 0 ) {
                /* Scalar object */
                oVal = hwReadObject( oRead, f );
                if( !oVal ) {
                    errnum = 21;
                    goto Error;
                }
                obj->modify( obj, pName, type, oVal );
            }
            else {
                for( i = 0; i < newSize; i++ ) {
                    oVal = hwReadObject( oRead, f );
                    if( !oVal ) {
                        errnum = 22;
                        goto Error;
                    }
                    ((hwObject *)val)[i] = oVal;
                }
                obj->modify( obj, pName, type, val );
            }
            break;
        default :
            errnum = 23;
            goto Error;
        }
    }
    return obj;

Error :
    if( obj ) {
        if( obj->name ) free( (void *)obj->name );
        obj->name = 0;
        obj->destroy( obj );
    }
    return 0;
}

hwInt32 hwReadBinary( hwObjRW oRead, void *f, hwInt32 numObjs, hwObject **result )
{
    int
        res = 0,
        swap = 0,
        doEnd = 0;
    int
        numFound = 0,
        numAlloc = 0;
    hwObject
        obj;
    hwInt32
        type;

    if( !stringTab ) {
        type = hwBeginBinary( oRead, f, HW_FILE_READ_HDR, &numObjs );
        if( !type ) goto Error;
        swap = (type == HW_HDR_MAGIC_SWAP);
        doEnd = 1;
    }

    *result = 0;
    while( numObjs-- ) {
        obj = hwReadObject( oRead, f );
        if( numFound >= numAlloc ) {
            if( !numAlloc ) numAlloc = 8;
            numAlloc *= 2;
            *result = realloc( *result, numAlloc * sizeof(hwObject *) );
            if( !*result )      goto Error;
        }
        (*result)[numFound++] = obj;
    }

    /* Successful! */
    res = numFound;
Error :
    if( doEnd ) {
        if( !hwEndBinary( oRead, f ) ) res = 0;
    }

    return res;
}

/*** EOF hwWrite.c ***/
