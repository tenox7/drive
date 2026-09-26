/*
 * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
 * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice including the dates of first publication and
 * either this permission notice or a reference to
 * http://oss.sgi.com/projects/FreeB/
 * shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * SILICON GRAPHICS, INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Silicon Graphics, Inc.
 * shall not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization from
 * Silicon Graphics, Inc.
 *
 * OpenGL ES 1.0 CM port of part of HWGLU by Mike Gorchak <mike@malva.ua>
 */

#ifndef __hwglu_h__
#define __hwglu_h__

/*************************************************************/

/* Boolean */
#define HWGLU_FALSE                          0
#define HWGLU_TRUE                           1

/* Version */
#define HWGLU_VERSION_1_1                    1
#define HWGLU_VERSION_1_2                    1
#define HWGLU_VERSION_1_3                    1

/* StringName */
#define HWGLU_VERSION                        100800
#define HWGLU_EXTENSIONS                     100801

/* ErrorCode */
#define HWGLU_INVALID_ENUM                   100900
#define HWGLU_INVALID_VALUE                  100901
#define HWGLU_OUT_OF_MEMORY                  100902
#define HWGLU_INCOMPATIBLE_GL_VERSION        100903
#define HWGLU_INVALID_OPERATION              100904

/*************************************************************/

void hwGluLookAt
(
    hwFloat eyeX, hwFloat eyeY, hwFloat eyeZ,
    hwFloat centerX, hwFloat centerY, hwFloat centerZ,
    hwFloat upX, hwFloat upY, hwFloat upZ
);
void hwGluOrtho2D
(
    hwFloat left, hwFloat right, hwFloat bottom, hwFloat top
);
void hwGluPerspective
(
    hwFloat fovy, hwFloat aspect, hwFloat zNear, hwFloat zFar
);
void hwGluPickMatrix
(
    hwFloat x, hwFloat y, hwFloat delX, hwFloat delY, hwInt32 *viewport
);
hwInt32 hwGluProject
(
    hwFloat objX, hwFloat objY, hwFloat objZ,
    const hwFloat *model, const hwFloat *proj, const hwInt32 *view,
    hwFloat* winX, hwFloat* winY, hwFloat* winZ
);
hwInt32 hwGluUnProject
(
    hwFloat winX, hwFloat winY, hwFloat winZ,
    const hwFloat *model, const hwFloat *proj, const hwInt32 *view,
    hwFloat* objX, hwFloat* objY, hwFloat* objZ
);
hwInt32 hwGluUnProject4
(
    hwFloat winX, hwFloat winY, hwFloat winZ, hwFloat clipW,
    const hwFloat *model, const hwFloat *proj, const hwInt32 *view,
    hwFloat nearVal, hwFloat farVal,
    hwFloat* objX, hwFloat* objY, hwFloat* objZ, hwFloat* objW
);
hwInt32 hwGluScaleImage
(
    hwInt32 format, hwInt32 widthin,
    hwInt32 heightin, hwInt32 typein,
    const void* datain, hwInt32 widthout,
    hwInt32 heightout, hwInt32 typeout, void* dataout
);
hwInt32 hwGluBuild2DMipmapLevels
(
    hwInt32 target, hwInt32 internalFormat,
    hwInt32 width, hwInt32 height, hwInt32 format,
    hwInt32 type, hwInt32 userLevel, hwInt32 baseLevel,
    hwInt32 maxLevel, const void *data
);
hwInt32 hwGluBuild2DMipmaps
(
    hwInt32 target, hwInt32 internalFormat,
    hwInt32 width, hwInt32 height, hwInt32 format,
    hwInt32 type, const void* data
);

#endif /* __hwglu_h__ */
