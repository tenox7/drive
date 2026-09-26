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


/* For programs that do a lot of small mallocs and not many frees, this
 * method is about six times faster.  It does its mallocs in large chunks
 * and apportions out the memory as it's asked for.
 *
 * Note that fastfree takes a size parameter as well as an address.
 *
 */


#include <stdio.h>
#include <stdlib.h>

#define BLOCKSIZE	(1<<17)	/* 128K */

typedef struct _largeblock {
    unsigned char *addr;	
    unsigned int size;
    struct _largeblock *last,*next;
} LARGEBLOCK;
static LARGEBLOCK *largeblock_head = NULL;


typedef struct _freeblock {
    unsigned char *addr;
    unsigned int size;
    struct _freeblock *next,*last;
} FREEBLOCK;

typedef struct _stdblock {
    unsigned int largest_free_block_size;
    unsigned int num_free_blocks;
    FREEBLOCK *freeblock;
    struct _stdblock *last,*next;

    unsigned char data[BLOCKSIZE];
} STDBLOCK;
static STDBLOCK *stdblock_head = NULL;



/**************************** FASTMALLOC **************************************/
unsigned char *fastmalloc(
    unsigned int size)
{
    unsigned char *addr;
    unsigned int oldsize;
    STDBLOCK *block;
    FREEBLOCK *freeblock,*stopblock;

    /* Longword align, please. */
    if (size & 0x03) size = (size+4) & ~0x03;

    /* If too big to fit a standard block. */
    if (size > BLOCKSIZE) {
	LARGEBLOCK *largeblock;

	if ((largeblock = (LARGEBLOCK *) malloc(sizeof(LARGEBLOCK))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    exit(1);
	}
	if ((largeblock->addr = (unsigned char *) malloc(size)) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    exit(1);
	}
	largeblock->size = size;
	if (largeblock_head) largeblock_head->last = largeblock;
	largeblock->next = largeblock_head;
	largeblock->last = NULL;
	largeblock_head = largeblock;
	return(largeblock->addr);
    }

    /* Search all the existing blocks for space. */
    block = stdblock_head;
    for (block=stdblock_head; block != NULL; block = block->next) {
	if (size > block->largest_free_block_size) continue;
	stopblock = block->freeblock + block->num_free_blocks;
	for (freeblock = block->freeblock; freeblock < stopblock; ++freeblock) {
	    if (freeblock->size >= size) {
		/* Got one!  Use it! */
		addr = freeblock->addr;
		oldsize = freeblock->size;
		if ((freeblock->size -= size) > 1) {	
		    /* just reduce the block */
		    freeblock->addr += size;
		}
		else {
		    /* remove the freeblock */
		    if (freeblock->last == NULL)
			block->freeblock = freeblock->next;
		    else (freeblock->last)->next = freeblock->next;
		    free(freeblock);
		    --block->num_free_blocks;
		}
		if (oldsize == block->largest_free_block_size) {
		    /* Search list for largest block */
		    block->largest_free_block_size = 0;
		    stopblock = block->freeblock + block->num_free_blocks;
		    for (freeblock = block->freeblock;
			    freeblock < stopblock;
			    ++freeblock) {
			if (freeblock->size > block->largest_free_block_size) {
			    block->largest_free_block_size = freeblock->size;
			}
		    }
		}
		return(addr);
	    }
	}
    }
    /* else */

    /* Didn't find one -- make one now. */
    if ((block = (STDBLOCK *) malloc(sizeof(STDBLOCK))) == NULL) {
	return(NULL);
    }
    if ((block->freeblock = (FREEBLOCK *) malloc(sizeof(FREEBLOCK))) == NULL) {
	free(block);
	return(NULL);
    }
    block->last = NULL;
    block->next = stdblock_head;
    if (stdblock_head != NULL) stdblock_head->last = block;
    stdblock_head = block;
    block->largest_free_block_size = BLOCKSIZE - size;
    block->num_free_blocks = 1;
    block->freeblock->addr = (unsigned char *) block->data + size;
    block->freeblock->size = BLOCKSIZE - size;
    block->freeblock->next = NULL;
    block->freeblock->last = NULL;
    addr = block->data;
    return(addr);
}


/****************************** FASTFREE **************************************/
void fastfree(
    unsigned char *addr,
    unsigned int size)
{
    STDBLOCK *block;
    FREEBLOCK *freeblock,*fb,*fb2;
    LARGEBLOCK *largeblock;
    unsigned char *nextaddr = addr + size;
    int merged;

    /* Find the right block */
    for (block=stdblock_head; block != NULL; block=block->next) {
	if ((addr >= block->data) && (addr < block->data+BLOCKSIZE)) 
	    break;
    }

    if (block) {
	merged = 0;
	/* See if we can add it onto an old freeblock */
	for (fb = block->freeblock;
		(fb != NULL) && (fb->addr < addr);
		fb = fb->next) {
	    if (fb->addr + fb->size == addr) {
		/* Tack it on to previous! */
		fb->size += size;
		/* See if we can add the next one too. */
		fb2 = fb->next;
		if ((fb2 != NULL) && (fb2->addr == nextaddr)) {
		    /* Free up the next block. */
		    fb->size += fb2->size;
		    if ((fb->next = fb2->next)) {
			(fb->next)->last = fb;
		    }
		    free(fb2);
		    --(block->num_free_blocks);
		}
		merged = 1;
		break;
	    }
	}

	if ((fb != NULL) && !merged && (fb->addr == nextaddr)) {
	    /* Tack it onto the end of the previous one. */
	    fb->addr -= size;
	    fb->size += size;
	    merged = 1;
	}

	if (merged) {
	    if (fb->size > block->largest_free_block_size)
		block->largest_free_block_size = fb->size;
	    if (block->largest_free_block_size == BLOCKSIZE) {
		/* Free up the whole block! */
		/* Might want to do something about thrashing here. */
		if (block == stdblock_head) {
		    if (block->next) (block->next)->last = NULL;
		    stdblock_head = block->next;
		}
		else if (block->next == NULL) {
		    if (block->last) (block->last)->next = NULL;
		}
		else {
		    (block->last)->next = block->next;
		    (block->next)->last = block->last;
		}
		free(block->freeblock);
		free(block);
	    }
	    return;
	}
	/* else */

	++(block->num_free_blocks);
	/* Create a new freeblock for this data. */
	if ((freeblock = (FREEBLOCK *) malloc(sizeof(FREEBLOCK))) == NULL) {
	    /* Oh well. */
	    return;
	}
	freeblock->addr = addr;
	freeblock->size = size;
	block->largest_free_block_size = size;
	/* Insert it in the proper order. */
	if (fb == block->freeblock) {
	    /* head of list */
	    freeblock->last = NULL;
	    freeblock->next = fb;
	    if (fb != NULL) fb->last = freeblock;
	    block->freeblock = freeblock;
	}
	else if (fb == NULL) {
	    /* end of list */
	    fb = block->freeblock;
	    while (fb->next != NULL) fb = fb->next;
	    freeblock->next = NULL;
	    freeblock->last = fb;
	    fb->next = freeblock;
	}
	else {
	    /* middle */
	    freeblock->next = fb;
	    freeblock->last = fb->last;
	    fb->last = freeblock;
	    (freeblock->last)->next = freeblock;
	}
	return;
    }
    /* else */

    /* check largeblocks */
    for (largeblock = largeblock_head;
	    largeblock != NULL;
	    largeblock = largeblock->next) {
	if (largeblock->addr == addr) {
	    free(addr);
	    if (largeblock->last == NULL) {
		/* head of the list */
		largeblock_head = largeblock->next;
		if (largeblock_head) largeblock_head->last = NULL;
	    }
	    else if (largeblock->next == NULL) {
		/* End of the list */
		(largeblock->last)->next = NULL;
	    }
	    else {
		(largeblock->last)->next = largeblock->next;
		(largeblock->next)->last = largeblock->last;
	    }
	    free(largeblock);
	    return;
	}
    }
}
