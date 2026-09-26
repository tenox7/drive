OBJS=camera.o \
	hwdash.o \
	clook.o \
	daylight.o \
	disp_ctrl.o \
	drive_msg.o \
	drive_sb.o \
	explosion.o \
	frame.o \
	headlight.o \
	joystick.o \
	parse_rcfile.o \
	sound.o \
	stars.o

OBJ1=$(OBJS) drive.o

LIBS=../client.a

#OPTG=-O2
OPTG=-g
LEGACY=-fcommon -std=gnu89 -Wno-implicit-int -Wno-implicit-function-declaration -Wno-return-type -Wno-int-conversion -Wno-incompatible-pointer-types
CC=gcc
CFLAGS=$(OPTG) -DGLFW -DMAC -I../server/objects -I../libnum -I../libphysics -I../include -I../../HW/Inc -I../libprims $(LEGACY)

all:	$(LIBS)

clean:
	rm -f *.o $(LIBS)

../client.a:     $(OBJ1)
	rm -f $@
	ar rv $@ $(OBJS)
