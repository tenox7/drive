O=Objs/
OBJS=$(O)main.o $(O)object.o $(O)parse.o
CC=gcc
OPTG=-O
CFLAGS=$(OPTG) -I../Inc

../gfx2hw:	$(OBJS) ../libhwnull.a
	$(CC) $(CFLAGS) -o ../gfx2hw $(OBJS) ../libhwnull.a ../../JPEG/libjpeg.a -lm

$(O)main.o:	main.c	$(HDRS)
	$(CC) $(CFLAGS) -c -o $@ main.c

$(O)object.o:	object.c	$(HDRS)
	$(CC) $(CFLAGS) -c -o $@ object.c

$(O)parse.o:	parse.c	$(HDRS)
	$(CC) $(CFLAGS) -c -o $@ parse.c
