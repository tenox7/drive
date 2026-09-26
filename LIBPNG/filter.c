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
 * Filter functions
 *****************************************************************************/

/* Note: a,b,c,x are defined as in the PNG specification:
 *
 *   c b
 *   a x
 *
 * If you are at row 0, c and b are both 0
 * If you are at col 0, a and c are both 0
 */
#define RECON_SUB(a,b,c,x)      ((x)+(a))
#define RECON_UP(a,b,c,x)       ((x)+(b))
#define RECON_AVG(a,b,c,x)      ((x) + (((a) + (b)) >> 1))
#define RECON_PAETH(a,b,c,x)    ((x) + PaethPredict(a,b,c))

static int PaethPredict(int a, int b, int c)
{
    int p, pa, pb, pc;
    p = a + b - c;
    pa = p - a;  if (pa < 0) pa = -pa;
    pb = p - b;  if (pb < 0) pb = -pb;
    pc = p - c;  if (pc < 0) pc = -pc;
    if ((pa <= pb) && (pa <= pc)) return a;
    else if (pb <= pc) return b;
    else return c;
}

static void recon_sub(int w, int n, PNG_U8 *prev, PNG_U8 *curr)
{
    int i;

    if (prev) {
        for (i = 0; i < n; i++) {
            curr[i] = RECON_SUB(        0, prev[i],         0, curr[i]);
        }
        for (; i < w; i++) {
            curr[i] = RECON_SUB(curr[i-n], prev[i], prev[i-n], curr[i]);
        }
    }
    else {
        for (i = 0; i < n; i++) {
            curr[i] = RECON_SUB(        0,       0,         0, curr[i]);
        }
        for (; i < w; i++) {
            curr[i] = RECON_SUB(curr[i-n],       0,         0, curr[i]);
        }
    }
}

static void recon_up(int w, int n, PNG_U8 *prev, PNG_U8 *curr)
{
    int i;

    if (prev) {
        for (i = 0; i < n; i++) {
            curr[i] = RECON_UP(        0, prev[i],         0, curr[i]);
        }
        for (; i < w; i++) {
            curr[i] = RECON_UP(curr[i-n], prev[i], prev[i-n], curr[i]);
        }
    }
    else {
        for (i = 0; i < n; i++) {
            curr[i] = RECON_UP(        0,       0,         0, curr[i]);
        }
        for (; i < w; i++) {
            curr[i] = RECON_UP(curr[i-n],       0,         0, curr[i]);
        }
    }
}

static void recon_avg(int w, int n, PNG_U8 *prev, PNG_U8 *curr)
{
    int i;

    if (prev) {
        for (i = 0; i < n; i++) {
            curr[i] = RECON_AVG(        0, prev[i],         0, curr[i]);
        }
        for (; i < w; i++) {
            curr[i] = RECON_AVG(curr[i-n], prev[i], prev[i-n], curr[i]);
        }
    }
    else {
        for (i = 0; i < n; i++) {
            curr[i] = RECON_AVG(        0,       0,         0, curr[i]);
        }
        for (; i < w; i++) {
            curr[i] = RECON_AVG(curr[i-n],       0,         0, curr[i]);
        }
    }
}

static void recon_paeth(int w, int n, PNG_U8 *prev, PNG_U8 *curr)
{
    int i;

    if (prev) {
        for (i = 0; i < n; i++) {
            curr[i] = RECON_PAETH(        0, prev[i],         0, curr[i]);
        }
        for (; i < w; i++) {
            curr[i] = RECON_PAETH(curr[i-n], prev[i], prev[i-n], curr[i]);
        }
    }
    else {
        for (i = 0; i < n; i++) {
            curr[i] = RECON_PAETH(        0,       0,         0, curr[i]);
        }
        for (; i < w; i++) {
            curr[i] = RECON_PAETH(curr[i-n],       0,         0, curr[i]);
        }
    }
}
