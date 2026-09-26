OBJ=Objs/
INC=../Inc/
OBJS=	$(OBJ)hwButton.o \
	$(OBJ)hwFont.o \
	$(OBJ)hwGauge.o \
        $(OBJ)hwGuiUtil.o \
	$(OBJ)hwJoystick.o \
	$(OBJ)hwLabel.o \
	$(OBJ)hwLayout.o \
	$(OBJ)hwRowCol.o \
	$(OBJ)hwToggle.o

HDRS=$(INC)hw_types.h $(INC)hw_strings.h $(INC)hw.h $(INC)hw_internal.h \
		$(INC)hw_os.h $(INC)hw_display.h

OPTG=-O +Olibcalls
CC=c89 -D_HPUX_SOURCE
CFLAGS=$(OPTG) -I/usr/include/X11R5 -I$(INC)

all:	../$(OBJ)Gui.o

../$(OBJ)Gui.o:	$(OBJS)
	ld -r $(OBJS) -o ../$(OBJ)Gui.o

$(OBJ)hwButton.o:		hwButton.c $(HDRS)
	$(CC) $(CFLAGS) hwButton.c -c -o $@

$(OBJ)hwFont.o:		hwFont.c $(HDRS)
	$(CC) $(CFLAGS) hwFont.c -c -o $@

$(OBJ)hwGauge.o:		hwGauge.c $(HDRS)
	$(CC) $(CFLAGS) hwGauge.c -c -o $@

$(OBJ)hwGuiUtil.o:		hwGuiUtil.c $(HDRS)
	$(CC) $(CFLAGS) hwGuiUtil.c -c -o $@

$(OBJ)hwJoystick.o:		hwJoystick.c $(HDRS)
	$(CC) $(CFLAGS) hwJoystick.c -c -o $@

$(OBJ)hwLabel.o:		hwLabel.c $(HDRS)
	$(CC) $(CFLAGS) hwLabel.c -c -o $@

$(OBJ)hwLayout.o:		hwLayout.c $(HDRS)
	$(CC) $(CFLAGS) hwLayout.c -c -o $@

$(OBJ)hwRowCol.o:		hwRowCol.c $(HDRS)
	$(CC) $(CFLAGS) hwRowCol.c -c -o $@

$(OBJ)hwToggle.o:		hwToggle.c $(HDRS)
	$(CC) $(CFLAGS) hwToggle.c -c -o $@
