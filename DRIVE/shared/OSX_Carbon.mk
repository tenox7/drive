OBJS=dirent_win.o \
	dlist.o \
	fastmalloc.o \
	filenames.o \
	ipc.o \
	message.o \
	nquery.o \
	random.o \
	scanargs.o \
	utils.o \
	version.o

LIBS=../shared.a

#OPTG=-O2
OPTG=-g
CFLAGS=$(OPTG) -m32 -DMAC -I../libnum -I../libphysics -I../include -I../../HW/Inc -I../libprims

all:	$(LIBS)

clean:
	rm -f *.o $(LIBS)

../shared.a:     $(OBJS)
	rm -f $@
	ar rv $@ $(OBJS)
