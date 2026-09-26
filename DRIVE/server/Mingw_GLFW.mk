OBJS=connection.o \
	parse_config.o \
	parse_scene.o \
	random_scene.o \
	read_scene.o \
	scene.o \
	server_interface.o \
	spline.o \
	surface_chars.o \
        textgad.o \
	town.o
OBJ1=$(OBJS) drive_server.o

LIBS=../server.a

#OPTG=-O2
OPTG=-g
LEGACY=-fcommon -std=gnu89 -Wno-implicit-int -Wno-implicit-function-declaration -Wno-return-type -Wno-int-conversion -Wno-incompatible-pointer-types
CC=gcc
CFLAGS=$(OPTG) -DGLFW -DMINGW -I../libnum -I../libphysics -I../include -I../../HW/Inc -I../libprims -I../../PDCURSES $(LEGACY)

all:	$(LIBS)

clean:
	rm -f *.o $(LIBS)

../server.a:     $(OBJ1)
	rm -f $@
	ar rv $@ $(OBJS)
