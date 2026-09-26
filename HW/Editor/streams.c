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

#ifndef WIN32
#include <fcntl.h>
#endif

#include <string.h>
#include "hwedit.h"

/* Character stream function forward refs */
static int charReady( CmdStream str );
static int charGetString( CmdStream str, char *, int );
static int charPutString( CmdStream str, char * );
static void charDestroy( CmdStream str );

typedef struct {
    struct _cmd_stream hdr;
    char *ptr;
    char buff[1];
} *CharStream;

CmdStream createCharStream( char *buf )
{
    CharStream
        result;

    result = malloc( sizeof(*result) + strlen(buf) + 1 );
    if( !result ) return 0;
    result->hdr.ready = charReady;
    result->hdr.getString = charGetString;
    result->hdr.putString = charPutString;
    result->hdr.destroy = charDestroy;
    (void)strcpy( result->buff, buf );
    result->ptr = result->buff;
    return (CmdStream)result;
}

/* File stream function forward refs */
static int fileReady( CmdStream str );
static int fileGetString( CmdStream str, char *, int );
static int filePutString( CmdStream str, char * );
static void fileDestroy( CmdStream str );

typedef struct {
    struct _cmd_stream hdr;
    FILE *in, *out;
} *FileStream;

CmdStream createFileStream( FILE *inbuf, FILE *outbuf )
{
    FileStream
        result;

    result = malloc( sizeof(*result) );
    if( !result ) return 0;
    result->hdr.ready = fileReady;
    result->hdr.getString = fileGetString;
    result->hdr.putString = filePutString;
    result->hdr.destroy = fileDestroy;
    result->in = inbuf;
    result->out = outbuf;
    return (CmdStream)result;
}

/* Console stream function forward refs */
static int consReady( CmdStream str );
static int consGetString( CmdStream str, char *, int );
static int consPutString( CmdStream str, char * );
static void consDestroy( CmdStream str );

typedef struct {
    struct _cmd_stream hdr;
#ifdef WIN32
    HANDLE hConsole;
#endif
    char buff[BSIZ];
    int sockIdx;
    int ready;
} *ConsStream;

static IPC_SOCK
    initialized = 0,
    serverSocket;

CmdStream createConsStream( int sockIdx )
{
    ConsStream
        result;
#ifdef WIN32
    DWORD
        mode;
#endif
    result = malloc( sizeof(*result) );
    if( !result ) return 0;
    result->hdr.ready = consReady;
    result->hdr.getString = consGetString;
    result->hdr.putString = consPutString;
    result->hdr.destroy = consDestroy;
    result->buff[0] = 0;
    result->sockIdx = sockIdx;
    result->ready = 0;

    return (CmdStream)result;
}

static int charReady( CmdStream str )
{
    CharStream
        cs;

    cs = (CharStream)str;
    return *cs->ptr ? 1 : 0;
}

static int charGetString( CmdStream str, char *buff, int bufflen )
{
    CharStream
        cs;
    int
        n;

    cs = (CharStream)str;
    n = strlen( cs->ptr );
    if( n < (bufflen-1) ) {
        (void)strcpy( buff, cs->ptr );
        cs->ptr += n;
        return 1;
    }
    else if( n ) {
        (void)strncpy( buff, cs->ptr, bufflen-1 );
        buff[bufflen-1] = 0;
        cs->ptr += bufflen-1;
        return 1;
    }
    else {
        return 0;
    }
}

static int charPutString( CmdStream str, char *buff )
{
    return 0;   /* Not supported */
}

static void charDestroy( CmdStream str )
{
    free( str );
}

static int fileReady( CmdStream str )
{
    FileStream
        fs;

    fs = (FileStream)str;
    return !feof( fs->in );
}

static int fileGetString( CmdStream str, char *buf, int buflen )
{
    FileStream
        fs;
    int
        c;

    fs = (FileStream)str;

    if( feof( fs->in ) )        return 0;
    if( buflen < 2 )            return 0;

    do {
        c = getc( fs->in );
        if( c != EOF ) {
            *buf++ = c;
            buflen--;
        }
    } while( (c != '\n') && (c != EOF) && (buflen > 1) );
    if( c == '\n' ) buf--;
    *buf = 0;

    return 1;
}

static int filePutString( CmdStream str, char *buf )
{
    FileStream
        fs;

    fs = (FileStream)str;
    if( fputs( buf, fs->out ) < 0 ) return 0;
    return 1;
}

static void fileDestroy( CmdStream str )
{
    free( str );
}

static int consReady( CmdStream str )
{
    ConsStream
        cs;
    int
        n;

    cs = (ConsStream)str;
    if( !cs->ready ) {
        (void)IPCWait( edit.socks[cs->sockIdx], 500 );
        n = GetMsg( edit.socks[cs->sockIdx], cs->buff );
        if (n > 0) {
            cs->buff[n] = 0;
            cs->ready = 1;
        }
    }
    return cs->ready;
}

static int consGetString( CmdStream str, char *buff, int bufflen )
{
    ConsStream
        cs;
    int
        n;

    cs = (ConsStream)str;
    if( !cs->ready ) {
        return 0;
    }

    n = strlen( cs->buff );
    if( n >= (bufflen-1) ) n = bufflen - 1;
    (void)strncpy( buff, cs->buff, n );
    buff[n] = 0;
    cs->ready = 0;
    return 1;
}

static int consPutString( CmdStream str, char *buff )
{
    ConsStream
        cs;

    cs = (ConsStream)str;
    (void)PutMsg(buff, edit.socks[cs->sockIdx]);
    return 1;
}

static void consDestroy( CmdStream str )
{
    free( str );
}
