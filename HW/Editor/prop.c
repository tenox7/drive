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

#include "hwedit.h"
#include <ctype.h>
#include <string.h>

static int
    ungotChar = -1,
    ungotTok = -1;

#define MAXTOK  512
static char
    currTok[MAXTOK];

enum {
        /* Tokens */
        T_LBRACE,       T_RBRACE,
        T_COMMA,

        /* Complex tokens */
        T_STRING,       T_INT,          T_FLOAT,        T_NAME,
        T_BOOL,

        /* Placeholders */
        T_UNKNOWN,      T_EOF
};

char *objectToString( hwObject obj )
{
    static char buff[16];

    if( obj->name ) {
        return (char *)obj->name;
    }
    else {
        sprintf( buff, "@%p", (void *)obj );
        return buff;
    }
}

hwObject stringToObject( char *s )
{
    hwObject
        result;

    if( s[0] == '@' ) {
        sscanf( s+1, "%p", (void **)&result );
    }
    else {
        result = hwFindObject( s );
    }
    return result;
}

ObjectList findObject( hwObject obj )
{
    ObjectList
        result;

    for( result = edit.olist; result; result = result->next ) {
        if( result->obj == obj ) return result;
    }
    return 0;
}
static int readChar( char **ptr )
{
    int
        result;

    if( ungotChar >= 0 ) {
        result = ungotChar;
        ungotChar = -1;
    }
    else if( **ptr ) {
        result = *(*ptr)++;
    }
    else {
        result = EOF;
    }
    return result;
}

static void ungetChar( int ch )
{
    if( !isspace( ch ) ) {
        ungotChar = ch;
    }
}

static void ungetTok( int t )
{
    ungotTok = t;
}

static int skipWhite( char **ptr )
{
    int
        c;

    if( ungotChar >= 0 ) {
         c = ungotChar;
         ungotChar = -1;
         return c;
    }

    do {
        c = readChar( ptr );
    } while( (c != EOF) && isspace( c ) );

    return c;
}

#ifdef WIN32 /* [ */
static int strcasecmp( char *a, char *b )
{
    int
        ca, cb;

    do {
        ca = *a++; if( ca ) ca = tolower( ca );
        cb = *b++; if( cb ) cb = tolower( cb );
    } while( ca && cb && (ca == cb) );
    return ca - cb;
}
#endif /* ] */

static int getComplex( char **ptr, int ch )
{
    int
        res, state, i, hex, octal;
    char
        *s;

    res = T_UNKNOWN;
    state = 1;
    hex = 0;
    octal = 0;
    s = currTok;

    do {
        switch( state ) {
        case 1 :
            if( ch == '0' )                                     state = 2;
            else if( isdigit(ch) || (ch == '-') )               state = 5;
            else if( isalpha(ch) | (ch == '_') | (ch == '$') )  state = 4;
            else if( ch == '@' )                                state = 4;
            else if( ch == '.' )                                state = 10;
            else                                                state = -1;
            break;

        case 2 :
            res = T_INT;
            octal = 1;
            if( (ch == 'x') | (ch == 'X') )                     state = 3;
            else if( (ch == 'e') | (ch == 'E') )                state = 7;
            else if( ch == '.' )                                state = 6;
            else if( isdigit(ch) )                              state = 5;
            else                                                state = 0;
            break;

        case 3 :
            hex = 1;
            if( isxdigit( ch ) )                                state = 3;
            else                                                state = 0;
            break;

        case 4 :
            res = T_NAME;
            if( isalpha(ch)|isdigit(ch)|(ch == '_')|(ch == '$') ) state = 4;
            else                                                state = 0;
            break;

        case 5 :
            res = T_INT;
            if( (ch == 'e') | (ch == 'E') )                     state = 7;
            else if( ch == '.' )                                state = 6;
            else if( isdigit(ch) )                              state = 5;
            else                                                state = 0;
            break;

        case 6 :
            res = T_FLOAT;
            if( (ch == 'e') | (ch == 'E') )                     state = 7;
            else if( isdigit(ch) )                              state = 6;
            else                                                state = 0;
            break;

        case 7 :
            res = T_FLOAT;
            if( (ch == '+') | (ch == '-') )                     state = 8;
            else if( isdigit( ch ) )                            state = 9;
            else                                                state = -1;
            break;

        case 8 :
            if( isdigit(ch) )                                   state = 9;
            else                                                state = -1;
            break;

        case 9 :
            if( isdigit(ch) )                                   state = 9;
            else                                                state = 0;
            break;

        case 10 :
            if( isdigit( ch ) )                                 state = 6;
            else                                                state = 0;
            break;
        }
        if( state > 0 ) {
            *s = ch;
            s = s + 1;
            ch = readChar( ptr );
        }
    } while( state > 0 );

    *s = 0;
    if( state == 0 )    ungetChar( ch );
    if( state < 0 ) {
        if( ch == EOF ) res = T_EOF;
        /* Error( BadChar ); */
    }

    if( res == T_NAME ) {
        if( strcasecmp( currTok, "True" ) == 0 ) {
            res = T_BOOL;
            (void)strcpy( currTok, "1" );
        }
        else if( strcasecmp( currTok, "False" ) == 0 ) {
            res = T_BOOL;
            (void)strcpy( currTok, "0" );
        }
    }
    else if( res == T_INT ) {
        if( hex ) {
            (void)sscanf( currTok+2, "%x", &hex );
            (void)sprintf( currTok, "%d", hex );
        }
        else if( octal ) {
            (void)sscanf( currTok, "%o", &octal );
            (void)sprintf( currTok, "%d", octal );
        }
    }

    return res;
}

static int getToken( char **ptr )
{
    int
        i, c, escaped;

    if( ungotTok >= 0 ) {
        i = ungotTok;
        ungotTok = -1;
        return i;
    }

    c = skipWhite( ptr );
    switch( c ) {
    case '{' :  return T_LBRACE;
    case '}' :  return T_RBRACE;
    case ',' :  return T_COMMA;
    case EOF :  return T_EOF;

    case '\"' :
        i = 0;
        do {
            c = readChar( ptr );
            if( c == '\\' ) {
                c = readChar( ptr );
                escaped = 1;
            }
            else {
                escaped = 0;
            }
            if( c == EOF ) {
                /* Error( Unterminated ); */
                return 0;
            }
            if( i >= MAXTOK ) {
                /* Error( TooLong ); */
                return 0;
            }
            currTok[i++] = c;
        } while( (c != '\"') || escaped );
        currTok[i-1] = 0;
        return T_STRING;

    default :
        return getComplex( ptr, c );
    }
}

static int reallocArray( void **arr, int *size )
{
    if( !*size )        *size = 16;
    *size *= 2;
    *arr = realloc( *arr, *size );

    if( *arr )  return 1;
    else        return 0;
}

static hwInt32 getArray( char **f, int base, void **val )
{
    int
        t, n, b;
    void
        *arr = 0;
    int
        arrSize = 0;
    hwFloat
        v;
    hwObject
        obj;
    hwInt32
        type;
    void
        *vval;

    switch( base ) {
    case HW_TYPE_STRING :
        b = sizeof(char *);
        break;
    case HW_TYPE_FLOAT :
        b = sizeof(hwFloat);
        break;
    case HW_TYPE_OBJECT :
        b = sizeof(hwObject);
        break;
    }

    n = 0;
    do {
        if( (n*b) >= arrSize ) {
            if( !reallocArray( &arr, &arrSize ) )       return 0;
        }
        t = getToken( f );
        switch( t ) {
        case T_RBRACE :
            /* End of the array */
            break;
        case T_EOF :
        case T_UNKNOWN :
        case T_COMMA :
        case T_LBRACE :
            goto BadProp;
        case T_BOOL :
            if( base != HW_TYPE_BOOL )  goto BadProp;
            ((hwInt32 *)arr)[n++] = atoi( currTok );
            break;
        case T_INT :
            if( base != HW_TYPE_FLOAT ) goto BadProp;
            ((hwFloat *)arr)[n++] = (hwFloat)atoi( currTok );
            break;
        case T_FLOAT :
            if( base != HW_TYPE_FLOAT ) goto BadProp;
            ((hwFloat *)arr)[n++] = atof( currTok );
            break;
        case T_NAME :
            type = hwFindConstant( currTok, &vval );
            if( base != HW_TYPE_OBJECT )        goto BadProp;
            obj = stringToObject( currTok );
            if( !obj ) {
                if( arr ) free( arr );
                return 0;
            }
            ((hwObject *)arr)[n++] = obj;
            break;
        default :
            goto BadProp;
        }
        if( t != T_RBRACE ) {
            t = getToken( f );
        }
    } while( t == T_COMMA );

    if( t != T_RBRACE ) {
        /* Error( RightExpected ); */
    }

    *val = arr;
    return HW_MAKE_TYPE( base, n );

BadProp :
    /* Error( BadProperty ); */
    if( arr ) free( arr );
    return 0;
}

hwInt32 getValue( char **f, void **val, int *needFree )
{
    hwObject
        Class;
    static hwFloat
        FVal;
    static hwInt32
        IVal;
    hwObject
        OVal;
    int
        i, t;
    hwInt32
        Type;
    void
        *vval;

    *needFree = 0;
    t = getToken( f );
    switch( t ) {
    case T_LBRACE :
        t = getToken( f );
        switch( t ) {
        case T_LBRACE :
            /* Hmm.  What will this be? */
            Type = 0;
            break;
        case T_STRING :
            Type = HW_TYPE_STRING;
            break;
        case T_BOOL :
            Type = HW_TYPE_BOOL;
            break;
        case T_INT :
            Type = HW_TYPE_FLOAT;
            break;
        case T_FLOAT :
            Type = HW_TYPE_FLOAT;
            break;
        case T_NAME :
            OVal = stringToObject( currTok );
            if( !OVal ) return 0;
            Type = HW_TYPE_OBJECT;
            break;
        default :
            Type = HW_TYPE_FLOAT;
            break;
        }
        ungetTok( t );
        Type = getArray( f, Type, val );
        if( !Type )     return 0;
        *needFree = 1;
        break;
    case T_STRING :
        Type = HW_TYPE_STRING;
        *val = currTok;
        break;
    case T_BOOL :
        Type = HW_TYPE_1B;
        IVal = atoi( currTok );
        *val = &IVal;
        break;
    case T_INT :
        Type = HW_TYPE_1I;
        IVal = atoi( currTok );
        *val = &IVal;
        break;
    case T_FLOAT :
        Type = HW_TYPE_1F;
        FVal = atof( currTok );
        *val = &FVal;
        break;
    case T_NAME :
        OVal = stringToObject( currTok );
        if( !OVal ) return 0;
        Type = HW_TYPE_OBJECT;
        *val = OVal;
        break;
    default :
        return 0;
    }

    return Type;
}

void printProp( hwInt32 type, void *val )
{
    hwInt32
        ival;
    int
        i, j, n;
    hwFloat
        *fPtr;
    hwInt32
        *iPtr;
    char
        **sPtr;
    hwObject
        tmp, *oPtr;
    char
        *s, buff[512];

    switch( type & ~HW_TYPE_CLEAN ) {
    case HW_TYPE_1I :
        ival = *(hwInt32 *)val;
        if( ival < 0 ) {
            (void)sprintf( buff, "0x%08x\n", ival );
        }
        else {
            (void)sprintf( buff, "%d\n", ival );
        }
        edit.activeCmd->putString( edit.activeCmd, buff );
        break;
    case HW_TYPE_1B :
        (void)sprintf( buff, "%s\n",
                    *(hwInt32 *)val ? "True" : "False" );
        edit.activeCmd->putString( edit.activeCmd, buff );
        break;
    case HW_TYPE_1F :
        (void)sprintf( buff, "%g\n", *(hwFloat *)val );
        edit.activeCmd->putString( edit.activeCmd, buff );
        break;
    case HW_TYPE_STRING :
        (void)sprintf( buff, "\"%s\"\n", (char *)val );
        edit.activeCmd->putString( edit.activeCmd, buff );
        break;
    case HW_TYPE_OBJECT :
        tmp = (hwObject)val;
        s = objectToString( tmp );
        (void)sprintf( buff, "%s\n", s );
        edit.activeCmd->putString( edit.activeCmd, buff );
        break;
    default :
        n = HW_GET_COUNT( type );
        edit.activeCmd->putString( edit.activeCmd, "{" );
        switch( HW_GET_BASE( type ) ) {
        case HW_TYPE_INT :
            iPtr = val;
            for( j = 0; j < n; j++ ) {
                ival = iPtr[j];
                (void)sprintf( buff, (ival < 0) ? "0x%08x" : "%d", ival );
                edit.activeCmd->putString( edit.activeCmd, buff );
                if( j < (n-1) ) edit.activeCmd->putString( edit.activeCmd, "," );
            }
            break;
        case HW_TYPE_BOOL :
            iPtr = val;
            for( j = 0; j < n; j++ ) {
                (void)sprintf( buff, "%s", iPtr[j] ? "True" : "False" );
                edit.activeCmd->putString( edit.activeCmd, buff );
                if( j < (n-1) ) edit.activeCmd->putString( edit.activeCmd, "," );
            }
            break;
        case HW_TYPE_FLOAT :
            fPtr = val;
            for( j = 0; j < n; j++ ) {
                (void)sprintf( buff, "%g", fPtr[j] );
                edit.activeCmd->putString( edit.activeCmd, buff );
                if( j < (n-1) ) edit.activeCmd->putString( edit.activeCmd, "," );
            }
            break;
        case HW_TYPE_STRING :
            sPtr = val;
            for( j = 0; j < n; j++ ) {
                edit.activeCmd->putString( edit.activeCmd, "\"" );
                edit.activeCmd->putString( edit.activeCmd, sPtr[j] );
                edit.activeCmd->putString( edit.activeCmd, "\"" );
                if( j < (n-1) ) edit.activeCmd->putString( edit.activeCmd, "," );
            }
            break;
        case HW_TYPE_OBJECT :
            oPtr = val;
            for( j = 0; j < n; j++ ) {
                tmp = (hwObject)oPtr[j];
                s = objectToString( tmp );
                edit.activeCmd->putString( edit.activeCmd, s );
                if( j < (n-1) ) edit.activeCmd->putString( edit.activeCmd, "," );
            }
            break;
        }
        edit.activeCmd->putString( edit.activeCmd, "}\n" );
    }
}
