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


/* Code Module for various guardrails */

#include "object.h"

extern void init_fence_object(
    DRIVE_OBJECT *obj);

/* The functionality for guardrails has been moved to fence.c, since
 * the guardrail is really just another kind of fence.  This onionskin
 * allows Object: guardrail to continue to work (as opposed to 
 * Object: fence; Type: guardrail).
 */
void init_guardrail_object(
    DRIVE_OBJECT *obj)
{
    strcpy(obj->subtype,"guardrail");
    init_fence_object(obj);
}
