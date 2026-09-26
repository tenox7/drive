# (c) Copyright Hewlett-Packard Company 2001
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or (at
# your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

##
##  Makefile for Windows NT / Microsoft Visual C/C++
##


!include <win32.mak>


OPTG    = -O2
#OPTG    = -Zi
OBJ	= Objs/
#OBJ	= Objs_d/
#CFLAGS   = -D_WINDOWS -DWIN32 -DJPEGLIB -I../PNG -IInc -I../JPEG -nologo \
#		-ML -W1 $(OPTG) -GX -YX -Fo$(OBJ)
CFLAGS   = -D_WINDOWS -DGLFW -DWIN32 -DJPEGLIB -IInc -I../LIBPNG -I../JPEG \
		-I../glfw-3.3/include -nologo  -W1 $(OPTG) -Fo$(OBJ) -ICommon
LFLAGS	= -nologo
LIB32	= link.exe -lib
PNG     = ../LIBPNG/OBJ/readpng.obj ../LIBPNG/OBJ/writepng.obj

BASE= \
	$(OBJ)fontdata.obj \
	$(OBJ)hwDisplay.obj \
	$(OBJ)hwFileIO.obj \
	$(OBJ)hwHash.obj \
	$(OBJ)hwSelect.obj \
	$(OBJ)hwShapes.obj \
	$(OBJ)hwStrings.obj \
	$(OBJ)hwUtils.obj \
	$(OBJ)polytext.obj \
	$(OBJ)ppm.obj \
	$(OBJ)spline.obj \
	$(OBJ)texfont.obj

COMMON= \
	$(BASE) \
	$(OBJ)hwParse.obj \
	$(OBJ)hwWrite.obj

STUB= \
        $(OBJ)hwStubs.obj

HWOBJ= \
	$(OBJ)hwBox.obj \
	$(OBJ)hwCamera.obj \
	$(OBJ)hwCone.obj \
	$(OBJ)hwData.obj \
	$(OBJ)hwDisc.obj \
	$(OBJ)hwEnviron.obj \
	$(OBJ)hwFile.obj \
	$(OBJ)hwGroup.obj \
	$(OBJ)hwImage.obj \
	$(OBJ)hwLight.obj \
	$(OBJ)hwMesh.obj \
	$(OBJ)hwOrient.obj \
	$(OBJ)hwPolygon.obj \
	$(OBJ)hwPolyline.obj \
	$(OBJ)hwPolymarker.obj \
	$(OBJ)hwStrip.obj \
	$(OBJ)hwQuads.obj \
	$(OBJ)hwRing.obj \
	$(OBJ)hwSphere.obj \
	$(OBJ)hwSpinner.obj \
	$(OBJ)hwSurface.obj \
	$(OBJ)hwSurfRev.obj \
	$(OBJ)hwSweep.obj \
	$(OBJ)hwText.obj \
	$(OBJ)hwText2D.obj \
	$(OBJ)hwTexture.obj \
	$(OBJ)hwTimer.obj \
	$(OBJ)hwTorus.obj \
	$(OBJ)hwTriangles.obj

GUI=\
        $(OBJ)hwButton.obj \
        $(OBJ)hwFont.obj \
        $(OBJ)hwGauge.obj \
        $(OBJ)hwGuiUtil.obj \
        $(OBJ)hwJoystick.obj \
        $(OBJ)hwLabel.obj \
        $(OBJ)hwLayout.obj \
        $(OBJ)hwRowCol.obj \
        $(OBJ)hwToggle.obj

WGL= \
	$(OBJ)gl_camera.obj \
	$(OBJ)gl_graph.obj \
	$(OBJ)gl_visual.obj \
	$(OBJ)glfw_visual.obj \
        $(OBJ)gl_list.obj \
        $(OBJ)gl_program.obj \
        $(OBJ)gl_state.obj

GLUES= \
	$(OBJ)glues_mipmap.obj \
	$(OBJ)glues_project.obj

NULL= \
	$(OBJ)null.obj

HDRS= \
	Inc/fontdata.h \
	Inc/hw.h \
	Inc/hw_display.h \
	Inc/hw_internal.h \
	Inc/hw_os.h \
	Inc/hw_protos.h \
	Inc/hw_strings.h \
	Inc/hw_types.h \
	Inc/hw_vecmath.h

all             : hwnull.lib hw.lib hwut.lib

clean:
        del Objs\*.obj Objs_d\*.obj *.lib

hwnull.lib:	$(COMMON) $(HWOBJ) $(GUI) $(NULL)
	$(LIB32) @<<
    $(COMMON) $(HWOBJ) $(GUI) $(NULL) $(PNG) -out:hwnull.lib
<<

hw.lib:	$(COMMON) $(HWOBJ) $(GUI) $(WGL) $(GLUES) $(NULL)
	$(LIB32) @<<
    $(COMMON) $(HWOBJ) $(GUI) $(WGL) $(GLUES) $(NULL) $(PNG) -out:hw.lib
<<

hwut.lib:	$(BASE) $(STUB) $(WGL)
	$(LIB32) @<<
    $(BASE) $(STUB) $(WGL) $(PNG) -out:hwut.lib
<<

### Common object files
$(OBJ)fontdata.obj:	Common/fontdata.c	$(HDRS)
	$(CC) $(CFLAGS) -c Common/fontdata.c

$(OBJ)hwDisplay.obj:	Common/hwDisplay.c	$(HDRS)
	$(CC) $(CFLAGS) -c Common/hwDisplay.c

$(OBJ)hwFileIO.obj:	Common/hwFileIO.c		$(HDRS)
	$(CC) $(CFLAGS) -c Common/hwFileIO.c

$(OBJ)hwHash.obj:	Common/hwHash.c		$(HDRS)
	$(CC) $(CFLAGS) -c Common/hwHash.c

$(OBJ)hwParse.obj:	Common/hwParse.c	$(HDRS)
	$(CC) $(CFLAGS) -c Common/hwParse.c

$(OBJ)hwSelect.obj:	Common/hwSelect.c	$(HDRS)
	$(CC) $(CFLAGS) -c Common/hwSelect.c

$(OBJ)hwShapes.obj:	Common/hwShapes.c	$(HDRS)
	$(CC) $(CFLAGS) -c Common/hwShapes.c

$(OBJ)hwStrings.obj:	Common/hwStrings.c	$(HDRS)
	$(CC) $(CFLAGS) -c Common/hwStrings.c

$(OBJ)hwStubs.obj:	Common/hwStubs.c	$(HDRS)
	$(CC) $(CFLAGS) -c Common/hwStubs.c

$(OBJ)hwUtils.obj:	Common/hwUtils.c	$(HDRS)
	$(CC) $(CFLAGS) -c Common/hwUtils.c

$(OBJ)hwWrite.obj:	Common/hwWrite.c	$(HDRS)
	$(CC) $(CFLAGS) -c Common/hwWrite.c

$(OBJ)polytext.obj:	Common/polytext.c	$(HDRS)
	$(CC) $(CFLAGS) -c Common/polytext.c

$(OBJ)ppm.obj:		Common/ppm.c		$(HDRS)
	$(CC) $(CFLAGS) -c Common/ppm.c

$(OBJ)spline.obj:	Common/spline.c		$(HDRS)
	$(CC) $(CFLAGS) -c Common/spline.c

$(OBJ)texfont.obj:	Common/texfont.c	$(HDRS)
	$(CC) $(CFLAGS) -c Common/texfont.c

### HW object .obj files
$(OBJ)hwBox.obj:	Objects/hwBox.c		$(HDRS)
	$(CC) $(CFLAGS) -c Objects/hwBox.c

$(OBJ)hwCamera.obj:	Objects/hwCamera.c	$(HDRS)
	$(CC) $(CFLAGS) -c Objects/hwCamera.c

$(OBJ)hwCone.obj:	Objects/hwCone.c	$(HDRS)
	$(CC) $(CFLAGS) -c Objects/hwCone.c

$(OBJ)hwData.obj:	Objects/hwData.c	$(HDRS)
	$(CC) $(CFLAGS) -c Objects/hwData.c

$(OBJ)hwDisc.obj:	Objects/hwDisc.c	$(HDRS)
	$(CC) $(CFLAGS) -c Objects/hwDisc.c

$(OBJ)hwEnviron.obj:	Objects/hwEnviron.c	$(HDRS)
	$(CC) $(CFLAGS) -c Objects/hwEnviron.c

$(OBJ)hwFile.obj:	Objects/hwFile.c	$(HDRS)
	$(CC) $(CFLAGS) -c Objects/hwFile.c

$(OBJ)hwGroup.obj:	Objects/hwGroup.c	$(HDRS)
	$(CC) $(CFLAGS) -c Objects/hwGroup.c

$(OBJ)hwImage.obj:	Objects/hwImage.c	$(HDRS)
	$(CC) $(CFLAGS) -c Objects/hwImage.c

$(OBJ)hwLight.obj:	Objects/hwLight.c	$(HDRS)
	$(CC) $(CFLAGS) -c Objects/hwLight.c

$(OBJ)hwMesh.obj:	Objects/hwMesh.c	$(HDRS)
	$(CC) $(CFLAGS) -c Objects/hwMesh.c

$(OBJ)hwOrient.obj:	Objects/hwOrient.c	$(HDRS)
	$(CC) $(CFLAGS) -c Objects/hwOrient.c

$(OBJ)hwPolygon.obj:	Objects/hwPolygon.c	$(HDRS)
	$(CC) $(CFLAGS) -c Objects/hwPolygon.c

$(OBJ)hwPolyline.obj:	Objects/hwPolyline.c	$(HDRS)
	$(CC) $(CFLAGS) -c Objects/hwPolyline.c

$(OBJ)hwPolymarker.obj:	Objects/hwPolymarker.c	$(HDRS)
	$(CC) $(CFLAGS) -c Objects/hwPolymarker.c

$(OBJ)hwStrip.obj:	Objects/hwStrip.c	$(HDRS)
	$(CC) $(CFLAGS) -c Objects/hwStrip.c

$(OBJ)hwQuads.obj:	Objects/hwQuads.c	$(HDRS)
	$(CC) $(CFLAGS) -c Objects/hwQuads.c

$(OBJ)hwRing.obj:	Objects/hwRing.c	$(HDRS)
	$(CC) $(CFLAGS) -c Objects/hwRing.c

$(OBJ)hwSphere.obj:	Objects/hwSphere.c	$(HDRS)
	$(CC) $(CFLAGS) -c Objects/hwSphere.c

$(OBJ)hwSpinner.obj:	Objects/hwSpinner.c	$(HDRS)
	$(CC) $(CFLAGS) -c Objects/hwSpinner.c

$(OBJ)hwSurface.obj:	Objects/hwSurface.c	$(HDRS)
	$(CC) $(CFLAGS) -c Objects/hwSurface.c

$(OBJ)hwSurfRev.obj:	Objects/hwSurfRev.c	$(HDRS)
	$(CC) $(CFLAGS) -c Objects/hwSurfRev.c

$(OBJ)hwSweep.obj:	Objects/hwSweep.c	$(HDRS)
	$(CC) $(CFLAGS) -c Objects/hwSweep.c

$(OBJ)hwText.obj:	Objects/hwText.c	$(HDRS)
	$(CC) $(CFLAGS) -c Objects/hwText.c

$(OBJ)hwText2D.obj:	Objects/hwText2D.c	$(HDRS)
	$(CC) $(CFLAGS) -c Objects/hwText2D.c

$(OBJ)hwTexture.obj:	Objects/hwTexture.c	$(HDRS)
	$(CC) $(CFLAGS) -c Objects/hwTexture.c

$(OBJ)hwTimer.obj:	Objects/hwTimer.c	$(HDRS)
	$(CC) $(CFLAGS) -c Objects/hwTimer.c

$(OBJ)hwTorus.obj:	Objects/hwTorus.c	$(HDRS)
	$(CC) $(CFLAGS) -c Objects/hwTorus.c

$(OBJ)hwTriangles.obj:	Objects/hwTriangles.c	$(HDRS)
	$(CC) $(CFLAGS) -c Objects/hwTriangles.c

### GUI object .obj files
$(OBJ)hwButton.obj:	GUI/hwButton.c		$(HDRS)
	$(CC) $(CFLAGS) -c GUI/hwButton.c
$(OBJ)hwFont.obj:	GUI/hwFont.c		$(HDRS)
	$(CC) $(CFLAGS) -c GUI/hwFont.c
$(OBJ)hwGauge.obj:	GUI/hwGauge.c		$(HDRS)
	$(CC) $(CFLAGS) -c GUI/hwGauge.c
$(OBJ)hwGuiUtil.obj:	GUI/hwGuiUtil.c		$(HDRS)
	$(CC) $(CFLAGS) -c GUI/hwGuiUtil.c
$(OBJ)hwJoystick.obj:	GUI/hwJoystick.c		$(HDRS)
	$(CC) $(CFLAGS) -c GUI/hwJoystick.c
$(OBJ)hwLabel.obj:	GUI/hwLabel.c		$(HDRS)
	$(CC) $(CFLAGS) -c GUI/hwLabel.c
$(OBJ)hwLayout.obj:	GUI/hwLayout.c		$(HDRS)
	$(CC) $(CFLAGS) -c GUI/hwLayout.c
$(OBJ)hwRowCol.obj:	GUI/hwRowCol.c		$(HDRS)
	$(CC) $(CFLAGS) -c GUI/hwRowCol.c
$(OBJ)hwToggle.obj:	GUI/hwToggle.c		$(HDRS)
	$(CC) $(CFLAGS) -c GUI/hwToggle.c

### WGL objects
$(OBJ)gl_camera.obj:	GLES/gl_camera.c	$(HDRS) Inc/hw_gl.h
	$(CC) $(CFLAGS) -c GLES/gl_camera.c

$(OBJ)gl_graph.obj:	GLES/gl_graph.c	$(HDRS) Inc/hw_gl.h
	$(CC) $(CFLAGS) -c GLES/gl_graph.c

$(OBJ)gl_list.obj:	GLES/gl_list.c	$(HDRS) Inc/hw_gl.h
	$(CC) $(CFLAGS) -c GLES/gl_list.c

$(OBJ)gl_program.obj:	GLES/gl_program.c	$(HDRS) Inc/hw_gl.h
	$(CC) $(CFLAGS) -c GLES/gl_program.c

$(OBJ)gl_state.obj:	GLES/gl_state.c	$(HDRS) Inc/hw_gl.h
	$(CC) $(CFLAGS) -c GLES/gl_state.c

$(OBJ)gl_visual.obj:	GLES/gl_visual.c	$(HDRS) Inc/hw_gl.h
	$(CC) $(CFLAGS) -c GLES/gl_visual.c

$(OBJ)glfw_visual.obj:	GLES/glfw_visual.c	$(HDRS) Inc/hw_gl.h
	$(CC) $(CFLAGS) -c GLES/glfw_visual.c

### GLUES objects
$(OBJ)glues_mipmap.obj:	GLES/glues_mipmap.c
	$(CC) $(CFLAGS) -c GLES/glues_mipmap.c

$(OBJ)glues_project.obj:	GLES/glues_project.c
	$(CC) $(CFLAGS) -c GLES/glues_project.c

### NULL objects
$(OBJ)null.obj:		Null/null.c	$(HDRS)
	$(CC) $(CFLAGS) -c Null/null.c


