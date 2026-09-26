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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#ifndef M_PI
#  define M_PI 3.141592653589
#endif
#include "hw.h"
#include "hw_internal.h"

static hwObject create( hwObject );
static void addref( hwObject );
static void destroy( hwObject );
static void modify( hwObject, const char *, hwInt32 type, const void * );
static hwInt32 inquire( hwObject, const char *, void ** );
static void draw( hwObject );

static const char
    *propList[] = {
        /* Specific to gauge */
        hwStrPosX, hwStrPosY,
        hwStrWidth, hwStrHeight,
        hwStrScale,     /* scale */
        hwStrLonRange,  /* a0, a1 */
        hwStrColor,     /* fgColor */
        hwStrBackgroundColor, /* bgColor */
        hwStrSpotColor, /* spotColor */
        hwStrNeedleColor,       /* needleColor */
        hwStrGaugeRange,        /* gaugeMin, gaugeMax, gaugeScaleMul */
        hwStrDivisions, /* majorDivs, minorDivs, babyDivs */
        hwStrFontHeight,    /* fontSize */
        hwStrLabel,     /* label */
        hwStrNeedleLength,
        hwStrNeedlePos,
        hwStrOptFlags,  /* optFlags */
        hwStrAlign,

        /* End of list */
        0
    };

#define HW_POS_X        1
#define HW_POS_Y        2
#define HW_WIDTH        3
#define HW_HEIGHT       4
#define HW_SCALE        5
#define HW_LONRANGE     6
#define HW_COLOR        7
#define HW_BGCOLOR      8
#define HW_SPOT         9
#define HW_NEEDLE       10
#define HW_RANGE        11
#define HW_DIVISIONS    12
#define HW_FONT_HEIGHT  13
#define HW_LABEL        14
#define HW_NEEDLE_LEN   15
#define HW_NEEDLE_POS   16
#define HW_OPTFLAGS     17
#define HW_ALIGN        18
#define HW_PARENT       19
#define HW_BOUNDS       20
#define HW_DIRTY        21

static struct _hwObjectStruct
    hwGaugeStruct = {
        0,      /* Parent - NULL */
        "hwGauge",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwGauge = &hwGaugeStruct;

/* Hash table for gauge strings */
static void *gaugeTab = 0;

#define MAX_NEEDLES     2

typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount,               /* Reference count */
        dirty;                  /* Dirty flag */
    hwInt32
        posX, posY,
        width, height,
        align, bounds[4],
        parentBounds[4];
    hwFloat
        cx, cy,
        radius, scale,
        a0, a1,
        fontSize;
    hwInt32
        gaugeMin, gaugeMax, gaugeScaleMul,
        majorDivs, minorDivs, babyDivs;
    hwObject
        parent;
    char
        *label;
    hwInt32
        fgColor, bgColor, spotColor, needleColor,
        numNeedles;
    hwFloat
        needleLengths[MAX_NEEDLES],
        needlePos[MAX_NEEDLES];
    hwInt32
        optFlags;
    hwInt32
        guiList;
} Gauge;

static hwObject create( hwObject gauge )
{
    Gauge
        *result;
    void
        *t;

    if( gauge != hwGauge )      return 0;

    HW_HASH_SETUP(gaugeTab, 24)
        t = gaugeTab;
        HW_INSERT( hwStrPosX,            HW_POS_X,       t );
        HW_INSERT( hwStrPosY,            HW_POS_Y,       t );
        HW_INSERT( hwStrWidth,           HW_WIDTH,       t );
        HW_INSERT( hwStrHeight,          HW_HEIGHT,      t );
        HW_INSERT( hwStrScale,           HW_SCALE,       t );
        HW_INSERT( hwStrLonRange,        HW_LONRANGE,    t );
        HW_INSERT( hwStrColor,           HW_COLOR,       t );
        HW_INSERT( hwStrBackgroundColor, HW_BGCOLOR,     t );
        HW_INSERT( hwStrSpotColor,       HW_SPOT,        t );
        HW_INSERT( hwStrNeedleColor,     HW_NEEDLE,      t );
        HW_INSERT( hwStrGaugeRange,      HW_RANGE,       t );
        HW_INSERT( hwStrDivisions,       HW_DIVISIONS,   t );
        HW_INSERT( hwStrFontHeight,      HW_FONT_HEIGHT, t );
        HW_INSERT( hwStrLabel,           HW_LABEL,       t );
        HW_INSERT( hwStrNeedleLength,    HW_NEEDLE_LEN,  t );
        HW_INSERT( hwStrNeedlePos,       HW_NEEDLE_POS,  t );
        HW_INSERT( hwStrOptFlags,        HW_OPTFLAGS,    t );
        HW_INSERT( hwStrAlign,           HW_ALIGN,       t );
        HW_INSERT( hwStrParent,          HW_PARENT,      t );
        HW_INSERT( hwStrBounds,          HW_BOUNDS,      t );
        HW_INSERT( hwStrDirty,           HW_DIRTY,       t );
    HW_HASH_CLEANUP

    result = malloc( sizeof(Gauge) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }
    memset( result, 0, sizeof(Gauge) );

    /* Class-specific initialization goes here */

    result->hdr = *gauge;
    result->hdr.parent = hwGauge;

    result->refCount = 1;
    result->dirty = 1;
    result->label = 0;
    result->guiList = 0;
    result->align = 0;
    result->parent = NULL;

    result->hdr.name = 0;
    return (hwObject)result;
}

static void addref( hwObject obj )
{
    Gauge
        *gauge = (Gauge *)obj;;

    /* This method only rarely needs to be changed */
    gauge->refCount++;
}

static void destroy( hwObject obj )
{
    Gauge
        *gauge = (Gauge *)obj;

    if( --gauge->refCount > 0 ) return;

    if( gauge->guiList ) {
        __hwIntDestroyGuiList( gauge->guiList );
    }
    if( gauge->label ) free( gauge->label );
    free( gauge );
}

static void modify
(
    hwObject obj, const char *prop,
    hwInt32 type, const void *val
)
{
    Gauge
        *gauge = (Gauge *)obj;
    int
        i, n,
        dirty = 1;      /* Presume that mods make the object dirty */

    switch( hwLookup( prop, gaugeTab ) ) {
    case HW_POS_X :
        if( type == HW_TYPE_1I )        gauge->posX = *(hwInt32 *)val;
        else if( type == HW_TYPE_1F )   gauge->posX = *(hwFloat *)val;
        else                            goto BadType;
        break;
    case HW_POS_Y :
        if( type == HW_TYPE_1I )        gauge->posY = *(hwInt32 *)val;
        else if( type == HW_TYPE_1F )   gauge->posY = *(hwFloat *)val;
        else                            goto BadType;
        break;
    case HW_WIDTH :
        if( type == HW_TYPE_1I )        gauge->width = *(hwInt32 *)val;
        else if( type == HW_TYPE_1F )   gauge->width = *(hwFloat *)val;
        else                            goto BadType;
        break;
    case HW_HEIGHT :
        if( type == HW_TYPE_1I )        gauge->height = *(hwInt32 *)val;
        else if( type == HW_TYPE_1F )   gauge->height = *(hwFloat *)val;
        else                            goto BadType;
        break;
    case HW_ALIGN :
        if( type == HW_TYPE_1I )        gauge->align = *(hwInt32 *)val;
        else if( type == HW_TYPE_1F )   gauge->align = *(hwFloat *)val;
        else                            goto BadType;
        break;
    case HW_SCALE :
        if( type == HW_TYPE_1I )        gauge->scale = *(hwInt32 *)val;
        else if( type == HW_TYPE_1F )   gauge->scale = *(hwFloat *)val;
        else                            goto BadType;
        break;
    case HW_LONRANGE :
        if( type != HW_TYPE_2F )        goto BadType;
        gauge->a0 = ((hwFloat *)val)[0];
        gauge->a1 = ((hwFloat *)val)[1];
        break;
    case HW_COLOR :
        gauge->fgColor = __hwIntGuiColor(type, val);
        break;
    case HW_BGCOLOR :
        gauge->bgColor = __hwIntGuiColor(type, val);
        break;
    case HW_SPOT :
        gauge->spotColor = __hwIntGuiColor(type, val);
        dirty = 0; /* Setting the spot color does not dirty the DL */
        break;
    case HW_NEEDLE :
        gauge->needleColor = __hwIntGuiColor(type, val);
        dirty = 0; /* Setting the needle color does not dirty the DL */
        break;
    case HW_RANGE :
        if( type == HW_TYPE_3I ) {
            gauge->gaugeMin = ((hwInt32 *)val)[0];
            gauge->gaugeMax = ((hwInt32 *)val)[1];
            gauge->gaugeScaleMul = ((hwInt32 *)val)[2];
        }
        else if( type == HW_TYPE_3F ) {
            gauge->gaugeMin = ((hwFloat *)val)[0];
            gauge->gaugeMax = ((hwFloat *)val)[1];
            gauge->gaugeScaleMul = ((hwFloat *)val)[2];
        }
        else {
            goto BadType;
        }
        break;
    case HW_DIVISIONS :
        switch( type ) {
        case HW_TYPE_1I :
            gauge->majorDivs = ((hwInt32 *)val)[0];
            break;
        case HW_TYPE_1F :
            gauge->majorDivs = ((hwFloat *)val)[0];
            break;
        case HW_TYPE_2I :
            gauge->majorDivs = ((hwInt32 *)val)[0];
            gauge->minorDivs = ((hwInt32 *)val)[1];
            break;
        case HW_TYPE_2F :
            gauge->majorDivs = ((hwFloat *)val)[0];
            gauge->minorDivs = ((hwFloat *)val)[1];
            break;
        case HW_TYPE_3I :
            gauge->majorDivs = ((hwInt32 *)val)[0];
            gauge->minorDivs = ((hwInt32 *)val)[1];
            gauge->babyDivs = ((hwInt32 *)val)[2];
            break;
        case HW_TYPE_3F :
            gauge->majorDivs = ((hwFloat *)val)[0];
            gauge->minorDivs = ((hwFloat *)val)[1];
            gauge->babyDivs = ((hwFloat *)val)[2];
            break;
        default :
            goto BadType;
        }
        break;
    case HW_FONT_HEIGHT :
        if( type == HW_TYPE_1I )        gauge->fontSize = *(hwInt32 *)val;
        else if( type == HW_TYPE_1F )   gauge->fontSize = *(hwFloat *)val;
        else                            goto BadType;
        break;
    case HW_LABEL :
        if( type != HW_TYPE_STRING )    goto BadType;
        if( gauge->label ) free( gauge->label );
        gauge->label = malloc( strlen( (char *)val ) + 1 );
        if( !gauge->label ) {
            __hwIntSetError( HW_ERROR_NO_MEMORY );
            break;
        }
        (void)strcpy( gauge->label, (char *)val );
        break;
    case HW_NEEDLE_LEN :
        if( HW_GET_BASE(type) != HW_TYPE_FLOAT ) goto BadType;
        gauge->numNeedles = n = HW_GET_COUNT(type);
        if( n > MAX_NEEDLES ) goto BadType;
        for( i = 0; i < n; i++ ) {
            gauge->needleLengths[i] = ((hwFloat *)val)[i];
        }
        dirty = 0; /* Setting the needle len does not dirty the DL */
        break;
    case HW_NEEDLE_POS :
        if( HW_GET_BASE(type) != HW_TYPE_FLOAT ) goto BadType;
        n = HW_GET_COUNT(type);
        if( n != gauge->numNeedles ) goto BadType;
        for( i = 0; i < n; i++ ) {
            gauge->needlePos[i] = ((hwFloat *)val)[i];
        }
        dirty = 0; /* Setting the needle pos does not dirty the DL */
        break;
    case HW_OPTFLAGS :
        if( type != HW_TYPE_1I )        goto BadType;
        gauge->optFlags = ((hwInt32 *)val)[0];
        break;
    case HW_PARENT :
        if( type != HW_TYPE_OBJECT )    goto BadType;
        gauge->parent = (hwObject)val;
        break;
    case HW_DIRTY :
        break;
    default :
        /* If you don't recognize a string, make sure to set an error */
        __hwIntSetError( HW_ERROR_BAD_PROP );
        break;
    }

    if (dirty) gauge->dirty = 1;
    return;

BadType:
    /* You should check the types of properties, and "goto BadType"
     * if they don't match what you expect
     */
    __hwIntSetError( HW_ERROR_BAD_TYPE );
}

static void cook( Gauge *gauge, hwDisplay disp );

static hwInt32 inquire( hwObject obj, const char *prop, void **val )
{
    Gauge
        *gauge = (Gauge *)obj;
    HW_USE_CURR_DISP;

    if( gauge->dirty ) {
        cook( gauge, __hwDisp );
    }

    switch( hwLookup( prop, gaugeTab ) ) {
    case HW_POS_X :
        *val = &gauge->posX;
        return HW_TYPE_1I;
    case HW_POS_Y :
        *val = &gauge->posY;
        return HW_TYPE_1I;
    case HW_WIDTH :
        *val = &gauge->width;
        return HW_TYPE_1I;
    case HW_HEIGHT :
        *val = &gauge->height;
        return HW_TYPE_1I;
    case HW_SCALE :
        *val = &gauge->scale;
        return HW_TYPE_1F;
    case HW_LONRANGE :
        *val = &gauge->a0;
        return HW_TYPE_1F;
    case HW_COLOR :
        *val = &gauge->fgColor;
        return HW_TYPE_1I;
    case HW_BGCOLOR :
        *val = &gauge->bgColor;
        return HW_TYPE_1I;
    case HW_SPOT :
        *val = &gauge->spotColor;
        return HW_TYPE_1I;
    case HW_NEEDLE :
        *val = &gauge->needleColor;
        return HW_TYPE_1I;
    case HW_RANGE :
        *val = &gauge->gaugeMin;
        return HW_TYPE_3I;
    case HW_DIVISIONS :
        *val = &gauge->majorDivs;
        return HW_TYPE_3I;
    case HW_FONT_HEIGHT :
        *val = &gauge->fontSize;
        return HW_TYPE_1F;
    case HW_LABEL :
        *val = gauge->label;
        return HW_TYPE_STRING;
    case HW_OPTFLAGS :
        *val = &gauge->optFlags;
        return HW_TYPE_1I;
    case HW_ALIGN :
        *val = &gauge->align;
        return HW_TYPE_1I;
    case HW_PARENT :
        *val = gauge->parent;
        return HW_TYPE_OBJECT;
    case HW_BOUNDS :
        /* TBD: Cook it if dirty */
        *val = gauge->bounds;
        return HW_TYPE_4I;
    case HW_NEEDLE_LEN :
        *val = gauge->needleLengths;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT, gauge->numNeedles);
    case HW_NEEDLE_POS :
        *val = gauge->needlePos;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT, gauge->numNeedles);
    default :
        __hwIntSetError( HW_ERROR_BAD_PROP );
        return 0;
    }

    /* NOTREACHED */
    return 0;
}

static void drawGaugeFace(hwDisplay disp, Gauge *gauge)
{
    hwFloat x0, y0, x1, y1, angle, inner, delta, delta0;
    hwFloat sa, ca;
    int i, j, start, exclude, num;
    hwFloat r0, r1;
    TexFont *txf;
    unsigned char *img;
    char gaugeScale[10];
    int n, np, width, maxAsc, maxDesc;
    hwFloat x, y, advance;
    hwInt32 *tics, ntics;
    hwInt32 circle[37*2];
    hwFloat pixScale;
    hwInt32 fontHeight;

    if (gauge->guiList) {
        disp->callGuiList(disp, gauge->guiList);
        return;
    }

    if( gauge->optFlags & HW_OPT_USE_DL ) {
        gauge->guiList = disp->openGuiList(disp);
        if (!gauge->guiList) {
            return;
        }
    }

    r0 = gauge->radius;
    np = 0;
    for (i = 0; i <= 360; i+= 10) {
        sa = -sin(i * M_PI / 180.0); ca = cos(i * M_PI / 180.0);
        x0 = r0 * ca; y0 = r0 * sa;
        circle[2*np  ] = (int)(gauge->cx + x0 + 0.5);
        circle[2*np+1] = (int)(gauge->cy + y0 + 0.5);
        np++;
    }
    disp->guiPolygon(disp, HW_GUI_BLEND, gauge->bgColor, np, circle);

    r0 = gauge->radius;
    np = 0;
    for (i = 0; i <= 360; i+= 10) {
        sa = -sin(i * M_PI / 180.0); ca = cos(i * M_PI / 180.0);
        x0 = r0 * ca; y0 = r0 * sa;
        circle[2*np  ] = (int)(gauge->cx + x0 + 0.5);
        circle[2*np+1] = (int)(gauge->cy + y0 + 0.5);
        np++;
    }
    disp->guiPolyline(disp, HW_GUI_SMOOTH, gauge->fgColor, np, circle);

    pixScale = gauge->radius * gauge->scale / 100.;

    r0 = 3*pixScale;
    np = 0;
    for (i = 0; i <= 360; i+= 10) {
        sa = -sin(i * M_PI / 180.0); ca = cos(i * M_PI / 180.0);
        x0 = r0 * ca; y0 = r0 * sa;
        circle[2*np  ] = (int)(gauge->cx + x0 + 0.5);
        circle[2*np+1] = (int)(gauge->cy + y0 + 0.5);
        np++;
    }
    disp->guiPolygon(disp, HW_GUI_SMOOTH, gauge->spotColor, np, circle);

    ntics = gauge->majorDivs + 1;
    if (gauge->minorDivs > ntics) {
        ntics = gauge->minorDivs;
    }
    if (gauge->babyDivs > ntics) {
        ntics = gauge->babyDivs;
    }

    tics = malloc(ntics * 4 * sizeof(hwInt32));
    if (!tics) {
        return;
    }

    /* Major tics */
    delta0 = (gauge->a0 - gauge->a1) / gauge->majorDivs;
    r0 = gauge->radius - 5*pixScale;
    r1 = gauge->radius - 1*pixScale;
    angle = gauge->a0;
    np = 0;
    for (i = 0; i <= gauge->majorDivs; i++) {
        sa = -sin(angle * M_PI / 180.); ca = cos(angle * M_PI / 180.);
        x0 = r0 * ca; y0 = r0 * sa;
        x1 = r1 * ca; y1 = r1 * sa;
        tics[2*np  ] = (int)(gauge->cx + x0 + 0.5);
        tics[2*np+1] = (int)(gauge->cy + y0 + 0.5);
        tics[2*np+2] = (int)(gauge->cx + x1 + 0.5);
        tics[2*np+3] = (int)(gauge->cy + y1 + 0.5);
        np += 2;
        angle -= delta0;
    }
    disp->guiLines(disp, HW_GUI_SMOOTH, gauge->fgColor, np, tics);

    /* Minor tics */
    if (gauge->minorDivs) {
        delta = delta0 / gauge->minorDivs;
        r0 = gauge->radius - 3*pixScale;
        r1 = gauge->radius - 1*pixScale;
        angle = gauge->a0;
        for (i = 0; i < gauge->majorDivs; i++) {
            inner = angle - delta;
            np = 0;
            for (j = 1; j < gauge->minorDivs; j++) {
                sa = -sin(inner*M_PI/180.); ca = cos(inner*M_PI/180.);
                x0 = r0 * ca; y0 = r0 * sa;
                x1 = r1 * ca; y1 = r1 * sa;
                tics[2*np  ] = (int)(gauge->cx + x0 + 0.5);
                tics[2*np+1] = (int)(gauge->cy + y0 + 0.5);
                tics[2*np+2] = (int)(gauge->cx + x1 + 0.5);
                tics[2*np+3] = (int)(gauge->cy + y1 + 0.5);
                np += 2;
                inner -= delta;
            }
            disp->guiLines(disp, HW_GUI_SMOOTH, gauge->fgColor, np, tics);
            angle -= delta0;
        }
    }

    /* Baby tics */
    if (gauge->babyDivs) {
        if (gauge->minorDivs) {
            exclude = gauge->babyDivs / gauge->minorDivs;
        } else {
            exclude = 0;
        }

        delta = delta0 / gauge->babyDivs;
        r0 = gauge->radius - 2*pixScale;
        r1 = gauge->radius - 1*pixScale;
        angle = gauge->a0;
        for (i = 0; i < gauge->majorDivs; i++) {
            inner = angle - delta;
            np = 0;
            for (j = 1; j < gauge->babyDivs; j++) {
                if ((!exclude) || (j % exclude) != 0) {
                    sa = -sin(inner*M_PI/180.); ca = cos(inner*M_PI/180.);
                    x0 = r0 * ca; y0 = r0 * sa;
                    x1 = r1 * ca; y1 = r1 * sa;
                    tics[2*np  ] = (int)(gauge->cx + x0 + 0.5);
                    tics[2*np+1] = (int)(gauge->cy + y0 + 0.5);
                    tics[2*np+2] = (int)(gauge->cx + x1 + 0.5);
                    tics[2*np+3] = (int)(gauge->cy + y1 + 0.5);
                    np += 2;
                }
                inner -= delta;
            }
            disp->guiLines(disp, HW_GUI_SMOOTH, gauge->fgColor, np, tics);
            angle -= delta0;
        }
    }

    fontHeight = (hwInt32)(gauge->fontSize * gauge->radius / 100. + 0.5);
    /* Label */
    if (gauge->label) {
        txf = txfLoadStaticFont(fontHeight);

        disp->guiText(disp, txf, gauge->fgColor,
                      HW_TEXT_ALIGN_CENTER,
                      HW_TEXT_ALIGN_CENTER,
                      fontHeight,
                      (int)gauge->cx,
                      (int)(gauge->cy + gauge->radius/3),
                      (unsigned char *)gauge->label);
#if 0
        if (gauge->label1) {
            disp->guiText(disp, txf, gauge->fgColor,
                          HW_TEXT_ALIGN_CENTER,
                          HW_TEXT_ALIGN_CENTER,
                          fontHeight,
                          (int)gauge->cx,
                          (int)(gauge->cy + gauge->radius/3 + gauge->fontSize),
                          (unsigned char *)gauge->label1);
        }
#endif
    }

    if (gauge->gaugeScaleMul) {
        txf = txfLoadStaticFont(fontHeight);

        delta0 = gauge->a0 - gauge->a1;

        if( (delta0 >= 359.5) || (delta0 <= -359.5) ) {
            /* Full circle - don't draw the beginning */
            start = 1;
        }
        else {
            /* Partial arc */
            start = 0;
        }
        
        delta0 = delta0 / gauge->majorDivs;
        r0 = gauge->radius - 10*pixScale;
        angle = gauge->a0 - start * delta0;
        for (i = start; i <= gauge->majorDivs; i++) {
            sa = -sin(angle * M_PI / 180.); ca = cos(angle * M_PI / 180.);
            x0 = r0 * ca; y0 = r0 * sa;

            num = gauge->gaugeScaleMul * i;
            sprintf(gaugeScale, "%d", num);

            disp->guiText(disp, txf, gauge->fgColor,
                          HW_TEXT_ALIGN_CENTER,
                          HW_TEXT_ALIGN_CENTER,
                          fontHeight,
                          (int)(gauge->cx + x0 + 0.5),
                          (int)(gauge->cy + y0 + 0.5),
                          (unsigned char *)gaugeScale);

            angle -= delta0;
        }
    }
    free(tics);

    if( gauge->optFlags & HW_OPT_USE_DL ) {
        disp->closeGuiList(disp);
        disp->callGuiList(disp, gauge->guiList);
    }
}

static void drawNeedle(hwDisplay disp, int speed, hwFloat length, Gauge *gauge)
{
#define DF      2
    hwFloat x0, y0, x1, y1;
    hwFloat sa, ca;
    int j, r0, r1, ix, iy;
    hwFloat angle, delta;
    hwInt32 tri[6];

    if (speed < gauge->gaugeMin) {
        speed = gauge->gaugeMin;
    }
    if (speed > gauge->gaugeMax) {
        speed= gauge->gaugeMax;
    }

    delta = gauge->a0 - gauge->a1;
    angle = gauge->a0 - (delta * (long)speed) / gauge->gaugeMax;

    r0 = length * gauge->radius / 100.;
    r1 = 5*gauge->radius*gauge->scale / 100.;

    sa = -sin(angle*M_PI/180.); ca = cos(angle*M_PI/180.);
    x0 = r0 * ca; y0 = r0 * sa;
    tri[0] = gauge->cx + x0 + 0.5;
    tri[1] = gauge->cy + y0 + 0.5;

    sa = -sin((angle+165.)*M_PI/180.); ca = cos((angle+165.)*M_PI/180.);
    x0 = r1 * ca; y0 = r1 * sa;
    tri[2] = gauge->cx + x0 + 0.5;
    tri[3] = gauge->cy + y0 + 0.5;

    sa = -sin((angle+195.)*M_PI/180.); ca = cos((angle+195.) * M_PI/180.);
    x0 = r1 * ca; y0 = r1 * sa;
    tri[4] = gauge->cx + x0 + 0.5;
    tri[5] = gauge->cy + y0 + 0.5;

    disp->guiPolygon(disp, HW_GUI_SMOOTH, gauge->needleColor, 3, tri);

#undef DF
}

static void cook( Gauge *gauge, hwDisplay disp )
{
    if( gauge->guiList ) {
        disp->destroyGuiList( disp, gauge->guiList );
        gauge->guiList = 0;
    }
    __hwIntGetParentBounds( disp, gauge->parent, gauge->parentBounds );

    __hwIntPositionWidget( gauge->bounds, gauge->parentBounds,
                           gauge->posX, gauge->posY,
                           gauge->width, gauge->height,
                           gauge->align,
                           0.0 );

    gauge->cx = gauge->bounds[0] + gauge->bounds[2] / 2;
    gauge->cy = gauge->bounds[1] + gauge->bounds[3] / 2;

    gauge->cx += gauge->parentBounds[0];
    gauge->cy += gauge->parentBounds[1];

    if( gauge->bounds[2] > gauge->bounds[3] ) {
        gauge->radius = gauge->bounds[3] / 2;
    }
    else {
        gauge->radius = gauge->bounds[2] / 2;
    }

    gauge->dirty = 0;
}

static void draw( hwObject obj )
{
    Gauge
        *gauge = (Gauge *)obj;
    int
        i;
    HW_USE_CURR_DISP;

    if( gauge->dirty ) {
        cook( gauge, __hwDisp );
    }
    drawGaugeFace( __hwDisp, gauge );
    for( i = 0; i < gauge->numNeedles; i++ ) {
        drawNeedle( __hwDisp, (int)gauge->needlePos[i], gauge->needleLengths[i],
                    gauge );
    }
}
/*** EOF hwGauge.c ***/
