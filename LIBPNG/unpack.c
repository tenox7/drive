/*
 * Copyright (c) 2012 Ross Cunniff
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


/******************************************************************************
 * Unpacking functions
 *****************************************************************************/

static void u_I1_RGB(int w, int s, PNG_U8 *dst, PNG_U8 *plte, PNG_U8 *src)
{
    int i, t;

    for (i = 0; i < w; i++) {
        t = src[i/8] & (0x80 >> (i % 8));
        dst[0] = plte[4*t  ];
        dst[1] = plte[4*t+1];
        dst[2] = plte[4*t+2];
        dst += s;
    }
}

static void u_I1_I8(int w, int s, PNG_U8 *dst, PNG_U8 *plte, PNG_U8 *src)
{
    int i, t;

    for (i = 0; i < w; i++) {
        t = src[i/8] & (0x80 >> (i % 8));
        dst[0] = t;
        dst += s;
    }
}

static void u_I1_RGBA(int w, int s, PNG_U8 *dst, PNG_U8 *plte, PNG_U8 *src)
{
    int i, t;

    for (i = 0; i < w; i++) {
        t = src[i/8] & (0x80 >> (i % 8));
        dst[0] = plte[4*t  ];
        dst[1] = plte[4*t+1];
        dst[2] = plte[4*t+2];
        dst[3] = plte[4*t+3];
        dst += s;
    }
}

static void u_L1_L(int w, int s, PNG_U8 *dst, PNG_U8 *xxxx, PNG_U8 *src)
{
    int i, t;

    for (i = 0; i < w; i++) {
        t = src[i/8] & (0x80 >> (i % 8));
        dst[0] = t ? 0xFF : 0x00;
        dst += s;
    }
}

static void u_L1_LA(int w, int s, PNG_U8 *dst, PNG_U8 *trns, PNG_U8 *src)
{
    int i, t;
    int tl = (trns[1] & 1);

    for (i = 0; i < w; i++) {
        t = src[i/8] & (0x80 >> (i % 8));
        dst[0] = t ? 0xFF : 0x00;
        dst[1] = (t == tl) ? 0x00 : 0xFF;
        dst += s;
    }
}

static void u_I2_RGB(int w, int s, PNG_U8 *dst, PNG_U8 *plte, PNG_U8 *src)
{
    int i, t;

    for (i = 0; i < w; i++) {
        t = src[i/4] & (0xC0 >> (2*(i % 4)));
        dst[0] = plte[4*t  ];
        dst[1] = plte[4*t+1];
        dst[2] = plte[4*t+2];
        dst += s;
    }
}

static void u_I2_I8(int w, int s, PNG_U8 *dst, PNG_U8 *plte, PNG_U8 *src)
{
    int i, t;

    for (i = 0; i < w; i++) {
        t = src[i/4] & (0xC0 >> (2*(i % 4)));
        dst[0] = t;
        dst += s;
    }
}

static void u_I2_RGBA(int w, int s, PNG_U8 *dst, PNG_U8 *plte, PNG_U8 *src)
{
    int i, t;

    for (i = 0; i < w; i++) {
        t = src[i/4] & (0xC0 >> (2*(i % 4)));
        dst[0] = plte[4*t  ];
        dst[1] = plte[4*t+1];
        dst[2] = plte[4*t+2];
        dst[3] = plte[4*t+3];
        dst += s;
    }
}

static void u_L2_L(int w, int s, PNG_U8 *dst, PNG_U8 *xxxx, PNG_U8 *src)
{
    int i, t;

    for (i = 0; i < w; i++) {
        t = src[i/4] & (0xC0 >> (2*(i % 4)));
        dst[0] = (t << 6) | (t << 4) | (t << 2) | t;
        dst += s;
    }
}

static void u_L2_LA(int w, int s, PNG_U8 *dst, PNG_U8 *trns, PNG_U8 *src)
{
    int i, t;
    int tl = (trns[1] & 0x03);

    for (i = 0; i < w; i++) {
        t = src[i/4] & (0xC0 >> (2*(i % 4)));
        dst[0] = (t << 6) | (t << 4) | (t << 2) | t;
        dst[1] = (t == tl) ? 0x00 : 0xFF;
        dst += s;
    }
}

static void u_I4_RGB(int w, int s, PNG_U8 *dst, PNG_U8 *plte, PNG_U8 *src)
{
    int i, t;

    for (i = 0; i < w; i++) {
        t = src[i/2] & (0xF0 >> (4*(i % 2)));
        dst[0] = plte[4*t  ];
        dst[1] = plte[4*t+1];
        dst[2] = plte[4*t+2];
        dst += s;
    }
}

static void u_I4_I8(int w, int s, PNG_U8 *dst, PNG_U8 *plte, PNG_U8 *src)
{
    int i, t;

    for (i = 0; i < w; i++) {
        t = src[i/2] & (0xF0 >> (4*(i % 2)));
        dst[0] = t;
        dst += s;
    }
}

static void u_I4_RGBA(int w, int s, PNG_U8 *dst, PNG_U8 *plte, PNG_U8 *src)
{
    int i, t;

    for (i = 0; i < w; i++) {
        t = src[i/2] & (0xF0 >> (4*(i % 2)));
        dst[0] = plte[4*t  ];
        dst[1] = plte[4*t+1];
        dst[2] = plte[4*t+2];
        dst[3] = plte[4*t+3];
        dst += s;
    }
}

static void u_L4_L(int w, int s, PNG_U8 *dst, PNG_U8 *xxxx, PNG_U8 *src)
{
    int i, t;

    for (i = 0; i < w; i++) {
        t = src[i/2] & (0xF0 >> (4*(i % 2)));
        dst[0] = (t << 4) | t;
        dst += s;
    }
}

static void u_L4_LA(int w, int s, PNG_U8 *dst, PNG_U8 *trns, PNG_U8 *src)
{
    int i, t;
    int tl = (trns[1] & 0x0F);

    for (i = 0; i < w; i++) {
        t = src[i/2] & (0xF0 >> (4*(i % 2)));
        dst[0] = (t << 4) | t;
        dst[1] = (t == tl) ? 0x00 : 0xFF;
        dst += s;
    }
}

static void u_I8_RGB(int w, int s, PNG_U8 *dst, PNG_U8 *plte, PNG_U8 *src)
{
    int i, t;

    for (i = 0; i < w; i++) {
        t = src[i];
        dst[0] = plte[4*t  ];
        dst[1] = plte[4*t+1];
        dst[2] = plte[4*t+2];
        dst += s;
    }
}

static void u_I8_I8(int w, int s, PNG_U8 *dst, PNG_U8 *plte, PNG_U8 *src)
{
    int i, t;

    for (i = 0; i < w; i++) {
        t = src[i];
        dst[0] = t;
        dst += s;
    }
}

static void u_I8_RGBA(int w, int s, PNG_U8 *dst, PNG_U8 *plte, PNG_U8 *src)
{
    int i, t;

    for (i = 0; i < w; i++) {
        t = src[i];
        dst[0] = plte[4*t  ];
        dst[1] = plte[4*t+1];
        dst[2] = plte[4*t+2];
        dst[3] = plte[4*t+3];
        dst += s;
    }
}

static void u_L_L(int w, int s, PNG_U8 *dst, PNG_U8 *xxxx, PNG_U8 *src)
{
    int i;

    for (i = 0; i < w; i++) {
        dst[0] = src[i];
        dst += s;
    }
}

static void u_L_LA(int w, int s, PNG_U8 *dst, PNG_U8 *trns, PNG_U8 *src)
{
    int i;
    int tl = trns[1];

    for (i = 0; i < w; i++) {
        dst[0] = src[i];
        dst[1] = (src[i] == tl) ? 0x00 : 0xFF;
        dst += s;
    }
}

static void u_LA_LA(int w, int s, PNG_U8 *dst, PNG_U8 *xxxx, PNG_U8 *src)
{
    int i;

    for (i = 0; i < w; i++) {
        dst[0] = src[2*i  ];
        dst[1] = src[2*i+1];
        dst += s;
    }
}

static void u_RGB_RGB(int w, int s, PNG_U8 *dst, PNG_U8 *xxxx, PNG_U8 *src)
{
    int i;

    for (i = 0; i < w; i++) {
        dst[0] = src[3*i  ];
        dst[1] = src[3*i+1];
        dst[2] = src[3*i+2];
        dst += s;
    }
}

static void u_RGB_RGBA(int w, int s, PNG_U8 *dst, PNG_U8 *trns, PNG_U8 *src)
{
    int i;
    int r, tr = trns[1];
    int g, tg = trns[3];
    int b, tb = trns[5];

    for (i = 0; i < w; i++) {
        dst[0] = r = src[3*i  ];
        dst[1] = g = src[3*i+1];
        dst[2] = b = src[3*i+2];
        dst[3] = ((r == tr) && (g == tg) && (b == tb)) ? 0x00 : 0xFF;
        dst += s;
    }
}

static void u_RGBA_RGBA(int w, int s, PNG_U8 *dst, PNG_U8 *xxxx, PNG_U8 *src)
{
    int i;

    for (i = 0; i < w; i++) {
        dst[0] = src[4*i  ];
        dst[1] = src[4*i+1];
        dst[2] = src[4*i+2];
        dst[3] = src[4*i+3];
        dst += s;
    }
}

static void u_L16_L(int w, int s, PNG_U8 *dst, PNG_U8 *xxxx, PNG_U8 *src)
{
    int i;

    for (i = 0; i < w; i++) {
        dst[0] = src[2*i]; /* Throw away LOB */
        dst += s;
    }
}

static void u_L16_L_16(int w, int s, PNG_U8 *dst, PNG_U8 *xxxx, PNG_U8 *src)
{
    int i;

    for (i = 0; i < w; i++) {
#ifdef PNG_LITTLE_ENDIAN
        dst[0] = src[2*i+1];
        dst[1] = src[2*i];
#else
        dst[0] = src[2*i];
        dst[1] = src[2*i+1];
#endif
        dst += s;
    }
}

static void u_L16_LA(int w, int s, PNG_U8 *dst, PNG_U8 *trns, PNG_U8 *src)
{
    int i;
    PNG_U16 l, tl = (trns[0] << 8) | trns[1];

    for (i = 0; i < w; i++) {
        l = (src[2*i] << 8) | src[2*i+1];
        dst[0] = l >> 8;
        dst[1] = (l == tl) ? 0x00 : 0xFF;
        dst += s;
    }
}

static void u_L16_LA_16(int w, int s, PNG_U8 *dst, PNG_U8 *trns, PNG_U8 *src)
{
    int i;
    PNG_U16 l, tl = (trns[0] << 8) | trns[1];

    for (i = 0; i < w; i++) {
        l = (src[2*i] << 8) | src[2*i+1];
#ifdef PNG_LITTLE_ENDIAN
        dst[0] = l & 0xFF;
        dst[1] = l >> 8;
        dst[2] = (l == tl) ? 0x00 : 0xFF;
        dst[3] = (l == tl) ? 0x00 : 0xFF;
#else
        dst[0] = l >> 8;
        dst[1] = l & 0xFF;
        dst[2] = (l == tl) ? 0x00 : 0xFF;
        dst[3] = (l == tl) ? 0x00 : 0xFF;
#endif
        dst += s;
    }
}

static void u_LA16_LA(int w, int s, PNG_U8 *dst, PNG_U8 *xxxx, PNG_U8 *src)
{
    int i;

    for (i = 0; i < w; i++) {
        dst[0] = src[4*i];   /* Throw away LOB */
        dst[1] = src[4*i+2]; /* Throw away LOB */
        dst += s;
    }
}

static void u_LA16_LA_16(int w, int s, PNG_U8 *dst, PNG_U8 *xxxx, PNG_U8 *src)
{
    int i;

    for (i = 0; i < w; i++) {
#ifdef PNG_LITTLE_ENDIAN
        dst[0] = src[4*i+1];
        dst[1] = src[4*i];
        dst[2] = src[4*i+3];
        dst[3] = src[4*i+2];
#else
        dst[0] = src[4*i];
        dst[1] = src[4*i+1];
        dst[2] = src[4*i+2];
        dst[3] = src[4*i+3];
#endif
        dst += s;
    }
}

static void u_RGB16_RGB(int w, int s, PNG_U8 *dst, PNG_U8 *xxxx, PNG_U8 *src)
{
    int i;

    for (i = 0; i < w; i++) {
        dst[0] = src[6*i];   /* Throw away LOB */
        dst[1] = src[6*i+2]; /* Throw away LOB */
        dst[2] = src[6*i+4]; /* Throw away LOB */
        dst += s;
    }
}

static void u_RGB16_RGB_16(int w, int s, PNG_U8 *dst, PNG_U8 *xxxx, PNG_U8 *src)
{
    int i;

    for (i = 0; i < w; i++) {
#ifdef PNG_LITTLE_ENDIAN
        dst[0] = src[6*i+1];
        dst[1] = src[6*i];
        dst[2] = src[6*i+3];
        dst[3] = src[6*i+2];
        dst[4] = src[6*i+5];
        dst[5] = src[6*i+4];
#else
        dst[0] = src[6*i];
        dst[1] = src[6*i+1];
        dst[2] = src[6*i+2];
        dst[3] = src[6*i+3];
        dst[4] = src[6*i+4];
        dst[5] = src[6*i+5];
#endif
        dst += s;
    }
}

static void u_RGB16_RGBA(int w, int s, PNG_U8 *dst, PNG_U8 *trns, PNG_U8 *src)
{
    int i;
    PNG_U16 r, tr = (trns[0] << 8) | trns[1];
    PNG_U16 g, tg = (trns[2] << 8) | trns[3];
    PNG_U16 b, tb = (trns[4] << 8) | trns[5];

    for (i = 0; i < w; i++) {
        r = (src[6*i  ] << 8) | src[6*i+1];
        g = (src[6*i+2] << 8) | src[6*i+3];
        b = (src[6*i+4] << 8) | src[6*i+5];
        dst[0] = r >> 8; /* Throw away LOB */
        dst[1] = g >> 8; /* Throw away LOB */
        dst[2] = b >> 8; /* Throw away LOB */
        dst[3] = ((r == tr) && (g == tg) && (b == tb)) ? 0x00 : 0xFF;
        dst += s;
    }
}

static void u_RGB16_RGBA_16(int w, int s, PNG_U8 *dst, PNG_U8 *trns, PNG_U8 *src)
{
    int i;
    PNG_U16 r, tr = (trns[0] << 8) | trns[1];
    PNG_U16 g, tg = (trns[2] << 8) | trns[3];
    PNG_U16 b, tb = (trns[4] << 8) | trns[5];

    for (i = 0; i < w; i++) {
        r = (src[6*i  ] << 8) | src[6*i+1];
        g = (src[6*i+2] << 8) | src[6*i+3];
        b = (src[6*i+4] << 8) | src[6*i+5];
#ifdef PNG_LITTLE_ENDIAN
        dst[0] = r & 0xFF;
        dst[1] = r >> 8;
        dst[2] = g & 0xFF;
        dst[3] = g >> 8;
        dst[4] = b & 0xFF;
        dst[5] = b >> 8;
#else
        dst[0] = r >> 8;
        dst[1] = r & 0xFF;
        dst[2] = g >> 8;
        dst[3] = g & 0xFF;
        dst[4] = b >> 8;
        dst[5] = b & 0xFF;
#endif
        dst[6] = ((r == tr) && (g == tg) && (b == tb)) ? 0x00 : 0xFF;
        dst[7] = ((r == tr) && (g == tg) && (b == tb)) ? 0x00 : 0xFF;
        dst += s;
    }
}

static void u_RGBA16_RGBA(int w, int s, PNG_U8 *dst, PNG_U8 *xxxx, PNG_U8 *src)
{
    int i;

    for (i = 0; i < w; i++) {
        dst[0] = src[8*i];   /* Throw away LOB */
        dst[1] = src[8*i+2]; /* Throw away LOB */
        dst[2] = src[8*i+4]; /* Throw away LOB */
        dst[3] = src[8*i+6]; /* Throw away LOB */
        dst += s;
    }
}

static void u_RGBA16_RGBA_16(int w, int s, PNG_U8 *dst, PNG_U8 *xxxx, PNG_U8 *src)
{
    int i;

    for (i = 0; i < w; i++) {
#ifdef PNG_LITTLE_ENDIAN
        dst[0] = src[8*i+1];
        dst[1] = src[8*i];
        dst[2] = src[8*i+3];
        dst[3] = src[8*i+2];
        dst[4] = src[8*i+5];
        dst[5] = src[8*i+4];
        dst[6] = src[8*i+7];
        dst[7] = src[8*i+6];
#else
        dst[0] = src[8*i];
        dst[1] = src[8*i+1];
        dst[2] = src[8*i+2];
        dst[3] = src[8*i+3];
        dst[4] = src[8*i+4];
        dst[5] = src[8*i+5];
        dst[6] = src[8*i+6];
        dst[7] = src[8*i+7];
#endif
        dst += s;
    }
}
