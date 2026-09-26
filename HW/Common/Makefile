OBJ=Objs/
INC=../Inc/
OOBJS=	$(OBJ)hwParse.o \
	$(OBJ)hwWrite.o

SOBJS=	$(OBJ)hwStubs.o

OBJS=	$(OBJ)hwDisplay.o \
	$(OBJ)hwFileIO.o \
	$(OBJ)hwHash.o \
	$(OBJ)hwSelect.o \
	$(OBJ)hwShapes.o \
	$(OBJ)hwStrings.o \
	$(OBJ)hwUtils.o \
	$(OBJ)polytext.o \
	$(OBJ)fontdata.o \
	$(OBJ)ppm.o \
	$(OBJ)spline.o \
	$(OBJ)texfont.o

HDRS=$(INC)hw_types.h $(INC)hw_strings.h $(INC)hw.h $(INC)hw_internal.h \
		$(INC)hw_os.h $(INC)hw_display.h
JPEG=../../JPEG
JPEGDEF=-I$(JPEG) -DJPEGLIB

PNG=../../LIBPNG

OPTG=-O +Olibcalls
CC=c89 -D_HPUX_SOURCE
CFLAGS=$(OPTG) -I/usr/include/X11R5 -I$(INC) $(JPEGDEF) -I$(PNG)

all:	../$(OBJ)Common.o ../$(OBJ)Stubs.o

../$(OBJ)Common.o:	$(OBJS) $(OOBJS)
	ld -r $(OBJS) $(OOBJS) -o ../$(OBJ)Common.o

../$(OBJ)Stubs.o:	$(OBJS) $(SOBJS)
	ld -r $(OBJS) $(SOBJS) -o ../$(OBJ)Stubs.o

$(OBJ)hwDisplay.o:	hwDisplay.c $(HDRS)
	$(CC) $(CFLAGS) hwDisplay.c -c -o $@

$(OBJ)hwFileIO.o:	hwFileIO.c $(HDRS)
	$(CC) $(CFLAGS) hwFileIO.c -c -o $@

$(OBJ)hwHash.o:		hwHash.c $(HDRS)
	$(CC) $(CFLAGS) hwHash.c -c -o $@

$(OBJ)hwParse.o:	hwParse.c $(HDRS)
	$(CC) $(CFLAGS) hwParse.c -c -o $@

$(OBJ)hwSelect.o:	hwSelect.c $(HDRS)
	$(CC) $(CFLAGS) hwSelect.c -c -o $@

$(OBJ)hwShapes.o:	hwShapes.c $(HDRS)
	$(CC) $(CFLAGS) hwShapes.c -c -o $@

$(OBJ)hwStrings.o:	hwStrings.c $(HDRS)
	$(CC) $(CFLAGS) hwStrings.c -c -o $@

$(OBJ)hwStubs.o:	hwStubs.c $(HDRS)
	$(CC) $(CFLAGS) hwStubs.c -c -o $@

$(OBJ)hwUtils.o:	hwUtils.c $(HDRS)
	$(CC) $(CFLAGS) hwUtils.c -c -o $@

$(OBJ)hwWrite.o:	hwWrite.c $(HDRS)
	$(CC) $(CFLAGS) hwWrite.c -c -o $@

$(OBJ)fontdata.o:	fontdata.c $(HDRS) $(INC)fontdata.h
	$(CC) $(CFLAGS) fontdata.c -c -o $@

$(OBJ)polytext.o:	polytext.c $(HDRS) $(INC)fontdata.h
	$(CC) $(CFLAGS) polytext.c -c -o $@

$(OBJ)ppm.o:		ppm.c $(HDRS)
	$(CC) $(CFLAGS) ppm.c -c -o $@

$(OBJ)spline.o:		spline.c $(HDRS)
	$(CC) $(CFLAGS) spline.c -c -o $@

$(OBJ)texfont.o:	texfont.c $(HDRS)
	$(CC) $(CFLAGS) texfont.c -c -o $@
