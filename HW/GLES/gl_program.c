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
#include <math.h>
#include <stdarg.h>

/* Define this or none of our gl* calls will work: */
#define HWGL_LIGHT_IMP 1

#include "hw.h"
#include "hw_internal.h"
#include "hw_gl.h"
#include "hw_vecmath.h"

static void rewindString(GlProgSource *ps);
static int addStr(GlProgSource *ps, const char *str);
static int addFmt(GlProgSource *ps, const char *fmt, ...);

#define STR(a)      do { if (!addStr(ps,a))     return 0; } while (0)
#define FMT(a,b)    do { if (!addFmt(ps,a,b))   return 0; } while (0)
#define FMT2(a,b,c) do { if (!addFmt(ps,a,b,c)) return 0; } while (0)
#define FMT3(a,b,c,d) do { if (!addFmt(ps,a,b,c,d)) return 0; } while (0)

#define PS_INITIAL_SIZE         2048
#define PS_INITIAL_LINES        256

#ifdef ANDROID
static const char
    GLSL_VERSION[] = "100",
    PREC_H[] = "highp   ",
    PREC_M[] = "mediump ",
    PREC_L[] = "lowp    ";
#else
static const char
    GLSL_VERSION[] = "120",
    PREC_H[] = "",
    PREC_M[] = "",
    PREC_L[] = "";
#endif

int __hwGlInitProgSource(GlProgSource *ps)
{
    ps->startPtr = malloc(PS_INITIAL_SIZE);
    ps->lines = malloc(PS_INITIAL_LINES*sizeof(char *));
    if (!(ps->startPtr && ps->lines)) return 0;

    ps->currPtr = ps->startPtr;
    ps->endPtr = ps->startPtr + PS_INITIAL_SIZE;
    ps->numLines = 0;

    ps->allocSize = PS_INITIAL_SIZE;
    ps->allocLines = PS_INITIAL_LINES;
    return 1;
}

#define ENA(a)          IS_COOKED(ENA_##a)
#define ENA_OFFS(a,i)   IS_COOKED(ENA_##a + i)
#define ENA_ANY(e0,e1)  IS_ANY_COOKED(ENA_##e0, ENA_##e1)

static int declareLightUniforms(GlProgSource *ps, hwProgInfo *ppi,
                                int isFragment)
{
    int i;
    const char *prec;
    const char *colPrec;

    prec = isFragment ? PREC_M : PREC_H;
    colPrec = isFragment ? PREC_L : PREC_H;

    for (i = 0; i < MAX_LIGHTS; i++) {
        if (!ENA_OFFS(LIGHT0,i)) continue;

        FMT2("uniform %svec4 uLightPos_%d;\n", prec, i);
        FMT2("uniform %svec4 uAmbient_%d;\n", colPrec, i);
        FMT2("uniform %svec4 uDiffuse_%d;\n", colPrec, i);
        FMT2("uniform %svec4 uSpecular_%d;\n", colPrec, i);

        if (ENA_OFFS(LIGHT0_SPOT,i)) {
            FMT2("uniform %svec3 uSpotDir_%d;\n", prec, i);
            FMT2("uniform %sfloat uSpotCutoff_%d;\n", prec, i);
            FMT2("uniform %sfloat uSpotExp_%d;\n", prec, i);
        }

        if (ENA_OFFS(LIGHT0_ATT,i)) {
            FMT2("uniform %sfloat uLinearAtten_%d;\n", prec, i);
        }
    }

    FMT("uniform %svec4 uMatEmission;\n", colPrec);
    FMT("uniform %svec4 uMatSpecular;\n", colPrec);
    FMT("uniform %sfloat uMatShininess;\n", prec);
    FMT("uniform %svec4 uGlobalAmbient;\n", colPrec);

    return 1;
}

static int declareLightLocals(GlProgSource *ps, hwProgInfo *ppi,
                              int isFragment)
{
    const char *prec;
    const char *colPrec;

    prec = isFragment ? PREC_M : PREC_H;
    colPrec = isFragment ? PREC_L : PREC_H;

    FMT("    %svec4 lightColor;\n", colPrec);
    if (ENA(COLOR_SUM) || isFragment) {
        FMT("    %svec4 specColor = vec4(0.,0.,0.,0.);\n", colPrec);
    }
    FMT("    %svec3 lightDir;\n", prec);
    FMT("    %svec3 halfAngle;\n", prec);
    FMT("    %sfloat diffAmt, specAmt;\n", prec);
    if (isFragment) {
        FMT("    %svec3 eyeDir;\n", prec);
        FMT("    %svec3 tmpDir;\n", prec);
    }
    else {
        FMT("    const %svec3 eyeDir = vec3(0.,0.,1.);\n", prec);
    }
    if (isFragment && ENA_ANY(TEX0_NORMAL, TEX7_RELIEF)) {
        FMT("    %svec3 tNormal;\n", colPrec);
    }
    return 1;
}

static int textureMap(GlProgSource *ps, hwProgInfo *ppi, char *fmt, int i)
{
    char tex[128];
    int tc;

    tc = ENA_OFFS(TEXGEN0_STAGE_0, i) ? 0 : i;

    if (ENA_OFFS(TEX0_PROJ, i)) {
#ifdef WIN32
        sprintf(tex, "texture2DProj(uTexture%d, vTexCoord%d)", i, tc);
#else
        snprintf(tex, 128, "texture2DProj(uTexture%d, vTexCoord%d)", i, tc);
#endif
    }
    else if (ENA_OFFS(TEX0_CUBE, i)) {
#ifdef WIN32
        sprintf(tex, "textureCube(uTexture%d, vTexCoord%d)", i, tc);
#else
        snprintf(tex, 128, "textureCube(uTexture%d, vTexCoord%d)", i, tc);
#endif
    }
    else {
#ifdef WIN32
        sprintf(tex, "texture2D(uTexture%d, vTexCoord%d)", i, tc);
#else
        snprintf(tex, 128, "texture2D(uTexture%d, vTexCoord%d)", i, tc);
#endif
    }
    FMT(fmt, tex);
    return 1;
}

static int performLighting(GlProgSource *ps, hwProgInfo *ppi,
                           const char *colInp, int isFragment)
{
    int i;
    const char *normal;

    if (isFragment && ENA_ANY(TEX0_NORMAL, TEX7_RELIEF)) {
        normal = "tNormal";
        for (i = 0; i < MAX_TEX; i++) {
            if (ENA_OFFS(TEX0_NORMAL, i) || ENA_OFFS(TEX0_RELIEF, i)) {
                textureMap(ps, ppi, "    tNormal = %s.xyz;\n", i);
                break;
            }
        }
        STR("    tNormal = vec3(2.0)*tNormal - vec3(1.0);\n");
        STR("    tNormal = normalize(tNormal);\n");
    }
    else {
        normal = "normal";
    }

    if (isFragment && ENA_ANY(LIGHT0, LIGHT7)) {
        STR("    eyeDir.x = tangent.z;\n");
        STR("    eyeDir.y = binormal.z;\n");
        STR("    eyeDir.z = normal.z;\n");
        STR("    eyeDir = normalize(eyeDir);\n");
        STR("\n");
    }

    FMT("    lightColor.a = %s.a;\n", colInp);
    STR("    lightColor.rgb = uMatEmission.rgb;\n");
    FMT("    lightColor.rgb += %s.rgb * uGlobalAmbient.rgb;\n", colInp);
    for (i = 0; i < MAX_LIGHTS; i++) {
        if (!ENA_OFFS(LIGHT0,i)) continue;

        FMT2("    lightColor.rgb += (uAmbient_%d * %s).rgb;\n", i, colInp);
        if (ENA_OFFS(LIGHT0_POS,i)) {
            if (isFragment) {
                /* We dot (0,0,1) with the tangent space for the eye dir */
                FMT("    tmpDir = uLightPos_%d.xyz - vEyePos;\n", i);

                STR("    lightDir.x = dot(tmpDir, tangent);\n");
                STR("    lightDir.y = dot(tmpDir, binormal);\n");
                STR("    lightDir.z = dot(tmpDir, normal);\n");
            }
            else {
                FMT("    lightDir = uLightPos_%d.xyz - eyePos;\n", i);
            }
            STR("    lightDir = normalize(lightDir);\n");
        }
        else {
            /* Light direction is pre-normalized */
            if (isFragment) {
                FMT("    tmpDir = uLightPos_%d.xyz;\n", i);
                STR("    lightDir.x = dot(tmpDir, tangent);\n");
                STR("    lightDir.y = dot(tmpDir, binormal);\n");
                STR("    lightDir.z = dot(tmpDir, normal);\n");
                STR("    lightDir = normalize(lightDir);\n");
            }
            else {
                FMT("    lightDir = uLightPos_%d.xyz;\n", i);
            }
        }
        FMT("    diffAmt = clamp(dot(lightDir, %s), 0., 1.);\n", normal);
        FMT2("    lightColor.rgb += (diffAmt * uDiffuse_%d * %s).rgb;\n",
                    i, colInp);

        STR("    halfAngle = normalize(lightDir + eyeDir);\n");
        FMT("    specAmt = clamp(dot(halfAngle, %s), 0., 1.);\n", normal);
        STR("    specAmt = pow(specAmt, uMatShininess);\n");
        STR("    specAmt *= (diffAmt < 0.) ? 0. : 1.;\n");
        if (ENA(COLOR_SUM) || isFragment) {
            FMT2("    specColor.rgb += (specAmt * uSpecular_%d * %s).rgb;\n",
                    i, "uMatSpecular");
        }
        else {
            FMT2("    lightColor.rgb += (specAmt * uSpecular_%d * %s).rgb;\n",
                    i, "uMatSpecular");
        }
    }
    return 1;
}

static int declareVaryings(GlProgSource *ps, hwProgInfo *ppi)
{
    int i;

    STR("\n");
    FMT("varying %svec4 vColor;\n", PREC_L);
    if (ENA(COLOR_SUM)) {
        FMT("varying %svec4 vColor2;\n", PREC_L);
    }
    if (ENA(LIGHT_PER_PIXEL)) {
        FMT("varying %svec3 vNormal;\n", PREC_L);
        FMT("varying %svec3 vTangent;\n", PREC_L);
        FMT("varying %svec3 vBinormal;\n", PREC_L);
        if (ENA_ANY(LIGHT0_POS, LIGHT7_ATT)) {
            FMT("varying %svec3 vEyePos;\n", PREC_M);
        }
    }
    if (ENA(FOG)) {
        FMT("varying %sfloat vFogDist;\n", PREC_M);
    }

    for (i = 0; i < MAX_TEX; i++) {
        if (ENA_OFFS(TEXGEN0_STAGE_0,i)) continue;
        // TBD: What about REFLECT when bump mapping?
        if (ENA_OFFS(TEX0_PROJ, i)) {
            FMT2("varying %svec3 vTexCoord%d;\n", PREC_M, i);
        }
        if (ENA_OFFS(TEX0_CUBE, i)) {
            FMT2("varying %svec3 vTexCoord%d;\n", PREC_M, i);
        }
        else if (ENA_OFFS(TEX0,i)) {
            FMT2("varying %svec2 vTexCoord%d;\n", PREC_M, i);
        }
    }

    return 1;
}

int __hwGlCreateVtxProgram(hwProgInfo *ppi)
{
    GlProgSource *ps = &ppi->vtxProg;
    const char *colInp;
    const char nmlInp[] = "aNormal";
    const char tanInp[] = "aTangent";
    const char *colOut;
    int i;

    rewindString(ps);

    /* Get the overall vertex color name */
    colInp = ENA(VERTEX_COLOR) ? "aColor" : "uColor";

    /* And the input color name */
    colOut = (ENA(LIGHTING) && !ENA(LIGHT_PER_PIXEL))
                ? "lightColor" : colInp;

    /*************************************************************************
     * Header boilerplate
     *************************************************************************/
    FMT("#version %s\n", GLSL_VERSION);

    /*************************************************************************
     * Uniforms
     *************************************************************************/
    STR("\n");
    FMT("uniform %smat4 uMvpMatrix;\n", PREC_H);
    if (ENA(LIGHTING) || ENA(FOG)) {
        FMT("uniform %smat4 uMvMatrix;\n", PREC_H);
    }
    if (ENA(LIGHTING) && !ENA(LIGHT_PER_PIXEL)) {
        if (!declareLightUniforms(ps, ppi, 0)) return 0;
    }
    if (ENA(LIGHTING)) {
        FMT("uniform %smat3 uMvInvXpose;\n", PREC_H);
    }
    if (!ENA(VERTEX_COLOR)) {
        FMT("uniform %svec4 uColor;\n", PREC_H);
    }

    /*************************************************************************
     * Vertex attributes
     *************************************************************************/
    STR("\n");
    FMT("attribute %svec4 aPosition;\n", PREC_H);
    FMT("attribute %svec4 aColor;\n", PREC_H);
    if (ENA(LIGHTING)) {
        FMT("attribute %svec3 aNormal;\n", PREC_H);
    }
    if (ENA(LIGHT_PER_PIXEL)) {
        FMT("attribute %svec4 aTangent;\n", PREC_H);
    }
    for (i = 0; i < MAX_TEX; i++) {
        if (ENA_OFFS(TEXGEN0_STAGE_0,i)) continue;
        if (ENA_OFFS(TEXGEN0_REFLECT,i)) continue;
        if (ENA_OFFS(TEX0,i)) {
            FMT2("attribute %svec4 aTexCoord%d;\n", PREC_H, i);
        }
    }

    /*************************************************************************
     * Varying outputs.
     *************************************************************************/
    if (!declareVaryings(ps, ppi)) return 0;

    /*************************************************************************
     * The main program
     *************************************************************************/
    STR("\n");
    STR("void main()\n");
    STR("{\n");

    /*************************************************************************
     * Declare locals
     *************************************************************************/
    if (ENA(LIGHTING) || ENA(FOG)) {
        /* TBD: We could optimize this if we knew there was no
         * projection in the modelview matrix
         */
        FMT("    %svec4 hEyePos = uMvMatrix * aPosition;\n", PREC_H);
        FMT("    %sfloat wRecip = 1.0 / hEyePos.w;\n", PREC_H);
        FMT("    %svec3 eyePos = hEyePos.xyz * vec3(wRecip);\n", PREC_H);
    }
    if (ENA(LIGHTING)) {
        if (ENA(LIGHT_PER_PIXEL)) {
            /* Transform the tangent and compute the binormal */
            FMT3("    %svec3 binormal = cross(%s, %s.xyz);\n",
                        PREC_H, nmlInp, tanInp);
            STR("    binormal = normalize(uMvInvXpose * binormal);\n");
            FMT("    binormal *= vec3(%s.w);\n", tanInp);
            FMT2("    %svec3 tangent = normalize(uMvInvXpose * %s.xyz);\n",
                        PREC_H, tanInp);
        }
        else {
            if (!declareLightLocals(ps, ppi, 0)) return 0;
        }
        FMT2("    %svec3 normal = normalize(uMvInvXpose * %s);\n",
                PREC_H, nmlInp);
    }
    STR("\n");

    /* Calculate lighting */
    if (ENA(LIGHTING) && !ENA(LIGHT_PER_PIXEL)) {
        if (!performLighting(ps, ppi, colInp, 0)) return 0;
    }

    /* Pass through the tangent space */
    if (ENA(LIGHTING) && ENA(LIGHT_PER_PIXEL)) {
        STR("    vNormal = normal;\n");
        STR("    vTangent = tangent;\n");
        STR("    vBinormal = binormal;\n");
        if (ENA_ANY(LIGHT0_POS, LIGHT7_ATT)) {
            STR("    vEyePos = eyePos;\n");
        }
    }

    /* Pass through the color */
    FMT("    vColor = %s;\n", colOut);

    /* Pass through the secondary color */
    if (ENA(COLOR_SUM)) {
        STR("    vColor2 = specColor;\n");
    }

    /* Pass through the texture */
    for (i = 0; i < MAX_TEX; i++) {
        if (ENA_OFFS(TEXGEN0_STAGE_0,i)) continue;
        // TBD: What about REFLECT when bump mapping?
        if (ENA_OFFS(TEX0_PROJ, i)) {
            FMT2("    vTexCoord%d = aTexCoord%d.stq;\n", i, i);
        }
        else if (ENA_OFFS(TEX0_CUBE, i)) {
            FMT2("    vTexCoord%d = aTexCoord%d.str;\n", i, i);
        }
        else if (ENA_OFFS(TEX0,i)) {
            FMT2("    vTexCoord%d = aTexCoord%d.st;\n", i, i);
        }
    }

    /* Pass through the fog dist */
    if (ENA(FOG)) {
        STR("    vFogDist = length(eyePos);\n");
    }
    /* Transform the position */
    STR("    gl_Position = uMvpMatrix * aPosition;\n");

    /* Close the main program */
    STR("}\n");

    return 1;
}

int __hwGlCreateFragProgram(hwProgInfo *ppi)
{
    GlProgSource *ps = &ppi->fragProg;
    char *color2;
    int anyGloss;
    int i;

    rewindString(ps);

    /*************************************************************************
     * Header
     *************************************************************************/
    FMT("#version %s\n", GLSL_VERSION);

    /*************************************************************************
     * Varying inputs
     *************************************************************************/
    if (!declareVaryings(ps, ppi)) return 0;

    /*************************************************************************
     * Uniforms (texture IDs, fog)
     *************************************************************************/
    for (i = 0; i < MAX_TEX; i++) {
        if (ENA_OFFS(TEX0_CUBE, i)) {
            FMT("uniform samplerCube uTexture%d;\n", i);
        }
        else if (ENA_OFFS(TEX0,i)) {
            FMT("uniform sampler2D uTexture%d;\n", i);
        }
    }
    if (ENA(FOG_LINEAR)) {
        FMT("uniform %sfloat uFogDenom;\n", PREC_M);
        FMT("uniform %sfloat uFogEnd;\n", PREC_M);
    }
    if (ENA(FOG_EXP) || ENA(FOG_EXP2)) {
        FMT("uniform %sfloat uFogDensity;\n", PREC_M);
    }
    if (ENA(FOG)) {
        FMT("uniform %svec4 uFogColor;\n", PREC_L);
    }
    if (ENA(LIGHT_PER_PIXEL)) {
        if (!declareLightUniforms(ps, ppi, 1)) return 0;
    }

    /*************************************************************************
     * The main program
     *************************************************************************/
    STR("void main()\n");
    STR("{\n");

    if (ENA(LIGHT_PER_PIXEL)) {
        if (!declareLightLocals(ps, ppi, 1)) return 0;
        FMT("    %svec3 normal = normalize(vNormal);\n", PREC_L);
        FMT("    %svec3 tangent = normalize(vTangent);\n", PREC_L);
        FMT("    %svec3 binormal = normalize(vBinormal);\n", PREC_L);
        if (!performLighting(ps, ppi, "vColor", 1)) return 0;
        FMT("    %svec4 tColor = lightColor;\n", PREC_L);
    }
    else {
        FMT("    %svec4 tColor = vColor;\n", PREC_L);
    }

    /* Sample the textures */
    anyGloss = 0;
    for (i = 0; i < MAX_TEX; i++) {
        if (!ENA_OFFS(TEX0,i)) continue;
        if (ENA_OFFS(TEX0_NORMAL,i)) continue;
        if (ENA_OFFS(TEX0_RELIEF,i)) continue;
        if (ENA_OFFS(TEX0_GLOSS,i)) { anyGloss = 1; continue; }
        if (!textureMap(ps, ppi, "    tColor *= %s;\n", i)) return 0;
    }

    /* Secondary (specular) color */
    color2 = ENA(LIGHT_PER_PIXEL) ? "specColor" : "vColor2";
    if (anyGloss) {
        FMT2("    %svec4 gloss = %s;\n", PREC_L, color2);
        for (i = 0; i < MAX_TEX; i++) {
            if (!ENA_OFFS(TEX0_GLOSS,i)) continue;
            if (!textureMap(ps, ppi, "    gloss *= %s;\n", i)) return 0;
        }
        STR("    tColor += gloss;\n");
    }
    else if (ENA(COLOR_SUM) || ENA(LIGHT_PER_PIXEL)) {
        FMT("    tColor += %s;\n", color2);
    }

    /* Fog me */
    if (ENA(FOG_LINEAR)) {
        FMT("    %sfloat fogFactor = (uFogEnd - vFogDist) * uFogDenom;\n",
                      PREC_M);
    }
    else if (ENA(FOG_EXP)) {
        FMT("    %sfloat fogFactor = exp(-uFogDensity * vFogDist);\n", PREC_M);
    }
    else if (ENA(FOG_EXP2)) {
        FMT("    %sfloat fogFactor = exp(-uFogDensity * vFogDist);\n", PREC_M);
        STR("    fogFactor = fogFactor * fogFactor;\n");
    }
    if (ENA(FOG)) {
        STR("    fogFactor = clamp(fogFactor, 0., 1.);\n");
        STR("    tColor.rgb = mix(uFogColor.rgb, tColor.rgb, fogFactor);\n");
    }

    /* Write the color */
    STR("    gl_FragColor = tColor;\n");

    /* Close the main program */
    STR("}\n");

    return 1;
}

/**************************************************************************
 * Local utility functions
 **************************************************************************/
static void rewindString(GlProgSource *ps)
{
    ps->currPtr = ps->startPtr;
    ps->numLines = 0;
}

static int addStr(GlProgSource *ps, const char *str)
{
    int n = strlen(str) + 1;
    char *oldStartPtr;
    int offs;
    int i;

    /* See if we need to grow the array */
    while ((ps->currPtr + n) >= ps->endPtr) {
        oldStartPtr = ps->startPtr;

        ps->allocSize  = (ps->allocSize * 3) / 2;
        ps->startPtr = realloc(ps->startPtr, ps->allocSize);

        if (!ps->startPtr) {
#ifdef ANDROID_NDK
            __android_log_print(ANDROID_LOG_INFO, "HB_HW", "addStr OOM");
#endif
            ps->currPtr = ps->startPtr = ps->endPtr = NULL;
            ps->allocLines = 0;
            return 0;
        }

        offs = ps->currPtr - oldStartPtr;

        ps->currPtr = ps->startPtr + offs;
        ps->endPtr = ps->startPtr + ps->allocSize;

        for (i = 0; i < ps->numLines; i++) {
            offs = ps->lines[i] - oldStartPtr;
            ps->lines[i] = ps->startPtr + offs;
        }
    }

    if (ps->numLines >= ps->allocLines) {
        ps->allocLines = (ps->allocLines * 3) / 2;
        ps->lines = realloc(ps->lines, ps->allocLines * sizeof(char *));
        if (!ps->lines) {
#ifdef ANDROID_NDK
            __android_log_print(ANDROID_LOG_INFO, "HB_HW", "addStr OOM");
#endif
            ps->currPtr = ps->startPtr = ps->endPtr = NULL;
            ps->allocLines = 0;
            return 0;
        }
    }

    strcpy(ps->currPtr, str);
    ps->lines[ps->numLines] = ps->currPtr;

    ps->numLines++;
    ps->currPtr += n;
    return 1;
}

static int addFmt(GlProgSource *ps, const char *fmt, ...)
{
    char buff[256];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buff, 256, fmt, args);
    va_end(args);

    return addStr(ps, buff);
}
