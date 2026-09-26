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

/* This is the UNIX/X11 version of HW OS definitions.  First, the X11
 * header files.
 */

#if defined(MAC)                /* ] [ */

/* Ugly Mac hack... */
#if 1   
#define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER
#define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_6
#define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_6
#define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_6
#define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_6
#define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_6
#define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_6
#endif

#include <stdint.h>

#if defined(GLFW) /* [ */

typedef void *OS_DISPLAY_TYPE;
typedef void *OS_VISUAL_TYPE;
typedef void *OS_DRAWABLE_TYPE;
typedef void *OS_WINDOW_TYPE;

#else

#include <Carbon/carbon.h>
#include <AGL/agl.h>

typedef void *     OS_DISPLAY_TYPE;
typedef AGLContext OS_VISUAL_TYPE;
typedef WindowRef  OS_DRAWABLE_TYPE;
typedef WindowRef  OS_WINDOW_TYPE;

#endif /* ] */

#elif defined(WIN32) /* [ */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;

typedef void    *OS_DISPLAY_TYPE;
#if defined(GLFW)
typedef void *OS_VISUAL_TYPE;
typedef void *OS_DRAWABLE_TYPE;
typedef void *OS_WINDOW_TYPE;
#else
typedef int OS_VISUAL_TYPE;
typedef HANDLE  OS_DRAWABLE_TYPE;
typedef HWND  OS_WINDOW_TYPE;
#endif

#elif defined(USE_X11) /* ] [ */

#include <stdint.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

/* Now, the type definitions */
typedef Display *OS_DISPLAY_TYPE;
typedef XVisualInfo *OS_VISUAL_TYPE;
typedef Window OS_DRAWABLE_TYPE;
typedef Window   OS_WINDOW_TYPE;

#else /* ] [ */

#include <stdint.h>

typedef void *OS_DISPLAY_TYPE;
typedef void *OS_VISUAL_TYPE;
typedef void *OS_DRAWABLE_TYPE;
typedef void *OS_WINDOW_TYPE;

#endif  /* ] */

#ifdef ANDROID_NDK
    /* #define LOG_PROCEDURE_ENTRY */
  #include <android/log.h>

  #ifdef LOG_PROCEDURE_ENTRY
    #define LOG_ENTRY __android_log_print(ANDROID_LOG_INFO,"HB", "%s : %s  Entry",__FILE__, __func__);
  #else
    #define LOG_ENTRY 
  #endif

  #ifdef LOG_PROCEDURE_EXIT
    #define LOG_EXIT __android_log_print(ANDROID_LOG_INFO,"HB", "%s : %s  Exit",__FILE__, __func__);
  #else
    #define LOG_EXIT 
  #endif

#else
  #define LOG_ENTRY 
  #define LOG_EXIT 

#endif
