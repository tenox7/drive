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

#include "convert.h"
#include <ctype.h>
#include <setjmp.h>

enum {
        /* Tokens */
        T_LPAR = 1,     T_RPAR,         T_LBRACE,       T_RBRACE,
        T_COMMA,        T_STRING,       T_EQUALS,       T_EOF,
        T_NUMBER,       T_NAME,         T_LBRAK,        T_RBRAK,
        T_PLUS,         T_MINUS,        T_MUL,          T_DIV,

        /* Keywords */
        T_PROPERTY,     T_BOOL
};

static struct {
    char
        *SymName;
    int
        SymType,
        SymVal,
        DataType;
}
    SymTab[] = {
        { "Color",      T_PROPERTY,     PR_COLOR },
        { "Transparency",T_PROPERTY,    PR_TRANSPARENCY },
        { "SpecColor",  T_PROPERTY,     PR_SPEC_COLOR },
        { "Shininess",  T_PROPERTY,     PR_SHININESS },
        { "Rotate",     T_PROPERTY,     PR_ROTPOS },
        { "Scale",      T_PROPERTY,     PR_SCALE },
        { "Position",   T_PROPERTY,     PR_POSITION },
        { "GraphType",  T_PROPERTY,     PR_GRAPHTYPE },
        { "StoreGraphic",T_PROPERTY,    PR_WHICHGRAPH },
        { "GraphN",     T_PROPERTY,     PR_GRAPHN },
        { "GraphM",     T_PROPERTY,     PR_GRAPHM },
        { "HasNormals", T_PROPERTY,     PR_NORMALS },
        { "HasRGB",     T_PROPERTY,     PR_RGB },
        { "HasUV",      T_PROPERTY,     PR_UV },
        { "Convex",     T_PROPERTY,     PR_CONVEX },
        { "Backface",   T_PROPERTY,     PR_BACKFACE },
        { "TwoSided",   T_PROPERTY,     PR_TWOSIDED },
        { "Uncolored",  T_PROPERTY,     PR_UNCOLORED },
        { "Textured",   T_PROPERTY,     PR_TEXTURED },
        { "Texture",    T_PROPERTY,     PR_TEXTURE },
        { "Bright",     T_PROPERTY,     PR_BRIGHT },
        { "Shiny",      T_PROPERTY,     PR_SPECULAR },
        { "Data",       T_PROPERTY,     PR_DATA },
        { "Visible",    T_PROPERTY,     PR_VISIBLE },
        { "Collides",   T_PROPERTY,     PR_COLLIDES },
        { "Volume",     T_PROPERTY,     PR_VOLUME },
        { "Animate",    T_PROPERTY,     PR_ANIMATE },
        { "TexGenSphere",T_PROPERTY,    PR_TEXGENSPHERE },
        { "TexGenPlane",T_PROPERTY,     PR_TEXGENPLANE },
        { "TexGenCyl",  T_PROPERTY,     PR_TEXGENCYL },
        { "Wireframe",  T_PROPERTY,     PR_WIREFRAME },
        { "True",       T_BOOL,         1 },
        { "False",      T_BOOL,         0 },

        { 0, 0 }
    };
static char
    Unterminated[] = "Unterminated string",
    BadForward[] = "Forward declaration does not match object",
    BadExpr[] = "Illegal expression",
    EqualsExpected[] = "'=' expected",
    BadChar[] = "Illegal character",
    LeftExpected[] = "'(' expected",
    RightExpected[] = "')' expected",
    CommaExpected[] = "',' expected",
    NumExpected[] = "Number expected",
    NameExpected[] = "Name expected",
    BraceExpected[] = "'{' expected",
    StringExpected[] = "String expected",
    KeywordExpected[] = "Keyword expected",
    TooLong[] = "Token too long",
    BadType[] = "Illegal type",
    BadObject[] = "Undeclared object",
    BadProperty[] = "Illegal property",
    ArrayTooBig[] = "Array would be too big";
static int
    CurrLine = 1,
    Erred = 0,
    InExpr = 0,
    UngotTok = -1,
    UngotChar = 0;
static char
    CurrFile[1024];

#define MAXTOK  4096
#define STATIC_ARRAY_COUNT      40000

static jmp_buf
    ExprErr;
static char
    Token[MAXTOK];
static int
    TokType,
    TokVal;

/* Forward declarations... */

static int GetExpression( FILE *, double * );
static double GetExpr( FILE * );

static void Error( char *Msg )
{
    (void)fprintf( stderr, "File %s, line %d: %s\n", CurrFile, CurrLine, Msg );
    Erred = 1;
}

int StrEQ( const char *s1, const char *s2 )
{
    int
        c1, c2;

    do {
        c1 = *s1++;
        c2 = *s2++;
        if( isupper(c1) )       c1 = tolower(c1);
        if( isupper(c2) )       c2 = tolower(c2);
    } while( c1 && c2 && (c1 == c2) );

    return c1 == c2;
}

static int ParseDirective( FILE *f )
{
    int
        c, n;
    char
        *s;

    /* Skip initial spaces */
    do {
        c = getc( f );
    } while( c == ' ' );
    if( c == EOF )      return EOF;

    if( isdigit( c ) ) {
        /* Aha!  It's going to be a line number directive */

        /* Get the number */
        n = 0;
        while( isdigit( c ) ) {
            n = n*10 + (c - '0');
            c = getc( f );
        }
        if( c == EOF )  return EOF;

        CurrLine = n - 1;

        /* Skip the space inbetween */
        while( c == ' ' ) {
            c = getc( f );
        }
        if( c == EOF )  return EOF;

        /* Get the filename */
        if( c == '\"' ) {
            s = CurrFile;
            do {
                c = getc( f );
                *s++ = c;
            } while( (c != EOF) && (c != '\"') );
            *--s = 0;
            if( c != EOF ) {
                c = getc( f );
            }
        }
    }

    /* Ignore the rest of the line */
    while( (c != EOF) && (c != '\n') ) {
        c = getc( f );
    }
    if( c == '\n' ) {
        CurrLine++;
    }

    return c;
}

static int SkipWhite( FILE *f )
{
    int
        c;

    if( UngotChar ) {
         c = UngotChar;
         UngotChar = 0;
         return c;
    }

    do {
        c = getc( f );
        if( c == '\n' ) {
            CurrLine++;
        }
        else if( c == '#' ) {
            c = ParseDirective( f );
        }
    } while( (c != EOF) && isspace( c ) );

    return c;
}

static int GetChar( FILE *f )
{
    int
        c;

    if( UngotChar ) {
         c = UngotChar;
         UngotChar = 0;
         return c;
    }

    c = getc( f );
    if( c == '\n' ) {
        CurrLine++;
    }
    return c;
}

static void UngetChar( int c )
{
    if( !isspace( c ) ) {
        UngotChar = c;
    }
}

static void UngetTok( int t )
{
    UngotTok = t;
}


static int GetTok( FILE *f )
{
    int
        i, c;

    if( UngotTok >= 0 ) {
        i = UngotTok;
        UngotTok = -1;
        return i;
    }

    c = SkipWhite( f );
    switch( c ) {
    case '(' :  return T_LPAR;
    case ')' :  return T_RPAR;
    case '{' :  return T_LBRACE;
    case '}' :  return T_RBRACE;
    case ',' :  return T_COMMA;
    case '=' :  return T_EQUALS;
    case EOF :  return T_EOF;
    case '[' :  return T_LBRAK;
    case ']' :  return T_RBRAK;
    case '*' :  return T_MUL;
    case '/' :  return T_DIV;

    case '\"' :
        i = 0;
        do {
            c = GetChar( f );
            if( c == EOF ) {
                Error( Unterminated );
                return 0;
            }
            if( i >= MAXTOK ) {
                Error( TooLong );
                return 0;
            }
            Token[i++] = c;
        } while( c != '\"' );
        Token[i-1] = 0;
        return T_STRING;

    default :
        if( ((c == '+') || (c == '-')) && InExpr ) {
            return (c == '+') ? T_PLUS : T_MINUS;
        }
        else if( isdigit(c) || (c == '.') || (c == '+') || (c == '-') ) {
            i = 0;
            Token[i++] = c;
            do {
                c = GetChar( f );
                if( i >= MAXTOK ) {
                    Error( TooLong );
                    return 0;
                }
                Token[i++] = c;
                if( (c == '+') || (c == '-') ) {
                    if( (i < 2) || (toupper(Token[i-2]) != 'E') ) {
                        break;
                    }
                }
            } while(isdigit(c)||(c=='.')||(c=='+')||(c == '-')||(c == 'e')||
                                (c=='E'));
            Token[--i] = 0;
            UngetChar( c );
            if( StrEQ( Token, "+" ) )                   return T_PLUS;
            else if( StrEQ( Token, "-" ) )              return T_MINUS;
            else                                        return T_NUMBER;
        }
        else if( isalpha(c) ) {
            i = 0;
            Token[i++] = c;
            do {
                c = GetChar( f );
                if( i >= MAXTOK ) {
                    Error( TooLong );
                    return 0;
                }
                Token[i++] = c;
            } while( isalpha(c) || isdigit(c) || (c == '_') );
            Token[--i] = 0;
            UngetChar( c );
            for( i = 0; SymTab[i].SymName; i++ ) {
                if( StrEQ( SymTab[i].SymName, Token ) ) {
                    TokType = SymTab[i].SymType;
                    TokVal = SymTab[i].SymVal;
                    return TokType;
                }
            }
            return T_NAME;
        }
        else {
            Error( BadChar );
            return 0;
        }
    }
}


static int GetNumber( FILE *f, double *Val )
{
    int
        t;

    t = GetTok( f );
    if( t == T_BOOL ) {
        *Val = TokVal;
    }
    else if( t == T_NUMBER ) {
        *Val = atof( Token );
    }
    else if( t == T_LBRAK ) {
        if( !GetExpression( f, Val ) )  return 0;
    }
    else {
        Error( NumExpected );
        return 0;
    }
    return 1;
}

/* Our grammar for expressions:
 *
 *      Expr -> Expr + Term
 *      Expr -> Term
 *      Term -> Term * Fact
 *      Term -> Fact
 *      Fact -> -Fact
 *      Fact -> T_NUMBER
 *      Fact -> ( Expr )
 */

static double GetFact( FILE *f )
{
    int
        t;
    double
        Res;

    t = GetTok( f );
    if( t == T_LPAR ) {
        Res = GetExpr( f );
        t = GetTok( f );
        if( t != T_RPAR ) {
            longjmp( ExprErr, 1 );
        }
    }
    else if( t == T_NUMBER ) {
        return atof( Token );
    }
    else if( t == T_MINUS ) {
        return -GetFact( f );
    }
    else {
        longjmp( ExprErr, 1 );
    }
}

static double GetTerm( FILE *f )
{
    double
        Res;
    int
        t;

    Res = GetFact( f );
    t = GetTok( f );
    while( (t == T_MUL) || (t == T_DIV) ) {
        if( t == T_MUL ) {
            Res *= GetFact( f );
        }
        else {
            Res /= GetFact( f );
        }
        t = GetTok( f );
    }
    UngetTok( t );
    return Res;
}

static double GetExpr( FILE *f )
{
    double
        Res;
    int
        t;

    Res = GetTerm( f );
    t = GetTok( f );
    while( (t == T_PLUS) || (t == T_MINUS) ) {
        if( t == T_PLUS ) {
            Res += GetTerm( f );
        }
        else {
            Res -= GetTerm( f );
        }
        t = GetTok( f );
    }
    UngetTok( t );
    return Res;
}

static int GetExpression( FILE *f, double *Res )
{
    int
        t;

    if( setjmp( ExprErr ) != 0 ) {
        Error( BadExpr );
        InExpr = 0;
        return 0;
    }

    InExpr = 1;
    *Res = GetExpr( f );
    InExpr = 0;

    t = GetTok( f );
    if( t != T_RBRAK ) {
        Error( BadExpr );
        return 0;
    }

    return 1;
}


struct Object *FindObject( char *Name )
{
    struct Object
        *Curr;

    for( Curr = Objects; Curr; Curr = Curr->Next ) {
        if( StrEQ( Curr->Name, Name ) ) {
            return Curr;
        }
    }

    for( Curr = Textures; Curr; Curr = Curr->Next ) {
        if( StrEQ( Curr->Name, Name ) ) {
            return Curr;
        }
    }
    return 0;
}


static int GetObjArray( FILE *f, struct Object **Array, int Count )
{
    int
        i, t;
    struct Object
        *OVal;

    t = GetTok( f );
    if( t != T_LPAR ) {
        Error( LeftExpected );
        return 0;
    }
    i = 0;
    do {
        t = GetTok( f );
        if( t != T_NAME ) {
            Error( NameExpected );
            return 0;
        }
        OVal = FindObject( Token );
        if( !OVal ) {
            Error( BadObject );
            return 0;
        }
        Array[i++] = OVal;
        t = GetTok( f );
    } while( (i < Count) && (t == T_COMMA) );
    if( t != T_RPAR ) {
        Error( RightExpected );
        return 0;
    }
    while( i < Count ) {
        Array[i++] = 0;
    }
    return 1;
}


static int GetArray( FILE *f, double *Array, int Count )
{
    int
        i, t;
    double
        d;

    t = GetTok( f );
    if( t != T_LPAR ) {
        Error( LeftExpected );
        return 0;
    }
    i = 0;
    do {
        t = GetTok( f );
        if( t == T_LBRAK ) {
            if( !GetExpression( f, &d ) )       return 0;
            Array[i++] = d;
        }
        else if( t == T_NUMBER ) {
            Array[i++] = atof( Token );
        }
        else {
            Error( NumExpected );
            return 0;
        }
        t = GetTok( f );
    } while( (i < Count) && (t == T_COMMA) );
    if( t != T_RPAR ) {
        Error( RightExpected );
        return 0;
    }
    while( i < Count ) {
        Array[i++] = 0.;
    }
    return 1;
}

static int GetValue( FILE *f, struct Object *Obj, ObjMethod Prop, int Type )
{
    struct Object
        *OVal;
    double
        Val;
    float
        FVal;
    int
        IVal;
    char
        *SVal;
    void
        *VVal;
    int
        i, t;
    static double
        Array[STATIC_ARRAY_COUNT];

    switch( Type & COMPOUND_MASK ) {
    case CT_SCALAR :
        switch( Type & BASE_MASK ) {
        case BT_VOID :
            IVal = 0;
            VVal = (void *)&IVal;
            break;
        case BT_INT :
            if( !GetNumber( f, &Val ) ) return 0;
            IVal = (int)(Val + 0.5);
            VVal = (void *)&IVal;
            break;
        case BT_FLOAT :
            if( !GetNumber( f, &Val ) ) return 0;
            FVal = Val;
            VVal = (void *)&FVal;
            break;
        case BT_DOUBLE :
            if( !GetNumber( f, &Val ) ) return 0;
            VVal = (void *)&Val;
            break;
        case BT_STRING :
            t = GetTok( f );
            if( t != T_STRING ) {
                Error( StringExpected );
                return 0;
            }
            SVal = malloc( strlen( Token ) + 1 );
            if( !SVal ) OutOfMemory();
            (void)strcpy( SVal, Token );
            VVal = (void *)SVal;
            break;
        case BT_OBJECT :
            t = GetTok( f );
            if( t != T_NAME ) {
                Error( NameExpected );
                return 0;
            }
            OVal = FindObject( Token );
            if( !OVal ) {
                Error( BadObject );
                return 0;
            }
            VVal = (void *)OVal;
            break;
        default :
            Error( BadType );
            return 0;
        }
        break;
    case CT_ARRAY :
        if( (Type & COUNT_MASK) > STATIC_ARRAY_COUNT ) {
            Error( ArrayTooBig );
            return 0;
        }
        switch( Type & BASE_MASK ) {
        case BT_INT :
            if( !GetArray( f, Array, Type & COUNT_MASK ) )      return 0;
            for( i = 0; i < (Type & COUNT_MASK); i++ ) {
                ((int *)Array)[i] = (int)(Array[i] + 0.5);
            }
            break;
        case BT_FLOAT :
            if( !GetArray( f, Array, Type & COUNT_MASK ) )      return 0;
            for( i = 0; i < (Type & COUNT_MASK); i++ ) {
                ((float *)Array)[i] = (float)Array[i];
            }
            break;
        case BT_DOUBLE :
            if( !GetArray( f, Array, Type & COUNT_MASK ) )      return 0;
            break;
        case BT_OBJECT :
            if(!GetObjArray(f, (struct Object **)Array, Type&COUNT_MASK)) {
                return 0;
            }
            break;
        default :
            Error( BadType );
            return 0;
        }
        VVal = (void *)Array;
        break;
    default :
        Error( BadType );
        return 0;
    }

    if( !PropVal( Obj, Prop, VVal ) ) {
        Error( BadType );
        return 0;
    }
    return 1;
}

static int GetObject( FILE *f )
{
    int
        k, t, Type;
    ObjMethod
        Prop;
    struct Object
        *Class, *Obj;

    Obj = 0;

    t = GetTok( f );
    if( t != T_NAME ) {
        if( t != T_EOF ) {
            Error( NameExpected );
        }
        return 0;
    }

    if( !StrEQ( Token, "GraphicClass" ) ) {
        do {
            t = GetTok( f );
        } while( (t != T_RBRACE) && (t != T_EOF) );
        return 1;
    }

    /* OK, now read the object name, if present */
    t = GetTok( f );
    if( t != T_NAME ) {
        Error( NameExpected );
        return 0;
    }

    /* Create a new object */
    Obj = malloc( sizeof(struct Object) );
    if( !Obj )  OutOfMemory();
    Obj->Name = malloc( strlen(Token) + 1 );
    if( !Obj->Name )    OutOfMemory();
    (void)strcpy( Obj->Name, Token );
    Obj->Next = 0;
    Obj->Graphic = 0;
    Obj->GraphTail = 0;
    Obj->Volume = 0;


    t = GetTok( f );
    if( t != T_LBRACE ) {
        Error( BraceExpected );
        goto Error;
    }

    /* Now, read all of the keyword/value pairs */
    do {
        t = GetTok( f );
        Prop = TokVal;
        if( t == T_PROPERTY ) {
            /* Figure out what type this property takes */
            Type = PropType( Prop );
            if( Type < 0 ) {
                Error( BadType );
                goto Error;
            }

            /* Get the value of this token (if it's not void) */
            if( Type != (BT_VOID|CT_SCALAR) ) {
                k = GetTok( f );
                if( k != T_EQUALS ) {
                    Error( EqualsExpected );
                    goto Error;
                }
            }
            if( !GetValue( f, Obj, Prop, Type ) )       goto Error;
        }
        else if( t != T_RBRACE ) {
            Error( KeywordExpected );
            goto Error;
        }
    } while( t != T_RBRACE );

    /* Insert it in the right list */
    if( Obj->Graphic->Type == G_TEXTURE ) {
        if( TextureTail ) {
            TextureTail->Next = Obj;
        }
        else {
            Textures = Obj;
        }
        TextureTail = Obj;
    }
    else {
        if( ObjTail ) {
            ObjTail->Next = Obj;
        }
        else {
            Objects = Obj;
        }
        ObjTail = Obj;
    }

    /* Got an object! */
    return 1;

Error :
    return 0;
}

int ParseFile( char *Name )
{
    FILE
        *f;
    static char
        Cmd[2048];
    char
        *Dir;
    int
        i, Slashed = 0;

    (void)strcpy( CurrFile, Name );
    CurrLine = 1;
    UngotChar = 0;
    Erred = 0;
    Dir = strrchr( Name, '/' );
    if( Dir ) {
        *Dir++ = 0;
        Slashed = 1;
    }

    (void)strcpy( Cmd, "/lib/cpp -I" );
    if( Slashed ) {
        (void)strcat( Cmd, Name );
        *--Dir = '/';
    }
    else {
        (void)strcat( Cmd, "." );
    }

    (void)strcat( Cmd, " " );
    (void)strcat( Cmd, Name );

    f = popen( Cmd, "r" );
    if( !f )    return 0;

    while( GetObject( f ) ) {
        /* NOTHING */
    }

    pclose( f );
    return !Erred;
}


/*** EOF parse.c ***/
