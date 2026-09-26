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

/*
 * Derived from code originally Copyright (c) 2023 Ross Cunniff
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>

#include "hw.h"
#include "hw_internal.h"
#include "hw_gl.h"

#ifdef WIN32
extern GL_GEN_BUFFERS_FPTR glGenBuffers;
extern GL_BIND_BUFFER_FPTR glBindBuffer;
extern GL_BUFFER_DATA_FPTR glBufferData;
extern GL_BUFFER_SUB_DATA_FPTR glBufferSubData;
extern GL_DELETE_BUFFERS_FPTR glDeleteBuffers;
#endif

/* Here we plug in the display list functions */
hwInt32 __hwGlOpenList( hwDisplay disp )
{
    USE_GL_CTX(disp);
    glDispList
        newList;
    int
        i, n;

    if( !glctx )                 return -1;
    if( glctx->openList )        return -1;

    newList = malloc( sizeof(struct __glDispList) );
    if( !newList )    return -1;

    newList->dlName = i = glctx->numLists++;
    newList->head = newList->tail = NULL;

    n = i % GL_DL_HASH_SIZE;
    newList->prevHash = NULL;
    if( glctx->displayHash[n] ) {
        glctx->displayHash[n]->prevHash = newList;
    }
    newList->nextHash = glctx->displayHash[n];
    glctx->displayHash[n] = newList;
    glctx->openList = newList;

    return i;
}

#if 0
static void __hwGlPrintList( glDispList currList, char *prefix )
{
    glDlElement
        currCall,
        nextCall;
    static const char
        *pn[] = { "PT", "LI", "LL", "LS", "TR", "TS", "TF" };

    printf("%s DL %d {\n", prefix, (int)currList->dlName);

    for( currCall = currList->head; currCall; currCall = nextCall ) {
        nextCall = currCall->hdr.next;

        switch( currCall->hdr.elemType ) {
        case GL_DL_SET_POINTERS :
            printf("    VF %d @ %d WPV\n",
                        (int)currCall->vtx.numVerts,
                        (int)currCall->vtx.wpv);
            break;
        case GL_DL_DRAW_ELEMENTS :
            printf("    DE %s x %d\n",
                        pn[currCall->idx.primType],
                        (int)currCall->idx.primCount);
            break;
        case GL_DL_PUSH_MATRIX :
            printf("    PU\n");
            break;
        case GL_DL_POP_MATRIX :
            printf("    PO\n");
            break;
        case GL_DL_SURF_ATTRS :
            printf("    SA\n");
            break;
        case GL_DL_CALL_LIST :
            printf("    CL %d\n", (int)currCall->call.listNum);
            break;
        case GL_DL_WIREFRAME :
            printf("    WF\n");
            break;
        case GL_DL_SOLID :
            printf("    SO\n");
            break;
        case GL_DL_VISIBILITY :
            printf("    VM %08x\n", (int)currCall->vis.visMask);
            break;
        case GL_DL_BUFF_OBJS :
            printf("    VB %d + %d\n",
                        (int)currCall->vbo.vBO,
                        (int)currCall->vbo.iBO);
            break;
        case GL_DL_BUFF_VERTS :
            printf("    BV %d @ %d WPV (offs %d)\n",
                        (int)currCall->bvtx.numVerts,
                        (int)currCall->bvtx.wpv,
                        (int)currCall->bvtx.offset);
            break;
        case GL_DL_BUFF_IDX :
            printf("    BI %s x %d (%d)\n",
                        pn[currCall->bidx.primType],
                        (int)currCall->bidx.primCount,
                        (int)currCall->bidx.idxOffs);
            break;
        }
    }
    printf("}\n");
}
#endif

static int surfCmp( hwSurfaceType *a, hwSurfaceType *b )
{
    int i, n;

    if( a->color[0]     != b->color[0]     ) return 0;
    if( a->color[1]     != b->color[1]     ) return 0;
    if( a->color[2]     != b->color[2]     ) return 0;
    if( a->transp       != b->transp       ) return 0;
    if( a->specColor[0] != b->specColor[0] ) return 0;
    if( a->specColor[1] != b->specColor[1] ) return 0;
    if( a->specColor[2] != b->specColor[2] ) return 0;
    if( a->shininess    != b->shininess    ) return 0;
    if( a->primSize     != b->primSize     ) return 0;
#define FL_MASK (HW_SURF_EMISSIVE|HW_SURF_BLEND)
    if( (a->flags ^ b->flags) & FL_MASK     ) return 0;
#undef  FL_MASK
    if( a->numTextures  != b->numTextures  ) return 0;
    n = a->numTextures;
    for( i = 0; i < n; i++ ) {
        if( a->textures[i]  != b->textures[i]  ) return 0;
    }
    return 1;
}

#define DELETE_ELEM(currList,prevCall,currCall,nextCall) do { \
    if( prevCall ) prevCall->hdr.next = nextCall; \
    else           currList->head = nextCall; \
    if( !nextCall ) currList->tail = prevCall; \
    free( currCall ); \
    currCall = NULL; \
} while(0)

#define REPLACE_ELEM(currList,prevCall,currCall,nextCall,tmp) do { \
    tmp->hdr.next = currCall->hdr.next; \
    if( prevCall ) prevCall->hdr.next = tmp; \
    else currList->head = tmp; \
    if( !nextCall ) currList->tail = tmp; \
    free( currCall ); \
    currCall = tmp; \
} while(0)

extern int __hwglDoVbo;

static void dlOptimRemoveRedundant( glDispList currList )
{
    glDlElement
        prevCall,
        currCall,
        nextCall;
    hwInt32
        visMask, visFound,
        isWireframe,
        surfFound;
    hwSurfaceType
        surf;

    visFound = 0;
    isWireframe = -1;
    surfFound = 0;

    prevCall = NULL;
    for( currCall = currList->head; currCall; currCall = nextCall ) {
        nextCall = currCall->hdr.next;

        switch( currCall->hdr.elemType ) {
        case GL_DL_SURF_ATTRS :
            if( surfFound ) {
                if( surfCmp( &surf, &currCall->surf.surf ) ) {
                    DELETE_ELEM(currList, prevCall, currCall, nextCall);
                }
            }
            if( currCall ) {
                surfFound = 1;
                surf = currCall->surf.surf;
            }
            break;
        case GL_DL_WIREFRAME :
            if( isWireframe == 1 ) {
                DELETE_ELEM(currList, prevCall, currCall, nextCall);
            }
            isWireframe = 1;
            break;
        case GL_DL_SOLID :
            if( isWireframe == 0 ) {
                DELETE_ELEM(currList, prevCall, currCall, nextCall);
            }
            isWireframe = 0;
            break;
        case GL_DL_VISIBILITY :
            if( visFound ) {
                if( currCall->vis.visMask == visMask ) {
                    DELETE_ELEM(currList, prevCall, currCall, nextCall);
                }
            }
            if( currCall ) {
                visFound = 1;
                visMask = currCall->vis.visMask;
            }
            break;
        }

        if( currCall ) {
            prevCall = currCall;
        }
    }
}

static void dlOptimCoalesceBuffers( glDispList currList )
{
    glDlElement
        prevCall,
        currCall,
        nextCall,
        firstVerts,
        tmp;
    hwInt32
        vtxFound,
        vtxSize,
        idxOffs,
        i, n, wpv;

    prevCall = NULL;
    for( currCall = currList->head; currCall; currCall = nextCall ) {
        nextCall = currCall->hdr.next;

        if( currCall->hdr.elemType != GL_DL_SET_POINTERS ) goto OuterContinue;

        /* Calculate size of coalesced vertices, up to 64k-1 verts */
        vtxSize = currCall->vtx.numVerts;
        vtxFound = 0;
        for( tmp = nextCall; tmp; tmp = tmp->hdr.next ) {
            if( tmp->hdr.elemType != GL_DL_SET_POINTERS ) continue;
            if( currCall->vtx.dataFlags != tmp->vtx.dataFlags ) break;
            if( (vtxSize+tmp->vtx.numVerts) > 65535 ) break;

            vtxSize += tmp->vtx.numVerts;
            vtxFound = 1;
        }

        if( !vtxFound ) goto OuterContinue;

        /* Allocate the new element */
        wpv = currCall->vtx.wpv;
        n = sizeof(glDlVertexes) + vtxSize * wpv * sizeof(hwFloat);
        firstVerts = malloc(n);
        if( !firstVerts ) break;

        /* Copy the first vertex data over */
        firstVerts->vtx = currCall->vtx;
        firstVerts->vtx.data = (hwFloat *)(1 + &firstVerts->vtx);
        vtxSize = firstVerts->vtx.numVerts;
        memcpy(firstVerts->vtx.data, currCall->vtx.data,
               vtxSize*wpv*sizeof(hwFloat));

        REPLACE_ELEM(currList, prevCall, currCall, nextCall, firstVerts);

        /* Now, copy the rest of the data over and adjust indices. Note
         * that we're actually changing the value of currCall as
         * we iterate, so we don't do this twice.
         */
        idxOffs = 0;
        for( currCall = nextCall; currCall; currCall = nextCall ) {
            nextCall = currCall->hdr.next;

            switch( currCall->hdr.elemType ) {
            case GL_DL_SET_POINTERS :
                if( currCall->vtx.dataFlags != firstVerts->vtx.dataFlags ) {
                    goto InnerBreak;
                }
                if( (vtxSize+currCall->vtx.numVerts) > 65535 ) {
                    goto InnerBreak;
                }
                memcpy(firstVerts->vtx.data+vtxSize*wpv, currCall->vtx.data,
                       currCall->vtx.numVerts*wpv*sizeof(hwFloat));
                idxOffs = vtxSize;
                vtxSize += currCall->vtx.numVerts;
                DELETE_ELEM(currList, prevCall, currCall, nextCall);
                break;

            case GL_DL_DRAW_ELEMENTS :
                n = currCall->idx.primCount;
                for( i = 0; i < n; i++ ) {
                    currCall->idx.indices[i] += idxOffs;
                }
                break;

            default :
                /* Nothing to do.  We actually don't care if visibility,
                 * surface attrs, etc. change for this phase of
                 * optimization.
                 */
                break;
            }

            if( currCall ) {
                prevCall = currCall;
            }
            continue;

        InnerBreak :
            /* We get here if we have a DL_SET_POINTERS mismatch.  We
             * will want to examine this same DL_SET_POINTERS on the
             * next outer iteration.
             */
            nextCall = currCall;
            currCall = NULL;
            break;
        }
        firstVerts->vtx.numVerts = vtxSize;

    OuterContinue :
        if( currCall ) {
            prevCall = currCall;
        }
    }
}

static void dlOptimCombinePrims( glDispList currList )
{
    glDlElement
        tmp,
        prevCall,
        currCall,
        nextCall;
    hwInt32
        n,
        idxCount;

    prevCall = NULL;
    for( currCall = currList->head; currCall; currCall = nextCall ) {
        nextCall = currCall->hdr.next;

        if( currCall->hdr.elemType != GL_DL_DRAW_ELEMENTS ) goto OuterContinue;

        switch( currCall->idx.primType ) {
        case GL_POINTS :
        case GL_LINES :
        case GL_TRIANGLES :
            /* These primitives are OK to coalesce */
            break;
        default :
            /* Any others are not OK */
            goto OuterContinue;
        }

        /* Count the number of compatible primitives */
        idxCount = currCall->idx.primCount;
        for( tmp = nextCall; tmp; tmp = tmp->hdr.next ) {
            /* Only handle sequences of consecutive draws of identical
             * primitive types.  We have already eliminated redundant
             * states and combined vertex arrays where possible.
             */
            if( tmp->hdr.elemType != GL_DL_DRAW_ELEMENTS ) break;
            if( tmp->idx.primType != currCall->idx.primType ) break;
            idxCount += tmp->idx.primCount;
        }

        /* If nothing to coalesce, don't do anything */
        if( idxCount == currCall->idx.primCount ) goto OuterContinue;

        /* Aha!  Let's combine these primitives! */
        n = sizeof(glDlIndexes) + idxCount*sizeof(hwInt16);
        tmp = malloc(n);
        if( !tmp ) break;
        tmp->idx = currCall->idx;
        tmp->idx.indices = (hwInt16 *)(1 + &tmp->idx);
        memcpy(tmp->idx.indices, currCall->idx.indices,
               currCall->idx.primCount*sizeof(hwInt16));
        tmp->idx.primCount = idxCount;
        idxCount = currCall->idx.primCount;
        REPLACE_ELEM(currList, prevCall, currCall, nextCall, tmp);

        prevCall = currCall;
        for( currCall = nextCall; currCall; currCall = nextCall ) {
            nextCall = currCall->hdr.next;

            if( currCall->hdr.elemType != GL_DL_DRAW_ELEMENTS ) break;
            if( tmp->idx.primType != currCall->idx.primType ) goto InnerBreak;

            memcpy(tmp->idx.indices + idxCount, currCall->idx.indices,
                   currCall->idx.primCount*sizeof(hwInt16));
            idxCount += currCall->idx.primCount;

            DELETE_ELEM(currList, prevCall, currCall, nextCall);

            if( currCall ) {
                prevCall = currCall;
            }
            continue;

        InnerBreak :
            /* We went one step too far - this is a DRAW_ELEMENTS that
             * might coalesce with another
             */
            nextCall = currCall;
            currCall = NULL;
            break;
        }

    OuterContinue :
        if( currCall ) {
            prevCall = currCall;
        }
    }
}

static void dlOptimInsertVBO( glDispList currList )
{
    glDlElement
        prevCall,
        currCall,
        nextCall,
        vboCall,
        tmp;
    hwInt32
        vtxSize, idxSize,
        idxOffs,
        i, n, wpv;
    GLuint
        ids[2];

    if( !__hwglDoVbo ) return;

    /* Count vertices, indices */
    vtxSize = idxSize = 0;
    for( currCall = currList->head; currCall; currCall = nextCall ) {
        nextCall = currCall->hdr.next;
        switch( currCall->hdr.elemType ) {
        case GL_DL_SET_POINTERS :
            wpv = currCall->vtx.wpv;
            n = wpv * currCall->vtx.numVerts;
            vtxSize += n;
            break;
        case GL_DL_DRAW_ELEMENTS :
            idxSize += currCall->idx.primCount;
            break;
        default :
            break;
        }
    }

    if( idxSize < VBO_THRESH ) return;

    /* Insert VBO reference at the top */
    glGenBuffers(2, ids);
    vboCall = __hwGlAllocBuffObjs(ids[0], ids[1]);
    vboCall->hdr.next = currList->head;
    currList->head = vboCall;

    /* Allocate data */
    glBindBuffer(GL_ARRAY_BUFFER, vboCall->vbo.vBO);
    glBufferData(GL_ARRAY_BUFFER, vtxSize*sizeof(float),
                        NULL, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboCall->vbo.iBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idxSize*sizeof(short),
                        NULL, GL_STATIC_DRAW);

    /* Copy data in and replace with VBO references */
    vtxSize = idxSize = 0;
    prevCall = NULL;
    for( currCall = currList->head; currCall; currCall = nextCall ) {
        nextCall = currCall->hdr.next;

        switch( currCall->hdr.elemType ) {
        case GL_DL_SET_POINTERS :
            wpv = currCall->vtx.wpv;
            n = wpv * currCall->vtx.numVerts;
            tmp = __hwGlAllocBuffVerts( currCall->vtx.dataFlags,
                                        currCall->vtx.numVerts,
                                        vtxSize * sizeof(float) );
            glBufferSubData(GL_ARRAY_BUFFER,
                                tmp->bvtx.offset,
                                n * sizeof(float),
                                currCall->vtx.data);
            vtxSize += n;

            REPLACE_ELEM(currList, prevCall, currCall, nextCall, tmp);
            break;

        case GL_DL_DRAW_ELEMENTS :
            n = currCall->idx.primCount;
            tmp = __hwGlAllocBuffIdx( currCall->idx.primType,
                                      n, idxSize*sizeof(short) );
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER,
                                tmp->bidx.idxOffs,
                                n * sizeof(short),
                                currCall->idx.indices);
            idxSize += n;

            REPLACE_ELEM(currList, prevCall, currCall, nextCall, tmp);
            break;
        }

        if( currCall ) {
            prevCall = currCall;
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void __hwGlCloseList( hwDisplay disp )
{
    USE_GL_CTX(disp);
    glDispList
        currList;

    if( !glctx )                 return;
    if( !glctx->openList )       return;

    /* Optimization pass */
    currList = glctx->openList;
    glctx->openList = NULL;

    //__hwGlPrintList( currList, "before" );

    /* First pass - eliminate redundant state calls. */
    dlOptimRemoveRedundant(currList);

    /* Second pass - coalesce vertex buffers */
    dlOptimCoalesceBuffers(currList);

    /* Third pass - combine draw calls */
    dlOptimCombinePrims(currList);

    /* Final pass - allocate VBOs */
    dlOptimInsertVBO(currList);

    //__hwGlPrintList( currList, "after" );
}

void __hwGlCallList( hwDisplay disp, hwInt32 dl )
{
    USE_GL_CTX(disp);
    glDispList
        currList;
    glDlElement
        currCall;
    int
        i, n;
    hwInt32
        currVis = 0xFFFFFFFF;
    float
        *ptr;
    hwInt16
        *iptr;

    if( !glctx )                 return;

    if( glctx->openList ) {
        __hwGlAppendCallList( glctx, dl );
        return;
    }

    /* Find the list in the hash table */
    n = dl % GL_DL_HASH_SIZE;
    for(    currList = glctx->displayHash[n];
            currList && currList->dlName != dl;
            currList = currList->nextHash )
    {
        /* NOTHING */
    }
    if( !currList ) return;

    for(    currCall = currList->head;
            currCall;
            currCall = currCall->hdr.next )
    {
        switch( currCall->hdr.elemType ) {
        case GL_DL_BUFF_OBJS :
            glBindBuffer(GL_ARRAY_BUFFER, currCall->vbo.vBO );
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, currCall->vbo.iBO );
            break;
        case GL_DL_SET_POINTERS :
            __hwGlSetOffsPointers( glctx,
                                   currCall->vtx.data,
                                   &currCall->vtx.vo,
                                   currCall->vtx.numVerts,
                                   currCall->vtx.wpv, 1 );
            break;
        case GL_DL_BUFF_VERTS :
            ptr = (float *)(currCall->bvtx.offset + (char *)0);
            __hwGlSetOffsPointers( glctx, ptr,
                                   &currCall->bvtx.vo,
                                   currCall->bvtx.numVerts,
                                   currCall->bvtx.wpv, 1 );
            break;
        case GL_DL_DRAW_ELEMENTS :
            if( !__hwGlAnimVisible( disp, currVis ) ) break;

            __hwGlBeginSelect(glctx);
            hwGlBeginRendering();

            glDrawElements( currCall->idx.primType,
                            currCall->idx.primCount,
                            GL_UNSIGNED_SHORT,
                            currCall->idx.indices );

            __hwGlEndSelect(glctx);
            break;
        case GL_DL_BUFF_IDX :
            if( !__hwGlAnimVisible( disp, currVis ) ) break;

            iptr = (hwInt16 *)(currCall->bidx.idxOffs + (char *)0);

            __hwGlBeginSelect(glctx);
            hwGlBeginRendering();

            glDrawElements( currCall->bidx.primType,
                            currCall->bidx.primCount,
                            GL_UNSIGNED_SHORT,
                            iptr );

            __hwGlEndSelect(glctx);
            break;
        case GL_DL_PUSH_MATRIX :
            __hwGlPushMat( disp, currCall->mat.mat );
            break;
        case GL_DL_POP_MATRIX :
            __hwGlPopMat( disp );
            break;
        case GL_DL_SURF_ATTRS :
            __hwGlSurfAttrs( disp, &currCall->surf.surf );
            break;
        case GL_DL_CALL_LIST :
            __hwGlCallList( disp, currCall->call.listNum );
            break;
        case GL_DL_WIREFRAME :
            __hwgl_SetWireframeState(glctx);
            break;
        case GL_DL_SOLID :
            __hwgl_SetSolidState(glctx);
            break;
        case GL_DL_VISIBILITY :
            currVis = currCall->vis.visMask;
            break;
        }
    }
    /* Turn off VBOs */
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void __hwGlDestroyList( hwDisplay disp, hwInt32 dl )
{
    USE_GL_CTX(disp);
    glDispList
        currList;
    glDlElement
        currCall,
        nextCall;
    int
        n;
    GLuint
        ids[2];

    if( !glctx )                 return;
    if( glctx->openList )        return;

    n = dl % GL_DL_HASH_SIZE;
    for(    currList = glctx->displayHash[n];
            currList && currList->dlName != dl;
            currList = currList->nextHash )
    {
        /* NOTHING */
    }
    if( !currList )     return;

    /* Delete all of the calls */
    for( currCall = currList->head; currCall; currCall = nextCall ) {
        nextCall = currCall->hdr.next;

        /* Do any special handling necessary */
        switch( currCall->hdr.elemType ) {
        case GL_DL_BUFF_OBJS :
            ids[0] = currCall->vbo.vBO;
            ids[1] = currCall->vbo.iBO;
            glDeleteBuffers(2, ids);
            break;
        }

        /* Because of the way we allocate, we free each call with
         * exactly ONE call to free.  So there.
         */
        free( currCall );
    }

    /* Delete this element from the list */
    if( currList->prevHash ) {
        currList->prevHash->nextHash = currList->nextHash;
    }
    else {
        glctx->displayHash[n] = currList->nextHash;
    }
    free( currList );
}

#define APPEND_ELEM(currList,currCall) do { \
    if( currList->tail ) currList->tail->hdr.next = currCall; \
    else currList->head = currCall; \
    currList->tail = currCall; \
} while(0)

void __hwGlAppendVertices
(
    glDrawable *glctx, hwFloat *data, hwInt32 dataFlags, hwInt32 nv
)
{
    glDispList
        currList;
    glDlElement
        currCall;
    hwVertexOffsets
        vOffs;
    int
        i, j, addAlpha = 0,
        totSize, wpv, dstWPV, vtxSize, strSize;
    hwFloat
        alpha,
        *src, *dst;

    if( !glctx ) return;

    currList = glctx->openList;
    if( !currList ) return;

    if( (dataFlags & HW_DATA_RGB) && !(dataFlags & HW_DATA_ALPHA) ) {
        /* We always store RGBA.  Synthesize alpha. */
        addAlpha = 1;
        dataFlags |= HW_DATA_ALPHA;
    }

    /* Allocate data */
    dstWPV = hwGetVertexOffsets( dataFlags, &vOffs );
    wpv = dstWPV - addAlpha;

    vtxSize = dstWPV*nv*sizeof(hwFloat);
    strSize = sizeof(glDlVertexes);
    totSize = strSize + vtxSize;

    currCall = malloc( totSize );
    if( !currCall ) return;

    /* Initialize data */
    currCall->vtx.hdr.elemType = GL_DL_SET_POINTERS;
    currCall->vtx.hdr.next = NULL;
    currCall->vtx.vo = vOffs;
    currCall->vtx.wpv = dstWPV;
    currCall->vtx.dataFlags = dataFlags;
    currCall->vtx.numVerts = nv;
    currCall->vtx.data = (hwFloat *)(strSize + (char *) currCall);

    if( addAlpha ) {
        src = data;
        dst = currCall->vtx.data;
        alpha = 1.0 - glctx->transp;
        for( i = 0; i < nv; i++ ) {
            /* Vertex position and RGB */
            for( j = 0; j < 6; j++ ) *dst++ = *src++;

            /* Add alpha */
            *dst++ = alpha;

            /* Everything else */
            for( ; j < wpv; j++ ) *dst++ = *src++;
        }
    }
    else {
        memcpy(currCall->vtx.data, data, vtxSize);
    }

    /* Stick it in the list */
    APPEND_ELEM(currList, currCall);
}

void __hwGlAppendDrawElements
(
    glDrawable *glctx, hwInt32 primType, hwInt32 primCount, hwInt16 *indices
)
{
    glDispList
        currList;
    glDlElement
        currCall;
    int
        totSize, wpv, idxSize, strSize;

    if( !glctx ) return;

    currList = glctx->openList;
    if( !currList ) return;

    /* Allocate data */
    idxSize = primCount * sizeof(hwInt16);
    strSize = sizeof(glDlIndexes);
    totSize = idxSize + strSize;

    currCall = malloc( totSize );
    if( !currCall ) return;

    /* Initialize result */
    currCall->idx.hdr.elemType = GL_DL_DRAW_ELEMENTS;
    currCall->idx.hdr.next = NULL;
    currCall->idx.primType = primType;
    currCall->idx.primCount = primCount;
    currCall->idx.indices = (hwInt16 *)(strSize + (char *) currCall);
    memcpy( currCall->idx.indices, indices, idxSize );

    /* Stick it in the list */
    APPEND_ELEM(currList, currCall);
}

void __hwGlAppendPushMat( glDrawable *glctx, hwFloat mat[4][4] )
{
    glDispList
        currList;
    glDlElement
        currCall;

    if( !glctx ) return;

    currList = glctx->openList;
    if( !currList ) return;

    /* Allocate data */
    currCall = malloc( sizeof(glDlMatrix) );
    if( !currCall ) return;

    /* Initialize result */
    currCall->mat.hdr.elemType= GL_DL_PUSH_MATRIX;
    currCall->mat.hdr.next = NULL;
    memcpy( currCall->mat.mat, mat, 4*4*sizeof(hwFloat) );

    /* Stick it in the list */
    APPEND_ELEM(currList, currCall);
}

void __hwGlAppendPopMat( glDrawable *glctx )
{
    glDispList
        currList;
    glDlElement
        currCall;

    if( !glctx ) return;

    currList = glctx->openList;
    if( !currList )             return;

    /* Allocate data */
    currCall = malloc( sizeof(glDlElemHdr) );
    if( !currCall )             return;

    /* Initialize result */
    currCall->hdr.elemType = GL_DL_POP_MATRIX;
    currCall->hdr.next = NULL;

    /* Stick it in the list */
    APPEND_ELEM(currList, currCall);
}

void __hwGlAppendSurf( glDrawable *glctx, hwSurfaceType *surf )
{
    glDispList
        currList;
    glDlElement
        currCall;

    if( !glctx ) return;

    currList = glctx->openList;
    if( !currList )             return;

    /* Allocate data */
    currCall = malloc( sizeof(glDlSurface) );
    if( !currCall )             return;

    /* Initialize result */
    currCall->surf.hdr.elemType = GL_DL_SURF_ATTRS;
    currCall->surf.hdr.next = NULL;
    currCall->surf.surf = *surf;

    /* Stick it in the list */
    APPEND_ELEM(currList, currCall);
}

void __hwGlAppendCallList( glDrawable *glctx, hwInt32 dl )
{
    glDispList
        currList;
    glDlElement
        currCall;

    if( !glctx ) return;

    currList = glctx->openList;
    if( !currList )             return;

    /* Allocate data */
    currCall = malloc( sizeof(glDlCallList) );
    if( !currCall )             return;

    /* Initialize result */
    currCall->call.hdr.elemType = GL_DL_CALL_LIST;
    currCall->call.hdr.next = NULL;
    currCall->call.listNum = dl;

    /* Stick it in the list */
    APPEND_ELEM(currList, currCall);
}

void __hwGlAppendVisibility( glDrawable *glctx, hwInt32 v )
{
    glDispList
        currList;
    glDlElement
        currCall;

    if( !glctx ) return;

    currList = glctx->openList;
    if( !currList )             return;

    /* Allocate data */
    currCall = malloc( sizeof(glDlVisibility) );
    if( !currCall )             return;

    /* Initialize result */
    currCall->vis.hdr.elemType = GL_DL_VISIBILITY;
    currCall->vis.hdr.next = NULL;
    currCall->vis.visMask = v;

    /* Stick it in the list */
    APPEND_ELEM(currList, currCall);
}

void __hwGlAppendWireframe( glDrawable *glctx )
{
    glDispList
        currList;
    glDlElement
        currCall;

    if( !glctx ) return;

    currList = glctx->openList;
    if( !currList )             return;

    /* Allocate data */
    currCall = malloc( sizeof(glDlElemHdr) );
    if( !currCall )             return;

    /* Initialize result */
    currCall->hdr.elemType = GL_DL_WIREFRAME;
    currCall->hdr.next = NULL;

    /* Stick it in the list */
    APPEND_ELEM(currList, currCall);
}

void __hwGlAppendSolid( glDrawable *glctx )
{
    glDispList
        currList;
    glDlElement
        currCall;

    if( !glctx ) return;

    currList = glctx->openList;
    if( !currList )             return;

    /* Allocate data */
    currCall = malloc( sizeof(glDlElemHdr) );
    if( !currCall )             return;

    /* Initialize result */
    currCall->hdr.elemType = GL_DL_SOLID;
    currCall->hdr.next = NULL;

    /* Stick it in the list */
    APPEND_ELEM(currList, currCall);
}

glDlElement __hwGlAllocBuffVerts( hwInt32 df, hwInt32 nv, hwInt32 offs )
{
    glDlElement
        currCall;
    hwInt32
        wpv;

    /* Allocate data */
    currCall = malloc( sizeof(glDlBuffVerts) );
    if( !currCall )             return NULL;

    /* Initialize result */
    currCall->hdr.elemType = GL_DL_BUFF_VERTS;
    currCall->hdr.next = NULL;
    currCall->bvtx.dataFlags = df;
    wpv = hwGetVertexOffsets( df, &currCall->bvtx.vo );
    currCall->bvtx.wpv = wpv;
    currCall->bvtx.numVerts = nv;
    currCall->bvtx.offset = offs;

    return currCall;
}

glDlElement __hwGlAllocBuffIdx( hwInt32 pt, hwInt32 np, hwInt32 idxOffs )
{
    glDlElement
        currCall;

    /* Allocate data */
    currCall = malloc( sizeof(glDlBuffIdx) );
    if( !currCall )             return NULL;

    /* Initialize result */
    currCall->hdr.elemType = GL_DL_BUFF_IDX;
    currCall->hdr.next = NULL;
    currCall->bidx.primType = pt;
    currCall->bidx.primCount = np;
    currCall->bidx.idxOffs = idxOffs;

    return currCall;
}

glDlElement __hwGlAllocBuffObjs( hwInt32 iBO, hwInt32 vBO )
{
    glDlElement
        currCall;

    /* Allocate data */
    currCall = malloc( sizeof(glDlBuffObjs) );
    if( !currCall )             return NULL;

    /* Initialize result */
    currCall->hdr.elemType = GL_DL_BUFF_OBJS;
    currCall->hdr.next = NULL;
    currCall->vbo.iBO = iBO;
    currCall->vbo.vBO = vBO;

    return currCall;
}

