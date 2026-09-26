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
CFLAGS=$(OPTG) -m32 -DMAC -I../libnum -I../libphysics -I../include -I../../HW/Inc -I../libprims

all:	$(LIBS)

clean:
	rm -f *.o $(LIBS)

../server.a:     $(OBJ1)
	rm -f $@
	ar rv $@ $(OBJS)
