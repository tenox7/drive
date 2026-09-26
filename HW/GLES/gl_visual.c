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
#include <string.h>
#include <ctype.h>

#include "hw.h"
#include "hw_internal.h"
#include "hw_gl.h"

#if defined(ANDROID_NDK)
//#  define ANDROID_LOG(a) __android_log_print a
#  define ANDROID_LOG(a)
#else
#  define ANDROID_LOG(a)
#endif

const struct __hwDisplayStruct
    __hwGlInitFunc = {
        "GLES",
        __hwGlCreate,
        __hwGlChooseVisual,
        __hwGlExtractVisual,
        __hwGlExtractWin,
        __hwGlCreateWin,
        __hwGlCreateChildWin,
        __hwGlInitDrawable,
        __hwGlInputHandler,
        __hwGlGetMousePos,

        __hwGlMakeCurrent,
        __hwGlGetInfo,
        __hwGlViewport,
        __hwGlScissor,
        __hwGlCamera,
        __hwGlPushMat,
        __hwGlPopMat,
        __hwGlXformPoint,
        __hwGlUpdate,
        __hwGlSetDrawBuffer,

        __hwGlRenderMode,
        __hwGlSelectionInfo,
        __hwGlWasSelected,
        __hwGlCurrObject,
        __hwGlCurrChild,

        __hwGlPosLight,
        __hwGlDirLight,
        __hwGlFog,
        __hwGlFogParams,
        __hwGlAmbient,
        __hwGlLighting,
        __hwGlBackground,

        __hwGlCreateTexture,
        __hwGlDestroyTexture,
        __hwGlCurrentTexture,

        __hwGlSurfAttrs,
        __hwGlSetVisibility,
        __hwGlSetInvisibility,
        __hwGlGetVisibility,
        __hwGlDrawBBox,
        __hwGlBoundsVisible,
        __hwGlBoundsSize,

        __hwGlDrawMesh,
        __hwGlDrawStrip,
        __hwGlDrawPolygon,
        __hwGlDrawPolyline,
        __hwGlDrawQuads,
        __hwGlIndexedTris,
        __hwGlDrawMarkers,

        __hwGlGuiText,
        __hwGlGuiRaster,
        __hwGlGuiRectangle,
        __hwGlGuiPolyline,
        __hwGlGuiLines,
        __hwGlGuiPolygon,
        __hwGlOpenGuiList,
        __hwGlCloseGuiList,
        __hwGlCallGuiList,
        __hwGlDestroyGuiList,

        __hwGlOpenList,
        __hwGlCloseList,
        __hwGlCallList,
        __hwGlDestroyList,

        __hwGlGetTextState,
        __hwGlGetDrawSize,

        __hwGlGetCurrTime,
        __hwGlGetStartTime,
        __hwGlGetElapsedTime,
        __hwGlSetCurrTime,
        __hwGlSetStartTime,
        __hwGlSetElapsedTime,
    };

const hwDisplay
    hwGlDisplay = (const hwDisplay)&__hwGlInitFunc,
    hwDefaultDisplay = (const hwDisplay)&__hwGlInitFunc;

int
    __hwglDoExtensions = 1,
    __hwglDoPrune = 1,
    __hwglDoVbo = 1,
    __hwglPruneEye = 0,
    __hwglShowBoxes = 0,
    __hwglDisableCull = 0;


void __hwGlInitState( glDrawable *draw )
{
    int
        i;

    __hwGlGetExtensions( draw->disp, draw );

#ifdef WIN32
    __hwglSetProcAddrs( draw->disp );
#endif

    draw->color[0] = -1.0;
    draw->color[1] = -1.0;
    draw->color[2] = -1.0;
    draw->ambFactSave = -1.0;
    draw->shininess = -1.0;
    draw->specColor[0] = -1.0;
    draw->specColor[1] = -1.0;
    draw->specColor[2] = -1.0;
    draw->shadeFlat = -1;
    draw->primSize = -1.0;
    draw->wireframeState = WFS_INVALID;

    if( draw->tmActive != -1 ) {
        draw->tmActive = -1;
        for( i = 0; i < HW_MAX_TEXTURES; i++ ) {
            hwGlActiveTexture( HWGL_TEXTURE0+i );
            hwGlDisable( HWGL_TEXTURE_2D );
            draw->tmId[i] = -1;
        }
    }
    if( draw->blend ) {
        draw->blend = 0;
        hwGlDisable( GL_BLEND );
    }
    draw->blendSrc = 0; // Reset it next time we draw something

    /* TBD: Controlled by camera perspective model... */
    //glLightModeli( GL_LIGHT_MODEL_LOCAL_VIEWER, 1 );

    hwGlEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );
#if defined(ANDROID_NDK)
    glClearDepthf( 1.0 );
#else
    glClearDepth( 1.0 );
#endif
    hwGlEnable( HWGL_NORMALIZE );
    hwGlEnable( GL_DITHER );
    glFrontFace( GL_CW );
    hwGlPointSize( 3.0 );
    hwGlEnable( HWGL_COLOR_MATERIAL );
    //glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    hwGlEnableClientState( HWGL_VERTEX_ARRAY );
}

const char *__hwGlGetInfo( hwDisplay disp, hwInt32 whichInfo )
{
    USE_GL_CTX(disp);
    glDisplay *gldisp = (glDisplay *)disp;
    int joy;
    char *s;

    switch( whichInfo ) {
    case HW_INFO_HW :
        return "OpenGL";
    case HW_INFO_VENDOR :
        return (char *)glGetString( GL_VENDOR );
    case HW_INFO_RENDERER :
        return (char *)glGetString( GL_RENDERER );
    case HW_INFO_VERSION :
        return (char *)glGetString( GL_VERSION );
    case HW_INFO_EXTENSIONS :
        if (!gldisp->glob.extensionString) {
            s = (char *)glGetString( GL_EXTENSIONS );
            if (s) {
                gldisp->glob.extensionString = strdup(s);
            }
        }
#if 0 && defined(GL_VERSION_3_0)
        if (    (gldisp->glob.extensions & HWGL_EXT_OPENGL_3_0)
             && !gldisp->glob.extensionString)
        {
            GLint numExt;
            int i, len;
            char *ptr;

            /* RANT!  This is quite possibly the stupidest of a set of
             * stupid decisions the ARB and Khronos have made
             * regarding backward compatibility and OpenGL.
             * Why in the world would you deprecate the old
             * GL_EXTENSIONS mechanism?  Older programs have already
             * been hardened to really long strings.  Adding this "fix"
             * of replacing glGetString(GL_EXTENSIONS) with
             * glGetStringi(GL_EXTENSIONS, i) does nothing for older
             * programs than just break them gratuitously.  Grumble.
             */

            // Get length of extensions string
            glGetIntegerv(GL_NUM_EXTENSIONS, &numExt);
            len = 0;
            for (i = 0; i < numExt; i++) {
                s = (char *)glGetStringi(GL_EXTENSIONS, i);
                if (!s) continue;
                len += strlen(s) + 1; // Space separator or null
            }

            // Build extensions string
            ptr = gldisp->glob.extensionString = malloc(len);
            if (ptr) {
                for (i = 0; i < numExt; i++) {
                    s = (char *)glGetStringi(GL_EXTENSIONS, i);
                    if (!s) continue;
                    while (*s) *ptr++ = *s++;
                    if (i < (numExt-1)) *ptr++ = ' ';
                }
                *ptr = 0;
            }
        }
#endif
        return gldisp->glob.extensionString;

    case HW_INFO_JOYSTICK0 :
    case HW_INFO_JOYSTICK1 :
    case HW_INFO_JOYSTICK2 :
    case HW_INFO_JOYSTICK3 :
        joy = whichInfo - HW_INFO_JOYSTICK0;
        if (glctx && glctx->joystick[joy]) {
            return glctx->joystick[joy]->name;
        }
        else {
            return NULL;
        }
    default :
        return NULL;
    }
}

void __hwGlUpdate( hwDisplay disp, hwInt32 updateFlags )
{
    USE_GL_CTX(disp);
    glDisplay
        *gldisp = (glDisplay *)disp;
    double
        myTime;

    if( !glctx ) return;

    if( updateFlags & HW_CHECK_EVENTS ) {
        __hwGlCheckInput( disp );
    }

    if( updateFlags & HW_UPDATE_GUI ) {
        __hwGlDrawElements( disp );
    }

    if( glctx->disp->dbuffer && (updateFlags & HW_UPDATE_SWAP)) {
        __hwGlSwapBuffers( glctx );
    }

    if( updateFlags & HW_UPDATE_CLEAR_COLOR ) {
        if( updateFlags & HW_UPDATE_CLEAR_DEPTH ) {
            glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
        }
        else {
            glClear( GL_COLOR_BUFFER_BIT );
        }
    }
    else if( updateFlags & HW_UPDATE_CLEAR_DEPTH ) {
        glClear( GL_DEPTH_BUFFER_BIT );
    }

    if( updateFlags & HW_UPDATE_MATRIX ) {
        hwGlMatrixMode( HWGL_MODELVIEW );
        while( glctx->stackDepth ) {
            hwGlPopMatrix();
            glctx->stackDepth--;
        }
        hwGlLoadIdentity();

        hwGlMatrixMode( HWGL_PROJECTION );
        hwGlLoadIdentity();

        glctx->numLights = 0;
        glctx->planesValid = 0;
        glctx->camSet = 0;

        hwIdentity( glctx->camMat );
    }

    if( updateFlags & HW_CLEAR_GUI ) {
        if( glctx->guiTail) {
            glctx->guiTail->next = glctx->guiFree;
            glctx->guiFree = glctx->guiList;
        }
        glctx->guiList = NULL;
        glctx->guiTail = NULL;
    }

    if( updateFlags & HW_UPDATE_TIME ) {
        /* Step to the next frame */
        myTime = hwGetSysTime();
        if( myTime < gldisp->glob.currTime ) myTime = gldisp->glob.currTime;
        gldisp->glob.currTime = myTime;
        gldisp->glob.elapsedTime = myTime - gldisp->glob.startTime;
    }
}

void __hwGlPushMat( hwDisplay disp, hwFloat Mat[4][4] )
{
    USE_GL_CTX(disp);
    int
        n;

#if defined(ANDROID_DEBUG)
    ANDROID_LOG((ANDROID_LOG_INFO, "HB_HW",
                "__hwGlPushMat: Calling hwGlPushMatrix"));
#endif

    if( !glctx ) return;

    if( glctx->openList ) {
        __hwGlAppendPushMat( glctx, Mat );
        return;
    }

    n = glctx->stackDepth++;
    if( glctx->stackDepth > MSD )        return;

    if( n > 0 ) {
        hwMatMult(      glctx->matStack[n],
                        Mat,
                        glctx->matStack[n-1] );
        hwMatMult(      glctx->rawMatStack[n],
                        Mat,
                        glctx->rawMatStack[n-1] );
    }
    else {
        hwMatMult(      glctx->matStack[0],
                        Mat,
                        glctx->camMat );
        (void)memcpy( glctx->rawMatStack[0],
                        Mat, 4*4*sizeof(hwFloat) );
    }

    hwGlMatrixMode( HWGL_MODELVIEW );
    hwGlPushMatrix();
    hwGlLoadMatrixf( &glctx->matStack[n][0][0] );
}

void __hwGlPopMat( hwDisplay disp )
{
    USE_GL_CTX(disp);
    if( !glctx ) return;

    if( glctx->openList ) {
        __hwGlAppendPopMat(glctx);
        return;
    }

    if( glctx->stackDepth < 1 )  return;

    glctx->stackDepth--;

    hwGlMatrixMode( HWGL_MODELVIEW );
    hwGlPopMatrix();
}

void __hwGlXformPoint( hwDisplay disp, hwFloat point[3] )
{
    USE_GL_CTX(disp);
    int n;

    if( !glctx ) return;
    if( glctx->openList ) return;
    if( glctx->stackDepth < 1 )  return;

    n = glctx->stackDepth - 1;

    hwTransform( glctx->matStack[n], point );
    hwTransformPersp( point, point, glctx->projMat );
}


hwInt32 __hwGlRenderMode ( hwDisplay disp, hwInt32 which )
{
    USE_GL_CTX(disp);
    hwInt32
       oldMode;

    if( !glctx ) return 0;

    if( which == HW_RENDER_QUERY) {
        return( glctx->renderMode);
    }

    switch( which & HW_RENDER_MASK ) {
    case HW_RENDER_DRAW:
        if( which & HW_RENDER_Z_TEST ) {
            hwGlEnable( GL_DEPTH_TEST );
        }
        else {
            hwGlDisable( GL_DEPTH_TEST );
        }
        if( which & HW_RENDER_Z_WRITE ) {
            glDepthMask( GL_TRUE );
        }
        else {
            glDepthMask( GL_FALSE );
        }

#if 0 /* [ */
        // TBD - do we want to do this ever?
        if( which & HW_RENDER_MULTISAMPLE ) {
            hwGlEnable( GL_HACK_MULTISAMPLE );
            hwGlEnable( HP_HACK_MULTISAMPLE );
            glctx->disp->dbuffer |= 0x80000000;
        }
        else {
            if( glctx->disp->dbuffer & 0x80000000 ) {
                hwGlDisable( GL_HACK_MULTISAMPLE );
                hwGlDisable( HP_HACK_MULTISAMPLE );
            }
            glctx->disp->dbuffer &= ~0x80000000;
        }

        if( which & HW_RENDER_EDGE_MODE ) {
            glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
        }
        else {
            glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
        }
#endif /* ] */

        glctx->wireframeState = WFS_INVALID;
        break;
    case HW_RENDER_SELECT :
        glctx->sel.picked = 0;
        glctx->sel.dist = 1.e38;
        glctx->currObj = 0;
        break;
    }

    if( __hwglDisableCull ) which &= ~HW_RENDER_CULL_FACE;

    oldMode = glctx->renderMode;

    glctx->renderMode = which;
    return(oldMode);
}

void __hwGlSelectionInfo
(
    hwDisplay disp, hwInt32 flags,
    hwFloat x, hwFloat y, hwFloat aperture
)
{
    USE_GL_CTX(disp);
    if( !glctx ) return;

    glctx->sel.flags = flags;
    glctx->sel.point[0] = x;
    glctx->sel.point[1] = y;
    glctx->sel.aperture = aperture;
}

hwInt32 __hwGlWasSelected( hwDisplay disp, hwObject *obj, hwInt32 *vert )
{
    USE_GL_CTX(disp);
    int
        result;

    if( !glctx ) return 0;
    if( obj )   *obj = glctx->sel.obj;
    if( vert )  *vert = glctx->sel.vert;
    return glctx->sel.picked;
}

void __hwGlCurrObject( hwDisplay disp, hwObject obj )
{
    USE_GL_CTX(disp);
    if( !glctx ) return;

    glctx->currObj = obj;
}

void __hwGlCurrChild( hwDisplay disp, hwObject obj )
{
    USE_GL_CTX(disp);
    if( !glctx ) return;

    if( glctx->sel.flags & HW_SELECT_CHILD ) {
        glctx->currObj = obj;
    }
}


void __hwGlSetVisibility( hwDisplay disp, hwInt32 mask )
{
    USE_GL_CTX(disp);
    if( !glctx ) return;
    glctx->animMask = mask;
}

void __hwGlSetInvisibility( hwDisplay disp, hwInt32 incl, hwInt32 excl )
{
    USE_GL_CTX(disp);
    if( !glctx ) return;
    glctx->animMask = 0;
    glctx->invisibleInclude = incl;
    glctx->invisibleExclude = excl;
}

hwInt32 __hwGlGetVisibility( hwDisplay disp)
{
    USE_GL_CTX(disp);
    if( !glctx ) return(0);
    return(glctx->animMask);
}

hwTextState *__hwGlGetTextState( hwDisplay disp )
{
    USE_GL_CTX(disp);
    if( !glctx )                 return 0;
    return &glctx->textState;
}

void __hwGlGetDrawSize( hwDisplay disp, hwInt32 *retSize )
{
    USE_GL_CTX(disp);
    if( !glctx )                 return;

    retSize[0] = glctx->winW;
    retSize[1] = glctx->winH;
}

hwInt16 *__hwGlGrowIdxPool( glDrawable *glctx, hwInt32 newSize )
{
    if( !glctx)                 return NULL;

    if( newSize > glctx->idxPoolSize ) {
        glctx->idxPoolSize = newSize;
        glctx->idxPool = realloc(glctx->idxPool, newSize*sizeof(hwInt16));
        if( !glctx->idxPool ) {
            /* TBD: good error */
#ifdef ANDROID_NDK
            __android_log_print(ANDROID_LOG_INFO, "HB_HW",
                                "__hwGlGrowIdxPool OOM");
#endif
            exit(1);
        }
    }
    return glctx->idxPool;
}

hwFloat *__hwGlGrowVtxPool( glDrawable *glctx, hwInt32 newSize )
{
    if( !glctx)                 return NULL;

    if( newSize > glctx->vtxPoolSize ) {
        glctx->vtxPoolSize = newSize;
        glctx->vtxPool = realloc(glctx->vtxPool, newSize*sizeof(hwFloat));
        if( !glctx->vtxPool ) {
            /* TBD: good error */
#ifdef ANDROID_NDK
            __android_log_print(ANDROID_LOG_INFO, "HB_HW",
                                "__hwGlGrowVtxPool OOM");
#endif
            exit(1);
        }
    }
    return glctx->vtxPool;
}

hwFloat *__hwGlGrowAuxPool( glDrawable *glctx, hwInt32 newSize )
{
    if( !glctx)                 return NULL;

    if( newSize > glctx->auxPoolSize ) {
        glctx->auxPoolSize = newSize;
        glctx->auxPool = realloc(glctx->auxPool, newSize*sizeof(hwFloat));
        if( !glctx->auxPool ) {
            /* TBD: good error */
#ifdef ANDROID_NDK
            __android_log_print(ANDROID_LOG_INFO, "HB_HW",
                                "__hwGlGrowAuxPool OOM");
#endif
            exit(1);
        }
    }
    return glctx->auxPool;
}

#ifdef WIN32 /* [ */

GL_GEN_BUFFERS_FPTR                     glGenBuffers;
GL_BIND_BUFFER_FPTR                     glBindBuffer;
GL_BUFFER_DATA_FPTR                     glBufferData;
GL_BUFFER_SUB_DATA_FPTR                 glBufferSubData;
GL_DELETE_BUFFERS_FPTR                  glDeleteBuffers;

GL_ACTIVE_TEXTURE_FPTR                  glActiveTexture;
GL_CLIENT_ACTIVE_TEXTURE_FPTR           glClientActiveTexture;

GL_GET_SHADER_IV_FPTR                   glGetShaderiv;
GL_GET_PROGRAM_IV_FPTR                  glGetProgramiv;
GL_GET_SHADER_SOURCE_FPTR               glGetShaderSource;
GL_GET_SHADER_INFO_LOG_FPTR             glGetShaderInfoLog;
GL_GET_PROGRAM_INFO_LOG_FPTR            glGetProgramInfoLog;
GL_GET_UNIFORM_LOCATION_FPTR            glGetUniformLocation;
GL_UNIFORM_1FV_FPTR                     glUniform1fv;
GL_UNIFORM_2FV_FPTR                     glUniform2fv;
GL_UNIFORM_3FV_FPTR                     glUniform3fv;
GL_UNIFORM_4FV_FPTR                     glUniform4fv;
GL_UNIFORM_1I_FPTR                      glUniform1i;
GL_UNIFORM_MATRIX_3FV_FPTR              glUniformMatrix3fv;
GL_UNIFORM_MATRIX_4FV_FPTR              glUniformMatrix4fv;
GL_CREATE_SHADER_FPTR                   glCreateShader;
GL_SHADER_SOURCE_FPTR                   glShaderSource;
GL_COMPILE_SHADER_FPTR                  glCompileShader;
GL_CREATE_PROGRAM_FPTR                  glCreateProgram;
GL_ATTACH_SHADER_FPTR                   glAttachShader;
GL_BIND_ATTRIB_LOCATION_FPTR            glBindAttribLocation;
GL_LINK_PROGRAM_FPTR                    glLinkProgram;
GL_USE_PROGRAM_FPTR                     glUseProgram;

GL_VERTEX_ATTRIB_POINTER_FPTR           glVertexAttribPointer;
GL_ENABLE_VERTEX_ATTRIB_ARRAY_FPTR      glEnableVertexAttribArray;
GL_DISABLE_VERTEX_ATTRIB_ARRAY_FPTR     glDisableVertexAttribArray;
GL_VERTEX_ATTRIB_4F_FPTR                glVertexAttrib4f;

GL_GET_STRING_I_FPTR                    glGetStringi;

#define GET_PROC(typ, prc) prc = (typ)wglGetProcAddress( #prc )

void __hwglSetProcAddrs(glDisplay *gldisp)
{
    GET_PROC(GL_GEN_BUFFERS_FPTR,                 glGenBuffers);
    GET_PROC(GL_BIND_BUFFER_FPTR,                 glBindBuffer);
    GET_PROC(GL_BUFFER_DATA_FPTR,                 glBufferData);
    GET_PROC(GL_BUFFER_SUB_DATA_FPTR,             glBufferSubData);
    GET_PROC(GL_DELETE_BUFFERS_FPTR,              glDeleteBuffers);

    GET_PROC(GL_ACTIVE_TEXTURE_FPTR,              glActiveTexture);
    GET_PROC(GL_CLIENT_ACTIVE_TEXTURE_FPTR,       glClientActiveTexture);

    GET_PROC(GL_GET_SHADER_IV_FPTR,               glGetShaderiv);
    GET_PROC(GL_GET_PROGRAM_IV_FPTR,              glGetProgramiv);
    GET_PROC(GL_GET_SHADER_SOURCE_FPTR,           glGetShaderSource);
    GET_PROC(GL_GET_SHADER_INFO_LOG_FPTR,         glGetShaderInfoLog);
    GET_PROC(GL_GET_PROGRAM_INFO_LOG_FPTR,        glGetProgramInfoLog);
    GET_PROC(GL_GET_UNIFORM_LOCATION_FPTR,        glGetUniformLocation);
    GET_PROC(GL_UNIFORM_1FV_FPTR,                 glUniform1fv);
    GET_PROC(GL_UNIFORM_2FV_FPTR,                 glUniform2fv);
    GET_PROC(GL_UNIFORM_3FV_FPTR,                 glUniform3fv);
    GET_PROC(GL_UNIFORM_4FV_FPTR,                 glUniform4fv);
    GET_PROC(GL_UNIFORM_1I_FPTR,                  glUniform1i);
    GET_PROC(GL_UNIFORM_MATRIX_3FV_FPTR,          glUniformMatrix3fv);
    GET_PROC(GL_UNIFORM_MATRIX_4FV_FPTR,          glUniformMatrix4fv);
    GET_PROC(GL_CREATE_SHADER_FPTR,               glCreateShader);
    GET_PROC(GL_SHADER_SOURCE_FPTR,               glShaderSource);
    GET_PROC(GL_COMPILE_SHADER_FPTR,              glCompileShader);
    GET_PROC(GL_CREATE_PROGRAM_FPTR,              glCreateProgram);
    GET_PROC(GL_ATTACH_SHADER_FPTR,               glAttachShader);
    GET_PROC(GL_BIND_ATTRIB_LOCATION_FPTR,        glBindAttribLocation);
    GET_PROC(GL_LINK_PROGRAM_FPTR,                glLinkProgram);
    GET_PROC(GL_USE_PROGRAM_FPTR,                 glUseProgram);

    GET_PROC(GL_VERTEX_ATTRIB_POINTER_FPTR,       glVertexAttribPointer);
    GET_PROC(GL_ENABLE_VERTEX_ATTRIB_ARRAY_FPTR,  glEnableVertexAttribArray);
    GET_PROC(GL_DISABLE_VERTEX_ATTRIB_ARRAY_FPTR, glDisableVertexAttribArray);
    GET_PROC(GL_VERTEX_ATTRIB_4F_FPTR,            glVertexAttrib4f);

    GET_PROC(GL_GET_STRING_I_FPTR,                glGetStringi);
}

#endif /* ] */

double __hwGlGetCurrTime( hwDisplay disp )
{
    glDisplay *gldisp = (glDisplay *)disp;

    return gldisp->glob.currTime;
}

double __hwGlGetStartTime( hwDisplay disp )
{
    glDisplay *gldisp = (glDisplay *)disp;

    return gldisp->glob.startTime;
}

double __hwGlGetElapsedTime( hwDisplay disp )
{
    glDisplay *gldisp = (glDisplay *)disp;

    return gldisp->glob.elapsedTime;
}

void __hwGlSetCurrTime( hwDisplay disp, double tm )
{
    glDisplay *gldisp = (glDisplay *)disp;

    gldisp->glob.currTime = tm;
}

void __hwGlSetStartTime( hwDisplay disp, double tm )
{
    glDisplay *gldisp = (glDisplay *)disp;

    gldisp->glob.startTime = tm;
}

void __hwGlSetElapsedTime( hwDisplay disp, double tm )
{
    glDisplay *gldisp = (glDisplay *)disp;

    gldisp->glob.elapsedTime = tm;
}

void __hwGlGetEnvVars( glDisplay *gldisp )
{
    gldisp->glob.doExtensions = 1;
    gldisp->glob.doPrune = 1;
    gldisp->glob.doVbo = 1;

    if( getenv( "HW_NO_EXTENSIONS" ) ) {
        gldisp->glob.doExtensions = 0;
    }
    if( getenv( "HW_DISABLE_VBO" ) ) {
        gldisp->glob.doVbo = 0;
    }
    if( getenv( "HW_DISABLE_PRUNE" ) ) {
        gldisp->glob.doPrune = 0;
    }
    if( getenv( "HW_SHOW_BOXES" ) ) {
        gldisp->glob.showBoxes = 1;
    }
    if( getenv( "HW_PRUNE_EYE" ) ) {
        gldisp->glob.pruneEye = 1;
    }
    if( getenv( "HW_DISABLE_CULL" ) ) {
        gldisp->glob.disableCull = 1;
    }
    if( getenv( "HW_DUMP_PROGS" ) ) {
        gldisp->glob.dumpProgs = 1;
    }
}

void __hwGlGetExtensions( glDisplay *gldisp, glDrawable *draw )
{
    const char *ver;
    int iVer;
    GLint maxTexNum;

    if( !gldisp->glob.doExtensions ) return;

    ver = (const char *)glGetString( GL_VERSION );
    if (!ver) return;

    iVer = 0;
    while (isdigit(*ver)) {
        iVer = 10*iVer + *ver - '0';
        ver++;
    }

    gldisp->glob.extensions = 0;

    switch (iVer) {
    case 0 :
        /* This should never happen */
        break;
    case 1 :
        gldisp->glob.extensions = HWGL_EXT_OPENGL_1_1;
        break;
    case 2 :
        gldisp->glob.extensions = HWGL_EXT_OPENGL_2_0;
        break;
    case 3 :
        gldisp->glob.extensions = HWGL_EXT_OPENGL_3_0;
        gldisp->glob.extensions |= HWGL_EXT_OPENGL_2_0;
        break;
    case 4 :
        gldisp->glob.extensions = HWGL_EXT_OPENGL_4_0;
        gldisp->glob.extensions |= HWGL_EXT_OPENGL_3_0;
        gldisp->glob.extensions |= HWGL_EXT_OPENGL_2_0;
        break;
    default :
        /* Assume that > 4 supports all the old things like other OGL
         * versions
         */
        gldisp->glob.extensions = HWGL_EXT_OPENGL_4_0;
        gldisp->glob.extensions |= HWGL_EXT_OPENGL_3_0;
        gldisp->glob.extensions |= HWGL_EXT_OPENGL_2_0;
        break;
    }

    draw->maxTexNum = 1;

#ifdef ANDROID_NDK
    /* Some days, those Khronos GLES guys really tick me off.  Really? */
    glGetIntegerv( GL_MAX_TEXTURE_IMAGE_UNITS, &maxTexNum );
#else
    glGetIntegerv( GL_MAX_TEXTURE_UNITS, &maxTexNum );
#endif
    if( maxTexNum > HW_MAX_TEXTURES ) {
        maxTexNum = HW_MAX_TEXTURES;
    }
    draw->maxTexNum = maxTexNum;
}

void __hwGlInitDrawableVars( glDrawable *draw )
{
    int i;

    hwGlInitProgState( &draw->progInfo );

    hwIdentity( draw->camMat );

    draw->numLights = 0;
    draw->camSet = 0;
    draw->lightOn = 1;
    draw->ambFact = 1.0;
    draw->stackDepth = 0;
    draw->planesValid = 0;
    draw->animMask = 0xFFFFFFFF;
    draw->renderMode = HW_RENDER_DEFAULT;
    draw->guiList = draw->guiTail = draw->guiFree = NULL;
    draw->enableBits = 0;
    draw->idxPool = NULL;
    draw->idxPoolSize = 0;
    draw->vtxPool = NULL;
    draw->vtxPoolSize = 0;

    for (i = 0; i < GUI_HT_SIZE; i++) {
        draw->guiListHash[i] = NULL;
    }
    draw->activeGuiList = NULL;

    __hwInitTextState( &draw->textState );
}

void __hwGlScissor( hwDisplay disp,
                    hwInt32 x, hwInt32 y, hwInt32 width, hwInt32 height )
{
    USE_GL_CTX(disp);

    glctx->scissor[0] = x;
    glctx->scissor[1] = y;
    glctx->scissor[2] = width;
    glctx->scissor[3] = height;

    if ((width == 0) || (height == 0)) {
        hwGlDisable(GL_SCISSOR_TEST);
    }
    else {
        glScissor( x, y, width, height );
        hwGlEnable(GL_SCISSOR_TEST);
    }
}

/*** EOF gl_visual.c ***/
