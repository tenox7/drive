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


#ifndef _XgStarbaseP_h
#define _XgStarbaseP_h

#include <Xm/DrawingAP.h>
#include "Starbase.h"

#ifdef __cplusplus
extern "C" {
#endif

/* New fields for the Starbase widget record */
typedef struct {
    int open_mode;
    int shade_mode;
    int transparent;
    Boolean overlay;
    Boolean retained;
    Visual *visual;
    String driver;
    int fildes;
    int wm_cmap;
    int rescale_policy;
    Dimension max_width;
    Dimension max_height;
} XgStarbasePart;

typedef struct _XgStarbaseRec {
    CorePart           core;
    CompositePart      composite;
    ConstraintPart     constraint;
    XmManagerPart      manager;
    XmDrawingAreaPart  drawing_area;
    XgStarbasePart     starbase;
} XgStarbaseRec;

#define XgStarbaseIndex (XmDrawingAIndex + 1)

typedef struct _XgStarbaseClassPart {
    caddr_t extension;
} XgStarbaseClassPart;

typedef struct _XgStarbaseClassRec {
    CoreClassPart           core_class;
    CompositeClassPart      composite_class;
    ConstraintClassPart     constraint_class;
    XmManagerClassPart      manager_class;
    XmDrawingAreaClassPart  drawing_area_class;
    XgStarbaseClassPart     starbase_class;
} XgStarbaseClassRec;

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XgStarbaseP_h */
