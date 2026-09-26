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
/* hwVtxFlags.c - a routine to calculate vertex offsets
 * and/or WPV from input flags
 */

#if defined(HW_CALC_WPV)
#    define VTX_STORE_OFFS(a)
#    define VTX_STORE_SIZE(a, n)
#else
#    define VTX_STORE_OFFS(a)   do { offs->a = WPV; } while (0)
#    define VTX_STORE_SIZE(a,n) do { offs->a = n; } while (0)
#endif

#if defined(HW_CALC_WPV)
hwInt32 hwCalcWPV( hwInt32 flags )
#else
hwInt32 hwGetVertexOffsets( hwInt32 flags, hwVertexOffsets *offs )
#endif
{
    hwInt32
        i, tmST, tmR, tmQ, WPV;

#if !defined(HW_CALC_WPV)
    if( offs ) {
        offs->w = 0;
        offs->rgb = 0;
        offs->alpha = 0;
        offs->normal = 0;
        offs->tangent = 0;
        for( i = 0; i < HW_MAX_TEXTURES; i++ ) {
            offs->texCoord[i] = 0;
            offs->texSize[i] = 0;
        }
    }
#endif

    WPV = 3;  // Start with XYZ

    if( flags & HW_DATA_W ) {
        VTX_STORE_OFFS(w);
        WPV += 1;
    }

    if( flags & HW_DATA_RGB ) {
        VTX_STORE_OFFS(rgb);
        WPV += 3;

        if( flags & HW_DATA_ALPHA ) {
            VTX_STORE_OFFS(alpha);
            WPV += 1;
        }
    }

    if( flags & HW_DATA_MD_FLAGS ) {
        /* We don'store this offset, checked here for completeness. */
        WPV += 1;
    }

    if( flags & HW_DATA_NORMALS ) {
        VTX_STORE_OFFS(normal);
        WPV += 3;
    }

    if( flags & HW_DATA_TANGENT ) {
        VTX_STORE_OFFS(tangent);
        WPV += 4;
    }

    tmST = HW_DATA_ST; tmR = HW_DATA_R; tmQ = HW_DATA_Q;
    for( i = 0; i < HW_MAX_TEXTURES; i++, tmST <<= 3, tmR <<= 3, tmQ <<= 3 ) {
        if( flags & tmST ) {
            VTX_STORE_OFFS(texCoord[i]);
            VTX_STORE_SIZE(texSize[i], 2);
            WPV += 2;

            if( flags & tmR ) {
                VTX_STORE_SIZE(texSize[i], 3);
                WPV += 1;

                if( flags & tmQ ) {
                    VTX_STORE_SIZE(texSize[i], 4);
                    WPV += 1;
                }
            }
        }
    }

    return WPV;
}

#undef VTX_STORE_OFFS
#undef VTX_STORE_SIZE
