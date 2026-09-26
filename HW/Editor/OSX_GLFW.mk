# SHELL=/bin/ksh
HWGL=../libhwgl.a ../../JPEG/libjpeg.a
GLLIBS= $(HWGL) -framework OpenGL -framework AGL -framework Cocoa -framework IOKit -framework CoreFoundation -framework CoreVideo -lm

INC=../Inc/
O=Objs/

CC=gcc
CFLAGS=-g -I$(INC)

OBJS=$(O)camera.o $(O)command.o $(O)ipc.o $(O)main.o $(O)prop.o $(O)streams.o
HDRS=hwedit.h ../Inc/hw.h ../Inc/hw_types.h ../Inc/hw_display.h

all:	hwedit cons

clean:
	rm $(GLPROGS)

hwedit:	$(OBJS) $(HWGL)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(GLLIBS)

cons: $(O)cons.o $(O)ipc.o
	$(CC) $(CFLAGS) -o $@ $(O)cons.o $(O)ipc.o

$(O)camera.o:	camera.c $(HDRS)
	$(CC) $(CFLAGS) -c -o $@ camera.c

$(O)command.o:	command.c $(HDRS)
	$(CC) $(CFLAGS) -c -o $@ command.c

$(O)cons.o:	cons.c $(HDRS)
	$(CC) $(CFLAGS) -c -o $@ cons.c

$(O)ipc.o:	ipc.c $(HDRS)
	$(CC) $(CFLAGS) -c -o $@ ipc.c

$(O)main.o:	main.c $(HDRS)
	$(CC) $(CFLAGS) -c -o $@ main.c

$(O)prop.o:	prop.c $(HDRS)
	$(CC) $(CFLAGS) -c -o $@ prop.c

$(O)streams.o:	streams.c $(HDRS)
	$(CC) $(CFLAGS) -c -o $@ streams.c
