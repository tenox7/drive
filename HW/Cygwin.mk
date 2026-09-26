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
##  Makefile for Cygwin gcc
##


OPTG    = -O
#OPTG    = -g
OBJ	= Objs/
#OBJ	= Objs_d/
#CFLAGS   = -DCYGWIN -D_WINDOWS -DWIN32 -DJPEGLIB -I../LIBPNG -IInc $(OPTG)
CFLAGS   = -DCYGWIN -D_WINDOWS -DWIN32 -DJPEGLIB -IInc -I../LIBPNG $(OPTG)
PNG     = ../LIBPNG/OBJ/readpng.o ../LIBPNG/OBJ/writepng.o

BASE= \
	$(OBJ)fontdata.o \
	$(OBJ)hwDisplay.o \
	$(OBJ)hwFileIO.o \
	$(OBJ)hwHash.o \
	$(OBJ)hwSelect.o \
	$(OBJ)hwShapes.o \
	$(OBJ)hwStrings.o \
	$(OBJ)hwUtils.o \
	$(OBJ)polytext.o \
	$(OBJ)ppm.o \
	$(OBJ)spline.o \
	$(OBJ)texfont.o

COMMON= \
	$(BASE) \
	$(OBJ)hwParse.o \
	$(OBJ)hwWrite.o

STUB= \
        $(OBJ)hwStubs.o

HWOBJ= \
	$(OBJ)hwBox.o \
	$(OBJ)hwCamera.o \
	$(OBJ)hwCone.o \
	$(OBJ)hwData.o \
	$(OBJ)hwDisc.o \
	$(OBJ)hwEnviron.o \
	$(OBJ)hwFile.o \
	$(OBJ)hwGroup.o \
	$(OBJ)hwImage.o \
	$(OBJ)hwLight.o \
	$(OBJ)hwMesh.o \
	$(OBJ)hwOrient.o \
	$(OBJ)hwPolygon.o \
	$(OBJ)hwPolyline.o \
	$(OBJ)hwPolymarker.o \
	$(OBJ)hwStrip.o \
	$(OBJ)hwQuads.o \
	$(OBJ)hwRing.o \
	$(OBJ)hwSphere.o \
	$(OBJ)hwSpinner.o \
	$(OBJ)hwSurface.o \
	$(OBJ)hwSurfRev.o \
	$(OBJ)hwSweep.o \
	$(OBJ)hwText.o \
	$(OBJ)hwText2D.o \
	$(OBJ)hwTexture.o \
	$(OBJ)hwTimer.o \
	$(OBJ)hwTorus.o \
	$(OBJ)hwTriangles.o

GUI=\
        $(OBJ)hwButton.o \
        $(OBJ)hwFont.o \
        $(OBJ)hwGauge.o \
        $(OBJ)hwGuiUtil.o \
        $(OBJ)hwJoystick.o \
        $(OBJ)hwLabel.o \
        $(OBJ)hwLayout.o \
        $(OBJ)hwRowCol.o \
        $(OBJ)hwToggle.o

DX= \
	$(OBJ)dx_camera.o \
	$(OBJ)dx_graph.o \
	$(OBJ)dx_list.o \
	$(OBJ)dx_visual.o

WGL= \
	$(OBJ)gl_camera.o \
	$(OBJ)gl_graph.o \
	$(OBJ)gl_visual.o \
	$(OBJ)win32_visual.o \
        $(OBJ)gl_list.o \
        $(OBJ)gl_program.o \
        $(OBJ)gl_state.o

GLUES= \
	$(OBJ)glues_mipmap.o \
	$(OBJ)glues_project.o

NULL= \
	$(OBJ)null.o

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

all             : libhwnull.a libhwgl.a libhwut.a

clean:
	rm Objs/*.o Objs_d/*.o *.a

libhwnull.a:	$(COMMON) $(HWOBJ) $(GUI) $(NULL)
	rm -f $@
	ar rv $@ $(COMMON) $(HWOBJ) $(GUI) $(NULL) $(PNG)

libhwgl.a:	$(COMMON) $(HWOBJ) $(GUI) $(WGL) $(GLUES) $(NULL)
	rm -f $@
	ar rv $@ $(COMMON) $(HWOBJ) $(GUI) $(WGL) $(GLUES) $(NULL) $(PNG)

libhwut.a:	$(BASE) $(STUB) $(WGL)
	rm -f $@
	ar rv $@ $(BASE) $(STUB) $(WGL) $(PNG)

### Common object files
$(OBJ)fontdata.o:	Common/fontdata.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Common/fontdata.c

$(OBJ)hwDisplay.o:	Common/hwDisplay.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Common/hwDisplay.c

$(OBJ)hwFileIO.o:	Common/hwFileIO.c		$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Common/hwFileIO.c

$(OBJ)hwHash.o:	Common/hwHash.c		$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Common/hwHash.c

$(OBJ)hwParse.o:	Common/hwParse.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Common/hwParse.c

$(OBJ)hwSelect.o:	Common/hwSelect.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Common/hwSelect.c

$(OBJ)hwShapes.o:	Common/hwShapes.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Common/hwShapes.c

$(OBJ)hwStrings.o:	Common/hwStrings.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Common/hwStrings.c

$(OBJ)hwStubs.o:	Common/hwStubs.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Common/hwStubs.c

$(OBJ)hwUtils.o:	Common/hwUtils.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Common/hwUtils.c

$(OBJ)hwWrite.o:	Common/hwWrite.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Common/hwWrite.c

$(OBJ)polytext.o:	Common/polytext.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Common/polytext.c

$(OBJ)ppm.o:		Common/ppm.c		$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Common/ppm.c

$(OBJ)spline.o:	Common/spline.c		$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Common/spline.c

$(OBJ)texfont.o:	Common/texfont.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Common/texfont.c

### HW object .o files
$(OBJ)hwBox.o:	Objects/hwBox.c		$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Objects/hwBox.c

$(OBJ)hwCamera.o:	Objects/hwCamera.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Objects/hwCamera.c

$(OBJ)hwCone.o:	Objects/hwCone.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Objects/hwCone.c

$(OBJ)hwData.o:	Objects/hwData.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Objects/hwData.c

$(OBJ)hwDisc.o:	Objects/hwDisc.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Objects/hwDisc.c

$(OBJ)hwEnviron.o:	Objects/hwEnviron.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Objects/hwEnviron.c

$(OBJ)hwFile.o:	Objects/hwFile.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Objects/hwFile.c

$(OBJ)hwGroup.o:	Objects/hwGroup.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Objects/hwGroup.c

$(OBJ)hwImage.o:	Objects/hwImage.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Objects/hwImage.c

$(OBJ)hwLight.o:	Objects/hwLight.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Objects/hwLight.c

$(OBJ)hwMesh.o:	Objects/hwMesh.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Objects/hwMesh.c

$(OBJ)hwOrient.o:	Objects/hwOrient.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Objects/hwOrient.c

$(OBJ)hwPolygon.o:	Objects/hwPolygon.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Objects/hwPolygon.c

$(OBJ)hwPolyline.o:	Objects/hwPolyline.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Objects/hwPolyline.c

$(OBJ)hwPolymarker.o:	Objects/hwPolymarker.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Objects/hwPolymarker.c

$(OBJ)hwStrip.o:	Objects/hwStrip.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Objects/hwStrip.c

$(OBJ)hwQuads.o:	Objects/hwQuads.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Objects/hwQuads.c

$(OBJ)hwRing.o:	Objects/hwRing.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Objects/hwRing.c

$(OBJ)hwSphere.o:	Objects/hwSphere.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Objects/hwSphere.c

$(OBJ)hwSpinner.o:	Objects/hwSpinner.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Objects/hwSpinner.c

$(OBJ)hwSurface.o:	Objects/hwSurface.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Objects/hwSurface.c

$(OBJ)hwSurfRev.o:	Objects/hwSurfRev.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Objects/hwSurfRev.c

$(OBJ)hwSweep.o:	Objects/hwSweep.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Objects/hwSweep.c

$(OBJ)hwText.o:	Objects/hwText.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Objects/hwText.c

$(OBJ)hwText2D.o:	Objects/hwText2D.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Objects/hwText2D.c

$(OBJ)hwTexture.o:	Objects/hwTexture.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Objects/hwTexture.c

$(OBJ)hwTimer.o:	Objects/hwTimer.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Objects/hwTimer.c

$(OBJ)hwTorus.o:	Objects/hwTorus.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Objects/hwTorus.c

$(OBJ)hwTriangles.o:	Objects/hwTriangles.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Objects/hwTriangles.c

### GUI object .o files
$(OBJ)hwButton.o:	GUI/hwButton.c		$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c GUI/hwButton.c
$(OBJ)hwFont.o:	GUI/hwFont.c		$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c GUI/hwFont.c
$(OBJ)hwGauge.o:	GUI/hwGauge.c		$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c GUI/hwGauge.c
$(OBJ)hwGuiUtil.o:	GUI/hwGuiUtil.c		$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c GUI/hwGuiUtil.c
$(OBJ)hwJoystick.o:	GUI/hwJoystick.c		$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c GUI/hwJoystick.c
$(OBJ)hwLabel.o:	GUI/hwLabel.c		$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c GUI/hwLabel.c
$(OBJ)hwLayout.o:	GUI/hwLayout.c		$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c GUI/hwLayout.c
$(OBJ)hwRowCol.o:	GUI/hwRowCol.c		$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c GUI/hwRowCol.c
$(OBJ)hwToggle.o:	GUI/hwToggle.c		$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c GUI/hwToggle.c

### WGL objects
$(OBJ)gl_camera.o:	GLES/gl_camera.c	$(HDRS) Inc/hw_gl.h
	$(CC) $(CFLAGS) -o $@ -c GLES/gl_camera.c

$(OBJ)gl_graph.o:	GLES/gl_graph.c	$(HDRS) Inc/hw_gl.h
	$(CC) $(CFLAGS) -o $@ -c GLES/gl_graph.c

$(OBJ)gl_list.o:	GLES/gl_list.c	$(HDRS) Inc/hw_gl.h
	$(CC) $(CFLAGS) -o $@ -c GLES/gl_list.c

$(OBJ)gl_program.o:	GLES/gl_program.c	$(HDRS) Inc/hw_gl.h
	$(CC) $(CFLAGS) -o $@ -c GLES/gl_program.c

$(OBJ)gl_state.o:	GLES/gl_state.c	$(HDRS) Inc/hw_gl.h
	$(CC) $(CFLAGS) -o $@ -c GLES/gl_state.c

$(OBJ)gl_visual.o:	GLES/gl_visual.c	$(HDRS) Inc/hw_gl.h
	$(CC) $(CFLAGS) -o $@ -c GLES/gl_visual.c

$(OBJ)win32_visual.o:	GLES/win32_visual.c	$(HDRS) Inc/hw_gl.h
	$(CC) $(CFLAGS) -o $@ -c GLES/win32_visual.c

### GLUES objects
$(OBJ)glues_mipmap.o: GLES/glues_mipmap.c
	$(CC) $(CFLAGS) -o $@ -c GLES/glues_mipmap.c

$(OBJ)glues_project.o:        GLES/glues_project.c
	$(CC) $(CFLAGS) -o $@ -c GLES/glues_project.c

### NULL objects
$(OBJ)null.o:		Null/null.c	$(HDRS)
	$(CC) $(CFLAGS) -o $@ -c Null/null.c


