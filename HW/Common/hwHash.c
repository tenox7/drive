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

typedef struct hashEntryStruct {
    int
        val;
    struct hashEntryStruct
        *next;
    char
        name[1];
} hashEntry;

typedef struct {
    int
        numEntries;
    hashEntry
        *hashTable[1];
} hashTable;

static unsigned long Hash( const char *s )
{
    unsigned long
        Res, t;

    Res = 0;
    for( /* NO INIT */; *s; s++ ) {
        Res = (Res << 4) + *s;
        t = Res & 0xF0000000;
        if( t ) {
            Res ^= (t >> 24);
            Res ^= t;
        }
    }
    return Res;
}

void *hwCreateHash( int size )
{
    hashTable
        *result;
    int
        i;

    result = malloc( sizeof(hashTable) + (size-1)*sizeof(hashEntry *) );
    if( !result )       return 0;

    result->numEntries = size;
    for( i = 0; i < size; i++ ) {
        result->hashTable[i] = 0;
    }

    return result;
}

void hwEmptyHash( void *table )
{
    hashTable
        *tab = table;
    hashEntry
        *curr, *next;
    int
        n;

    for( n = 0; n < tab->numEntries; n++ ) {
        for( curr = tab->hashTable[n]; curr; curr = next ) {
            next = curr->next;
            free( curr );
        }
        tab->hashTable[n] = 0;
    }
}

int hwInsert( const char *name, int value, void *table )
{
    hashTable
        *tab = table;
    hashEntry
        *curr;
    int
        n;

    curr = malloc( sizeof(hashEntry) + strlen(name) );
    if( !curr ) return 0;
    (void)strcpy( curr->name, name );
    curr->val = value;

    n = Hash( name ) % tab->numEntries;

    curr->next = tab->hashTable[n];
    tab->hashTable[n] = curr;

    return 1;
}

int hwLookup( const char *name, void *table )
{
    hashTable
        *tab = table;
    hashEntry
        *curr;
    int
        n;

    n = Hash( name ) % tab->numEntries;
    for( curr = tab->hashTable[n]; curr; curr = curr->next ) {
        if( strcmp( curr->name, name ) == 0 ) {
            return curr->val;
        }
    }

    return -1;
}

int hwFindSym( char name[256], hwInt32 value, void *table )
{
    hashTable
        *tab = table;
    hashEntry
        *curr;
    int
        n;

    /* Too bad there's not a better way :-( */
    for( n = 0; n < tab->numEntries; n++ ) {
        for( curr = tab->hashTable[n]; curr; curr = curr->next ) {
            if( curr->val == value ) {
                (void)strncpy( name, curr->name, 255 );
                name[255] = 0;
                return 1;
            }
        }
    }

    return -1;
}

void hwDestroyHash( void *table )
{
    hashTable
        *tab = table;
    hashEntry
        *curr, *next;
    int
        i, n;

    n = tab->numEntries;
    for( i = 0; i < n; i++ ) {
        for( curr = tab->hashTable[i]; curr; curr = next ) {
            next = curr->next;
            free( curr );
        }
    }
    free( table );
}

/*** EOF hwHash.c ***/
