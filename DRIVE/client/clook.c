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


/* This file provides a way of translating the user's rgb vehicle color into
 * a text description of the color.  The file /usr/lib/X11/rgb.txt was used
 * to define the text names. */

#include "global.h"
#include "drive.h"


typedef struct {
    float r,g,b;
    char *name;
} color_name;

static color_name COLORS[] = {
    { 240,248,255,	"Alice Blue" },
    { 255,239,219,	"Antique White" },
    { 50,191,193,	"Aquamarine" },
    { 240,255,255,	"Azure" },
    { 245,245,220,	"Beige" },
    { 255,228,196,	"Bisque" },
    { 0,0,0,		"Black" },
    { 255,235,205,	"Blanched Almond" },
    { 138,43,226,	"Blue Violet" },
    { 0,0,205,		"Blue" },
    { 0,0,255,		"Bright Blue" },
    { 0,255,255,	"Bright Cyan" },
    { 0,255,0,		"Bright Green" },
    { 255,0,255,	"Bright Magenta" },
    { 255,0,0,		"Bright Red" },
    { 255,255,0,	"Bright Yellow" },
    { 165,42,42,	"Brown" },
    { 222,184,135,	"Burlywood" },
    { 95,146,158,	"Cadet Blue" },
    { 127,255,0,	"Chartreuse" },
    { 210,105,30,	"Chocolate" },
    { 255,114,86,	"Coral" },
    { 34,34,152,	"Cornflower Blue" },
    { 255,248,220,	"Cornsilk" },
    { 0,205,205,	"Cyan" },
    { 0,0,100,		"Dark Blue" },
    { 0,100,100,	"Dark Cyan" },
    { 184,134,11,	"Dark Goldenrod" },
    { 0,100,0,		"Dark Green" },
    { 0,86,45,		"Dark Green" },
    { 189,183,107,	"Dark Khaki" },
    { 85,86,47,		"Dark Olive Green" },
    { 255,140,0,	"Dark Orange" },
    { 100,0,100,	"Dark Magenta" },
    { 100,0,0,		"Dark Red" },
    { 233,150,122,	"Dark Salmon" },
    { 143,188,143,	"Dark Sea Green" },
    { 56,75,102,	"Dark Slate Blue" },
    { 47,79,79,		"Dark Slate Gray" },
    { 0,166,166,	"Dark Turquoise" },
    { 148,0,211,	"Dark Violet" },
    { 100,100,0,	"Dark Yellow" },
    { 255,20,147,	"Deep Pink" },
    { 0,191,255,	"Deep Sky Blue" },
    { 84,84,84,		"Dim Gray" },
    { 142,35,35,	"Firebrick" },
    { 255,250,240,	"Floral White" },
    { 80,159,105,	"Forest Green" },
    { 220,220,220,	"Gainsboro" },
    { 248,248,255,	"Ghost White" },
    { 218,170,0,	"Gold" },
    { 239,223,132,	"Goldenrod" },
    { 126,126,126,	"Gray" },
    { 173,255,47,	"Green Yellow" },
    { 0,205,0,		"Green" },
    { 240,255,240,	"Honeydew" },
    { 255,105,180,	"Hot Pink" },
    { 107,57,57,	"Indian Red" },
    { 255,255,240,	"Ivory" },
    { 179,179,126,	"Khaki" },
    { 255,240,245,	"Lavender Blush" },
    { 230,230,250,	"Lavender" },
    { 124,252,0,	"Lawn Green" },
    { 255,250,205,	"Lemon Chiffon" },
    { 176,226,255,	"Light Blue" },
    { 240,128,128,	"Light Coral" },
    { 224,255,255,	"Light Cyan" },
    { 250,250,210,	"Light Goldenrod Yellow" },
    { 238,221,130,	"Light Goldenrod" },
    { 168,168,168,	"Light Gray" },
    { 255,182,193,	"Light Pink" },
    { 255,160,122,	"Light Salmon" },
    { 32,178,170,	"Light Sea Green" },
    { 135,206,250,	"Light Sky Blue" },
    { 96,123,139,	"Light Sky Blue" },
    { 132,112,255,	"Light Slate Blue" },
    { 119,136,153,	"Light Slate Gray" },
    { 124,152,211,	"Light Steel Blue" },
    { 255,255,224,	"Light Yellow" },
    { 0,175,20,		"Lime Green" },
    { 250,240,230,	"Linen" },
    { 205,0,205,	"Magenta" },
    { 143,0,82,		"Maroon" },
    { 0,147,143,	"Medium Aquamarine" },
    { 50,50,204,	"Medium Blue" },
    { 50,129,75,	"Medium Forest Green" },
    { 209,193,102,	"Medium Goldenrod" },
    { 189,82,189,	"Medium Orchid" },
    { 147,112,219,	"Medium Purple" },
    { 52,119,102,	"Medium Sea Green" },
    { 106,106,141,	"Medium Slate Blue" },
    { 35,142,35,	"Medium Spring Green" },
    { 0,210,210,	"Medium Turquoise" },
    { 213,32,121,	"Medium Violet Red" },
    { 47,47,100,	"Midnight Blue" },
    { 245,255,250,	"Mint Cream" },
    { 255,228,225,	"Misty Rose" },
    { 255,228,181,	"Moccasin" },
    { 255,222,173,	"Navajo White" },
    { 35,35,117,	"Navy Blue" },
    { 35,35,117,	"Navy" },
    { 253,245,230,	"Old Lace" },
    { 107,142,35,	"Olive Drab" },
    { 255,135,0,	"Orange" },
    { 255,69,0,		"Orange Red" },
    { 239,132,239,	"Orchid" },
    { 238,232,170,	"Pale Goldenrod" },
    { 115,222,120,	"Pale Green" },
    { 175,238,238,	"Pale Turquoise" },
    { 219,112,147,	"Pale Violet Red" },
    { 255,239,213,	"Papaya Whip" },
    { 255,218,185,	"Peach Puff" },
    { 205,133,63,	"Peru" },
    { 255,181,197,	"Pink" },
    { 197,72,155,	"Plum" },
    { 176,224,230,	"Powder Blue" },
    { 160,32,240,	"Purple" },
    { 205,0,0,		"Red" },
    { 188,143,143,	"Rosy Brown" },
    { 65,105,225,	"Royal Blue" },
    { 139,69,19,	"Saddle Brown" },
    { 233,150,122,	"Salmon" },
    { 244,164,96,	"Sandy Brown" },
    { 82,149,132,	"Sea Green" },
    { 255,245,238,	"Seashell" },
    { 150,82,45,	"Sienna" },
    { 200,200,200,	"Silver" },
    { 114,159,255,	"Sky Blue" },
    { 126,136,171,	"Slate Blue" },
    { 112,128,144,	"Slate Gray" },
    { 255,250,250,	"Snow" },
    { 65,172,65,	"Spring Green" },
    { 84,112,170,	"Steel Blue" },
    { 222,184,135,	"Tan" },
    { 216,191,216,	"Thistle" },
    { 255,99,71,	"Tomato" },
    { 25,204,223,	"Turquoise" },
    { 243,62,150,	"Violet Red" },
    { 156,62,206,	"Violet" },
    { 245,222,179,	"Wheat" },
    { 245,245,245,	"White Smoke" },
    { 255,255,255,	"White" },
    { 50,216,56,	"Yellow Green" },
    { 205,205,0,	"Yellow" },
};



char *lookup_rgb(
    float r, float g, float b)
{
    int entries,i,closest;
    int sum, best_sum, d;
    int ir,ig,ib;

    closest = 0;
    best_sum = 0x7fffffff;
    ir = (int) ((r * 255.0) + 0.499);
    ig = (int) ((g * 255.0) + 0.499);
    ib = (int) ((b * 255.0) + 0.499);

    entries = sizeof( COLORS ) / sizeof( color_name );

    for (i = 0; i<entries; i++ ) {
	if ((sum = ir - COLORS[i].r) < 0) sum = -sum;
	if (sum > best_sum) continue;

	if ((d = ig - COLORS[i].g) < 0) d = -d;
	if ((sum += d) > best_sum) continue;

	if ((d = ib - COLORS[i].b) < 0) d = -d;
	if ((sum += d) > best_sum) continue;

	/* must be best sum yet... */
	closest = i;
	if ((best_sum = sum) == 0) break;
    }

    return(COLORS[closest].name);
}


void rgb_to_hsv(
    float r, float g, float b,
    float *h,float *s,float *v)
{
    float max,min;
    float rc,gc,bc;

    if (r > g) max = r;
    else max = g;
    if (b > max) max = b;

    if (r < g) min = r;
    else min = g;
    if (b < min) min = b;

    *v = max;

    if (max != 0) *s = (max - min) / max;
    else *s = 0.0;

    if (*s == 0.0) {
	*h = 0.0;
    }
    else {
	rc = (max - r) / (max - min);
	gc = (max - g) / (max - min);
	bc = (max - b) / (max - min);
	if (r == max)      *h = 60.0*(bc - gc);
	else if (g == max) *h = 120.0 + 60.0*(rc - bc);
	else               *h = 240.0 + 60.0*(gc - rc);

	if (*h < 0.0) *h += 360.0;
    }
    *h /= 360.0;
}


void hsv_to_rgb(
    float h, float s, float v,
    float *r,float *g,float *b)
{
    float i, f, p, q, t;

    if (v > 1.0) v = 1.0;

    if ((h *= 360.0) >= 360.0) h = 0.0;
    h /= 60.0;
    i = floor(h);
    f = h - i;
    p = v * (1.0 - s);
    q = v * (1.0 - (s * f));
    t = v * (1.0 - (s * (1.0 - f)));
    switch ((int) i) {
      case 0:
	*r = v; *g = t; *b = p; break;
      case 1:
	*r = q; *g = v; *b = p; break;
      case 2:
	*r = p; *g = v; *b = t; break;
      case 3:
	*r = p; *g = q; *b = v; break;
      case 4:
	*r = t; *g = p; *b = v; break;
      case 5:
	*r = v; *g = p; *b = q; break;
    }
}
