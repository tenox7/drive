OBJ=Objs/
INC=../Inc/
OBJS=	$(OBJ)hwBox.o \
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

HDRS=$(INC)hw_types.h $(INC)hw_strings.h $(INC)hw.h $(INC)hw_internal.h \
		$(INC)hw_os.h $(INC)hw_display.h

OPTG=-O +Olibcalls
CC=c89 -D_HPUX_SOURCE
CFLAGS=$(OPTG) -I/usr/include/X11R5 -I$(INC)

all:	../$(OBJ)Objects.o

../$(OBJ)Objects.o:	$(OBJS)
	ld -r $(OBJS) -o ../$(OBJ)Objects.o

$(OBJ)hwBox.o:		hwBox.c $(HDRS)
	$(CC) $(CFLAGS) hwBox.c -c -o $@

$(OBJ)hwCamera.o:	hwCamera.c $(HDRS)
	$(CC) $(CFLAGS) hwCamera.c -c -o $@

$(OBJ)hwCone.o:		hwCone.c $(HDRS)
	$(CC) $(CFLAGS) hwCone.c -c -o $@

$(OBJ)hwData.o:		hwData.c $(HDRS)
	$(CC) $(CFLAGS) hwData.c -c -o $@

$(OBJ)hwDisc.o:		hwDisc.c $(HDRS)
	$(CC) $(CFLAGS) hwDisc.c -c -o $@

$(OBJ)hwEnviron.o:	hwEnviron.c $(HDRS)
	$(CC) $(CFLAGS) hwEnviron.c -c -o $@

$(OBJ)hwFile.o:		hwFile.c $(HDRS)
	$(CC) $(CFLAGS) hwFile.c -c -o $@

$(OBJ)hwGroup.o:	hwGroup.c $(HDRS)
	$(CC) $(CFLAGS) hwGroup.c -c -o $@

$(OBJ)hwImage.o:	hwImage.c $(HDRS)
	$(CC) $(CFLAGS) hwImage.c -c -o $@

$(OBJ)hwLight.o:	hwLight.c $(HDRS)
	$(CC) $(CFLAGS) hwLight.c -c -o $@

$(OBJ)hwMesh.o:		hwMesh.c $(HDRS)
	$(CC) $(CFLAGS) hwMesh.c -c -o $@

$(OBJ)hwOrient.o:	hwOrient.c $(HDRS)
	$(CC) $(CFLAGS) hwOrient.c -c -o $@

$(OBJ)hwPolygon.o:	hwPolygon.c $(HDRS)
	$(CC) $(CFLAGS) hwPolygon.c -c -o $@

$(OBJ)hwPolyline.o:	hwPolyline.c $(HDRS)
	$(CC) $(CFLAGS) hwPolyline.c -c -o $@

$(OBJ)hwPolymarker.o:	hwPolymarker.c $(HDRS)
	$(CC) $(CFLAGS) hwPolymarker.c -c -o $@

$(OBJ)hwStrip.o:	hwStrip.c $(HDRS)
	$(CC) $(CFLAGS) hwStrip.c -c -o $@

$(OBJ)hwQuads.o:	hwQuads.c $(HDRS)
	$(CC) $(CFLAGS) hwQuads.c -c -o $@

$(OBJ)hwRing.o:		hwRing.c $(HDRS)
	$(CC) $(CFLAGS) hwRing.c -c -o $@

$(OBJ)hwSphere.o:	hwSphere.c $(HDRS)
	$(CC) $(CFLAGS) hwSphere.c -c -o $@

$(OBJ)hwSpinner.o:	hwSpinner.c $(HDRS)
	$(CC) $(CFLAGS) hwSpinner.c -c -o $@

$(OBJ)hwSurface.o:	hwSurface.c $(HDRS)
	$(CC) $(CFLAGS) hwSurface.c -c -o $@

$(OBJ)hwSurfRev.o:	hwSurfRev.c $(HDRS)
	$(CC) $(CFLAGS) hwSurfRev.c -c -o $@

$(OBJ)hwSweep.o:	hwSweep.c $(HDRS)
	$(CC) $(CFLAGS) hwSweep.c -c -o $@

$(OBJ)hwText.o:		hwText.c $(HDRS)
	$(CC) $(CFLAGS) hwText.c -c -o $@

$(OBJ)hwText2D.o:		hwText2D.c $(HDRS)
	$(CC) $(CFLAGS) hwText2D.c -c -o $@

$(OBJ)hwTexture.o:	hwTexture.c $(HDRS)
	$(CC) $(CFLAGS) hwTexture.c -c -o $@

$(OBJ)hwTimer.o:	hwTimer.c $(HDRS)
	$(CC) $(CFLAGS) hwTimer.c -c -o $@

$(OBJ)hwTorus.o:	hwTorus.c $(HDRS)
	$(CC) $(CFLAGS) hwTorus.c -c -o $@

$(OBJ)hwTriangles.o:	hwTriangles.c $(HDRS)
	$(CC) $(CFLAGS) hwTriangles.c -c -o $@
