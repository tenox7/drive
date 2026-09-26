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
#include <stdio.h>

static int errors_enabled = 1;

void nrerror(
    char *error_text)
{
    if (errors_enabled) {
	fprintf(stderr,"Numerical library run-time error.\n");
	if (*error_text != '\0') fprintf(stderr,"%s.\n",error_text);
    }
}


void disable_numerical_library_errors(
    void)
{
    errors_enabled = 0;
}


void enable_numerical_library_errors(
    void)
{
    errors_enabled = 1;
}
