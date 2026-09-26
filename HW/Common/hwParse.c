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

#include "hw.h"
#include "hw_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <setjmp.h>
#include <string.h>

enum {
        /* Tokens */
        T_LPAR = 1,     T_RPAR,         T_LBRACE,       T_RBRACE,
        T_COMMA,        T_EQUALS,       T_PLUS,         T_MINUS,
        T_MUL,          T_DIV,

        /* Complex tokens */
        T_STRING,       T_INT,          T_FLOAT,        T_NAME,
        T_BOOL,

        /* Placeholders */
        T_UNKNOWN,      T_EOF
};

static const struct {
    char
        *SymName;
    int
        SymType;
    char
        *SymVal;
}
    SymTab[] = {
        { "True",       T_BOOL,         "1" },
        { "False",      T_BOOL,         "0" },

        { 0, 0 }
    };
static const char
    Unterminated[] = "Unterminated string",
    BadForward[] = "Forward declaration does not match object",
    ClassExpected[] = "Class expected",
    BadExpr[] = "Illegal expression",
    EqualsExpected[] = "'=' expected",
    BadChar[] = "Illegal character",
    RightExpected[] = "'}' expected",
    NumExpected[] = "Number expected",
    NameExpected[] = "Name expected",
    BraceExpected[] = "'{' expected",
    StringExpected[] = "String expected",
    KeywordExpected[] = "Keyword expected",
    TooLong[] = "Token too long",
    BadType[] = "Illegal type",
    BadObject[] = "Undeclared object",
    BadProperty[] = "Illegal property",
    ArrayTooBig[] = "Array would be too big",
    OutOfMemory[] = "Out of memory";

#define MAXTOK  1024
#define STATIC_ARRAY_COUNT      3000    /* Big enough for 17*17*XYZRGBNxNyNz */

/* A Hoverware input / output stream, including everything needed for parsing */
typedef struct __hwParseStream {
    int
        (*getChar)( struct __hwParseStream * );
    int
        CurrLine,
        Erred,
        UngotTok,
        UngotChar;
    char
        CurrFile[1024];
    jmp_buf
        ExprErr;
    char
        Token[MAXTOK];
    int
        TokType,
        TokVal;
    hwObject
        OVal;
    hwFloat
        FVal;
    hwInt32
        IVal;
    // Private data follows
 } *hwParseStream;

/* Forward declarations... */
static int GetExpression( hwParseStream, hwFloat * );
static hwFloat GetExpr( hwParseStream );
static hwObject hwGetObject( hwParseStream );

static void Error( hwParseStream str, const char *Msg )
{
#ifdef ANDROID_NDK
    __android_log_print(ANDROID_LOG_INFO, "HW", "File %s, line %d: %s",
                        str->CurrFile, str->CurrLine, Msg );
#else
    (void)fprintf( stderr, "File %s, line %d: %s\n",
                        str->CurrFile, str->CurrLine, Msg );
#endif
    str->Erred = 1;
}

static int ParseDirective( hwParseStream str )
{
    int
        c, n;
    char
        *s;

    /* Skip initial spaces */
    do {
        c = str->getChar( str );
    } while( c == ' ' );
    if( c == EOF )      return EOF;

    if( isdigit( c ) ) {
        /* Aha!  It's going to be a line number directive */

        /* Get the number */
        n = 0;
        while( isdigit( c ) ) {
            n = n*10 + (c - '0');
            c = str->getChar( str );
        }
        if( c == EOF )  return EOF;

        str->CurrLine = n - 1;

        /* Skip the space inbetween */
        while( c == ' ' ) {
            c = str->getChar( str );
        }
        if( c == EOF )  return EOF;

        /* Get the filename */
        if( c == '\"' ) {
            s = str->CurrFile;
            do {
                c = str->getChar( str );
                *s++ = c;
            } while( (c != EOF) && (c != '\"') );
            *--s = 0;
            if( c != EOF ) {
                c = str->getChar( str );
            }
        }
    }

    /* Ignore the rest of the line */
    while( (c != EOF) && (c != '\n') ) {
        c = str->getChar( str );
    }
    if( c == '\n' ) {
        str->CurrLine++;
    }

    return c;
}

static int SkipWhite( hwParseStream str )
{
    int
        c;

    if( str->UngotChar ) {
         c = str->UngotChar;
         str->UngotChar = 0;
         return c;
    }

    do {
        c = str->getChar( str );
        if( c == '\n' ) {
            str->CurrLine++;
        }
        else if( c == '#' ) {
            c = ParseDirective( str );
        }
    } while( (c != EOF) && isspace( c ) );

    return c;
}

static int GetChar( hwParseStream str )
{
    int
        c;

    if( str->UngotChar ) {
         c = str->UngotChar;
         str->UngotChar = 0;
         return c;
    }

    c = str->getChar( str );
    if( c == '\n' ) {
        str->CurrLine++;
    }
    return c;
}

static void UngetChar( hwParseStream str, int c )
{
    if( !isspace( c ) ) {
        str->UngotChar = c;
    }
}

static void UngetTok( hwParseStream str, int t )
{
    str->UngotTok = t;
}


#if defined(WIN32) && !defined(CYGWIN) && !defined(MINGW) /* [ */
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

static int GetComplex( hwParseStream str, int ch )
{
    int
        res, state, i, hex, octal;
    char
        *s;

    res = T_UNKNOWN;
    state = 1;
    hex = 0;
    octal = 0;
    s = str->Token;

    do {
        switch( state ) {
        case 1 :
            if( ch == '0' )                                     state = 2;
            else if( isdigit(ch) )                              state = 5;
            else if( isalpha(ch) | (ch == '_') | (ch == '$') )  state = 4;
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
            ch = GetChar( str );
        }
    } while( state > 0 );

    *s = 0;
    if( state == 0 )    UngetChar( str, ch );
    if( state < 0 ) {
        if( ch == EOF ) res = T_EOF;
        Error( str, BadChar );
    }

    if( res == T_NAME ) {
        for( i = 0; SymTab[i].SymName; i++ ) {
            if( strcasecmp( SymTab[i].SymName, str->Token ) == 0 ) {
                res = SymTab[i].SymType;
                (void)strcpy( str->Token, SymTab[i].SymVal );
                break;
            }
        }
    }
    else if( res == T_INT ) {
        if( hex ) {
            (void)sscanf( str->Token+2, "%x", &hex );
            (void)sprintf( str->Token, "%d", hex );
        }
        else if( octal ) {
            (void)sscanf( str->Token, "%o", &octal );
            (void)sprintf( str->Token, "%d", octal );
        }
    }

    return res;
}

static int GetTok( hwParseStream str )
{
    int
        i, c, escaped;

    if( str->UngotTok >= 0 ) {
        i = str->UngotTok;
        str->UngotTok = -1;
        return i;
    }

    c = SkipWhite( str );
    switch( c ) {
    case '(' :  return T_LPAR;
    case ')' :  return T_RPAR;
    case '{' :  return T_LBRACE;
    case '}' :  return T_RBRACE;
    case ',' :  return T_COMMA;
    case '=' :  return T_EQUALS;
    case EOF :  return T_EOF;
    case '*' :  return T_MUL;
    case '/' :  return T_DIV;
    case '+' :  return T_PLUS;
    case '-' :  return T_MINUS;

    case '\"' :
        i = 0;
        do {
            c = GetChar( str );
            if( c == '\\' ) {
                c = GetChar( str );
                if( c == 'n' ) {
                    c = '\n';
                }
                escaped = 1;
            }
            else {
                escaped = 0;
            }
            if( c == EOF ) {
                Error( str, Unterminated );
                return 0;
            }
            if( i >= MAXTOK ) {
                Error( str, TooLong );
                return 0;
            }
            str->Token[i++] = c;
        } while( (c != '\"') || escaped );
        str->Token[i-1] = 0;
        return T_STRING;

    default :
        return GetComplex( str, c );
    }
}


/* Our grammar for expressions:
 *
 *      Expr -> Expr + Term
 *      Expr -> Term
 *      Term -> Term * Fact
 *      Term -> Fact
 *      Fact -> -Fact
 *      Fact -> T_FLOAT | T_INT
 *      Fact -> ( Expr )
 */

static hwFloat GetFact( hwParseStream str )
{
    int
        t;
    hwFloat
        Res;
    hwInt32
        type;
    void
        *val;

    t = GetTok( str );
    if( t == T_LPAR ) {
        Res = GetExpr( str );
        t = GetTok( str );
        if( t != T_RPAR ) {
            longjmp( str->ExprErr, 1 );
        }
        return Res;
    }
    else if( t == T_FLOAT ) {
        return atof( str->Token );
    }
    else if( t == T_INT ) {
        return (hwFloat)atoi( str->Token );
    }
    else if( t == T_MINUS ) {
        return -GetFact( str );
    }
    else if( t == T_NAME ) {
        type = hwFindConstant( str->Token, &val );
        if( type == HW_TYPE_1F ) {
            return *(hwFloat *)val;
        }
        else if( type == HW_TYPE_1I ) {
            return *(hwInt32 *)val;
        }
        else  {
            longjmp( str->ExprErr, 1 );
            /*NOTREACHED*/
        }
    }
    else {
        longjmp( str->ExprErr, 1 );
        /*NOTREACHED*/
    }
}

static hwFloat GetTerm( hwParseStream str )
{
    hwFloat
        Res;
    int
        t;

    Res = GetFact( str );
    t = GetTok( str );
    while( (t == T_MUL) || (t == T_DIV) ) {
        if( t == T_MUL ) {
            Res *= GetFact( str );
        }
        else {
            Res /= GetFact( str );
        }
        t = GetTok( str );
    }
    UngetTok( str, t );
    return Res;
}

static hwFloat GetExpr( hwParseStream str )
{
    hwFloat
        Res;
    int
        t;

    Res = GetTerm( str );
    t = GetTok( str );
    while( (t == T_PLUS) || (t == T_MINUS) ) {
        if( t == T_PLUS ) {
            Res += GetTerm( str );
        }
        else {
            Res -= GetTerm( str );
        }
        t = GetTok( str );
    }
    UngetTok( str, t );
    return Res;
}

static int GetExpression( hwParseStream str, hwFloat *Res )
{
    if( setjmp( str->ExprErr ) != 0 ) {
        Error( str, BadExpr );
        return 0;
    }

    *Res = GetExpr( str );

    return 1;
}

static int ReallocArray( void **arr, int *size )
{
    if( !*size )        *size = 16;
    *size *= 2;
    *arr = realloc( *arr, *size );

    if( *arr )  return 1;
    else        return 0;
}

static hwInt32 GetArray( hwParseStream str, int base, void **val )
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
            if( !ReallocArray( &arr, &arrSize ) )       return 0;
        }
        t = GetTok( str );
        switch( t ) {
        case T_RBRACE :
            /* End of the array */
            break;
        case T_EOF :
        case T_UNKNOWN :
        case T_COMMA :
        case T_MUL :
        case T_DIV :
        case T_LBRACE :
            goto BadProp;
        case T_BOOL :
            if( base != HW_TYPE_BOOL )  goto BadProp;
            ((hwInt32 *)arr)[n++] = atoi( str->Token );
            break;
        case T_INT :
            if( base != HW_TYPE_FLOAT ) goto BadProp;
            ((hwFloat *)arr)[n++] = (hwFloat)atoi( str->Token );
            break;
        case T_FLOAT :
            if( base != HW_TYPE_FLOAT ) goto BadProp;
            ((hwFloat *)arr)[n++] = atof( str->Token );
            break;
        case T_NAME :
            type = hwFindConstant( str->Token, &vval );
            if( type ) {
                switch( HW_GET_BASE(type) ) {
                case HW_TYPE_BOOL :
                    if( base != HW_TYPE_BOOL )  goto BadProp;
                    if( HW_GET_COUNT(type) != 1 ) goto BadProp;
                    ((hwInt32 *)arr)[n++] = *(hwInt32 *)vval;
                    break;
                case HW_TYPE_INT :
                    if( base != HW_TYPE_FLOAT ) goto BadProp;
                    if( HW_GET_COUNT(type) != 1 ) goto BadProp;
                    ((hwFloat *)arr)[n++] = *(hwInt32 *)vval;
                    break;
                case HW_TYPE_FLOAT :
                    if( base != HW_TYPE_FLOAT ) goto BadProp;
                    if( HW_GET_COUNT(type) != 1 ) goto BadProp;
                    ((hwFloat *)arr)[n++] = *(hwFloat *)vval;
                    break;
                case HW_TYPE_OBJECT :
                    if( base != HW_TYPE_OBJECT )        goto BadProp;
                    if( HW_GET_COUNT(type) != 0 ) goto BadProp;
                    ((hwObject *)arr)[n++] = vval;
                    break;
                case HW_TYPE_STRING :
                    if( base != HW_TYPE_STRING )        goto BadProp;
                    if( HW_GET_COUNT(type) != 0 ) goto BadProp;
                    ((char **)arr)[n++] = vval;
                    break;
                default :
                    goto BadProp;
                }
            }
            else {
                if( base != HW_TYPE_OBJECT )    goto BadProp;
                obj =  hwFindObject( str->Token );
                if( !obj ) {
                    UngetTok( str, t );
                    obj = hwGetObject( str );
                    if( !obj ) {
                        if( arr ) free( arr );
                        return 0;
                    }
                }
                ((hwObject *)arr)[n++] = obj;
            }
            break;
        default :
            if( base != HW_TYPE_FLOAT ) goto BadProp;
            UngetTok( str, t );
            if( !GetExpression( str, &v ) ) {
                if( arr ) free( arr );
                return 0;
            }
            ((hwFloat *)arr)[n++] = v;
            break;
        }
        if( t != T_RBRACE ) {
            t = GetTok( str );
        }
    } while( t == T_COMMA );

    if( t != T_RBRACE ) {
        Error( str, RightExpected );
    }

    *val = arr;
    return HW_MAKE_TYPE( base, n );

BadProp :
    Error( str, BadProperty );
    if( arr ) free( arr );
    return 0;
}

static hwInt32 GetValue( hwParseStream str, void **val, int *needFree )
{
    hwObject
        Class;
    char
        *SVal;
    int
        i, t;
    hwInt32
        Type;
    void
        *vval;

    *needFree = 0;
    t = GetTok( str );
    switch( t ) {
    case T_LBRACE :
        t = GetTok( str );
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
            Type = hwFindConstant( str->Token, &vval );
            if( Type ) {
                switch( HW_GET_BASE(Type) ) {
                case HW_TYPE_BOOL :
                    if( HW_GET_COUNT(Type) != 1 )       goto BadProp;
                    Type = HW_TYPE_BOOL;
                    break;
                case HW_TYPE_INT :
                    if( HW_GET_COUNT(Type) != 1 )       goto BadProp;
                    Type = HW_TYPE_FLOAT;
                    break;
                case HW_TYPE_FLOAT :
                    if( HW_GET_COUNT(Type) != 1 )       goto BadProp;
                    Type = HW_TYPE_FLOAT;
                    break;
                case HW_TYPE_OBJECT :
                    if( HW_GET_COUNT(Type) != 0 )       goto BadProp;
                    Type = HW_TYPE_OBJECT;
                    break;
                case HW_TYPE_STRING :
                    if( HW_GET_COUNT(Type) != 0 )       goto BadProp;
                    Type = HW_TYPE_STRING;
                    break;
                default : BadProp :
                    Error( str, BadProperty );
                    return 0;
                }
            }
            else {
                Type = HW_TYPE_OBJECT;
            }
            break;
        default :
            Type = HW_TYPE_FLOAT;
            break;
        }
        UngetTok( str, t );
        Type = GetArray( str, Type, val );
        if( !Type )     return 0;
        *needFree = 1;
        break;
    case T_STRING :
        Type = HW_TYPE_STRING;
        *val = str->Token;
        break;
    case T_BOOL :
        Type = HW_TYPE_1B;
        str->IVal = atoi( str->Token );
        *val = &str->IVal;
        break;
    case T_INT :
        Type = HW_TYPE_1I;
        str->IVal = atoi( str->Token );
        *val = &str->IVal;
        break;
    case T_FLOAT :
        Type = HW_TYPE_1F;
        str->FVal = atof( str->Token );
        *val = &str->FVal;
        break;
    case T_NAME :
        Type = hwFindConstant( str->Token, val );
        if( Type )      break;

        Class = hwFindClass( str->Token );
        if( Class ) {
            /* Inline object declaration... */
            UngetTok( str, t );
            str->OVal = hwGetObject( str );
            if( !str->OVal )    return 0;
            Type = HW_TYPE_OBJECT;
            *val = str->OVal;
            break;
        }

        str->OVal = hwFindObject( str->Token );
        if( str->OVal ) {
            Type = HW_TYPE_OBJECT;
            *val = str->OVal;
            break;
        }

        Error( str, BadProperty );
        return 0;

    default :
        UngetTok( str, t );
        if( !GetExpression( str, &str->FVal ) ) return 0;
        /* Expressions are always hwFloats... */
        Type = HW_TYPE_1F;
        *val = &str->FVal;
        break;
    }

    return Type;
}

static hwObject hwGetObject( hwParseStream str )
{
    int
        k, t;
    char
        Prop[MAXTOK];
    hwObject
        Class, Obj;
    hwInt32
        type;
    int
        needFree;
    void
        *val;

    Obj = 0;

    t = GetTok( str );
    if( t != T_NAME ) {
        if( t != T_EOF ) {
            Error( str, NameExpected );
        }
        return 0;
    }

    do {
        Class = hwFindClass( str->Token );
        if( !Class ) {
            (void)strcpy( Prop, str->Token );
            t = GetTok( str );
            if( t == T_EQUALS ) {
                type = GetValue( str, &val, &needFree );
                if( !type )     goto Error;
                hwRegisterConstant( Prop, type, val );
                if( needFree )  free( val );
                t = GetTok( str );
            }
            else {
                Error( str, ClassExpected );
                return 0;
            }
        }
    } while( (t == T_NAME) && !Class );

    if( t != T_NAME ) {
        if( t != T_EOF ) {
            Error( str, NameExpected );
        }
        return 0;
    }

    /* OK, now read the object name, if present */
    t = GetTok( str );
    if( t == T_NAME ) {
        Obj = hwFindObject( str->Token );
        if( Obj ) {
            /* Was a previous forward declaration */
            if( Obj->parent != Class ) {
                Error( str, BadForward );
                return 0;
            }
        }
        else {
            /* Create a new object */
            Obj = (*Class->create)( Class );
            if( !Obj ) {
                Error( str, OutOfMemory );
                return 0;
            }
            Obj->name = malloc( strlen(str->Token) + 1 );
            if( !Obj->name ) {
                Error( str, OutOfMemory );
                goto Error;
            }
            (void)strcpy( (char *)Obj->name, str->Token );
            hwRegisterObject( Obj );
        }
        t = GetTok( str );
    }
    else {
        /* Create a new object */
        Obj = (*Class->create)( Class );
        if( !Obj ) {
            Error( str, OutOfMemory );
            return 0;
        }
        Obj->name = 0;  /* Null out its name */
    }

    if( t != T_LBRACE ) {
        Error( str, BraceExpected );
        goto Error;
    }

    /* Now, read all of the keyword/value pairs */
    do {
        t = GetTok( str );
        (void)strcpy( Prop, str->Token );
        if( t == T_NAME ) {
            /* Get the value of this token */
            k = GetTok( str );
            if( k != T_EQUALS ) {
                Error( str, EqualsExpected );
                goto Error;
            }
            type = GetValue( str, &val, &needFree );
            if( !type )                         goto Error;

            if( strcmp( Prop, hwStrOptFlags ) == 0 ) {
                if( type == HW_TYPE_1I ) {
                    /* Parsed data is never "safe" */
                    *(hwInt32 *)val &= ~HW_OPT_SAFE_USER_DATA;
                }
            }

            Obj->modify( Obj, Prop, type, val );
            if( needFree )      free( val );
        }
        else if( t != T_RBRACE ) {
            Error( str, KeywordExpected );
            goto Error;
        }
    } while( t != T_RBRACE );

    /* Success! */
    return Obj;

Error :
    if( Obj ) {
        (*Obj->destroy)( Obj );
    }
    return 0;
}

typedef struct __hwParseFileStream {
    struct __hwParseStream str;
    FILE *f;
} *hwParseFileStream;

static int hwFileStreamGetChar( hwParseStream str )
{
    hwParseFileStream
        pfs;

    pfs = (hwParseFileStream) str;
    return hwFgetc( pfs->f );
}

hwInt32 hwParseFile( const char *name, hwObject **result )
{
    FILE
        *f;
    hwInt32
        i, n;
    hwObject
        *arr = 0;
    hwInt32
        arrSize = 0;
    hwObject
        obj;
    struct __hwParseFileStream
        pfs;

    (void)strcpy( pfs.str.CurrFile, name );
    pfs.str.CurrLine = 1;
    pfs.str.UngotChar = 0;
    pfs.str.Erred = 0;
    pfs.str.UngotTok = -1;
    pfs.str.getChar = hwFileStreamGetChar;

    f = hwFopen( name, "rb" );
    if( !f )    return 0;

    pfs.f = f;

    /* First, see if it's a binary file */
    if( hwBeginBinary( (hwObjRW)hwFread, f, HW_FILE_READ_HDR, &n ) ) {
        n = hwReadBinary( (hwObjRW)hwFread, f, n, result );
        hwEndBinary( (hwObjRW)hwFread, f );
        (void)hwFclose( f );
        return n;
    }

    /* Nope - rewind and parse */
    (void)hwFrewind( f );

    n = 0;
    do {
        obj = hwGetObject( &pfs.str );
        if( obj ) {
            if( n >= arrSize ) {
                if( arrSize )   arrSize *= 2;
                else            arrSize = 16;
                arr = realloc( arr, arrSize * sizeof(hwObject) );
                if( !arr ) {
                    Error( &pfs.str, OutOfMemory );
                    return 0;
                }
            }
            arr[n++] = obj;
        }
    } while( obj );

    (void)hwFclose( f );

    if( pfs.str.Erred ) return 0;

    *result = arr;
    return n;
}

typedef struct __hwParseArrayStream {
    struct __hwParseStream str;
    char **array;
    int arrLine, arrCol;
} *hwParseArrayStream;

static int hwArrayStreamGetChar( hwParseStream str )
{
    hwParseArrayStream
        pas;
    int
        c;

    pas = (hwParseArrayStream) str;

    if( !pas->array[pas->arrLine] ) {
        /* We're at the end. */
        return EOF;
    }

    /* Fetch the next character */
    c = pas->array[pas->arrLine][pas->arrCol];

    if( c ) {
        /* Advance the character cursor */
        pas->arrCol++;
    }
    else {
        /* End of the current line - return EOL and advance to next line */
        c = '\n';
        pas->arrLine++;
        pas->arrCol = 0;
    }

    return c;
}

hwInt32 hwParseArray( char **array, hwObject **result )
{
    hwInt32
        i, n;
    hwObject
        *arr = 0;
    hwInt32
        arrSize = 0;
    hwObject
        obj;
    struct __hwParseArrayStream
        pas;

    (void)strcpy( pas.str.CurrFile, "hwParseArray" );
    pas.str.CurrLine = 1;
    pas.str.UngotChar = 0;
    pas.str.Erred = 0;
    pas.str.UngotTok = -1;
    pas.str.getChar = hwArrayStreamGetChar;

    pas.array = array;
    pas.arrLine = 0;
    pas.arrCol = 0;

    n = 0;
    do {
        obj = hwGetObject( &pas.str );
        if( obj ) {
            if( n >= arrSize ) {
                if( arrSize )   arrSize *= 2;
                else            arrSize = 16;
                arr = realloc( arr, arrSize * sizeof(hwObject) );
                if( !arr ) {
                    Error( &pas.str, OutOfMemory );
                    return 0;
                }
            }
            arr[n++] = obj;
        }
    } while( obj );

    if( pas.str.Erred ) return 0;

    *result = arr;
    return n;
}

/*******************************************************************************
 * Hash table manipulation
 ******************************************************************************/
typedef struct {
    hwInt32
        type;
    void
        *val;
} hwConstant;

static void /* mutex-protected */
    *classHash,
    *constHash,
    *procHash;
static hwObject /* mutex-protected */
    *classTable;
static hwCallback /* mutex-protected */
    *procTable;
static hwConstant /* mutex-protected */
    *constTable;
static int /* mutex-protected */
    numClass, allocClass,
    numConst, allocConst,
    numProc, allocProc;

void hwRegisterConstant( const char *name, hwInt32 type, void *val )
{
    hwInt32
        size, i, n;
    hwObject
        *arr;
    void
        *tval;

    HW_GLOBAL_LOCK();

    if( !constHash ) {
        /* Gotta initialize */
        constHash = hwCreateHash( 256 );
        if( !constHash )        goto ERROR;
        allocConst = 256;
        constTable = malloc( allocConst * sizeof(hwConstant) );
    }

    if( numConst >= allocConst ) {
        allocConst *= 2;
        constTable = realloc( constTable, allocConst * sizeof(hwConstant) );
    }
    if( !constTable )   goto ERROR;

    switch( type ) {
    case HW_TYPE_OBJECT :
        tval = val;
        ((hwObject)val)->addref( (hwObject)val );
        break;
    case HW_TYPE_STRING :
        tval = malloc( strlen( val ) + 1 );
        if( !tval )     goto ERROR;
        (void)strcpy( tval, val );
        break;
    default :
        switch( HW_GET_BASE(type) ) {
        case HW_TYPE_BYTE :
            size = sizeof(hwInt8);
            break;
        case HW_TYPE_SHORT :
            size = sizeof(hwInt16);
            break;
        case HW_TYPE_INT :
        case HW_TYPE_BOOL :
            size = sizeof(hwInt32);
            break;
        case HW_TYPE_FLOAT :
            size = sizeof(hwFloat);
            break;
        case HW_TYPE_STRING :
            size = sizeof(char *);
            break;
        case HW_TYPE_OBJECT :
            size = sizeof(hwObject);
            arr = val;
            n = HW_GET_COUNT(type);
            for( i = 0; i < n; i++ ) {
                arr[i]->addref( arr[i] );
            }
            break;
        }
        size *= HW_GET_COUNT(type);
        tval = malloc( size );
        if( !tval )     goto ERROR;
        (void)memcpy( tval, val, size );
        break;
    }

    /* Check for multiple definition */
    n = hwLookup( name, constHash );
    if( n >= 0 ) {
        if( constTable[n].type != HW_TYPE_OBJECT ) {
            free( constTable[n].val );
        }
        constTable[n].type = type;
        constTable[n].val = tval;
        goto ERROR;
    }

    (void)hwInsert( name, numConst, constHash );
    constTable[numConst].type = type;
    constTable[numConst].val = tval;
    numConst++;

ERROR :
    HW_GLOBAL_UNLOCK();
}

void hwRegisterClass( hwObject class )
{
    HW_GLOBAL_LOCK();

    if( !classHash ) {
        /* Gotta initialize */
        classHash = hwCreateHash( 256 );
        if( !classHash )        goto ERROR;
        allocClass = 32;
        classTable = malloc( allocClass * sizeof(hwObject) );
    }

    if( numClass >= allocClass ) {
        allocClass *= 2;
        classTable = realloc( classTable, allocClass * sizeof(hwObject) );
    }
    if( !classTable )   goto ERROR;

    (void)hwInsert( class->name, numClass, classHash );
    classTable[numClass++] = class;

ERROR :
    HW_GLOBAL_UNLOCK();
}

void hwRegisterObject( hwObject obj )
{
    HW_USE_CURR_DISP;
    hwDisplayInternal
        intDisp = (hwDisplayInternal)__hwDisp;
    hwNameSpace
        ns = intDisp->nameSpace;

    if (ns->shared) {
        HW_LOCK_MUTEX(ns->mutex);
    }

    if( !ns->objectHash ) {
        /* Gotta initialize */
        ns->objectHash = hwCreateHash( 256 );
        if( !ns->objectHash ) goto ERROR;
        ns->allocObject = 32;
        ns->objectTable = malloc( ns->allocObject * sizeof(hwObject) );
    }

    if( ns->numObject >= ns->allocObject ) {
        ns->allocObject *= 2;
        ns->objectTable = realloc( ns->objectTable,
                                   ns->allocObject * sizeof(hwObject) );
    }
    if( !ns->objectTable ) goto ERROR;

    (void)hwInsert( obj->name, ns->numObject, ns->objectHash );

    obj->addref( obj );
    ns->objectTable[ns->numObject++] = obj;

ERROR :
    if (ns->shared) {
        HW_UNLOCK_MUTEX(ns->mutex);
    }
}

void hwRegisterCallback( const char *name, hwCallback proc  )
{
    HW_GLOBAL_LOCK();

    if( !procHash ) {
        /* Gotta initialize */
        procHash = hwCreateHash( 256 );
        if( !procHash ) goto ERROR;
        allocProc = 32;
        procTable = malloc( allocProc * sizeof(hwCallback) );
    }

    if( numProc >= allocProc ) {
        allocProc *= 2;
        procTable = realloc( procTable, allocProc * sizeof(hwCallback) );
    }
    if( !procTable ) goto ERROR;

    (void)hwInsert( name, numProc, procHash );

    procTable[numProc++] = proc;

ERROR :
    HW_GLOBAL_UNLOCK();
}

void hwUnregisterObjects( void )
{
    int
        i;
    hwObject
        obj;
    HW_USE_CURR_DISP;
    hwDisplayInternal
        intDisp = (hwDisplayInternal)__hwDisp;
    hwNameSpace
        ns = intDisp->nameSpace;

    if (ns->shared) {
        HW_LOCK_MUTEX(ns->mutex);
    }

    for( i = 0; i < ns->numObject; i++ ) {
        obj = ns->objectTable[i];
        obj->destroy( obj );
    }
    ns->numObject = 0;
    if( ns->objectHash ) hwEmptyHash( ns->objectHash );

    if (ns->shared) {
        HW_UNLOCK_MUTEX(ns->mutex);
    }
}

hwObject hwFindClass( const char *name )
{
    static volatile int
        Init = 0;
    int
        doInit = 0,
        n;

    while( !Init ) {
        HW_GLOBAL_LOCK();
        if( !Init ) {
            doInit = 1;
            Init = 1;
        }
        HW_GLOBAL_UNLOCK();
    }

    if( doInit ) {
        /* Oops - gotta insert the builtin classes real fast... */
        hwRegisterClass( hwCamera );
        hwRegisterClass( hwLight );
        hwRegisterClass( hwEnviron );
        hwRegisterClass( hwImage );
        hwRegisterClass( hwTexture );
        hwRegisterClass( hwSphere );
        hwRegisterClass( hwCone );
        hwRegisterClass( hwRing );
        hwRegisterClass( hwTorus );
        hwRegisterClass( hwDisc );
        hwRegisterClass( hwMesh );
        hwRegisterClass( hwPolygon );
        hwRegisterClass( hwPolyline );
        hwRegisterClass( hwBox );
        hwRegisterClass( hwGroup );
        hwRegisterClass( hwSpinner );
        hwRegisterClass( hwFile );
        hwRegisterClass( hwSurface );
        hwRegisterClass( hwOrient );
        hwRegisterClass( hwSurfRev );
        hwRegisterClass( hwText );
        hwRegisterClass( hwText2D );
        hwRegisterClass( hwTimer );
        hwRegisterClass( hwPolymarker );
        hwRegisterClass( hwTriangles );
        hwRegisterClass( hwQuads );
        hwRegisterClass( hwStrip );
        hwRegisterClass( hwSweep );
        hwRegisterClass( hwData );

        hwRegisterClass( hwButton );
        hwRegisterClass( hwFont );
        hwRegisterClass( hwGauge );
        hwRegisterClass( hwJoystick );
        hwRegisterClass( hwLabel );
        hwRegisterClass( hwLayout );
        hwRegisterClass( hwRowCol );
        hwRegisterClass( hwToggle );
    }

    n = hwLookup( name, classHash );
    if( n < 0 ) return 0;

    return classTable[n];
}

hwObject hwFindObject( const char *name )
{
    int
        n;
    HW_USE_CURR_DISP;
    hwDisplayInternal
        intDisp = (hwDisplayInternal)__hwDisp;
    hwNameSpace
        ns = intDisp->nameSpace;
    hwObject
        result = NULL;

    if( ns->shared ) {
        HW_LOCK_MUTEX(ns->mutex);
    }

    if( !ns->objectHash )   goto ERROR;       /* No registry */

    n = hwLookup( name, ns->objectHash );
    if( n < 0 ) goto ERROR;

    result = ns->objectTable[n];

ERROR :
    if( ns->shared ) {
        HW_UNLOCK_MUTEX(ns->mutex);
    }
    return result;
}

hwCallback hwFindCallback( const char *name )
{
    int
        n;

    if( !procHash )     return 0;       /* No registry */

    n = hwLookup( name, procHash );
    if( n < 0 ) return 0;

    return procTable[n];
}

hwInt32 hwFindConstant( const char *name, void **val  )
{
    static volatile int
        Init = 0;
    hwInt32
        ival;
    int
        doInit = 0,
        n;

    while( !Init ) {
        HW_GLOBAL_LOCK();
        if( !Init ) {
            doInit = 1;
            Init = 1;
        }
        HW_GLOBAL_UNLOCK();
    }

    if( doInit ) {
        /* Register all of the built-in constants */
        ival = HW_TM_MODULATE;
        hwRegisterConstant( "HW_TM_MODULATE", HW_TYPE_1I, &ival );
        ival = HW_TM_BUMP;
        hwRegisterConstant( "HW_TM_BUMP", HW_TYPE_1I, &ival );
        ival = HW_TM_RELIEF;
        hwRegisterConstant( "HW_TM_RELIEF", HW_TYPE_1I, &ival );
        ival = HW_TM_GLOSS;
        hwRegisterConstant( "HW_TM_GLOSS", HW_TYPE_1I, &ival );
        ival = HW_TM_SHADOW;
        hwRegisterConstant( "HW_TM_SHADOW", HW_TYPE_1I, &ival );
        ival = HW_TM_SHADOW_VAR;
        hwRegisterConstant( "HW_TM_SHADOW_VAR", HW_TYPE_1I, &ival );
        ival = HW_TM_REPEAT;
        hwRegisterConstant( "HW_TM_REPEAT", HW_TYPE_1I, &ival );
        ival = HW_TM_CLAMP;
        hwRegisterConstant( "HW_TM_CLAMP", HW_TYPE_1I, &ival );
        ival = HW_TM_EXPLICIT;
        hwRegisterConstant( "HW_TM_EXPLICIT", HW_TYPE_1I, &ival );
        ival = HW_TM_PLANAR;
        hwRegisterConstant( "HW_TM_PLANAR", HW_TYPE_1I, &ival );
        ival = HW_TM_SPHERE;
        hwRegisterConstant( "HW_TM_SPHERE", HW_TYPE_1I, &ival );
        ival = HW_TM_CYLINDER;
        hwRegisterConstant( "HW_TM_CYLINDER", HW_TYPE_1I, &ival );
        ival = HW_TM_ENVMAP;
        hwRegisterConstant( "HW_TM_ENVMAP", HW_TYPE_1I, &ival );
        ival = HW_TM_STAGE_0;
        hwRegisterConstant( "HW_TM_STAGE_0", HW_TYPE_1I, &ival );
        ival = HW_TM_COLOR;
        hwRegisterConstant( "HW_TM_COLOR", HW_TYPE_1I, &ival );
        ival = HW_TM_ALPHA;
        hwRegisterConstant( "HW_TM_ALPHA", HW_TYPE_1I, &ival );
        ival = HW_TM_INTENSITY;
        hwRegisterConstant( "HW_TM_INTENSITY", HW_TYPE_1I, &ival );
        ival = HW_TM_HEIGHT;
        hwRegisterConstant( "HW_TM_HEIGHT", HW_TYPE_1I, &ival );
        ival = HW_TM_NORMAL;
        hwRegisterConstant( "HW_TM_NORMAL", HW_TYPE_1I, &ival );
        ival = HW_TM_NORMAL_HEIGHT;
        hwRegisterConstant( "HW_TM_NORMAL_HEIGHT", HW_TYPE_1I, &ival );
        ival = HW_TM_DEPTH;
        hwRegisterConstant( "HW_TM_DEPTH", HW_TYPE_1I, &ival );
        ival = HW_TM_DEPTH_VAR;
        hwRegisterConstant( "HW_TM_DEPTH_VAR", HW_TYPE_1I, &ival );
        ival = HW_TEXT_ALIGN_LEFT;
        hwRegisterConstant( "HW_TEXT_ALIGN_LEFT", HW_TYPE_1I, &ival );
        ival = HW_TEXT_ALIGN_CENTER;
        hwRegisterConstant( "HW_TEXT_ALIGN_CENTER", HW_TYPE_1I, &ival );
        ival = HW_TEXT_ALIGN_RIGHT;
        hwRegisterConstant( "HW_TEXT_ALIGN_RIGHT", HW_TYPE_1I, &ival );
        ival = HW_TEXT_ALIGN_TOP;
        hwRegisterConstant( "HW_TEXT_ALIGN_TOP", HW_TYPE_1I, &ival );
        ival = HW_TEXT_ALIGN_BOTTOM;
        hwRegisterConstant( "HW_TEXT_ALIGN_BOTTOM", HW_TYPE_1I, &ival );
        ival = HW_TEXT_ALIGN_FRONT;
        hwRegisterConstant( "HW_TEXT_ALIGN_FRONT", HW_TYPE_1I, &ival );
        ival = HW_TEXT_ALIGN_BACK;
        hwRegisterConstant( "HW_TEXT_ALIGN_BACK", HW_TYPE_1I, &ival );
        ival = HW_SPIN_CONSTANT;
        hwRegisterConstant( "HW_SPIN_CONSTANT", HW_TYPE_1I, &ival );
        ival = HW_SPIN_LINEAR;
        hwRegisterConstant( "HW_SPIN_LINEAR", HW_TYPE_1I, &ival );
        ival = HW_SPIN_SMOOTH;
        hwRegisterConstant( "HW_SPIN_SMOOTH", HW_TYPE_1I, &ival );
        ival = HW_SPIN_RANDOM;
        hwRegisterConstant( "HW_SPIN_RANDOM", HW_TYPE_1I, &ival );
        ival = HW_OPT_SAFE_USER_DATA;
        hwRegisterConstant( "HW_OPT_SAFE_USER_DATA", HW_TYPE_1I, &ival );
        ival = HW_OPT_CACHE_DATA;
        hwRegisterConstant( "HW_OPT_CACHE_DATA", HW_TYPE_1I, &ival );
        ival = HW_OPT_USE_DL;
        hwRegisterConstant( "HW_OPT_USE_DL", HW_TYPE_1I, &ival );
        ival = HW_OPT_DL_ATTRS;
        hwRegisterConstant( "HW_OPT_DL_ATTRS", HW_TYPE_1I, &ival );
        ival = HW_GUI_REL_RIGHT;
        hwRegisterConstant( "HW_GUI_REL_RIGHT", HW_TYPE_1I, &ival );
        ival = HW_GUI_REL_BOT;
        hwRegisterConstant( "HW_GUI_REL_BOT", HW_TYPE_1I, &ival );
        ival = HW_GUI_REL_WIDTH;
        hwRegisterConstant( "HW_GUI_REL_WIDTH", HW_TYPE_1I, &ival );
        ival = HW_GUI_REL_HEIGHT;
        hwRegisterConstant( "HW_GUI_REL_HEIGHT", HW_TYPE_1I, &ival );
        ival = HW_GUI_FRACTIONAL;
        hwRegisterConstant( "HW_GUI_FRACTIONAL", HW_TYPE_1I, &ival );
        ival = HW_GUI_CENTER;
        hwRegisterConstant( "HW_GUI_CENTER", HW_TYPE_1I, &ival );
        ival = HW_GUI_LABEL_LEFT;
        hwRegisterConstant( "HW_GUI_LABEL_LEFT", HW_TYPE_1I, &ival );
        ival = HW_GUI_LABEL_RIGHT;
        hwRegisterConstant( "HW_GUI_LABEL_RIGHT", HW_TYPE_1I, &ival );
        ival = HW_GUI_LABEL_TOP;
        hwRegisterConstant( "HW_GUI_LABEL_TOP", HW_TYPE_1I, &ival );
        ival = HW_GUI_LABEL_BOTTOM;
        hwRegisterConstant( "HW_GUI_LABEL_BOTTOM", HW_TYPE_1I, &ival );
        ival = HW_GUI_TOGGLE_RIGHT;
        hwRegisterConstant( "HW_GUI_TOGGLE_RIGHT", HW_TYPE_1I, &ival );
        ival = HW_GUI_FONT_FRACTIONAL;
        hwRegisterConstant( "HW_GUI_FONT_FRACTIONAL", HW_TYPE_1I, &ival );
        ival = HW_GUI_FONT_FRACT_W;
        hwRegisterConstant( "HW_GUI_FONT_FRACT_W", HW_TYPE_1I, &ival );
        ival = HW_JOY_DPAD_UP;
        hwRegisterConstant( "HW_JOY_DPAD_UP", HW_TYPE_1I, &ival );
        ival = HW_JOY_DPAD_DOWN;
        hwRegisterConstant( "HW_JOY_DPAD_DOWN", HW_TYPE_1I, &ival );
        ival = HW_JOY_DPAD_LEFT;
        hwRegisterConstant( "HW_JOY_DPAD_LEFT", HW_TYPE_1I, &ival );
        ival = HW_JOY_DPAD_LEFT;
        hwRegisterConstant( "HW_JOY_DPAD_LEFT", HW_TYPE_1I, &ival );
        ival = HW_JOY_DPAD_RIGHT;
        hwRegisterConstant( "HW_JOY_DPAD_RIGHT", HW_TYPE_1I, &ival );
        ival = HW_JOY_START;
        hwRegisterConstant( "HW_JOY_START", HW_TYPE_1I, &ival );
        ival = HW_JOY_SELECT;
        hwRegisterConstant( "HW_JOY_SELECT", HW_TYPE_1I, &ival );
        ival = HW_JOY_THUMB_LEFT;
        hwRegisterConstant( "HW_JOY_THUMB_LEFT", HW_TYPE_1I, &ival );
        ival = HW_JOY_THUMB_RIGHT;
        hwRegisterConstant( "HW_JOY_THUMB_RIGHT", HW_TYPE_1I, &ival );
        ival = HW_JOY_SHOULDER_LEFT;
        hwRegisterConstant( "HW_JOY_SHOULDER_LEFT", HW_TYPE_1I, &ival );
        ival = HW_JOY_SHOULDER_RIGHT;
        hwRegisterConstant( "HW_JOY_SHOULDER_RIGHT", HW_TYPE_1I, &ival );
        ival = HW_JOY_HOME;
        hwRegisterConstant( "HW_JOY_HOME", HW_TYPE_1I, &ival );
        ival = HW_JOY_A;
        hwRegisterConstant( "HW_JOY_A", HW_TYPE_1I, &ival );
        ival = HW_JOY_B;
        hwRegisterConstant( "HW_JOY_B", HW_TYPE_1I, &ival );
        ival = HW_JOY_X;
        hwRegisterConstant( "HW_JOY_X", HW_TYPE_1I, &ival );
        ival = HW_JOY_Y;
        hwRegisterConstant( "HW_JOY_Y", HW_TYPE_1I, &ival );
        ival = HW_JOY_TRIGGER_LEFT;
        hwRegisterConstant( "HW_JOY_TRIGGER_LEFT", HW_TYPE_1I, &ival );
        ival = HW_JOY_TRIGGER_RIGHT;
        hwRegisterConstant( "HW_JOY_TRIGGER_RIGHT", HW_TYPE_1I, &ival );
        ival = HW_AXIS_LEFT_X;
        hwRegisterConstant( "HW_AXIS_LEFT_X", HW_TYPE_1I, &ival );
        ival = HW_AXIS_LEFT_Y;
        hwRegisterConstant( "HW_AXIS_LEFT_Y", HW_TYPE_1I, &ival );
        ival = HW_AXIS_RIGHT_X;
        hwRegisterConstant( "HW_AXIS_RIGHT_X", HW_TYPE_1I, &ival );
        ival = HW_AXIS_RIGHT_Y;
        hwRegisterConstant( "HW_AXIS_RIGHT_Y", HW_TYPE_1I, &ival );
        ival = HW_AXIS_LTRIGGER;
        hwRegisterConstant( "HW_AXIS_LTRIGGER", HW_TYPE_1I, &ival );
        ival = HW_AXIS_RTRIGGER;
        hwRegisterConstant( "HW_AXIS_RTRIGGER", HW_TYPE_1I, &ival );
        ival = HW_AXIS_HAT_X;
        hwRegisterConstant( "HW_AXIS_HAT_X", HW_TYPE_1I, &ival );
        ival = HW_AXIS_HAT_Y;
        hwRegisterConstant( "HW_AXIS_HAT_Y", HW_TYPE_1I, &ival );
    }

    if( !constHash )    return 0;       /* No registry */
    n = hwLookup( name, constHash );
    if( n < 0 ) return 0;

    *val = constTable[n].val;
    return constTable[n].type;
}

/*** EOF hwParse.c ***/
