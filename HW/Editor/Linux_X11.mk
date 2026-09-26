# SHELL=/bin/ksh
HWNULL=../libhwnull.a ../../JPEG/libjpeg.a

OGL_HOME=/opt/Mesa-3.0
HWGL=../libhwgl.a ../../JPEG/libjpeg.a
GLLIBS= $(HWGL)  -L$(OGL_HOME) -L/usr/X11R6/lib -lXm -lXt -lGLU -lGL -lm

INC=../Inc/
O=Objs/

CC=gcc 
CFLAGS=-g -I/usr/include/X11R6 -I$(INC) -I$(OGL_HOME)

GLPROGS=hwedit

OBJS=$(O)camera.o $(O)command.o $(O)main.o $(O)prop.o $(O)streams.o
HDRS=hwedit.h ../Inc/hw.h ../Inc/hw_types.h ../Inc/hw_display.h

all:	$(GLPROGS)

gl:	$(GLPROGS)

clean:
	rm $(GLPROGS)

hwedit:	$(OBJS) $(HWGL)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(GLLIBS)

$(O)camera.o:	camera.c $(HDRS)
	$(CC) $(CFLAGS) -c -o $@ camera.c

$(O)command.o:	command.c $(HDRS)
	$(CC) $(CFLAGS) -c -o $@ command.c

$(O)main.o:	main.c $(HDRS)
	$(CC) $(CFLAGS) -c -o $@ main.c

$(O)prop.o:	prop.c $(HDRS)
	$(CC) $(CFLAGS) -c -o $@ prop.c

$(O)streams.o:	streams.c $(HDRS)
	$(CC) $(CFLAGS) -c -o $@ streams.c
